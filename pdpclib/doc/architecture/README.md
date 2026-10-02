# PDPCLIB architecture

Shared portable C, headers, native assembler and source-owned macros live in
`../../src/`. `src/profiles/` selects system services and configuration;
`src/interfaces/` owns the original compatibility macro implementations.
The [PDOS31 interfaces](PDOS31-INTERFACES.md) and [classic linkage](CLASSIC-LINKAGE.md)
describe reached contracts and limits.

MVSSUPA consists of shared source modules in `src/native/mvssupa/` and
profile-specific modules selected by each `mvssupa.inputs` list. Preparation
concatenates them into one assembler input; it does not apply patches. The
five selections reproduce the pre-reorganisation source bytes exactly.
