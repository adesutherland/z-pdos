# PDPCLIB

Paul Edwards created the Public Domain C Library. We maintain our descendant here with his dedication/fallback permission and all other contributor notices.

The shared `pdos-zarch` selection is used by z/PDOS 0.1. Retained ports and other service profiles have their own support and qualification limits.

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
