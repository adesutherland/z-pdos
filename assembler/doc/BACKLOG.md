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

## AS-005: Mainframe ELF SDK native entry and PDPCLIB source

- Type: qualification
- Status: In progress
- Target: z/Architecture classic entry and PDPCLIB service decks used by the Mainframe ELF SDK and z/PDOS
- Observation: The SDK TSO24, TSO31 and both TSO64 entry variants assemble with selected source-owned service interfaces. A direct `FUNHEAD` isolation proves the assembler already supports that maintained macro's local path. The original whole-source stop at its invocation hid an expanded `GETMAIN` operation; the CLI now reports that expanded operation and its model location. The complete `pdos-zarch` MVSSUPA source assembles with source-owned PDOS31 and linkage macros. Selected TSO PUTLINE/GETLINE forms now assemble; the MVS `tso31-lean` selection reaches VSAM `SHOWCB`. A host object is not a functional native link or guest result.
- Evidence: IBM *z/Architecture Principles of Operation* SA22-7832-14 defines EPSW as RRE opcode B98D; the encoder vector expects `B98D006F`. On 3 October 2026 the full local suite passed 93/93. `pdpclib_source_macros` checks independently expected FUNHEAD and TSO terminal bytes and assembles the full maintained `pdos-zarch` source. Its source SHA-256 is `8327b212648224f25dc0d0268b8d32fffb69ca36cce5cd304c08f75852f3c3d0` and its 20-section/908-symbol/50-fixup object is `9a7a69a5501edd1dbba0b60dd371c16636ce577d2cb1a41f59fade04f8f3fb58`. The TSO31 recipe reaches `SHOWCB` at prepared line 1993. Both TSO64 entries assemble with the source-built assembler, and their SDK object checks pass; the AMODE64/RMODE ANY entry SHA-256 is `5a12f242e19b1f9a89ee0203e7f3e3d8f9f8a27253c4658954dd46e8114a603f`. An isolated host XMIT link resolved the entry's six externals against deliberately nonfunctional fixture exports, proving linker shape only.
- Acceptance: Supply independently authored service definitions from selected primary interface editions, assemble the complete SDK native entries and service support from maintained source, compare independently expected object properties, link the SDK consumer, and repeat affected guest checks.
