# Mainframe Classic Linker backlog

This is the only live roadmap, defect and qualification queue for this component.
Follow [the shared workflow](../../doc/WORKFLOW.md). Existing findings are
recorded here; their repair is outside the 2 October 2026 reorganisation.
Open items require separate implementation authority.

## LD-001: Inherited backends beyond the reached Classic route

- Type: qualification
- Status: Open
- Target: Reached Classic formats; other inherited backends separately
- Observation: Importing PDLD backends does not qualify them here.
- Evidence: doc/user/USER.md and doc/qualification/QA.md.
- Acceptance: Name each format/target and prove its actual object, relocation, packaging and downstream execution contract.

## LD-002: Native guest tool hosting

- Type: qualification
- Status: Open
- Target: Reached Classic formats; other inherited backends separately
- Observation: The maintained linker is built and checked on the host.
- Evidence: Host-only component QA records.
- Acceptance: Build and run the same maintained implementation on the named guest with exact inputs and independently checked output.
