# How z/PDOS works

z/PDOS is a small mainframe operating system descended from Paul Edwards's
PDOS. Its current purpose is to boot from a disk built entirely from source
and run useful native mainframe applications through a defined subset of
MVS-style interfaces. cREXX supplies the first substantial workload.

This guide describes the active `pdos-zarch` build. The source retains other
historical configurations, but their presence does not give them the same
support or qualification. See [profiles](../../../doc/PROFILES.md) for those
boundaries and the [exact guest record](../qualification/QUALIFICATION.md)
for what has run.

The two-address-space successor, its K64/C31/shared-U proofs, checked guest-DAT
IPL, bounded interruption/service gate, live storage, K-owned channel and
terminal operations, dual CMS image maps and wide heaps are in
[the two-space PoC contract](TWO-SPACE-POC.md).
The current released disk-boot kernel described below has not yet adopted
that layout.

The successor now has a coherent but bounded memory path. K64 owns the ASCE
switch and saves full-width context; a K-only C31 endpoint walks U's tables
through a K alias. U allocation chooses an interval in one shared 64-bit map,
backs it with real frames outside the core, zeroes them and changes the live
U DAT with a single-CPU purge. It can return a low U address backed by high
real storage. The selected conditional SVC 120 GETMAIN/FREEMAIN subset and
an internal high-U diagnostic entry allocate, touch and release simultaneous
64 MiB U31 and 128 MiB U64 heaps while CMS24/CMS31 RXVM images are mapped.
The [256 MiB profile result](../qualification/TWO-SPACE-256M-HEAPS-2026-10-06.md)
measures 909,312 U table bytes in that guest. Reversible
fixed-origin child backing is also exercised in the guest.

K reads checked CKD records through separate low-real disk and console
workspaces, writes and reads the 3270, and validates and maps the pinned
CMS24/CMS31 RXVM MODULEs. None of this places K storage in U's 24-bit
virtual range. Both unchanged RXVM `-v` programs run in this diagnostic
IPL. A [selected CMS31 file result](../qualification/TWO-SPACE-CMS31-IOQUAL-2026-10-06.md)
also runs IOQUAL with LIBRARY through checked K file records and transient
output. The successor still lacks persistent output, general CMS/TSO file
and command adapters, REXX `ADDRESS` calls and the explicitly selected
replacement image. A post-qualification review will decide whether CMS
and TSO continue to share one U ASCE.

## The system at a glance

```text
Hercules: CPU, memory, channel subsystem, 3390 disk and 3270 terminal
  │ IPL from the disk
  ▼
PLOAD.SYS       Find, reconstruct and enter PDOS.SYS
  │
  ▼
PDOS.SYS       Initialize memory, devices and application context
  │
  ├── COMMAND.EXE (PCOMM) ── command / ATTACH ──► native application
  │                                                 │
  ◄────────────────── return / service request ──────┘
  │
  └── kernel service handlers ── channel I/O ──► disk or console
```

Hercules emulates the machine. z/PDOS supplies the guest operating system:
it issues channel operations, reads the disk's structures and handles
application requests. Guest file operations are not direct calls to the
host's filesystem API. A host file holds the emulated disk, but the guest sees
a 3390 device with its own records and volume metadata.

There are three linked programs in the base image:

| Disk dataset | Responsibility | Runtime |
| --- | --- | --- |
| `PLOAD.SYS` | Initial boot and kernel loading. Installed as a flat image with explicit IPL records. | PDPCLIB standalone startup/support, plus `PLOADSUP`. |
| `PDOS.SYS` | Kernel: memory, application execution, loading, disk, terminal and service handling. | PDPCLIB standalone startup/support, plus `PDOSSUP`. |
| `COMMAND.EXE` | PCOMM command processor. Runs as an application and requests kernel services. | PDPCLIB MVS-style startup and the selected `MVSSUPA` service layer. |

`CONFIG.SYS` selects the console. The release image contains these four
datasets; cREXX applications are external consumers.

## Boot: from IPL to the prompt

