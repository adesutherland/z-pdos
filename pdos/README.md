# z/PDOS

z/PDOS is a small mainframe operating system descended from Paul Edwards's
PDOS. It boots from a freshly built 3390 disk and supplies the selected
MVS-style services needed to run native applications. The base system consists
of PLOAD, the kernel and PCOMM command processor.

The current kernel is written in **32-bit C and runs in AMODE31**. Assembler
support lets it preserve and dispatch 64-bit application state, with fixed
high-memory windows for the qualified workloads. It has one address space and
synchronous application execution. A native 64-bit C kernel is a separate goal.

The recorded source-built milestone runs unchanged cREXX TSO31, TSO64 ANY and
TSO64 HIGH packages, including compiler, assembler, terminal and file workloads.
The later [stage 3 source result](doc/qualification/STAGE3-2026-10-05.md) runs
unchanged CMS31 RXVM/RXAS/RXC MODULEs and the separate fixed-origin CMS24 RXVM
IO24 subset. Native TSO24 loading and broader service compatibility remain
open. The downloadable 0.1.0 image has its own host-build evidence, distinct
from these guest runs.

- [Boot the image or build your own](doc/user/README.md)
- [Architecture: boot, execution, memory, loading and services](doc/architecture/README.md)
- [Development and source map](doc/development/README.md)
- [Exact guest qualification](doc/qualification/QUALIFICATION.md)
- [Stage 3 CMS guest qualification](doc/qualification/STAGE3-2026-10-05.md)
- [Known issues and next steps](doc/BACKLOG.md)
- [Origins and contributors](UPSTREAM.md), [licence](LICENSE), [agent rules](AGENTS.md)

The maintained code is in `src/`. The shared C library and native runtime are
owned by [PDPCLIB](../pdpclib/README.md); this component does not keep another
runtime copy. Build recipes select `pdos-zarch` explicitly and use the
maintained Classic tools. Optional frozen `archive/` material is reference
only and is excluded from normal builds.
