# PDPCLIB consolidation and classic linker checkpoint

30 September 2026. I selected z/PDOS as the maintained home of our PDPCLIB
source, fixes and target variants, then promoted PDLD as Mainframe Classic
Linker, command `mf-classic-ld`, beside `mf-classic-as`. I approved committing
and publishing this independently reviewed delivery on 1 October 2026.
The preceding published entry/language checkpoint is
`ff7e282808194a33e4d6a2da49e52018fb2775f3`.

This records the original consolidation delivery. The current single-source
runtime merge is recorded in [CHANGES.md](CHANGES.md) and the
[OS checkpoint](../os/pdos/CHECKPOINT.md). Whole-source hashes below identify
the earlier delivery, which remains in Git history.

## Source ownership

The complete 288-file PDPCLIB mirror subtree was verified byte for byte before
the three shared-source corrections were applied. Its exact revision, archive,
manifest and contributor terms are in [SOURCES.md](SOURCES.md) and [LICENSE](LICENSE).
[CHANGES.md](CHANGES.md) consolidates all six inventoried Lab/SDK deltas. Only
`mvssupa.asm`, `string.h` and `stdio.c` differ from their imported upstream
bytes. Configuration changes and deliberate omissions are named profiles.
The upstream PDPTOP member remains unchanged.

| Maintained shared source | SHA-256 |
| --- | --- |
| `mvssupa.asm` | `f66fa3f287847b45172f8049953ced0429fa1e34e7e842d13b1b7413d5efde45` |
| `string.h` | `882e343400985c2d230415ab2edda358d9ee67e4e094f9be548eb96ad468efc6` |
| `stdio.c` | `79c29be4fad37e2a2af7c5fc0290cc71daffd8001379bf7eac7659b659eb0f3a` |

Independent review checked the import, notices, precise patch dispositions,
profile identities and host harness assumptions. Mainframe Lab's references,
worktrees, native assets and qualification history remain intact. SDK copies
retain their old identities until a separately qualified consumer transition;
future maintained fixes belong here.

## Focused host QA

Apple Clang 21 on macOS passes the four changed C checks, five-profile source
preparation check and maintained-source macro consumer. The profile preparation
checks fixed source/member/C hashes, rejects unknown profiles and unowned paths,
and preserves an existing output directory and receipt. The lean TSO31 native
source exactly reproduces its retained qualified hash `b8389b2b39bc945a5480311dc274a84d47f7c04db98b1a66fa82f6cdde20753d`.
The selected macro definitions are unchanged and retain the previously checked
consumer semantics. [The C test guide](tests/README.md) describes the guarded
source paths, native stubs and four independently failing mutation controls.

The combined root build passes all 39 affected checks in the normal build:
six library checks and 33 linker checks. ASan/UBSan passes the same six library
checks and 33 linker checks in focused runs. Unchanged assembler/entry checks
retain their earlier results. The linker checker stays strict C90; inherited
PDLD builds as C99 with its own warning policy. The parent assembler's strict
C90 diagnostics are unchanged. The cREXX root recipe now passes a constructed
`-DMF_SANITIZERS=OFF` or `ON` argument; the earlier embedded substitution passed
the literal `:sanitize` to CMake. Normal and sanitized cache/compile flags were
checked explicitly after repair.

Repeat only affected checks from the repository root:

```sh
crexx tools/build.crexx --args build
ctest --test-dir build/classic-as -R '^(pdpclib_|pdld_)' --output-on-failure
cmake -S . -B build/classic-as-sanitize -DMF_SANITIZERS=ON
cmake --build build/classic-as-sanitize
ctest --test-dir build/classic-as-sanitize -R '^(pdpclib_|pdld_)' --output-on-failure
```

## Retained package repair

The separate [classic linker](../tools/classic-ld/README.md) retains its own
source record and notices. Its first repair normalizes assembled section
origins consistently for text, labels and relocation. Original fixtures cover
nonzero origins, appended zero-origin PC sections, exported LD/V symbols,
positive/negative/difference A constants, eight-byte fields and two image
bases. Independent review uses separately authored expectations.

The retained three-object entry/C/native package now stores `0x30238` at field
`0x2fb48`, matching `@@PCLST`; the earlier value was `0x34028`. Coordinator
readback verifies the 243,840-byte XMIT hash
`dc422d41b8cf1ea3d923303edc8348898998c4527be1a54477dbd9b92467f913`,
all 4,573 relocation positions, unchanged structural records and every other
image byte. The [entry checkpoint](../runtime/tso31/CHECKPOINT.md) records the
exact retained inputs and host-link qualification.

## Still open

MVS 3.8 / real System/370 / AMODE 24 and RMODE 24 remains the first complete
library target. Its source configuration exists; complete assembly still needs
additional ordinary language features, conditional macros and independently
supplied service/layout definitions. No IBM macro library or native object is
imported. Full library/service execution, the small target C caller's link/run,
native z/PDOS tool hosting and source-to-IPL are separate unfinished work.

The z/OS 1.5 compatibility recipe uses an AMODE31/RMODEANY service member for a
24-bit caller; its final module mode and load/execution gate remain separate.
It requires later instructions and services and cannot select the real S/370
target. The [known MVS FILE initialization question](KNOWN-ISSUES.md) also needs
a defined repair before complete libc qualification. The linker documents its
inherited MVS writer limitations; focused origin QA does not qualify every
input/output format it retains.
