#!/usr/bin/env python3
"""Inspect unchanged, length-framed CMS MODULE bytes from a checked ZIP.

The cREXX operator recipe owns orchestration. This binary interface validates
the bounded CMS24 and CMS31 records before a guest candidate is constructed.
It never rewrites a MODULE or the release archive.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys
import zipfile

sys.dont_write_bytecode = True

SHA = re.compile(r'[0-9a-f]{64}')
PROFILES = {'cms24': ('RXVM',), 'cms31': ('RXVM', 'RXAS', 'RXC')}
FIXTURES = {
    'cms24': ('IO24.rxbin', 'IO24.crexx'),
    'cms31': ('IOQUAL.rxbin', 'LIBRARY.rxbin', 'RXCEXITS.rxbin',
              'IOQUAL.crexx', 'IOBAD.crexx', 'IOBAD.rxas'),
}
MAX_IMAGE = 8 * 1024 * 1024
BLOCK = 18452
TRACKS_PER_CYLINDER = 15
BLOCKS_PER_TRACK = 3
ENVELOPE = 64


def need(ok, message):
    if not ok:
        raise ValueError(message)


def word(data, at):
    return int.from_bytes(data[at:at + 4], 'big')


def digest(data):
    return hashlib.sha256(data).hexdigest()


def fnv32(data):
    value = 0x811c9dc5
    for byte in data:
        value = ((value ^ byte) * 0x01000193) & 0xffffffff
    return value


def parse_records(data):
    need(data and len(data) <= MAX_IMAGE + 1024 * 1024,
         'empty or oversized MODULE transport')
    records, at = [], 0
    while at < len(data):
        need(at + 2 <= len(data), 'truncated MODULE record length')
        size = int.from_bytes(data[at:at + 2], 'big')
        at += 2
        need(0 < size <= 65535 and at + size <= len(data),
             'zero, oversized or truncated MODULE record')
        records.append(data[at:at + size])
        at += size
    return records


def parse_module(data, profile):
    need(profile in PROFILES, 'unknown CMS profile')
    records = parse_records(data)
    need(records and len(records[0]) == 80, 'MODULE header must be 80 bytes')
    header = records[0]
    entry, origin, end, location = (word(header, at) for at in (0, 4, 8, 12))
    need(origin >= 0x20000 and origin % 8 == 0 and end == location
         and origin <= entry < end and end % 8 == 0
         and 0 < end - origin <= MAX_IMAGE, 'invalid MODULE address range')
    image_size = end - origin
    image_count = (image_size + 65534) // 65535
    need(len(records) >= 1 + image_count, 'missing MODULE image records')
    image_records = records[1:1 + image_count]
    need(all(len(record) == min(65535, image_size - i * 65535)
             for i, record in enumerate(image_records)),
         'MODULE image record length mismatch')
    image = b''.join(image_records)
    fixups = []
    if profile == 'cms24':
        need(end < 0x1000000 and header[43] == 0x80
             and header[40:42] == b'\0\0'
             and all(byte == 0 for i, byte in enumerate(header)
                     if i not in tuple(range(16)) + (43,)),
             'unsupported fixed-origin CMS24 header')
        need(len(records) == 1 + image_count,
             'unexpected CMS24 map or relocation records')
    else:
        need(end < 0x80000000 and header[32] == 0x48
             and header[34] == 2 and header[40:42] == b'\0\3'
             and header[43:45] == b'\3\xa0' and header[46] == 0x80
             and header[45] in (0xd0, 0xf0),
             'unsupported CMS31 MODULE header flags')
        count = int.from_bytes(header[48:50], 'big')
        need(count > 0 and len(records) == image_count + 2 + count
             and (header[45] == 0xf0) == (count > 1),
             'CMS31 relocation record count mismatch')
        load_map = records[1 + image_count]
        need(len(load_map) == 72 and load_map[0:8] ==
             bytes((0xc5, 0xd3, 0xc6, 0xd7, 0xd6, 0xc3, 0x40, 0x40))
             and word(load_map, 12) == (entry | 0x80000000)
             and word(load_map, 20) == origin,
             'unsupported CMS31 load map')
        previous = end
        for index, record in enumerate(records[image_count + 2:]):
            need(len(record) % 5 == 0 and len(record) <= 65535
                 and (index == count - 1 or len(record) == 65535),
                 'invalid CMS31 relocation record framing')
            for at in range(0, len(record), 5):
                address = word(record, at + 1)
                need(record[at] == 3 and origin <= address <= end - 4
                     and address < previous,
                     'unsupported or unordered CMS31 relocation')
                previous = address
                offset = address - origin
                value = word(image, offset)
                need(origin <= value <= end,
                     'CMS31 relocation target outside image')
                fixups.append(address)
        need(fixups, 'CMS31 MODULE has no relocations')
    return dict(profile=profile, entry=entry, origin=origin, end=end,
                image_bytes=image_size, image_sha256=digest(image),
                records=len(records), relocations=len(fixups),
                records_sha256=digest(b''.join(len(record).to_bytes(4, 'big') + record
                                                for record in records)),
                module_sha256=digest(data), module_bytes=len(data))


def fixture_records(data, suffix):
    if suffix == 'rxbin':
        parts = [data[i:i + 256] for i in range(0, len(data), 256)]
    else:
        table = Path(__file__).with_name('ibm1047.table').read_text().splitlines()
        values = [int(value, 16) for line in table if not line.startswith('#')
                  for value in line.split()]
        need(len(values) == 256 and len(set(values)) == 256,
             'invalid checked IBM1047 table')
        reverse = {value: index for index, value in enumerate(values)}
        need(all(byte < 128 for byte in data), 'source fixture is not ASCII')
        parts = [bytes(reverse[byte] for byte in line) if line else b'\x40'
                 for line in data.splitlines()]
    need(parts and len(parts) <= 65535
         and all(0 < len(part) <= 256 for part in parts),
         'invalid fixture record boundaries')
    return b''.join(len(part).to_bytes(2, 'big') + part for part in parts), len(parts)


def fixture_contract(release, profile, filename):
    name, suffix = filename.split('.')
    member = f'{profile}/{filename}'
    need(member in release.namelist(), 'missing fixture ' + member)
    source = release.read(member)
    framed, count = fixture_records(source, suffix)
    return dict(profile=profile, name=name, type=suffix.upper(), member=member,
                source_sha256=digest(source), source_bytes=len(source),
                records=count, framed_sha256=digest(framed), framed_bytes=len(framed))


def inspect(archive, expected_sha, out):
    need(archive.is_absolute() and archive.is_file()
         and SHA.fullmatch(expected_sha), 'absolute ZIP and pinned SHA-256 required')
    need(not out.exists(), 'fresh output receipt path required')
    need(digest(archive.read_bytes()) == expected_sha, 'release ZIP hash mismatch')
    modules, fixtures = [], []
    with zipfile.ZipFile(archive) as release:
        names = release.namelist()
        need(len(names) == len(set(names)), 'duplicate ZIP members')
        for profile, programs in PROFILES.items():
            for program in programs:
                member = f'{profile}/{program}.module'
                need(member in names and release.getinfo(member).file_size
                     <= MAX_IMAGE + 1024 * 1024, 'missing or oversized ' + member)
                result = parse_module(release.read(member), profile)
                result['member'] = member
                result['program'] = program
                modules.append(result)
        for profile, files in FIXTURES.items():
            for filename in files:
                fixtures.append(fixture_contract(release, profile, filename))
    receipt = dict(format='pdos-cms-module-contract-v2',
                   archive_sha256=expected_sha, modules=modules,
                   fixtures=fixtures)
    out.write_text(json.dumps(receipt, indent=2) + '\n')
    for item in modules:
        print(f"{item['member']}: bytes={item['module_bytes']} "
              f"image={item['image_bytes']} RLD={item['relocations']} "
              f"SHA-256={item['module_sha256']}")
    print('Pinned contract:', out)


def stage(archive, contract_file, out):
    need(archive.is_absolute() and archive.is_file()
         and not out.exists(), 'absolute release ZIP and fresh stage path required')
    contract = json.loads(contract_file.read_text())
    need(contract.get('format') == 'pdos-cms-module-contract-v2'
         and SHA.fullmatch(contract.get('archive_sha256', ''))
         and digest(archive.read_bytes()) == contract['archive_sha256'],
         'contract or release ZIP hash mismatch')
    expected = {(p, n) for p, names in PROFILES.items() for n in names}
    need({(m['profile'], m['program']) for m in contract['modules']} == expected
         and len(contract['modules']) == len(expected), 'incomplete MODULE contract')
    expected_fixtures = {f'{p}/{f}' for p, names in FIXTURES.items() for f in names}
    need({f['member'] for f in contract['fixtures']} == expected_fixtures
         and len(contract['fixtures']) == len(expected_fixtures),
         'incomplete fixture contract')
    out.mkdir(parents=True)
    control = ['CMSX01 3390-1 100', 'SYSVTOC VTOC CYL 2']
    locked, fixture_locks = [], []
    with zipfile.ZipFile(archive) as release:
        need(len(release.namelist()) == len(set(release.namelist())),
             'duplicate ZIP members')
        for item in contract['modules']:
            profile, program = item['profile'], item['program']
            member = f'{profile}/{program}.module'
            need(member == item['member'] and member in release.namelist(),
                 'MODULE member mismatch')
            payload = release.read(member)
            parsed = parse_module(payload, profile)
            need(all(parsed[k] == item[k] for k in parsed),
                 'MODULE changed since contract inspection')
            profile_number = 24 if profile == 'cms24' else 31
            header = (b'PDCMSM01' + profile_number.to_bytes(4, 'big')
                      + len(payload).to_bytes(4, 'big')
                      + parsed['image_bytes'].to_bytes(4, 'big')
                      + parsed['records'].to_bytes(4, 'big')
                      + bytes.fromhex(parsed['module_sha256'])
                      + fnv32(payload).to_bytes(4, 'big') + b'\0' * 4)
            need(len(header) == ENVELOPE, 'invalid stage envelope')
            dataset = f'CMS{profile_number}.{program}'
            media_name = f'{profile}-{program}.bin'
            raw = header + payload
            (out / media_name).write_bytes(raw)
            cylinders = (len(raw) + BLOCK * TRACKS_PER_CYLINDER
                         * BLOCKS_PER_TRACK - 1) // (BLOCK * TRACKS_PER_CYLINDER
                                                    * BLOCKS_PER_TRACK) + 1
            need(1 <= cylinders <= 16, 'MODULE exceeds bounded exchange extent')
            control.append(f'{dataset} SEQ {media_name} CYL {cylinders} 1 0 '
                           f'PS F {BLOCK} {BLOCK}')
            locked.append(dict(dataset=dataset, member=member, module_sha256=digest(payload),
                               module_bytes=len(payload), staged_sha256=digest(raw),
                               staged_bytes=len(raw), cylinders=cylinders))
        for item in contract['fixtures']:
            checked = fixture_contract(release, item['profile'],
                                       item['name'] + '.' + item['type'].lower())
            need(item == checked, 'fixture changed since contract inspection')
            source = release.read(item['member'])
            framed, records = fixture_records(source, item['type'].lower())
            profile_number = 24 if item['profile'] == 'cms24' else 31
            header = (b'PDCMSF01' + profile_number.to_bytes(4, 'big')
                      + len(framed).to_bytes(4, 'big')
                      + len(source).to_bytes(4, 'big')
                      + records.to_bytes(4, 'big')
                      + bytes.fromhex(digest(source))
                      + fnv32(framed).to_bytes(4, 'big') + b'\0' * 4)
            need(len(header) == ENVELOPE, 'invalid fixture envelope')
            dataset = f'CMS{profile_number}.{item["name"]}.{item["type"]}'
            media_name = f'{item["profile"]}-{item["name"]}-{item["type"]}.bin'
            raw = header + framed
            (out / media_name).write_bytes(raw)
            cylinders = (len(raw) + BLOCK * TRACKS_PER_CYLINDER
                         * BLOCKS_PER_TRACK - 1) // (BLOCK * TRACKS_PER_CYLINDER
                                                    * BLOCKS_PER_TRACK) + 1
            need(1 <= cylinders <= 16, 'fixture exceeds bounded exchange extent')
            control.append(f'{dataset} SEQ {media_name} CYL {cylinders} 1 0 '
                           f'PS F {BLOCK} {BLOCK}')
            fixture_locks.append(dict(dataset=dataset, member=item['member'],
                                      source_sha256=digest(source),
                                      staged_sha256=digest(raw),
                                      staged_bytes=len(raw), cylinders=cylinders))
    (out / 'ctl.txt').write_text('\n'.join(control) + '\n')
    (out / 'lock.json').write_text(json.dumps(dict(
        format='pdos-cms-stage-v2', archive_sha256=contract['archive_sha256'],
        contract_sha256=digest(contract_file.read_bytes()), modules=locked,
        fixtures=fixture_locks),
        indent=2) + '\n')
    print(f'Prepared {len(locked)} MODULE and {len(fixture_locks)} fixture datasets')


def verify_stage(disk_file, stage_dir):
    import importlib.util
    helper = Path(__file__).with_name('conformance-media.py')
    spec = importlib.util.spec_from_file_location('pdos_ckd', helper)
    ckd = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(ckd)
    disk = ckd.read_disk(disk_file)
    lock = json.loads((stage_dir / 'lock.json').read_text())
    need(lock.get('format') == 'pdos-cms-stage-v2', 'invalid stage lock')
    for item in lock['modules'] + lock['fixtures']:
        _, raw, base, end = ckd.dscb(disk, item['dataset'])
        need(raw[82:84] == b'\x40\0' and raw[84] == 0x80
             and int.from_bytes(raw[86:90], 'big') == (BLOCK << 16) + BLOCK
             and base[0] >= 3 and end[0] < 100,
             'CMS dataset geometry mismatch')
        data = ckd.seq(disk, item['dataset'])
        need(len(data) >= item['staged_bytes'] and
             not any(data[item['staged_bytes']:]), 'CMS stage padding mismatch')
        staged = data[:item['staged_bytes']]
        need(digest(staged) == item['staged_sha256'],
             'CMS bytes differ after CKD readback')
        if item in lock['modules']:
            need(staged[:8] == b'PDCMSM01'
                 and int.from_bytes(staged[12:16], 'big') == item['module_bytes']
                 and digest(staged[ENVELOPE:]) == item['module_sha256'],
                 'MODULE bytes differ after CKD readback')
            print(f"PASS {item['dataset']}: {item['module_bytes']} unchanged MODULE bytes")
        else:
            need(staged[:8] == b'PDCMSF01', 'fixture envelope mismatch')
            print(f"PASS {item['dataset']}: {item['staged_bytes']} fixture bytes")


def verify_output(disk_file, receipt_file, names, profile):
    import importlib.util
    need(not receipt_file.exists(), 'fresh output receipt path required')
    helper = Path(__file__).with_name('conformance-media.py')
    spec = importlib.util.spec_from_file_location('pdos_ckd', helper)
    ckd = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(ckd)
    disk = ckd.read_disk(disk_file)
    table = Path(__file__).with_name('ibm1047.table').read_text().splitlines()
    values = [int(value, 16) for line in table if not line.startswith('#')
              for value in line.split()]
    need(len(values) == 256, 'invalid IBM1047 table')
    encoder = {codepoint: byte for byte, codepoint in enumerate(values)}
    required = 2 if profile == 24 else 3
    need(len(names) == required and all(re.fullmatch(r'[A-Z0-9]{1,8}', n)
                                        for n in names),
         f'expected {required} CMS file names')
    expected = {
        f'CMS{profile}.' + names[0] + '.D':
            [bytes(encoder[ord(c)] for c in line)
             for line in ('Native café! [] ^', ' ', 'end')],
        f'CMS{profile}.' + names[1] + '.D': [bytes(range(256))],
    }
    if profile == 31:
        expected[f'CMS{profile}.' + names[2] + '.D'] = [bytes((0x40,))]
    results = []
    for dataset, wanted in expected.items():
        _, dscb, _, _ = ckd.dscb(disk, dataset)
        need(dscb[84] == 0x90 and int.from_bytes(dscb[86:90], 'big') ==
             (BLOCK << 16) + BLOCK, 'output dataset geometry mismatch')
        data = ckd.seq(disk, dataset)
        need(len(data) >= ENVELOPE and data[:8] == b'PDCMSF01'
             and word(data, 8) == profile, 'output envelope mismatch')
        length, source_bytes, count = (word(data, at) for at in (12, 16, 20))
        need(length > 0 and ENVELOPE + length <= len(data)
             and not any(data[ENVELOPE + length:]),
             'output length or padding mismatch')
        framed = data[ENVELOPE:ENVELOPE + length]
        need(fnv32(framed) == word(data, 56), 'output FNV mismatch')
        parts, at = [], 0
        while at < len(framed):
            need(at + 2 <= len(framed), 'truncated output record')
            size = int.from_bytes(framed[at:at + 2], 'big')
            at += 2
            need(1 <= size <= 256 and at + size <= len(framed),
                 'invalid output record')
            parts.append(framed[at:at + size])
            at += size
        need(parts == wanted and count == len(parts)
             and source_bytes == sum(map(len, parts)),
             'output logical records differ: ' + dataset)
        results.append(dict(dataset=dataset, records=count,
                            source_bytes=source_bytes,
                            framed_sha256=digest(framed)))
        print(f'PASS {dataset}: {count} exact logical records')
    receipt_file.write_text(json.dumps(dict(format='pdos-cms-output-v1',
                                            profile=profile, files=results),
                                       indent=2) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='mode', required=True)
    p = sub.add_parser('inspect')
    p.add_argument('archive', type=Path)
    p.add_argument('sha256')
    p.add_argument('out', type=Path)
    p = sub.add_parser('stage')
    p.add_argument('archive', type=Path)
    p.add_argument('contract', type=Path)
    p.add_argument('out', type=Path)
    p = sub.add_parser('verify-stage')
    p.add_argument('disk', type=Path)
    p.add_argument('stage', type=Path)
    p = sub.add_parser('verify-output')
    p.add_argument('disk', type=Path)
    p.add_argument('receipt', type=Path)
    p.add_argument('names', nargs='*',
                   help='text binary [empty] CMS file names, without .D')
    p.add_argument('--profile', type=int, choices=(24, 31), default=31)
    args = parser.parse_args()
    try:
        if args.mode == 'inspect':
            inspect(args.archive, args.sha256, args.out)
        elif args.mode == 'stage':
            stage(args.archive, args.contract, args.out)
        elif args.mode == 'verify-stage':
            verify_stage(args.disk, args.stage)
        else:
            defaults = ['B3TEXT', 'B3BIN', 'B3EMP'] if args.profile == 31 \
                else ['C24TXT', 'C24BIN']
            verify_output(args.disk, args.receipt,
                          args.names or defaults, args.profile)
    except (OSError, ValueError, zipfile.BadZipFile) as exc:
        parser.exit(1, f'CMS MODULE validation failed: {exc}\n')


if __name__ == '__main__':
    main()
