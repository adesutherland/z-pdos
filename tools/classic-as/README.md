# Mainframe Classic Assembler

`mf-classic-as` assembles a small traditional mainframe language into classic
80-byte ESD/TXT/RLD/END object records. This is an original portable C89 bootstrap
implementation. It is intended to grow into the cross-assembler for native
runtime/OS source and later run on z/PDOS.

The [checkpoint](docs/CHECKPOINT.md) distinguishes the published migration
snapshot from the completed local implementation and independent host QA.

Build and run from the repository root:

```sh
crexx tools/build.crexx --args test
build/classic-as/tools/classic-as/mf-classic-as \
  tools/classic-as/tests/fixtures/bootstrap.asm build/example.obj
```

The [direct C build](docs/BOOTSTRAP.md) needs no cREXX or CMake. The core's
explicit callbacks also support different record, storage and output adapters.
An existing output path is refused; use a fresh path for each assembly.

- [User guide](docs/USER.md): invocation, actual language/ISA/object subset and failures.
- [Architecture](docs/ARCHITECTURE.md): modules, responsibility and optional growth.
- [Interfaces](docs/INTERFACES.md): storage, providers, encoder and writer contracts.
- [Bootstrap host/build guide](docs/BOOTSTRAP.md): minimal services and QA commands.
- [Consumer programme](docs/CONSUMERS.md): runtime/OS expansion and separate qualification.
- [MIT licence](LICENSE), [source/licence map](../../LICENSES.md), [agent guide](AGENTS.md).

This seed qualifies the documented host component subset. It does not assemble
all newlib/PDPCLIB/PDOS sources, implement a macro engine or emulator, qualify
native z/PDOS hosting, or establish source-to-IPL. Traditional macros and optional
cREXX preprocessing are future backends through the same statement interface.
