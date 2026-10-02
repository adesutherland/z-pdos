# z/PDOS backlog

This is the only live roadmap, defect and qualification queue for this component.
Follow [the shared workflow](../../doc/WORKFLOW.md). Existing findings are
recorded here; their repair is outside the 2 October 2026 reorganisation.
Open items require separate implementation authority.

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
- Status: Open
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: Raw EBCDIC input uses hex-15 newlines. Unsupported framing and joined commands are difficult to diagnose.
- Evidence: Operator observations recorded in the previous ROADMAP.md.
- Acceptance: Document framing, diagnose unsupported input and provide a repeatable text-import route.

## PD-005: Command-length diagnostics

- Type: defect
- Status: Open
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: Console input is limited to 80 columns; batch uses a 200-byte buffer with at most 198 characters before its delimiter.
- Evidence: Previous operator roadmap.
- Acceptance: Report the limit before truncation and test both routes.

## PD-006: Console input and prompts

- Type: defect
- Status: Open
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: The newly built shell echoes individual characters on separate lines.
- Evidence: Operator observations in the previous roadmap.
- Acceptance: Echo a typed line together, retain one prompt and preserve its editable field.

## PD-007: Results and scrollback

- Type: improvement
- Status: Open
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: Repeated screens can make an older result look current.
- Evidence: Previous operator roadmap.
- Acceptance: Identify each command and its completion code unambiguously through long output.

## PD-008: Operator diagnostics

- Type: improvement
- Status: Open
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: Expected missing AUTOEXEC/BAT probes and actionable failures need clearer presentation.
- Evidence: Previous operator roadmap.
- Acceptance: Distinguish expected probes and announce unsupported addressing/residence before dispatch.

## PD-009: Temporary package lifecycle

- Type: improvement
- Status: Open
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: Package replacement/testing/removal needs a clear repeatable operator route.
- Evidence: Existing stopped-disk qualification and Lab lease process.
- Acceptance: Preserve the base OS, leave one working image and remove duplicate temporary installations.

## PD-010: Hosted fresh-image delivery

- Type: qualification
- Status: In progress
- Target: Fresh base-OS CCKD delivery built on the Linux GitHub runner
- Observation: The distro converter expands the 100-cylinder disk to the full 1,113-cylinder device by default. Conversion now specifies `-cyls 100` explicitly. The transport checker also retains identical legacy zero serial bytes, while still rejecting non-digit serial changes and all guest-byte changes. Fresh local source-to-image, full readback and corruption controls pass; distro utility qualification remains pending. Packaged host checks do not establish fresh guest execution.
- Evidence: `scripts/package-image.crexx`, `../../.github/workflows/build-release.yml`, and `../../doc/BUILD-AND-RELEASE.md`.
- Acceptance: Linux builds the fresh disk from source, passes loader/dataset/compression controls, records its actual utility identities and delivers an archive whose extracted files match their inventory. Keep any new guest qualification explicitly separate.
