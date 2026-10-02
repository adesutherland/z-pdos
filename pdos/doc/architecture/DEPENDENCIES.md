# PDIO1 build dependencies

2 October 2026. This records the selected repaired OS workload. The independent
source-to-image build now passes locally. Its boot and application acceptance
remain separate from assembly, linking and disk readback.

| Stage | Selected input or interface | Current state |
| --- | --- | --- |
| OS C source | `pdos.c`, `pdosutil.c/.h`, `pload.c`, `pcomm.c` | Exact retained PDIO1 selection; all four C units compile and independently assemble with Classic C/Assembler |
| Runtime C | START, STDIO, STDLIB, CTYPE, STRING, TIME, ERRNO, ASSERT, LOCALE, MATH, SETJMP, SIGNAL, MEMMGR | One maintained PDPCLIB with merged canonical/Lab fixes and `pdos-zarch` configuration; all 13 units compile and independently assemble with Classic C/Assembler |
| Source macros | PDPTOP, PDPMAIN, PDPPRLG, PDPEPIL | Retained source-owned bytes; all selected C and handwritten consumers assemble with bounded COPY, library lookup and conditional state |
| Kernel startup and support | SAPSTART, SAPSUPA, PDOSSUP | All assemble under the z900 ceiling and explicit PDOS services; freshly linked PLOAD/PDOS load reconstruction passes |
| Loader support | PLOADSUP plus PLOAD and PDOSUTIL | Fresh PLOAD links; source-described first-record placement, startup PSW and all disk bytes pass |
| Command processor | MVSSTART, MVSSUPA, PCOMM and the selected common runtime | Fresh PCOMM assembles/links and passes PDOS's actual loader at two bases; absent MVS/TSO services are explicitly rejected |
| External native macro interfaces | YREGS, SAVE, RETURN, CVT and the reached control-block/service forms | Original build used IBM MACLIB/MODGEN. This route uses independently authored selected interfaces, checked offsets/bytes and source-owned service expansions; it does not import IBM macros or establish a complete replacement library |
| Qualified C producer | Repaired GCCMVS 3.2.3 v90, binary SHA-256 `f85eb831865c7de8eb12f74d26b5607414cf7a9204a4609fcd9f1826c3fa7bec` | Historical native kernel build input; the local Classic C checkpoint is a different GCC 3.4.6 producer |
| Qualified assembler/binder | ASMA90 and IEWL | Historical qualification route; replacing these requires independent object, relocation and load-image checks |
| Independent tools | `mf-classic-cc`, `mf-classic-as`, `mf-classic-ld` | 23 fresh objects and all three load modules pass; no reused native objects or binder |
| IPL source | Historical `s370/ipl3390.txt` in the frozen upstream archive, current PLOAD startup and explicit IPL1/2 CCWs in src/install-ipl.c | Original host byte writer/checker verifies literal CCW vectors, source PLOAD PSW, VTOC extents and complete payload readback; builds do not read the archive |
| Disk producer | Hercules `dasdload`, CKD/CCKD conversion, source-described IPL installation | `pdos/scripts/image.crexx` builds a new 100-cylinder 3390, checks compression readback, rejects corruption and preserves existing outputs; utilities are explicit external host inputs |
| Application inputs | Pinned external cREXX TSO31/TSO64 packages from their producer | External consumers; package binaries and private disk inputs are not OS source |

The accepted kernel link included the common runtime plus SAPSTART/SAPSUPA,
PDOS, PDOSSUP and PDOSUTIL. PLOAD uses the common runtime, PLOAD/PLOADSUP
and PDOSUTIL. PCOMM uses MVSSTART/MVSSUPA and the common runtime. Original
native member names such as SPSZ1, SPUZ1, PDIO1O, PDSIO1O and PDUZ8O are
build identities, not additional missing C source units. The repair rebuilt
only the changed kernel objects and reused identified common objects.

The retained [`compile-inputs.txt`](../../scripts/compile-inputs.txt) reproduces the 17 C
units and historical compile switches with the new producer. The retained
[`assembly-inputs.txt`](../../scripts/assembly-inputs.txt) selects ten source inputs.
`pdos/scripts/inventory.crexx` creates a per-input operation-count CSV in ignored output.
It counts spelled directives, instructions, macros and continuation records;
inactive conditional branches and macro prototypes are included. It cannot
establish active instruction coverage or a final-object ISA ceiling.

The generated kernel starts with `COPY PDPTOP`. Its output uses source-owned
entry/exit macros, `EQU *`, branch aliases, literal pools, address literals
with expressions, external references and page tables. The full runtime also
uses IBM hexadecimal floating point; the shared SDK integer-only contract
does not automatically admit that code.

PDOSSUP starts with `TITLE`, then `COPY PDPTOP` and YREGS. It uses AIF/AGO
and source-owned architecture switches. Selected z/Architecture branches
include `STMG`, `LPSWE`, subchannel operations and full-width context code.
An AMODE31 kernel can require these instructions. The assembler's `s370`
selector is only a first-language diagnostic probe here, not the OS's machine
profile. `&ZSYS='S380'` is the inherited application step-down convention;
`&XSYS='ZARCH'` selects OS instructions. This is standard z/Architecture,
not the community S/380 ISA or the fictional counterfactual architecture.

Some upstream IPL/conversion utilities cast host pointers to `int` or serialize
native `short`/`int` fields. A successful native host compile would not make
those tools endian-, alignment- or host-width-safe. The standalone producer
must use explicit target byte encodings and checked relocation/layout rules.
