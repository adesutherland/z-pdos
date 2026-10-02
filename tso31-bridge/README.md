# TSO31 entry bridge

A project-authored MIT 31-bit TSO-to-ELF entry adapter, maintained independently of the z/PDOS kernel and PDPCLIB.

It is synchronous and non-reentrant. Host source/deck, target-layout and entry-only link checks do not establish complete runtime linking or guest execution.

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
