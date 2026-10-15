"""Actual card/form/restart IVP observations, printer and stopped VB oracle.

This is the existing binary/terminal interface, not a second batch controller.
The cREXX recipe and operator own its lifecycle.
"""
import hashlib
import re
import struct
from job_actor import decode, snapshot

MARKER = 'PD033 STAGE4 LIBRARY new payload'
LINK_SHA = '7d6d8bf1520b6c55a37c6a93bd14c13cb03700d74c842f7cf1033c0f25e68f95'

def inspect(out, raw, resume=False):
    from console_machine import parse_transcript
    words=struct.unpack_from('>9I',raw,0x3ba00)
    if words[0]!=0x50444a32 or words[8]:raise ValueError('IVP K spool snapshot')
    data=snapshot(raw,words[7]);spool=decode(data)
    (out/'job-spool.bin').write_bytes(data)
    events=parse_transcript(raw)['events']
    texts=[e['text'] for e in events if e['type']==2]
    revisions={};current=None
    for text in texts:
        match=re.fullmatch(r'BATCH / (SUBMIT|JOBS|OUTPUT|JCL PREVIEW) / REVISION (\d+)',text)
        if match:
            current=int(match[2]);revisions[current]=[]
        elif text.startswith('PCOMM END '):current=None
        elif current is not None:revisions[current].append(text.rstrip())
    job=2 if resume else 1
    lines=(out/'job-first.txt').read_bytes().decode('latin1').splitlines()
    records=[];labels=len(lines)%2==0
    for i in range(0,len(lines)-1,2):
        match=re.fullmatch(r'JOB (\d+) STEP ([A-Z0-9.]+) DD ([A-Z0-9]+) SEQ (\d+)',lines[i])
        if not match or int(match[1])!=job or int(match[4])!=len(records)+1:
            labels=False;continue
        records.append((match[2],match[3],lines[i+1]))
    rcs=[(step,int(text[8:])) for step,dd,text in records if dd=='JESMSGLG' and text.startswith('STEP RC ')]
    expected=[('BUILD.'+step,0) for step in ('COMP','ASSEM','LINK','RUN')]
    state_ok=not spool['jobs'] if resume else len(spool['jobs'])==1 and spool['jobs'][0]['header'][0]==2 and spool['jobs'][0]['header'][2]==4 and spool['jobs'][0]['header'][7]==0
    checks={
        'ivp_exact_job_identity_and_retention':spool['last_id']==2 and not spool['active_id'] and bool(state_ok),
        'ivp_all_physical_form_interactions':set(revisions)==set(range(10 if resume else 11)),
        'ivp_actual_four_step_printer_outcome':bool(labels) and rcs==expected and sum(text==MARKER for _,_,text in records)==1 and sum(text=='JOB END STATE 4' for _,_,text in records)==1,
        'ivp_repeatable_filtered_form_output':MARKER in revisions.get(6 if resume else 9,[]),
        'ivp_interrupt_owners_released':struct.unpack_from('>3I',raw,0x4fb4)==(0,0,0),
    }
    if resume:
        from console_machine import text1047
        begins=[text1047(bytes.fromhex(e['payload_hex'])).rstrip() for e in events if e['type']==1 and e['encoding']==1047]
        # Besides no explicit SUBMIT/RELEASE, require no automatic compiler
        # invocation and only the deliberately requested RXVM -v invocation.
        checks['ivp_restart_did_not_submit_or_reexecute']=not any('SUBMITTED JOB' in text for text in texts) and not any(text.startswith('PCOMM BEGIN ') and ('SUBMIT' in text or 'RELEASE' in text) for text in texts) and not any(name in ('RXC','RXAS','RXLINK') for name in begins) and begins.count('RXVM')==1
    else:
        checks['ivp_source_visible']=any('changestr(' in text for text in texts) and any('options levelb' in text for text in texts)
        checks['ivp_real_3505_deck_bound']='reader-LGO' in (out/'job-actions.jsonl').read_text() and (out/'machine.cnf').read_text().count('000C 3505')==1
        checks['ivp_preview_before_publication']=any('PREVIEW 1:' in text for text in revisions.get(4,[])) and any('JOB 2 STATE=4' in text for text in revisions.get(5,[]))
    return checks

def stored(out, files):
    from sdk_actor import logical
    from console_machine import text1047
    live=[item for item in files if not item['erased']]
    spools=[item for item in live if item['kind']==0x50444a32]
    named={}
    for item in live:
        if item['kind']==0x54534f:
            key=bytes.fromhex(item['key_hex'])
            if text1047(key[44:]).strip():raise ValueError('unexpected IVP member output')
            named[text1047(key[:44]).rstrip()]=logical(item)
    body=named.get('LABA01.B33.GOLINK',b'')
    return {
        'ivp_durable_spool_matches_memory':len(spools)==1 and spools[0]['sha256']==hashlib.sha256((out/'job-spool.bin').read_bytes()).hexdigest(),
        'ivp_only_intended_named_output':set(named)=={'LABA01.B33.GOLINK'},
        'ivp_binary_matches_native_reference':len(body)==6018 and hashlib.sha256(body).hexdigest()==LINK_SHA,
        'ivp_no_temporary_output_residue':all(item['kind'] in (0x54534f,0x50444a32,0x5443) for item in live),
    }
