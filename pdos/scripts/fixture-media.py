#!/usr/bin/env python3
"""Bounded binary tape/CKD interface for fixtures.crexx.

The cREXX recipe owns orchestration. This helper decodes VMFPLC2 through the
Hercules utility, checks an uncompressed standard-label AWS subset, and edits
only a freshly built, stopped exchange CKD image.
"""

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import zipfile

HERE = Path(__file__).resolve().parent
sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location('ckd', HERE / 'conformance-media.py')
ckd = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ckd)
DSN = re.compile(r'[A-Z#$@][A-Z0-9#$@.]{0,43}')
CMS = re.compile(r'[A-Z0-9#$@]{1,8}')
HEX = re.compile(r'[0-9a-f]{64}')


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def framed(records):
    return b''.join(len(record).to_bytes(4, 'big') + record for record in records)


def parse_cms_v(data):
    records, at = [], 0
    while at < len(data):
        need(at + 2 <= len(data), 'truncated CMS variable record length')
        n = int.from_bytes(data[at:at + 2], 'big')
        at += 2
        need(at + n <= len(data), 'truncated CMS variable record')
        records.append(data[at:at + n])
        at += n
    return records


def aws_blocks(data):
    at = previous = 0
    while at < len(data):
        need(at + 6 <= len(data), 'truncated AWS header')
        size, prior, flag1, flag2 = struct.unpack_from('<HHBB', data, at)
        need(prior == previous and flag2 == 0
             and flag1 == (0xa0 if size else 0x40), 'unsupported AWS header')
        end = at + 6 + size
        need(end <= len(data), 'truncated AWS block')
        yield data[at + 6:end]
        previous, at = size, end


def parse_zos(item, data):
    blocks = list(aws_blocks(data))
    need(len(blocks) >= 9 and blocks[-2:] == [b'', b''],
         'standard-label tape needs two final marks')
    files, current = [], []
    for block in blocks:
        if block:
            current.append(block)
        else:
            files.append(current)
            current = []
    need(len(files) == 4 and not files[3]
         and len(files[0]) == 3 and len(files[2]) == 2,
         'one standard-label data file required')
    vol, hdr1, hdr2 = files[0]
    eof1, eof2 = files[2]
    need(all(len(label) == 80 for label in (vol, hdr1, hdr2, eof1, eof2)),
         'standard labels must be 80 bytes')
    need(vol[:4] == 'VOL1'.encode('cp037')
         and hdr1[:4] == 'HDR1'.encode('cp037')
         and hdr2[:4] == 'HDR2'.encode('cp037')
         and eof1[:4] == 'EOF1'.encode('cp037')
         and eof2[:4] == 'EOF2'.encode('cp037'), 'standard label identity')
    name = item['source']['dataset']
    need(DSN.fullmatch(name) and hdr1[4:21].decode('cp037').rstrip() == name
         and eof1[4:21] == hdr1[4:21]
         and eof2[4:] == hdr2[4:], 'standard label dataset mismatch')
    volser = item['source']['volser']
    need(len(volser) == 6 and vol[4:10].decode('cp037') == volser
         and hdr1[21:27].decode('cp037') == volser, 'standard label volume mismatch')
    need(hdr2[4:5] == 'F'.encode('cp037'), 'only fixed standard-label data')
    blocksize = int(hdr2[5:10].decode('cp037'))
    lrecl = int(hdr2[10:15].decode('cp037'))
    need(lrecl == item['lrecl'] and blocksize == item['blksize'],
         'HDR2 record geometry differs from manifest')
    need(files[1], 'empty standard-label data file unsupported')
    records = []
    for block in files[1]:
        need(0 < len(block) <= blocksize and len(block) % lrecl == 0,
             'bad fixed tape block')
        records.extend(block[i:i + lrecl] for i in range(0, len(block), lrecl))
    return records


