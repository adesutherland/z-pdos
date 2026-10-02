# Component licences and source origins

The [root MIT licence](LICENSE) covers original project material only.
It does not relicense inherited source, derived GPL repairs or cited manuals.
Each component preserves its actual terms, source notices and contributors.

| Component | Origin and terms |
| --- | --- |
| [z/PDOS](pdos/UPSTREAM.md) | Paul Edwards's PDOS; inherited public-domain declarations and notices. [Component licence](pdos/LICENSE) scopes original MIT work. |
| [PDPCLIB](pdpclib/UPSTREAM.md) | Paul Edwards's Public Domain C Library and its contributors; inherited dedication/fallback permission and file notices. [Licence](pdpclib/LICENSE). |
| [Assembler](assembler/UPSTREAM.md) | Independently authored original implementation and tests, [MIT](assembler/LICENSE). |
| [Compiler](compiler/UPSTREAM.md) | GCC 3.4.6/i370/cc370 lineage, with recorded repairs. [Licence scope](compiler/LICENSE), [GPL text](compiler/COPYING), and inherited library/header notices and exceptions in src/. |
| [Linker](linker/UPSTREAM.md) | Inherited PDLD with public-domain declarations and contributors; original integration/tests/documentation retain [their MIT scope](linker/LICENSE). |
| [TSO31 bridge](tso31-bridge/UPSTREAM.md) | Original project-authored source, reference, ABI and tests, [MIT](tso31-bridge/LICENSE). |
| [Machine descriptions](machines/UPSTREAM.md) | Original descriptions and cited facts, [MIT](machines/LICENSE); external manuals/websites are not included or relicensed. |
| Root documentation and orchestration | Original project material, MIT. |

Frozen component archive/ baselines preserve their source notices and licences;
normal builds use src/ directly. Provenance records do not establish support or
qualification for every inherited target. No as370 implementation, inherited
assembler opcode table, IBM macro library, private native object/listing, guest
disk or correspondence is included by this reorganisation.
