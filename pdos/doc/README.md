# z/PDOS documentation

Read the [user guide](user/README.md) to boot the image, then the
[architecture](architecture/README.md) to understand what happens between IPL
and an application's service call.

- [Architecture](architecture/README.md): protected K64/C31, shared U,
  ownership, native loading, storage, file and terminal services.
- [K/U contract](architecture/TWO-SPACE-POC.md) and
  [ABI inventory](architecture/TWO-SPACE-ABI.md): design decisions, native
  entry/service forms and the separately labelled development checkpoints.
- [P0–P6 qualification](qualification/TWO-SPACE-P6-2026-10-07.md): completed
  normal-image workload, media, console and lifecycle results.
- [0.2.0 release record](qualification/RELEASE-0.2.0.md): candidate review,
  build, operator acceptance and publication status.
- [Source-to-image dependencies](architecture/DEPENDENCIES.md): selected inputs,
  Classic/GNU links and disk construction.
- [Development guide](development/README.md): source map and change contracts.
- [Build contract](development/BUILD-CONTRACT.md): recorded acceptance stages.
- [0.1 guest qualification](qualification/QUALIFICATION.md): exact historical
  machine, inputs, outcomes and limits.
- [Stage 3 CMS qualification](qualification/STAGE3-2026-10-05.md): bounded
  unchanged CMS31 and CMS24 MODULE results from current source.
- [0.1.1 local operator acceptance](qualification/0.1.1-OPERATOR-UAT-2026-10-05.md):
  managed guest, CMS/TSO cREXX, CKD/tape and shutdown checks on the repaired
  candidate; separate from a tagged or published release.
- [Backlog](BACKLOG.md): the single current defect and development queue.
- [AI guidance](ai/README.md): component guidance for automated work.

The [repository documentation](../../doc/README.md) covers host packages,
profiles, licences and the shared workflow. Current guides explain the
maintained design; dated records remain evidence for their named inputs.