def parse_cms(item, tape, work, hercules):
    source = item['source']
    name, kind, mode, recfm = (source[k] for k in ('name', 'type', 'mode', 'recfm'))
    need(CMS.fullmatch(name) and CMS.fullmatch(kind) and mode == 'A1'
         and recfm in ('F', 'V'), 'unsupported CMS identity or record form')
    scan = subprocess.run([str(hercules / 'vmfplc2'), '-c', '819/1047',
                           'SCAN', str(tape)], capture_output=True, text=True)
    need(scan.returncode == 0, 'VMFPLC2 tape scan failed')
    entries = re.findall(r'>>>\s+(\S+)\s+(\S+)\s+(\S+)\s+([FV])\s+',
                         scan.stdout)
    matches = [entry for entry in entries if entry[:3] == (name, kind, mode)]
    need(matches == [(name, kind, mode, recfm)],
         'missing, duplicate or wrong-format CMS tape identity')
    output = work / (item['id'] + '.cms')
    control = work / (item['id'] + '.ctl')
    clause = f'{name} {kind} {mode} {recfm} '
    if recfm == 'F':
        clause += f'{item["lrecl"]} B '
    else:
        clause += 'S '
    control.write_text(clause + str(output) + '\n')
    result = subprocess.run([str(hercules / 'vmfplc2'), '-c', '819/1047',
                             'LOAD', str(control), str(tape)],
                            capture_output=True, text=True)
    need(result.returncode == 0 and output.is_file(),
         'VMFPLC2 extraction failed: ' + result.stderr[-300:])
    raw = output.read_bytes()
    need(raw, 'empty CMS extraction unsupported')
    if recfm == 'F':
        need(len(raw) % item['lrecl'] == 0, 'CMS fixed record length')
        return [raw[i:i + item['lrecl']]
                for i in range(0, len(raw), item['lrecl'])]
    return parse_cms_v(raw)


def geometry(item):
    target = item['target']
    need(DSN.fullmatch(target), 'invalid target dataset')
    fmt, lrecl, blksize = item['recfm'], item['lrecl'], item['blksize']
    need(fmt in ('FB', 'VB') and type(lrecl) is int and type(blksize) is int
         and 1 <= lrecl <= blksize <= 32760, 'unsupported dataset geometry')
    if fmt == 'FB':
        need(blksize % lrecl == 0, 'FB block is not whole records')
    else:
        need(lrecl >= 4 and blksize >= lrecl + 4,
             'VB needs BDW and RDW capacity')
    need(item['encoding'] in ('ibm1047', 'binary'), 'explicit encoding required')


def load_manifest(path):
    data = json.loads(path.read_text())
    need(data.get('format') == 'pdos-fixtures-v1'
         and data.get('volume_serial') and len(data['volume_serial']) == 6
         and re.fullmatch('[A-Z0-9]{6}', data['volume_serial']),
         'manifest format/serial')
    items, outputs = data['inputs'], data['outputs']
    archive = data.get('archive')
    need(isinstance(archive, dict) and Path(archive.get('file', '')).is_absolute()
         and HEX.fullmatch(archive.get('sha256', ''))
         and isinstance(archive.get('members'), dict),
         'release ZIP path, hash and member map required')
    need(1 <= len(items) <= 12 and 1 <= len(outputs) <= 12,
         'bounded input and output inventory required')
    ids, targets = set(), set()
    for item in items:
        geometry(item)
        need(CMS.fullmatch(item['id']) and item['id'] not in ids
             and item['target'] not in targets, 'duplicate input')
        ids.add(item['id']); targets.add(item['target'])
        need(item['medium'] in ('cms-vmfplc2-het', 'zos-sl-aws')
             and Path(item['tape']).is_absolute()
             and HEX.fullmatch(item['tape_sha256'])
             and HEX.fullmatch(item['records_sha256'])
             and HEX.fullmatch(item['payload_sha256']), 'input hash/medium')
    for item in outputs:
        geometry(item)
        need(item['target'] not in targets and item['same_as'] in ids,
             'output target/source')
        targets.add(item['target'])
    need(set(archive['members']) == ids
         and all(isinstance(x, str) and x and not x.startswith('/')
                 and '..' not in Path(x).parts for x in archive['members'].values()),
         'archive member map must cover each fixture')
    return data


