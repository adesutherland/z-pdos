#!/usr/bin/env python3
"""RDW/package and stopped-CKD binary adapter for the installed developer profile.

cREXX owns orchestration. This adapter never starts a guest or updates an
existing workspace. The operator supplies one checked four-tool package.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
TOOLS = ('RXC', 'RXAS', 'RXLINK', 'RXVM')
LIBS = {'LIBRARY': 'library.rxbin', 'RXFNSC': 'rxfnsc.rxbin',
        'RXFNSG': 'rxfnsg.rxbin', 'RXCEXITS': 'rxcexits.rxbin'}
APPS = ('edit', 'find', 'hex', 'cmp', 'hello', 'recio', 'panel', 'diskmap')
PROCS = ('CRXC', 'CRXCG', 'CRXCL', 'CRXLGO')
PROFILE = ROOT / 'pdos/src/profiles/crexx-development'
CYLS = 150


def digest(data):
    return hashlib.sha256(data).hexdigest()


def media_module():
    spec = importlib.util.spec_from_file_location('developer_ckd', Path(__file__).with_name('conformance-media.py'))
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    m.CYLS = CYLS
    m.DISK_SIZE = m.HEADER + m.TRACK * m.HEADS * CYLS
    return m


def package_inputs(package, expected):
    """Require the owning pipeline's exact manifest and all eight payloads."""
    package = package.resolve(strict=True)
    manifest = (package / 'batch-inputs.sha256').read_bytes()
    if not re.fullmatch('[0-9a-f]{64}', expected) or digest(manifest) != expected:
        raise ValueError('explicit checked batch-inputs.sha256 identity required')
    rows = {}
    for row in manifest.decode('utf-8').splitlines():
        sha, separator, path = row.partition('  ')
        if not separator or not re.fullmatch('[0-9a-f]{64}', sha) or path in rows:
            raise ValueError('invalid/duplicate package manifest entry')
        canonical = str(Path(path).resolve())
        if canonical in rows:
            raise ValueError("duplicate canonical package entry")
        rows[canonical] = sha
    inputs = {}
    for name in [str(Path(n) / (n + '.rdw')) for n in TOOLS] + ['shared/' + n for n in LIBS.values()]:
        path = (package / name).resolve(strict=True)
        if not path.is_relative_to(package) or rows.get(str(path)) != digest(path.read_bytes()):
            raise ValueError('package payload absent/changed: ' + name)
        if not path.stat().st_size:
            raise ValueError('empty package payload: ' + name)
        inputs[name] = rows[str(path)]
    entry = json.loads((package / 'batch-entry-check.json').read_text())
    if entry.get('amode') != 31 or entry.get('profile_bits') != 31 or entry.get('batch') is not True:
        raise ValueError('native TSO31 batch package required')
    return inputs


def fixed(text):
    m = media_module()
    values = [m.encoded(row, m.codec()) for row in text.splitlines()]
    if any(len(row) > 80 for row in values):
        raise ValueError('FB80 input too wide')
    return b''.join(row.ljust(80, b'\x40') for row in values)


