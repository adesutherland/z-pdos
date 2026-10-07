# Profiles and interfaces

A machine name alone cannot tell you whether a program will run. The CPU must
understand its instructions, the caller and callee must agree on registers and
data layout, and the operating system must provide the services it requests.
The word *profile* appears at several of these boundaries in this project.

## Four choices that must agree

| Choice | What it controls | Example |
| --- | --- | --- |
| Machine / instruction ceiling | Instructions the assembler may emit and the CPU must execute. | The active OS uses standard z/Architecture with a z900 instruction ceiling. |
| C data model and calling convention | Sizes of pointers and integers; argument, register and stack rules. | Classic C uses 32-bit pointers. Modern ELF LP64 application code uses 64-bit pointers and a different calling convention. |
| Addressing and residence modes | How execution interprets addresses and where the loader may put code. | AMODE31 runs with 31-bit addressing. RMODE24 requires code below 16 MiB; RMODE ANY allows residence below 2 GiB. |
| Runtime / service selection | Startup, C library and operating-system calls supplied to the program. | PDPCLIB `pdos-zarch` selects the OS build's native support; `tso31-sdk-files` selects a narrower TSO dataset service. |

AMODE64 permits 64-bit execution; it does not require that the code itself be
above 2 GiB. The qualified TSO64 ANY application route has low-resident code
and a high heap. TSO64 HIGH adds a low launcher which loads an RMODE64 body
into the OS's high code window. The 0.2 kernel uses an AMODE64 assembler nucleus and protected Classic C31
services. Application AMODE does not change the C service pointer width.

## The active OS build

The OS recipe selects these inputs explicitly:

- Classic C's **MVS** variant, with `ZARCH`, `__MVS__`, `__PDOS390__` and
  `USE_MEMMGR` compile definitions and the selected PDPCLIB headers.
- PDPCLIB's **`pdos-zarch`** source configuration, including its macro settings
  and selected native modules.
- Classic Assembler's **z900** instruction ceiling and the source-owned
  linkage and PDOS service macros.
- PLOAD/C31 handover **AMODE31, RMODE24**, protected nucleus **AMODE64**,
  K services **Classic C31**, and U command processor **AMODE31, RMODE ANY**.
- GNU s390 assembler/linker for the K64 nucleus, with the z900 ISA ceiling.
- The [documented Hercules machine](../machines/profiles/pdos-zarch.md).

These are choices made by the build recipes. They are not an implemented
universal `--profile` option across every tool. Classic C's launcher rejects
unqualified named profile requests. Proposed shared selectors in the machine
design documents remain development work.

Inherited macro names such as `S380` occur in the selected source. In this
build they participate in the existing source and linkage conventions; they
do not select the community S/380 architecture. The active OS is standard
z/Architecture. The fictional architecture discussed in separate research is
not an implementation target here.

## PDPCLIB service profiles

These names select files under [`pdpclib/src/profiles/`](../pdpclib/src/profiles/).
Preparation copies the common source, chooses `PDPTOP`, and joins the listed
`MVSSUPA` modules. It does not patch a hidden upstream copy or infer services
from the host computer.

| Selection | Purpose and present boundary |
| --- | --- |
| `pdos-zarch` | Active OS runtime, with standard z/Architecture support. Used by the recorded source-built z/PDOS milestone. |
| `pdos390-esa` | Retained ESA/390 configuration; not interchangeable with the qualified z/Architecture image. |
| `mvs38-s370-24` | Historical MVS 3.8/System/370, 24-bit configuration. It does not establish z/OS or z/PDOS AMODE24 support. |
| `tso-zos15-24` | Retained 24-bit caller compatibility configuration; its SWA lookup temporarily uses AMODE31. |
| `tso31-lean` | Limited inherited TSO selection with the unused prefix parser omitted. Its broader native source still encounters an unsupported VSAM `SHOWCB` form in the maintained assembler route. |
| `tso31-sdk-files` | Selected sequential/PDS service used by the ELF SDK. TSO31 and TSO64 file checks ran on z/OS 1.5. TSO24 dataset I/O, positioning and broader service paths remain open. |

The last selection was added on `develop` after the z/PDOS 0.1.0 release source.
Do not assume an older package contains it. The
[PDPCLIB guide](../pdpclib/doc/user/README.md) gives preparation commands and
links the exact service limits.

## Classic tools and the ELF SDK

The Classic route builds the OS using PDPCLIB as its C library. Classic C
emits assembler using its inherited mainframe linkage macros; Classic Assembler
turns that text into classic object cards; Classic Linker builds the native
load images.

The [ELF SDK](https://github.com/adesutherland/mainframe-elf-sdk) uses a modern
GCC/Binutils compiler route and Newlib. Its own entry code and adapters connect
ELF application conventions to native services. It uses selected PDPCLIB
assembler services without replacing Newlib with the entire PDPCLIB C library.
Its exporters and native tools produce the load format required by the target.

There are therefore two distinct compatibility questions: whether two objects
can be linked, and whether the resulting program can call the runtime and OS
correctly. Matching instruction sets or filenames answers neither question.
For a reproducible result, record the compiler variant, data model, runtime
selection, macro inputs, load modes and execution environment together.
