# z/PDOS

z/PDOS is a small mainframe operating system descended from Paul Edwards's
PDOS. It boots from a freshly built 3390 disk and supplies the selected
MVS-style services needed to run native applications. The base system consists
of PLOAD, the kernel and PCOMM command processor.

Version 0.2.0 uses **one protected kernel address space (K) and one shared
application address space (U)**. An AMODE64 assembler nucleus handles
interruptions and calls Classic C31 services in K. PCOMM runs above 16 MiB in
U; selected unchanged AMODE24/31/64 CMS and TSO applications use that same U
translation. K owns native loading, invocation resources, files, terminal
leases and interruption-driven I/O completion.

The [completed P0–P6 record](doc/qualification/TWO-SPACE-P6-2026-10-07.md)
qualifies fresh CMS31 and TSO31/64 ANY/HIGH compiler chains, interactive I/O,
selected native calls, models 2–5, line/monitor consoles, exact disk/tape
workflows and cleanup. Native TSO24 and CMS24 have separate, library-free
RXVM IO24 results. General CMS/TSO services, full-library TSO24 and CMS24
compiler/assembler execution remain outside the accepted scope.

The source factory and 0.2.0 release recipe select K/U. The
[release record](doc/qualification/RELEASE-0.2.0.md) distinguishes review,
build, exact operator acceptance and publication. The
[architecture contract](doc/architecture/TWO-SPACE-POC.md) and
[ABI inventory](doc/architecture/TWO-SPACE-ABI.md) own the design and native
compatibility details. The earlier 0.1.1 C32/AMODE31 one-space image remains
available through the explicit `one-space-image.crexx` recipe and its dated
qualification reports.

The develop source also contains the [0.2.1 console checkpoint](doc/qualification/CONSOLE-STAGE3-2026-10-09.md):
retained Workbench panels/history, owned monitor recovery, tested text modes,
basic 3287 printing and the optional DISKMAP C application. Its 21 guest runs
and 333 checks qualify the recorded subset. Full 3270 conformance and closure
of every facility testable with installed providers remain open; this source
checkpoint does not select a 0.2.1 release.

- [Boot the image or build your own](doc/user/README.md)
- [Architecture: boot, execution, memory, loading and services](doc/architecture/README.md)
- [Development and source map](doc/development/README.md)
- [K/U guest qualification](doc/qualification/TWO-SPACE-P6-2026-10-07.md)
- [0.2.0 build and operator acceptance](doc/qualification/RELEASE-0.2.0.md)
- [Historical 0.1 guest qualification](doc/qualification/QUALIFICATION.md)
- [Stage 3 CMS guest qualification](doc/qualification/STAGE3-2026-10-05.md)
- [0.1.1 local operator acceptance](doc/qualification/0.1.1-OPERATOR-UAT-2026-10-05.md)
- [Known issues and next steps](doc/BACKLOG.md)
- [Origins and contributors](UPSTREAM.md), [licence](LICENSE), [agent rules](AGENTS.md)

The maintained code is in `src/`. The shared C library and native runtime are
owned by [PDPCLIB](../pdpclib/README.md); this component does not keep another
runtime copy. Build recipes select `pdos-zarch` explicitly and use the
maintained Classic tools. Optional frozen `archive/` material is reference
only and is excluded from normal builds.
