# Mainframe Classic Assembler

An original assembler by Adrian Sutherland, command `mf-classic-as`, under MIT. Its portable C89/C90 core emits classic 80-byte ESD/TXT/RLD/END object records.

The selected z/PDOS build route is recorded separately; native guest hosting remains open.

`src/` is the single maintained implementation. `tests/` owns its checks;
`scripts/` owns build recipes, run from the repository root. The optional
`archive/` is frozen reference material and is excluded from normal builds
and required tests. Source changes are ordinary Git changes without patch stacks.

- [Build and user guide](doc/user/README.md)
- [Architecture](doc/architecture/README.md)
- [Upstream and contributors](UPSTREAM.md)
- [Licence scope](LICENSE)
- [Documentation index](doc/README.md)
- [Agent guidance](AGENTS.md)
- [Backlog and known issues](doc/BACKLOG.md)