**IPL** is the machine's initial program load. The image recipe uses Hercules
disk utilities to create a new volume, then
[`install-ipl.c`](../../src/install-ipl.c) writes and checks the target IPL
records and channel command words. It encodes target bytes explicitly rather
than dumping host C structures onto disk.

PLOAD begins at address zero. Its assembler support establishes a stack;
[`pload.c`](../../src/pload.c) obtains the IPL device, locates `PDOS.SYS` in
the volume table of contents (VTOC), and reads its records into storage at
2 MiB. The shared load-module reader reconstructs the kernel and applies
relocations for that address. PLOAD calls the resolved entry, passing a
standalone-runtime parameter block with a heap starting at 3 MiB.

The kernel then initializes the console from `CONFIG.SYS`, builds its memory
translation tables and service control blocks, and enters PCOMM from
`COMMAND.EXE`. PCOMM is a 31-bit application placed above 16 MiB. It may probe
for `AUTOEXEC.BAT`; an absent startup file is normal in the base image.

The loader and kernel are linked **AMODE31/RMODE24**: they execute with 31-bit
addressing but reside below 16 MiB. PCOMM is **AMODE31/RMODE ANY**, allowing its
code above 16 MiB and below 2 GiB. Residence and execution mode are separate
properties.

## Execution: a C32 kernel with 64-bit application support

Classic C builds the kernel with 32-bit pointers. A 64-bit application therefore
cannot enter arbitrary C kernel code with full-width pointers and expect a
compatible call. The boundary is implemented explicitly in
[`pdossup.asm`](../../src/pdossup.asm) and the service dispatcher in
[`pdos.c`](../../src/pdos.c).

The main structures are:

| Structure | What it holds |
| --- | --- |
| `PDOS` | Global kernel state, the current address-space selection and current application context. |
| `ONESPACE` | Translation tables, below/above-line storage managers, request-block chain and task control block. The active z/Architecture build has one address space. |
| `RB` | A request block for an executing program: saved registers and PSW, parent linkage, entry/return state and owned load image. The z/Architecture fields also hold full 64-bit register and PSW state. |

`pdosRun` repeatedly dispatches the current context and processes the event
that returns control. `pdosDispatchUntilInterrupt` prepares the low-memory
handoff area. The assembler entry `ADISP` resumes a 31-bit context; `ADISP64`
restores full-width registers and a 16-byte PSW for a 64-bit context. SVC and
program-interruption handlers save the application state and return to C.
The C/assembler boundary preserves register high halves when a service updates
only a 32-bit result.

An **SVC**, or supervisor call, is how an application asks the OS for a service.
The current `ATTACH` path is synchronous: it creates a child context and leaves
the parent suspended until that child completes. SVC 3 performs return and
cleanup, reports the result and resumes the parent. `WAIT` is currently a
no-op; `DETACH` provides limited storage cleanup. These names do not imply a
complete MVS task scheduler.

The accepted machine has one CPU. Applications share the current mappings;
the native application PSW uses problem state with storage key zero. This is
a development system without a general process-isolation or multi-user
security model. Some child program interruptions are converted into a failing
return and cleanup; that is not a promise that every faulty application can
be recovered safely.

## Memory: what is mapped and what is allocated

The layout is deliberately fixed. Dynamic address translation (DAT) maps
virtual addresses to real storage; the storage managers then allocate blocks
within the supplied ranges. There is no demand pager or general virtual-memory
object manager behind these calls.

| Region in the active disk-boot configuration | Current placement |
| --- | --- |
| PLOAD, boot stack and boot heap | PLOAD starts at 0; the z/Architecture standalone stack starts in the reserved 15–16 MiB system range; PLOAD heap starts at 1 MiB. Other standalone profiles retain their historical 0.5 MiB stack. |
| Kernel image and standalone heap | Kernel starts at 2 MiB; its heap starts at 3 MiB and shares the 3–12 MiB range with its large DAT tables. |
| Below-line application allocation | 12 MiB to 15 MiB, managed by `btlmem` in the active z/Architecture build. CMS24 MODULE images use their checked fixed origin at `0x20000` and must end by 2 MiB. “The line” is the 16 MiB boundary. |
| Above-line application allocation | 16 MiB to 512 MiB, managed by `atlmem`. PCOMM and direct native loads use low-address storage in this area. |
| High application heap | Virtual `0x100000000` (4 GiB), backed by 128 MiB of real storage starting at `0x30000000` (768 MiB). |
| High code/stack windows | Two 16 MiB slots starting at virtual `0x110000000` (4 GiB + 256 MiB), backed by 32 MiB starting at real `0x38000000` (896 MiB). |