def prepare(prior, command, apps, package, expected, output):
    inputs = package_inputs(package, expected)
    if output.exists() or output.is_symlink() or output.resolve() == prior.resolve():
        raise ValueError('fresh factory output required; preserve installed workspace')
    control = (prior / 'media/ctl.txt').read_text().splitlines()
    if control[0] != 'PDOS00 3390-1 100' or any(row.startswith(('PDOS.USER.', 'PDOS.CREXX.', 'CREXX.')) for row in control):
        raise ValueError('unchanged bare 100-cylinder K/U factory input required')
    control[0] = 'PDOS00 3390-1 150'
    output.mkdir(parents=True)
    media = output / 'media'
    media.mkdir()
    (output / 'base').mkdir()
    shutil.copy2(prior / 'launch.bin', output / 'launch.bin')
    for row in control:
        fields = row.split()
        if len(fields) > 2 and fields[1] == 'SEQ':
            source = prior / 'media' / fields[2]
            shutil.copy2(source, media / source.name)
    # Always select the newly built command processor, never the prior one.
    command_row = next(row.split() for row in control if row.startswith('U.COMMAND '))
    shutil.copy2(command / 'u-command.bin', media / command_row[2])
    additions, native = [], {}
    for name in TOOLS:
        stage = media / (name + '.bin')
        subprocess.run([str(apps / 'pack-tso'), str(package / name / (name + '.rdw')), str(stage)], check=True)
        cylinders = (stage.stat().st_size + 45 * 18452 - 1) // (45 * 18452) + 1
        additions.append(f'TSO31.{name} SEQ {stage.name} CYL {cylinders} 1 0 PS F 18452 18452')
        native['TSO31.' + name] = {'file': stage.name, 'sha256': digest(stage.read_bytes())}
    for name in APPS:
        stage = apps / (name + '.bin')
        shutil.copy2(stage, media / stage.name)
        additions.append(f'TSO31.{name.upper()} SEQ {stage.name} CYL 1 1 0 PS F 18452 18452')
        native['TSO31.' + name.upper()] = {'file': stage.name, 'sha256': digest(stage.read_bytes())}
    additions.append('TSO31.FC SEQ cmp.bin CYL 1 1 0 PS F 18452 18452')
    native['U.COMMAND'] = {'file': command_row[2], 'sha256': digest((media / command_row[2]).read_bytes())}
    members = {'PDOS.CREXX.RXBIN': [], 'PDOS.CREXX.PROCLIB': [], 'PDOS.USER.CREXX': [], 'PDOS.USER.RXBIN': []}
    profile_inputs = dict(inputs)
    for member, filename in LIBS.items():
        data = (package / 'shared' / filename).read_bytes()
        (media / filename).write_bytes(data)
        members['PDOS.CREXX.RXBIN'].append({'member': member, 'kind': 'binary', 'media_file': filename, 'sha256': digest(data)})
    for member in PROCS:
        filename = member + '.jcl'
        data = (ROOT / 'pdos/src/procs' / filename).read_bytes()
        (media / filename).write_bytes(data)
        profile_inputs['procs/' + filename] = digest(data)
        members['PDOS.CREXX.PROCLIB'].append({'member': member, 'kind': 'text', 'media_file': filename, 'sha256': digest(data)})
    batch_site = (PROFILE / 'BATCH.SITE').read_bytes()
    profile_inputs['BATCH.SITE'] = digest(batch_site)
    fingerprint = digest(json.dumps(profile_inputs, sort_keys=True, separators=(',', ':')).encode())
    developer_site = ('PD035 DEV 1\nSOURCE PDOS.USER.CREXX\nOUTPUT PDOS.USER.RXBIN\n'
                      'WORK PDOS.USER.WORK\nSTATE PDOS.USER.STATE\nPROFILE ' + fingerprint + '\n')
    (media / 'batch-site.fb80').write_bytes(fixed(batch_site.decode('utf-8')))
    (media / 'developer-site.fb80').write_bytes(fixed(developer_site))
    additions += ['PDOS.BATCH.SITE SEQ batch-site.fb80 TRK 1 1 0 PS FB 80 80',
                  'PDOS.DEV.SITE SEQ developer-site.fb80 TRK 1 1 0 PS FB 80 80',
                  'PDOS.CREXX.PROCLIB EMPTY CYL 1 1 0 PO FB 80 3120',
                  'PDOS.CREXX.RXBIN EMPTY CYL 20 1 0 PO VB 260 2640',
                  'PDOS.USER.CREXX EMPTY CYL 2 1 0 PO FB 80 3120',
                  'PDOS.USER.RXBIN EMPTY CYL 2 1 0 PO VB 260 264',
                  'PDOS.USER.WORK EMPTY CYL 2 1 0 PS VB 260 264',
                  'PDOS.USER.STATE EMPTY TRK 1 1 0 PS VB 260 264']
    at = next(i for i, row in enumerate(control) if row.startswith('PDOS.STORE '))
    names = [row.split()[0] for row in control[1:] + additions if row.strip() and not row.lstrip().startswith('#')]
    if len(names) != len(set(names)):
        raise ValueError('duplicate dataset in profile factory input')
    control[at:at] = additions
    (media / 'ctl.txt').write_text('\n'.join(control) + '\n')
    shutil.copy2(PROFILE / 'hercules.cnf', output / 'hercules.cnf')
    (output / 'reader.cards').write_bytes(b'')
    (output / 'developer.json').write_text(json.dumps({'version': 1, 'profile': fingerprint, 'package_manifest': expected,
        'inputs': profile_inputs, 'native': native, 'members': members, 'program_capacity': 2, 'cylinders': CYLS}, indent=2) + '\n')


