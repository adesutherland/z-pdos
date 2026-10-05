# z/PDOS documentation

Read the [user guide](user/README.md) to boot the image, then the
[architecture](architecture/README.md) to understand what happens between IPL
and an application's service call.

- [Architecture](architecture/README.md): boot, C32/64-bit boundaries, storage,
  program loading, datasets and terminal services.
- [Source-to-image dependencies](architecture/DEPENDENCIES.md): selected inputs,
  the three linked programs and disk construction.
- [Development guide](development/README.md): source map and change contracts.
- [Build contract](development/BUILD-CONTRACT.md): recorded acceptance stages.
- [0.1 guest qualification](qualification/QUALIFICATION.md): exact historical
  machine, inputs, outcomes and limits.
- [Stage 3 CMS qualification](qualification/STAGE3-2026-10-05.md): bounded
  unchanged CMS31 and CMS24 MODULE results from current source.
- [Backlog](BACKLOG.md): the single current defect and development queue.
- [AI guidance](ai/README.md): component guidance for automated work.

The [repository documentation](../../doc/README.md) covers host packages,
profiles, licences and the shared workflow. Current guides explain the
maintained design; dated records remain evidence for their named inputs.
