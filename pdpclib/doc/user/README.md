# PDPCLIB user guide

The library is maintained in src/ and consumed by selected target builds.
Focused host checks use CMake, Clang/GCC and cREXX:

```sh
crexx -nokeep pdpclib/scripts/build.crexx --args test
crexx -nokeep pdpclib/scripts/prepare.crexx --args pdos-zarch build/my-runtime
```

Preparation refuses an existing output and writes a source-identity receipt.
It copies maintained source, selects the profile's PDPTOP member and joins its
MVSSUPA source modules; it does not apply patches. Source generation is not
assembly, linking or guest qualification.

| Profile | Service/configuration scope |
| --- | --- |
| mvs38-s370-24 | MVS 3.8, historical System/370, AMODE24/RMODE24 |
| tso31-lean | Existing limited TSO service subset, with unused prefix parser omitted |
| tso-zos15-24 | z/OS 1.5 compatibility for a 24-bit caller; its SWAREQ path temporarily uses AM31 |
| pdos390-esa | Retained ESA/390 OS configuration |
| pdos-zarch | Active z/PDOS configuration with standard z/Architecture support |

The inherited S380/ZARCH switch names are configuration history, not proof
of a community S/380 ISA/ABI. Only the recorded selected z/PDOS route has its
0.1 guest acceptance. [Architecture](../architecture/README.md),
[host test scope](../development/TESTS.md) and [backlog](../BACKLOG.md) explain
other retained ports and service limits. Unsupported historical build recipes
are preserved in the frozen upstream archive.
