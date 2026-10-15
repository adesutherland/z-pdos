"""Binary IBM1047/FB80 and fresh stopped CKD interfaces for Stage 4.

Orchestration and lifecycle decisions remain in cREXX and with the operator.
No routine here connects to or operates a running guest.
"""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
from dd_data import records, track, TRACK

CASES = 'C CG CL LGO LGODEF BAD RC8 COND PARSE INLINE CPROBE SPINBLD SPIN NOPROC NOLIB WORK7 WORK8 QUOTA'.split()
PROCS = 'CRXC CRXCG CRXCL CRXLGO'.split()
SOURCES = 'LIBRARY BAD PARSE SPIN'.split()
HLQ = 'LABA01.B33'
OWNER_JCL = """//OWNER JOB CLASS=A,MSGCLASS=A,REGION=0M
//RUN EXEC PGM=BTEST31,PARM='BATCH',REGION=256M
//SYSIN DD DUMMY
//SYSPRINT DD SYSOUT=*,DCB=(RECFM=FB,LRECL=132,BLKSIZE=132)
//SYSTERM DD SYSOUT=*,DCB=(RECFM=FB,LRECL=132,BLKSIZE=132)
//SYSTSPRT DD SYSOUT=*,DCB=(RECFM=FB,LRECL=132,BLKSIZE=132)
//
"""

def fixed(text):
    table = Path('pdos/scripts/ibm1047.table').read_text().splitlines()
    scalars = [int(x,16) for row in table if not row.startswith('#') for x in row.split()]
    if len(scalars) != 256 or len(set(scalars)) != 256: raise ValueError('IBM1047 table')
    native = {chr(scalar): byte for byte,scalar in enumerate(scalars)}
    result = bytearray()
    for row in text.splitlines():
        if len(row) > 80: raise ValueError('source exceeds complete FB80 card')
        result += bytes(native[c] for c in row).ljust(80,b'\x40')
    if not result or len(result) > 40960: raise ValueError('bounded nonempty card source')
    return bytes(result)

def write(path,data):
    if path.is_symlink(): raise ValueError('refuse symlink output')
    path.write_bytes(data)

def encode(source,output):
    write(output,fixed(source.read_text()))

def site(root,hlq,volume):
    if hlq != HLQ or len(volume) != 6 or not volume.isalnum(): raise ValueError('selected site')
    text = Path('pdos/src/procs/BATCH.SITE').read_text()
    text = text.replace('PDOS.BATCH.LOAD',hlq+'.LOAD').replace('PDOS.BATCH.PROCLIB',hlq+'.PROCLIB')
    text = text.replace('PDOS.CREXX',hlq).replace('PDOS00',volume)
    write(root/'SITE.fb80',fixed(text))
    write(root/'SYSIN.fb80',fixed('Input café!\n\n'))
    write(root/'BYTES.fb256',bytes(range(256)))

def prepare(media,fixtures,owners=None):
    """Extend the checked SDK fresh-media manifest, never an installed disk."""
    if owners is not None and str(owners)!='forms-owner':raise ValueError('selected ownership fixture only')
    ctl = media/'ctl.txt'; rows = ctl.read_text().splitlines()
    if rows[0] != 'PDOS00 3390-1 150': raise ValueError('selected SDK geometry')
    rename = {'PD4.'+a:HLQ+'.'+b for a,b in
              [('OBJ','OBJ'),('LINK','LINK'),('AOBJ','GOLINK'),('POBJ','SPIN'),('BYTES','BYTES'),('RESULT','RESULT')]}
    seen = set(); result = []
    for row in rows:
        fields = row.split()
        if fields and fields[0] in rename:
            old = fields[0]; fields[0] = rename[old]; seen.add(old); row = ' '.join(fields)
        if fields and fields[0] == 'PDOS.STORE':
            for name in ('SITE','SYSIN'):
                data = (fixtures/(name+'.fb80')).read_bytes()
                if not data or len(data)%80: raise ValueError('complete native records')
                if name=='SITE' and owners is not None:
                    old=fixed('BTEST31 BTEST31\n');new=fixed('BTEST31 FOWNER\n')
                    if data.count(old)!=1:raise ValueError('one original BTEST31 site binding required')
                    data=data.replace(old,new)
                write(media/(name+'.fb80'),data)
                dsn = 'PDOS.BATCH.SITE' if name == 'SITE' else HLQ+'.SYSIN'
                result.append(f'{dsn} SEQ {name}.fb80 TRK 1 1 0 PS FB 80 3120')
            for name in ('PROCLIB','SRC','JCL'):
                result.append(f'{HLQ}.{name} EMPTY CYL 1 1 0 PO FB 80 3120')
        result.append(row)
    if seen != set(rename): raise ValueError('all and only SDK task outputs must be renamed')
    if len([r for r in result if r.startswith('PDOS.BATCH.SITE ')]) != 1: raise ValueError('one site binding')
    write(ctl, ('\n'.join(result)+'\n').encode())
    if owners is not None:write(media/'OWNER.fb80',fixed(OWNER_JCL))

