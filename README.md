# z/PDOS and Mainframe Classic Tools

Cross-platform development tools and an operating system for mainframe software,
with [cREXX](https://github.com/adesutherland/CREXX), a REXX language
implementation, as their first substantial application.

I want to make it practical to develop mainframe software on an everyday
computer: edit the source, cross-compile it, build an executable, and test it
on a mainframe system. This repository brings together the compiler, assembler,
linker, C runtime and z/PDOS operating system needed for that work.

cREXX gives us a real language implementation and real applications to develop
and test. Its compiler, assembler and runtime exercise much more than a small
demonstration program: they need working file I/O, character encoding, memory
management, loading and error handling. As cREXX and other applications develop,
their needs will shape these tools. The aim is to turn problems encountered by
those applications into useful, tested improvements to the shared toolchain.

## A maintained fork, with an acknowledged heritage

**Paul Edwards created PDOS and PDPCLIB.** His work is the foundation of the
operating system and C runtime maintained here. z/PDOS is our fork of the
mainframe PDOS implementation, and its name identifies the system developed
in this repository.

The tools have their own histories too. Mainframe Classic C descends from
GCC 3.4.6 and the i370/cc370 work of **Jan Stein, Dave Pitts, Linas Vepstas,
Paul Edwards** and other GCC and cc370 contributors. It incorporates recorded
repairs by **Mike Beer**, alongside our integration and portability work.
Mainframe Classic Linker derives from **PDLD** and preserves its contributors'
work and notices. Mainframe Classic Assembler is an original implementation
by **Adrian Sutherland**.

These contributions make the project possible. Each component's `UPSTREAM.md`
records its origins, contributors and the changes brought into our maintained
source. Original notices and licences stay with that source.

This is an evolving fork with its own development direction. We will make
choices around the applications and mainframe environments we are supporting,
and those choices may differ from upstream. We intend to keep that development
respectful and traceable: credit the people whose work we build on, explain our
changes, and make useful work easy to review and share.

## The first milestone: cross-building z/PDOS 0.1

On **2 October 2026**, z/PDOS 0.1 was built from source on an Apple Silicon Mac
using the Mainframe Classic compiler, assembler and linker maintained in this
repository. The complete OS build uses **no separately supplied mainframe
compiler, assembler or binder, and no prebuilt mainframe objects**.

That build produces the loader, operating system and command processor, and
constructs a fresh bootable disk image. The resulting system boots under
Hercules and runs the unchanged cREXX TSO31, TSO64 ANY and TSO64 HIGH packages.
Those checks include compiling and assembling cREXX programs, running the
results, interactive input, diagnostics and complete file-output readback.

Ordinary host build tools are still needed to build the Classic tools, and
Hercules supplies disk utilities and emulation. The independence achieved here
is the mainframe compile, assemble and link chain, taking our maintained source
through to a system running real applications.

The [0.1 qualification record](pdos/doc/qualification/QUALIFICATION.md) describes
the exact machine, inputs, results and limits. The subsequent
[repository reorganisation](doc/REORGANISATION-20261002.md) preserved the OS
payloads and guest-visible disk bytes, with all 91 integration checks passing.

The current accepted route is the documented Hercules configuration. Loading
24-bit applications and several operator-interface improvements remain in the
[z/PDOS backlog](pdos/doc/BACKLOG.md). Running the build tools on mainframe
hosts themselves is a further development goal.

## What is in the repository?

| Component | What it provides |
| --- | --- |
| [z/PDOS](pdos/README.md) | The mainframe operating system, loader and command processor |
| [PDPCLIB](pdpclib/README.md) | The shared C library, native support and explicit system-service configurations |
| [Mainframe Classic Assembler](assembler/README.md) | `mf-classic-as`: a portable assembler for the supported classic mainframe source and object formats |
| [Mainframe Classic C](compiler/README.md) | `mf-classic-cc`: the maintained GCC/i370/cc370-derived C cross-compiler, with MVS and CMS targets |
| [Mainframe Classic Linker](linker/README.md) | `mf-classic-ld`: the PDLD-derived linker used to produce mainframe load modules |
| [TSO31 entry bridge](tso31-bridge/README.md) | An experimental adapter between a 31-bit TSO entry and an ELF program |

[Machine definitions](machines/README.md) describe the hardware targets shared
by the work. Each component documents its own calling conventions, object
formats and operating-system services; a shared machine name alone does not
make two toolchains interchangeable.

The separate [Mainframe ELF SDK](https://github.com/adesutherland/mainframe-elf-sdk)
provides the modern GCC/GNU assembler route. It is a related project with its
own source, build process and application interfaces.

## Build and explore

The complete source-to-image route has been checked on Apple Silicon macOS.
You need a native C compiler, make, CMake and cREXX for the commands below.
Other host and target combinations have their own component checks and
qualification work.

From the repository root, build and check both Classic C variants, then the
assembler, linker, entry bridge and focused C-library tests:

```sh
crexx -nokeep compiler/scripts/build.crexx --args test mvs
crexx -nokeep compiler/scripts/build.crexx --args test cms
crexx -nokeep scripts/build.crexx --args test
```

To build a fresh OS image, follow the [z/PDOS build guide](pdos/doc/user/README.md).
It explains the additional Clang and Hercules utility requirements and the
image checks. The [compiler guide](compiler/doc/user/README.md) and other
component guides describe individual tool use. Generated outputs go into
ignored `build/`; `-nokeep` removes cREXX's temporary compiler files.

The [build and release guide](doc/BUILD-AND-RELEASE.md) describes the prepared
GitHub runner matrix, macOS PKG and Windows installers, local Windows signing,
and tag-driven releases. The recipes have local Apple Silicon checks;
hosted platform and signing qualification still require their first run.

## Working on the project

Each component has one current `src/` tree and its own user, architecture,
development, AI and qualification documentation under `doc/`. We maintain
source directly, with changes recorded through normal Git commits. Optional
`archive/` directories preserve frozen upstream baselines as acknowledgement
and reference; builds and required tests work without them.

Bug reports and proposals are especially useful when they come with a real
program, a named target and a reproducible result. Each component's
`doc/BACKLOG.md` keeps its defects, planned improvements and missing checks
together. The [shared workflow](doc/WORKFLOW.md) explains how we take an item
through implementation and validation. Read [AGENTS.md](AGENTS.md) for the
repository's engineering and source-maintenance rules.

Licensing follows the individual components and their inherited source.
Original project material is MIT-licensed; the compiler retains its GPL terms
and applicable exceptions, while inherited PDOS, PDPCLIB and PDLD material
retains its own declarations and notices. [LICENSES.md](LICENSES.md) maps those
terms and links to the component licences and upstream records.
