# CMS file envelope validation checkpoint, 6 October 2026

The successor's CMS31 K file reader now calls one C89 validator for the
`PDCMSF01` header and complete staged record stream. The validator bounds
profile, payload, source bytes, record count and block count; checks the
FNV payload value and zero padding; and walks every 1–256 byte record,
requiring the declared byte and record totals to match. The K reader
allocates real backing from the checked first block, materializes only
records within the dataset extent and releases backing on a validation
failure. No U virtual address is used as a K pointer.

The host corruption check passed under Clang C89 with address and undefined
behavior sanitizers at `build/pdos/two-space-cms-validated-build/cms-file-test`.
It accepted a valid two-record envelope and rejected wrong profiles or
lengths, changed hash, nonzero padding, zero-length record, source-byte and
record-count mismatch, nonzero reserved trailer, oversized payload and bad
magic. The complete diskless host gate passed 89 checks at
`build/pdos/two-space-cms-validated-build/run/receipt.json`.

The rebuilt Classic C31 service and pure-C validator were linked into a
source core with SHA-256
`1f47c817273b9daab82a902ca2370cfc5ef4a3969dbab073b19a04195d7fb6b3`.
A fresh full CMS31 IOQUAL IPL passed 119 checks at
`build/pdos/two-space-cms-validated-ipl/run/receipt.json`, including the
unchanged IOQUAL/LIBRARY load, RC 0, `PASS=8 FAIL=0 SKIP=3`, exact output
record hashes and buffer release. Its disposable disk SHA-256 was
`b8c41184de9759bbc2ab2e7231733101dd2bdef21cd5bdc13e4a9c4dbebdc3a6`
before and after IPL. It used one model-2064 ESAME CPU, 256 MiB, 3390
`01B9`, 3270 `0009`, GNU binutils 2.47, maintained Classic tools and local
Hercules. Guest completion remained a disabled-wait event; clocks only
bounded failures.

This tightens the selected CMS31 file path. It does not add CKD output
persistence, a general file cache, CMS24 file services, TSO workloads in
the successor or application command dispatch.
