#!/usr/bin/env python3
"""Bounded CKD and IBM-1047 interface for the cREXX conformance recipe.

Lifecycle and Hercules commands belong to conformance.crexx. This helper only
reads a stopped 100-cylinder base disk, writes checked dasdload inputs, and
installs a native U/18452 batch record into a new flat disk.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re

HEADER, TRACK, HEADS, CYLS = 512, 56832, 15, 100
DISK_SIZE = HEADER + TRACK * HEADS * CYLS
BASE_NAMES = ('PLOAD.SYS', 'PDOS.SYS', 'CONFIG.SYS', 'COMMAND.EXE')
BASE_FILES = {'PLOAD.SYS': 'pload.sys', 'PDOS.SYS': 'pdos.sys',
              'CONFIG.SYS': 'config.sys', 'COMMAND.EXE': 'pcomm.exe'}
NAME = re.compile(r'[A-Z][A-Z0-9]{0,7}')
DATASET = re.compile(r'[A-Z#$@][A-Z0-9#$@.]{0,43}')
SHA = re.compile(r'[0-9a-f]{64}')


def digest(data):
    return hashlib.sha256(data).hexdigest()


def records(disk, cyl, head):
    start = HEADER + (cyl * HEADS + head) * TRACK
    track = disk[start:start + TRACK]
    if track[:5] != b'\0' + cyl.to_bytes(2, 'big') + head.to_bytes(2, 'big'):
        raise ValueError('invalid CKD track header')
    result, at = {}, 5
    while at + 8 <= TRACK:
        count = track[at:at + 8]
        if count == b'\xff' * 8:
            break
        number, keylen = count[4], count[5]
        size = int.from_bytes(count[6:8], 'big')
        end = at + 8 + keylen + size
        if count[:4] != cyl.to_bytes(2, 'big') + head.to_bytes(2, 'big') or end > TRACK or number in result:
            raise ValueError('invalid CKD record')
        result[number] = (bytes(track[at + 8:at + 8 + keylen]),
                          bytes(track[at + 8 + keylen:end]))
        at = end
    return result


def write_track(disk, cyl, head, values):
    track = bytearray(b'\0' + cyl.to_bytes(2, 'big') + head.to_bytes(2, 'big'))
    for number, (key, data) in sorted(values.items()):
        track += (cyl.to_bytes(2, 'big') + head.to_bytes(2, 'big')
                  + bytes((number, len(key))) + len(data).to_bytes(2, 'big')
                  + key + data)
    track += b'\xff' * 8
    if len(track) > TRACK:
        raise ValueError('CKD track capacity exceeded')
    start = HEADER + (cyl * HEADS + head) * TRACK
    disk[start:start + TRACK] = track.ljust(TRACK, b'\0')


def dscb(disk, name):
    wanted = name.encode('cp037').ljust(44, b'\x40')
    for cyl in (1, 2):
        for head in range(HEADS):
            for number, (key, data) in records(disk, cyl, head).items():
                if key == wanted:
                    raw = key + data
                    if len(raw) != 140 or raw[44] != 0xf1 or raw[59] != 1:
                        raise ValueError('single format-1 extent required: ' + name)
                    base = (int.from_bytes(raw[107:109], 'big'), int.from_bytes(raw[109:111], 'big'))
                    end = (int.from_bytes(raw[111:113], 'big'), int.from_bytes(raw[113:115], 'big'))
                    if not (base <= end and end[0] < CYLS and end[1] < HEADS):
                        raise ValueError('invalid dataset extent: ' + name)
                    return (cyl, head, number), raw, base, end
    raise ValueError('missing dataset: ' + name)


def seq(disk, name):
    _, raw, base, end = dscb(disk, name)
    if raw[82:84] != b'\x40\0':
        raise ValueError('expected sequential dataset: ' + name)
    parts = []
    for absolute in range(base[0] * HEADS + base[1], end[0] * HEADS + end[1] + 1):
        for number, (key, data) in records(disk, *divmod(absolute, HEADS)).items():
            if number == 0:
                continue
            if key:
                raise ValueError('keyed sequential record: ' + name)
            if not data:
                return b''.join(parts)
            parts.append(data)
    raise ValueError('missing EOF inside extent: ' + name)


def read_disk(path):
    disk = path.read_bytes()
    if len(disk) != DISK_SIZE or disk[:8] != b'CKD_P370':
        raise ValueError('expected flat 100-cylinder 3390 CKD image')
    return disk


def codec():
    table = Path(__file__).with_name('ibm1047.table').read_text().splitlines()
    values = [int(word, 16) for line in table if not line.startswith('#') for word in line.split()]
    if len(values) != 256 or len(set(values)) != 256:
        raise ValueError('invalid IBM-1047 table')
    return {chr(value): index for index, value in enumerate(values)}


def encoded(value, mapping):
    try:
        return bytes(mapping[char] for char in value)
    except KeyError as exc:
        raise ValueError('character outside IBM-1047: ' + repr(exc.args[0])) from exc


def checked_file(item):
    path = Path(item['file'])
    if not path.is_absolute() or not path.is_file() or not SHA.fullmatch(item['sha256']):
        raise ValueError('absolute existing input and SHA-256 required')
    payload = path.read_bytes()
    if not payload or digest(payload) != item['sha256']:
        raise ValueError('input hash/length mismatch: ' + str(path))
    return payload


def prepare(args):
    if args.out.exists():
        raise ValueError('fresh output directory required')
    manifest = json.loads(args.manifest.read_text())
    programs = manifest['programs']
    datasets = manifest.get('datasets', [])
    checks = manifest['checks']
    members = manifest.get('members', [])
    if not programs or not checks or len(programs) > 12 or len(datasets) > 20 or len(checks) > 30 or len(members) > 30:
        raise ValueError('bounded nonempty program/check inventory required')
    mapping = codec()
    disk = read_disk(args.base)
    extracted = {name: seq(disk, name) for name in BASE_NAMES}
    args.out.mkdir(parents=True)
    names = {'CONFORM.BAT', *BASE_NAMES}
    control = ['PDOS00 3390-1 100',
               'PLOAD.SYS SEQ pload.sys TRK 10 1 0 PS F 18452 18452',
               'SYSVTOC VTOC CYL 2',
               'PDOS.SYS SEQ pdos.sys CYL 1 1 0 PS F 18452 18452',
               'CONFIG.SYS SEQ config.sys CYL 1 1 0 PS F 10 10',
               'COMMAND.EXE SEQ pcomm.exe CYL 1 1 0 PS F 18452 18452']
    for name, payload in extracted.items():
        (args.out / BASE_FILES[name]).write_bytes(payload)
    installed = []
    for index, item in enumerate(programs, 1):
        name = item['name'].upper()
        dsn = name + '.EXE'
        if not NAME.fullmatch(name) or dsn in names or not 1 <= item['cylinders'] <= 16:
            raise ValueError('invalid or duplicate program')
        names.add(dsn)
        payload = checked_file(item)
        filename = f'program{index:02d}.rdw'
        (args.out / filename).write_bytes(payload)
        control.append(f'{dsn} SEQ {filename} CYL {item["cylinders"]} 1 0 PS F 18452 18452')
        installed.append({'dataset': dsn, 'sha256': digest(payload),
                          'bytes': len(payload), 'media_file': filename})
    for item in datasets:
        dsn = item['name'].upper()
        if not DATASET.fullmatch(dsn) or dsn in names or not 1 <= item['cylinders'] <= 16:
            raise ValueError('invalid or duplicate dataset')
        if item['organization'] not in ('PO', 'PS') or item['record_format'] not in ('VB', 'FB'):
            raise ValueError('unsupported dataset geometry')
        lrecl, blksize = item['lrecl'], item['blksize']
        if not 1 <= lrecl <= blksize <= 32760:
            raise ValueError('invalid dataset record length')
        names.add(dsn)
        if item['kind'] != 'empty':
            raise ValueError('step-1 datasets must be empty; checked members are separate')
        operation = 'EMPTY'
        control.append(f'{dsn} {operation} CYL {item["cylinders"]} 1 5 '
                       f'{item["organization"]} {item["record_format"]} {lrecl} {blksize}')
    installed_members = []
    for index, item in enumerate(members, 1):
        dsn, member = item['dataset'].upper(), item['member'].upper()
        parent = next((d for d in datasets if d['name'].upper() == dsn), None)
        if (parent is None or parent['kind'] != 'empty' or parent['organization'] != 'PO'
                or parent['record_format'] != 'VB' or not NAME.fullmatch(member)
                or item['kind'] not in ('binary', 'text')
                or any(x['dataset'] == dsn and x['member'] == member for x in installed_members)):
            raise ValueError('members require unique names in an empty PO/VB dataset')
        payload = checked_file(item)
        filename = f'member{index:02d}.input'
        (args.out / filename).write_bytes(payload)
        installed_members.append({'dataset': dsn, 'member': member,
                                  'kind': item['kind'], 'media_file': filename,
                                  'sha256': digest(payload)})
    control.append('CONFORM.BAT EMPTY CYL 1 1 0 PS VB 80 800')
    ids, output_phrases, batch = set(), set(), [encoded('echo off', mapping)]
    response_plan = []
    for item in checks:
        name = item['id'].upper()
        command = item['command']
        if not NAME.fullmatch(name) or name in ids or '\n' in command or '\r' in command:
            raise ValueError('invalid check id or command')
        if not command.split() or command.split()[0].upper() + '.EXE' not in names or type(item['expected_rc']) is not int:
            raise ValueError('check must call an installed program and name its expected RC')
        expected = item.get('output_contains')
        if (not isinstance(expected, list) or not expected
                or any(not isinstance(s, str) or not s or '\n' in s or '\r' in s
                       or s in command or s in output_phrases for s in expected)):
            raise ValueError('nonempty output checks distinct from command are required')
        output_phrases.update(expected)
        responses = item.get('responses', [])
        if not isinstance(responses, list):
            raise ValueError('responses must be a list')
        for response in responses:
            prompt, answer = response['prompt'], response['text']
            if not prompt or any(c in prompt + answer for c in '\n\r\t"\\'):
                raise ValueError('unsupported prompt/response character')
            response_plan.append(prompt + '\t' + answer)
        line = encoded(command, mapping)
        if not 1 <= len(line) <= 198:
            raise ValueError('PCOMM batch command must be 1..198 target bytes')
        ids.add(name)
        batch.append(line)
    marker = 'CONFORM_DONE_' + digest(args.manifest.read_bytes())[:12].upper()
    batch.append(encoded('echo ' + marker, mapping))
    raw = b'\x15'.join(batch) + b'\x15'
    if len(raw) > 18452:
        raise ValueError('batch exceeds one native block')
    (args.out / 'conform.raw').write_bytes(raw)
    (args.out / 'response-plan.tsv').write_text('\n'.join(response_plan) + ('\n' if response_plan else ''))
    (args.out / 'expected-output.txt').write_text('\n'.join(sorted(output_phrases)) + '\n')
    (args.out / 'completion-marker.txt').write_text(marker + '\n')
    (args.out / 'ctl.txt').write_text('\n'.join(control) + '\n')
    (args.out / 'manifest.lock.json').write_text(json.dumps({
        'base_sha256': digest(disk), 'manifest_sha256': digest(args.manifest.read_bytes()),
        'base_files': {name: digest(payload) for name, payload in extracted.items()},
        'installed': installed, 'members': installed_members,
        'batch_sha256': digest(raw), 'checks': checks,
        'completion_marker': marker,
    }, indent=2) + '\n')
    print(f'Prepared {len(programs)} native programs, {len(datasets)} datasets, {len(checks)} checks')


def patch_batch(args):
    if args.out.exists():
        raise ValueError('fresh candidate flat disk required')
    disk = bytearray(read_disk(args.disk))
    payload = args.batch.read_bytes()
    if not payload or len(payload) > 18452 or payload[-1:] != b'\x15':
        raise ValueError('invalid native batch')
    location, raw, base, end = dscb(disk, 'CONFORM.BAT')
    if raw[82:84] != b'\x40\0':
        raise ValueError('expected sequential batch placeholder')
    cyl, head = base
    old = records(disk, cyl, head)
    if 0 not in old:
        raise ValueError('missing record zero')
    write_track(disk, cyl, head, {0: old[0], 1: (b'', payload), 2: (b'', b'')})
    vc, vh, vn = location
    values = records(disk, vc, vh)
    patched = bytearray(raw)
    patched[84] = 0xc0  # RECFM U; PCOMM OPEN requests U/18452.
    patched[86:88] = (18452).to_bytes(2, 'big')
    patched[88:90] = b'\0\0'
    patched[98:101] = b'\0\0\x02'
    values[vn] = (bytes(patched[:44]), bytes(patched[44:]))
    write_track(disk, vc, vh, values)
    if seq(disk, 'CONFORM.BAT') != payload:
        raise ValueError('native batch readback mismatch')
    lock = json.loads(args.lock.read_text())
    grouped = {}
    for item in lock['members']:
        grouped.setdefault(item['dataset'], []).append(item)
    for dsn, items in grouped.items():
        stage_members(disk, dsn, items, args.media)
    args.out.write_bytes(disk)
    print('Native batch exact readback passed')


def verify(args):
    disk = read_disk(args.disk)
    lock = json.loads(args.lock.read_text())
    for name, expected in lock['base_files'].items():
        if digest(seq(disk, name)) != expected:
            raise ValueError('base dataset changed: ' + name)
    for item in lock['installed']:
        dscb(disk, item['dataset'])
        if 'sha256' in item:
            actual = seq(disk, item['dataset'])
            source = (args.media / item['media_file']).read_bytes()
            if actual[:len(source)] != source or any(actual[len(source):]):
                raise ValueError('program readback mismatch: ' + item['dataset'])
    if digest(seq(disk, 'CONFORM.BAT')) != lock['batch_sha256']:
        raise ValueError('batch readback mismatch')
    for item in lock['members']:
        blocks = member_blocks(disk, item['dataset'], item['member'])
        parent = dscb(disk, item['dataset'])[1]
        logical = parse_vb(blocks)
        source = (args.media / item['media_file']).read_bytes()
        expected = member_values(source, item['kind'], int.from_bytes(parent[88:90], 'big'))
        if logical != expected:
            raise ValueError('member logical readback mismatch: ' + item['member'])
    print('Base, native programs and batch pass stopped-disk readback')


def member_values(payload, kind, lrecl):
    if kind == 'binary':
        return [payload[i:i + lrecl - 4] for i in range(0, len(payload), lrecl - 4)]
    mapping = codec()
    return [encoded(line, mapping) for line in payload.decode('utf-8').splitlines()]


def vb_blocks(values, lrecl, blksize):
    result, block = [], bytearray(b'\0' * 4)
    for value in values:
        if len(value) > lrecl - 4:
            raise ValueError('member record exceeds LRECL')
        framed = (len(value) + 4).to_bytes(2, 'big') + b'\0\0' + value
        if len(block) + len(framed) > blksize:
            block[:2] = len(block).to_bytes(2, 'big')
            result.append(bytes(block))
            block = bytearray(b'\0' * 4)
        block += framed
    block[:2] = len(block).to_bytes(2, 'big')
    result.append(bytes(block))
    return result


def parse_vb(blocks):
    values = []
    for block in blocks:
        if len(block) < 4 or int.from_bytes(block[:2], 'big') != len(block):
            raise ValueError('invalid BDW')
        at = 4
        while at < len(block):
            size = int.from_bytes(block[at:at + 2], 'big')
            if size < 4 or at + size > len(block):
                raise ValueError('invalid RDW')
            values.append(block[at + 4:at + size])
            at += size
    return values


def directory(disk, dsn):
    _, raw, base, end = dscb(disk, dsn)
    if raw[82:84] != b'\x02\0' or raw[84] != 0x50:
        raise ValueError('simple VB PDS required: ' + dsn)
    found, directory_numbers = {}, []
    for number, (key, data) in records(disk, *base).items():
        if key != b'\xff' * 8:
            continue
        directory_numbers.append(number)
        used = int.from_bytes(data[:2], 'big')
        if used == 0:
            continue
        at = 2
        while at + 8 <= used:
            if data[at:at + 8] == b'\xff' * 8:
                break
            name = data[at:at + 8].decode('cp037').rstrip()
            if at + 12 > used or data[at + 11] != 0 or name in found:
                raise ValueError('unsupported PDS directory entry')
            found[name] = data[at + 8:at + 11]
            at += 12
    if not directory_numbers:
        raise ValueError('missing PDS directory blocks')
    return raw, base, end, directory_numbers, found


def member_blocks(disk, dsn, member):
    _, base, end, _, found = directory(disk, dsn)
    if member not in found:
        raise ValueError('missing member: ' + dsn + '/' + member)
    ttr = found[member]
    track, first_record = int.from_bytes(ttr[:2], 'big'), ttr[2]
    blocks = []
    while True:
        absolute = base[0] * HEADS + base[1] + track
        if absolute > end[0] * HEADS + end[1]:
            raise ValueError('member crosses extent')
        for number, (key, data) in records(disk, *divmod(absolute, HEADS)).items():
            if number < first_record or number == 0:
                continue
            if key:
                raise ValueError('keyed PDS member record')
            if not data:
                return blocks
            blocks.append(data)
        track += 1
        first_record = 1


def physical_cost(keylen, datalen):
    def field(length, factor):
        intervals = (length + 6 + 231) // 232
        amount = 34 * factor + length + 6 + 6 * intervals
        return ((amount + 33) // 34) * 34
    return field(datalen, 19) + (field(keylen, 9) if keylen else 0)


def stage_members(disk, dsn, items, media):
    location, raw, base, end = dscb(disk, dsn)
    _, _, _, directory_numbers, prior = directory(disk, dsn)
    if prior:
        raise ValueError('member staging requires empty PDS: ' + dsn)
    lrecl, blksize = int.from_bytes(raw[88:90], 'big'), int.from_bytes(raw[86:88], 'big')
    content = {}
    for item in items:
        source = (media / item['media_file']).read_bytes()
        if digest(source) != item['sha256']:
            raise ValueError('member source changed')
        content[item['member']] = vb_blocks(member_values(source, item['kind'], lrecl), lrecl, blksize)
    if len(content) * 12 + 14 > 256:
        raise ValueError('one-block PDS directory capacity exceeded')
    base_track = base[0] * HEADS + base[1]
    end_track = end[0] * HEADS + end[1]
    track, number, used, pending = 1, 0, 0, {}
    def flush():
        absolute = base_track + track
        if absolute > end_track:
            raise ValueError('PDS extent exhausted')
        cyl, head = divmod(absolute, HEADS)
        old = records(disk, cyl, head)
        if 0 not in old:
            raise ValueError('PDS track missing R0')
        write_track(disk, cyl, head, {0: old[0], **pending})
    def put(data):
        nonlocal track, number, used, pending
        cost = physical_cost(0, len(data))
        if used + cost > 58786 or number == 255:
            flush()
            track, number, used, pending = track + 1, 0, 0, {}
        if base_track + track > end_track:
            raise ValueError('PDS extent exhausted')
        number += 1
        used += cost
        pending[number] = (b'', data)
        return track.to_bytes(2, 'big') + bytes((number,))
    entries = {}
    for name in sorted(content, key=lambda value: value.encode('cp037')):
        first = None
        for block in content[name]:
            ttr = put(block)
            if first is None:
                first = ttr
        eof = put(b'')
        entries[name] = first if first is not None else eof
    flush()
    packed = (b''.join(name.encode('cp037').ljust(8, b'\x40') + ttr + b'\0'
                       for name, ttr in entries.items()) + b'\xff' * 8 + b'\0' * 4)
    cyl, head = base
    old = records(disk, cyl, head)
    updated = {0: old[0]}
    for number in directory_numbers:
        data = ((len(packed) + 2).to_bytes(2, 'big') + packed).ljust(256, b'\0') if number == directory_numbers[0] else b'\0' * 256
        updated[number] = (b'\xff' * 8, data)
    updated[max(directory_numbers) + 1] = (b'', b'')
    write_track(disk, cyl, head, updated)
    vc, vh, vn = location
    vtoc = records(disk, vc, vh)
    revised = bytearray(raw)
    revised[98:101] = eof
    vtoc[vn] = (bytes(revised[:44]), bytes(revised[44:]))
    write_track(disk, vc, vh, vtoc)
    for name, blocks in content.items():
        if member_blocks(disk, dsn, name) != blocks:
            raise ValueError('PDS member readback mismatch: ' + name)


def evaluate(args):
    if args.out.exists():
        raise ValueError('fresh result receipt required')
    lock = json.loads(args.lock.read_text())
    trace = args.trace.read_text(errors='replace')
    marker = lock['completion_marker']
    if marker not in trace:
        raise ValueError('fresh batch completion marker missing')
    lines = trace.splitlines()
    results = []
    cursor = 0
    for number, item in enumerate(lock['checks'], 1):
        begin = re.compile(r'PCOMM BEGIN ' + str(number) + r'\s+', re.I)
        end = re.compile(r'PCOMM END ' + str(number) + r' RC=(-?\d+)')
        start = next((i for i in range(cursor, len(lines)) if begin.search(lines[i])), None)
        finish = next((i for i in range(start + 1, len(lines)) if end.search(lines[i])), None) if start is not None else None
        actual = int(end.search(lines[finish]).group(1)) if finish is not None else None
        between = [line.strip() for line in lines[start + 1:finish]
                   if 'PCOMM BEGIN ' not in line and 'PCOMM END ' not in line] if finish is not None else []
        missing = [value for value in item['output_contains']
                   if not any(value in line for line in between)]
        passed = actual == item['expected_rc'] and not missing
        results.append({'id': item['id'], 'command': item['command'],
                        'expected_rc': item['expected_rc'], 'actual_rc': actual,
                        'missing_output': missing, 'result': 'PASS' if passed else 'FAIL'})
        cursor = finish + 1 if finish is not None else len(lines)
    receipt = {'result': 'PASS' if all(row['result'] == 'PASS' for row in results) else 'FAIL',
               'base_sha256': lock['base_sha256'], 'manifest_sha256': lock['manifest_sha256'],
               'batch_sha256': lock['batch_sha256'], 'trace_sha256': digest(args.trace.read_bytes()),
               'checks': results}
    args.out.write_text(json.dumps(receipt, indent=2) + '\n')
    for row in results:
        print(row['id'], row['result'], 'RC', row['actual_rc'], 'expected', row['expected_rc'])
    print('Overall', receipt['result'])
    if receipt['result'] != 'PASS':
        raise SystemExit(1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='action', required=True)
    prep = sub.add_parser('prepare')
    prep.add_argument('--base', type=Path, required=True)
    prep.add_argument('--manifest', type=Path, required=True)
    prep.add_argument('--out', type=Path, required=True)
    patch = sub.add_parser('patch-batch')
    patch.add_argument('--disk', type=Path, required=True)
    patch.add_argument('--batch', type=Path, required=True)
    patch.add_argument('--out', type=Path, required=True)
    patch.add_argument('--lock', type=Path, required=True)
    patch.add_argument('--media', type=Path, required=True)
    check = sub.add_parser('verify')
    check.add_argument('--disk', type=Path, required=True)
    check.add_argument('--lock', type=Path, required=True)
    check.add_argument('--media', type=Path, required=True)
    judge = sub.add_parser('evaluate')
    judge.add_argument('--lock', type=Path, required=True)
    judge.add_argument('--trace', type=Path, required=True)
    judge.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    {'prepare': prepare, 'patch-batch': patch_batch, 'verify': verify,
     'evaluate': evaluate}[args.action](args)


if __name__ == '__main__':
    main()
