# PDIO1 source identity and ownership

1 October 2026. I selected the repaired PDIO1 baseline used for today's
cREXX qualification as the OS source to preserve and rebuild here.

| Input | Identity and disposition |
| --- | --- |
| Canonical upstream | Paul Edwards's [SourceForge PDOS](https://sourceforge.net/p/pdos/gitcode/ci/a65eddb9ef4b27a6844f2857db0c98696137612b/tree/), revision `a65eddb9ef4b27a6844f2857db0c98696137612b` |
| Verified tracked-file archive | `pdos-sourceforge-a65eddb9.tar.gz`, SHA-256 `4bed44b93fbb09f0f40442e29ab6073ced049a1ce4659a341cf89eb16f17ad93`; selected files checked against both archive and Git objects |
| Source selection | 37 files from `s370/` and 44 canonical runtime/header/macro inputs; 81 files, 1,396,526 bytes |
| Maintained OS | `os/pdos/source/s370/`, exact qualified bytes at import |
| Qualification runtime | `pdpclib/reference/pdio1/source/`, frozen canonical input selection |
| Original repair series | [Seven retained patches](patches/series.txt), individually identified by [original.sha256](patches/original.sha256) |
| Current accepted kernel | PDIO1 strict RDW SHA-256 `1fcae79b32314adce6838498e60760bfdc612d759c46fb025635073f960ca199`; native bytes and listings remain private |
| 1 October guest acceptance | Fresh cREXX beta 3 at commit `ae1607b8e145174422cee7f3e73fbcc37a65226c`; managed image label `beta3-ae1607b8e145-20261001`, unchanged PDIO1 kernel |

The seven patches are applied in this order:

1. PDOS390 0001: multi-CSECT loader and PDLD repairs.
2. PDOS390 0002: standard S/390 native-build compatibility.
3. PDOS64 0001: restore the ZARCH OS configuration.
4. PDOS64 0002: full-width native application context.
5. PDOS64 0003: native PDS and cREXX application services.
6. PDOS64 0004: RMODE64 high application loading.
7. PDOS64 0005: checked DASD writes and console wrapping.

Only eight selected files differ from the pinned upstream: `pdos.c`,
`pdossup.asm`, `pdosutil.c`, `pdosutil.h`, `pload.c`, and the runtime's
`pdptop.mac`, `stdio.c`, `string.h`. Replaying the original series with its
two PDLD prerequisites reproduced all 81 selected files exactly. Original
PDLD hunks remain attributed history; maintained linker changes belong to
`tools/classic-ld/`. The consolidated recovery patch contains only the eight
OS/runtime file deltas and needs no linker copy.

[UPSTREAM.txt](UPSTREAM.txt) preserves the upstream project notice and
description. The actual OS sources retain Paul Edwards's public-domain
notices. The frozen runtime retains its dedication, fallback permission and
individual contributor notices. Original Lab/project repairs, orchestration,
tests and documentation use the root MIT grant; that grant does not relicense
the inherited source or an external IBM macro library.

The original 362-file Lab replay included the whole 324-file canonical
PDPCLIB subtree. We retain the 13 runtime C units, all 21 headers, four
startup/support sources, four source-owned macros and two runtime documents
needed for the selected build inputs. Other ports/build files are outside
this selection. The upstream `s370/saved.log` is also excluded. No saved
guest log, IBM macro implementation, native expansion, listing, object,
executable, cREXX package, disk or correspondence is imported.

The earlier Lab report recorded an outer archive checksum `4a0b3e…`.
The retained archive and its contemporaneous Git-source manifest instead
agree on `4bed44…`. This import pins Git file objects and verified selected
archive bytes, rather than equating differently recorded archive identities.

`source.sha256` uses repository-relative maintained paths. `upstream.sha256`
and `qualified.sha256` use the original `s370/` and `pdpclib/` layout in a
prepared or recovered source directory. The [checkpoint](CHECKPOINT.md)
records manifest and recovery-patch identities.
