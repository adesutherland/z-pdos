# PDPCLIB

PDPCLIB is the Public Domain C Library created by **Paul Edwards**, with the
contributions and notices recorded in [UPSTREAM.md](UPSTREAM.md). We maintain
the shared C implementation, headers, mainframe startup and native service
support here.

It has two important roles in this project. The z/PDOS build uses PDPCLIB as
its C library for the loader, kernel and command processor. The separate
[Mainframe ELF SDK](https://github.com/adesutherland/mainframe-elf-sdk) uses
selected native assembler services beneath its own Newlib runtime; it does
not use the whole PDPCLIB C library as a replacement for Newlib.

A **profile** chooses the source modules, macro configuration and operating-system
interfaces needed by a caller. `pdos-zarch` is the active OS selection.
`tso31-sdk-files`, added after the z/PDOS 0.1.0 release source, supplies a narrow
TSO sequential/PDS service for SDK consumers. Its TSO31/64 file checks ran on
z/OS 1.5; TSO24 dataset I/O, positioning and broader services remain open.
Retained historical configurations have their own limits.

- [Use and prepare a profile](doc/user/README.md)
- [Architecture: portable C, startup and native services](doc/architecture/README.md)
- [Profiles across the toolchain](../doc/PROFILES.md)
- [Known issues and qualification gaps](doc/BACKLOG.md)
- [Licence and inherited notices](LICENSE), [origins](UPSTREAM.md), [agent rules](AGENTS.md)

The maintained implementation is `src/`. `src/profiles/` selects modules;
`src/interfaces/` contains independently authored, selected macro interfaces.
Preparation uses those files directly and records their identities. Optional
frozen `archive/` material is reference only, not a build dependency.
