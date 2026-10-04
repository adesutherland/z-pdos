# TSO31 entry bridge

This component is an original MIT adapter between a 31-bit TSO entry and an
ELF program. It makes entry state, service callbacks, registers and character
handling explicit so those boundaries can be investigated independently.

The bridge is synchronous and non-reentrant. Its service vector describes a
selected interface; it does not provide every underlying native service or a
complete C runtime. It is separate from both the z/PDOS kernel and PDPCLIB's
profile-selected native support.

Recorded host checks cover source/deck correspondence, target layouts and
entry-only linking. They do not establish complete runtime linking or guest
execution. For maintained application packages, use the separate
[Mainframe ELF SDK](https://github.com/adesutherland/mainframe-elf-sdk) and its
own release/profile evidence rather than treating this experimental bridge as
a ready-to-use SDK.

- [User guide](doc/user/README.md)
- [Architecture and interfaces](doc/architecture/README.md)
- [Development](doc/development/README.md)
- [Known issues and next steps](doc/BACKLOG.md)
- [Origins and contributors](UPSTREAM.md), [licence](LICENSE), [agent rules](AGENTS.md)

`src/` is the maintained implementation; `scripts/` contains recipes run from
the repository root. Generated output belongs under ignored root `build/`.
Optional frozen `archive/` material is reference only and is excluded from
normal builds. Each component has one backlog and records its own evidence.
