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
qualification and an inherited PDPCLIB relocation defect. Full
link/execution acceptance and complete PDPCLIB/PDOS assembly remain open.

**Mainframe Classic C**, command `mf-classic-cc`, is the intended classic
GCC/cc370 lineage. Its maintained implementation and OS source are not yet
included. Each future import retains its actual licence. See [licences](LICENSES.md)
and [agent guidance](AGENTS.md).

The separate [Mainframe ELF SDK](https://github.com/adesutherland/mainframe-elf-sdk)
owns the modern GCC and GNU assembler route, with `mf-elf-cc`, `mf-elf-as` and
`mf-elf-pack`. Real 24-, 31- and 64-bit architectures and any future
counterfactual machines are explicit profiles, not different product names.
