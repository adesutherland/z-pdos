# z/PDOS and Mainframe Classic Tools

I want a mainframe system and toolchain that we can build from maintained source
on ordinary hosts, and eventually use to rebuild itself. This repository is the
home of z/PDOS and **Mainframe Classic Tools**, which use traditional mainframe
assembly and object conventions.

The [repaired PDIO1 OS source](os/pdos/README.md) now preserves the kernel
used for the 1 October cREXX beta 3 HIGH qualification, with repair history
and recovery checks. It uses our one maintained PDPCLIB, with the relevant
canonical upstream and Lab fixes merged into that library. Its 17 C build units
compile with Mainframe Classic C. The earlier exact qualified input is kept
in Git history. All selected sources now independently assemble and link into
a checked fresh 100-cylinder 3390 image. Source-built boot and application
qualification remain open under the
[OS build plan](os/pdos/BUILD-PLAN.md).

The first component is [Mainframe Classic Assembler](tools/classic-as/README.md),
command `mf-classic-as`. Its original portable C core has a small bootstrap
language and explicit interfaces for source records, storage and object output.
The first optional traditional macro provider uses those interfaces; conditional
assembly supports the selected PDOS sources. Host assembly and disk building
pass; native z/PDOS hosting remains a future qualification step.

The first real consumer, the [TSO31 entry adapter](runtime/tso31/README.md), now
assembles with independent object checks and passes target C layout assertions.
Its [checkpoint](runtime/tso31/CHECKPOINT.md) records entry-only host link
qualification and the subsequent repair of an inherited PDPCLIB address defect
in the maintained linker. Guest execution remains open; the selected PDOS
runtime now assembles through the independent route.

[PDPCLIB](pdpclib/README.md), Paul Edwards's Public Domain C Library, is now
maintained here with its upstream notices and named target variants. I use
this repository as the source of truth for our library changes. Mainframe Lab
keeps reference snapshots and experiments. The first complete library target
is MVS 3.8 / real System/370 / 24-bit; importing source does not qualify its
complete assembly or guest execution.

[Mainframe Classic Linker](tools/classic-ld/README.md), command `mf-classic-ld`,
is the assembler's sibling in this tool family. Its initial implementation
maintains the inherited PDLD linker with explicit source provenance and fixes;
it is distinct from the independently authored assembler. The first repair
addresses nonzero section origins in classic object input.
The inherited linker currently uses C99 and heap-allocated object state; the
assembler's C89/C90 bootstrap core retains its supplied-storage interfaces.
Each component owns its implementation, architecture, user guide and tests.

The root CMake build includes both tools and the focused PDPCLIB checks:

```sh
crexx tools/build.crexx --args test
```

Use `-DMF_CLASSIC_LINKER=OFF` when configuring a build that needs only the
assembler and its current consumers. The linker can also be built separately
using its [component instructions](tools/classic-ld/README.md).

**[Mainframe Classic C](tools/classic-cc/README.md)**, command `mf-classic-cc`,
now maintains the GCC 3.4.6 / cc370 compiler source with the reconciled upstream,
Mike Beer and Mainframe Lab repairs. Native MVS and CMS cross-builds pass the
code-generation checks. The selected PDOS compile/assemble/link/image route
passes locally; guest execution remains open. See its
[checkpoint](tools/classic-cc/CHECKPOINT.md), [roadmap](tools/classic-cc/ROADMAP.md),
[licences](LICENSES.md) and [agent guidance](AGENTS.md).

The separate [Mainframe ELF SDK](https://github.com/adesutherland/mainframe-elf-sdk)
owns the modern GCC and GNU assembler route, with `mf-elf-cc`, `mf-elf-as` and
`mf-elf-pack`. Real 24-, 31- and 64-bit architectures and any future
counterfactual machines are explicit profiles, not different product names.
The [shared machine-profile direction](architecture/MACHINE-PROFILES.md)
aligns names and instruction ceilings while preserving each tool family's
ABI, object, character and runtime contracts.
