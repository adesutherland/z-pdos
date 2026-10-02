# Consolidated PDPCLIB changes

## Classic-built z/PDOS runtime — 2 October 2026

The `pdos-zarch` profile now runs its C32 kernel, IPL loader and legacy
runtime in AMODE31. SAPSUPA's console return PSWs use the existing `AM64BIT`
profile value rather than forcing AMODE64. Native 64-bit applications retain
their separate full-width dispatch contexts. This fixes reached profile
behavior without changing the real System/370 configuration.

The shared MVS `filedef` and `fdclr` C helpers now terminate SVC99 text-unit
lists by flagging the last actual pointer. The earlier null slots followed
by a standalone high-bit sentinel did not satisfy the native list contract.
The existing-read, new-binary, new-text and unallocation cases pass through
the actual STDIO implementation in both MVS-control and PDOS390 host tests;
restoring the earlier list format makes both tests fail. Focused normal and
sanitizer suites pass 11/11. The unchanged source-owned macros retain their
previous qualification. [z/PDOS qualification](../../../pdos/doc/qualification/QUALIFICATION.md)
records the separate complete guest result for the selected library.

30 September 2026. The source here owns the reviewed library changes previously
held in Lab and SDK patch sets. An inventory of the main checkout, retained
worktrees and SDK found the six deltas below; duplicate copies did not add
another library fix. SDK adapters, newlib changes, linker patches and OS kernel
repairs belong to their own components.

| Origin | Maintained disposition |
| --- | --- |
| PDPCLIB 0001: PDSE check | Shared `mvssupa.asm` correction: SMS flags at DSCB byte 78; PDSEs bypass the PDS-only DS1LSTAR test. |
| PDPCLIB 0003: DYNAL pointer words | Shared `mvssupa.asm` correction: write both complete request-list pointers with their terminating high bit; do not retain a relocated template's high byte. |
| PDPCLIB 0002: prefix-parser omission | Optional `tso31-lean/omit-prefix-parser.patch`; full maintained source retains `@@GETEPF` and `@@PCLST`. |
| PDOS390 0002: native-build compatibility | Guarded C fixes in `string.h` and `stdio.c`; ESA configuration changes in `pdos390-esa/pdptop.mac`. |
| PDOS64 0001: restore OS switches | Explicit `pdos-zarch/pdptop.mac`, preserving the original switch meaning separately from ESA and MVS profiles. |
| SDK PDPCLIB 0004 / Lab TSO15 compatibility | Optional `tso-zos15-24/swareq.patch`; temporary SAM31/SAM24 and z/OS SWAREQ remain restricted to that later-machine variant. |

## Original delta identities

| Delta | Original SHA-256 |
| --- | --- |
| `0001-pdse-no-lstar-check.patch` | `369c9b70cfc08ab7611c8be73fc437d36365b0325ee12f98a538233a08504972` |
| `0002-tso31-omit-prefix-parser.patch` | `f10f3b5b126c1f140cec763306f913cdda4875b704bbe8ffa11f8afb5a8b5ccf` |
| `0003-tso31-dynal-low-request-pointers.patch` | `8cf5fcd4af68175eca488c65a7f142d403037ae63fe0cc74090db611e1bd8576` |
| `0002-standard-s390-native-build.patch` | `d5d726311a2aaff94076a8191f96eab29b1e8fcf6ee0cf7b991bb986e17f0b25` |
| `0001-restore-zarch-native-configuration.patch` | `5c0eaaba536280fa2891bba89800892199bddf5911857b4b6842666bc899f020` |
| `0004-zos15-tso24-swareq.patch` | `320fab07ddc6492a94b8f348c6b652e550ada47d2303fecd6a1c6b1e9c094b5d` |

The SWAREQ profile changes only the original patch's path headers to address
our library location. Its instruction and storage hunks remain unchanged.
Configuration profiles keep the main imported PDPTOP unchanged. Original Lab
README cleanup changed documentation references only, not library code.

## Qualification limits

The earlier lean TSO31 source identity was
`b8389b2b39bc945a5480311dc274a84d47f7c04db98b1a66fa82f6cdde20753d`.
The initial consolidated main source plus the lean omission reproduced that
exact native file. The five-service closure and high-placement
checks for that source are retained; no passing guest run is repeated here.

The earlier PDSE-only plus TSO24 SWAREQ source identity was
`50cad35c7e5fa4c88a2efcf4f8f3999948087edad828bc4d5b032b5f0a3bd3b1`.
Our newly prepared variant also includes the DYNAL fix, so it has a different
identity and does not inherit a whole-source guest claim. The older SDK
`T24SUP` object predates SWAREQ repair and is not proof of the patched source.

The imported mirror's C fixes have focused host QA. They do not inherit binary
qualification from the different canonical PDPCLIB source. Complete MVS 3.8
assembly/service execution, later profile execution and native library/tool
hosting remain open. The independent PDLD nonzero-origin regression is a
linker repair, not a change to the parser source or a reason to omit it.

## One maintained runtime — 1 October

The repaired OS import first preserved the exact canonical runtime input in
commit `d62b109ed986bd455732484173ff4ebe0533045a`. I then selected one maintained
PDPCLIB for all consumers. Git retains that original qualification checkpoint;
the duplicate source directory is removed.

We compared all 44 selected files and merged 19 differing shared files:
`assert.h`, `ctype.c/.h`, `limits.h`, `locale.h`, `math.h`, `mvssupa.asm`,
`pdpclib.txt`, `setjmp.h`, `signal.h`, `start.c`, `stdarg.h`, `stddef.h`,
`stdio.c/.h`, `stdlib.c/.h`, `string.h` and `time.h`. The low-core configuration
already existed as a profile; it is now named `pdos-zarch` and is selected by
the OS recipe. Other selected inputs were already identical.

