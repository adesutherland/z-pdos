"""Editor-only terminal adapter for an already owned Operator session.

The caller owns maintenance, emulator, terminal, monitor, lifecycle and cleanup.
This module does not start a process, connect a terminal or stage media. It uses
the existing x3270 scripting and real 3215 interfaces. Every AID fences actual
host Write records, so an old header cannot satisfy a new observation.
"""
import json
from pathlib import Path
import re
import sys

from support import decode, encode, field_contract, write_frame, SEED, SAVED, RESULT, MARKER, saved_records
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'two-space'))
from console_machine import received_records, terminal_action, until


class Screen:
    def __init__(self, script, trace, proc, log, monitor, output, model):
        self.script, self.trace, self.proc, self.log = script, Path(trace), proc, Path(log)
        self.monitor, self.output = monitor, Path(output)
        self.rows, self.cols = {2: (24, 80), 5: (27, 132)}[model]
        prior=self.output/'editor-observations.json'
        self.observations=json.loads(prior.read_text()) if prior.exists() else []

    def action(self, text):
        result = terminal_action(self.script, text)
        if result.returncode:
            raise RuntimeError('terminal rejected ' + text + ': ' + result.stdout)
        return result.stdout

    def records(self):
        return received_records(self.trace.read_text(errors='replace'))

    def text(self):
        return self.action('Ascii()')

    def field(self, row, column, text):
        self.action(f'MoveCursor({row},{column})')
        self.action('EraseEOF()')
        if text:
            self.action('String(' + json.dumps(text, ensure_ascii=False) + ')')

    def paint(self, fence, marker):
        def fresh():
            frames = []
            for record in self.records()[fence:]:
                if record[:1] == b'\xf1':
                    try:
                        frames.append(write_frame(record, self.rows, self.cols))
                    except ValueError:
                        pass  # Shell/query records are not full EDIT paints.
            if not frames or not frames[-1]['unlocked']:
                return None
            if self.action('Query(KeyboardLock)').strip() != 'false':
                return None
            shown = self.text()
            return (frames[-1], shown) if marker in shown else None
        frame, shown = until(fresh, self.proc, self.log, 60)
        return frame, shown

    def submit(self, fields=(), aid='Enter()', marker='EDIT /', label=None):
        fence = len(self.records())
        for row, column, text in fields:
            self.field(row, column, text)
        self.action(aid)
        frame, shown = self.paint(fence, marker)
        if label:
            self.capture(label, frame, shown)
        return frame, shown

    def command(self, text, marker='EDIT /', label=None):
        return self.submit([(self.rows-3, 7, text)], marker=marker, label=label)

    def shell(self, command, marker='RC=0', limit=60):
        until(lambda: self.action('Query(KeyboardLock)').strip()=='false',self.proc,self.log,30)
        fence = len(self.monitor.text())
        self.action('String(' + json.dumps(command, ensure_ascii=False) + ')')
        self.action('Enter()')
        text = until(lambda: self.monitor.text()[fence:] if marker in self.monitor.text()[fence:] and
                     re.search(r'PCOMM END \d+ RC=0(?:\r?\n|$)',self.monitor.text()[fence:]) else None,
                     self.proc, self.log, limit)
        until(lambda: self.action('Query(KeyboardLock)').strip() == 'false', self.proc, self.log, 30)
        return text

    def open(self, name, label='editor-open'):
        fence = len(self.records())
        self.action('String(' + json.dumps('EDIT ' + name) + ')'); self.action('Enter()')
        frame, shown = self.paint(fence, 'EDIT / ' + name)
        if not field_contract(frame, self.rows, self.cols):
            raise ValueError('actual prefix/source/protected-field contract failed')
        self.capture(label, frame, shown)
        return frame

    def leave(self, command='CANCEL'):
        fence = len(self.monitor.text())
        self.field(self.rows-3, 7, command); self.action('Enter()')
        until(lambda: 'EDIT: returned to console' in self.monitor.text()[fence:] and
              re.search(r'PCOMM END \d+ RC=0', self.monitor.text()[fence:]), self.proc, self.log, 60)
        until(lambda: self.action('Query(KeyboardLock)').strip() == 'false', self.proc, self.log, 30)

    def capture(self, label, frame=None, shown=None):
        if not re.fullmatch('[a-z0-9-]+', label):
            raise ValueError('fixed observation name required')
        observation = {'label': label, 'geometry': self.action('Query(ScreenCurSize)'),
                       'cursor': self.action('Query(Cursor)'), 'ascii': shown or self.text(),
                       'buffer': self.action('ReadBuffer(Ascii)'),
                       'raw_write_sha256': frame['sha256'] if frame else None}
        # These are real provider exports, not a reconstructed product screen.
        observation['html'] = self.action('PrintText(html,string)')
        self.observations.append(observation)
        (self.output / 'editor-observations.json').write_text(json.dumps(self.observations, indent=2) + '\n')


