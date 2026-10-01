# z/PDOS and Mainframe Classic Tools

I want a mainframe system and toolchain that we can build from maintained source
on ordinary hosts, and eventually use to rebuild itself. This repository is the
home of z/PDOS and **Mainframe Classic Tools**, which use traditional mainframe
assembly and object conventions.

The first component is [Mainframe Classic Assembler](tools/classic-as/README.md),
command `mf-classic-as`. Its original portable C core has a small bootstrap
language and explicit interfaces for source records, storage and object output.
The first optional traditional macro provider uses those interfaces; conditional
assembly and optional cREXX integration follow. This is a host component proof; complete
runtime/OS assembly and native z/PDOS hosting remain future qualification steps.

The first real consumer, the [TSO31 entry adapter](runtime/tso31/README.md), now
assembles with independent object checks and passes target C layout assertions.
Its [checkpoint](runtime/tso31/CHECKPOINT.md) records entry-only host link
qualification and the subsequent repair of an inherited PDPCLIB address defect
in the maintained linker. Guest execution and complete PDPCLIB/PDOS source
assembly remain open.

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

**Mainframe Classic C**, command `mf-classic-cc`, is the intended classic
GCC/cc370 lineage. Its maintained implementation and OS source are not yet
included. Each future import retains its actual licence. See [licences](LICENSES.md)
and [agent guidance](AGENTS.md).

The separate [Mainframe ELF SDK](https://github.com/adesutherland/mainframe-elf-sdk)
owns the modern GCC and GNU assembler route, with `mf-elf-cc`, `mf-elf-as` and
`mf-elf-pack`. Real 24-, 31- and 64-bit architectures and any future
counterfactual machines are explicit profiles, not different product names.
