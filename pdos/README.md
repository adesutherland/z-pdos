# z/PDOS

z/PDOS is a small mainframe operating system descended from Paul Edwards's
PDOS. It boots from a freshly built 3390 disk and supplies the selected
MVS-style services needed to run native applications. The base system consists
of PLOAD, the kernel and PCOMM command processor.

The source-built successor uses **one protected kernel address space (K)
and one shared application address space (U)**. An AMODE64 assembler nucleus
handles interruptions and calls Classic C31 services in K. U contains PCOMM
and selected unchanged AMODE24/31/64 CMS and TSO applications. Native loading,
application calls, files, terminal ownership and cleanup belong to K.

P0–P6 are accepted; the source factory selects this K/U kernel by default. The [P6 qualification record](doc/qualification/TWO-SPACE-P6-2026-10-07.md)
records the frozen source build, normal workload and media results, final
reviewed checks and source-default selection. The [completion plan](doc/BACKLOG.md#pd-003-completion-plan-6-october-2026)
owns checkpoint status; the [architecture contract](doc/architecture/TWO-SPACE-POC.md)
and [ABI inventory](doc/architecture/TWO-SPACE-ABI.md) own design and native
compatibility scope.

The published 0.1.1 image retains its C32, AMODE31, one-address-space kernel.
It remains available through the explicit one-space producer. Selecting a
source default does not publish a release or replace the managed guest.

The recorded source-built milestone runs unchanged cREXX TSO31, TSO64 ANY and
TSO64 HIGH packages, including compiler, assembler, terminal and file workloads.
The later [stage 3 source result](doc/qualification/STAGE3-2026-10-05.md) runs
unchanged CMS31 RXVM/RXAS/RXC MODULEs and the separate fixed-origin CMS24 RXVM
IO24 subset. The [0.1.1 local operator acceptance](doc/qualification/0.1.1-OPERATOR-UAT-2026-10-05.md)
covers the repaired shutdown path, CMS and TSO cREXX workloads, CKD/tape media
and managed-image lifecycle. On that published route, native TSO24 loading and broader service
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
