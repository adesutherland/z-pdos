# Component licences and sources

The [MIT licence](LICENSE) covers original project code, documentation, tests
and independently authored machine descriptions in this initial source tree.
The assembler also has its [own licence](tools/classic-as/LICENSE).

| Material | Terms and provenance |
| --- | --- |
| `tools/classic-as/` | Original Mainframe Classic Assembler implementation and tests, MIT |
| `architecture/` | Original descriptions and references, MIT |
| `runtime/tso31/` | Original project-authored TSO entry bridge, preserved reference and maintained explicit-service variant, ABI documentation and tests, MIT |
| Root documentation and build configuration | Original project material, MIT |

This tree contains no imported as370 implementation, upstream opcode table,
IBM macro library, guest object/listing, compiler source or OS source. It is
independently authored; no formal clean-room claim is made. Architecture and
object-format facts are cited in [the source guide](architecture/SOURCES.md).
The cited manuals and websites are not included and are not relicensed here.

When a compiler, runtime or OS component is added, record its exact source,
licence and notices here and retain its own terms. The MIT grant cannot change
GCC, newlib, PDPCLIB, PDOS or any other inherited component's licence.
