"""Bounded editor terminal/binary adapter; Operator cREXX recipe owns dispatch.

One existing disposable candidate, one maintained QA slot, no managed guest
changes. Refuses to act without a reviewed freeze/GO and matching maintenance
receipt. Lifecycle primitives come from the existing console-machine adapter.
"""
import argparse
import hashlib
import json
from pathlib import Path
import queue
import re
import shutil
import socket
import struct
import subprocess
import sys
import threading
from types import SimpleNamespace

from payload import ROOT, envelope
from support import SAVED, store_checks, batch_checks, owner_checks, owner_cards
from terminal import Screen, Line, exercise_screen, exercise_line, batch_form, batch_line
from console_machine import (REAL, LineClient, start_terminal, close_terminal, until,
                             recover_shutdown, stop_for_capture, parse_transcript)
from ipl_machine import unused_loopback_port, drain_output
sys.path.insert(0,str(ROOT/'pdos/tests/batch'))
from job_actor import prepare as devices, _commands, close as close_devices, snapshot, decode as decode_spool
from store_check import inspect


def digest(path):
    h=hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda:stream.read(1024*1024),b''):
            h.update(block)
    return h.hexdigest()


def expected_keys(cell):
    keys=set('adapter_completed emulator_exit_zero terminal_child_exited terminal_io_threads_exited listener_sockets_closed output_drain_exited normal_shutdown native_final_status owners_released invocation_depth_zero actual_geometry expected_check_keys'.split())
    keys.update('editor_named_outputs_exact editor_outside_store_unchanged editor_no_temporary_outputs editor_exact_seed editor_exact_saved editor_exact_ps_records'.split())
    if cell=='restart':
        keys.update('restart_member_native_exact restart_no_auto_execution restart_owner_reviewed_and_purged batch_form_truthful batch_changed_marker'.split())
    else:
        keys.update('native_probe_seed native_probe_cas native_probe_busy'.split())
        if cell=='line':
            keys.update('line_undo line_dirty_guard line_refusal_keeps_dirty line_member_save line_saved_member_exact line_nonascii_adapter_refused_before_send line_changed_source_batch_marker'.split())
        else:
            keys.update('physical_field_protection interior_null_columns_preserved data_cursor_retained invalid_prefix_page_atomic page_down horizontal_scroll find_last_record pf3_dirty_guard screen_change_command_rehearsed failed_save_keeps_dirty_screen saved_member_exact_native_records member_reopen ps_empty_padded_accented_saved batch_form_truthful batch_preview_no_submission'.split())
    if cell=='mono':keys.add('clean_primary_reconnect_saved_member')
    if cell=='model2':
        keys.update('owner_job_retained owner_native_os_rc_zero owner_device_open_refused20'.split())
    if cell=='restart':
        keys.update('restart_owner_job_retained restart_owner_native_os_rc_zero restart_owner_device_open_refused20'.split())
    if cell=='model2':keys.add('owner_real_reader_submitted')
    if cell in ('line','restart'):
        keys.update('editor_job_identity_complete editor_four_native_step_rc_zero editor_four_native_step_os_zero editor_exact_executed_marker'.split())
    return keys


def authority(freeze_path,lease_path,hercules):
    private=Path('/Users/adrian/MainframeLab/private')
    for path in (freeze_path,lease_path):
        if not path.resolve().is_relative_to(private) or path.is_symlink() or path.stat().st_mode & 0o077:
            raise ValueError('mode0600 private freeze and maintenance receipts required')
    freeze=json.loads(freeze_path.read_text()); lease=json.loads(lease_path.read_text())
    if freeze.get('review_complete') is not True or freeze.get('guest_go') is not True:
        raise ValueError('reviewed final freeze and explicit guest GO required')
    ledger=json.loads((private/'leases.json').read_text())
    actual=ledger['leases'].get('pdos:maintenance',{})
    if (not actual or lease.get('token')!=actual.get('token') or
            lease.get('owner')!=actual.get('owner') or freeze.get('owner')!=actual.get('owner')):
        raise ValueError('exact live Operator maintenance receipt required')
    if ledger.get('processes',{}).get('pdos') or any(k.startswith('pdos:') and k!='pdos:maintenance' for k in ledger['leases']):
        raise ValueError('PDOS fleet resources must be handed back first')
    files=freeze.get('files',{})
    required={str(p.resolve()) for p in (ROOT/'pdos/tests/editor').iterdir() if p.is_file()}
    required.update(str(ROOT/p) for p in ('pdos/scripts/editor-probe.crexx','pdos/scripts/editor-qa.crexx',
        'pdos/scripts/batch-sdk-media.crexx','build/pdos/batch-sdk/core/run/image.core',
        'build/pdos/batch-sdk/command/u-command.bin','build/pdos/batch-sdk/apps/edit.bin',
        'build/pdos/batch-sdk/apps/editor-probe/probe.bin'))
    required.add(str(hercules))
    if not required<=set(files):
        raise ValueError('freeze omits current fixture/product/emulator inputs')
    for name,expected in files.items():
        path=Path(name)
        if not path.is_absolute() or path.is_symlink() or digest(path)!=expected:
            raise ValueError('freeze input changed: '+name)
    return freeze


