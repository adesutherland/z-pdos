# Mainframe toolchain names

I adopted these product names on 30 September 2026. The maintained toolchains have their own product identities and source homes.

## Products and components

| Product family | Component | Agreed command name | Source basis |
| --- | --- | --- | --- |
| Mainframe ELF SDK | Mainframe ELF C | `mf-elf-cc` | Maintained GCC 16.2.0 port |
| Mainframe ELF SDK | Mainframe ELF Assembler | `mf-elf-as` | GNU assembler |
| Mainframe ELF SDK | Mainframe ELF Packager | `mf-elf-pack` | ELF-to-native executable packaging |
| Mainframe Classic Tools | Mainframe Classic C | `mf-classic-cc` | GCC 3.4.6 / cc370 lineage |
| Mainframe Classic Tools | Mainframe Classic Assembler | `mf-classic-as` | Independently authored portable C; as370 retained as a Lab reference |

These are the agreed public interface names. The existing experimental commands
and source filenames remain the implemented entry points until the owning
projects provide the named commands. Naming adoption does not claim a new
compiler build, assembler repair or guest qualification. Upstream names,
versions, attribution and component licences remain visible.

The 1 October [Classic C source checkpoint](../../classic-cc/CHECKPOINT.md)
now provides `mf-classic-cc` for native host assembly-text generation. Its
independent assembler integration and guest qualification remain open.

The same scheme can extend to linkers and librarians, such as `mf-classic-ld`
and `mf-classic-ar`, when those components are selected and qualified.

## Toolchain contracts

Mainframe ELF SDK uses ELF for compilation and linking, followed by explicit
native CMS/MVS executable packaging. Forward-looking application work focuses
on 31-bit and 64-bit profiles, retaining the existing historical 24-bit route.
ELF identifies the object/linking route; each profile still defines its C ABI,
instruction set, encoding, runtime services and native executable format.

Mainframe Classic Tools uses traditional mainframe assembler syntax, object
records and calling conventions. GCC 3.4.6 / cc370 is the classic compiler
lineage. My subsequent 30 September decision selects an independently authored
portable C assembler; as370 is retained as a Lab reference rather than imported
as product code. The [architecture proposal](APPROVED-DESIGN.md),
[interfaces](APPROVED-CONTRACTS.md) and [first-increment plan](PLAN.md)
define the approved bootstrap and intended richer builds. Existing GCCCMS/GCCMVS and
recovered XF results remain separately named references. Compiler repair,
the full assembler consumer coverage, native hosting and complete source rebuilding remain open.

Addressing widths and machine generations belong to profiles, rather than
separate product names. Real 24-bit, 31-bit and 64-bit targets acquire their own
supported contracts and checks. Counterfactual profiles require separately
agreed ISA and ABI definitions; their names do not imply historical IBM support.
The [shared machine-profile direction](../../../architecture/MACHINE-PROFILES.md)
records consistent names and instruction ceilings across Classic and ELF routes.

## Macro policy

The classic assembler owns a documented subset of the traditional macro
language. Project-authored service macros and control-block definitions belong
with the runtime or source component using them. Prefer simpler explicit source
where that removes unnecessary dependence on a large macro library.

Retain each inherited file's actual notices and provenance. Independently write
later service definitions from documented interfaces; copying private IBM macro
source or expansions is not part of this route. Traditional macro-language
support does not imply full HLASM or IBM macro-library compatibility. Later cREXX
macro integration is an extension of the classic assembler, with its interface
still to be designed.

## Repository names

Mainframe ELF SDK's GitHub home is
[adesutherland/mainframe-elf-sdk](https://github.com/adesutherland/mainframe-elf-sdk).
The modern repository owns the maintained GCC/ELF route.

[adesutherland/z-pdos](https://github.com/adesutherland/z-pdos) owns Mainframe Classic Tools and the OS. The original assembler and explicitly attributed inherited compiler, linker and PDPCLIB components live here with their own licences, source, documentation and tests. Manual downloads, mail archives and private native qualification outputs remain outside the retained source. A local checkpoint is not a public release.

The [assembler guide](../README.md), [plan](PLAN.md) and [checkpoint](CHECKPOINT.md) own current implementation status and next steps.
