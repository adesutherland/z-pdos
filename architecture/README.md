# Machine descriptions

The assembler's original C descriptions currently live in
[`machine.c`](../tools/classic-as/src/machine.c), with checked value and
character conversion helpers in [`values.c`](../tools/classic-as/src/values.c).
Their interface is independent of source syntax and object serialization.
Use the [source guide](SOURCES.md) for edition-specific references.

A future decoder or emulator can reuse these descriptions and helpers. CPU
execution, registers, memory, faults, interrupts and devices need separate
interfaces and independent semantic tests. There is no emulator in this seed.
Historical S/360 and S/370 profiles are distinct; later real IBM, community
S/380 and counterfactual profiles need their own decisions and qualification.

[The shared machine-profile direction](MACHINE-PROFILES.md) aligns the Classic
and ELF tool families' machine names and instruction ceilings. It records
the current implementation gaps without claiming ABI or object compatibility.
