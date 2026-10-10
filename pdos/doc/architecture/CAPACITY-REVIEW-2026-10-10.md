# Workstation capacity review for the 0.2.1 development increment

The application build exposed a 546-byte gap in a 384 KiB K C service bank.
I want useful headroom rather than another small extension of the bootstrap
fixture. This review covers the default machine, protected storage layout,
host assembler interface and application/storage limits. The canonical cREXX
release package remains unchanged; its runtime profiles are external inputs.

## Recommended profile

Use **512 MiB real storage**, one ESAME 2064 CPU, a **16 MiB bootstrap core**,
a **2 MiB K C service bank**, **512 KiB K stack**, and **3 MiB each for K/U
DAT pools**. Keep application heaps at the qualified 64 MiB U31 and 128 MiB
U64 targets. This accommodates their simultaneous use, images and OS buffers
with substantial remaining real storage. Wider heaps remain explicit runtime
profiles rather than patches to the canonical cREXX binaries.

| Budget | Existing candidate | Recommended default |
| --- | --- | --- |
| Real machine | 256 MiB | 512 MiB |
| Core | 4 MiB | 16 MiB |
| K service | 384 KiB; 546 bytes spare | 2 MiB, contiguous real backing |
| K stack | 128 KiB | 512 KiB, disjoint backing |
| K/U DAT | 1.5 / 1.375 MiB | 3 / 3 MiB |
| Real, virtual and U allocation ledgers | 128 entries | 512 entries |
| Invocation resources | 128 entries | 512 entries; depth remains an explicit policy |
| Line-editor records | 240 × 256 bytes | 4096 × 256 bytes in U; file limits remain separately checked |
| Desktop assembler | 32 MiB host budget; scattered 4K/8K capacities | Named desktop profile, 256 MiB budget, generous symbol/literal/fixup capacities |

512 MiB is adequate for this single-user foreground workload and the declared
heap profiles. Raising only Hercules MAINSIZE would leave the kernel unaware
of the extra memory. Conversely, raising the kernel's real limit with the old
machine would permit out-of-machine accesses. The profile, DAT pools,
bootstrap descriptor, native staging, package checks and guest oracle must
move together.

## Implemented real and virtual layout

| Real interval | Ownership / K virtual mapping |
| --- | --- |
| Existing low islands and 3E/3F MiB channel regions | Keep interruption, entry, PC and 24-bit channel workspaces separate from U. |
| 4–6 MiB | Contiguous K service; virtual `02000000`–`021FFFFF`. |
| 6 MiB plus 496 KiB | K stack extension; retain four low entry/control pages, total 512 KiB. |
| 8–11 MiB | K DAT pool; privileged alias beginning `05000000`. |
| 11–14 MiB | U DAT pool; privileged K alias beginning `05300000`. |
| 16 MiB upward | Allocatable real frames after core ownership is established. |

Move the trampoline from `02060000` to `02800000`; the old address sits
inside the enlarged service bank. The stack stays at `03000000` in K.
No K service, stack, table or aperture mapping is added to U. The bootstrap
source and launch page use checked real allocations outside the final core
during copy. The PLOAD stack is live only during loading, not the final copy.
The package must reserve enough records/extents for future bank growth rather
than retain a 40-record/one-cylinder limit.

## Assembler interface

The portable supplied-storage core already has configurable capacities.
The desktop wrapper adds fixed defaults and a 32 MiB total allocation budget;
product recipes then repeatedly choose 4096 or 8192 literals/symbols. A
larger service can therefore fail in the host wrapper despite sufficient
guest memory. A named desktop capacity profile should set generous symbol,
literal, fixup, macro and host-storage budgets together, with checked explicit
overrides and clear diagnostics. Keep bootstrap defaults and existing strict
failure/output-publication contracts. No timer or retry establishes success.

This is preferable to adding another literal override to each command.
Separate K compilation units and supplied-storage symbol indexing are useful
future maintainability/performance work; capacity alone does not remove the
assembler's linear symbol searches or change the C linkage contract.

The walkthrough also identified an encoding boundary: character constants
use the assembler's CP037 conversion table, while the console/text adapters
use IBM1047. The new operator messages use portable punctuation. AS-008 owns
an explicit literal-data code-page selection with independent byte controls;
classic object names and macro comparisons must not change by accident.

