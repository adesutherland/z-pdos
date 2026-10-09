"""Parse native PD-025 reports; clocks measure cost, fixed counts prove work."""
import re
import struct

def observations(texts):
    parts={}
    for text in texts:
        m=re.fullmatch(r"PD25 ([ABC]) ([0-9A-F]+)",text.rstrip())
        if m:
            if m[1] in parts:raise ValueError("duplicate PD25 report")
            parts[m[1]]=bytes.fromhex(m[2])
    if set(parts)!={"A","B","C"} or len(parts["A"])!=64 or len(parts["B"])!=64 or len(parts["C"])!=32:
        raise ValueError("complete native PD25 report")
    raw=parts["A"]+parts["B"];v=struct.unpack(">32I",raw)
    clock=lambda at:struct.unpack_from(">Q",raw,at)[0]
    cap_start,cap_end,calls=struct.unpack_from(">2QI",parts["C"])
    checks={"pd025_owned_u64_request_buffer":v[:4]==(1,128,0x50443235,40),
            "pd025_normal_ipl_profile":v[23]==1,
            "pd025_fixed_disk_operation_counts":v[12:15]==(120,40,18452),
            "pd025_grouped_disk_bytes_match":v[15]==1,
            "pd025_bulk_fixed_work":v[20:22]==(100,65536),
            "pd025_fixed_service_returns":calls==1000,
            "pd025_clock_observations":clock(24)>clock(16) and clock(40)>clock(32) and clock(72)>clock(64) and cap_end>cap_start}
    single=(clock(24)-clock(16))/4096;batch=(clock(40)-clock(32))/4096;bulk=(clock(72)-clock(64))/4096
    return checks,{"platform":"single-CPU Hercules, normal IPL","svc_calls":calls,"svc_mean_us":(cap_end-cap_start)/4096/calls,
                   "disk_records_per_iteration":3,"disk_iterations":40,"disk_record_bytes":18452,
                   "single_channel_submissions":v[12],"grouped_channel_submissions":v[13],
                   "single_disk_us":single,"grouped_disk_us":batch,"grouped_elapsed_ratio":batch/single,
                   "bulk_roundtrips":100,"bulk_bytes_each_direction":65536,"bulk_total_us":bulk,
                   "bulk_effective_mib_per_second":100*2*65536/(bulk/1e6)/(1024*1024),"storage_observation_scans":v[22]}
