"""Independent shared-package spool and exact/portable native output checks."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
from job_actor import decode
from console_machine import text1047

RC=[[0],[0,0,0,0],[0,0,0],[0,0],[2,None],[2],[8],[16],[0],[0,0,0],[0,0,0]]
def inspect(out,raw):
    magic,_,_,_,_,_,_,real,failed=struct.unpack_from(">9I",raw,0x3ba00)
    if magic!=0x50444a32 or not 0x1000000<=real<=len(raw)-262144:raise ValueError("SDK spool missing")
    data=raw[real:real+262144];spool=decode(data)
    (out/"job-spool.bin").write_bytes(data);(out/"job-spool.json").write_text(json.dumps(spool,indent=2)+"\n")
    printed=(out/"job-first.txt").read_bytes().decode("latin1").splitlines()
    status=[x for x in printed if x.startswith(("JOB BEGIN ","STEP BEGIN ","STEP OS ","STEP RC ","STEP SKIP ","JOB END STATE "))]
    expected=[]
    for job,steps in enumerate(RC,1):
        expected.append(f"JOB BEGIN {job}")
        for step,rc in enumerate(steps,1):
            if rc is None:continue
            expected.extend([f"STEP BEGIN {step}","STEP OS 0",f"STEP RC {rc}"])
        expected.append("JOB END STATE 4")
    outputs=[x for x in printed if x.startswith("BATCH ") or x.startswith("PD033 CREXX BATCH ")]
    return {"sdk_exact_job_step_results":status==expected,
            "sdk_probe_text_and_binary_markers":outputs==["BATCH STDOUT café! [] ^","BATCH STDERR PASS","BATCH INPUT BINARY MISSING PASS","PD033 CREXX BATCH 42","PD033 CREXX BATCH 42","BATCH ADDRESS TSO PASS"],
            "sdk_crexx_address_command_output":printed.count("PD033 CREXX ADDRESS TSO PASS")==1 and sum(x.startswith("TIME ") for x in printed)==2,
            "sdk_native_compiler_exit_parse":printed.count("PD033 CREXX PARSE PASS")==1,
            "sdk_all_jobs_purged":spool["last_id"]==11 and not spool["jobs"] and not spool["active_id"],
            "sdk_spool_commit_succeeded":failed==0,
            "sdk_native_versions_and_linker_help":sum(x.startswith("crexx-1.0.0-beta.3+local.g78761cc5affe") for x in printed)==3 and "cREXX Linker" in printed,
            "sdk_interrupt_flags_clear":struct.unpack_from(">3I",raw,0x4fb4)==(0,0,0)}

def logical(item):
    result=bytearray()
    for value in item["native_blocks_hex"]:
        b=bytes.fromhex(value)
        if len(b)<4 or struct.unpack_from(">HH",b)!=(len(b),0):raise ValueError("VB BDW")
        at=4
        while at<len(b):
            if at+4>len(b):raise ValueError("short RDW")
            n,z=struct.unpack_from(">HH",b,at)
            if n<4 or z or at+n>len(b):raise ValueError("VB RDW")
            result.extend(b[at+4:at+n]);at+=n
    return bytes(result)

def stored(out,files):
    named={}
    for x in files:
        if x["kind"]==0x54534f and not x["erased"]:
            key=bytes.fromhex(x["key_hex"]);named[text1047(key[:44]).rstrip()+"("+text1047(key[44:]).rstrip()+")"]=x
    want={"PD4.RESULT()","PD4.OBJ()","PD4.LINK()","PD4.AOBJ()","PD4.POBJ()"}
    checks={"sdk_named_output_set_exact":set(named)==want}
    details={}
    if checks["sdk_named_output_set_exact"]:
        for dsn,item in named.items():
            data=logical(item);details[dsn]={"logical_sha256":hashlib.sha256(data).hexdigest(),"logical_bytes":len(data),"native_sha256":item["sha256"]}
            if dsn=="PD4.RESULT()":checks["sdk_exact_ordered_binary_result"]=data==bytes(range(256))
            elif dsn in ("PD4.OBJ()","PD4.LINK()"):
                file=out/("fresh.rxbin" if "OBJ" in dsn else "linked.rxbin");file.write_bytes(data)
                vm="/Users/adrian/CLionProjects/CREXX-level-l-inline-storage/cmake-build-levell-integration/bin/rxbvm"
                r=subprocess.run([vm,str(file),"-a","SHARED"],capture_output=True,timeout=30)
                checks["sdk_portable_"+("fresh" if "OBJ" in dsn else "linked")+"_exec"]=r.returncode==0 and r.stdout==b"PD033 CREXX BATCH 42\n"
    checks["sdk_temporary_data_not_persistent"]=all(x["kind"] in (0x54534f,0x50444a32,0x5443) for x in files)
    (out/"sdk-named-outputs.json").write_text(json.dumps(details,indent=2)+"\n")
    return checks
