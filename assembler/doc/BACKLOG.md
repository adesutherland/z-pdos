# Mainframe Classic Assembler backlog

This is the only live roadmap, defect and qualification queue for this component.
Follow [the shared workflow](../../doc/WORKFLOW.md). Existing findings are
recorded here; their repair is outside the 2 October 2026 reorganisation.
Open items require separate implementation authority.

## AS-001: Native assembler hosting

- Type: qualification
- Status: Open
- Target: Classic mainframe source/object profiles; native hosts separately
- Observation: Host builds do not establish that the tool runs on z/PDOS or CMS.
- Evidence: Existing bootstrap and consumer checkpoints.
- Acceptance: Cross-build the maintained portable source, run real guest assembly, measure storage and independently compare output.

## AS-002: Shared named machine selectors

- Type: improvement
- Status: Open
- Target: Classic mainframe source/object profiles; native hosts separately
- Observation: The implemented s360/s370/z900 ceilings are distinct from the shared named workload profiles.
- Evidence: machines/doc/MACHINE-PROFILES.md.
- Acceptance: Use reviewed compiler/assembler selectors, post-expansion instruction checks and independently audited final objects.

## AS-003: Additional source-language and library coverage

- Type: qualification
- Status: Open
- Target: Classic mainframe source/object profiles; native hosts separately
- Observation: Selected PDOS sources pass; broader macro/conditional forms and complete historical library profiles remain separately qualified work.
- Evidence: Consumer inventory, macro contracts and PDPCLIB checkpoints.
- Acceptance: Choose an actual consumer, implement only authorised forms and retain independently expected bytes and failure cases.

The earlier bootstrap plan is preserved in Git history. Its approved contracts
remain in doc/architecture; selected z/PDOS source-language coverage has since
passed the recorded 0.1 source-to-image and guest route.

## AS-004: GCC host warning during release build

- Type: portability
- Status: In progress
- Target: Linux and Windows desktop release hosts
- Observation: GCC's warning-as-error build rejects the single-line conditional assignment and unconditional return in `product()`. The two statements are now on separate lines without changing their control flow.
- Evidence: Linux job in [the first hosted release build](https://github.com/adesutherland/z-pdos/actions/runs/37057263103); all 92 local toolchain checks pass after the formatting fix.
- Acceptance: The GCC host build passes with the existing strict warnings and the unchanged assembler fixtures.
