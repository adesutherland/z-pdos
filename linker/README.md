# Mainframe Classic Linker

Classic Linker, command `mf-classic-ld`, is our maintained descendant of PDLD.
It resolves symbols and relocations in supported classic mainframe objects
and writes flat images, native MVS load modules and XMIT transports. The
z/PDOS build uses it for PLOAD, the kernel and PCOMM; the ELF SDK also uses its
selected native-format route.

The linker preserves PDLD's public-domain declarations and contributor
notices, including Paul Edwards's credited encoding routines. Original
changes, integration and documentation have their stated MIT scope.

The maintained route handles the reached multi-section object layouts and
relocations, including section-origin normalization. A flat image, a load
module and an XMIT envelope serve different purposes: the envelope transports
native content, while the OS loader reconstructs the executable image.

The host implementation is C99. Its other inherited backends do not inherit
the mainframe route's qualification, and support for these formats does not
make it a complete replacement for every IBM binder function. Native guest
hosting remains separate work.

- [User guide](doc/user/README.md)
- [Architecture and interfaces](doc/architecture/README.md)
- [Development](doc/development/README.md)
- [Known issues and next steps](doc/BACKLOG.md)
- [Origins and contributors](UPSTREAM.md), [licence](LICENSE), [agent rules](AGENTS.md)

`src/` is the maintained implementation; `scripts/` contains recipes run from
the repository root. Generated output belongs under ignored root `build/`.
Optional frozen `archive/` material is reference only and is excluded from
normal builds. Each component has one backlog and records its own evidence.