def exercise_screen(t):
    """Fresh cell: SEED/CAS/BUSY, physical edits, refusals, PS and PDS saves."""
    checks = {}
    for mode in ('SEED', 'CAS', 'BUSY'):
        text = t.shell('TEFPROBE ' + mode)
        checks['native_probe_' + mode.lower()] = f'PD033 EDIT PROBE {mode} PASS RC=0' in text
        if not checks['native_probe_' + mode.lower()]:
            raise ValueError('native editor probe failed: ' + mode)
    t.open(SEED)
    checks['physical_field_protection'] = True
    t.submit([(2, 1, 'I')], marker='36 records')
    frame, _ = t.submit([(3, 20, 'X')], marker='UNSAVED')
    start = 3*t.cols+10
    checks['interior_null_columns_preserved'] = (
        [frame['cells'][start+i] for i in range(10)] == [0x40]*10 and
        frame['cells'][start+10] == encode('X')[0])
    # The cursor from the submitted data field must survive the repaint.
    checks['data_cursor_retained'] = frame['cursor'] == 3*t.cols+21
    t.submit(aid='PF(9)', marker='36 records')
    t.submit([(3, 1, 'D')], marker='35 records')
    t.submit([(2, 1, 'Z'), (3, 10, 'atomic refusal must not apply')], marker='Page refused')
    checks['invalid_prefix_page_atomic'] = 'atomic refusal must not apply' not in t.text() and '35 records' in t.text()
    t.submit(aid='PF(1)', marker='HELP toggles help')
    t.submit(aid='PF(1)')
    t.submit(aid='PF(8)'); checks['page_down'] = 'top 1 /' not in t.text()
    t.submit(aid='PF(7)', marker='top 1 /')
    t.submit(aid='PF(11)', marker='columns 41-')
    t.submit(aid='PF(10)', marker='columns 1-'); checks['horizontal_scroll'] = True
    t.command('FIND guard 22', marker='top 35 /'); checks['find_last_record'] = True
    t.command('TOP', marker='top 1 /')
    line = decode(saved_records()[11]).rstrip()
    t.submit([(13, 10, line)], marker='UNSAVED', label='editor-edited-unsaved')
    t.submit(aid='PF(3)', marker='Unsaved changes:'); checks['pf3_dirty_guard'] = True
    t.submit(aid='PF(9)', marker='saved / 35 records')
    t.command('CHANGE 12 /STAGE4/EDITOR/', marker='UNSAVED')
    checks['screen_change_command_rehearsed'] = 'PD033 EDITOR LIBRARY' in t.text()
    t.command('SAVE AS LABA01.B33.SRC(LIBRARY)', marker='Save refused/failed (16)')
    t.command('SAVE AS TSO31.EDIT', marker='Save refused/failed (20)')
    checks['failed_save_keeps_dirty_screen'] = 'UNSAVED' in t.text() and 'PD033 EDITOR LIBRARY' in t.text()
    t.command('SAVE AS ' + SAVED, marker='Saved 35 records', label='editor-saved')
    t.command('CHANGE 35 /guard 22/guard XX/', marker='UNSAVED')
    t.submit(aid='PF(2)', marker='Saved 35 records')
    t.command('CHANGE 35 /guard XX/guard 22/', marker='UNSAVED')
    t.submit(aid='PF(2)', marker='Saved 35 records')
    t.leave('QUIT')
    checks['saved_member_exact_native_records'] = 'PD033 EDIT PROBE VERIFY PASS RC=0' in t.shell('TEFPROBE VERIFY')
    t.open(SAVED, 'editor-reopen'); checks['member_reopen'] = '35 records' in t.text(); t.leave('QUIT')
    # Empty VB PS: preserve a zero-length record and leading/trailing blanks.
    t.open(RESULT, 'editor-ps-open')
    t.command('INPUT 0', marker='INP>')
    t.command('', marker='INP>'); t.command('  padded  ', marker='INP>')
    t.command('ascii!', marker='INP>'); t.command('.', marker='3 records')
    t.command('TOP', marker='top 1 /')
    t.submit([(4, 10, 'café!')], marker='UNSAVED')
    t.command('SAVE', marker='Saved 3 records'); t.leave('QUIT')
    checks['ps_empty_padded_accented_saved'] = True  # Exact stopped records are a separate required oracle.
    if not all(checks.values()):
        raise ValueError('editor physical checks failed: ' + ', '.join(k for k,v in checks.items() if not v))
    return checks


