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

[Version 0.2.1](https://github.com/adesutherland/z-pdos/releases/tag/v0.2.1)
· [Run z/PDOS](pdos/doc/user/README.md)
· [Understand the architecture](pdos/doc/architecture/README.md)
· [Documentation](doc/README.md)

Version 0.2.1 adds the retained Workbench console, a small C record line editor,
utilities, a DASD allocation graph and canonical cREXX TSO31 commands.
The default machine has 512 MiB real storage and generous kernel/assembler
capacities. The [release record](pdos/doc/qualification/RELEASE-0.2.1.md)
links the bounded console and application qualification. Complete 3270
conformance, larger files and disk-layout expansion remain separate work.

## What works today

Version 0.2.0 introduces **a protected kernel address space (K) and one shared
application address space (U)**. A 64-bit assembler nucleus handles
interruptions; Classic C services run with 31-bit pointers inside K. PCOMM,
the command processor, runs in U alongside the selected native applications.
K owns loading, memory, files, terminal access and cleanup. This separation
removes the kernel's body and tables from scarce application storage below
16 MiB while preserving the existing synchronous command and application-call
behavior. It does not introduce a native 64-bit C compiler or a general
multi-user scheduler.

The [completed P0–P6 qualification](pdos/doc/qualification/TWO-SPACE-P6-2026-10-07.md)
runs unchanged cREXX CMS31 and TSO31/TSO64 ANY/HIGH packages through fresh
compiler → assembler → virtual-machine chains, interactive input and exact
file-output readback. CMS24 and native TSO24 run their bounded, library-free
RXVM IO24 workloads. Full-library TSO24 and CMS24 compiler/assembler support
remain outside that result. Selected native CMS-to-CMS and TSO-to-TSO calls
restore the caller's registers, file cursors, personality state and terminal
ownership after return or a recoverable child fault.

The normal development machine uses **one z/Architecture CPU and 512 MiB real storage**.
The C console driver supports configured 3270 models 2–5, a line-only primary,
and an optional Telnet 3215 text monitor. Ordered text is also committed to a
durable store on the IPL disk; raw screens or lost monitor output produce
explicit capture gaps. Existing mount/select/unmount, allocation, record-copy
and raw tape operations use their shared implementation through checked K
services. See the [operator guide](pdos/doc/user/TWO-SPACE.md) and
[media guide](pdos/doc/user/MEDIA.md) for the accepted limits.

The build creates the boot loader, C31 handover, K64/C31 core and U command
processor from maintained source, then constructs and reads back a fresh disk.
**It requires no proprietary mainframe compiler, assembler, binder, IBM macro
library or prebuilt mainframe objects.** Classic C, Classic Assembler and
Classic Linker build the C/runtime route; GNU s390 Binutils assemble and link
the 64-bit nucleus. Hercules supplies the emulated machine and disk utilities.

The [0.2.0 release record](pdos/doc/qualification/RELEASE-0.2.0.md) distinguishes
candidate review, local build, operator acceptance and published artifacts.
Host tools are packaged for macOS Apple Silicon/Intel, Linux x64 and Windows
x64. The base image contains the OS; application packages are separate inputs.
The earlier [0.1.1 release](https://github.com/adesutherland/z-pdos/releases/tag/v0.1.1)
and its one-space qualification remain historical references. The explicit
`one-space-image.crexx` recipe can still reproduce that architecture.

The system implements the CMS/TSO service forms reached by its qualified
workloads. It is not a general replacement for CMS, TSO or z/OS. The
[backlog](pdos/doc/BACKLOG.md) records remaining service and qualification work.

## How the pieces fit

```text
OS + PDPCLIB C ── Classic C ──► assembler source
handwritten C31/native assembler + PDPCLIB macros
                               │ Classic Assembler
                               ▼
                        classic object decks
                               │ Classic Linker
                               ▼
                     loader, C31 services and U shell

K64 nucleus assembler ── GNU s390 assembler/linker ──► K64 core

                   both routes + image recipe
                               │ Hercules disk utilities
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
published 0.2.0 base OS; cREXX applications for that release are separate
downloads. The [0.2.1 development source image](pdos/doc/user/APPLICATIONS.md)
also builds a small C line editor, utilities and examples, and bundles the
pinned cREXX TSO31 compiler, assembler and VM.

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