The canonical updates include initialized FILE update state before native
open, run-specific exit handlers, the full-width LP64 unsigned-long limit,
64-bit argument alignment, EBCDIC table selection, additional header guards
and conditional upstream port support. These upstream fixes are adopted
once; previously fixed PDOS/390 builtin and update-service guards remain.
MVSSUPA keeps the shared DSCB/PDSE and complete DYNAL-pointer repairs while
adopting upstream CSECT mode metadata and USING-scope corrections. The three
source-owned macro definitions used by the assembler consumer are unchanged.

Earlier update initialization exposes an MVS regression: `w+b` uses the
existing write-only fallback, so native open must not prime it through the
read/update block. The shared source excludes that mode from the block. Host
tests check stale allocation flags, ordinary opens, both update spellings,
the unchanged native write mode and absence of read/point calls for that
fallback. Removing initialization or the fallback guard fails the controls.

The optional lean patch is rebased to omit the new empty `@@PCLST` declaration
and its conditional DSN-stub references as well as the unchanged parser. The
SWAREQ profile remains an explicit later-system adapter. Both now produce new
whole-source identities; old variant hashes are historical guest inputs.
There is no automatic transfer of their previous whole-source qualification.

## Original selected linkage interfaces — 1 October 2026

The original selected WTO uses the published SVC35 header, descriptor/routing
fields and register contract. MVSSUPA's two text-tail EQU aliases now allow
for its explicit R0 clearing instruction; named message text still starts
eight bytes after the invocation. Independent message/alias byte checks pass.
This is a new producer layout, not inherited IBM macro code or guest proof.

Original selected SNAP list/execute, user ABEND, NOTE and POINT interfaces
pass 120 independently expected linked bytes and a rejected-option control
in normal and sanitizer builds. They retain the existing PDOS service limits;
complete runtime assembly and execution are still separate gates.

Original selected JFCB and standard-section IOB maps now pass 20 independent
field/length checks. SAPSUPA's IEZIOB call receives a comma before its remark,
as required when the operand is omitted. This source correction does not
change emitted storage or instructions. The original qualified file remains
in Git. Complete SAPSUPA now assembles identically in normal and sanitizer
builds; linking and service execution remain separate.

The original short-list library now covers selected OPEN/CLOSE/RDJFCB execute
forms and list addresses, catalog SEARCH/SEEK/NAME records, OBTAIN/LOCATE,
eight-byte DEVTYPE and classic BLDL/FIND D setup. Independent linked fixtures
check 160 additional bytes; unsupported mode, FIND type and catalog-function
controls reject the object. These interface checks do not qualify guest I/O.

`interfaces/classic-linkage` supplies newly authored bounded SAVE/RETURN macro
definitions from documented public register-save contracts. It is an explicit
assembler library input, not an inherited IBM macro library. The small Classic C
object gate checks identifier layout and RC=(15) register restoration with
independently expected bytes. Full runtime assembly and execution remain open.

The explicit original classic-linkage library adds the reached symbolic,
no-parameter CALL form. It follows public register linkage facts and supplies
no control-program service or imported macro implementation. Independent
link bytes and rejected-operand output checks qualify the host interface.

The explicit `interfaces/pdos31` library adds original sparse control-block
mappings from public layout facts and the retained PDOS compatibility
declarations. It preserves source-owned lowcore constants and supplies no OS
service. Independent field/length bytes and rejected-variant output checks
pass; PDOSSUP, PLOADSUP and SAPSTART assemble in normal and sanitizer builds.
Runtime service closure, whole-image linking and guest execution remain open.

The selected library also supplies original GETMAIN RU/RC and FREEMAIN
RU/RC/R register interfaces from public SVC120/SVC10 contracts. Four calls
pass independent linked register-setup checks and failure controls under
normal and sanitizer builds. Complete MVSSTART assembly now passes with
identical decks from both builds. Allocation, I/O, image and guest evidence
remain separate; no native macro source or expansion was used.

Selected original short OPEN/CLOSE placeholder lists, 96-byte QSAM PS DCB
templates (GL/PL/PM) and locate GET linkage now pass independent binary checks.
The DCB check covers all 308 linked bytes, including EODAD/EXLST relocations;
GET covers all 20 bytes. Other service forms remain explicit failures.

## Named PDOS services — 2 October 2026

The selected configuration now sets OS=PDOS. MVSSUPA retains the common
MVS implementation and conditionally rejects services the owned PDOS kernel
does not supply: TSO terminal/command processing, VSAM, IDCAMS and MODESET.
Normal sequential DD I/O, allocation, ATTACH/DETACH and supported diagnostics
retain their service paths. Prefix queries return no prefix; odd SWA tokens
return no address; DCB lookup stays within the current task. Subtask completion
uses PDOS's completion word without reading absent MVS TCB flags. The normal
MVS/TSO branches are retained under their other explicit configurations.

The SNAP flag address now groups its DCB-relative difference before adding
the independent SNAP-area base; its mathematical address is unchanged. The
lean omission patch is rebased to remove the whole guarded parser and its
stub reference. The SWAREQ patch still applies with zero fuzz. Every profile
has a new whole-source hash and needs its own future guest qualification.

All six handwritten PDOS support modules assemble with explicit z900 ISA and
literal/model/definition limits. Normal and ASan/UBSan objects match exactly.
The 53 affected assembler/runtime checks pass in both builds. Current recovery
reproduces all 81 selected files from pinned upstream with zero-fuzz patching;
actual repaired I/O functions and their old failing controls also pass their
host checks. This establishes source and host assembly evidence only.
