# Classic C source and host checkpoint

1 October 2026. **Source integration and native host code-generation proof.**
This is not a release or a qualified target toolchain.
The available upstream, v6 and Lab repairs are reconciled and passing, with
manual checks of the feasible later defect descriptions. Mike's reported
18-patch inventory remains unverified; this checkpoint does not claim an exact
copy of that private source series.

## Inputs

- Upstream cc370: `ece26349fc1096804e82e5618bb8255054035247`, compiler subtree
  `6f54ea46dc2c9f92a0b32975485855d8b60cf3eb`.
- Mike Beer's 22 September v6 compiler series, reconciled as described in
  [SOURCES.md](SOURCES.md), plus the applicable Mainframe Lab Darwin ARM64 repair.
- Reproduced surviving comment-emission defects and checked later descriptions
  as recorded in [MANUAL-RECONCILIATION.md](MANUAL-RECONCILIATION.md).
- Maintained 819-file source manifest SHA-256:
  `9158c6337d091cee9f547ee2c69ebad5688c724f05e2c93231afc4f0aad5217b`.
- Consolidated recovery patch SHA-256:
  `95b0488325bdd50ea249760665978b00ea25834c58514602e2be4c676f818e04`.
- Native host: macOS 26.6.2, build 25G83, Apple Silicon;
  Apple Clang 21.0.0 (`clang-2100.1.1.101`), GNU89 host compiler mode.
- Targets: `i370-ibm-mvspdp` and `i370-ibm-cms`, inherited PDPCLIB macro ABI,
  EBCDIC character values, 32-bit `int`/`long`/pointers.

The modified compiler files carry dated notices and retain their inherited
licences. A clean copy of the pinned compiler subtree plus the consolidated
patch reproduces every maintained source file exactly. All 819 source checksum
entries passed. Correspondence and native packages remain outside Git.

## Passing checks

Both native builds complete with:

```sh
crexx tools/classic-cc/build.crexx --args test mvs
crexx tools/classic-cc/build.crexx --args test cms
```

Each target passes all 23 inherited regression groups, including 246
page-boundary variants, six of which exercise a back edge at the new page's
first instruction. The suites retain the signed/unsigned DI helper and shift
checks, EBCDIC folds, UTF-8 input checks and optimization-default checks.

The original consolidation fixtures pass at `-O0`, `-O1`, `-O2` and `-Os`
for each target. They check deterministic long-name linkage across translation
units, `#pragma map`, existing runtime helper symbols, multi-page computed
gotos with a nonzero address register, `BALR` base reloads at both address-taken
targets, and nested calls with an implicit conversion helper before publishing
the outer arguments. A 700-character function identifier compiles without the
reproduced comment-formatting crash, and long function, global and common
comments stay within 71 columns. Optimized newline and repeated outgoing
argument-store checks pass; the focused repeated-call check also passes at
`-O3`, without establishing general `-O3` qualification.
An explicit unit-at-a-time fixture has a definition for its renamed static
reference. OS predefines and C widths pass compile-time assertions.
The launcher rejects object generation and unqualified named profiles.

The source reconciliation tests caught a runtime ABI regression introduced
by combining long-name hashing with existing helpers; the explicit eight-byte
helper names now preserve that linkage. Alias hashing is fixed to 32 bits
across hosts, with independently calculated expected names in the tests.

Build and test logs are disposable ignored outputs under
`build/classic-cc/{mvs,cms}/` and `build/classic-cc/*-build-driver.log`.

## Independent assembler gate

A minimal `int add(int a, int b) { return a + b; }` compiles at `-O1`.
Passing its unmodified output to the existing independent assembler with
`--profile s370 --macros` fails at line 1, `COPY PDPTOP`, with an unsupported
bootstrap feature diagnostic. No object is produced. The compiler also emits
PDPCLIB entry/exit macros, `EQU *`, address literals and literal pools, which
need a deliberate source-language contract rather than using another assembler
to mask the gap. No as370 implementation was imported, built or invoked.

This attempt does not establish assemble/link/load or guest execution.
The [roadmap](ROADMAP.md) owns the compiler/assembler tradeoffs and those
acceptance gates. Named machine profiles remain unqualified; their
[shared direction](../../architecture/MACHINE-PROFILES.md) does not imply
Classic/ELF ABI compatibility or wider addressing support.

The later PD-03 consumer now compiles `os/pdos/tests/classic-call.c` and
assembles its unchanged text through maintained macros. An independent checker
verifies its complete object bytes and relocations; see the
[OS checkpoint](../../os/pdos/CHECKPOINT.md). This supersedes the earlier COPY
probe for that bounded language path. Link/load, whole-source and guest gates
remain open.

## Representable fullword data — 1 October 2026

PDOS MATH exposed raw word 2827508273 emitted as an out-of-range signed F
constant. The producer now emits its equivalent signed value -1467459023,
preserving all target bits and automatic fullword alignment. Host-width-safe
arithmetic also covers 80000000, FFFFFFFF and both halves of a DI constant.
The assembler continues to reject out-of-range signed F values.

Both MVS and CMS host builds and all 23 inherited regression groups pass; the
new fixture passes at O0/O1/O2/Os. Clean recovery again reproduces all 819 files.
A compile/assemble/link check independently validates the fixture's 28 exact
bytes for both producers. This proves constant encoding through the flat-binary
route, without target execution or general floating-point qualification.

## Exact hexadecimal floating literals — 1 October 2026

Both MVS and CMS producers now emit XL4/XL8 literals from the existing GCC
i370 target real encoder, preserving its rounding, target word order and pool
widths. Independent one, half and negative-sixteen vectors pass at O0/O1/O2/Os;
all 23 inherited regression groups still pass for each producer. Clean source
recovery reproduces all 819 files. The Classic Assembler's selected HFP subset
now accepts all 17 compiled PDOS C units; handwritten support, named kernel
profile, linking/loading and guest execution remain open.

## Complete numeric-byte consumer — 2 October 2026

A reproduced pre-repair compiler output emitted 0x15 for both `\x25` and
`\045`. An original array containing all 256 numeric escapes then failed
independent assembler/link byte checking. After completing the newline/NEL
exchange, every byte 00–FF plus the string terminator passes at O0/O1/O2/Os
for both MVS and CMS producers, using normal and sanitizer Classic tools.
Existing inherited code-generation and consolidation checks pass for both
compiler builds. Zero-fuzz clean recovery reproduces all 819 source files.
These are code-generation/object tests; no target execution is claimed.

Repeat the byte consumer after building compiler and Classic tools:

```sh
crexx tools/classic-cc/check-bytes.crexx --args mvs build/classic-as/tools/classic-as/mf-classic-as build/classic-as/tools/classic-ld/mf-classic-ld build/classic-as/tools/classic-as/test_cli_output
```

## z/PDOS 0.1 consumer qualification — 2 October 2026

The MVS producer now builds the complete maintained OS through the independent
Classic assembler and linker. The resulting kernel boots and passes unchanged
cREXX TSO31, TSO64 ANY and TSO64 HIGH packages, including fresh compiler chains
and complete stopped-disk output readback. The [OS qualification record](../../os/pdos/QUALIFICATION.md)
pins the exact source/tool identities and limits. This is the selected OS
consumer qualification; it does not qualify the compiler's CMS variant,
native compiler hosting or unrelated application ABI/service profiles.
