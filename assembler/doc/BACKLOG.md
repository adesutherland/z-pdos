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

## AS-006: Explicit desktop symbol capacity for the PD-024 kernel

- Type: improvement
- Status: Done
- Target: Desktop CLI; portable assembler API unchanged
- Observation: The retained console service exceeds the CLI's fixed 4096-symbol table. The supplied-storage core already accepts a configured maximum.
- Direction: Add `--symbol-limit 1..65536`, retain default 4096, and select 8192 explicitly for the large service. This is required build support for the authorized PD-024 increment.
- Acceptance: CLI rejects invalid limits, the default rejects a 5000-symbol source without publishing an object, selected capacity assembles it, existing CLI checks pass, and the PD-024 service assembles with the selected capacity.

- Evidence: `classic_cli_contracts` passes invalid bounds, unchanged default capacity and the independently generated 5000-symbol source with selected capacity. The reviewed PD-024 native24 service assembles 51262 statements and 4142 symbols using an explicit 8192 limit, then links and passes the bounded guest matrix; portable core/API unchanged.

## AS-007: Coordinated desktop assembler capacities

- Type: improvement / capacity
- Status: Done
- Target: Desktop CLI and maintained PDOS build interface
- Observation: The 32 MiB host budget and scattered 4096/8192 overrides risk another avoidable build failure as K grows. The supplied-storage core already streams replayable source and sequential object output.
- Direction: Add named bootstrap/desktop capacity profiles, checked storage/fixup overrides and measured storage diagnostics. Keep bootstrap defaults and classic object/source constraints explicit; select desktop in PDOS recipes.
- Acceptance: Invalid/overflowing options and insufficient storage leave output absent; profile overrides work in documented order; existing and desktop profiles produce identical objects for the same source; actual K/app compile, assemble and link succeed with recorded headroom.
- Evidence: CLI contracts pass desktop and macro object comparisons, bootstrap/default refusal, ordered overrides and storage exhaustion. The fresh 10 October PDOS image compiles, assembles and links all K/C applications with the desktop profile, then passes the 68-check bounded application matrix. The actual service uses 215,779,644 of 268,435,456 host bytes and leaves 78 percent of its enlarged guest service bank free. Object/source format and checked wrapper bounds remain explicit.

## AS-008: Explicit character-constant code page at the PDOS interface

- Type: interface / encoding
- Status: Open
- Target: Desktop product selection and portable character-constant emission
- Observation: The assembler's `mf_ascii_to_ebcdic` table is CP037 and is used for character constants as well as object names. PDOS's modern console and native text adapters use IBM1047. The 10 October native editor walkthrough shows bracket literals as different glyphs; the new operator messages now use portable punctuation.
- Direction: Review an explicit data/character-constant code-page selection for modern PDOS consumers, preserving classic object-name encoding, macro comparison semantics and the unchanged CP037 bootstrap default. Do not change the shared conversion table globally or silently convert runtime file data.
- Acceptance: Independent CP037/IBM1047 literal-byte vectors, compatible object names and macro/source replay, exact native C string and record-byte behavior, and affected guest consumers. This is a separate selected interface slice; the desktop capacity profile does not change encoding.
