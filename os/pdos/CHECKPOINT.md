# Repaired PDIO1 preservation and C-build checkpoint

1 October 2026. **Preserved qualified source, recovered source and native host
component/C-to-assembly-text proof.** A Classic-produced linked or bootable OS
has not yet been established.

The 81 selected files contain 37 OS inputs and 44 frozen runtime inputs,
1,396,526 bytes. Every retained byte matches the actual PDIO1 repair source
used by the native qualified kernel. Selected upstream bytes match both the
pinned Git objects and verified tracked-file archive. Applying all seven
original patches, including the two separate PDLD prerequisites, reproduced
the selected source exactly. A clean selected upstream copy plus the normalized
recovery patch also reproduced every selected file exactly.

| Record | SHA-256 |
| --- | --- |
| Repository-relative maintained selection manifest | `bbe468943245339c5cfb08f607bffc3805b5de11335e0f5bb26ff3695a11782d` |
| Selected upstream manifest | `53b98cece9972a9589d54407de4094119a5a40a1f725cbc2d3b85a3765d2d813` |
| Qualified/prepared layout manifest | `ea6dcdbfae004f3a1c0f2c8159d779800d30858fc4e46c1f52b1ae7476f1d732` |
| Consolidated OS/runtime recovery patch | `aeab2ad2550ee82b73c47019e4d5d5f6a7229974939de5e2e3a83a09c6ecf00e` |

The retained cREXX commands passed source preparation/checks, source recovery
and all 17 C compile units. Native host is Apple Silicon macOS 26.6.2 with
Clang 21.0.0. The new cross producer is Mainframe Classic C, GCC 3.4.6 lineage,
MVS variant from the [compiler checkpoint](../../tools/classic-cc/CHECKPOINT.md).
Flags are `-Os -DZARCH -U__CMS__ -D__MVS__ -D__PDOS390__ -S -I.
-I../pdpclib -DUSE_MEMMGR`. START and STDLIB emit noreturn warnings;
all compile return codes are zero. Output hashes and diagnostics
remain disposable ignored build outputs.

The host fault fixture extracts actual PDS-write and 3270-output functions
from the retained source. Reversing only 0005 in a generated copy makes both
failure controls return 1. The repaired versions return 0 under Clang
ASAN/UBSAN, checking complete/short/error retry, payload preservation, EOF,
extent exhaustion, invalid lengths, failed state, empty and long output,
scrolling, protected input-field bytes and buffer guards. No OS source was
altered to make those tests pass.

The new inventory covers ten source assembly/macro inputs and 17 compiler
outputs. It records spelled operations, including conditional paths and macro
prototypes; it does not claim expanded instructions or final-object ISA proof.
A direct independent assembler probe of the generated kernel fails at line 1,
`COPY PDPTOP`. The raw support source fails at line 1, `TITLE`. Both return
unsupported-feature status 3 (CLI exit 1) and produce no object. The broad
`s370` selector was used only to probe the first language failure; it does not
qualify a z/Architecture kernel profile. No as370 was imported or invoked.

The accepted Lab kernel is PDIO1, strict RDW SHA-256
`1fcae79b32314adce6838498e60760bfdc612d759c46fb025635073f960ca199`,
AMODE31/RMODE24, produced with repaired GCCMVS and native ASMA90/IEWL.
Today's cREXX beta 3 qualification retained that kernel and passed fresh
native compile/assemble/execution, supplied 9 PASS/0 FAIL/2 SKIP, actual input
10 PASS/0 FAIL/1 SKIP, help/error checks and complete stopped output readback.
The assembly was 3,108 records/138,311 bytes and matched the host reference;
the binary fixture was ordered 00–FF. Those are retained historical input
and behavior results, not proof for the new Classic compiler outputs.

Private disks, native artifacts, packages and raw guest receipts stay in Lab.
The running accepted guest and its leases were not changed. The
[build plan](BUILD-PLAN.md) owns the remaining assembly/link/boot/application
gates; the [dependency inventory](DEPENDENCIES.md) records their exact scope.
