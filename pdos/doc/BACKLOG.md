# z/PDOS backlog

This is the only live roadmap, defect and qualification queue for this component.
Follow [the shared workflow](../../doc/WORKFLOW.md). Items distinguish
implemented behavior, recorded qualification and source-review findings. An open source-review item does not imply a reproduced guest failure.

## Ordered delivery roadmap

I want the operator route to lead the work: load a program, run a script against
it and see whether it conforms. Moving fixtures in and out comes next, with
CMS and TSO record and naming conventions handled explicitly. Then I want
z/PDOS to run the CMS cREXX programs as well as the already qualified TSO
ones. These are ordered outcomes, not claims that the current release provides
them or authorisation to implement every backlog item.

| Order | Outcome and first checkpoint | Items |
| --- | --- | --- |
| 1 | **Load, run and judge.** Start with an end-to-end TSO31 or TSO64 cREXX check from the base image using unchanged native program bytes. Make program installation, native batch delivery, per-command results and cleanup one repeatable operator flow. A human must also be able to enter commands longer than the 80-column 3270 field without silent truncation, read a stable prompt and input line, and tell which command produced each result. Bound executable reads to their dataset extent before accepting arbitrary staged packages. | [PD-013](#pd-013-repeatable-program-conformance-run), [PD-009](#pd-009-temporary-package-lifecycle), [PD-004](#pd-004-batch-file-delivery), [PD-005](#pd-005-command-length-diagnostics), [PD-007](#pd-007-results-and-scrollback), [PD-008](#pd-008-operator-diagnostics), [PD-006](#pd-006-console-input-and-prompts), [PD-012](#pd-012-direct-executable-reads-need-an-extent-boundary) |
| 2 | **Move fixtures both ways.** Make a second CKD volume discoverable, mountable and selectable while protecting IPL. Let the operator allocate a bounded output dataset on the selected exchange volume. Expose sequential tape record I/O using unchanged mounted images. Import/export CMS and TSO fixtures with explicit naming, framing and encoding, then check exact guest results. | [PD-018](#pd-018-multiple-dasd-volumes), [PD-020](#pd-020-safe-guest-dataset-allocation), [PD-019](#pd-019-virtual-tape-io), [PD-014](#pd-014-cms-and-tso-fixture-exchange) |
| 3 | **Run CMS cREXX binaries.** The current source's checked unchanged-MODULE route passed [local guest qualification](qualification/STAGE3-2026-10-05.md): CMS31 RXVM and a fresh RXC → RXAS → RXVM chain, plus the separate fixed-origin CMS24 RXVM IO24 subset. This does not qualify native TSO24 or the full historical CMS24 compiler chain. A TSO build of the same source does not establish CMS binary compatibility. | [PD-015](#pd-015-cms-crexx-binary-contract), [PD-016](#pd-016-cms31-crexx-execution), [PD-017](#pd-017-cms24-crexx-execution), [PD-001](#pd-001-amode24rmode24-application-loading) |
| Supporting gates | Qualify truthful invalid storage requests before widening the service surface, and correct region-table initialization before expanding mappings. Stage 3 measured less than 1 MiB of low heap headroom after the initial kernel allocation. The two-space contract and diskless machine proof address the first design gate; a booted redesign still needs its own build and guest qualification. | [PD-002](#pd-002-conditional-storage-service-errors), [PD-011](#pd-011-region-first-table-padding), [PD-003](#pd-003-two-space-supervisor-and-shared-application-memory) |

The current TSO31, TSO64 ANY and TSO64 HIGH guest results can be reused for
unchanged bytes and services. A new installer, fixture path, loader or CMS
service needs its own affected host, disk and guest checks. The Lab's private
stopped-disk tools and qualification scripts are useful prototypes; the
supported operator path belongs with this product and must not depend on
private paths, accounts or image versions.

Step 1 is implemented, locally guest-qualified and manually accepted in the
current source; see
the [operator guide](user/CONFORMANCE.md) and
[4 October evidence](qualification/STEP1-2026-10-04.md). Order 2 is implemented
and locally guest-qualified in the bounded source profile; see the
[media guide](user/MEDIA.md), [fixture guide](user/FIXTURES.md) and
[phase 2 evidence](qualification/PHASE2-2026-10-04.md). Order 3 is implemented
and locally guest-qualified for the bounded CMS31 and CMS24 workloads recorded
in the [stage 3 result](qualification/STAGE3-2026-10-05.md). These routes are
in the 0.1.1 release; the
[operator acceptance](qualification/0.1.1-OPERATOR-UAT-2026-10-05.md)
names the separate locally tested image. Native TSO24 and broader CMS services
remain separate work.

### Phase 2 delivery contract

1. **DASD discovery and selection.** `DEVICES` shows attached channel addresses
   without treating every address as CKD. `MOUNT address expected-volser` reads
   the real VOL1 label and refuses a mismatch or duplicate; `VOLUMES` shows
   mounted volumes; `SELECT volser` changes dataset and executable lookup.
   IPL stays protected and selected by default. An allocated DD pins its
   device; `UNMOUNT` refuses the IPL, selected and open volumes, and releases
   closed DD bindings. The first implementation supports four CKD volumes
   with LIFO unmount.
2. **Guest output allocation.** `ALLOC` creates a named sequential
   dataset on the selected exchange volume from an explicit record format,
   logical/block length and bounded space request. Refuse IPL, duplicates,
   unsupported formats, insufficient free space and invalid VTOC state
   without publishing a partial dataset. Report the chosen extent and
   return code. Existing-dataset `COPY`, record-preserving `RCOPY`, and native
   programs can then use it. This first allocator is confined to the
   100-cylinder `dasdload` 3390 exchange layout without a format-5 free map.
3. **Tape record I/O.** An operator attaches an unchanged AWS or HET image to
   Hercules, registers its device read-only with `TAPE MOUNT address`, and
   reads physical records through `TAPE READ` or an allocated `TAP:address` DD.
   Output uses a separate, explicitly writable tape image. Raw record,
   file-mark, rewind and output round trips have local guest evidence, as
   do short native `TAP:` read and write transfers. Larger native transfers
   and release-image interpretation still need their own checks;
   the container alone does not identify CMS or z/OS logical contents.
4. **Fixture interpretation and proof.** A manifest pins archive and image
   SHA-256, medium, format, logical name, source/destination, record length,
   code page and expected record/byte hashes. VMFPLC2 CMS HET and z/OS
   standard-label AWS are distinct adapters. Preserve their original tape
   bytes in the release ZIP. Stage disposable output on exchange CKD, run the
   checked mount/allocation/record-copy list, then export and compare logical
   records and exact binary bytes. Run an unchanged native program separately
   where its binary contract is supported. CMS MODULE execution has its
   separate order 3 qualification; a tape read alone is transport evidence,
   not a CMS or z/OS program pass.

## PD-001: AMODE24/RMODE24 application loading

- Type: defect
- Status: Open
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: TSO24 is rejected before dispatch; z/PDOS 0.1 qualifies the wider application routes.
- Evidence: doc/qualification/QUALIFICATION.md and qualification-0.1.json.
- Acceptance: Place code/save areas/parameters below 16 MiB, dispatch in the declared mode and qualify unchanged TSO24 RXVM I/O with its separate storage budget.

## PD-002: Conditional storage-service errors

- Type: qualification
- Status: Open
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: Successful GETMAIN/FREEMAIN paths do not establish all invalid-request results.
- Evidence: The existing storage-service and guest checks.
- Acceptance: Check invalid requests and truthful failures without weakening unchanged-binary acceptance.

## PD-003: Two-space supervisor and shared application memory

- Type: improvement
- Status: Open
- Target: pdos-zarch nucleus, C31 supervisor services and shared AMODE24/31/64 application space
- Observation: The current C32 kernel runs AMODE31 below 16 MiB with a one-MiB linked image slot. Its `PDOS` structure, mostly eager DAT and address-space tables, is 8,802,304 bytes; the initial aligned allocation leaves at most 630,784 bytes before the 12 MiB below-line application pool. Stage 3 exposed an overlap between the CMS24 fixed-origin image and its inherited low stack; the bounded bridge moves that stack to the reserved 15–16 MiB range. The image also covers PLOAD's 1 MiB boot heap. An initial 0.1.1 operator candidate returned from PCOMM but failed in PLOAD's C exit; the repaired z/Architecture kernel reaches its own `0444` wait after DAT is off. A trial with an 8 MiB heap start and 10 MiB application-pool start booted PCOMM but stalled at CMS24 execution; its initial kernel allocation could not fit in that gap, so the trial did not isolate the image-slot limit. Existing 31/64 application loads use above-line storage, but GETMAIN can still use or fall back to `btlmem`. Dropping CMS24 alone would not remove that dependency or bound the kernel heap. Kernel growth requires an independently backed K home for its body and bulky structures, with the real interruption island and 24-bit channel buffers handled separately. Moving the current linked image wholesale would put its 24-bit channel addresses and startup assumptions out of contract.
- Evidence: Machine contract, existing 0.1 qualification, and the [stage 3 guest record](qualification/STAGE3-2026-10-05.md).
- Direction: The [two-space contract and diskless proof](architecture/TWO-SPACE-POC.md) select one protected supervisor ASCE and one shared application ASCE. A small AMODE64 assembler nucleus crosses between them; Classic C services remain C31 in the supervisor space. The machine fixture demonstrates four 24/31/64-bit SVC crossings, distinct same-VA mappings and a fault on an unmapped kernel entry page, with zero kernel pages in the fixture's 24-bit application virtual range. This changes the target design, not the active one-ASCE release kernel.
- Acceptance: Separate the kernel image limit from its heap address, reserve independently backed K storage for DAT and kernel structures, eliminate PLOAD's boot-heap overlap, and audit 24-bit channel buffers plus GETMAIN low requests and fallback. Extend the real interruption island for every required interruption class; implement full-width context, PSW/key, K/U transition, bounded U-buffer access and C31 service dispatch with Classic tools. Inventory CMS lowcore/API conventions and preserve TSO SVC contracts without consuming unnecessary U low virtual pages. Add collision-checked relocatable/fixed module loading and a nested application-command gate, including a fixed-origin collision control. Qualify fresh IPL, CMS24/CMS31, TSO31/TSO64, low-memory budgets and failure recovery on named images. The Step 2 fixture, stage 3 stack move, low-heap guard and direct shutdown wait are partial evidence, not completion.

## PD-004: Batch-file delivery

- Type: improvement
- Status: Done in 0.1.1
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: The conformance recipe now encodes bounded UTF-8 script commands into IBM1047 raw U/18452 records with hex-15 delimiters. It rejects unsupported manifest geometry and overlong lines, then checks exact batch readback.
- Evidence: [4 October host and guest result](qualification/STEP1-2026-10-04.md); [operator guide](user/CONFORMANCE.md).
- Acceptance: Document framing, diagnose unsupported input and provide a repeatable text-import route.

## PD-005: Command-length diagnostics

- Type: defect
- Status: Done in 0.1.1
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: A trailing `&` joins 3270 field entries at `MORE>` up to 198 characters. Console and batch records beyond that limit are rejected without dispatch.
- Evidence: [PCOMM boundary control](../tests/pcomm-operator.crexx) checks both routes at 198/199 characters; the [guest result](qualification/STEP1-2026-10-04.md) includes a human entry beyond 80 columns.
- Acceptance: Let a human submit a command longer than the 80-column console field through a documented continuation or short-script route, up to the tested command limit. Diagnose an overlong console or batch command before executing any truncated fragment; test both routes at and beyond their limits.

## PD-006: Console input and prompts

- Type: defect
- Status: Done in 0.1.1
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: The 3270 writer now keeps a row and column across fragmented writes, wraps output above the editable field and presents a distinct `MORE>` continuation prompt.
- Evidence: [ASAN/UBSAN console control](../tests/io-repair.c) and the [guest operator result](qualification/STEP1-2026-10-04.md).
- Acceptance: Echo a typed line together, retain one stable prompt and its editable field through normal and long output, and show a distinct continuation prompt when a command spans console entries.

## PD-007: Results and scrollback

- Type: improvement
- Status: Done in 0.1.1
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: External commands now print numbered BEGIN/END markers with RC. The runner requires a fresh prompt, captures a complete ScreenTrace and judges ordered results against the locked manifest; a missing expected output fails.
- Evidence: [4 October fresh and deliberately corrupted trace results](qualification/STEP1-2026-10-04.md).
- Acceptance: Identify each submitted command and its completion code unambiguously through long output, including the command following a failed one. Keep the latest result readable at a fresh prompt and retain a complete trace for automated checks.

## PD-008: Operator diagnostics

- Type: improvement
- Status: Done in 0.1.1
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: The kernel hides expected missing PCOMM `.BAT` allocation probes, while a present invalid dataset and other allocation failures remain visible. `HELP TSO`, `HELP CMS` and `VERSION` explain the command boundary and build identity. The direct loader already announces unsupported addressing/residence.
- Evidence: [4 October guest result](qualification/STEP1-2026-10-04.md), [operator guide](user/README.md) and prior [0.1 qualification](qualification/QUALIFICATION.md).
- Acceptance: Distinguish expected probes and announce unsupported addressing/residence before dispatch.

## PD-009: Temporary package lifecycle

- Type: improvement
- Status: Done for the bounded conformance flow in 0.1.1
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: Preparation creates a fresh disposable candidate from an untouched base, checks it offline, and documents selection, stop, readback and removal. Three local guest candidates were selected only while stopped and removed after returning to the accepted image.
- Evidence: [operator guide](user/CONFORMANCE.md) and [4 October handback](qualification/STEP1-2026-10-04.md).
- Acceptance: Preserve the base OS, leave one working image and remove duplicate temporary installations.

## PD-010: Hosted fresh-image delivery

- Type: qualification
- Status: Done
- Target: Fresh base-OS CCKD delivery built on the Linux GitHub runner
- Observation: The distro converter expands the 100-cylinder disk to the full 1,113-cylinder device by default. Conversion now specifies `-cyls 100` explicitly. The transport checker also retains identical legacy zero serial bytes, while still rejecting non-digit serial changes and all guest-byte changes. The 0.1.1 Linux release job completed source-to-image, loader/dataset/compression checks and image packaging. Packaged host checks do not establish fresh guest execution.
- Evidence: [0.1.1 release run 37308481475](https://github.com/adesutherland/z-pdos/actions/runs/37308481475), source `69b638d38dc4221cbed399d8fde5bf4883673901`, and the published `z-pdos-0.1.1-pdos-image.zip`; [release guide](../../doc/BUILD-AND-RELEASE.md).
- Acceptance: Linux builds the fresh disk from source, passes loader/dataset/compression controls, records its actual utility identities and delivers an archive whose extracted files match their inventory. Keep any new guest qualification explicitly separate.

## PD-011: Region-first table padding

- Type: defect found by source review
- Status: Open; runtime impact not reproduced
- Target: `pdos-zarch` DAT setup
- Observation: In `pdosInitAspaces`, the loop following initialization of `region1[0]` writes invalid entries to `region2[1..511]` again, leaving the corresponding `region1` entries uninitialized. The enclosing `PDOS` allocation uses `malloc`, and `pdosDefaults` does not clear it. The documented low/high windows use the first region-first entry, so this finding does not establish failure of the existing qualified workloads.
- Evidence: [Current source](../src/pdos.c), `pdosInitAspaces`, `main` and `pdosDefaults`; documentation review on 4 October 2026. No code change or execution was performed for this finding.
- Acceptance: Initialize unused entries in the intended table, check table contents independently, and qualify invalid translations outside the mapped region without changing accepted mappings.

## PD-012: Direct executable reads need an extent boundary

- Type: robustness issue found by source review
- Status: Done in 0.1.1
- Target: Disk path in `pdosLoadExe`
- Observation: The direct loader now validates a single selected extent, bounds track reads and requires an EOF before accepting the image. A malformed RXC64 executable without EOF was rejected; a following PCOMM command ran normally.
- Evidence: [Current source](../src/pdos.c), `pdosLoadExe` and [4 October malformed-disk guest result](qualification/STEP1-2026-10-04.md).
- Acceptance: Validate the selected extent, stop reads at its boundary and require the appropriate termination. Preserve valid native loads and reject a missing-EOF image without reading an adjacent dataset.

## PD-013: Repeatable program conformance run

- Type: improvement
- Status: Done in 0.1.1
- Target: z/PDOS operator commands, package staging, PCOMM batch execution and result capture
- Observation: The product-level manifest and cREXX recipe now stage checked native programs, simple VB members and raw batch on a fresh disk, read them back, run a leased 3270 script with live replies, and emit per-command and overall PASS/FAIL. A check may judge a silent command by its RC and may reuse an output phrase in another check because the judge searches each numbered command interval separately. The 0.1.1 base image has no application package or in-guest runner; the host recipe is in the source archive.
- Evidence: [4 October qualification](qualification/STEP1-2026-10-04.md), [operator guide](user/CONFORMANCE.md), and the [product recipe](../scripts/conformance.crexx).
- Acceptance: From an untouched base image, a checked native program package and a declarative test script, provide one documented product-level flow that creates a disposable candidate, loads the program without changing its native bytes, runs ordered commands and reports each command's identity, return code, expected result and overall PASS/FAIL. Verify fresh output rather than accepting stale data; retain input/output hashes and guest evidence in a portable receipt. Demonstrate a positive and an intentionally failing cREXX case, an interactive-input case, a command longer than 80 columns, and cleanup that preserves the base image. Do not require a private Lab path or manual interpretation of a screen snapshot.

## PD-014: CMS and TSO fixture exchange

- Type: improvement
- Status: Done
- Target: z/PDOS host/disk fixture tooling and the selected cREXX record-file contract
- Observation: The product recipe inspects a release ZIP and unchanged tape images to pin SHA-256 and logical-record hashes, strictly rechecks them, decodes bounded CMS VMFPLC2 HET and standard-label TSO AWS records, builds a fresh exchange CKD, runs checked guest `ALLOC`/`RCOPY` commands, and exports exact logical records after shutdown. It keeps CMS file name/type/mode and TSO dataset/label rules separate. The route does not execute CMS MODULEs.
- Evidence: [Phase 2 guest and stopped-disk results](qualification/PHASE2-2026-10-04.md), [fixture guide](user/FIXTURES.md), and `pdos/scripts/fixtures.crexx` with its bounded binary helper. The unchanged beta-3 CMS31 HET `IOBAD CREXX` fixture and a synthetic standard-label AWS both passed exact readback.
- Acceptance: Define a portable fixture manifest with explicit logical name, destination/source, record format and length, text encoding or binary mode, and expected hashes. Import and export the cREXX text, blank/empty, and ordered 00–FF binary cases through the supported CMS and TSO fixture routes and z/PDOS; compare logical records and binary bytes after execution. Create output datasets through PD-020 rather than requiring the operator to prebuild every output slot. Reject unsupported framing, record length, missing datasets and ambiguous conversions without partially accepting a candidate. Check unaffected disk content and keep image changes offline. State any CMS or TSO record forms outside the supported subset.

## PD-018: Multiple DASD volumes

- Type: improvement
- Status: Done
- Target: z/PDOS CKD discovery, mount registry, DD bindings and direct loader
- Observation: Phase 1 bound native datasets and executable loads to the IPL subchannel even though `DIR` already used a separate current-device field. The current phase 2 source provides explicit address discovery, VOL1-checked mount, volume selection and protected unmount, then routes native DD, PDS and executable I/O through the selected or bound device. `ALLOC` now creates bounded sequential outputs on a selected exchange image.
- Evidence: [phase 2 guest and stopped-disk checks](qualification/PHASE2-2026-10-04.md) and [operator guide](user/MEDIA.md).
- Acceptance: On a stopped, disposable two-3390 image, mount a labeled exchange volume without changing the IPL disk; reject absent, wrong-label and duplicate addresses. Select it, list and read its datasets, run a native program or batch from it, write and read back output there, return to IPL, and refuse unsafe unmount. Verify unchanged IPL bytes and complete cleanup. Show accurate prompt and return codes.

## PD-020: Safe guest dataset allocation

- Type: improvement
- Status: Done
- Target: selected exchange CKD volume, VTOC and PCOMM allocation command
- Observation: `ALLOC` now checks the selected non-IPL 100-cylinder exchange volume's VOL1, VTOC layout and occupied extents, initializes EOF before publishing a new sequential DSCB, reads that DSCB back and reports the chosen extent. FB80 and VB outputs then passed guest writes and exact stopped-disk readback. Duplicate, malformed geometry and IPL requests returned RC 8. The inherited `pdosNewF` remains separate.
- Evidence: [phase 2 guest record](qualification/PHASE2-2026-10-04.md), `pdos/src/pdos.c` (`pdosAllocateDataset`), and the [media guide](user/MEDIA.md).
- Acceptance: Offer `ALLOC` with explicit dataset name, PS record format, logical/block length and bounded cylinder request on a selected non-IPL CKD volume. Discover and reserve a real free extent, update the VTOC consistently, initialize an empty dataset, and report volume, extent and return code. Reject duplicate names, unsupported geometry, full or malformed VTOCs, insufficient space, IPL requests and open conflicting DDs before publication. On an injected write failure, leave no discoverable partial dataset. Guest-qualify allocation followed by native program output and stopped-disk byte readback, and show the command in `HELP`. Keep a simple path for the operator while retaining exact geometry in the qualification receipt.

## PD-019: Virtual tape I/O

- Type: improvement
- Status: Done
- Target: Hercules virtual 3420 attachments, z/PDOS raw record service and native `TAP:` DD
- Observation: Inherited `RDTAPE`/`WRTAPE` and old pseudo-dataset names exist, but phase 1 had no mounted tape, no native DD admission and no qualified record/error semantics. The current phase 2 source registers one attached tape read-only by default, reports physical record length/hash/prefix, reads across file marks, writes to a separate explicitly writable image, and admits a matching `TAP:` DD. The inherited `TAV:` path is outside this delivery.
- Evidence: [phase 2 guest and stopped-tape checks](qualification/PHASE2-2026-10-04.md) and [operator guide](user/MEDIA.md). Short native `TAP:` read and write transfers passed; larger application records remain unqualified.
- Acceptance: Mount unchanged checked HET and AWS images on a leased guest, read records across file marks with explicit rewind and error/EOF behavior, compare guest observations with host records, and reject wrong/unregistered devices. On a separate writable output image, write records and file marks, stop the guest, export and compare exact records and hashes. Do not claim VMFPLC2 or standard-label interpretation from raw tape transport alone.

## PD-015: CMS cREXX binary contract

- Type: qualification
- Status: Done
- Target: unchanged CMS24 and CMS31 cREXX MODULE packages and the z/PDOS loader/service boundary
- Observation: The 0.1.1 source pins the unchanged beta 3 CMS24/CMS31 MODULE bytes, record and relocation contracts, and distinct CMS service surfaces. Checked exchange-disk staging and `CMS CHECK` reject malformed format and address inputs before dispatch. The local guest proof names its own installed image; the downloadable base image was built separately.
- Evidence: [Stage 3 local guest qualification](qualification/STAGE3-2026-10-05.md), `pdos/scripts/cms.crexx`, `pdos/scripts/cms-module.py`, and Mainframe Lab's tagged beta 3 CMS qualification.
- Acceptance: Pin the exact unchanged CMS cREXX package bytes and inventory their loader records, address/mode, entry/return, SVC/CMSCALL/DIAGNOSE, file naming/record, console and storage requirements against current z/PDOS behavior. Run a bounded loader/service probe with explicit rejection of unsupported operations. Record the selected adapter or environment design, its owner and the specific prerequisites for PD-016 and PD-017; do not treat a repackaged or recompiled TSO executable as a CMS pass.

## PD-016: CMS31 cREXX execution

- Type: improvement
- Status: Done
- Target: `cms20-esa31-v1` cREXX MODULEs on the selected z/PDOS machine
- Observation: The unchanged CMS31 RXVM/RXAS/RXC MODULEs run through the checked 0.1.1 guest path. Supplied and fresh IOQUAL, live input, library/imports, bounded heap, failure cases and exact stopped-disk outputs passed locally. The downloadable base image contains the adapter, but was not the exact image booted for this result.
- Evidence: [Stage 3 local guest qualification](qualification/STAGE3-2026-10-05.md), the pinned PD-015 contract and complete private 3270 traces.
- Acceptance: On a named z/PDOS build and machine, load the unchanged CMS31 RXVM MODULE and run supplied and fresh cREXX bytecode with the selected console and file fixtures, exact outputs and return codes. Then qualify unchanged CMS31 RXAS and RXC through a fresh RXC → RXAS → RXVM chain with the required library/import cases, bounded heap and failure controls. Preserve the separate CMS 20 result and identify unsupported CMS services explicitly.

## PD-017: CMS24 cREXX execution

- Type: improvement
- Status: Done
- Target: `vm370-4381-v1` cREXX MODULEs on a separately qualified low-address z/PDOS path
- Observation: The unchanged fixed-origin CMS24 RXVM MODULE ran the library-free IO24 subset at `0x20000`, with live input and exact stopped-disk output. Missing-file and invalid-mode controls returned expected errors; an altered-origin MODULE was rejected with RC 8. Native TSO24 loading remains open under PD-001. The downloadable base image contains this adapter, but the local CMS24 proof names its own installed image.
- Evidence: [Stage 3 local guest qualification](qualification/STAGE3-2026-10-05.md), the pinned PD-015 contract, and [PD-001](#pd-001-amode24rmode24-application-loading).
- Acceptance: After PD-015, load the unchanged historical CMS24 RXVM MODULE at its required origin and run its qualified library-free IO24 subset with actual input, exact binary/text readback, expected return codes and negative service/address controls. Keep its constrained heap separate from the wider profiles. The distinct native TSO24 path remains PD-001; a full historical CMS24 compiler chain requires separate evidence and is not implied by this subset.
