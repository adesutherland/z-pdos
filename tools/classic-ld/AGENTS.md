# Mainframe Classic Linker guide

The root `AGENTS.md` applies. Work on this component in `tools/classic-ld/`.
The command is `mf-classic-ld`; PDLD is its inherited implementation lineage.
Keep the naming and attribution together.

- Read `SOURCES.md`, `docs/ARCHITECTURE.md` and `docs/QA.md` before changing the
  reader or writer. Preserve upstream public-domain notices and the unchanged
  upstream `readme.txt` and makefiles.
- The imported baseline is public PDLD revision
  `a65eddb9ef4b27a6844f2857db0c98696137612b`, followed by the five retained SDK
  patches in `provenance/`. The two hash manifests distinguish those baselines
  from maintained product changes.
- Keep original MIT changes identifiable. Do not describe the inherited
  implementation as an original C89 linker. Its host build uses C99; write new
  helpers and fixture checks in portable C89/C90 where practical. Scope legacy
  build exceptions to this component and retain the checker and assembler's
  strict flags.
- Generate small original object fixtures and independently expected results.
  Keep native reference objects, private macro expansions, guest assets and
  detailed local package receipts outside Git.
- Qualify changes on the affected reader/linker/writer route. Reuse unchanged
  test results; repeat checks when code or a concrete concern warrants it.
  A host package audit is not guest or whole-toolchain qualification.
- New retained development orchestration uses cREXX. Do not commit, push,
  publish, alter external originals or start guests without session authority.
