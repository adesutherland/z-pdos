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
