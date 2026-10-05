# z/PDOS build and qualification contract

1 October 2026. I selected the qualified PDIO1 source as our first OS rebuild
target. We preserve its accepted behavior and drive the Classic Tools work
from its actual compiler, runtime and support inputs. Source preservation,
C compilation, assembly, linking, source-built boot and application execution
have separate acceptance results. There is one maintained PDPCLIB source;
the exact original qualified runtime is preserved in Git. The active merged
runtime has passed the boot and application gates recorded for z/PDOS 0.1.

| Gate | Acceptance | State |
| --- | --- | --- |
| PD-01: preserve source | Original qualified input preserved by commit; one maintained OS/runtime selection, notices and current-source build receipts; real-function checks and explicit old failure fixtures | Complete locally; see CHECKPOINT.md |
| PD-02: new C producer | All 17 selected units compile with pinned Classic C and explicit source/runtime configuration | Complete; the resulting OS passes PD-06/07 |
| PD-03: first Classic object | A small C function uses the retained PDPMAC convention, assembles independently and passes separately expected object/relocation checks | Complete locally; PDPROBE has independently checked bytes and two A relocations |
| PD-04: source assembly | The selected runtime, loader and kernel source assemble under an explicit z/Architecture kernel contract, with required macro/service and instruction coverage | Complete locally: all 17 C objects and six handwritten modules, including MVSSUPA; selected original interfaces and the explicit kernel contract below |
| PD-05: independent link/image | Fresh PLOAD, PDOS and PCOMM link without reused native objects; validate entry/mode, relocation, payload and source-described IPL records | Complete locally: real loader checks at two bases, checked fresh 100-cylinder image, compression readback and corruption controls; see CHECKPOINT.md |
| PD-06: source-built boot | Fresh 100-cylinder 3390 image boots to a usable PCOMM prompt on the selected standard z/Architecture profile | Complete for z/PDOS 0.1 |
| PD-07: application qualification | Pinned cREXX package passes compile/assemble/fresh execution, supplied/interactive I/O, diagnostics, checked-write recovery and complete stopped output readback | Complete for TSO31, TSO64 ANY and TSO64 HIGH; TSO24 was tested and requires the low-residence path in ../BACKLOG.md |

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

The [dependency record](../architecture/DEPENDENCIES.md) owns the current source/interface
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
The [Classic compiler backlog](../../../compiler/doc/BACKLOG.md) records
this decision policy and the existing alias/profile limitations.

The OS machine contract is standard z/Architecture, one CPU, 4,096 MiB real
storage, a 100-cylinder 3390 and the qualified IBM1047 console behavior.
The kernel's AMODE31/RMODE24 metadata and 32-bit C pointers do not restrict
its support instructions to System/370. The named `pdos-zarch` kernel contract selects the z900 base ISA ceiling.
Classic C currently emits the System/370 subset of that ceiling; handwritten
support uses the assembler z900 subset, including 64-bit and channel support.
Compiler pointer width remains 32 bits; final modules use classic objects and
AMODE31/RMODE24 for the kernel/loader and AMODE31/RMODE ANY for PCOMM.
This code role does not select the SDK's integer application
ABI or claim a new instruction facility. Later selectors must use the same
hardware ceiling in Classic and ELF tools.
The SDK's historical integer application profiles remain separate contracts.
Retain the existing generous high heap and code/stack mappings for 64-bit
applications. The proposed [two-space successor](../architecture/TWO-SPACE-POC.md)
uses an AMODE64 assembler nucleus with C31 supervisor services; integrating it
into a native boot image remains separate work.

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

[QUALIFICATION.md](../qualification/QUALIFICATION.md) records the completed 2 October checks.
[BACKLOG.md](../BACKLOG.md) owns the next compatibility and UI work; earlier
checkpoints remain in Git rather than separate maintained trees.
