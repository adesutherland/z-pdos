# Repaired PDIO1 and consolidated runtime checkpoint

1 October 2026. **One maintained OS/runtime selection; host controls and
C-to-assembly-text generation pass.** Independent whole-source assembly,
linking, boot and guest qualification of this candidate remain open.

The original exact qualified input is preserved by commit
[`d62b109`](https://github.com/adesutherland/z-pdos/commit/d62b109ed986bd455732484173ff4ebe0533045a).
All 37 OS files remain unchanged. We compared its 44 runtime inputs to the
maintained library, merged the relevant differences and removed the duplicate
runtime directory. The current OS and application recipes use `pdpclib/`,
with explicit configuration/service profiles for genuine target differences.
The current selection contains 81 files, 1,396,934 bytes.

| Current record | SHA-256 |
| --- | --- |
| Repository-relative maintained selection manifest | `b4fd6e044875593f63466a121f13fce93c6be9f975d5485252642c4b67b53d16` |
| Selected canonical upstream manifest | `53b98cece9972a9589d54407de4094119a5a40a1f725cbc2d3b85a3765d2d813` |
| Prepared-layout manifest | `e59d51e28e0fb2898a1d85e556e439e166dc1965fec8bd2cc1ebc4028736db9e` |
| Consolidated current OS/runtime recovery patch | `9800394bc50943692d025903ea71c6f67dce9e284a29c5b27e23351b4d341c41` |

A clean selected canonical upstream copy plus the current recovery patch
reproduces every selected input exactly. The retained preparation recipe also
checks those bytes after copying the maintained OS/runtime files and selecting
`pdos-zarch`. Neither route consumes an earlier frozen runtime or a Lab build
copy. The original seven patches remain provenance records.

All 17 C units compile with Mainframe Classic C's native-host MVS variant,
GCC 3.4.6 lineage. Flags remain `-Os -DZARCH -U__CMS__ -D__MVS__
-D__PDOS390__ -S -I. -I../pdpclib -DUSE_MEMMGR`. START and STDLIB retain
noreturn warnings; all compiler return codes are zero. The host is Apple
Silicon macOS 26.6.2, Clang 21.0.0. These are assembly-text outputs, not
independently assembled or executed objects.

The host fixture still extracts actual repaired PDS-write and 3270-output
functions from the OS. Reversing only 0005 in a generated copy makes both
controls fail; repaired functions pass ASAN/UBSAN. The affected PDPCLIB
normal checks pass 7/7, including profile preparation and the unchanged
source-owned macro consumer. Focused sanitizer checks pass 5/5. Five explicit
profiles prepare correctly and reject existing/unowned outputs. Disposable
controls fail when FILE initialization, the MVS write-handle guard or the
full-width LP64 unsigned-long limit is removed.

The operation inventory covers ten source assembly/macro inputs and 17
compiler outputs, including unexpanded conditional branches and prototypes.
The earlier first-language assembler probes stopped at generated `COPY PDPTOP`
and raw support `TITLE`. Those open facilities are unchanged by runtime
consolidation. The assembler's broad `s370` selector was only a language
probe, not qualification of the standard z/Architecture kernel profile.
No as370 was imported or invoked.

The accepted Lab kernel remains PDIO1, strict RDW SHA-256
`1fcae79b32314adce6838498e60760bfdc612d759c46fb025635073f960ca199`,
AMODE31/RMODE24, produced with repaired GCCMVS and native ASMA90/IEWL.
Today's cREXX beta 3 qualification retained that kernel and passed fresh
native compile/assemble/execution, supplied 9 PASS/0 FAIL/2 SKIP, actual input
10 PASS/0 FAIL/1 SKIP, help/error checks and complete stopped output readback.
The assembly was 3,108 records/138,311 bytes and matched the host reference;
the binary fixture was ordered 00–FF. These are historical results for the
original source/tool/runtime input, not qualification of the merged candidate.
The accepted image and leases were not changed.

[BUILD-PLAN.md](BUILD-PLAN.md) owns independent assembly, link, boot and
application gates. [DEPENDENCIES.md](DEPENDENCIES.md) records the source and
service interfaces. Generated outputs and detailed receipts remain ignored;
Git preserves sources, recipes and these compact results.
