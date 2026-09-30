# Mainframe Classic Assembler

The first implementation is an original bootstrap assembler. Follow the root
guide and docs/ARCHITECTURE.md, docs/INTERFACES.md and docs/USER.md.

Public contracts are in include/mf_classic.h. Changes must remain C89 and must
be coordinated with provider, engine and writer users. Core modules do not
open files, allocate from a host heap or terminate a process. The host driver
owns storage and devices. Machine encoding is independent of source parsing.

Reject unsupported syntax, profiles, object widths and macro features explicitly.
Never truncate a name, wrap an operand or manufacture a successful deck after
an error. Test independent expected bytes and record semantics as well as
capacity, changed replay and I/O failures. No guest run is required to qualify
this initial host component slice.
