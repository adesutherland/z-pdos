# How z/PDOS 0.2 works

z/PDOS boots from a source-built 3390 disk and runs selected native CMS and
TSO applications. Version 0.2 uses two translations on one z/Architecture
CPU: a protected supervisor address space **K**, and one shared application
address space **U**. The [P0–P6 record](../qualification/TWO-SPACE-P6-2026-10-07.md)
identifies the accepted workloads. The [release record](../qualification/RELEASE-0.2.0.md)
identifies the 0.2.0 candidate and operator acceptance separately.

## Boot and execution

```text
Hercules: one z/Architecture CPU, 256 MiB, 3390 and terminal devices
  │ IPL 01B9
  ▼
PLOAD.SYS → PDOS.SYS C31 handover → KCORE.BIN
                                    │ protected K
                                    ├─ AMODE64 interruption nucleus
                                    ├─ Classic C31 services and channel I/O
                                    │ checked dispatch and U-buffer transfers
                                    ▼ shared U
                              U.COMMAND (PCOMM)
                                    │ existing ATTACH/WAIT/DETACH route
                                    ▼
                         native CMS or TSO application
                                    │ service, return or recoverable fault
                                    └──────────────────────────────► K
```

The IPL records and channel commands are explicitly encoded by the source
image builder. `PLOAD.SYS` locates and reconstructs `PDOS.SYS`. That C31
handover loads the checked `KCORE.BIN`, establishes K/U translation and enters
the K64 nucleus. K starts the native U PCOMM stage from `U.COMMAND`.
`COMMAND.EXE` remains a bootstrap/legacy payload in the base disk;
`CONFIG.SYS` and the normal core configuration identify terminal setup.
`PDOS.STORE` supplies the preformatted durable output banks.

An **ASCE** selects the address-space translation tables; an **AMODE** selects
24-, 31- or 64-bit execution addressing. All applications use the same U
ASCE regardless of AMODE. On interruption the nucleus saves the full GPR,
FPR, access-register, floating-point-control and PSW state, selects K, and
calls checked C31 services. Dispatch restores the selected U context and
ASCE. High U addresses cross explicit two-word address interfaces; C31
pointers do not become 64-bit pointers.

PCOMM uses its existing PDPCLIB `system()` and ATTACH/completion behavior.
One foreground invocation chain executes at a time. K records each child's
personality, parent state, loaded image, allocations, open files and terminal
lease. Return or a recoverable U fault releases child resources and restores
the caller, including file cursors and personality lowcore. Pending I/O is
completed or cancelled through its owned operation before reclamation.
K can emit emergency output and shut down even if the U command processor
faults. This is a single-user development OS, without a general multitasking
scheduler or isolation between applications sharing U.

## Memory and native loading

Dynamic address translation maps sparse U virtual pages to separately owned
real frames. K administers those frames and its table aliases; U cannot map
K's body, stack or private tables. The normal profile uses 256 MiB real
storage. K's original 4 MiB bootstrap reservation contains its 96-page C31
code bank and protected 128 KiB C stack, plus low-real channel resources.
Disk I/O uses a separate workspace at real `0x3e0000`; terminal I/O uses
`0x3f0000`. These real reservations consume no low-U virtual interval.

Placement respects each native image's declared addressing and residence
mode. Fixed CMS24 images keep their checked origin; relocatable CMS31 and
TSO images use suitable U intervals. A live module is not silently displaced
to load another. Checked fixed-origin overlays restore a suspended caller's
backing; a collision that cannot be supported fails explicitly. Normal-image
loading has a 32-entry image registry and bounded native input/reconstruction
capacities. HIGH uses the unchanged low launcher, native LOAD/DELETE and a
separately placed AMODE64 body above 4 GiB. It is not arbitrary dynamic linking.

PCOMM's C arena is 16 MiB above the line, with low control blocks placed beyond
the declared fixed CMS24 image. The qualified cREXX runtime heaps are 64 MiB
for 31-bit profiles and 128 MiB for 64-bit profiles. Their backing fits the
selected sequential/nested workloads; these numbers do not promise that every
combination fits simultaneously. AMODE24 images, stacks, control blocks and
heap must fit below 16 MiB. Native TSO24's 4 MiB heap qualifies the library-free
IO24 route; full-library IOQUAL remains unqualified.

