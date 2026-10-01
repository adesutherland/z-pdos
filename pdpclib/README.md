# PDPCLIB in z/PDOS

PDPCLIB is Paul Edwards's Public Domain C Library. I maintain our copy and
library fixes here, alongside the classic tools and future OS source. This is
our source of truth. Mainframe Lab retains upstream snapshots and experiments.
The modern compiler SDK is a consumer: future library/linker changes belong
here and move into the SDK after their artifact qualification. Existing SDK
copies retain their recorded identities until that transition is qualified.

The initial import preserved the complete 288-file PDPCLIB subtree at PDOS mirror
revision `0fe81209e78d022b40301f86f97c7f4d3e406d0a`: C source, headers,
native assembler, source-owned macro members, original examples and build
recipes. Those historical recipes are retained as upstream material; they are
not all supported build commands here. See [sources](SOURCES.md),
[licence](LICENSE), [changes](CHANGES.md) and [agent guidance](AGENTS.md).
[Known issues](KNOWN-ISSUES.md) retain concrete concerns found during host QA.
The selected runtime files now include canonical SourceForge upstream changes
from the repaired PDIO1 input, together with our existing MVS fixes. OS and
application profiles consume this same source tree. Git preserves the earlier
qualified input; there is no second frozen runtime directory.
PDPCLIB's original version text is `4.xx`; this import is not a release 4.00.

Portable C library code lives in files such as `stdio.c`, `stdlib.c` and
`string.c`. The native support files bridge that code to particular systems.
`mvssupa.asm` supplies MVS services; `cmssupa.asm`, `sapsupa.asm` and the other
native files have separate system roles. Imported source-owned macros retain
their original notices. Calls to external IBM service and mapping macros do
not supply those macro implementations or grant rights to them.

## Maintained source and profiles

Shared fixes are applied directly to the source files here. The deltas in
`fixes/` record their origin; do not reapply them to corrected source.
Deliberate omissions and target-specific changes live under `profiles/`.

| Profile | Selection and status |
| --- | --- |
| `mvs38-s370-24` | First complete target: MVS 3.8, real System/370, GCC calling convention, AMODE 24/RMODE 24. The explicit PDPTOP member exists; complete source/service assembly and execution remain open. |
| `tso31-lean` | Existing S390/31-bit TSO service variant with the unused prefix parser omitted. Its five-call subset is distinct from the full library. |
| `tso-zos15-24` | z/OS 1.5 compatibility recipe for a 24-bit caller, using temporary AM31 for SWAREQ. Its retained service member selects AMODE 31/RMODE ANY; final module mode and load/execution qualification are separate. Requires a later machine and cannot be used for the real S/370 target. |
| `pdos390-esa` | Retained ESA/390 OS configuration, with the two low-core equates from the Lab native-build work. |
| `pdos-zarch` | Active repaired OS configuration, with OS=PDOS, inherited S380 application/ZARCH OS switches and low-core equates. These inherited switches do not imply a community S/380 application ISA or ABI. |

The top-level imported `pdptop.mac` retains its upstream S380/ZARCH defaults.
It must not silently become the selected MVS 3.8 configuration. Preparation
uses an explicit profile and records the resulting source identities.

From the repository root, prepare a new owned build directory:

```sh
crexx tools/prepare-pdpclib.crexx --args mvs38-s370-24 build/pdpclib-mvs38
```

The recipe copies the maintained library, selects the profile's PDPTOP member
and applies only that profile's optional native delta. It rejects an existing
output directory. The resulting `profile-source.sha256` records the selected
native source, member and changed C files. Preparation does not assemble source
or acquire an external macro library.

## Qualification

Focused host tests cover changed C behavior, profile preparation and the three
already qualified source-owned macro definitions. Full MVSSUPA now assembles
under the named PDOS configuration and selected original interfaces; other
whole-library profiles retain separate assembly and service gates. No IBM macro
library or guest object is part of this import.

The root CMake build includes these focused checks. See the
[C test scope](tests/README.md); the profile recipe can also be checked with
`crexx tools/check-pdpclib-profiles.crexx --args build/pdpclib-profile-qa`.
The existing [macro consumer](../tools/check-pdpclib-macros.crexx) now uses
the maintained native source identity; its selected definitions are unchanged.

The `@@PCLST` address defect reproduced during the entry audit belongs to
PDLD's nonzero-section-origin handling, maintained in
[Mainframe Classic Linker](../tools/classic-ld/README.md), command `mf-classic-ld`.
Retaining the parser is essential to
test the full library; the optional lean omission does not fix relocation.
Library assembly, linker correctness, guest execution and native tool hosting
remain separately named gates. See the [entry audit](../runtime/tso31/CHECKPOINT.md)
and the [assembler plan](../tools/classic-as/docs/PLAN.md).
