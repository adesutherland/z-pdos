#!/usr/bin/env python3
"""Independent record checks for stopped normal-workload store receipts."""
import json
from pathlib import Path
import struct
import sys

def verify(stored,profile):
    files=[f for f in stored['files'] if not f['erased']]
    def cms(name,kind):
        key=(name[0].ljust(8)+name[1].ljust(8)+'A1').encode('cp037').hex()
        match=[f for f in files if f['kind']==kind and f['key_hex']==key]
        return [bytes.fromhex(v) for v in match[0]['records_hex']] if len(match)==1 else None
    def tso(suffix,member):
        key=('B3IO.'+suffix).ljust(44).encode('cp037')+member.ljust(8).encode('cp037')
        match=[f for f in files if f['kind']==0x54534f and f['key_hex']==key.hex()]
        if len(match)!=1:return None
        records=[]
        for value in match[0]['native_blocks_hex']:
            block=bytes.fromhex(value)
            if len(block)<4 or struct.unpack_from('>H',block)[0]!=len(block) or block[2:4]!=bytes(2):raise ValueError('native BDW')
            at=4
            while at<len(block):
                size=struct.unpack_from('>H',block,at)[0]
                if size<4 or size>len(block)-at or block[at+2:at+4]!=bytes(2):raise ValueError('native RDW')
                records.append(block[at+4:at+size]);at+=size
        return records
    if profile=='cms31':
        text=cms(('T','TXT'),31);binary=cms(('B','BIN'),31);empty=cms(('E','TXT'),31)
        expected=[bytes.fromhex('d581a389a58540838186515a40adbd405f'),b'\x40',bytes.fromhex('859584')]
        # Native variable CMS records represent an empty line by one blank.
        checks={'exact_native_text_records':text==expected,'exact_opaque_binary_00_ff':binary is not None and b''.join(binary)==bytes(range(256)),'native_empty_file_record':empty==[b'\x40']}
    elif profile=='cms24':
        text=cms(('QTEXT','D'),24);binary=cms(('QBIN','D'),24)
        checks={'cms24_text_present':text is not None,'exact_opaque_binary_00_ff':binary is not None and b''.join(binary)==bytes(range(256))}
    else:
        text=tso('RXAS','TEXT');binary=tso('RXBIN','BYTES');empty=tso('RXAS','EMPTY')
        checks={'exact_native_text_records':text==[bytes.fromhex('d581a389a58540838186515a40adbd405f'),b'',bytes.fromhex('859584')],'exact_opaque_binary_00_ff':binary is not None and b''.join(binary)==bytes(range(256)),'zero_record_empty_file':empty==[]}
    return checks
if __name__=='__main__':
    checks=verify(json.loads(Path(sys.argv[1]).read_text()),sys.argv[2]);print(json.dumps(checks));raise SystemExit(0 if all(checks.values()) else 1)
