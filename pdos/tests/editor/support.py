"""Independent editor fixture, physical field and native-record oracles.

No process, lease, guest, image or build operations occur on import.
"""
from pathlib import Path
import hashlib
import struct

ROOT = Path(__file__).resolve().parents[3]
TABLE = [int(x, 16) for row in (ROOT / 'pdos/scripts/ibm1047.table').read_text().splitlines()
         if not row.startswith('#') for x in row.split()]
if len(TABLE) != 256 or len(set(TABLE)) != 256:
    raise ValueError('bijective IBM1047 table required')
ENCODE = {chr(value): byte for byte, value in enumerate(TABLE)}
HLQ = 'LABA01.B33'
SEED = HLQ + '.SRC(EDSEED)'
SAVED = HLQ + '.SRC(EDSAVE)'
RESULT = HLQ + '.RESULT'
MARKER = 'PD033 EDITOR LIBRARY new payload'
OLD = 'PD033 STAGE4 LIBRARY'
NEW = 'PD033 EDITOR LIBRARY'
CODED = bytes.fromhex('40c1c2c3c4c5c6c7c8c94a4b4c4d4e4f50d1d2d3d4d5d6d7d8d95a5b5c5d5e5f6061e2e3e4e5e6e7e8e96a6b6c6d6e6ff0f1f2f3f4f5f6f7f8f97a7b7c7d7e7f')


def encode(text):
    return bytes(ENCODE[c] for c in text)


def decode(data):
    return ''.join(chr(TABLE[b]) for b in data)


def seed_records(source=None):
    source = source or ROOT / 'build/pdos/batch-jcl/fixtures/pdos/SRC-LIBRARY.fb80'
    data = source.read_bytes()
    if len(data) != 13 * 80 or sum(OLD in decode(data[i:i+80]) for i in range(0, len(data), 80)) != 1:
        raise ValueError('exact 13-record canonical LIBRARY source required')
    return [data[i:i+80] for i in range(0, len(data), 80)] + [
        encode('/* editor page guard %02d */' % i).ljust(80, b'\x40') for i in range(1, 23)]


def saved_records(source=None):
    result = seed_records(source)
    for i, record in enumerate(result):
        if OLD in decode(record):
            result[i] = encode(decode(record).rstrip().replace(OLD, NEW)).ljust(80, b'\x40')
    return result


def owner_cards():
    rows=(Path(__file__).resolve().parent/'OWNER.jcl').read_text().splitlines()
    if not rows or any(not row or len(row)>80 for row in rows):
        raise ValueError('complete bounded OWNER FB80 source required')
    return b''.join(encode(row).ljust(80,b'\x40') for row in rows)


def native_records(item, recfm, lrecl, blksize):
    """Keep record boundaries, including sole blank VB records and FB padding."""
    result = []
    for value in item['native_blocks_hex']:
        block = bytes.fromhex(value)
        if not block or len(block) > blksize:
            raise ValueError('native block bound')
        if recfm == 'FB':
            if not lrecl or len(block) % lrecl:
                raise ValueError('incomplete fixed record')
            result.extend(block[i:i+lrecl] for i in range(0, len(block), lrecl))
        elif recfm == 'VB':
            if len(block) < 4 or struct.unpack_from('>HH', block) != (len(block), 0):
                raise ValueError('VB BDW')
            at = 4
            while at < len(block):
                if len(block)-at < 4:
                    raise ValueError('short RDW')
                n, reserved = struct.unpack_from('>HH', block, at)
                if n < 4 or n > lrecl or reserved or at+n > len(block):
                    raise ValueError('VB RDW')
                result.append(block[at+4:at+n]); at += n
        else:
            raise ValueError('explicit FB/VB profile required')
    return result


def named(files):
    result = {}
    for item in files:
        if item['erased'] or item['kind'] != 0x54534f:
            continue
        key = bytes.fromhex(item['key_hex'])
        if len(key) != 52:
            raise ValueError('native journal key')
        dsn, member = decode(key[:44]).rstrip(), decode(key[44:]).rstrip()
        name = dsn + ('(' + member + ')' if member else '')
        if name in result:
            raise ValueError('duplicate live native key')
        result[name] = item
    return result


def address(raw):
    if len(raw) != 2:
        raise ValueError('complete SBA required')
    if raw[0] & 0xc0:
        try:
            return CODED.index(raw[0]) * 64 + CODED.index(raw[1])
        except ValueError as exc:
            raise ValueError('invalid coded address') from exc
    return (raw[0] & 0x3f) * 256 + raw[1]


