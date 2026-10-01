# Mainframe Classic Assembler: first increment and product seed

30 September 2026. **Approved; first bootstrap implementation and host QA
published.** The user
agreed the principles, interfaces, C89 baseline and MIT grant for original
material, and requested parallel implementation with unit tests and QA.
This repository owns the product. Bounded agents own the engine/CLI,
reader/values/encoder, and classic writer/tests;
the coordinator owns documentation, build integration and independent review.
Compiler repair, kernel work and guest runs are outside this seed. A local
snapshot was committed after the clean ownership migration and pushed on the
user's explicit instruction. [Commit 04d41ec](https://github.com/adesutherland/z-pdos/commit/04d41ecf1cf1f89f7125a2a127cc53cd5e5b0be5)
is the published migration checkpoint on `develop`. The user subsequently
approved committing and pushing the completed bootstrap implementation and QA,
then working through `entry31.asm`, PDPCLIB `mvssupa.asm`, and PDOS loader/kernel
support in that order. Each stage retains exact source provenance and separate
assembly, link and execution results.
The completed bootstrap is published as
[`e87652d`](https://github.com/adesutherland/z-pdos/commit/e87652d2bc1efdee4439302be4420a5c3ac8911e).
I subsequently approved committing and publishing the reviewed TSO31 entry
and first PDPCLIB language delivery, and selected MVS 3.8 / real System/370 /
24-bit as the first complete PDPCLIB target.
I then selected this repository as the maintained home of PDPCLIB and our
library fixes. The [library source record](../../../pdpclib/SOURCES.md)
identifies the complete import; its [change ledger](../../../pdpclib/CHANGES.md)
consolidates the Lab/SDK deltas and relevant canonical upstream fixes with explicit target
profiles. The selected MVS 3.8 configuration member is now present; full
assembly and native service qualification remain open.

Read the [architecture](APPROVED-DESIGN.md) and [component interfaces](APPROVED-CONTRACTS.md)
first. The home is [adesutherland/z-pdos](https://github.com/adesutherland/z-pdos).
It is selected for the OS and all Mainframe Classic Tools. The separately renamed
[Mainframe ELF SDK](https://github.com/adesutherland/mainframe-elf-sdk) owns the
modern GCC/ELF route. Their build formats and inherited licences remain distinct.

## Approved decisions

| Decision | Recommendation |
| --- | --- |
| Implementation | Original portable C; keep as370 and other inherited tools as Lab references |
| Language floor | C89/C90 common subset, with no mandatory native 64-bit integer type or POSIX service |
| Modularity | Synchronous source-level C interfaces, build-time binding, optional richer backends |
| Small build | Replayable input, supplied bounded storage, sequential output and essential diagnostics; identity preprocessing first |
| Internal text | UTF-8, with an ASCII-subset bootstrap language; binary data and target character encoding remain explicit |
| Richer build | Same assembly/encoding/object core, bounded traditional macro engine, optional later cREXX provider |
| Shared assets | Original machine descriptions and checked value/encoding utilities suitable for future decoding; CPU execution remains separate |
| New-code licence | MIT for original project code, descriptions and product documentation; preserve inherited component terms |
| First implementation target | Consumer-selected historical instruction/language subset and independently authored classic object writer |
| First complete PDPCLIB target | MVS 3.8, real System/370, AMODE 24/RMODE 24, traditional MVS linkage and classic object decks |

Machine/profile names, exact directive/macro coverage and resource budgets are
chosen from the source inventory before claiming implementation support. No
counterfactual ISA/ABI or exhaustive HLASM compatibility is selected.

## Product layout

```text
z-pdos/
  README.md                     OS and Classic Tools purpose/current status
  AGENTS.md                     Public development and qualification rules
  LICENSES.md                   Component terms and source ownership map
  architecture/                 Original machine/profile descriptions and citations
  lib/mf-machine/               Portable values and encode/decode utilities as needed
  pdpclib/                      Maintained inherited C library and named profiles
  tools/classic-ld/             Mainframe Classic Linker, inherited PDLD implementation
  tools/classic-as/
    README.md                   Purpose, implemented capabilities and build entry
    LICENSE                     Grant for original assembler material
    AGENTS.md                   Component instructions and contract checks
    include/                    Small public C interfaces
    src/                        Reader, engine, writer and optional backends
    docs/ARCHITECTURE.md         Current implemented modules plus proposed extensions
    docs/INTERFACES.md           Source-level contracts and ownership
    docs/USER.md                 Actual invocation, source/object profiles and errors
    docs/BOOTSTRAP.md            Minimal host, build and resource requirements
    tests/                      Reviewed original fixtures and independent expectations
  tools/build.crexx             Orchestration for capable development hosts
```

This is the selected layout. Create shared libraries and optional directories only
when they have actual contents/consumers; no empty emulator implementation or
placeholder compiler is required. The classic linker and PDPCLIB now have their
own components. The compiler now has its own source and host-codegen component
in `tools/classic-cc/`. OS and future librarian source areas are separate
additions. The assembler's documented direct C compile/link
recipe must work without cREXX or a code generator on a bootstrap host. Retained
project orchestration/generation scripts use cREXX on hosts that can run it.

## Work sequence and acceptance

1. **Agree the design — complete.** The nine principles and component contracts,
   including C89, replay, explicit encodings, optional macros and the original
   MIT grant, were approved on 30 September 2026.
2. **Prepare the original product seed locally — complete.** Create the selected source
   home and documentation. Grant MIT only to original material, with a separate
   component licensing map. Do not copy Lab history, private evidence, guest
   assets, upstream assembler code/tables or unreviewed macro source. User and
   architecture documentation must distinguish implemented and proposed features.
3. **Inventory exact consumers — initial inventory complete; build selection open.** Pin newlib/PDPCLIB/PDOS source variants and
   compiler-generated assembly for tool rebuilding. Record directives, machine
   features, expressions, macros, sections/entries, mode metadata, relocations,
   encodings and downstream linker requirements. Existing GNU-syntax adapters
   are separate source inputs, not silently retagged as classic syntax.
   The initial [consumer inventory](CONSUMER-INVENTORY.md)
   records the local source hashes and feature groups. The first complete
   PDPCLIB target and source member are selected; its service-definition
   edition, later build qualification and compiler-output consumers remain open.
4. **Build a narrow original vertical slice — complete locally.** Implement the basic source
   adapter, provider, bounded expression/symbol/layout engine, selected formats
   and classic object output needed by the first reviewed sources. No macro
   engine or emulator dependency is required. Check successful semantic output,
   useful failures and invalidated output on capacity/I/O errors.
5. **Expand to actual runtime/OS assembly.** Add only features found in the
   consumer inventory, with independently expected values and diagnostic cases.
   Service macros/definitions live with their source component. Document any
   simplified bootstrap variants rather than hiding a source transformation.
   The [TSO31 entry checkpoint](../../../runtime/tso31/CHECKPOINT.md) closes its
   host assembly, target layout and entry-only link checks. The subsequent
   `mf-classic-ld` repair closes the nonzero-origin address audit for its retained
   package; full maintained-library assembly and guest execution remain open.
   The [first PDPCLIB language checkpoint](PDPCLIB-CHECKPOINT.md) closes
   bounded ordinary layout semantics and the optional streaming traditional
   substitution provider. Conditional controls, further layout/attributes, service definitions,
   target-member assembly and whole-library qualification remain open.
6. **Qualify native hosting separately.** Cross-build the same portable source
   for the chosen z/PDOS host; measure code, data/BSS, stack and peak memory;
   assemble a real component there and compare output. Missing services are
   diagnosed at the point reached, not supplied speculatively.

The first seed is accepted when the home and original source boundary are clear,
the own README/licence/user/agent/architecture documents are present, the small
build is reproducible, and the selected vertical slice passes independent output
and failure checks. It does not establish whole newlib/PDPCLIB/PDOS assembly,
native hosting, downstream link/load execution or an open source-to-IPL route.

## References and current checkpoint

Use [the source guide](../../../architecture/SOURCES.md), [consumer inventory](CONSUMER-INVENTORY.md) and [local checkpoint](CHECKPOINT.md). Original test sources and fixed expected bytes belong here; retained native guest outputs and inherited implementations remain outside the product. No passing guest run is repeated for this seed.

## Repaired PDIO1 OS consumer — 1 October

The [repaired PDIO1 OS import](../../../os/pdos/README.md) preserves the actual
qualified input in Git history. Its working build uses the single maintained
PDPCLIB with merged fixes, with all 17 C units compiling to text
under Classic C. Its [build plan](../../../os/pdos/BUILD-PLAN.md) now owns the
concrete OS consumer sequence and first source-language failures. The existing
MVS 3.8/System/370 library target retains its separate scope.
