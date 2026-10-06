# z/PDOS

z/PDOS is a small mainframe operating system descended from Paul Edwards's
PDOS. It boots from a freshly built 3390 disk and supplies the selected
MVS-style services needed to run native applications. The base system consists
of PLOAD, the kernel and PCOMM command processor.

The current kernel is written in **32-bit C and runs in AMODE31**. Assembler
support lets it preserve and dispatch 64-bit application state, with fixed
high-memory windows for the qualified workloads. It has one address space and
synchronous application execution. A native 64-bit C kernel is a separate goal.

The [two-space successor proof](doc/architecture/TWO-SPACE-POC.md) exercises
an AMODE64 assembler nucleus, protected Classic C31 services and applications
sharing a separate 64-bit address space. The
[slices 3/4 qualification](doc/qualification/TWO-SPACE-SLICES3-4-2026-10-05.md)
boots its bounded interruption and U-buffer gate through a fresh 3390 IPL;
the [storage and overlay checkpoint](doc/qualification/TWO-SPACE-STORAGE-OVERLAY-2026-10-05.md)
adds live K-owned U allocation and a reversible fixed-origin backing swap.
The [K channel checkpoint](doc/qualification/TWO-SPACE-CHANNEL-2026-10-06.md)
adds a bounded post-handover 3390 record read through K-owned low-real
buffers.
The release kernel has not adopted this successor. It does not yet run
unchanged CMS/TSO binaries or provide their file, terminal and command APIs.

The recorded source-built milestone runs unchanged cREXX TSO31, TSO64 ANY and
TSO64 HIGH packages, including compiler, assembler, terminal and file workloads.
The later [stage 3 source result](doc/qualification/STAGE3-2026-10-05.md) runs
unchanged CMS31 RXVM/RXAS/RXC MODULEs and the separate fixed-origin CMS24 RXVM
IO24 subset. The [0.1.1 local operator acceptance](doc/qualification/0.1.1-OPERATOR-UAT-2026-10-05.md)
covers the repaired shutdown path, CMS and TSO cREXX workloads, CKD/tape media
and managed-image lifecycle. Native TSO24 loading and broader service
compatibility remain open. The downloadable 0.1.1 base image has its own host
build and disk checks; it was not the installed image used for operator acceptance.

- [Boot the image or build your own](doc/user/README.md)
- [Architecture: boot, execution, memory, loading and services](doc/architecture/README.md)
- [Development and source map](doc/development/README.md)
- [Exact guest qualification](doc/qualification/QUALIFICATION.md)
- [Stage 3 CMS guest qualification](doc/qualification/STAGE3-2026-10-05.md)
- [0.1.1 local operator acceptance](doc/qualification/0.1.1-OPERATOR-UAT-2026-10-05.md)
- [Known issues and next steps](doc/BACKLOG.md)
- [Origins and contributors](UPSTREAM.md), [licence](LICENSE), [agent rules](AGENTS.md)

The maintained code is in `src/`. The shared C library and native runtime are
owned by [PDPCLIB](../pdpclib/README.md); this component does not keep another
runtime copy. Build recipes select `pdos-zarch` explicitly and use the
maintained Classic tools. Optional frozen `archive/` material is reference
only and is excluded from normal builds.