## Limits that need separate storage or protocol work

The current 100-cylinder 3390 layout and **eight 2 MiB, two-bank stored-file
slots** are substantial constraints. Multiplying slots or per-file size also
multiplies allocated disk space and changes durable layout/version and recovery
contracts. A safe improvement needs configurable larger media, an extent-aware
store/directory, migration rules and crash/readback qualification. It must not
be presented as a constant-only enlargement. Record that work in the component
backlog and expose current capacity to the operator.

The initial record-file API accepts first-track PS FB/VB files and saves to a
separate empty destination. Multi-track I/O, multi-extent files and atomic
replacement require their own admission, transfer and failure semantics.
Raising the editor's buffer budget does not widen that disk contract.

Keep genuine architecture/format bounds: 24-bit virtual addressability,
24-bit channel addresses, classic object identifiers, 3270 address forms and
CKD record/track geometry. Terminal transfer, session, input and parser bounds
must remain checked and report exhaustion. Additional panels, long sessions,
dynamic input and disk capacity should grow through owned storage rather than
large unqualified stack arrays. The whole 3270-family backlog remains open.

## Qualification needed

Review and freeze the coordinated change before guest runs. Check the final
service page, full stack, DAT aliases and pool/aperture endpoints; prove no U
alias and reject overlap. Reconstruct the larger sparse package exactly and
check negative headers/record/extent bounds. Run the actual normal image on
models 2/5, monochrome and a line primary, with exact editor file readback,
canonical cREXX execution and clean shutdown. A real allocation above the old
256 MiB machine budget should be touched, verified and released. Keep earlier
256 MiB evidence labelled with its original profile.

## Implementation and initial evidence

The coordinated profile is implemented in the owning source and build adapters.
The final source-built service is 454,214 bytes in a 2,097,152-byte bank, leaving
1,642,938 bytes (78 percent). K DAT uses 2,260,992 of 3,145,728 bytes; normal
boot U starts with only its 16,384-byte root. The simultaneous 320 MiB U31 and
128 MiB U64 DAT check uses 1,929,216 bytes. Entire service/stack mappings,
table/aperture endpoints, U isolation and overlap refusal pass C90 ASan/UBSan
checks. The desktop assembler reports 215,779,644 of 268,435,456 host bytes
for the actual service; its fixed supplied arenas account for most of that
budget. Indexing and more economical optional macro arenas are separate
performance work, rather than a claim that larger capacities speed assembly.

Unchanged wrapper bounds include 64 sections, 256-byte source cards, 16 library
directories, 256 known library members and 16 simultaneously open library
sources. These are checked interface/policy bounds, distinct from the enlarged
symbol/literal/fixup arenas. The selected compiler output fits them; they are
not a claim of an unbounded assembler.

Hercules's 3215 provider has a separate 150-byte keyboard buffer, including
CRLF. The line-console walkthrough therefore qualifies 148 input characters,
while preserving loaded 256-byte records. A larger PDOS read buffer cannot
enlarge an emulator's internal keyboard buffer; overflow and return to the
shell are recorded separately.

The initial six-cylinder image check found the installer's old one-cylinder
KCORE expectation; it now checks the selected complete extent. The first
whole-system boot found another genuine architecture bound: **MVCL lengths
are 24 bits**, including in AMODE64. A single 16 MiB move became zero. The
launcher now uses 8 MiB chunks with a checked non-overlapping source and
destination. Exact real-instruction readback passes 4 MiB, 16 MiB and 16 MiB
plus one page, including observed completion. Failed candidates remain
unqualified; the combined application walkthrough selects the repaired image.

The native 320 MiB allocation probe touches and verifies each page, frees the
allocation, runs another command and shuts down with no remaining invocation
or input/screen lease. Its observed peak real use is 402,026,496 bytes, within
the selected machine. The separate 320/128 MiB DAT fixture proves table
capacity, rather than claiming that the unchanged canonical cREXX runtime
selects those heaps. The current TSO31 package keeps its published 64 MiB
runtime profile.

The final application matrix passes models 2/5, monochrome and the line primary,
with a complete walkthrough peak of 409,493,504 bytes (about 391 MiB).
Exact stopped-media guards preserve all records outside STORE and match the
durable transcript to the stopped K capture. The application qualification
record owns the selected 512 MiB distribution, source freezes and receipts.
