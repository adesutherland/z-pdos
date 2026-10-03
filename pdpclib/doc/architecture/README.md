# PDPCLIB architecture

Shared portable C, headers, native assembler and source-owned macros live in
`../../src/`. `src/profiles/` selects system services and configuration;
`src/interfaces/` owns the original compatibility macro implementations.
The [PDOS31 interfaces](PDOS31-INTERFACES.md) and [classic linkage](CLASSIC-LINKAGE.md)
describe reached contracts and limits.
The [selected TSO/E interfaces](TSO31-INTERFACES.md) record the source-owned
terminal, EXTRACT and control-block forms and their host-only validation.

MVSSUPA consists of shared source modules in `src/native/mvssupa/` and
profile-specific modules selected by each `mvssupa.inputs` list. Preparation
concatenates them into one assembler input; it does not apply patches. The
original five selections reproduced the pre-reorganisation source bytes at
the reorganisation checkpoint. The additional SDK file profile is a new
source selection; subsequent maintained changes have new source identities.