def write_frame(record, rows, columns):
    """Decode the actual EDIT Write EOR, with explicit protected-field cells."""
    if len(record) < 2 or record[0] != 0xf1:
        raise ValueError('EDIT Write record required')
    cells, fields, cursor = {}, {}, None
    pos, at, size = 0, 2, rows * columns
    while at < len(record):
        byte = record[at]; at += 1
        if byte == 0x11:
            pos = address(record[at:at+2]); at += 2
        elif byte == 0x1d:
            if at == len(record):
                raise ValueError('short SF')
            fields[pos] = record[at]; at += 1
            cells[pos] = None; pos += 1
        elif byte == 0x13:
            cursor = pos
        elif byte == 0x28:
            if at+2 > len(record):
                raise ValueError('short SA')
            at += 2
        else:
            if byte in (0x3c, 0x12, 0x29, 0x2c):
                raise ValueError('unexpected EDIT order; extend oracle explicitly')
            cells[pos] = byte; pos += 1
        if not 0 <= pos <= size:
            raise ValueError('screen address outside geometry')
    if set(cells) != set(range(size)):
        raise ValueError('complete physical editor repaint required')
    return {'cells': cells, 'fields': fields, 'cursor': cursor,
            'unlocked': bool(record[1] & 2), 'sha256': hashlib.sha256(record).hexdigest()}


def field_contract(frame, rows, columns):
    fields = frame['fields']
    for row in range(2, rows-3):
        expected = {0: False, 3: True, 9: False, columns-1: True}
        for col, protected in expected.items():
            if (row*columns+col not in fields or
                    bool(fields[row*columns+col] & 0x20) != protected):
                return False
    return ((rows-3)*columns+6 in fields and
            not fields[(rows-3)*columns+6] & 0x20 and
            all(fields[row*columns] & 0x20 for row in (0, 1, rows-2, rows-1)))


def store_checks(before, after, line=False):
    live = named(after['files'])
    wanted = {SEED, SAVED, RESULT}
    # CRXLGO passes &&WORK.L to RUN and deletes it there. The exact changed
    # SYSPRINT marker proves execution; no permanent binary was requested.
    checks = {'editor_named_outputs_exact': set(live) == wanted,
              'editor_outside_store_unchanged': before['outside_store_sha256'] == after['outside_store_sha256'],
              'editor_no_temporary_outputs': all(x['kind'] in (0x54534f,0x50444a32,0x5443)
                                               for x in after['files'] if not x['erased'])}
    for name, records in ((SEED, seed_records()), (SAVED, saved_records())):
        checks['editor_exact_'+('seed' if name==SEED else 'saved')] = name in live and native_records(live[name],'FB',80,3120)==records
    ps = [b'', encode('  padded  '), encode('ascii!' if line else 'café!')]
    checks['editor_exact_ps_records'] = RESULT in live and native_records(live[RESULT],'VB',260,264)==ps
    return checks


def batch_checks(raw, expected_id):
    """Actual K snapshot, exact four step returns and changed run DD marker."""
    import sys
    sys.path.insert(0,str(ROOT/'pdos/tests/batch'))
    from job_actor import snapshot, decode as decode_spool
    words=struct.unpack_from('>9I',raw,0x3ba00)
    if words[0]!=0x50444a32:
        raise ValueError('K spool receipt absent')
    spool=decode_spool(snapshot(raw,words[7]))
    jobs=[job for job in spool['jobs'] if job['header'][0]==expected_id]
    if len(jobs)!=1:
        raise ValueError('exact edited-source job absent')
    job=jobs[0]
    records=[(r['step'],decode(bytes.fromhex(r['dd_hex'])).rstrip(),
              decode(bytes.fromhex(r['payload_hex'])).rstrip()) for r in job['records']]
    return {'editor_job_identity_complete': spool['last_id']==expected_id and not spool['active_id'] and len(spool['jobs'])==1 and
            job['header'][2]==4 and job['header'][7]==0 and words[8]==0,
            'editor_four_native_step_rc_zero': [(s,t) for s,d,t in records if t.startswith('STEP RC ')]==[(i,'STEP RC 0') for i in range(1,5)],
            'editor_four_native_step_os_zero': [(s,t) for s,d,t in records if t.startswith('STEP OS ')]==[(i,'STEP OS 0') for i in range(1,5)],
            'editor_exact_executed_marker': sum((s,d,t)==(4,'SYSPRINT',MARKER) for s,d,t in records)==1}


def owner_checks(spool):
    jobs=[job for job in spool['jobs'] if job['header'][0]==1]
    if len(jobs)!=1:
        raise ValueError('retained ownership job1 absent')
    job=jobs[0]
    records=[(r['step'],decode(bytes.fromhex(r['dd_hex'])).rstrip(),decode(bytes.fromhex(r['payload_hex'])).rstrip()) for r in job['records']]
    return {'owner_job_retained': job['header'][2]==4 and job['header'][7]==0,
            'owner_native_os_rc_zero': [(s,t) for s,d,t in records if t.startswith(('STEP OS ','STEP RC '))]==[(1,'STEP OS 0'),(1,'STEP RC 0')],
            'owner_device_open_refused20': sum((s,d,t)==(1,'SYSPRINT','PD033 EDIT BATCH OWNER REFUSED 20') for s,d,t in records)==1}
