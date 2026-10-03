# PDPCLIB user guide

The library is maintained in src/ and consumed by selected target builds.
Focused host checks use CMake, Clang/GCC and cREXX:

```sh
crexx -nokeep pdpclib/scripts/build.crexx --args test
crexx -nokeep pdpclib/scripts/prepare.crexx --args pdos-zarch build/my-runtime
crexx -nokeep pdpclib/scripts/build-native.crexx --args pdos-zarch build/tools/assembler/mf-classic-as build/my-native-runtime
```

Preparation refuses an existing output and writes a source-identity receipt.
It copies maintained source, selects the profile's PDPTOP member and joins its
MVSSUPA source modules; it does not apply patches. Source generation is not
assembly, linking or guest qualification.

The native recipe uses the source-built Classic Assembler and the maintained
PDOS31 service and linkage macros. It requires a new output directory and
writes a source/object SHA-256 receipt. The `pdos-zarch` source now assembles
as a classic object deck; that result has not been linked or newly run in a
guest. The same recipe accepts `tso31-lean` to expose its historical assembly
boundary. For the SDK's documented file subset, run it with
`tso31-sdk-files`. This selection assembles a complete TSO service object
using source-owned TSO terminal, EXTRACT and control-block forms. It keeps
sequential and partitioned dataset paths and explicitly rejects VSAM,
IDCAMS and supervisor-mode switching. Assembly and host linking have passed;
z/OS guest behavior remains unqualified. Other PDOS31 macro interfaces
used by this object are not thereby proven on z/OS.

| Profile | Service/configuration scope |
| --- | --- |
| mvs38-s370-24 | MVS 3.8, historical System/370, AMODE24/RMODE24 |
| tso31-lean | Existing limited TSO service subset, with unused prefix parser omitted |
| tso31-sdk-files | SDK TSO service with sequential/partitioned dataset paths and explicit VSAM rejection; guest checks open |
| tso-zos15-24 | z/OS 1.5 compatibility for a 24-bit caller; its SWAREQ path temporarily uses AM31 |
| pdos390-esa | Retained ESA/390 OS configuration |
| pdos-zarch | Active z/PDOS configuration with standard z/Architecture support |

The inherited S380/ZARCH switch names are configuration history, not proof
of a community S/380 ISA/ABI. Only the recorded selected z/PDOS route has its
0.1 guest acceptance. [Architecture](../architecture/README.md),
[host test scope](../development/TESTS.md) and [backlog](../BACKLOG.md) explain
other retained ports and service limits. Unsupported historical build recipes
are preserved in the frozen upstream archive.