def plan(args):
    need(not args.out.exists(), 'fresh output directory required')
    manifest = load_manifest(args.manifest)
    hercules = args.hercules.resolve()
    need((hercules / 'vmfplc2').is_file(), 'Hercules VMFPLC2 utility missing')
    # All source validation precedes the CKD build. The original tape is read only.
    args.out.mkdir(parents=True)
    archive = manifest['archive']
    zipdata = Path(archive['file']).read_bytes()
    need(sha(zipdata) == archive['sha256'], 'release ZIP SHA-256 mismatch')
    with zipfile.ZipFile(archive['file']) as release:
        names = release.namelist()
        need(len(names) == len(set(names)), 'duplicate ZIP entries')
        for item in manifest['inputs']:
            member = archive['members'][item['id']]
            need(member in names and release.getinfo(member).file_size <= 100000000,
                 'missing or oversized tape member')
            need(release.read(member) == Path(item['tape']).read_bytes(),
                 'release ZIP tape bytes differ: ' + item['id'])
    control = [f'{manifest["volume_serial"]} 3390-1 100', 'SYSVTOC VTOC CYL 2']
    locked = []
    for item in manifest['inputs']:
        tape = Path(item['tape'])
        data = tape.read_bytes()
        need(sha(data) == item['tape_sha256'], 'tape SHA-256 mismatch: ' + item['id'])
        if item['medium'] == 'cms-vmfplc2-het':
            records = parse_cms(item, tape, args.out, hercules)
        else:
            records = parse_zos(item, data)
        need(records and all(len(r) <= item['lrecl'] - (4 if item['recfm'] == 'VB' else 0)
                             for r in records), 'record exceeds destination LRECL')
        if item['recfm'] == 'FB':
            need(all(len(r) == item['lrecl'] for r in records), 'FB record length')
        need(sha(framed(records)) == item['records_sha256']
             and sha(b''.join(records)) == item['payload_sha256'],
             'logical record hash differs: ' + item['id'])
        (args.out / (item['id'] + '.records')).write_bytes(framed(records))
        control.append(f'{item["target"]} EMPTY CYL 1 1 0 PS {item["recfm"]} '
                       f'{item["lrecl"]} {item["blksize"]}')
        locked.append(dict(id=item['id'], target=item['target'],
                           recfm=item['recfm'], lrecl=item['lrecl'],
                           blksize=item['blksize'], encoding=item['encoding'],
                           records_sha256=sha(framed(records)),
                           payload_sha256=sha(b''.join(records)),
                           count=len(records), tape_sha256=sha(data)))
    (args.out / 'ctl.txt').write_text('\n'.join(control) + '\n')
    commands = [f'MOUNT 01BA {manifest["volume_serial"]}',
                f'SELECT {manifest["volume_serial"]}']
    for output in manifest['outputs']:
        source = next(item for item in locked if item['id'] == output['same_as'])
        commands.append(f'ALLOC {output["target"]} {output["recfm"]} '
                        f'{output["lrecl"]} {output["blksize"]} 1')
        commands.append(f'RCOPY {source["target"]} {output["target"]}')
    commands.extend(('SELECT PDOS00', f'UNMOUNT {manifest["volume_serial"]}'))
    (args.out / 'guest-commands.txt').write_text('\n'.join(commands) + '\n')
    (args.out / 'lock.json').write_text(json.dumps(dict(
        manifest_sha256=sha(args.manifest.read_bytes()),
        archive_sha256=archive['sha256'],
        volume_serial=manifest['volume_serial'], inputs=locked,
        outputs=manifest['outputs']), indent=2) + '\n')
    print(f'Validated {len(locked)} logical inputs; exchange control ready')


def inspect(args):
    need(not args.out.exists(), 'fresh pinned manifest path required')
    manifest = load_manifest(args.draft)
    hercules = args.hercules.resolve()
    need((hercules / 'vmfplc2').is_file(), 'Hercules VMFPLC2 utility missing')
    archive = manifest['archive']
    archive_path = Path(archive['file'])
    archive['sha256'] = sha(archive_path.read_bytes())
    with zipfile.ZipFile(archive_path) as release:
        names = release.namelist()
        need(len(names) == len(set(names)), 'duplicate ZIP entries')
        for item in manifest['inputs']:
            tape = Path(item['tape'])
            data = tape.read_bytes()
            member = archive['members'][item['id']]
            need(member in names and release.getinfo(member).file_size <= 100000000,
                 'missing or oversized tape member')
            need(release.read(member) == data,
                 'release ZIP tape bytes differ: ' + item['id'])
            item['tape_sha256'] = sha(data)
            with tempfile.TemporaryDirectory(prefix='pdos-fixture-inspect-') as temp:
                if item['medium'] == 'cms-vmfplc2-het':
                    records = parse_cms(item, tape, Path(temp), hercules)
                else:
                    records = parse_zos(item, data)
            need(records and all(len(r) <= item['lrecl'] - (4 if item['recfm'] == 'VB' else 0)
                                 for r in records), 'record exceeds destination LRECL')
            if item['recfm'] == 'FB':
                need(all(len(r) == item['lrecl'] for r in records),
                     'FB record length')
            item['records_sha256'] = sha(framed(records))
            item['payload_sha256'] = sha(b''.join(records))
            print(f'{item["id"]}: records={len(records)} bytes={sum(map(len, records))} '
                  f'records_sha256={item["records_sha256"]}')
    args.out.write_text(json.dumps(manifest, indent=2) + '\n')
    print('Pinned manifest:', args.out)


