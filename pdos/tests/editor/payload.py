"""Current EDIT envelope and independent stopped PS readback; no guest control.

The cREXX producer calls these interfaces only after operator review/GO.
Construction never changes the qualified BRXC/BRXAS/BRXLINK/BRXVM package.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
BLOCK = 18452


def sha(data):
    return hashlib.sha256(data).hexdigest()


def envelope(data):
    if not data or len(data) % BLOCK or data[:8] != b'PDTSO31\x01':
        raise ValueError('complete current PDTSO31 envelope required')
    size, count, expected, reserved = struct.unpack_from('>4I', data, 8)
    if not 644 <= size <= 6 * 1024 * 1024 or count != (size + 64 + BLOCK - 1) // BLOCK:
        raise ValueError('native EDIT envelope bounds')
    if count * BLOCK != len(data) or reserved or any(data[24:64]) or any(data[64 + size:]):
        raise ValueError('native EDIT envelope padding/count')
    value = 0x811c9dc5
    for byte in data[64:64 + size]:
        value = ((value ^ byte) * 0x01000193) & 0xffffffff
    if value != expected:
        raise ValueError('native EDIT payload checksum')
    return {'bytes': len(data), 'raw_bytes': size, 'blocks': count, 'sha256': sha(data)}


def prepare(media, editor, probe=False):
    media, editor = Path(media), Path(editor)
    if media.resolve() != ROOT / 'build/pdos/batch-sdk/image-inputs/media':
        raise ValueError('one fixed candidate input slot required')
    if editor.resolve() != ROOT / 'build/pdos/batch-sdk/apps/edit.bin' or editor.is_symlink():
        raise ValueError('current guarded apps/edit.bin required')
    body = editor.read_bytes()
    identity = envelope(body)
    control = media / 'ctl.txt'
    rows = control.read_text().splitlines()
    found = [i for i, row in enumerate(rows) if row.split()[:1] == ['TSO31.EDIT']]
    if len(found) != 1:
        raise ValueError('one inherited TSO31.EDIT binding required')
    fields = rows[found[0]].split()
    if len(fields) != 11 or fields[1:4] != ['SEQ', 'edit.bin', 'CYL'] or fields[7:] != ['PS', 'F', str(BLOCK), str(BLOCK)]:
        raise ValueError('unchanged native EDIT PS/F18452 route required')
    fields[4] = str(max(1, (identity['blocks'] + 1 + 44) // 45))
    rows[found[0]] = ' '.join(fields)
    target = media / 'edit.bin'
    if target.is_symlink() or control.is_symlink():
        raise ValueError('refuse symlink staging target')
    probe_data = None
    if probe:
        probe_path = ROOT / 'build/pdos/batch-sdk/apps/editor-probe/probe.bin'
        if probe_path.is_symlink() or any(row.split()[:1] == ['TSO31.TEFPROBE'] for row in rows):
            raise ValueError('fresh explicit editor probe binding required')
        probe_data = probe_path.read_bytes()
        probe_id = envelope(probe_data)
        site = media / 'SITE.fb80'
        owner_target = media / 'EDITOR-OWNER.fb80'
        if site.is_symlink() or (media / 'TEFPROBE.bin').exists() or owner_target.exists() or owner_target.is_symlink():
            raise ValueError('fresh probe targets required')
        sys.path.insert(0, str(Path(__file__).resolve().parent))
        from support import encode, owner_cards
        old = encode('BTEST31 BTEST31').ljust(80, b'\x40')
        site_data = site.read_bytes()
        if site_data.count(old) != 1:
            raise ValueError('one original BTEST31 site alias required')
        site_data = site_data.replace(old, encode('BTEST31 TEFPROBE').ljust(80, b'\x40'))
        owner_data = owner_cards()
        identity.update(probe=probe_id, owner_cards_sha256=sha(owner_data), owner_cards_bytes=len(owner_data))
        cylinders = max(1, (probe_id['blocks'] + 45) // 45)
        index = next(i for i, row in enumerate(rows) if row.startswith('PDOS.STORE '))
        rows.insert(index, f'TSO31.TEFPROBE SEQ TEFPROBE.bin CYL {cylinders} 1 0 PS F {BLOCK} {BLOCK}')
    target.write_bytes(body)
    if probe_data is not None:
        (media / 'TEFPROBE.bin').write_bytes(probe_data)
        owner_target.write_bytes(owner_data)
        site.write_bytes(site_data)
    control.write_text('\n'.join(rows) + '\n')
    (media / 'editor-payload.json').write_text(json.dumps(identity, indent=2) + '\n')


def ps_records(raw, dsn):
    """Independent count/key/data parser, separate from the image writer."""
    sys.path.insert(0, str(ROOT / 'pdos/tests/batch'))
    from dd_data import records, TRACK
    tracks = (len(raw) - 512) // TRACK
    if len(raw) != 512 + 2250 * TRACK or tracks != 2250:
        raise ValueError('selected 150-cylinder flat candidate required')
    matches = []
    key = dsn.ljust(44).encode('cp037')
    for track in range(15, 45):
        for k, data, _ in records(raw, *divmod(track, 15)).values():
            if k == key:
                matches.append(k + data)
    if len(matches) != 1:
        raise ValueError('unique staged EDIT DSCB required')
    d = matches[0]
    if len(d) != 140 or d[82:85] != b'\x40\x00\x80' or struct.unpack_from('>HH', d, 86) != (BLOCK, BLOCK):
        raise ValueError('native EDIT DCB differs from PS/F18452')
    c, h, ec, eh = struct.unpack_from('>4H', d, 107)
    if not 0 <= c * 15 + h <= ec * 15 + eh < tracks:
        raise ValueError('staged EDIT extent bounds')
    result = bytearray()
    for track in range(c * 15 + h, ec * 15 + eh + 1):
        for number, (k, data, _) in sorted(records(raw, *divmod(track, 15)).items()):
            if not number:
                continue
            if k:
                raise ValueError('keyed native EDIT block')
            if not data:
                return bytes(result)
            if len(data) != BLOCK:
                raise ValueError('native EDIT physical block length')
            result.extend(data)
    raise ValueError('native EDIT physical EOF absent')


def verify(work, herc, editor, probe=False):
    work, herc, editor = Path(work), Path(herc), Path(editor)
    if work.resolve() != ROOT / 'build/pdos/batch-sdk/image':
        raise ValueError('one fixed candidate image slot required')
    disk = work / 'media/pdos00.cckd'
    flat = work / 'editor-readback.ckd'
    if flat.exists() or flat.is_symlink():
        raise ValueError('fresh bounded readback slot required')
    if editor.resolve() != ROOT / 'build/pdos/batch-sdk/apps/edit.bin' or editor.is_symlink():
        raise ValueError('current guarded EDIT producer required')
    expected = editor.read_bytes()
    identity = envelope(expected)
    try:
        subprocess.run([str(herc / 'cckd2ckd'), '-q', '-cyls', '150', str(disk), str(flat)],
                       check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        actual = ps_records(flat.read_bytes(), 'TSO31.EDIT')
        if actual != expected or envelope(actual) != identity:
            raise ValueError('stopped native EDIT differs from current apps/edit.bin')
        if probe:
            expected_probe = (ROOT / 'build/pdos/batch-sdk/apps/editor-probe/probe.bin').read_bytes()
            actual_probe = ps_records(flat.read_bytes(), 'TSO31.TEFPROBE')
            if actual_probe != expected_probe or envelope(actual_probe) != envelope(expected_probe):
                raise ValueError('stopped TEFPROBE differs from current fixture')
            from support import owner_cards
            actual_cards=(ROOT/'build/pdos/batch-sdk/image-inputs/media/EDITOR-OWNER.fb80').read_bytes()
            if actual_cards!=owner_cards():raise ValueError('owned reader deck differs from source')
            identity.update(probe=envelope(actual_probe),owner_cards_sha256=sha(actual_cards),owner_cards_bytes=len(actual_cards))
        (work / 'editor-readback.json').write_text(json.dumps({
            'pass': True, 'dataset': 'TSO31.EDIT', 'producer': str(editor.resolve()),
            **identity, 'independent_physical_readback': True}, indent=2) + '\n')
    finally:
        flat.unlink(missing_ok=True)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    sub = p.add_subparsers(dest='action', required=True)
    a = sub.add_parser('prepare')
    a.add_argument('media', type=Path); a.add_argument('editor', type=Path)
    a.add_argument('--probe', action='store_true')
    a = sub.add_parser('verify')
    for name in ('work', 'herc', 'editor'):
        a.add_argument(name, type=Path)
    a.add_argument('--probe', action='store_true')
    a = p.parse_args()
    if a.action == 'prepare':
        prepare(a.media, a.editor, a.probe)
    else:
        verify(a.work, a.herc, a.editor, a.probe)


if __name__ == '__main__':
    main()