def install(source, output, inputs):
    m = media_module()
    if output.exists() or source.resolve() == output.resolve():
        raise ValueError('fresh stopped factory output required')
    disk = bytearray(m.read_disk(source))
    before = bytes(disk)
    info = json.loads((inputs / 'developer.json').read_text())
    allowed = set()
    for dsn, item in info['native'].items():
        payload = (inputs / 'media' / item['file']).read_bytes()
        actual = m.seq(disk, dsn)
        if digest(payload) != item['sha256'] or actual[:len(payload)] != payload or any(actual[len(payload):]):
            raise ValueError('native disk readback failed: ' + dsn)
    for dsn, items in info['members'].items():
        location, _, base, end = m.dscb(disk, dsn)
        old = m.records(disk, *base)
        if any(data for number, (_, data) in old.items() if number):
            raise ValueError('fresh empty PDS required: ' + dsn)
        allowed.add(location[0] * 15 + location[1])
        allowed.update(range(base[0] * 15 + base[1], end[0] * 15 + end[1] + 1))
        m.write_track(disk, *base, {0: old[0], 1: (b'\xff' * 8, (b'\0\x0e' + b'\xff' * 8 + b'\0' * 4).ljust(256, b'\0')), 2: (b'', b'')})
        if items:
            m.stage_members(disk, dsn, items, inputs / 'media')
    for track in range(CYLS * 15):
        start = 512 + track * m.TRACK
        if track not in allowed and disk[start:start + m.TRACK] != before[start:start + m.TRACK]:
            raise ValueError('unrelated disk track changed')
    output.write_bytes(disk)


def verify(source, readback):
    m = media_module()
    a, b = m.read_disk(source), m.read_disk(readback)
    if (a[:20] != b[:20] or a[32:512] != b[32:512] or
            any(serial != bytes(12) and not serial.isdigit() for serial in (a[20:32], b[20:32]))):
        raise ValueError('container header changed')
    for c in range(CYLS):
        for h in range(15):
            if m.records(a, c, h) != m.records(b, c, h):
                raise ValueError('compressed CKD readback changed records')
    print('PD035 installed native tools/libraries/PROCs and empty user workspace: stopped readback PASS')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='action', required=True)
    prep = sub.add_parser('prepare')
    for name in ('prior', 'command', 'apps', 'package'):
        prep.add_argument(name, type=Path)
    prep.add_argument('manifest_sha256')
    prep.add_argument('output', type=Path)
    ins = sub.add_parser('install')
    for name in ('source', 'output', 'inputs'):
        ins.add_argument(name, type=Path)
    ver = sub.add_parser('verify')
    for name in ('source', 'readback'):
        ver.add_argument(name, type=Path)
    a = parser.parse_args()
    if a.action == 'prepare':
        prepare(a.prior, a.command, a.apps, a.package, a.manifest_sha256, a.output)
    elif a.action == 'install':
        install(a.source, a.output, a.inputs)
    else:
        verify(a.source, a.readback)


if __name__ == '__main__':
    main()
