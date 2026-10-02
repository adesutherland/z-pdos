# Classic C architecture

`../../src/` contains the inherited GCC configure/build tree, its i370 backend
and the original host launcher `driver.c`. The backend is in
`src/gcc/config/i370/`; the launcher runs its sibling xgcc/cc1 and preserves
the existing compile-to-text interface.

The current C data model uses 32-bit int/long/pointers and EBCDIC characters.
GCC/cc370 lineage and repairs are in [UPSTREAM.md](../../UPSTREAM.md).
The [shared machine direction](../../../machines/doc/MACHINE-PROFILES.md)
does not imply object-format, ABI or runtime compatibility.
