# z/PDOS 0.1

Paul Edwards created PDOS. We maintain z/PDOS as a descendant of his mainframe implementation, preserving the inherited public-domain notices and attribution while maintaining our repairs directly in source.

The recorded 0.1 fresh image boots and runs unchanged cREXX TSO31, TSO64 ANY and TSO64 HIGH packages. AMODE24 loading remains a known limitation. New build evidence and that exact guest qualification are distinct.

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
