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
- Status: Open
- Target: Named OS/application service profiles and retained ports
- Observation: The pdos-zarch selection has z/PDOS 0.1 acceptance; that does not qualify complete MVS 3.8, lean TSO or z/OS service paths.
- Evidence: doc/qualification/CHECKPOINT.md and ../pdos/doc/qualification/QUALIFICATION.md.
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
- Status: In progress
- Target: Linux and Windows host execution of the PDOS390/MVS native-stub fixtures
- Observation: Linux's predefined `__gnu_linux__` selected the inherited Linux port alongside the explicit mainframe profile, duplicating the file handle and local mode. The fixture targets now undefine the inherited Linux and Windows port selectors. Maintained target library source is unchanged.
- Evidence: [Hosted Linux failure](https://github.com/adesutherland/z-pdos/actions/runs/37060007541), `../CMakeLists.txt`; all 92 required toolchain tests pass locally with GCC 16.1 and the Linux host selector explicitly defined.
- Acceptance: Both stdio/string profiles run on the declared desktop hosts with their native service stubs and full suite enabled.
