# Mainframe Classic C

Our maintained GCC 3.4.6/i370/cc370 compiler, command `mf-classic-cc`, retaining GCC terms, Jan Stein, Dave Pitts, Linas Vepstas, Paul Edwards and other contributors, and Mike Beer's recorded repairs.

MVS and CMS native host code-generation checks pass. The selected z/PDOS workload builds through the Classic route. The current launcher supports compile-to-text and rejects unsupported driver operations; named machine-profile selectors remain unqualified.

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
