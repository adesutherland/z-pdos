# Repaired z/PDOS build plan

1 October 2026. I selected the qualified PDIO1 source as our first OS rebuild
target. We preserve its accepted behavior and drive the Classic Tools work
from its actual compiler, runtime and support inputs. Source preservation,
C compilation, assembly, linking, source-built boot and application execution
have separate acceptance results. There is one maintained PDPCLIB source;
the exact original qualified runtime is preserved in Git. The active merged
runtime is a new candidate for the later link, boot and application gates.

| Gate | Acceptance | State |
| --- | --- | --- |
| PD-01: preserve source | Original qualified input preserved by commit; one maintained OS/runtime selection, notices, manifests and clean recovery; real-function old/new failure controls | Complete locally; see CHECKPOINT.md |
| PD-02: new C producer | All 17 selected units compile with pinned Classic C and explicit source/runtime configuration | Complete for assembly-text generation; symbol/helper closure and target execution open |
| PD-03: first Classic object | A small C function uses the retained PDPMAC convention, assembles independently and passes separately expected object/relocation checks | Complete locally; PDPROBE has independently checked bytes and two A relocations |
| PD-04: source assembly | The selected runtime, loader and kernel source assemble under an explicit z/Architecture kernel contract, with required macro/service and instruction coverage | C-object portion complete; six handwritten modules and named kernel ISA open; support first stops at TITLE |
| PD-05: independent link/image | Fresh PLOAD, PDOS and PCOMM link without reused native objects; validate entry/mode, relocation, payload and source-described IPL records | Open |
| PD-06: source-built boot | Fresh 100-cylinder 3390 image boots to a usable PCOMM prompt on the selected standard z/Architecture profile | Open for Classic-produced output |
| PD-07: application qualification | Pinned cREXX package passes compile/assemble/fresh execution, supplied/interactive I/O, diagnostics, checked-write recovery and complete stopped output readback | Open for the rebuilt candidate; historical PDIO1 result retained separately |

## First assembler increments

1. **Source resolution and bounded conditional macros.** Define COPY and
   macro-library lookup through explicit source-provider interfaces. Preserve
   source coordinates and verify every dependency across both passes. Reject
   missing/cyclic inputs and capacity exhaustion. Add the selected global/local
   variables, SET operations, AIF/AGO/ANOP/MEXIT, concatenation and SYSNDX
   needed by PDPTOP and the call macros. Use original small fixtures and
   independently expected output before the source-owned consumer tests.
2. **Compiler output and ordinary source directives.** Close TITLE/PRINT
   behavior, EQU/location expressions, address literals with checked addends,
   the reached constant/reservation forms and expression arithmetic. Keep
   layout, relocation and source metadata tests separate from instruction tests.
   Complete the PD-03 object before broadening to the whole runtime.
3. **Interface definitions and the selected ISA.** Inventory active forms after
   conditional expansion. Write independently sourced register, save/return,
   low-core/control-block and reached service definitions with explicit layout
   and byte checks. Extend the encoder for the selected kernel/support ISA
   using primary architecture facts and independent vectors. Reject other
   instructions after expansion and audit the final object independently.
4. **Whole-source and linker closure.** Assemble the actual runtime and support
   sources, then close external names, helper calls, alignment, entry metadata,
   linker placement and MVS load-record semantics. Qualify the kernel, loader
   and command processor as separate modules before constructing disk media.

The [dependency record](DEPENDENCIES.md) owns the current source/interface
inventory. These increments refine the existing assembler component contracts;
they do not import as370 code/tables or an IBM macro library. Each increment
must have a working consumer and appropriate affected regression checks.

## Compiler versus assembler choices

We maintain both tools. For each missing output form, compare a useful general
assembler facility with simpler compiler emission and record its ABI, object,
encoding and profile effects. COPY/conditional source handling also serves
the runtime and OS, so avoiding one generated COPY would not remove that
dependency. Direct frame emission or exact hexadecimal floating constants
may offer bounded compiler changes, but require separate review and tests.
The [Classic compiler roadmap](../../tools/classic-cc/ROADMAP.md) records
this decision policy and the existing alias/profile limitations.

The OS machine contract is standard z/Architecture, one CPU, 4,096 MiB real
storage, a 100-cylinder 3390 and the qualified IBM1047 console behavior.
The kernel's AMODE31/RMODE24 metadata and 32-bit C pointers do not restrict
its support instructions to System/370. Define an explicit kernel code-role
ceiling consistently across both tool families before enabling a selector.
The SDK's historical integer application profiles remain separate contracts.
Retain the existing generous high heap and code/stack mappings for 64-bit
applications. The later native 64-bit kernel conversion remains separate work.

## Boot and application evidence

The new producer must build all common objects from identified source; the
previous two-object native repair is historical evidence for its stated scope.
Generate IPL and CKD bytes explicitly and check them before starting a guest.
Use a new stopped candidate and Mainframe Lab's managed guest lease procedure,
retaining the accepted image through candidate acceptance. Source control
holds sources, recipes and compact results; disks and raw receipts remain private.

After boot, run the pinned cREXX package through its actual native chain and
input/error cases. Force or observe writes crossing a full track and verify
that the same complete record is retried. Stop normally and inspect every
output record and binary byte against the host reference. Independent review
then establishes acceptance of that exact source/tool/runtime/image candidate.