The shared load-module reader in `pdosutil.c` reconstructs native sections and
relocations. `twospace_tso.c`, the CMS reader and K placement/invocation code
preserve native entry and return conventions. An XMIT envelope is transport,
not the executed format. An ELF object needs its SDK export/native-entry route.
The [ABI inventory](TWO-SPACE-ABI.md) pins the unchanged package bytes and
reached service forms.

## Services, files and media

The selected TSO surface includes ATTACH/LINK and completion, low storage,
HIGH LOAD/DELETE and IARV64-style storage, terminal calls, DD binding,
dataset metadata, OPEN/CLOSE and native sequential/PDS access. CMS uses its
selected SVC 202/204/205 entry, terminal, storage and file forms. These names
identify the reached compatibility subset; they do not imply general IBM
service support. Unknown services in the normal image return unsupported
status 20. PCOMM reports a launch/service failure as OS status with
`RC=unavailable`, separately from an application's normal return code.

K validates complete U buffers and control blocks against the invocation and
address mode before transfer. The driver submits channel work, then publishes
data and completion after the matching I/O interruption. A watchdog detects
failure; elapsed time cannot manufacture a successful completion.

Native CMS/TSO application output and the ordered transcript commit to banked
`PDOS.STORE` on the IPL disk. A selected exchange volume does not redirect
those journaled outputs. Stopped-store adapters independently verify text,
binary bytes, record order and empty-file semantics. The store is bounded;
it is not a general filesystem or unlimited logging service.

The existing `DEVICES`, `VOLUMES`, `MOUNT`, `SELECT`, `UNMOUNT`, `DIR`,
`ALLOC`, `RCOPY` and `TAPE` algorithms live once in `media_commands.inc`.
The K port supplies private buffers and interruption-driven channel I/O.
A DD retains its actual device binding across volume selection. Qualified
native sequential COPY writes/readbacks its physical exchange target;
raw tape preserves physical records and file marks. See [MEDIA](../user/MEDIA.md)
for the allocator, extent, record and tape limits. Exact stopped readback,
not console success alone, establishes the output.

## Console and text

The [3270 family driver contract](CONSOLE-STANDARD.md) records the accepted
0.2.1 development scope and distinguishes its implementation checkpoints from
the qualified 0.2.0 behavior below.

A C31 K driver owns channel devices and encodes checked 3270 data streams.
The reusable C presentation library runs in U and provides PCOMM's header,
footer, scrollable output and editable prompt. Configured models 2–5, a
line-only primary and an optional Telnet 3215 monitor have accepted results.
Child entry suspends the caller's screen lease; return restores it and the
caller repaints. There is one input owner, with explicit line-prompt handoff.

The monitor captures ordered logical text while the 3270 remains interactive.
Raw screens and monitor disconnects produce explicit gaps; a passing loss or
recovery control is not complete capture. EXIT commits durable text, reports
`K SHUTDOWN` and enters disabled wait. Connect the configured primary and
monitor before IPL.

Native terminal/batch text uses IBM1047 and hex `15` line delimiters; Hercules
uses `CODEPAGE 819/1047`. Classic C's execution text, application UTF-8,
console conversion and binary dataset bytes are distinct. No generic text
conversion is applied to binary records.

## Implementation and qualification

The [development guide](../development/README.md) maps these roles to source;
[dependencies](DEPENDENCIES.md) describes the Classic/GNU source-to-disk route.
The [K/U contract](TWO-SPACE-POC.md) contains decisions and dated development
checkpoints. Dated 0.1/0.1.1 reports retain the previous one-ASCE layout; their
4096 MiB machine and eager tables are not the 0.2 memory contract.
The [backlog](../BACKLOG.md) owns remaining compatibility and qualification work.
The [shared I/O foundation](IO-FOUNDATION.md) describes the locally qualified
PD-025 operation lifetime, common U transfers and record grouping used by the
0.2.1 development candidate.
