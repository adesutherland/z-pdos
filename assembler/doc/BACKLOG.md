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
- Observation: GCC's warning-as-error build rejects the single-line conditional assignment and unconditional return in `product()`, and the allocation macro's conditional return followed by an unconditional field assignment. The statements are now separated, with explicit braces in the macro, without changing their control flow.
- Evidence: Linux job in [the first hosted release build](https://github.com/adesutherland/z-pdos/actions/runs/37057263103); the subsequent GCC build also identified three test-fixture lines containing adjacent conditional statements, now separated. All 92 local toolchain checks pass after the formatting fixes.
- Acceptance: The GCC host build passes with the existing strict warnings and the unchanged assembler fixtures.

## AS-005: Mainframe ELF SDK native entry source

- Type: qualification
- Status: In progress
- Target: z/Architecture classic object decks consumed by the Mainframe ELF SDK TSO entry path
- Observation: The SDK `entry31-any.asm` initially stopped at `EPSW`; after adding its RRE encoding, it reached `GETMAIN`. The SDK TSO24 and TSO31 entries now use selected explicit public SVC register interfaces and assemble successfully. The TSO64 entry and larger PDPCLIB service source still need coverage. A successful assembler host build does not establish complete native link or guest compatibility.
- Evidence: IBM *z/Architecture Principles of Operation* SA22-7832-14 defines EPSW as RRE opcode B98D; the local encoder vector expects `B98D006F`. The 3 October 2026 Classic Assembler suite passes 37/37. SDK `build/tso24-source.obj` and `build/tso31-any-source.obj` pass independent object mode, closure and service-instruction checks, with a changed opcode rejected. The prepared `tso31-lean` PDPCLIB source (SHA-256 `da54a119cf2b94e6e4b21a6619e795638e2122c476885286ceeee5a93451a49a`) stops at `FUNHEAD` on line 679 with unsupported feature status 3.
- Acceptance: Supply independently authored service definitions from selected primary interface editions, assemble the complete SDK native entries and service support from maintained source, compare independently expected object properties, link the SDK consumer, and repeat affected guest checks.