def run(a):
    herc=Path(a.hercules).resolve()
    authority(Path(a.freeze),Path(a.lease),herc)
    image=ROOT/'build/pdos/batch-sdk/image'
    disk=image/'media/pdos00.cckd'
    out=ROOT/'build/pdos/batch-sdk/qa'
    if out.exists() or disk.is_symlink():
        raise ValueError('fresh owned QA slot and fixed candidate required')
    held=subprocess.run(['/usr/sbin/lsof','-t',str(disk)],capture_output=True,text=True)
    if held.returncode!=1:
        raise ValueError('candidate has an active consumer or lsof failed')
    if shutil.disk_usage(ROOT).free<2*1024**3:
        raise ValueError('need2GiB for bounded core/flat adapter workspace')
    expected=json.loads((image/'editor-readback.json').read_text())
    if not expected['pass'] or expected['sha256']!=envelope((ROOT/'build/pdos/batch-sdk/apps/edit.bin').read_bytes())['sha256']:
        raise ValueError('current staged EDIT readback required')
    probe=ROOT/'build/pdos/batch-sdk/apps/editor-probe/probe.bin'
    envelope(probe.read_bytes())
    out.mkdir();(out/'.pd033-build').write_text('pd033-batch-build-v1\n')
    port=unused_loopback_port();script=unused_loopback_port()
    script_ports=[script]
    if port==script:raise ValueError('independent listener/script ports required')
    line=a.cell=='line';model=5 if a.cell=='model5' else 2
    appearance=a.cell!='mono'
    profile=SimpleNamespace(model=model,colour=appearance,oversize=None,basic=False,stage3=None)
    primary='3215 noprompt' if line else '3270'
    cfg='ARCHLVL ESAME\nMAINSIZE 512\nNUMCPU 1\nCPUMODEL 2064\nDIAG8CMD DISABLE\nSHCMDOPT DISABLE\nECPSVM NO\nCODEPAGE 819/1047\nLEGACYSENSEID ON\n'
    cfg+=f'CNSLPORT 127.0.0.1:{port}\n01B9 3390 {disk}\n0009 {primary}\n000A 3215 noprompt\n'+devices(out)
    (out/'machine.cnf').write_text(cfg);(out/'run.rc').write_text('sysclear\n')
    log=out/'console.log';core=out/'result.core'
    proc=subprocess.Popen([str(herc),'-t','-f',str(out/'machine.cnf'),'-o',str(log),'-r',str(out/'run.rc')],
                          cwd=out,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,bufsize=1)
    lines=[];events=queue.Queue();drain=threading.Thread(target=drain_output,args=(proc.stdout,lines,events),daemon=True);drain.start()
    terminal=client=monitor=None;error=None;checks={};monitor_text='';primary_text=''
    try:
        def listener():
            try:
                with socket.create_connection(('127.0.0.1',port),timeout=.2):return True
            except OSError:return False
        until(listener,proc,log,10)
        if line:client=LineClient(port,9)
        else:terminal=start_terminal(profile,out,proc,log,port,script,'3270.trace')
        monitor=LineClient(port,10)
        until(lambda:all(x in log.read_text(errors='replace') for x in ('0:0009 COMM: client','0:000A COMM: client')),proc,log,20)
        proc.stdin.write('ipl 01B9\n');proc.stdin.flush()
        until(lambda:'welcome to pcomm' in monitor.text(),proc,log,60)
        if line:
            actor=Line(client,proc,log);checks.update(exercise_line(actor));checks.update(batch_line(actor))
        else:
            actor=Screen(script,out/'3270.trace',proc,log,monitor,out,model)
            until(lambda:actor.action('Query(KeyboardLock)').strip()=='false',proc,log,30)
            if a.cell=='restart':
                actor.shell('JOBS','JOB 1')
                small=out/'restart-before-purge-header.bin';body=out/'restart-before-purge-spool.bin'
                stop_for_capture(proc,log)
                try:
                    _commands(proc,log,out,'editor-restart-header',[f'savecore "{small}" 3ba00 3ba23'])
                    words=struct.unpack('>9I',small.read_bytes());real=words[7]
                    if words[0]!=0x50444a32 or not 0x1000000<=real<=REAL-294912:
                        raise ValueError('restart native job snapshot absent')
                    _commands(proc,log,out,'editor-restart-spool',[f'savecore "{body}" {real:x} {real+294912-1:x}'])
                    spool=decode_spool(body.read_bytes())
                    checks.update({'restart_'+key:value for key,value in owner_checks(spool).items()})
                    checks['restart_no_auto_execution']=spool['last_id']==1 and spool['active_id']==0 and len(spool['jobs'])==1
                finally:
                    _commands(proc,log,out,'editor-restart-resume',['startall'])
                checks['restart_member_native_exact']='PD033 EDIT PROBE VERIFY PASS RC=0' in actor.shell('TEFPROBE VERIFY')
                actor.open(SAVED,'editor-after-restart');actor.leave('QUIT')
                actor.shell('STATUS 1','STEP RUN STATE=2 OS=0 RC=0')
                actor.shell('OUTPUT 1 STEP=RUN DD=SYSPRINT','PD033 EDIT BATCH OWNER REFUSED 20')
                actor.shell('PURGE 1')
                checks['restart_owner_reviewed_and_purged']=True
                checks.update(batch_form(actor,True))
            else:
                checks.update(exercise_screen(actor))
                if a.cell=='model2':
                    deck=ROOT/'build/pdos/batch-sdk/image-inputs/media/EDITOR-OWNER.fb80'
                    if deck.read_bytes()!=owner_cards():raise ValueError('exact owned reader deck required')
                    _commands(proc,log,out,'editor-owner-reader',[f'devinit 000C {deck} eof ebcdic'])
                    actor.shell('SUBMIT READER HOLD','SUBMITTED JOB 1',900)
                    actor.shell('STATUS 1','STEP RUN STATE=2 OS=0 RC=0')
                    actor.shell('OUTPUT 1 STEP=RUN DD=SYSPRINT','PD033 EDIT BATCH OWNER REFUSED 20')
                    checks['owner_real_reader_submitted']=True
                checks.update(batch_form(actor,False))
            if a.cell=='mono':
                # Saved clean session: controller shutdown/reconnect/reopen.
                close_terminal(terminal,script);terminal=None
                script=unused_loopback_port()
                script_ports.append(script)
                terminal=start_terminal(profile,out,proc,log,port,script,'3270-reconnect.trace')
                actor=Screen(script,out/'3270-reconnect.trace',proc,log,monitor,out,model)
                actor.action('Clear()')
                until(lambda:actor.action('Query(KeyboardLock)').strip()=='false',proc,log,30)
                actor.open(SAVED,'editor-reconnected');actor.leave('QUIT')
                checks['clean_primary_reconnect_saved_member']=True
        recover_shutdown(proc,log,client,script)
    except (OSError,RuntimeError,TimeoutError,ValueError,KeyError,subprocess.SubprocessError) as exc:
        error=str(exc)
    finally:
        # Export failure must never bypass independent process/controller stop.
        try:close_devices()
        except Exception as exc:error=(error or '')+'; device close: '+str(exc)
        if error and proc.poll() is None:
            try:recover_shutdown(proc,log,client,script)
            except Exception as exc:error+='; native recovery: '+str(exc)
        if proc.poll() is None:
            try:
                stop_for_capture(proc,log)
                _commands(proc,log,out,'editor-capture',[f'savecore "{core}" 0 {REAL-1:x}'])
            except Exception as exc:error=(error or '')+'; capture: '+str(exc)
            finally:
                try:proc.stdin.write('quit\n');proc.stdin.flush()
                except OSError:pass
                try:proc.wait(timeout=30)
                except subprocess.TimeoutExpired:
                    proc.terminate()
                    try:proc.wait(timeout=10)
                    except subprocess.TimeoutExpired:proc.kill();proc.wait(timeout=10)
        if terminal:
            try:close_terminal(terminal,script)
            except Exception as exc:error=(error or '')+'; terminal close: '+str(exc)
        for owned,kind in ((client,'primary'),(monitor,'monitor')):
            if owned:
                try:
                    if kind=='primary':primary_text=owned.text()
                    else:monitor_text=owned.text()
                except Exception as exc:error=(error or '')+'; '+kind+' export: '+str(exc)
                finally:
                    try:owned.close()
                    except Exception as exc:error=(error or '')+'; '+kind+' close: '+str(exc)
        drain.join(timeout=2)
        proc.stdin.close();proc.stdout.close()
    def listening(number):
        with socket.socket() as connection:
            connection.settimeout(.2)
            return connection.connect_ex(('127.0.0.1',number))==0
    checks.update(adapter_completed=error is None,emulator_exit_zero=proc.returncode==0,
                  terminal_child_exited=terminal is None or terminal.poll() is not None,
                  terminal_io_threads_exited=all(not owned.thread.is_alive() for owned in (client,monitor) if owned),
                  listener_sockets_closed=not any(listening(number) for number in [port]+script_ports),
                  output_drain_exited=not drain.is_alive())
    try:
        raw=core.read_bytes()
        if len(raw)!=REAL:raise ValueError('complete stopped core absent')
        transcript=parse_transcript(raw)
        checks.update(normal_shutdown=struct.unpack_from('>Q',raw,0x2010)[0]==1 and struct.unpack_from('>2I',raw,0x2018)==(0,0),
                      native_final_status=struct.unpack_from('>2I',raw,0x4f5c)==(1,0),
                      owners_released=transcript['read_owner']==transcript['screen_owner']==0 and struct.unpack_from('>3I',raw,0x4fb4)==(0,0,0),
                      invocation_depth_zero=struct.unpack_from('>I',raw,0x3c800)[0]==0x4d455431 and struct.unpack_from('>8I',raw,0x3c800)[7]==0,
                      actual_geometry=(transcript['rows'],transcript['columns'])==((0,0) if line else (24,80) if model==2 else (27,132)))
        compiled=a.cell in ('line','restart')
        if compiled:checks.update(batch_checks(raw,2 if a.cell=='restart' else 1))
        if a.cell=='model2':
            words=struct.unpack_from('>9I',raw,0x3ba00)
            checks.update(owner_checks(decode_spool(snapshot(raw,words[7]))))
        flat=out/'readback.ckd'
        try:
            subprocess.run([str(herc.parent/'cckd2ckd'),'-q','-cyls','150',str(disk),str(flat)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
            after=inspect(flat);before=json.loads((image/'store-before.json').read_text())
            checks.update(store_checks(before,after,line))
            (out/'store-after.json').write_text(json.dumps(after,indent=2)+'\n')
        finally:flat.unlink(missing_ok=True)
    except Exception as exc:
        checks['complete_stopped_oracles']=False;error=(error or '')+'; stopped oracle: '+str(exc)
    expected=expected_keys(a.cell)
    checks['expected_check_keys']=set(checks)|{'expected_check_keys'}==expected
    (out/'monitor.txt').write_text(monitor_text);(out/'primary.txt').write_text(primary_text)
    report={'pass':bool(checks) and all(checks.values()),'cell':a.cell,'checks':checks,'error':error,
            'disk':str(disk),'post_disk_sha256':digest(disk),'ports':[port]+script_ports,
            'missing_checks':sorted(expected-set(checks)),'unexpected_checks':sorted(set(checks)-expected),
            'limits':['Raw EDIT records do not establish complete semantic capture.','No physical-I/O fault or power-loss injection.']}
    (out/'receipt.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'pass':report['pass'],'failed':[k for k,v in checks.items() if not v],'error':error}))
    return 0 if report['pass'] else 1


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('cell',choices=('model2','restart','model5','mono','line'))
    p.add_argument('hercules');p.add_argument('freeze');p.add_argument('lease')
    raise SystemExit(run(p.parse_args()))
