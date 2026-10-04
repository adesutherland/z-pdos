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
| 2 | **Move fixtures both ways.** Import and export text, binary and record-oriented fixtures with explicit CMS or TSO names, framing and encoding. Check logical records and exact binary bytes after a guest run. | [PD-014](#pd-014-cms-and-tso-fixture-exchange) |
| 3 | **Run CMS cREXX binaries.** Inventory the exact CMS24 and CMS31 MODULE, entry and service contracts first. Qualify unchanged CMS31 RXVM, then its RXAS/RXC chain, against the bounded cREXX workload. Add the separate low-address path before attempting the historical CMS24 package. A TSO build of the same source does not establish CMS binary compatibility. | [PD-015](#pd-015-cms-crexx-binary-contract), [PD-016](#pd-016-cms31-crexx-execution), [PD-001](#pd-001-amode24rmode24-application-loading), [PD-017](#pd-017-cms24-crexx-execution) |
| Supporting gates | Qualify truthful invalid storage requests before widening the service surface, and correct region-table initialization before expanding mappings. Keep shared instruction selectors and native 64-bit kernel conversion behind the operator and compatibility outcomes unless a measured dependency moves them forward. | [PD-002](#pd-002-conditional-storage-service-errors), [PD-011](#pd-011-region-first-table-padding), [PD-003](#pd-003-shared-selectors-and-native-64-bit-kernel) |

The current TSO31, TSO64 ANY and TSO64 HIGH guest results can be reused for
unchanged bytes and services. A new installer, fixture path, loader or CMS
service needs its own affected host, disk and guest checks. The Lab's private
stopped-disk tools and qualification scripts are useful prototypes; the
supported operator path belongs with this product and must not depend on
private paths, accounts or image versions.

Step 1 is implemented, locally guest-qualified and manually accepted in the
current source; see
the [operator guide](user/CONFORMANCE.md) and
[4 October evidence](qualification/STEP1-2026-10-04.md). The published 0.1.0
image predates it. Orders 2 and 3 remain open.

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

## PD-003: Shared selectors and native 64-bit kernel

- Type: improvement
- Status: Open
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: The current C32 kernel runs AMODE31 with z/Architecture support; a full-width C kernel is a separate conversion.
- Evidence: Machine contract and existing 0.1 qualification.
- Acceptance: Review selectors against shared ceilings and give a 64-bit conversion its own ABI, source and guest acceptance.

## PD-004: Batch-file delivery

- Type: improvement
- Status: Done in current source; unreleased
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: The conformance recipe now encodes bounded UTF-8 script commands into IBM1047 raw U/18452 records with hex-15 delimiters. It rejects unsupported manifest geometry and overlong lines, then checks exact batch readback.
- Evidence: [4 October host and guest result](qualification/STEP1-2026-10-04.md); [operator guide](user/CONFORMANCE.md).
- Acceptance: Document framing, diagnose unsupported input and provide a repeatable text-import route.

## PD-005: Command-length diagnostics

- Type: defect
- Status: Done in current source; unreleased
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: A trailing `&` joins 3270 field entries at `MORE>` up to 198 characters. Console and batch records beyond that limit are rejected without dispatch.
- Evidence: [PCOMM boundary control](../tests/pcomm-operator.crexx) checks both routes at 198/199 characters; the [guest result](qualification/STEP1-2026-10-04.md) includes a human entry beyond 80 columns.
- Acceptance: Let a human submit a command longer than the 80-column console field through a documented continuation or short-script route, up to the tested command limit. Diagnose an overlong console or batch command before executing any truncated fragment; test both routes at and beyond their limits.

## PD-006: Console input and prompts

- Type: defect
- Status: Done in current source; unreleased
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: The 3270 writer now keeps a row and column across fragmented writes, wraps output above the editable field and presents a distinct `MORE>` continuation prompt.
- Evidence: [ASAN/UBSAN console control](../tests/io-repair.c) and the [guest operator result](qualification/STEP1-2026-10-04.md).
- Acceptance: Echo a typed line together, retain one stable prompt and its editable field through normal and long output, and show a distinct continuation prompt when a command spans console entries.

## PD-007: Results and scrollback

- Type: improvement
- Status: Done in current source; unreleased
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: External commands now print numbered BEGIN/END markers with RC. The runner requires a fresh prompt, captures a complete ScreenTrace and judges ordered results against the locked manifest; a missing expected output fails.
- Evidence: [4 October fresh and deliberately corrupted trace results](qualification/STEP1-2026-10-04.md).
- Acceptance: Identify each submitted command and its completion code unambiguously through long output, including the command following a failed one. Keep the latest result readable at a fresh prompt and retain a complete trace for automated checks.

## PD-008: Operator diagnostics

- Type: improvement
- Status: Done in current source; unreleased
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: The kernel hides expected missing PCOMM `.BAT` allocation probes, while a present invalid dataset and other allocation failures remain visible. `HELP TSO`, `HELP CMS` and `VERSION` explain the command boundary and build identity. The direct loader already announces unsupported addressing/residence.
- Evidence: [4 October guest result](qualification/STEP1-2026-10-04.md), [operator guide](user/README.md) and prior [0.1 qualification](qualification/QUALIFICATION.md).
- Acceptance: Distinguish expected probes and announce unsupported addressing/residence before dispatch.

## PD-009: Temporary package lifecycle

- Type: improvement
- Status: Done for the bounded conformance flow; unreleased
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: Preparation creates a fresh disposable candidate from an untouched base, checks it offline, and documents selection, stop, readback and removal. Three local guest candidates were selected only while stopped and removed after returning to the accepted image.
- Evidence: [operator guide](user/CONFORMANCE.md) and [4 October handback](qualification/STEP1-2026-10-04.md).
- Acceptance: Preserve the base OS, leave one working image and remove duplicate temporary installations.

## PD-010: Hosted fresh-image delivery

- Type: qualification
- Status: Done
- Target: Fresh base-OS CCKD delivery built on the Linux GitHub runner
- Observation: The distro converter expands the 100-cylinder disk to the full 1,113-cylinder device by default. Conversion now specifies `-cyls 100` explicitly. The transport checker also retains identical legacy zero serial bytes, while still rejecting non-digit serial changes and all guest-byte changes. The 0.1.0 Linux release job completed source-to-image, loader/dataset/compression checks and image packaging. Packaged host checks do not establish fresh guest execution.
- Evidence: [Release run 37103970675](https://github.com/adesutherland/z-pdos/actions/runs/37103970675), source `3394771ee4d1c62054ae4f052a502bec2ba93ace`, and the published `z-pdos-0.1.0-pdos-image.zip`; [release guide](../../doc/BUILD-AND-RELEASE.md).
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
- Status: Done in current source; unreleased
- Target: Disk path in `pdosLoadExe`
- Observation: The direct loader now validates a single selected extent, bounds track reads and requires an EOF before accepting the image. A malformed RXC64 executable without EOF was rejected; a following PCOMM command ran normally.
- Evidence: [Current source](../src/pdos.c), `pdosLoadExe` and [4 October malformed-disk guest result](qualification/STEP1-2026-10-04.md).
- Acceptance: Validate the selected extent, stop reads at its boundary and require the appropriate termination. Preserve valid native loads and reject a missing-EOF image without reading an adjacent dataset.

## PD-013: Repeatable program conformance run

- Type: improvement
- Status: Done in current source; unreleased
- Target: z/PDOS operator commands, package staging, PCOMM batch execution and result capture
- Observation: The product-level manifest and cREXX recipe now stage checked native programs, simple VB members and raw batch on a fresh disk, read them back, run a leased 3270 script with live replies, and emit per-command and overall PASS/FAIL. The published 0.1.0 base image still has no application package or this runner.
- Evidence: [4 October qualification](qualification/STEP1-2026-10-04.md), [operator guide](user/CONFORMANCE.md), and the [product recipe](../scripts/conformance.crexx).
- Acceptance: From an untouched base image, a checked native program package and a declarative test script, provide one documented product-level flow that creates a disposable candidate, loads the program without changing its native bytes, runs ordered commands and reports each command's identity, return code, expected result and overall PASS/FAIL. Verify fresh output rather than accepting stale data; retain input/output hashes and guest evidence in a portable receipt. Demonstrate a positive and an intentionally failing cREXX case, an interactive-input case, a command longer than 80 columns, and cleanup that preserves the base image. Do not require a private Lab path or manual interpretation of a screen snapshot.

## PD-014: CMS and TSO fixture exchange

- Type: improvement
- Status: Open
- Target: z/PDOS host/disk fixture tooling and the selected cREXX record-file contract
- Observation: The existing Lab delivery tool inserts text or binary members into an existing simple VB PDS on a stopped image, and separate private staging/readback tools handle the qualified package. The product image has no general, documented import/export route. CMS file name/type/mode and TSO dataset/member naming and record conventions are different.
- Evidence: [0.1 qualification](qualification/QUALIFICATION.md), [user guide](user/README.md), and the Lab's managed-PDOS delivery and readback reports. No common CMS/TSO physical disk format is established.
- Acceptance: Define a portable fixture manifest with explicit logical name, destination/source, record format and length, text encoding or binary mode, and expected hashes. Import and export the cREXX text, blank/empty, and ordered 00–FF binary cases through the supported CMS and TSO fixture routes and z/PDOS; compare logical records and binary bytes after execution. Reject unsupported framing, record length, missing datasets and ambiguous conversions without partially accepting a candidate. Check unaffected disk content and keep image changes offline. State any CMS or TSO record forms outside the supported subset.

## PD-015: CMS cREXX binary contract

- Type: qualification
- Status: Open
- Target: unchanged CMS24 and CMS31 cREXX MODULE packages and the z/PDOS loader/service boundary
- Observation: The current native loader accepts selected TSO-style load modules. CMS24 uses a fixed-origin MODULE; CMS31 uses a separate relocatable MODULE and CMS entry, storage, terminal and file services. Matching C source or machine instruction ceiling alone does not make either package executable on z/PDOS.
- Evidence: Mainframe Lab's CMS MODULE, CMS31 and ABI/runtime guides and the tagged beta 3 CMS24/CMS31 qualification; z/PDOS [architecture](architecture/README.md) and [0.1 qualification](qualification/QUALIFICATION.md).
- Acceptance: Pin the exact unchanged CMS cREXX package bytes and inventory their loader records, address/mode, entry/return, SVC/CMSCALL/DIAGNOSE, file naming/record, console and storage requirements against current z/PDOS behavior. Run a bounded loader/service probe with explicit rejection of unsupported operations. Record the selected adapter or environment design, its owner and the specific prerequisites for PD-016 and PD-017; do not treat a repackaged or recompiled TSO executable as a CMS pass.

## PD-016: CMS31 cREXX execution

- Type: improvement
- Status: Open
- Target: `cms20-esa31-v1` cREXX MODULEs on the selected z/PDOS machine
- Observation: CMS31 has a qualified cREXX native compiler chain on CMS 20, but no unchanged CMS31 MODULE has been loaded or run on z/PDOS. Its relocatable MODULE and CMS service contract need their own implementation and guest evidence.
- Evidence: Mainframe Lab's tagged beta 3 CMS31 qualification and the contract to be recorded under PD-015.
- Acceptance: On a named z/PDOS build and machine, load the unchanged CMS31 RXVM MODULE and run supplied and fresh cREXX bytecode with the selected console and file fixtures, exact outputs and return codes. Then qualify unchanged CMS31 RXAS and RXC through a fresh RXC → RXAS → RXVM chain with the required library/import cases, bounded heap and failure controls. Preserve the separate CMS 20 result and identify unsupported CMS services explicitly.

## PD-017: CMS24 cREXX execution

- Type: improvement
- Status: Open
- Target: `vm370-4381-v1` cREXX MODULEs on a separately qualified low-address z/PDOS path
- Observation: The historical CMS24 package uses a fixed-origin MODULE and a constrained storage/service profile. z/PDOS currently rejects even TSO24 AMODE24/RMODE24 direct loading before dispatch; PD-001 is a prerequisite, but it does not by itself provide CMS services.
- Evidence: Mainframe Lab's tagged beta 3 CMS24 qualification, [PD-001](#pd-001-amode24rmode24-application-loading), and [0.1 qualification](qualification/QUALIFICATION.md).
- Acceptance: After PD-015 and PD-001, load the unchanged historical CMS24 RXVM MODULE at its required origin and run its qualified library-free IO24 subset with actual input, exact binary/text readback, expected return codes and negative service/address controls. Keep its constrained heap separate from the wider profiles. A full historical CMS24 compiler chain requires separate evidence and is not implied by this subset.