The qualified Hercules configuration supplies **4096 MiB of real storage**.
The linked kernel is 0x2f628 bytes in its one MiB slot at 2–3 MiB. Its
compiled `PDOS` structure is 8,802,304 bytes, mostly eager DAT tables. The
initial allocation requests another 4,096 bytes for alignment, leaving at
most 630,784 bytes before the 12 MiB application pool, even before allocator
overhead and later kernel allocations. The boot check rejects layouts where
that initial allocation crosses the pool; the standalone heap itself has no
general upper bound. The reserved 15–16 MiB stack is a bounded CMS24
compatibility bridge. A CMS24 fixed-origin image also covers PLOAD's 1 MiB
boot heap. After the kernel has run, z/Architecture shutdown loads its own
`0444` wait PSW instead of returning through PLOAD's C exit path. This avoids
using the overwritten boot heap during shutdown; it does not give that heap
separate storage. Moving the kernel body above the line, while retaining
below-line entry and channel-I/O resources, belongs to
[PD-003](../BACKLOG.md#pd-003-two-space-supervisor-and-shared-application-memory).
High virtual addresses do not require physical storage at the same address:
the tables map those windows to the real ranges above. The selected code
initially maps the low 4 GiB directly and aliases it into the next 4 GiB,
then replaces the high heap and code-window mappings. These aliases are
another reason not to infer isolation from the existence of DAT.

The assembler `P64PC` entry implements a bounded IARV64-style interface for
high storage. It accepts the selected version-zero GETSTOR requests for 32 or
128 segments, tracks one allocation, and returns the fixed 4 GiB address.
DETACH validates and releases that allocation state. This is the interface
needed by the selected application route, not general IARV64 support.

Changing this layout affects more than a C constant. The DAT tables, allocator
limits, assembler service constants, image sizes and application's storage
requests must agree. A source-review concern in the unused region-table
initialization is recorded as [PD-011](../BACKLOG.md#pd-011-region-first-table-padding).

## Loading a program

The program loader consumes the native load format produced by the supported
linker/exporter route. `fixPE`, `fixPEMode` and `fixPEHigh` in
[`pdosutil.c`](../../src/pdosutil.c) reconstruct sections and apply relocation
records with explicit capacity limits. The inherited `PE` name here refers
to the mainframe load representation; it does not mean Windows Portable
Executable support.

**Direct load.** `pdosLoadExe` looks for the executable dataset, reads its
record stream and reconstructs an image in above-line storage. The current
input and reconstruction buffers are bounded at 8 MiB. The native directory supplies
AMODE and RMODE. This path accepts the supported 31/64-bit, RMODE ANY route;
it rejects AMODE24 or low-residence requirements before dispatch. Retained
flat `.COM` handling is a separate legacy path, outside the native-module
qualification described here.

**High load.** An AMODE64/RMODE ANY launcher requests a high body through the
selected SVC 8 `LOAD` path. The caller temporarily enters AMODE31 for this
parameter-list interface; the handler checks that mode and the low-address
module-name pointer. `pdos64HighService` reads an executable within its
recorded first extent, reconstructs it with `fixPEHigh`, and places it in one
of the 16 MiB high slots. Its input and reconstruction buffer is capped at
5 MiB; the larger slot does not lift that limit. There is one active
high load at a time; the slots alternate between loads. The service returns
a full-width entry address. SVC 9 `DELETE`, or owning-task cleanup, releases
the high body and clears its loaded bytes. This supports the recorded cREXX
HIGH launcher/body arrangement, not arbitrary dynamic linking.

An **XMIT** file is a transport envelope for native mainframe content. It is
not the executable format dispatched by the CPU. The cREXX qualification
removed that envelope while preserving the native load bytes; it did not
recompile or patch the application for z/PDOS. Likewise, ELF source and
objects need their SDK's export/entry route before this loader can use them.

## Services supplied to applications

The kernel implements a selected compatibility surface, not every form of
the IBM services sharing these names. The principal cases in
`pdosProcessSVC` are:

| Area | Selected implementation and boundary |
| --- | --- |
| Execution | SVC 42 `ATTACH` and SVC 3 completion form a synchronous parent/child chain. |
| Storage below 2 GiB | SVC 10/120 GETMAIN/FREEMAIN forms use the two storage managers. Invalid-request behavior remains a qualification item. High storage uses `P64PC` separately. |
| High program loading | SVC 8/9 LOAD/DELETE serve the bounded launcher/body route above. |
| Terminal | SVC 93 TGET/TPUT and SVC 35 WTO. Native terminal buffers stay below 2 GiB; selected reads are limited to 256 bytes and writes to 132. |
| DD binding | SVC 99 binds selected DD names to existing disk datasets through a 32-entry TIOT. It is not a general dataset-creation or allocation service. |
| Dataset metadata | Selected SVC 18 BLDL/FIND, SVC 27 OBTAIN, SVC 24 DEVTYPE and SVC 64 RDJFCB forms supply the metadata needed by the runtime. |
| Dataset access | SVC 22 OPEN and SVC 20 CLOSE establish and finish supported DCB operations. Native read/write callbacks use project-specific SVC 251/250. |

A **DD name** is a program's logical name for a dataset; the **TIOT** holds
those bindings. A **DCB** is the runtime/OS data-control block for an open
file. A **DSCB** describes a dataset on disk. The kernel supplies a small
set of these structures, along with a synthetic device control block for
`PDOS00`, so the selected native runtime can use familiar interfaces.

Some inherited callbacks are placeholders: `CHECK` is a no-op in the
synchronous route, and NOTE/POINT/TRKCALC must not be read as evidence of full
positioning support. Unhandled SVCs in the selected native RMODE ANY context
print a diagnostic and return 12 in registers 15 and 0. Other retained
contexts do not necessarily have that same fallback behavior.

## Disk records, datasets and character encoding

The base disk is a 100-cylinder 3390. Its count-key-data (CKD) records and VTOC
are interpreted inside z/PDOS. `rdblock` and `wrblock` in the assembler support
perform channel I/O; the C kernel handles dataset lookup, sequential access,
selected partitioned-dataset member operations and directory updates.

A partitioned dataset (PDS) has a directory pointing to members by track and
record. The selected PDS writer retains pending state and commits the directory
entry after writing the member and its end-of-file record. The recorded repair
checks complete data and directory results, including full-track handling;
a successful OPEN alone is not sufficient evidence. The implementation remains
bounded by its selected extent and directory handling. It is not a complete
MVS access-method implementation.

Text conversion belongs at the appropriate runtime boundary. The Classic
compiler's selected execution character set is EBCDIC CP037, with hex `15`
used for newline; the supplied Hercules console configuration uses IBM1047.
Binary files must preserve bytes. cREXX's UTF-8 handling is application/runtime
behavior, not a claim that every z/PDOS dataset is UTF-8 text.

## Where to work next

The [development guide](../development/README.md) maps changes to source and
interfaces. [Build dependencies](DEPENDENCIES.md) trace the source-to-image
route. The [single backlog](../BACKLOG.md) owns known defects and next steps,
including AMODE24 loading, storage errors and operator improvements.

The longer-term direction includes a native 64-bit C kernel. That requires
explicit decisions about the compiler ABI, pointer-bearing control blocks,
bootstrap, low-address service interfaces and memory ownership. Existing
64-bit application execution provides useful experience; it does not complete
that conversion.