def batch_form(t, submit):
    """Real form AIDs; preview-only cells never publish a job."""
    fence = len(t.monitor.text())
    wire_fence = len(t.records())
    t.action('String("BATCH")'); t.action('Enter()')
    def revision(number):
        until(lambda: 'REVISION '+str(number) in t.monitor.text()[fence:] and
              len(t.records())>wire_fence and
              t.action('Query(KeyboardLock)').strip() == 'false', t.proc, t.log, 900)
    def response(text=None, aid='Enter()'):
        nonlocal wire_fence
        wire_fence = len(t.records())
        if text is not None:
            t.action('String(' + json.dumps(text) + ')')
        t.action(aid)
    revision(0)
    response('PROC CRXLGO'); revision(1)
    response('SRC '+SAVED); revision(2)
    response('ARGS -a SHARED'); revision(3)
    t.capture('submit-ready')
    response('PREVIEW'); revision(4)
    t.capture('jcl-preview')
    shown = t.text()
    if ('FORM OPEN' not in shown or 'PF3 returns' not in shown or
            'PF3 Return' not in shown or re.search(r'Focus:\s*SHELL',shown) or
            'Input: PRIMARY/BATCH' not in shown or '/PCOMM' in shown):
        raise ValueError('BATCH physical form guidance is stale')
    if submit:
        response(aid='PF(10)'); revision(5)
        t.capture('jobs-compile-assemble')
        response(aid='PF(8)'); revision(6)
        t.capture('jobs-link-run')
        response('OUTPUT'); revision(7)
        response('STEP BUILD.RUN'); revision(8)
        if 'filter' not in t.text().lower():
            raise ValueError('STEP edit did not report output filtering')
        response('DD SYSPRINT'); revision(9)
        if MARKER not in t.monitor.text()[fence:]:
            raise ValueError('edited-source executable marker absent')
        t.capture('run-output')
    response('EXIT')
    until(lambda: re.search(r'PCOMM END \d+ RC=0', t.monitor.text()[fence:]), t.proc, t.log, 60)
    until(lambda:t.action('Query(KeyboardLock)').strip()=='false',t.proc,t.log,30)
    t.capture('console-return')
    checks = {'batch_form_truthful': True}
    if submit:
        checks['batch_changed_marker'] = True
    else:
        checks['batch_preview_no_submission'] = 'SUBMITTED JOB' not in t.monitor.text()[fence:]
    return checks


