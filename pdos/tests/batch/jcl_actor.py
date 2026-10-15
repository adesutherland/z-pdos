"""Stage 4 native console/device interface and independent stopped oracle."""
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import time
from job_actor import _commands, action as old_action, decode, snapshot

FIXTURES=Path(__file__).resolve().parents[3]/'build/pdos/batch-jcl/fixtures/pdos'
RCS=[[0,0],[0,0,0],[0,0,0],[0,0,0],[0,0,0],[0,0,0,0],
     [0,0,0,0],[2,None,None],[0,0,8],[8,None,None,None],
     [0,0,0],[0,0,0],[0],[0,0,0],[0,0],None,[0,0,0]]

def spin_ready(raw):
    spool=decode(raw)
    if spool['active_id']!=16 or len(spool['jobs'])!=1:return False
    job=spool['jobs'][0]
    if job['header'][0]!=16 or job['header'][2]!=3:return False
    return any(r['step']==1 and r['dd_hex']=='SYSPRINT'.encode('cp037').hex() and
               bytes.fromhex(r['payload_hex']).rstrip(b'\x40')==b'PD033 STAGE4 SPIN READY'.decode().encode('cp037')
               for r in job['records'])

def spin_interrupt(proc,log,out):
    from console_machine import stop_for_capture,REAL
    state=out/'spin-state.bin';body=out/'spin-spool.bin';ready=False
    def capture(path,address,count):
        if path.is_symlink():raise ValueError('refuse symlink capture')
        path.unlink(missing_ok=True)
        _commands(proc,log,out,'spin-capture',[f'savecore "{path}" {address:x} {address+count-1:x}'])
        raw=path.read_bytes()
        if len(raw)!=count:raise ValueError('complete bounded spin capture required')
        return raw
    end=time.monotonic()+300
    try:
        while time.monotonic()<end:
            stop_for_capture(proc,log)
            try:
                words=struct.unpack('>9I',capture(state,0x3ba00,36))
                if words[0]!=0x50444a32 or words[8]:raise ValueError('live job snapshot identity/commit')
                real=words[7]
                if not 0x1000000<=real<=REAL-294912:raise ValueError('live job snapshot address')
                ready=words[1]==16 and spin_ready(capture(body,real,294912))
            finally:
                _commands(proc,log,out,'spin-resume',['startall'])
            if ready:break
            time.sleep(.1)
        if not ready:raise TimeoutError('SPIN readiness absent from actual job SYSOUT')
        # Existing device action requires a fresh running, enabled 31-bit U
        # PSW after resume. Never infer execution from SUBMITTED or a sleep.
        old_action('interrupt',proc,log,out)
        (out/'spin-readiness.json').write_text(json.dumps({'job':16,'sysout_ready':True,'enabled_native_cpu':True})+'\n')
    finally:
        state.unlink(missing_ok=True);body.unlink(missing_ok=True)

def action(name,proc,log,out,client,script):
    if name=='spin-interrupt':return spin_interrupt(proc,log,out)
    if name.startswith('reader-'):
        member=name[7:]
        if member not in ('C','CG'):raise ValueError('selected owned reader deck')
        return _commands(proc,log,out,name,[f'devinit 000C {FIXTURES/(member+".fb80")} eof ebcdic'])
    if name in ('interrupt','printer-offline','printer-restore'):
        return old_action(name,proc,log,out)
    if name=='frame-CG':
        from console_machine import until,terminal_action
        cards=(FIXTURES/'CG.jcl').read_text().splitlines()+['END PROOF']
        for text in cards:
            if client:
                client.enter(text)
                # The 3215 service has no per-record prompt; allow its next
                # read to arm. Exact source fingerprints below detect loss.
                time.sleep(.05)
            else:
                until(lambda:terminal_action(script,'Query(KeyboardLock)').stdout.strip()=='false',proc,log,60)
                if terminal_action(script,'String('+json.dumps(text)+')').returncode or terminal_action(script,'Enter()').returncode:
                    raise RuntimeError('framed 3270 card input')
        return
    raise ValueError('selected JCL action required')

