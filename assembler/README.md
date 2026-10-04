# Mainframe Classic Assembler

Classic Assembler, command `mf-classic-as`, is an original implementation by
**Adrian Sutherland**, under MIT. It turns the supported classic mainframe
assembler language into 80-byte ESD/TXT/RLD/END object records for the linker.
It assembles both Classic C output and the handwritten native support used by
the source-built z/PDOS route.

The portable C89/C90 core separates source processing, macro expansion,
instruction encoding and object output. It uses caller-supplied storage and
callbacks so host I/O policy is outside the core. The command-line program
supplies the file and memory handling needed on desktop hosts; the resulting
executable has no cREXX runtime dependency.

This is a supported subset of the classic language and instruction families,
not a claim of full HLASM compatibility. Machine instruction ceilings, macro
operand coverage and object-format support are separate contracts. No as370
implementation, inherited assembler opcode table or IBM macro library was
imported. Native guest hosting is still separate development work.

- [User guide](doc/user/README.md)
- [Architecture and interfaces](doc/architecture/README.md)
- [Development](doc/development/README.md)
- [Known issues and next steps](doc/BACKLOG.md)
- [Origins and contributors](UPSTREAM.md), [licence](LICENSE), [agent rules](AGENTS.md)

`src/` is the maintained implementation; `scripts/` contains recipes run from
the repository root. Generated output belongs under ignored root `build/`.
Optional frozen `archive/` material is reference only and is excluded from
normal builds. Each component has one backlog and records its own evidence.