def read_records_file(path):
    data = path.read_bytes(); at = 0; records = []
    while at < len(data):
        need(at + 4 <= len(data), 'truncated framed record')
        n = int.from_bytes(data[at:at + 4], 'big'); at += 4
        need(at + n <= len(data), 'truncated framed payload')
        records.append(data[at:at + n]); at += n
    return records


def blocks_for(item, values):
    if item['recfm'] == 'VB':
        return ckd.vb_blocks(values, item['lrecl'], item['blksize'])
    per = item['blksize'] // item['lrecl']
    return [b''.join(values[i:i + per]) for i in range(0, len(values), per)]


def logical(disk, item):
    _, raw, _, _ = ckd.dscb(disk, item['target'])
    need(raw[82:84] == b'\x40\0' and raw[84] == (0x90 if item['recfm'] == 'FB' else 0x50)
         and int.from_bytes(raw[86:88], 'big') == item['blksize']
         and int.from_bytes(raw[88:90], 'big') == item['lrecl'],
         'dataset geometry mismatch: ' + item['target'])
    raw_data = ckd.seq(disk, item['target'])
    if item['recfm'] == 'VB':
        # seq concatenates physical blocks; split using each BDW.
        blocks = []; at = 0
        while at < len(raw_data):
            need(at + 4 <= len(raw_data), 'short BDW')
            size = int.from_bytes(raw_data[at:at + 2], 'big')
            need(4 <= size <= item['blksize'] and at + size <= len(raw_data), 'bad BDW')
            blocks.append(raw_data[at:at + size]); at += size
        return ckd.parse_vb(blocks)
    need(len(raw_data) % item['lrecl'] == 0, 'short FB record')
    return [raw_data[i:i + item['lrecl']]
            for i in range(0, len(raw_data), item['lrecl'])]


def stage(args):
    need(not args.out.exists(), 'fresh staged flat CKD required')
    lock = json.loads(args.lock.read_text())
    disk = bytearray(ckd.read_disk(args.disk))
    original = bytes(disk)
    edited = set()
    for item in lock['inputs']:
        values = read_records_file(args.media / (item['id'] + '.records'))
        need(sha(framed(values)) == item['records_sha256'], 'staging source changed')
        location, raw, base, end = ckd.dscb(disk, item['target'])
        need(base[0] == end[0] and base[1] == 0 and end[1] == 14,
             'one-cylinder input extent required')
        blocks = blocks_for(item, values)
        need(blocks and sum(ckd.physical_cost(0, len(b)) for b in blocks)
             + ckd.physical_cost(0, 0) < ckd.TRACK - 128,
             'fixture exceeds one track')
        track = ckd.records(disk, *base)
        need(list(track) == [0, 1] and track[1] == (b'', b''),
             'input dataset is not initially empty')
        values_by_record = {0: track[0]}
        for n, block in enumerate(blocks, 1):
            values_by_record[n] = (b'', block)
        values_by_record[len(blocks) + 1] = (b'', b'')
        ckd.write_track(disk, *base, values_by_record)
        edited.add(base)
        vc, vh, vn = location
        vtoc = ckd.records(disk, vc, vh)
        patched = bytearray(raw)
        patched[98:101] = b'\0\0' + bytes((len(blocks) + 1,))
        vtoc[vn] = (bytes(patched[:44]), bytes(patched[44:]))
        ckd.write_track(disk, vc, vh, vtoc)
        edited.add((vc, vh))
        need(logical(disk, item) == values, 'staged logical readback failed')
    for cylinder in range(ckd.CYLS):
        for head in range(ckd.HEADS):
            if (cylinder, head) in edited: continue
            offset = ckd.HEADER + (cylinder * ckd.HEADS + head) * ckd.TRACK
            need(disk[offset:offset + ckd.TRACK]
                 == original[offset:offset + ckd.TRACK], 'unrelated track changed')
    args.out.write_bytes(disk)
    print('Staged logical records; unrelated tracks unchanged')