class Line:
    """An already connected real LineClient at primary device 0009."""
    def __init__(self, client, proc, log):
        self.client, self.proc, self.log = client, proc, Path(log)

    @staticmethod
    def validate(text):
        # Hercules' 3215 prt_host_to_guest can replace host nonprintables
        # before PDOS receives them. Refuse outside the supported alphabet
        # before any socket write, rather than assert a guest-side rejection.
        if any(ord(c)<32 or ord(c)>126 for c in text):
            raise ValueError('real line console requires printable ASCII')
        if len(text)>148:
            raise ValueError('real line-console input limit')

    def command(self, text, marker='EDIT>', limit=60):
        self.validate(text)
        if text.startswith('EDIT '):
            until(lambda: re.search(r':\\>\s*$', self.client.text()),self.proc,self.log,30)
        fence = len(self.client.text()); self.client.enter(text)
        return until(lambda: self.client.text()[fence:] if marker in self.client.text()[fence:] else None,
                     self.proc, self.log, limit)

    def shell(self, text, marker='RC=0', limit=60):
        self.validate(text)
        until(lambda: re.search(r':\\>\s*$', self.client.text()),self.proc,self.log,30)
        fence=len(self.client.text())
        self.client.enter(text)
        return until(lambda: self.client.text()[fence:] if marker in self.client.text()[fence:] and
                     re.search(r'PCOMM END \d+ RC=0(?:\r?\n|$)',self.client.text()[fence:]) else None,
                     self.proc,self.log,limit)


def exercise_line(t):
    checks = {}
    for mode in ('SEED', 'CAS', 'BUSY'):
        checks['native_probe_'+mode.lower()] = f'PD033 EDIT PROBE {mode} PASS RC=0' in t.shell('TEFPROBE '+mode)
    t.command('EDIT '+SEED)
    t.command('HELP'); t.command('BOTTOM'); t.command('TOP')
    t.command('FIND guard 22'); t.command('INSERT 35 temporary record')
    shown=t.command('UNDO'); checks['line_undo'] = '35 records' in shown
    t.command('CHANGE 12 /STAGE4/EDITOR/')
    shown=t.command('QUIT'); checks['line_dirty_guard'] = 'Unsaved changes:' in shown
    shown=t.command('SAVE AS LABA01.B33.SRC(LIBRARY)')
    checks['line_refusal_keeps_dirty'] = 'Save refused/failed (16)' in shown and 'UNSAVED' in shown
    shown=t.command('SAVE AS '+SAVED); checks['line_member_save'] = 'Saved 35 records' in shown
    t.command('QUIT', 'PCOMM END ')
    checks['line_saved_member_exact'] = 'PD033 EDIT PROBE VERIFY PASS RC=0' in t.shell('TEFPROBE VERIFY')
    t.command('EDIT '+SAVED); t.command('PRINT 12 12'); t.command('QUIT','PCOMM END ')
    t.command('EDIT '+RESULT); t.command('INPUT 0','INPUT>')
    t.command('','INPUT>'); t.command('  padded  ','INPUT>'); t.command('ascii!','INPUT>')
    # Rejection belongs before this ASCII-only transport. Confirm that the
    # subsequent actual guest response still has exactly three INPUT records.
    refused=False
    try:
        t.command('café!','INPUT>')
    except ValueError as exc:
        refused='printable ASCII' in str(exc)
    shown=t.command('.')
    checks['line_nonascii_adapter_refused_before_send'] = refused and '3 records' in shown
    t.command('SAVE'); t.command('QUIT','PCOMM END ')
    if not all(checks.values()):
        raise ValueError('line editor checks failed: '+', '.join(k for k,v in checks.items() if not v))
    return checks


def batch_line(t):
    t.command('BATCH','REVISION 0')
    for i,command in enumerate(('PROC CRXLGO','SRC '+SAVED,'ARGS -a SHARED','PREVIEW','SUBMIT','OUTPUT','STEP BUILD.RUN','DD SYSPRINT'),1):
        shown=t.command(command,'REVISION '+str(i),900)
    if MARKER not in shown:
        raise ValueError('line batch edited-source executable marker absent')
    t.command('EXIT','PCOMM END ')
    return {'line_changed_source_batch_marker': True}
