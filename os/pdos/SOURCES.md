# PDIO1 source identity and ownership

1 October 2026. I selected the repaired PDIO1 baseline used for today's
cREXX qualification as our OS rebuild target. Each product has one maintained
source version. We merged relevant runtime fixes into PDPCLIB and removed the
duplicate frozen runtime; Git preserves the earlier qualification input.

| Input | Identity and disposition |
| --- | --- |
| Canonical upstream | Paul Edwards's [SourceForge PDOS](https://sourceforge.net/p/pdos/gitcode/ci/a65eddb9ef4b27a6844f2857db0c98696137612b/tree/), revision `a65eddb9ef4b27a6844f2857db0c98696137612b` |
| Verified tracked-file archive | `pdos-sourceforge-a65eddb9.tar.gz`, SHA-256 `4bed44b93fbb09f0f40442e29ab6073ced049a1ce4659a341cf89eb16f17ad93`; selected files checked against both archive and Git objects |
| Original qualified source checkpoint | [`d62b109`](https://github.com/adesutherland/z-pdos/commit/d62b109ed986bd455732484173ff4ebe0533045a), exact 37 OS and 44 runtime inputs, 1,396,526 bytes |
| Maintained OS | `os/pdos/source/s370/`; all 37 files remain identical to that qualified checkpoint |
| Maintained runtime | Shared `pdpclib/`, selected by [runtime-inputs.txt](runtime-inputs.txt), with `pdpclib/profiles/pdos-zarch/pdptop.mac` |
| Current selected input | 81 files, 1,400,063 bytes; a new runtime/build candidate |
| Original repair series | [Seven retained patches](patches/series.txt), individually identified by [original.sha256](patches/original.sha256); provenance records, not additional source versions |
| Accepted native kernel | PDIO1 strict RDW SHA-256 `1fcae79b32314adce6838498e60760bfdc612d759c46fb025635073f960ca199`; native bytes and listings remain private |
| 1 October guest acceptance | Fresh cREXX beta 3 at commit `ae1607b8e145174422cee7f3e73fbcc37a65226c`; managed image label `beta3-ae1607b8e145-20261001`, unchanged PDIO1 kernel |

The original native repair series applied PDOS390 0001 (multi-CSECT loader
and PDLD repair), PDOS390 0002 (native-build compatibility), then PDOS64
0001–0005 (OS switches, full-width context, native services, high loading,
checked DASD writes and console wrapping). Replaying that series reproduced
the original qualified selection. The exact source and manifest remain in
commit `d62b109`; no second maintained runtime is needed to preserve them.

The working OS recipe now uses the same PDPCLIB as other consumers. Nineteen
shared files gain the canonical runtime changes, with existing MVS DSCB/PDSE
and DYNAL-pointer fixes retained. The merged STDIO also guards its write-only
MVS `w+b` fallback against read/update initialization. [PDPCLIB's change
record](../../pdpclib/CHANGES.md) identifies the integration and tests. The
explicit `pdos-zarch` profile changes configuration, not the shared C source.
This candidate does not inherit the old native whole-source or guest claim.

The consolidated recovery patch produces the current maintained selection
from the hash-checked canonical upstream. It includes the five original OS
file deltas and current changes to `mvssupa.asm`, `pdptop.mac`, `stdio.c`
and `string.h`. Original PDLD hunks remain historical attribution; maintained
linker changes belong to `tools/classic-ld/`, with no linker copy here.

[UPSTREAM.txt](UPSTREAM.txt) preserves the upstream project notice. OS and
runtime sources retain their inherited dedication, fallback permission and
contributor notices. Original project repairs, orchestration, tests and
documentation use the root MIT grant; it does not relicense inherited material
or external IBM macros. No native object, expansion, listing, executable,
private disk, package, guest log or correspondence is imported.

The earlier Lab report recorded an outer archive checksum `4a0b3e…`.
The retained archive and its contemporaneous Git-source manifest instead
agree on `4bed44…`. We pin verified Git file objects and selected archive
bytes rather than equating those differently recorded archive identities.

[source.sha256](source.sha256) identifies the current repository inputs.
[upstream.sha256](upstream.sha256) and [prepared.sha256](prepared.sha256)
use the generated `s370/` and `pdpclib/` layout. The current manifests and
recovery patch are checked by the retained recipes; earlier versions live in
Git. [CHECKPOINT.md](CHECKPOINT.md) records the current host results and the
separate historical guest acceptance.