def verify(args):
    need(not args.out.exists(), 'fresh export directory required')
    lock = json.loads(args.lock.read_text())
    disk = ckd.read_disk(args.disk)
    baseline = ckd.read_disk(args.base)
    inputs = {}
    for item in lock['inputs']:
        values = logical(disk, item)
        need(sha(framed(values)) == item['records_sha256'],
             'import dataset changed: ' + item['target'])
        inputs[item['id']] = values
    exports = []
    for item in lock['outputs']:
        values = logical(disk, item)
        expected = inputs[item['same_as']]
        need(values == expected, 'guest output differs: ' + item['target'])
        exports.append((item, values))
    output_locations = set()
    output_tracks = set()
    for item, _ in exports:
        need(item['target'] not in [x['target'] for x in lock['inputs']],
             'output overlaps input')
        try:
            ckd.dscb(baseline, item['target'])
        except ValueError:
            pass
        else:
            raise ValueError('output existed before guest ALLOC: ' + item['target'])
        location, _, start, end = ckd.dscb(disk, item['target'])
        need(start[0] == end[0] and start[1] == 0 and end[1] == 14,
             'unexpected output extent')
        output_locations.add(location)
        output_tracks.add(start)
    for cylinder in (1, 2):
        for head in range(ckd.HEADS):
            before = ckd.records(baseline, cylinder, head)
            after = ckd.records(disk, cylinder, head)
            need(set(before) == set(after), 'VTOC record layout changed')
            for number in before:
                if (cylinder, head, number) in output_locations:
                    need(before[number] == (b'\0' * 44, b'\0' * 96),
                         'output did not occupy a blank VTOC slot')
                else:
                    need(before[number] == after[number],
                         'unrelated VTOC entry changed')
    for cylinder in range(ckd.CYLS):
        for head in range(ckd.HEADS):
            if cylinder in (1, 2) or (cylinder, head) in output_tracks:
                continue
            offset = ckd.HEADER + (cylinder * ckd.HEADS + head) * ckd.TRACK
            need(baseline[offset:offset + ckd.TRACK]
                 == disk[offset:offset + ckd.TRACK],
                 'unrelated CKD track changed')
    args.out.mkdir(parents=True)
    receipt = {'format': 'pdos-fixture-export-v1',
               'disk_sha256': sha(disk), 'manifest_sha256': lock['manifest_sha256'],
               'archive_sha256': lock['archive_sha256'],
               'unrelated_tracks_unchanged': True,
               'exports': []}
    for item, values in exports:
        target = item['target']
        (args.out / (target + '.records')).write_bytes(framed(values))
        (args.out / (target + '.bin')).write_bytes(b''.join(values))
        receipt['exports'].append(dict(target=target, same_as=item['same_as'],
                                       records=len(values),
                                       records_sha256=sha(framed(values)),
                                       payload_sha256=sha(b''.join(values))))
    (args.out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(f'PASS: {len(exports)} exact guest output datasets')


def compare(args):
    left = ckd.read_disk(args.left)
    right = ckd.read_disk(args.right)
    need(left[ckd.HEADER:] == right[ckd.HEADER:],
         'whole CKD track-byte readback differs')
    print('Whole CKD track-byte readback passed')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='mode', required=True)
    p = sub.add_parser('plan'); p.add_argument('--manifest', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--hercules', type=Path, required=True)
    p = sub.add_parser('inspect')
    p.add_argument('--draft', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--hercules', type=Path, required=True)
    for mode in ('stage', 'verify'):
        p = sub.add_parser(mode)
        p.add_argument('--disk', type=Path, required=True)
        p.add_argument('--lock', type=Path, required=True)
        p.add_argument('--media', type=Path, required=True)
        p.add_argument('--out', type=Path, required=True)
        if mode == 'verify':
            p.add_argument('--base', type=Path, required=True)
    p = sub.add_parser('compare')
    p.add_argument('--left', type=Path, required=True)
    p.add_argument('--right', type=Path, required=True)
    args = parser.parse_args()
    try:
        {'inspect': inspect, 'plan': plan, 'stage': stage, 'verify': verify,
         'compare': compare}[args.mode](args)
    except (OSError, KeyError, TypeError, ValueError) as exc:
        parser.exit(1, f'Fixture validation failed: {exc}\n')


if __name__ == '__main__':
    main()
