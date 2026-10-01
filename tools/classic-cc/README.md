# Mainframe Classic C

`mf-classic-cc` now maintains the GCC 3.4.6 / cc370 cross-compiler source in
this repository. It combines the recorded baseline's successor with Mike
Beer's compiler repairs and the relevant Mainframe Lab macOS host repair.
The [source record](SOURCES.md) identifies the inputs and reconciliation.
The available v6 compiler series is integrated. Mike's later report names an
18-patch series, but the build-86 and build-92 attachments contain CMS card
decks rather than that source bundle. We have manually checked the relevant
described failures as far as the available code permits; the
[manual reconciliation](MANUAL-RECONCILIATION.md) records reproduced defects,
passing cases and limits. The later patch count is not an exact source identity.
This is a source and native host code-generation checkpoint. The independent
`mf-classic-as` integration and guest qualification are still open.

Run from the repository root with native C development tools, `make` and cREXX:

```sh
crexx tools/classic-cc/build.crexx --args test mvs
crexx tools/classic-cc/build.crexx --args test cms
build/classic-cc/mvs/gcc/mf-classic-cc -S -O1 hello.c -o build/hello.asm
```

The MVS target is `i370-ibm-mvspdp`; the CMS target is `i370-ibm-cms` and
defines `__CMS__`. Both emit the inherited PDPCLIB macro calling convention,
HLASM-style assembly text and EBCDIC C character values. `int`, `long` and
pointers are 32 bits, `long long` is 64 bits and plain `char` is unsigned.
This does not select AMODE31 or provide a 64-bit ABI.

The development launcher supports `-S`, `-E`, `-fsyntax-only` and compiler
queries. It rejects object generation, the default assemble/link path and
unqualified `--profile` selections. It runs the sibling GCC driver and `cc1`
from its build directory; rebuild after moving the checkout. There is no
as370 dependency or fallback. Target headers and runtime closure must be
supplied explicitly for application work; `--without-headers` is not a sysroot.

Build outputs and private input archives stay in ignored `build/classic-cc/`.
The root CMake build continues to build the assembler, linker and library
checks; the legacy compiler uses its own inherited configure/Makefile build.

`-O0`, `-O1`, `-O2` and `-Os` pass the focused consolidation fixtures.
Upstream's default disabling of strict aliasing and unit-at-a-time remains;
explicit overrides are not a claim of general optimized-program correctness.
The inherited regression suite also checks page-boundary branches, DI helpers
and EBCDIC folds. See [the checkpoint](CHECKPOINT.md) for exact results and
[the roadmap](ROADMAP.md) for the remaining toolchain work.

Shared machine-profile names and instruction ceilings are recorded in
[the architecture contract](../../architecture/MACHINE-PROFILES.md). The
Classic ABI, character model, object format and OS services remain separate
from the Mainframe ELF SDK. No named Classic machine profile is qualified yet.

The inherited compiler and its derived repairs retain [GPL terms](COPYING).
The original launcher, cREXX orchestration and new fixtures are MIT.
`UPSTREAM.md` preserves the upstream description of a wider toolchain; its
assembler, linker and packaging claims do not describe this compiler import.