def inspect(out,raw):
    from console_machine import parse_transcript
    magic,_,_,_,_,_,_,real,failed=struct.unpack_from('>9I',raw,0x3ba00)
    if magic!=0x50444a32:raise ValueError('K JCL snapshot identity')
    data=snapshot(raw,real);spool=decode(data)
    (out/'job-spool.bin').write_bytes(data)
    texts=[e['text'] for e in parse_transcript(raw)['events'] if e['type']==2]
    sections={};current=None
    for text in texts:
        m=re.fullmatch(r'PCOMM BEGIN (\d+) (.*)',text)
        if m:current=m[2];sections.setdefault(current,[]).append([])
        elif text.startswith('PCOMM END '):current=None
        elif current:sections[current][-1].append(text)
    hashes=[]
    for id in (2,3,4):
        block=sections.get('STATUS '+str(id),[[]])[0]
        hashes.append([x for x in block if x.startswith(('SOURCE SHA256 ','PROC '))])
    expected_hash=hashlib.sha256((FIXTURES/'CG.fb80').read_bytes()).hexdigest()
    lines=(out/'job-first.txt').read_bytes().decode('latin1').splitlines()
    rows={};sequence={};labels=True
    if len(lines)%2:labels=False
    for i in range(0,len(lines)-1,2):
        m=re.fullmatch(r'JOB (\d+) STEP ([A-Z0-9.]+) DD ([A-Z0-9]+) SEQ (\d+)',lines[i])
        if not m:labels=False;continue
        job,step,dd,seq=int(m[1]),m[2],m[3],int(m[4])
        if seq!=sequence.get(job,0)+1:labels=False
        sequence[job]=seq;rows.setdefault(job,[]).append((step,dd,lines[i+1]))
    proof=out/'spin-readiness.json'
    spin=json.loads(proof.read_text()) if proof.is_file() else None
    results=True
    for job,rcs in enumerate(RCS,1):
        if rcs is None:
            results &= any(x[2]=='JOB END STATE 6' for x in rows.get(job,[]));continue
        actual=[int(x[2][8:]) for x in rows.get(job,[]) if x[1]=='JESMSGLG' and x[2].startswith('STEP RC ')]
        results &= actual==[x for x in rcs if x is not None]
        results &= any(x[2]=='JOB END STATE 4' for x in rows.get(job,[]))
    return {'jcl_spool_v4_all_identities_purged':spool['last_id']==17 and not spool['jobs'] and not spool['active_id'] and struct.unpack_from('>I',data,4)[0]==4,
            'jcl_three_routes_exact_source_and_proc_fingerprints':bool(hashes[0]) and hashes[0]==hashes[1]==hashes[2] and hashes[0][0]=='SOURCE SHA256 '+expected_hash,
            'jcl_labelled_ordered_printer_records':labels and set(rows)==set(range(1,18)),
            'jcl_expected_step_results_and_cpu_cancel':bool(results),
            'jcl_real_spooled_spin_before_cpu_interrupt':spin=={'job':16,'sysout_ready':True,'enabled_native_cpu':True} and (out/'cpu-interrupt-before.txt').is_file(),
            'jcl_held_output_policy_and_explicit_delivery':any('PRINT=0' in x for x in sections.get('STATUS 17',[[]])[0]) and any('OUTPUT POLICY=HELD' in x for x in sections.get('STATUS 17',[[]])[0]) and any('PRINT=2' in x for x in sections.get('STATUS 17',[[],[]])[-1]),
            'jcl_real_library_markers':sum(x[2]=='PD033 STAGE4 LIBRARY new payload' for rows0 in rows.values() for x in rows0)==8,
            'jcl_compiler_exit_parse_marker':sum(x[2]=='PD033 CREXX PARSE PASS' for rows0 in rows.values() for x in rows0)==1,
            'jcl_spool_commit_and_interrupt_cleanup':failed==0 and struct.unpack_from('>3I',raw,0x4fb4)==(0,0,0)}

def stored(out,files):
    from sdk_actor import logical
    from console_machine import text1047
    named={}
    for item in files:
        if item['kind']==0x54534f and not item['erased']:
            key=bytes.fromhex(item['key_hex']);name=text1047(key[:44]).rstrip()
            if text1047(key[44:]).strip():raise ValueError('unexpected named member output')
            named[name]=logical(item)
    wanted={'LABA01.B33.'+x for x in ('OBJ','LINK','GOLINK','SPIN','RESULT')}
    checks={'jcl_exact_named_output_set':set(named)==wanted,
            'jcl_exact_binary_probe':named.get('LABA01.B33.RESULT')==bytes(range(256)),
            'jcl_linked_output_equivalence':named.get('LABA01.B33.LINK')==named.get('LABA01.B33.GOLINK'),
            'jcl_no_temporary_store_objects':all(x['kind'] in (0x54534f,0x50444a32,0x5443) for x in files)}
    vm='/Users/adrian/CLionProjects/CREXX-level-l-inline-storage/cmake-build-levell-integration/bin/rxbvm'
    library='/Users/adrian/CLionProjects/CREXX-level-l-inline-storage/build/mainframe-batch/shared/library.rxbin'
    for suffix in ('OBJ','LINK','GOLINK'):
        body=named.get('LABA01.B33.'+suffix,b'');path=out/(suffix+'.rxbin');path.write_bytes(body)
        args=[vm,'--no-autoload',str(path)]+([library] if suffix=='OBJ' else [])+['-a','SHARED']
        r=subprocess.run(args,capture_output=True,timeout=30)
        checks['jcl_portable_'+suffix]=r.returncode==0 and r.stdout==b'PD033 STAGE4 LIBRARY new payload\n'
    summary={name:{'sha256':hashlib.sha256(body).hexdigest(),'bytes':len(body)} for name,body in named.items()}
    (out/'jcl-outputs.json').write_text(json.dumps(summary,indent=2)+'\n')
    return checks
