# Bootstrap implementation checkpoint

30 September 2026. The user requested a clean pause, migration into this
repository, a local snapshot commit, and then restarted agents working here.
All assembler implementation files were already written here. The five
assembler/naming/design/inventory documents formerly held in the research
repository are now owned here; only a short ownership pointer remains there.
Historical licensed tools, private guest outputs and unrelated changes remain
outside this source tree.

The migration snapshot was committed as
`04d41ecf1cf1f89f7125a2a127cc53cd5e5b0be5` and, on the user's subsequent
instruction, pushed to [adesutherland/z-pdos](https://github.com/adesutherland/z-pdos)
on `develop`. The remote branch was read back at that exact commit. The saved
state below describes that snapshot; follow-on QA and repairs are recorded
separately.

## Completed local implementation after restart

The original bootstrap slice is implemented and independently reviewed in this
repository. On 30 September 2026 the user approved committing and pushing these
completed changes, then qualifying the 31-bit entry adapter, PDPCLIB and PDOS
support in that order. `04d41ec` identifies the earlier migration snapshot;
the completed bootstrap was committed and pushed as
[`e87652d2bc1efdee4439302be4420a5c3ac8911e`](https://github.com/adesutherland/z-pdos/commit/e87652d2bc1efdee4439302be4420a5c3ac8911e).
The remote `develop` branch was read back at that exact SHA before consumer work.

The subsequent local [TSO31 consumer checkpoint](../../../runtime/tso31/CHECKPOINT.md)
records its source variant, expanded language subset, independent object QA,
target C compilation and the downstream link audit. It is separate from the
published bootstrap results below.
The subsequent [PDPCLIB language checkpoint](PDPCLIB-CHECKPOINT.md) records
ordinary layout semantics, the optional macro provider, actual source-owned
consumer bytes and final independent host QA.

- `crexx tools/build.crexx --args test` passed all **19** registered checks
  using Apple Clang 21.0.0, strict C90, warnings as errors and no C extensions.
  Engine registration is now mandatory. Six C unit suites cover values, reader,
  encoder, object writer, engine and independently authored contract controls.
- `crexx tools/build.crexx --args sanitize` passed the same **19** checks with
  address and undefined-behaviour sanitizers. The engine suite has **2,881**
  assertions; the independent contract suite has **45**. Both implementer and
  independent reviewer ran focused QA before the coordinator's integrated run.
- Reviewed repairs cover returning to empty sections, failed API result status,
  parenthesized displacement expressions and malformed provider spans. Layout
  counts known duplicated constants in aggregate; it does not iterate billions
  of elements before the classic writer can reject an oversized section.
- Engine controls verify H/F signed bounds, absolute AD extremes, A/V fixup
  identity and sign, modes, forward references, long internal/DSECT names,
  USING/DROP and all selected operand formats. Limits, changed replay, provider
  errors, and writer/sink failures through END and completion invalidate output.
  One and 10,000 unlabeled instructions request identical allocator storage;
  the independent reviewer also checked repeated unlabeled DC statements.
- Actual CLI decks pass independent ESD/TXT/RLD/END semantic checks: bootstrap
  **149**, constants **141**, formats **139** and selected S/370 BASR **26**.
  The optional retained cREXX suite checks help/version, return codes, profiles,
  existing binary preservation, failed output absence, CRLF and final EOF.
  The CLI reviewer separately rejected eleven targeted deck corruptions.
- The coordinator compiled the documented direct C89 recipe with `-O2` and
  assembled the bootstrap fixture without CMake or cREXX. Its 13-card output
  passed the same 149 independent semantic checks. Documentation and build
  integration were reviewed against the actual command and core contracts.

This qualifies the documented host component subset. Native z/PDOS hosting,
16/32-bit host execution and non-ASCII C execution-character-set hosts have not
been run. The core's numeric character handling and bounded arithmetic were
reviewed; the desktop configuration explicitly rejects an insufficient
`size_t` range. Complete newlib/PDPCLIB/PDOS assembly and downstream link/load or
source-to-IPL qualification remain open. No guest run, compiler repair or
OS-service implementation was started.

## Saved state before restart

- Original `values.c`, `reader.c`, `machine.c`, `assemble.c`, `object.c` and
  standard-C `main.c` are present, with the public header and own MIT licence.
- Values, reader and encoder unit suites passed strict C89 and ASan/UBSan.
  Independent codec and integer-oracle checks passed. The coordinator also
  compiled and ran the three sanitizer unit executables independently.
- Object writer tests passed 661 checks with strict C89 and ASan/UBSan,
  including name/ID/range validation, written fixup coverage, gaps and failed
  begin/write/completion. The coordinator independently reviewed its records
  and failure handling and ran the sanitizer unit suite.
- The first actual CLI deck passed 149 independent checks across 13 cards:
  two sections, entry offset 2, four A/V relocations, modes and reserved gaps.
  The independent checker rejected nine targeted corruptions during its QA.
- The engine builds with strict C89 and creates the agreed fixture deck.
  Its dedicated unit suite was still being written when paused; it is **pending**.
  Full integrated QA and final documentation review are also pending.
- The migrated snapshot's cREXX/CMake pipeline passed all seven registered
  checks. Engine unit registration is visibly conditional at this checkpoint;
  seven passing checks do not imply completion of that pending suite.

## Completed restart scope

Restarted work was confined to this repository. The engine unit suite now covers
replay changes, symbol/section/fixup/expression/storage limits, invalid operands,
unresolved/duplicate names and failing providers/writers. Constant widths,
signed bounds, long internal names and storage independent of repeated unlabeled
statements were checked.

The integrated strict-C89 build, independent deck checker and sanitizer QA
passed. CLI output preservation, diagnostics and the user guide were reviewed.
The temporary conditional engine-test registration was removed; builds now
require the complete suite.
Retained development orchestration is `tools/build.crexx`; direct C compilation
remains documented for hosts without cREXX/CMake.

The implementer owns QA and the coordinator independently validates delivery.
Agents use GPT-6.1 Sol Extra High for the engine/complex work and High for defined
review tasks. No inherited assembler code, opcode table, private macro source,
guest run, OS-service work or compiler repair is part of this seed.
Complete runtime/OS assembly and native z/PDOS hosting remain later gates.

## PDOS-driven source resolution, 1 October 2026

The optional provider now resolves open-code COPY and library macro definitions
through explicit host-owned record handles. No filesystem or heap service enters
the core. Bounded nesting, active-member cycles, per-member encoding and original
coordinates, replay of unused/comment records, and failed-open/close cleanup have
original independent unit checks. A CLI fixture checks copied source and a
library-loaded macro against independently fixed BASR bytes and deck semantics.
PDPTOP is now reached; its conditional variables remain the next build gap.

## PDOS-driven scalar conditional assembly, 1 October 2026

The provider executes the bounded scalar/branch subset documented in MACROS.md.
Original fixtures independently check global persistence, fresh locals, scope
visibility, sequence loops, numeric/character relations, arithmetic overflow,
division by zero, MEXIT, SYSNDX replay and inactive undefined operands. Existing
allocation/replay/I/O and deck checks remain. The selected PDPTOP now passes its
variable setup; the real output reaches the next engine dependency, its leading
AMODE ANY before the unnamed CSECT. No source/profile rewrite is used.

## Deferred section modes, 1 October 2026

AMODE/RMODE associate by section name independently of source position; blank
names identify an actual unnamed section. Declarations create no section and
missing targets or duplicate declarations fail. AMODE ANY is distinct from 31
and the object writer emits its independently checked ESD flag. The bootstrap
fixture now names its CODE and SECOND mode declarations correctly; the expected
object bytes remain unchanged. The selected PDPTOP is consumed unchanged and
the real ASSERT output reaches the prologue's external SAVE dependency.
Normal and sanitizer affected checks pass 37/37, including the TSO31 consumer.

## Local V-reference namespace — 1 October 2026

V constants retain a separate external identity when their name also defines
a local entry or CSECT. Independent engine/writer checks preserve A versus V
relocation kinds and reject ordinary references to an undeclared V-only name.
The original `local-v.asm` fixture links through Classic Linker at image base
4096; all 14 resulting bytes are checked independently, including entry address
4108 and section address 4096. This is a flat-binary host path, not OS loading
or guest execution.
Affected Classic checks pass 33/33 in normal and sanitizer builds.

## Masked compare and selected literals — 1 October 2026

The original encoder now includes S/370 CLM with independent boundary vectors
and rejection under S/360. The source engine accepts signed H literals and
explicit-length X literals with checked widths and padding. A small original
source checks their combined instruction bytes and literal-pool placement.
Actual PCOMM, PDOSUTIL, START and STRING C outputs now assemble, alongside
the earlier eight successful units. PDOS and STDIO reach the desktop literal
capacity; MATH/STDLIB/TIME still reach fixed/floating constant/instruction gaps.
These are host language/object checks, not the kernel ISA or complete OS build.
Affected Classic checks pass 33/33 in normal and sanitizer builds.

## Explicit desktop literal bound — 1 October 2026

`--literal-limit` selects a checked decimal count from zero through 65536; the
default remains 256 and the unchanged 32 MiB payload budget still bounds all
allocations. Invalid/overflowing arguments fail before opening source or output.
CLI controls reject insufficient capacity and pass an explicitly sufficient
count in normal and sanitizer builds. With a selected limit of 4096, the actual
PDOS C output assembles: 13,349 statements, 1,043 symbols and 670 fixups. The
remaining four C units first require floating-point forms; raw support and
complete linking remain open.

## Selected HFP instructions and whole-C consumer — 1 October 2026

Original HFP RR/RX descriptions cover the reached historical instruction forms
and restrict floating registers to 0,2,4,6. Independent vectors validate 20
instruction byte sequences, invalid register fields and displacement overflow;
LRER is excluded from the common S/360 selection. The compiler's exact XL4/XL8
literals reuse the existing generic hexadecimal parser/pool rules. All 17 C
units assemble through maintained macros. Raw support, the named kernel
profile and whole-toolchain guest evidence remain separate gates.
Affected normal/sanitizer suites pass 42/42; all 17 C objects also pass
through the sanitizer assembler using the retained recipe.

## Deck identification and register interface — 1 October 2026

TITLE now validates headings and sends a bounded named ID through an explicit
optional writer callback before record output. IDs do not enter the ordinary
symbol namespace; all record trailers carry the independently checked CP037
ID. Names above eight characters and duplicate named TITLEs fail, and writers
without metadata support reject named TITLEs. PRINT accepts only the documented
validated listing-control subset; no source listing is claimed.

The original no-operand YREGS interface supplies R0..R15 and passes an independent
CLI instruction-byte check. The six raw inputs now reach concrete next gaps:
forward PSA mapping/USING, source-dependent duplication, PUSH USING, GETMAIN
and the empty FIXWRITE prototype. No missing service was treated as a no-op.
Affected normal and sanitizer suites pass 45/45.

## Source addressability checkpoint

The engine now supports multiple-register USING, zero-register mappings,
absolute addressability, deferred forward USING expressions and a 16-frame
PUSH/POP USING stack. PUSH preserves the active mappings. Independent instruction
bytes cover the zero base, successive 4096-byte regions, minimum-displacement
selection, DROP-all and restored mappings. Missing forward definitions, stack
underflow and capacity exhaustion fail. All 84 integrated host tests pass.
The six raw PDOS modules advance to ISA, duplication, OS service and macro
prototype gaps; no complete kernel or guest result is claimed.

## Declaration expression checkpoint

Parenthesized absolute DC/DS duplication factors and signed 32-bit binary
self-defining terms are implemented. PDOS's source-defined PSA reservation
now advances past its duplication expression. Independent data/instruction
bytes cover expressions, binary masks, signed boundaries and incrementing
location-counter address constants. Undefined layout factors, negative sizes
and malformed binary terms fail. Forward duplication dependencies remain open.

## Reached system instruction checkpoint

Original instruction descriptions add the reached S/370 control/mask/clock
operations, ESA/390 BSM/subchannel/PR forms and z900 LPSWE, STMG/LMG and
BRCL/LARL. Nineteen independent architectural vectors cover opcode fields,
capacity, architecture exclusion and displacement limits. Source tests cover
forward and backward halfword-relative targets, odd-target rejection and both
signed RSY bounds. SIO is explicitly excluded from ESA/390 and z900.

The raw kernel probe now advances into source scanning, SAPSTART reaches ORG,
and PLOADSUP reaches missing public control-block mappings. OS service macros
remain real dependencies. Normal and sanitizer suites pass 47 affected tests;
no named kernel profile, full build or guest acceptance is claimed.

## Inactive open-code search

Forward conditional searches no longer diagnose operands in bypassed source
records. Original tests cover malformed inactive operands, macro-scope target
isolation, malformed active targets and changed inactive replay. The repaired
kernel's bypassed S/370 channel-status branch now reaches subsequent source.

## Selected ORG checkpoint

The engine retains section high water independently of position. An explicit
optional writer origin event flushes TXT before a cursor change; subsequent
absolute TXT overlays survive independent Classic linking. Original tests
check ten final bytes, three TXT records, label/layout semantics, DSECT
redefinitions, default restoration, missing callbacks and invalid origins.
Both engine and writer reject origins below prior relocation fields. This
bounded suffix policy deliberately leaves arbitrary relocation overlays open.

## Character and location-counter literals

Selected C/CL literals preserve case, CP037 encoding and explicit blank padding.
Original pool tests distinguish lower/upper text and reject truncation. A(*)
literals capture the referencing instruction's section and offset; independently
expected bytes and fixups prove that two references have different values.
SAPSTART now passes its SAPLOAD character literal and advances to CCW1.

## Channel word checkpoint

Original CCW1 encoding uses the public format-1 fields and zero alignment
fill. Independent source and link checks validate command, flags, count,
forward A relocation and all 30 final bytes at image base 4096. Oversized
fields, a 32nd address bit and missing addresses fail. The repaired kernel
advances beyond its write CCW chain. Channel execution and final OS address
constraints remain unqualified; CCW0 support remains open.

Fixed-card continuation now works in the optional provider without streaming
allocations. Original cases cover comma truncation, quoted column padding, a
continued logical condition, CP037, changed replay, missing/invalid cards,
statement capacity and physical-card step limits. Kernel support advances
from the continued AIF to external mapping declarations.

Ordinary length attributes and implicit SS lengths are independently checked
with forward fields, duplication, expression addends, explicit bases and
length overrides, EQU inheritance, instruction labels and L'*. Undefined,
oversized and forbidden forward layout attributes fail. Kernel startup now
passes its forward PGMNAME move and reaches CALL.
