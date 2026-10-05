# z/PDOS and Mainframe Classic Tools

Build mainframe software on an everyday computer, from C source to a bootable
operating system. This repository contains **z/PDOS**, a small mainframe OS,
and the compiler, assembler, linker and C library used to build it.

**Paul Edwards created PDOS and PDPCLIB.** His work is the foundation of the
operating system and runtime maintained here. z/PDOS is our fork of his
mainframe implementation, with its own development direction and preserved
upstream attribution.

I want to make mainframe systems understandable and practical to develop:
read the source, change it, build it locally, and see what happens on the
machine. [cREXX](https://github.com/adesutherland/CREXX), a Rexx language
implementation, is our first substantial application. Its compiler, assembler
and virtual machine exercise loading, storage, files, terminal input and error
handling together.

[Download 0.1.0](https://github.com/adesutherland/z-pdos/releases/tag/v0.1.0)
· [Run z/PDOS](pdos/doc/user/README.md)
· [Understand the architecture](pdos/doc/architecture/README.md)
· [Documentation](doc/README.md)

## What works today

The 0.1 milestone established a complete build from maintained source using
Mainframe Classic C, Classic Assembler and Classic Linker. It produces the
boot loader, kernel and command processor, then constructs a fresh disk.
**The mainframe build requires no proprietary compiler, assembler, binder,
IBM macro library or prebuilt mainframe objects.**

The recorded source-built system boots under Hercules and runs unchanged
cREXX TSO31, TSO64 ANY and TSO64 HIGH application packages. That includes
compiling and assembling Rexx programs, running the results, interactive input
and file-output readback. The [guest qualification record](pdos/doc/qualification/QUALIFICATION.md)
names the exact source, binaries, machine and remaining limits.

The later [stage 3 current-source result](pdos/doc/qualification/STAGE3-2026-10-05.md)
adds checked unchanged CMS31 RXVM/RXAS/RXC MODULE execution and a separate
fixed-origin CMS24 RXVM IO24 subset. This local guest result is newer than the
published 0.1.0 image and does not qualify general CMS services.

z/PDOS currently has a **32-bit C kernel running in 31-bit addressing mode**.
Handwritten assembler preserves and dispatches 64-bit application contexts;
selected applications can also load code above 4 GiB. A native 64-bit C kernel
is future work. The current system has one address space and synchronous
application execution. It implements the MVS-style services needed by the
qualified workloads; it is not a general replacement for z/OS or TSO.

The [0.1.0 release](https://github.com/adesutherland/z-pdos/releases/tag/v0.1.0)
provides macOS Apple Silicon and Intel packages, Linux x64 tools, signed
Windows x64 packages, a source archive and a base OS disk image. All four host
builds and relocated compiler/assembler/linker checks passed. The release
image passed host loader and disk checks; it does **not** carry a new guest
qualification beyond the separately recorded milestone run.

Native TSO24 AMODE24/RMODE24 direct loading, broader file semantics and parts
of the console interface remain open. Start with the [known issues](pdos/doc/BACKLOG.md)
when deciding whether the present system fits an experiment.

## How the pieces fit

```text
OS + PDPCLIB C ── Classic C ──► assembler source
                               │
handwritten assembler ─────────┤
PDPCLIB native code and macros ─┘
                               │ Classic Assembler
                               ▼
                        classic object decks
                               │ Classic Linker
                               ▼
                     loader, kernel and shell
                               │ image recipe + Hercules disk utilities
                               ▼
                       bootable z/PDOS disk
```

| Component | Role |
| --- | --- |
| [z/PDOS](pdos/README.md) | Boot loader, kernel, program loader, disk and console services, and PCOMM command processor. |
| [PDPCLIB](pdpclib/README.md) | C library and native support. Explicit profiles choose the system services and source modules used by a build. |
| [Classic C](compiler/README.md) | GCC 3.4.6/i370/cc370-derived C compiler. The MVS and CMS launchers produce assembler text; the MVS route builds the OS. |
| [Classic Assembler](assembler/README.md) | Original portable assembler by Adrian Sutherland. Produces classic mainframe object records from the supported source language. |
| [Classic Linker](linker/README.md) | PDLD-derived linker. Resolves classic objects and produces flat images, native load modules and XMIT transports. |
| [TSO31 entry bridge](tso31-bridge/README.md) | Separate experimental adapter for calling an ELF program from a 31-bit TSO entry. It is not the z/PDOS kernel. |

The separate [Mainframe ELF SDK](https://github.com/adesutherland/mainframe-elf-sdk)
uses modern GCC, GNU Binutils and Newlib for application development. It
consumes selected Classic tools and PDPCLIB native services from this
repository. The two toolchains share some components, but their C calling
conventions and runtimes differ. An ELF object cannot be handed directly to
the z/PDOS loader: the application route must produce the supported native
load format and obey its service contract.

[Profiles and interfaces](doc/PROFILES.md) explains how hardware, C data width,
application addressing mode and runtime services are selected independently.
The [OS architecture guide](pdos/doc/architecture/README.md) follows boot,
loading, memory and service calls through to their implementation.

## Get started

To explore the OS, download the **PDOS image ZIP**, keep an untouched copy,
and follow the [boot guide](pdos/doc/user/README.md). The image contains the
base OS only; cREXX applications are separate downloads.

To develop software on your computer, choose the **Classic tools package**
for your host. The [installation guide](doc/BUILD-AND-RELEASE.md) explains the
installers, portable ZIPs and source build. See the
[compiler guide](compiler/doc/user/README.md) for its supported operations.
Installing the tools does not install a mainframe guest or a target sysroot.

For OS development, read the [source and change guide](pdos/doc/development/README.md)
after the architecture. Work takes place on `develop`, the public default
branch. Each component has one maintained `src/` tree and one
`doc/BACKLOG.md`; optional `archive/` directories are frozen references and
are excluded from normal builds. [The shared workflow](doc/WORKFLOW.md)
describes how changes and their evidence are recorded.

## Origins and licensing

Alongside Paul Edwards's PDOS and PDPCLIB, Classic C preserves the work of
Jan Stein, Dave Pitts, Linas Vepstas, Paul Edwards and other GCC/cc370
contributors, including Mike Beer's recorded repairs. Classic Linker derives
from PDLD. Each component's `UPSTREAM.md` records its lineage and contributors.

We avoided proprietary build dependencies by maintaining the toolchain and
implementing the selected assembler macros and service interfaces in source.
That does not make every file MIT-licensed: inherited public-domain notices,
GNU GPL terms and file-level exceptions remain in force. The
[licensing and provenance guide](LICENSES.md) explains the boundary, including
external operating systems used for qualification. No IBM operating system,
macro library or private guest disk is included in the release.
