"""Independent Stage 5 generated-card, printer and stopped-output oracle.

This extends the existing binary device interface. It reads actual stopped
state and records; response revisions only locate the observed interaction.
"""
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
from job_actor import decode, snapshot

FIXTURE = Path(__file__).with_name('forms-complete-commands.json')
NATIVE = {
    'OBJ': (2554, 'bfe9c456f2a88e17f681e53fb200674bae714f9407e1d29ff87d632f0f326123'),
    'LINK': (6018, '7d6d8bf1520b6c55a37c6a93bd14c13cb03700d74c842f7cf1033c0f25e68f95'),
    'GOLINK': (6018, '7d6d8bf1520b6c55a37c6a93bd14c13cb03700d74c842f7cf1033c0f25e68f95'),
}

def generated(proc):
    # Independently specified exact cards, rather than invoking the generator.
    rows = ['//BATCH JOB CLASS=A,MSGCLASS=A,REGION=0M',
            '//LIBS JCLLIB ORDER=LABA01.B33.PROCLIB',
            '//JOBLIB DD DSN=LABA01.B33.LOAD,DISP=SHR',
            '//BUILD EXEC '+proc+',', '// LIB=LABA01.B33,',
            '// VOL=PDOS00,']
    if proc in ('CRXC', 'CRXCL'):
        rows += ['// OUT=LABA01.B33.'+('OBJ' if proc=='CRXC' else 'LINK')]
    else:
        rows += ["// ARGS='-a SHARED'"]
    rows += ['//COMP.SRC DD DSN=LABA01.B33.SRC(LIBRARY),DISP=SHR']
    if proc=='CRXLGO':
        rows += ['//LINK.RXBIN DD DSN=LABA01.B33.GOLINK,DISP=OLD',
                 '//RUN.RXBIN DD DSN=LABA01.B33.GOLINK,DISP=SHR']
    rows += ['//']
    from jcl_fixture import fixed
    return fixed('\n'.join(rows)+'\n')

def inspect(out, raw):
    from console_machine import parse_transcript
    words=struct.unpack_from('>9I',raw,0x3ba00)
    if words[0]!=0x50444a32 or words[8]:raise ValueError('forms K spool snapshot')
    data=snapshot(raw,words[7]);spool=decode(data)
    (out/'job-spool.bin').write_bytes(data)
    texts=[e['text'] for e in parse_transcript(raw)['events'] if e['type']==2]
    revisions={};current=None
    for text in texts:
        match=re.fullmatch(r'BATCH / (SUBMIT|JOBS|OUTPUT|JCL PREVIEW) / REVISION (\d+)',text)
        if match:
            current=int(match[2]);revisions[current]=[match[1]]
        elif text.startswith('PCOMM END '):current=None
        elif current is not None:revisions[current].append(text.rstrip())
    actions=[row[1] for row in json.loads(FIXTURE.read_text())[1][2] if len(row)<3]
    def after(action, occurrence=0):
        index=[i for i,value in enumerate(actions) if value==action][occurrence]
        return revisions.get(index+1,[])
    def has(rows, fragment):return any(fragment in row for row in rows)
    lines=[]
    for name in ('job-first.txt','job-recovered.txt'):
        path=out/name
        if path.exists():lines+=path.read_bytes().decode('latin1').splitlines()
    labels=len(lines)%2==0;records={};sequence={}
    for i in range(0,len(lines)-1,2):
        match=re.fullmatch(r'JOB (\d+) STEP ([A-Z0-9.]+) DD ([A-Z0-9]+) SEQ (\d+)',lines[i])
        if not match:labels=False;continue
        job,step,dd,seq=int(match[1]),match[2],match[3],int(match[4])
        labels &= seq==sequence.get(job,0)+1
        sequence[job]=seq;records.setdefault(job,[]).append((step,dd,lines[i+1]))
    expected={1:[('BUILD.COMP',0),('BUILD.ASSEM',0)],
              2:[('BUILD.COMP',0),('BUILD.ASSEM',0),('BUILD.RUN',0)],
              3:[('BUILD.COMP',0),('BUILD.ASSEM',0),('BUILD.LINK',0)],
              4:[('BUILD.COMP',0),('BUILD.ASSEM',0),('BUILD.LINK',0),('BUILD.RUN',0)],
              5:[('BUILD.COMP',2)]}
    results=labels and set(records)==set(expected)
    for job,want in expected.items():
        rows=records.get(job,[])
        actual=[(step,int(text[8:])) for step,dd,text in rows if dd=='JESMSGLG' and text.startswith('STEP RC ')]
        results &= actual==want and sum(text=='JOB END STATE 4' for _,_,text in rows)==1
    rendered=[row for block in revisions.values() for row in block]
    hashes=['SOURCE SHA256 '+hashlib.sha256(generated(proc)).hexdigest()
            for proc in ('CRXC','CRXCG','CRXCL','CRXLGO')]
    long_edit=next(value for value in actions if value.startswith('DECK XXX'))
    print_results=[after('PRINT',i) for i in range(actions.count('PRINT'))]
    checks={
      'forms_complete_all_interactions_observed':set(revisions)==set(range(len(actions))),
      'forms_complete_no_duplicate_or_residue':spool['last_id']==7 and not spool['active_id'] and not spool['jobs'],
      'forms_complete_required_and_long_fields':has(after('PREVIEW'),'OUT dataset/member is required') and has(after('SUBMIT'),'No unused preview') and has(after(long_edit),'Field is too long') and after(long_edit)[0]=='JCL PREVIEW',
      'forms_complete_exact_generated_source_hashes':all(value in rendered for value in hashes),
      'forms_complete_four_procs_and_failed_compiler':bool(results),
      'forms_complete_two_real_library_executions':sum(text=='PD033 STAGE4 LIBRARY new payload' for rows in records.values() for _,_,text in rows)==2,
      'forms_complete_nonconsuming_filtered_review':all(has(after('REFRESH',i),'PD033 STAGE4 LIBRARY new payload') for i in range(actions.count('REFRESH'))),
      'forms_complete_invalid_filters_recover':has(after('STEP MISSING'),'BATCH COMMAND STATUS=8') and not has(after('STEP MISSING'),'PD033 STAGE4 LIBRARY new payload') and has(after('STEP BUILD.RUN',1),'PD033 STAGE4 LIBRARY new payload') and has(after('DD TOOLONG99'),'Field is too long; value and preview were preserved') and has(after('DD TOOLONG99'),'PD033 STAGE4 LIBRARY new payload'),
      'forms_complete_printer_failure_and_recovery':len(print_results)==6 and any(re.search(r'Job 2 action status [1-9]',row) for row in print_results[1]) and has(print_results[2],'Job 2 action status 0') and (out/'job-recovered.txt').is_file(),
      'forms_complete_busy_stale_cancelled_actions':has(after('RELEASE'),'action status 16') and has(after('CANCEL'),'action status 8') and has(after('PURGE',5),'action status 16') and has(after('CANCEL',1),'action status 0') and has(after('RELEASE',1),'action status 8'),
      'forms_complete_failed_publication_cannot_replay':has(after('SUBMIT',-2),'Attempt consumed; acknowledgement unavailable') and has(after('SUBMIT',-1),'No unused preview'),
      'forms_complete_interrupt_cleanup':struct.unpack_from('>3I',raw,0x4fb4)==(0,0,0),
    }
    (out/'forms-complete-summary.json').write_text(json.dumps({'last_id':spool['last_id'],'revisions':len(revisions),'step_results':expected,'generated_source_hashes':hashes},indent=2)+'\n')
    return checks

