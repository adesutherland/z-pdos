# z/PDOS architecture

The maintained C32 kernel and native support live in ../../src/. The OS uses
PDPCLIB's explicit pdos-zarch service configuration; no runtime source copy is
maintained inside this component. [DEPENDENCIES.md](DEPENDENCIES.md) identifies
the selected source/service interfaces and their coverage. The
[build contract](../development/BUILD-CONTRACT.md) separates compile, assembly,
link/load, media and guest gates. [Machine setup](../../../machines/profiles/pdos-zarch.md)
records hardware separately from C width, addressing modes and services.

The current kernel/loader are AMODE31/RMODE24; PCOMM is AMODE31/RMODE ANY.
Support code dispatches qualified native 64-bit applications. A full-width C
kernel and 24-bit application loading remain distinct backlog work.
