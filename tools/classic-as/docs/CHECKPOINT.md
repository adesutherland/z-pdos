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