def stored(out, files):
    from sdk_actor import logical
    from console_machine import text1047
    named={}
    for item in files:
        if item['kind']==0x54534f and not item['erased']:
            key=bytes.fromhex(item['key_hex'])
            if text1047(key[44:]).strip():raise ValueError('unexpected member output')
            named[text1047(key[:44]).rstrip()]=logical(item)
    wanted={'LABA01.B33.'+suffix for suffix in NATIVE}
    checks={'forms_complete_exact_named_output_set':set(named)==wanted,
            'forms_complete_no_temporary_objects':all(item['kind'] in (0x54534f,0x50444a32,0x5443) for item in files if not item['erased'])}
    spool=[item for item in files if not item['erased'] and item['kind']==0x50444a32]
    checks['forms_complete_durable_spool']=len(spool)==1 and spool[0]['sha256']==hashlib.sha256((out/'job-spool.bin').read_bytes()).hexdigest()
    vm='/Users/adrian/CLionProjects/CREXX-level-l-inline-storage/cmake-build-levell-integration/bin/rxbvm'
    library='/Users/adrian/CLionProjects/CREXX-level-l-inline-storage/build/mainframe-batch/shared/library.rxbin'
    summary={}
    for suffix,(size,sha) in NATIVE.items():
        body=named.get('LABA01.B33.'+suffix,b'')
        summary[suffix]={'bytes':len(body),'sha256':hashlib.sha256(body).hexdigest()}
        checks['forms_complete_native_'+suffix]=len(body)==size and summary[suffix]['sha256']==sha
        path=out/(suffix+'.rxbin');path.write_bytes(body)
        result=subprocess.run([vm,'--no-autoload',str(path)]+([library] if suffix=='OBJ' else [])+['-a','SHARED'],capture_output=True,timeout=30)
        checks['forms_complete_portable_'+suffix]=result.returncode==0 and result.stdout==b'PD033 STAGE4 LIBRARY new payload\n'
        if suffix=='OBJ':
            result=subprocess.run([vm,'--no-autoload',str(path),'-a','SHARED'],capture_output=True,timeout=30)
            checks['forms_complete_object_requires_library']=result.returncode==11
    (out/'forms-complete-outputs.json').write_text(json.dumps(summary,indent=2)+'\n')
    return checks
