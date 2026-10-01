# Repaired PDIO1 and consolidated runtime checkpoint

2 October 2026. **One maintained OS/runtime selection; the complete Classic
compile/assemble/link/image build passes locally.** Real load-module
reconstruction, disk payload and IPL checks pass. Boot and guest application
qualification remain separate gates.

The original exact qualified input is preserved by commit
[`d62b109`](https://github.com/adesutherland/z-pdos/commit/d62b109ed986bd455732484173ff4ebe0533045a).
All 37 OS files remain unchanged. We compared its 44 runtime inputs to the
maintained library, merged the relevant differences and removed the duplicate
runtime directory. The current OS and application recipes use `pdpclib/`,
with explicit configuration/service profiles for genuine target differences.
The current selection contains 81 files, 1,400,063 bytes.

| Current record | SHA-256 |
| --- | --- |
| Repository-relative maintained selection manifest | `ea155653d15c92b7be8e0f9759e8b6e012921d3974784b6dd6e55ec7155d143c` |
| Selected canonical upstream manifest | `53b98cece9972a9589d54407de4094119a5a40a1f725cbc2d3b85a3765d2d813` |
| Prepared-layout manifest | `5c315c80f87c00864c4ad4019d519999b448d2775b3e510ece756435e3e3adb9` |
| Consolidated current OS/runtime recovery patch | `3999b3a6712290c7cede320fd06c9a42de74accb32ab58941fed293b728303e1` |

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

## First Classic object — 1 October 2026

The retained `tests/classic-call.c` function compiles at `-Os` with the current
Classic C MVS producer. The assembler reads unchanged PDPTOP/PDPPRLG/PDPEPIL
through explicit maintained library inputs plus original selected SAVE/RETURN
definitions in `pdpclib/interfaces/classic-linkage`. No native object is reused.
`tools/check-classic-call.crexx` repeats this PD-03 gate and independently checks
the entire 92-byte layout, CP037 entry identifier, register save/restore, result
path, section/export metadata and two A relocations with expected values 88
and 46. The probe uses only the existing S/370 instruction subset; it does not
qualify a named z/Architecture kernel profile or guest execution.

Affected normal and sanitizer suites pass 38/38; macros-disabled bootstrap
checks pass 21/21. The full source assembly gate remains open. Its reached
gaps include local V references, CLM/explicit-length literals, compiler fixed
constants and support-source directives. Accepted PDIO1 guest evidence remains
separate from this new object proof.

## All Classic C objects — 1 October 2026

`assemble.crexx` prepares the single maintained source/runtime selection,
compiles all 17 selected units, and independently assembles their unmodified
output with the selected HFP/source subset and explicit literal limit 4096.
The PDOS kernel object contains 13,349 statements, 1,043 symbols and 670
fixups; STDIO contains 10,171 statements, 968 symbols and 422 fixups. No native
objects are reused. Exact decimal floating conversion belongs to the compiler;
it publishes XL4/XL8 target HFP bits, while the assembler encodes reached
HFP instructions with historical FPR restrictions.

This closes the C-object portion of PD-04. It does not close the six handwritten
support modules, named z/Architecture kernel code-role ceiling, helper/name
closure, linking, image construction or boot/application gates. The accepted
Lab guest remains untouched.
Affected normal/sanitizer suites pass 42/42; all 17 C objects also pass
through the sanitizer assembler using the retained recipe.

## Complete selected assembly — 2 October 2026

All 17 C objects and six handwritten modules now assemble independently.
MVSSUPA emits 3,981 statements, 20 sections, 908 symbols and 50 fixups.
The six normal and sanitizer decks are byte identical. The named PDOS service
configuration explicitly rejects absent MVS/TSO facilities; its details are
in PDPCLIB's change record. The current 81-file selection has new hashes above.
Clean upstream recovery and real-function pre/post-repair controls pass.
The 53 affected assembler/runtime regressions pass in normal and sanitizer
builds. No guest state or accepted PDIO1 image has been changed.

## Complete Classic source-to-image build — 2 October 2026

`image.crexx` now retains the full native macOS recipe. It starts with the
single maintained OS/runtime selection and Classic C after the numeric
string-escape repair in `84279f9`. All 17 C units and six handwritten support
modules are built afresh; no earlier native object, archive or disk is used.
`link.crexx` closes the common runtime and program-specific helpers explicitly.

PDOS's actual `fixPEMode` reads each MVS RDW module under ASAN/UBSAN and
reconstructs its complete payload against separate flat links at zero and
2 MiB. AMODE31/RMODE24, entry offset, relocations, final alignment zeros and
three malformed-input controls pass for each placement.

| Fresh module | RDW bytes | Flat bytes | Entry offset | SHA-256 of disk payload |
| --- | ---: | ---: | --- | --- |
| PLOAD | 145,920 | 129,448 | `0x13fc8` | `a745ecd53d13141bfd041c8d6f4d47cb43e2803bf5587f25394ebb6e83f26424` (flat) |
| PDOS | 180,680 | 157,432 | `0x14138` | `d1d83ab23f222cb7f1733e6d624d847a345902d6965a6b35096276d44cb5ab18` (RDW) |
| PCOMM | 105,416 | 88,333 | `0x13a90` | `8075781324b2a4817680120dedc8ac5b547eed6429f77d755c2611a81b724c14` (RDW) |

Hercules 4.9.1.0-SDL host utilities place those payloads on a new 100-cylinder
3390. PLOAD starts at cylinder 0/head 1, the two-cylinder VTOC at cylinder 1,
PDOS at cylinder 3, CONFIG at cylinder 4 and COMMAND at cylinder 5. Physical
fixed 18,452-byte program blocks preserve the RDW stream and zero final
padding. `CONFIG.SYS` contains target bytes for `0009 3270` and NEL; it is
not inferred from the host newline or locale.

The original C90 `install-ipl.c` writes the source-described IPL1/2 CCWs and
checks separately expected vectors. Its startup PSW is the fresh PLOAD's
`000c000080002010`. The checker validates disk geometry, IPL keys/counts,
VTOC names/extents/attributes and every source payload/padding byte. It checks
that installation changes only the 168 IPL payload bytes. Compression and
decompression preserve every guest byte; Hercules may regenerate only the
12-digit host-container serial. Five one-byte geometry/count/PSW/payload/IPL
corruptions fail and an existing output's hash is preserved.

The recipe requires a new ignored output directory and writes source/tool
hashes, final payload/image hashes, versions and check logs there. The locally
built image is a new candidate. PD-05 is complete; PD-06 boot and PD-07 cREXX
application acceptance remain open. The accepted Lab image and guest state
were not accessed or changed.
