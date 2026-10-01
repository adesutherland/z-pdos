# Component licences and sources

The [MIT licence](LICENSE) covers original project code, documentation, tests
and independently authored machine descriptions in this initial source tree.
The assembler also has its [own licence](tools/classic-as/LICENSE).

| Material | Terms and provenance |
| --- | --- |
| `tools/classic-as/` | Original Mainframe Classic Assembler implementation and tests, MIT |
| Upstream PDLD files in `tools/classic-ld/` | Mainframe Classic Linker maintains canonical PDOS revision `a65eddb9ef4b27a6844f2857db0c98696137612b`; upstream public-domain dedication and unrestricted fallback permission with notices retained; see its [licence](tools/classic-ld/LICENSE) and [source record](tools/classic-ld/SOURCES.md) |
| Original classic linker changes, build integration, guides and tests | MIT; inherited PDLD material retains its own terms |
| `architecture/` | Original descriptions and references, MIT |
| `runtime/tso31/` | Original project-authored TSO entry bridge, preserved reference and maintained explicit-service variant, ABI documentation and tests, MIT |
| Upstream files in `pdpclib/` | Paul Edwards's Public Domain C Library, pinned PDOS mirror revision `0fe81209e78d022b40301f86f97c7f4d3e406d0a`; upstream public-domain dedication and fallback permission, with original contributor notices retained; see its [licence](pdpclib/LICENSE) and [source record](pdpclib/SOURCES.md) |
| Original PDPCLIB changes, integration guides, patches and tests | MIT; this grant does not alter the imported library's public-domain terms |
| Root documentation and build configuration | Original project material, MIT |

This tree contains no imported as370 implementation, upstream opcode table,
IBM macro library, guest object/listing, compiler source or OS source. It is
an original assembler plus the explicitly attributed inherited runtime and linker above;
no formal clean-room claim is made. Architecture and object-format facts are
cited in [the source guide](architecture/SOURCES.md).
The cited manuals and websites are not included and are not relicensed here.

When a compiler, runtime or OS component is added, record its exact source,
licence and notices here and retain its own terms. The MIT grant cannot change
GCC, newlib, PDPCLIB, PDOS or any other inherited component's licence.
