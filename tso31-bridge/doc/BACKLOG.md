# TSO31 entry bridge backlog

This is the only live roadmap, defect and qualification queue for this component.
Follow [the shared workflow](../../doc/WORKFLOW.md). Existing findings are
recorded here; their repair is outside the 2 October 2026 reorganisation.
Open items require separate implementation authority.

## TSO-001: Complete native link and guest execution

- Type: qualification
- Status: Open
- Target: 31-bit TSO entry and its ELF-style C ABI
- Observation: Host assembly, target C layout and entry-only linking do not qualify the complete maintained runtime and guest path.
- Evidence: doc/qualification/CHECKPOINT.md and doc/architecture/ABI.md.
- Acceptance: Resolve the six native external identities, use the exact compatible C/ELF-to-classic producer and run an actual caller in the named TSO guest.

## TSO-002: Reentrancy and service expansion

- Type: improvement
- Status: Open
- Target: 31-bit TSO entry and its ELF-style C ABI
- Observation: The adapter is deliberately synchronous and non-reentrant, with selected service forms.
- Evidence: doc/architecture/ABI.md and SERVICES.md.
- Acceptance: Only after a separate requirement, design and qualify broader state ownership or service forms.
