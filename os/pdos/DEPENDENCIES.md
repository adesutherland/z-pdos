# PDIO1 build dependencies

1 October 2026. This records the selected repaired OS workload. Passing the
source checks and C-to-text build does not close its assembler, linker or
image dependencies.

| Stage | Selected input or interface | Current state |
| --- | --- | --- |
| OS C source | `pdos.c`, `pdosutil.c/.h`, `pload.c`, `pcomm.c` | Exact retained PDIO1 selection; all four C units compile with Classic C |
| Runtime C | START, STDIO, STDLIB, CTYPE, STRING, TIME, ERRNO, ASSERT, LOCALE, MATH, SETJMP, SIGNAL, MEMMGR | One maintained PDPCLIB with merged canonical/Lab fixes and `pdos-zarch` configuration; all 13 units compile with Classic C |
| Source macros | PDPTOP, PDPMAIN, PDPPRLG, PDPEPIL | Retained source-owned bytes; COPY, macro lookup and conditional state remain open |
| Kernel startup and support | SAPSTART, SAPSUPA, PDOSSUP | Retained source; whole assembly and z/Architecture encoding remain open |
| Loader support | PLOADSUP plus PLOAD and PDOSUTIL | Retained source; IPL placement and independent whole-module reconstruction remain open |
| Command processor | MVSSTART, MVSSUPA, PCOMM and the selected common runtime | Retained source; native services and whole assembly remain open |
| External native macros | YREGS, SAVE, RETURN, CVT, IEZJSCB, IHAPSA, IHARB, IHACDE, IEFJFCBN, IEZIOB, IHASVC; OPEN/CLOSE/DCB and other reached service forms | Calls exist in retained source. Original build supplied IBM MACLIB/MODGEN. Selected active forms need public interface/layout evidence and independently authored definitions |
| Qualified C producer | Repaired GCCMVS 3.2.3 v90, binary SHA-256 `f85eb831865c7de8eb12f74d26b5607414cf7a9204a4609fcd9f1826c3fa7bec` | Historical native kernel build input; the local Classic C checkpoint is a different GCC 3.4.6 producer |
| Qualified assembler/binder | ASMA90 and IEWL | Historical qualification route; replacing these requires independent object, relocation and load-image checks |
| Independent tools | `mf-classic-cc`, `mf-classic-as`, `mf-classic-ld` | C-to-text passes; full assembler/runtime/name closure and MVS load-image construction remain open |
| IPL source | Retained `s370/ipl3390.txt`, PLOAD startup and source-described IPL1/2 CCWs | Source is present; independently generated first-track/image checks remain open |
| Disk producer | Hercules `dasdload`, CKD/CCKD conversion, source-described IPL installation | Lab demonstrated source-built disk construction; standalone retained producer recipe and fresh inputs remain to be implemented here |
| Application inputs | Pinned external cREXX TSO31/TSO64 packages from their producer | External consumers; package binaries and private disk inputs are not OS source |

The accepted kernel link included the common runtime plus SAPSTART/SAPSUPA,
PDOS, PDOSSUP and PDOSUTIL. PLOAD uses the common runtime, PLOAD/PLOADSUP
and PDOSUTIL. PCOMM uses MVSSTART/MVSSUPA and the common runtime. Original
native member names such as SPSZ1, SPUZ1, PDIO1O, PDSIO1O and PDUZ8O are
build identities, not additional missing C source units. The repair rebuilt
only the changed kernel objects and reused identified common objects.

The retained [`compile-inputs.txt`](compile-inputs.txt) reproduces the 17 C
units and historical compile switches with the new producer. The retained
[`assembly-inputs.txt`](assembly-inputs.txt) selects ten source inputs.
`inventory.crexx` creates a per-input operation-count CSV in ignored output.
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
