#!/usr/bin/env python3
"""Binary/ZIP interface for the P0 audit; cREXX owns the workflow."""
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys
import zipfile
sys.dont_write_bytecode=True

def module(path,name):
    spec=importlib.util.spec_from_file_location(name,path)
    loaded=importlib.util.module_from_spec(spec);spec.loader.exec_module(loaded)
    return loaded

def sha(raw):return hashlib.sha256(raw).hexdigest()

archive,extractor,out=map(Path,sys.argv[1:])
raw=archive.read_bytes()
if sha(raw)!="fd11ae260bba169653126a861ecae545cee9bcfbe1fad4083c4a913241e3aedd":
    raise ValueError("pinned beta3 release package required")
out.mkdir(exist_ok=False)
xmit=module(extractor,"p0_xmit")
cms=module(Path(__file__).resolve().parents[3]/"scripts/cms-module.py","p0_cms")
report={"package_sha256":sha(raw),"extractor_sha256":sha(extractor.read_bytes()),
        "cms":[],"tso":[]}
with zipfile.ZipFile(archive) as release:
    for name in sorted(release.namelist()):
        if name.endswith('.module') and name.split('/')[0] in ('cms24','cms31'):
            data=release.read(name);info=cms.parse_module(data,name.split('/')[0])
            info.update({"member":name,"module_sha256":sha(data)})
            report['cms'].append(info)
        if name.endswith('.XMI') and name.split('/')[0] in ('tso24','tso31','tso64-any','tso64-high'):
            profile=name.split('/')[0];member=Path(name).stem
            mode=24 if profile=='tso24' else 31 if profile=='tso31' else 64
            residence='24' if mode==24 else '64' if profile=='tso64-high' and member in ('RXCH','RXASH','RXVMH') else 'ANY'
            data=release.read(name);native,info=xmit.extract(data,member,mode,residence)
            path=out/(profile+'-'+member+'.rdw');path.write_bytes(native)
            info.update({'package_member':name,'xmi_sha256':sha(data),
                         'native_sha256':sha(native),'native_bytes':len(native)})
            info['entry_offset']=int.from_bytes(bytes.fromhex(info['directory_hex'])[49:52],'big')
            report['tso'].append(info)
if len(report['cms'])!=4 or len(report['tso'])!=13:
    raise ValueError('required six-profile application inventory is incomplete')
(out/'inventory.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({'cms':len(report['cms']),'tso':len(report['tso']),
                  'inventory_sha256':sha((out/'inventory.json').read_bytes())}))
