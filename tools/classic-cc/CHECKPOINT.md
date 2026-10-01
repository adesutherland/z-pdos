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
  `ec8fdaffd9c60e173203703f92c07f330d04c10b04ba0b7d008ec87af562de3f`.
- Consolidated recovery patch SHA-256:
  `cb25690232b7ed5be111142d82912a7abd9c494cc02724fccd4ad77a23a1cbca`.
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
