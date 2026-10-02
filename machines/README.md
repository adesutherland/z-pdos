# Shared machine definitions and references

This directory describes hardware targets and their instruction ceilings.
[The shared profile direction](doc/MACHINE-PROFILES.md) states which named
selectors are planned, enabled or unsupported; [source references](UPSTREAM.md)
identify architecture editions and factual interfaces used by original code.
[The qualified z/PDOS machine](profiles/pdos-zarch.md) records its concrete
hardware setup separately from kernel, ABI and runtime contracts.

The assembler owns its encoder implementation in ../assembler/src/machine.c.
Compiler-specific lowering and library/OS service selections stay in their
own src/ trees. Sharing a machine name does not establish compatible object
formats, C data models, character encodings, native entry or services.
Historical IBM profiles, community S/380 extensions and counterfactual designs
remain distinct. There is no CPU emulator implementation in this repository.
