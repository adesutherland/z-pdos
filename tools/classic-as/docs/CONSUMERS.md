# Consumer-driven expansion

I want this assembler to remove the need to return to z/OS when assembling the
native portions of newlib adapters, PDPCLIB and PDOS. That is the next consumer
qualification programme, not a claim about this initial bootstrap slice.

We first need the exact maintained source variant and its build inputs, then
the instructions, directives, expressions, macros, encodings, mode metadata,
relocations and downstream linker expectations it uses. Existing GNU-syntax
adapters belong to the ELF route until an explicit classic source counterpart
is maintained and qualified.

| Consumer | Features that guide later work | Separate acceptance |
| --- | --- | --- |
| Small 24/31-bit native entry adapter | RR/RX/RS/SI/SS, USING/DROP, literals, branch aliases, constants, mode metadata and independently owned service definitions | Assemble its exact maintained source; compare bytes/relocations and link/run a real caller |
| PDPCLIB native support | Multiple sections/entries, DSECT, symbol expressions, original traditional macros, COPY, conditional assembly and service/control-block dependencies | Preserve its own notices and source variant; assemble/link/run required library services |
| PDOS loader/kernel support | Privileged/profile-specific instructions, PSWs/channel words, mode changes and OS-owned definitions | Exact source-to-boot pipeline and a bounded guest proof |
| Classic compiler output and tool rebuilding | Generated language, literal/section conventions, external names, relocations and linkage | Compile/assemble/link the named tool from pinned source |
| Later 64-bit adapter | Selected real z/Architecture forms, long internal labels and an explicitly capable object format | Wider ISA, relocation/object route and native hosting qualified separately |

The first extension should close a small real consumer, adding one feature at a
time with independent byte and diagnostic tests. Any simplified bootstrap source
variant must be explicit, live with its component and be checked against the
maintained source. We do not hide an automatic rewrite or claim to rebuild an
unmodified source through a transformed variant.

Traditional service and control-block macros need individual rights review.
Where suitable, independently written small definitions or explicit source code
can replace them. Public availability or an old system's provenance does not
grant rights to later macro libraries. cREXX macro integration follows as an
optional provider after the basic traditional contract is useful.

Native z/PDOS hosting is a separate step: cross-build the same C source, supply
record/storage/sink adapters, measure code/BSS/stack/peak storage, and assemble a
real component there. No emulator, new OS service or guest asset is a dependency
of the present host seed.
