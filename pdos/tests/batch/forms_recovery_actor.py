"""Read-only oracle for physical editor/reconnect and native ownership proof."""
import hashlib
import json
import re
import struct
from job_actor import decode,snapshot

PARENT='PD033 OWNER PARENT REFUSED FORM=16 SHELL=16'
CHILD='PD033 OWNER CHILD REFUSED FORM=16 SHELL=16'
BATCH_PARENT='PD033 OWNER PARENT REFUSED FORM=20 SHELL=20'
BATCH_CHILD='PD033 OWNER CHILD PASS EXPECTED=20'
ADDRESS='BATCH ADDRESS TSO PASS'
DONE='PD033 OWNER NESTED AND ADDRESS PASS'
FOREGROUND='PD033 OWNER FOREGROUND CHILD PASS'
EDGE='PD033 LEFT EDGE '+'X'*100+'RIGHT EDGE PD033'

def inspect(out,raw):
    from console_machine import parse_transcript
    words=struct.unpack_from('>9I',raw,0x3ba00)
    if words[0]!=0x50444a32 or words[8]:raise ValueError('ownership spool snapshot identity/commit')
    data=snapshot(raw,words[7]);spool=decode(data)
    (out/'job-spool.bin').write_bytes(data)
    t=parse_transcript(raw);texts=[e['text'] for e in t['events'] if e['type']==2]
    sections={};current=None;revisions={};revision=None
    for text in texts:
        begin=re.fullmatch(r'PCOMM BEGIN (\d+) (.*)',text)
        form=re.fullmatch(r'BATCH / (SUBMIT|JOBS|OUTPUT|JCL PREVIEW) / REVISION (\d+)',text)
        if begin:current=int(begin[1]);sections[current]=[];revision=None
        elif text.startswith('PCOMM END '):current=None;revision=None
        elif current is not None:sections[current].append(text)
        if form:revision=int(form[2]);revisions[revision]=[]
        elif revision is not None:revisions[revision].append(text.rstrip())
    printed=(out/'job-first.txt').read_bytes().decode('latin1').splitlines()
    records=[];labels=len(printed)%2==0
    for i in range(0,len(printed)-1,2):
        m=re.fullmatch(r'JOB 1 STEP RUN DD ([A-Z0-9]+) SEQ (\d+)',printed[i])
        if not m or int(m[2])!=len(records)+1:labels=False;continue
        records.append((m[1],printed[i+1]))
    observations=json.loads((out/'form-observations.json').read_text())
    reconnects=json.loads((out/'primary-reconnects.json').read_text())
    def physical(prompt,needle):
        return any(x['prompt']==prompt and needle in (out/x['screen']).read_text() for x in observations)
    def in_revision(n,needle):return any(needle in x for x in revisions.get(n,[]))
    foreground=all(all(sections.get(n,[]).count(x)==1 for x in (PARENT,CHILD,'PD033 OWNER CHILD PASS EXPECTED=16',FOREGROUND)) and ADDRESS not in sections.get(n,[]) for n in (1,3))
    payloads=[x[1] for x in records]
    wire='\n'.join((out/name).read_text() for name in ('3270.trace','3270-reconnect-1.trace','3270-reconnect-2.trace'))
    return {
      'recovery_one_identity_purged':spool['last_id']==1 and not spool['active_id'] and not spool['jobs'],
      'recovery_real_native_owner_step':labels and payloads.count('STEP RC 0')==1 and payloads.count('STEP OS 0')==1 and payloads.count('JOB END STATE 4')==1,
      'recovery_child_and_ADDRESS_under_FORM':all(payloads.count(x)==1 for x in (BATCH_PARENT,BATCH_CHILD,ADDRESS,DONE,EDGE)) and any(x.startswith('TIME ') for x in payloads),
      'recovery_foreground_child_ownership':foreground,
      'recovery_prefilled_editor_real_draft':physical('EDIT CRXCG','EDIT CRXCG') and in_revision(3,'> DECK'),
      'recovery_prepared_primary_reconnect':len(reconnects)==2 and reconnects[0]['prompt']=='REVISION 6' and in_revision(7,'Form repainted; submission attempt identity is preserved') and in_revision(8,'//OWNER JOB'),
      'recovery_published_primary_reconnect':len(reconnects)==2 and reconnects[1]['prompt']=='REVISION 9' and in_revision(10,'JOB 1 STATE=2 PRINT=0') and in_revision(11,'No unused preview'),
      'recovery_real_FORM_keys_survive_nested_return':all(re.search(r'^> PF%d\('%key,wire,re.M) for key in (3,4,8)) and in_revision(13,'Attempt already consumed') and 15 in revisions,
      'recovery_horizontal_record_complete_nonconsuming':physical('REVISION 18','PD033 LEFT EDGE') and physical('REVISION 19','RIGHT EDGE PD033') and physical('REVISION 20','PD033 LEFT EDGE') and payloads.count(EDGE)==1,
      'recovery_explicit_print_and_purge':in_revision(22,'JOB 1 STATE=4 PRINT=2') and in_revision(23,'No retained jobs'),
      'recovery_ordinary_form_reentry':in_revision(24,'No retained jobs') and 'PCOMM END 4 RC=0' in texts,
      'recovery_no_native_ownership_residue':struct.unpack_from('>3I',raw,0x4fb4)==(0,0,0) and not t['read_owner'] and not t['screen_owner']}

def stored(out,files):
    spools=[x for x in files if not x['erased'] and x['kind']==0x50444a32]
    return {'recovery_durable_spool_matches':len(spools)==1 and spools[0]['sha256']==hashlib.sha256((out/'job-spool.bin').read_bytes()).hexdigest(),
            'recovery_no_named_or_temporary_output':all(x['kind'] in (0x5443,0x50444a32) for x in files if not x['erased'])}
