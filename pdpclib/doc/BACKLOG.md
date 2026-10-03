# PDPCLIB backlog

This is the only live roadmap, defect and qualification queue for this component.
Follow [the shared workflow](../../doc/WORKFLOW.md). Existing findings are
recorded here; their repair is outside the 2 October 2026 reorganisation.
Open items require separate implementation authority.

## PCL-001: Partial MVS binary update support

- Type: defect
- Status: Open
- Target: Named OS/application service profiles and retained ports
- Observation: Upstream r+b is an attempted update route; w+b is a write-only fallback. Full update/seek/reopen semantics remain unqualified. PDOS390 rejects update modes before native open.
- Evidence: Existing host stdio checks and the earlier KNOWN-ISSUES.md in Git history.
- Acceptance: Define the supported modes, qualify real guest behaviour and report unsupported operations truthfully.

## PCL-002: Other whole-library profiles

- Type: qualification
- Status: In progress
- Target: Named OS/application service profiles and retained ports
- Observation: The complete `pdos-zarch` MVSSUPA source assembles with maintained PDOS31 service definitions. The new `tso31-sdk-files` selection assembles a TSO service object with sequential/partitioned dataset paths; VSAM, IDCAMS and supervisor-mode switching return unsupported status. The broader `tso31-lean` selection still reaches a VSAM `SHOWCB` barrier. No new z/OS service behavior is guest qualified. Selected PDOS31 macro semantics cannot be reused as proof of MVS/TSO semantics.
- Evidence: On Apple Silicon macOS, 3 October 2026, the SDK file selection assembled 4331 statements, 25 sections, 971 symbols and 55 fixups with the source-built Classic Assembler. Its object linked with four TSO24/31/64 entry variants and source-built C decks to XMIT transports. The independent TSO fixture checks PUTLINE, GETLINE and EXTRACT service bytes. The local host suite passed 93/93 with `-j 4`. Earlier z/PDOS guest scope remains in doc/qualification/CHECKPOINT.md and ../pdos/doc/qualification/QUALIFICATION.md.
- Acceptance: Run each named compile, assemble, link and guest path with exact service/profile inputs.

## PCL-003: Retained ports and file-level notices

- Type: qualification
- Status: Open
- Target: Named OS/application service profiles and retained ports
- Observation: The full inherited implementation includes ports outside the qualified mainframe route. A later canonical VSE file carried a copyright question and was not imported by the runtime merge.
- Evidence: LICENSE and UPSTREAM.md.
- Acceptance: Review a port and its actual file notices before replacing its implementation or claiming support.

## PCL-004: FILE update-state initialization

- Type: defect
- Status: Done before this reorganisation
- Result: The canonical initialization fix and the w+b write-handle guard were already integrated. Existing host fixtures retain failing guard-removal and stale-allocation controls. Full guest update semantics remain PCL-001.

## PCL-005: Mainframe host-fixture profile isolation

- Type: test portability
- Status: Done
- Target: Linux and Windows host execution of the PDOS390/MVS native-stub fixtures
- Observation: Linux's predefined `__gnu_linux__` selected the inherited Linux port alongside the explicit mainframe profile, duplicating the file handle and local mode. The fixture targets now undefine the inherited Linux and Windows port selectors. Maintained target library source is unchanged.
- Evidence: [Hosted Linux failure](https://github.com/adesutherland/z-pdos/actions/runs/37060007541), `../CMakeLists.txt`; all 92 required toolchain tests pass locally with GCC 16.1 and the Linux host selector explicitly defined.
- Acceptance: Both stdio/string profiles run on the declared desktop hosts with their native service stubs and full suite enabled.

- Result: The [hosted Windows run](https://github.com/adesutherland/z-pdos/actions/runs/37070532323) built and tested MVS/CMS and passed all 92 required toolchain tests at source `8642c3543e3cce3d5a801bf300a9e2792e7574c2`. Packaging stopped later at the separate NSIS literal-dollar filename guard; it does not invalidate these component checks.

## PCL-006: Windows source-macro fixture path ownership

- Type: test portability
- Status: Done
- Target: Native Windows execution of the maintained source-macro fixture
- Observation: CMake supplied a forward-slash absolute build path while native cREXX reported its working directory with backslashes. The fixture rejected the selected owned directory before preparing or assembling source. Both paths now normalize separators before the existing repository/build ownership check; parent traversal and unowned outputs remain rejected.
- Evidence: [Windows suite](https://github.com/adesutherland/z-pdos/actions/runs/37069466661) passed 91 of 92 tests and both compiler variants. A local backslash-path probe reproduces the old rejection and passes after normalization, including the existing independent macro-deck check.
- Acceptance: The source-macro fixture and complete hosted Windows suite pass with the ownership restriction intact.

- Result: The [hosted Windows run](https://github.com/adesutherland/z-pdos/actions/runs/37070532323) built and tested MVS/CMS and passed all 92 required toolchain tests at source `8642c3543e3cce3d5a801bf300a9e2792e7574c2`. Packaging stopped later at the separate NSIS literal-dollar filename guard; it does not invalidate these component checks.

## PCL-007: Qualify selected TSO service interfaces on z/OS 1.5

- Type: qualification
- Status: In progress
- Target: `tso31-sdk-files` service object and its TSO24/31/64 consumers
- Observation: Basic TSO24/31/64 entry transports and TSO31/64 sequential/PDS file operations ran on z/OS 1.5. TSO24 dataset I/O faults in above-line SWA lookup. The selected service skips inherited NOTE/TRKCALC positioning, so FBS extend remains outside the file subset. Dynamic allocation, command and prefix routes and explicit VSAM failure still need guest checks. The service still consumes selected PDOS31 mappings for other MVS fields, and inherited code reads `TCBFA`, which IBM does not designate as a programming interface.
- Evidence: `doc/architecture/TSO31-INTERFACES.md`, the 3 October host source build and 93/93 host suite.
- Acceptance: Exercise sequential and partitioned dataset open/read/write/close, terminal I/O, dynamic allocation, command and prefix routes, explicit VSAM failure, and the two TSO64 entry modes on the leased z/OS 1.5 guest. Resolve offset or linkage mismatches in maintained source and repeat affected checks.
