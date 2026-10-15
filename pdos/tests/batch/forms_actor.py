"""Initial form/3270 binary transcript, printer and stopped-spool oracle."""
import hashlib
import re
import struct
from job_actor import decode, snapshot

def inspect(out, raw):
    from console_machine import parse_transcript
    words=struct.unpack_from('>9I',raw,0x3ba00)
    if words[0]!=0x50444a32 or words[8]:raise ValueError('forms K spool snapshot')
    data=snapshot(raw,words[7]);spool=decode(data)
    (out/'job-spool.bin').write_bytes(data)
    texts=[e['text'] for e in parse_transcript(raw)['events'] if e['type']==2]
    revisions={};current=None
    for text in texts:
        m=re.fullmatch(r'BATCH / (SUBMIT|JOBS|OUTPUT|JCL PREVIEW) / REVISION (\d+)',text)
        if m:
            current=int(m[2]);revisions[current]=[]
        elif text.startswith('PCOMM END '):current=None
        elif current is not None:revisions[current].append(text.rstrip())
    lines=(out/'job-first.txt').read_bytes().decode('latin1').splitlines()
    labels=True;records=[]
    if len(lines)%2:labels=False
    for i in range(0,len(lines)-1,2):
        m=re.fullmatch(r'JOB 1 STEP ([A-Z0-9.]+) DD ([A-Z0-9]+) SEQ (\d+)',lines[i])
        if not m or int(m[3])!=len(records)+1:labels=False;continue
        records.append((m[1],m[2],lines[i+1]))
    rcs=[(step,int(text[8:])) for step,dd,text in records if dd=='JESMSGLG' and text.startswith('STEP RC ')]
    return {
      'forms_one_real_identity_no_duplicate':spool['last_id']==1 and not spool['active_id'] and not spool['jobs'],
      'forms_preview_and_repeat_guards':all(i in revisions for i in range(15)) and any('PREVIEW 1:' in x for x in revisions[3]) and any('No unused preview' in x for x in revisions[6]),
      'forms_runhold_before_dispatch':any('JOB 1 STATE=2 PRINT=0' in x for x in revisions[5]),
      'forms_scoped_PF8_pages_actual_RUN_result':any('STEP BUILD.RUN STATE=2 OS=0 RC=0' in x for x in revisions[8]),
      'forms_filtered_retained_output':any(x=='PD033 STAGE4 LIBRARY new payload' for x in revisions[11]),
      'forms_explicit_print_and_three_native_steps':labels and rcs==[('BUILD.COMP',0),('BUILD.ASSEM',0),('BUILD.RUN',0)] and sum(text=='PD033 STAGE4 LIBRARY new payload' for _,_,text in records)==1 and any('JOB 1 STATE=4 PRINT=2' in x for x in revisions[13]),
      'forms_no_interrupt_or_resource_residue':struct.unpack_from('>3I',raw,0x4fb4)==(0,0,0)
    }

def stored(out, files):
    spool=[f for f in files if not f['erased'] and f['kind']==0x50444a32]
    return {'forms_durable_spool_equals_stopped_state':len(spool)==1 and spool[0]['sha256']==hashlib.sha256((out/'job-spool.bin').read_bytes()).hexdigest(),
            'forms_no_temporary_or_named_output_residue':all(f['kind'] in (0x5443,0x50444a32) for f in files if not f['erased'])}
