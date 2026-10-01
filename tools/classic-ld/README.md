# Mainframe Classic Linker

Mainframe Classic Linker, command `mf-classic-ld`, is the maintained PDLD
linker in z/PDOS. It is a sibling of Mainframe Classic Assembler. The inherited
implementation and its public-domain notices remain explicit; this is not an
independent linker implementation or a promise of IBM binder compatibility.

The current repair corrects classic object relocations when a control section
has a nonzero assembled origin. It also retains five earlier SDK fixes for
MVS/XMIT layout and packaging. The original focused tests generate their own
object cards and check independently expected bytes and relocation records.

Run from the repository root:

```sh
crexx tools/classic-ld/check.crexx --args test
crexx tools/classic-ld/check.crexx --args sanitize
build/classic-ld/mf-classic-ld --version
```

CMake and a C99 host compiler build the inherited linker. cREXX only runs the
development recipe; the resulting linker has no cREXX dependency. The original
fixture checker uses strict C90. Root integration is controlled by
`MF_CLASSIC_LINKER`; it does not weaken the assembler's strict C90 compilation.

Read [the user guide](docs/USER.md), [the repair design](docs/ARCHITECTURE.md),
[QA results and limits](docs/QA.md), and [the source record](SOURCES.md).
[readme.txt](readme.txt) is the unchanged upstream documentation.

The qualified route reads classic ESD/TXT/RLD/END objects and writes binary
images or the tested mixed PC/SD MVS/XMIT route. Other inherited backends have
not been qualified here. Host linking and package inspection do not establish
guest execution.
