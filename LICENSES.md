# Licensing, origins and build independence

The complete z/PDOS mainframe build uses source maintained in this repository:
Classic C, Classic Assembler, Classic Linker, PDPCLIB and the OS. It does not
require a separately licensed proprietary mainframe compiler, assembler,
binder or IBM macro library. Ordinary host build tools and Hercules utilities
remain external inputs.

This is a statement about the build's dependencies. The repository contains
several licences, and the [root MIT licence](LICENSE) covers only original
project material. It does not relicense inherited source or external manuals.

## Component terms

| Component | Origin and applicable terms |
| --- | --- |
| [z/PDOS](pdos/UPSTREAM.md) | Paul Edwards's PDOS. Inherited public-domain declarations and contributor notices remain; [original work has its stated MIT scope](pdos/LICENSE). |
| [PDPCLIB](pdpclib/UPSTREAM.md) | Paul Edwards's Public Domain C Library and its contributors. Preserve its [dedication, fallback permission and file notices](pdpclib/LICENSE). |
| [Classic Assembler](assembler/UPSTREAM.md) | Original implementation by Adrian Sutherland, under [MIT](assembler/LICENSE). No as370 implementation or inherited assembler opcode table was imported. |
| [Classic C](compiler/UPSTREAM.md) | GCC 3.4.6/i370/cc370 lineage and recorded repairs. Inherited source and derived changes retain their [file-level terms](compiler/LICENSE), including [GPL version 2 or later where stated](compiler/COPYING); library/header notices and exceptions remain with the files. The standalone launcher and original integration have their stated MIT scope. |
| [Classic Linker](linker/UPSTREAM.md) | PDLD lineage and contributors. Inherited public-domain declarations remain; original changes and integration have [their stated MIT scope](linker/LICENSE). |
| [TSO31 bridge](tso31-bridge/UPSTREAM.md) | Original entry adapter, interface description and checks, under [MIT](tso31-bridge/LICENSE). |
| [Machine descriptions](machines/UPSTREAM.md) | Original descriptions and cited interface facts, under [MIT](machines/LICENSE). External manuals and websites are not included or relicensed. |
| Root documentation and orchestration | Original project material, MIT. |

The compiler's GPL terms are part of the distribution, not a commercial
compiler licence requirement. Retain the corresponding source and notices
when redistributing its binaries. Review any runtime or header actually linked
into an application under that file's terms; the repository's root licence
cannot answer that question for all inherited ports.

Windows packages also carry the selected GNU libiconv runtime beside the
private compiler. Its notices, pinned package identity and matching source/build
archive are supplied under `share/z-pdos/host-libraries/libiconv`. This is a host
compiler dependency, separate from the mainframe runtime.

## How proprietary build dependencies were removed

The original native build used IBM assembler macros and an assembler/binder
available in its mainframe environment. The maintained route makes each
replacement visible:

1. **Compile C with maintained Classic C.** Its GCC/i370/cc370 source and
   recorded repairs are present, rather than supplied only as an opaque native
   compiler executable.
2. **Assemble with the original Classic Assembler.** Its parser, macro engine,
   encoders and object writer are maintained here.
3. **Supply the selected macro interfaces in source.**
   [Classic linkage](pdpclib/doc/architecture/CLASSIC-LINKAGE.md),
   [PDOS31 services](pdpclib/doc/architecture/PDOS31-INTERFACES.md) and
   [selected TSO interfaces](pdpclib/doc/architecture/TSO31-INTERFACES.md)
   implement the operand forms and control-block facts needed by the maintained
   callers. They are independently authored, bounded implementations, not copies
   of IBM MACLIB/MODGEN and not complete replacements for those libraries.
4. **Link with the maintained PDLD-derived linker.** The OS recipe builds all
   23 objects afresh and checks load reconstruction instead of reusing private
   native objects or binder output.
5. **Construct the disk from those outputs.** Hercules utilities and an
   explicit IPL writer create the base image; no existing IBM or private guest
   disk is used as a release template.

The [source-to-image dependencies](pdos/doc/architecture/DEPENDENCIES.md) show
where each input enters the build. Frozen `archive/` baselines retain upstream
notices for reference; normal builds use `src/` and do not require the archives.

## Boundaries of this claim

No IBM operating system, proprietary macro library, external architecture
manual, private native listing or private guest disk is bundled. Documentation
may cite published interface facts without granting rights to the referenced
material. The limited macro implementations must not be described as an IBM
macro-library distribution.

Hercules supplies CPU and device emulation; it does not provide a licence for
an operating system used as a guest. Some PDPCLIB/SDK service qualification was
performed on z/OS 1.5. Those results describe that environment, and do not make
z/OS part of this release or grant access to it. The supplied z/PDOS base image
is a separate system built from this project's source.

The full inherited library contains ports outside the selected build. A later
canonical VSE implementation with a recorded copyright question was not
imported by the runtime merge. Review actual file notices before bringing in
new upstream material or enabling another port; see
[PCL-003](pdpclib/doc/BACKLOG.md#pcl-003-retained-ports-and-file-level-notices).