def fb_pds(raw,dsn,items,writer,audit):
    loc,full,meta = writer.locate(raw,dsn)
    if full[82:85] != b'\x02\x00\x90' or full[86:90] != struct.pack('>HH',3120,80):
        raise ValueError('fresh FB80 PDS required: '+dsn)
    c,h = meta['base']; old = records(raw,c,h)
    if any(any(data) for n,(_,data,_) in old.items() if n): raise ValueError('fresh empty PDS only')
    out = writer.ExtentWriter(raw,meta,1); entries = {}
    for member,body in sorted(items.items(), key=lambda x:x[0].encode('cp037')):
        if not body or len(body)%80: raise ValueError('complete FB80 member')
        first = None
        for at in range(0,len(body),3120):
            ttr = out.put(body[at:at+3120])
            if first is None: first = ttr
        eof = out.put(b''); entries[member] = first
    out.finish()
    packed = b''.join(n.encode('cp037').ljust(8,b'\x40')+ttr+b'\0' for n,ttr in entries.items())+b'\xff'*8+b'\0'*4
    if len(packed)+2 > 256: raise ValueError('bounded one-block directory')
    directory = struct.pack('>H',len(packed)+2)+packed
    track(raw,c,h,{0:old[0][:2],1:(b'\xff'*8,directory.ljust(256,b'\0')),2:(b'',b'')})
    writer.update_lstar(raw,loc,eof)
    actual,_ = audit.member_directory(raw,meta)
    if set(actual) != set(items): raise ValueError('FB80 directory readback')
    for member,body in items.items():
        blocks,_ = audit.member_records(raw,meta,actual[member])
        if b''.join(blocks) != body or any(len(b)%80 for b in blocks): raise ValueError('FB80 exact readback')
    first = c*15+h; last = meta['end'][0]*15+meta['end'][1]
    record = records(raw,loc[0],loc[1])[loc[2]]
    return [(512+first*TRACK,512+(last+1)*TRACK),(record[2]+8+98,record[2]+8+101)]

def stage(work,herc,fixtures,lab,owners=None):
    if owners is not None and owners.resolve()!=(work.parent/(work.name+'-inputs')/'media').resolve():
        raise ValueError('selected derived ownership fixture path only')
    sys.path.insert(0,str(lab/'tools'))
    import pdos_host_stage as writer
    import pdos64_crexx_pds_audit as audit
    audit.CYLS = 150
    disk = work/'media/pdos00.cckd'; flat = work/'jcl-fixture.ckd'; new = work/'jcl-fixture.cckd'
    if flat.exists() or new.exists(): raise ValueError('unfinished binary fixture operation')
    subprocess.run([str(herc/'cckd2ckd'),'-q','-cyls','150',str(disk),str(flat)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    raw = bytearray(flat.read_bytes()); before = bytes(raw); spans = []
    groups = [('PROCLIB',{n:(fixtures/(n+'.fb80')).read_bytes() for n in PROCS}),
              ('SRC',{n:(fixtures/('SRC-'+n+'.fb80')).read_bytes() for n in SOURCES}),
              ('JCL',{n:(fixtures/(n+'.fb80')).read_bytes() for n in CASES})]
    if owners is not None:
        body=(owners/'OWNER.fb80').read_bytes()
        if body!=fixed(OWNER_JCL):raise ValueError('exact ownership deck required')
        groups[-1][1]['OWNER']=body
    for suffix,items in groups: spans += fb_pds(raw,HLQ+'.'+suffix,items,writer,audit)
    at = 0
    for lo,hi in sorted(spans):
        if before[at:lo] != raw[at:lo]: raise ValueError('unrelated staging bytes changed')
        at = hi
    if before[at:] != raw[at:]: raise ValueError('unrelated staging bytes changed')
    flat.write_bytes(raw)
    subprocess.run([str(herc/'ckd2cckd'),'-q','-bz2','-cyls','150',str(flat),str(new)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    new.replace(disk)
    # All fixture writes precede the immutable outside-STORE baseline.
    sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'two-space'))
    from store_check import inspect
    write(work/'store-before.json',(json.dumps(inspect(flat),indent=2)+'\n').encode())
    flat.unlink()
    print('PD033 fresh FB80 PROCLIB/SRC/JCL exact readback and unrelated-byte preservation PASS')

if __name__ == '__main__':
    action = sys.argv[1]
    if action == 'encode': encode(*(Path(x) for x in sys.argv[2:]))
    elif action == 'site': site(Path(sys.argv[2]),sys.argv[3],sys.argv[4])
    elif action == 'prepare': prepare(*(Path(x) for x in sys.argv[2:]))
    elif action == 'stage': stage(*(Path(x) for x in sys.argv[2:]))
    else: raise ValueError('selected binary interface required')
