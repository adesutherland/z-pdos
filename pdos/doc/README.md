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
- [3270 family coverage contract](architecture/CONSOLE-STANDARD.md) and
  [Workbench ABI](architecture/CONSOLE-WORKBENCH-ABI.md): shared sessions,
  retained presentation, input ownership and capability boundaries.
- [Console stage 3 checkpoint](qualification/CONSOLE-STAGE3-2026-10-09.md):
  21 passing guest runs/333 checks for the bounded console/text/printer/DASD
  increment; full provider/conformance closure and the 0.2.1 release remain open.
- [Source-to-image dependencies](architecture/DEPENDENCIES.md): selected inputs,
  Classic/GNU links and disk construction.
- [Application command environments](architecture/COMMAND-ENVIRONMENTS.md)
  and [review/freeze record](qualification/COMMAND-ENVIRONMENTS-2026-10-10.md):
  shared U/C dispatch and the bounded CMS31/TSO31/PDOS client increment.
- [Card-reader batch jobs and 3270 job forms](architecture/BATCH-JOBS.md):
  selected console-preserving design; [PD-033](BACKLOG.md#pd-033-card-reader-batch-jobs-and-3270-job-forms)
  owns the staged implementation and qualification gates; the checkpoints
  below distinguish the successive delivered subsets from later forms work.
- [Submit and review batch output](user/BATCH.md): current source-development
  command sequence, output HOLD versus execution RUNHOLD, actual job identities,
  retained step/DD output and printer recovery.
- [Initial batch contracts and reader/printer proof](qualification/BATCH-STAGE1-2026-10-13.md):
  locally qualified Stage 1 neutral devices on model 2 and plain line consoles;
  native prebound DD/batch runtime, JCL and forms remain later stages.
- [Serial jobs, spool and operator recovery](qualification/BATCH-STAGE2-2026-10-14.md):
  Stage 2's descriptor service, 93 guest checks, printer retry and interrupted-job recovery; current console preserved.
- [First native batch DD checkpoint](qualification/BATCH-STAGE3-2026-10-14.md):
  prebound bindings, native BSAM input/SYSOUT and nested DD ownership pass 52
  local checks.
- [Native datasets, concatenation and temporary data](qualification/BATCH-STAGE3-DATA-2026-10-14.md):
  that Stage 3 increment passes 99 checks.
- [Shared Stage 3 batch package](qualification/BATCH-STAGE3-PACKAGE-2026-10-14.md):
  accepted native z/OS and PDOS results for the pinned common SDK/cREXX package.
- [Stage 4 JCL contract](architecture/BATCH-STAGE4-JCL.md) and
  [current qualification](qualification/BATCH-STAGE4-2026-10-15.md): common
  procedures and console/card submission, accepted on native z/OS and both
  PDOS console routes.
- [Stage 5 form contract](architecture/BATCH-STAGE5-FORMS.md) and
  [qualification](qualification/BATCH-STAGE5-2026-10-15.md): Submit, Jobs and
  Output over the same service; 212 checks across colour models 2–5, model-2
  monochrome and physical line input, including reconnect and native ownership.
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
