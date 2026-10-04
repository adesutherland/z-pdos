# Mainframe Classic C

Classic C is the maintained GCC 3.4.6/i370/cc370 C cross-compiler. Its MVS
launcher is `mf-classic-cc`; its CMS launcher is `mf-classic-cc-cms`. They run
on the host computer and produce mainframe assembler text for the next stage.

The compiler preserves the work of Jan Stein, Dave Pitts, Linas Vepstas,
Paul Edwards and other GCC/cc370 contributors, including Mike Beer's recorded
repairs. Inherited source and derived changes retain their GNU GPL and other
file-level terms; the original launcher and integration have their own stated
MIT scope.

Both variants use 32-bit `int`, `long` and pointers, unsigned plain `char`,
EBCDIC characters and the inherited PDPCLIB macro calling convention. This
compiler does not generate a native LP64 C kernel. Its data model also differs
from the modern [ELF SDK](https://github.com/adesutherland/mainframe-elf-sdk)
application route.

The launcher supports `-S`, `-E`, `-fsyntax-only` and compiler queries. It
rejects object generation, implicit assembly/linking and unqualified named
profile requests. Target headers and runtime are explicit inputs. MVS and CMS
host code-generation checks pass; the selected MVS route builds z/PDOS. That
does not establish a complete Classic CMS application/runtime port.

- [User guide](doc/user/README.md)
- [Architecture and interfaces](doc/architecture/README.md)
- [Development](doc/development/README.md)
- [Known issues and next steps](doc/BACKLOG.md)
- [Origins and contributors](UPSTREAM.md), [licence](LICENSE), [agent rules](AGENTS.md)

`src/` is the maintained implementation; `scripts/` contains recipes run from
the repository root. Generated output belongs under ignored root `build/`.
Optional frozen `archive/` material is reference only and is excluded from
normal builds. Each component has one backlog and records its own evidence.
