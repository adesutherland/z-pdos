# z/PDOS 0.2.2 3270 preview

Adrian authorised a downloadable preview with the current 3270 cREXX workspace
and an illustrated end-to-end walkthrough on 4 November 2026. The selected
[preview download](https://github.com/adesutherland/z-pdos/releases/tag/pdos-3270-preview-2026-11-04)
contains the exact archive accepted below. This is a development preview,
separate from the 0.2.1 base image and general host-tool release.

The runtime source is `bea3126`, including the earlier output-padding repair.
The 31-file implementation/check freeze remains
`d45e7fdce46f350cbfda7f96455fc871fba47359f882f6f028ee02737ef27a91`.
It exactly matches the reviewed and accepted
[common-panel increment](CREXX-PANELS-2026-10-17.md). KCORE, command and EDIT
payloads likewise match that record, so its source review, host controls and
bounded native results are reused. This preview adds a clean factory image,
portable package and fresh-consumer walkthrough; it does not rebuild cREXX.

The source factory uses the unchanged bare 100-cylinder bootstrap input,
accepted 150-cylinder batch K core, current command/editor payloads and the
matched October-qualified TSO31 batch tools/libraries. It starts with empty
user collections, not a copy of Adrian's persistent workspace. The owning
cREXX package manifest and all eight tool/library payloads must verify before
image construction. The existing user disk and reservation are preserved.

The Mainframe Operator owns all media, devices, leases, IPL and console work.
The release target is the stable `build/pdos/release-3270/image` slot; the
portable archive is staged under `build/release/pdos-3270`. Runtime code is
frozen during qualification. A material repair returns to review and affected
checks before a new freeze; packaging/document-only changes do not require
repeating unchanged runtime checks.

## Fresh consumer and exact archive acceptance

The Mainframe Operator constructed a fresh factory image and drove the visible
DX3270 1.7.5 build20261017.2 model-5/IBM1047 interface. Application actions used
visible keyboard/mouse input, with no hidden source writes or job submissions.

- Created HELLO from empty collections, saved/reopened its exact 12 records,
  including blank records 3/8 and bracket syntax. Protected-field Reset,
  refused prefix/draft correction, unsaved exit, Help and Details were checked.
- Build 1 showed actual RXC progress and completed RXC, RXAS and RXLINK with
  OS0/RC0. Explicit Output review preceded Run 2, which returned OS0/RC0 and
  consecutive `Hello from wide PDOS`, `one`, `two` output lines.
- Saved `return )` made Run refuse a stale executable. Its build reported
  BADEXPR at line 12 with RC2 and skipped dependent steps. Restoring the exact
  valid source allowed Run 4 without rebuilding, proving preservation of the
  previous executable. Output was explicitly reviewed.
- Same-model reconnect recovered the form. Stopped independent readback found
  exact saved records, a matching source/artifact/profile triple and the durable
  native Run 4 result. Normal model-5 restart recovered source/output; model 2
  connected before IPL passed bounded startup, saved-source and recovered-output
  checks. These were persistence checks, not duplicate native compiler runs.
- The Operator verified the packaged ZIP's CRC and every one of its 28 file
  checksums, extracted it into the disposable consumer slot and booted with the
  supplied relative-path configuration. Programs was empty and a new member
  opened in the model-5 editor. Its pristine disk hash matches the image used
  for the full journey, so that journey is reused without another native build.

The final document-only repack adds the Operator's DX3270 Host/Port dialog
clarification to START-HERE and the walkthrough. Its runtime image, Hercules
configuration and screenshots are unchanged. Independent CRC/all-28-hash
verification of that final ZIP reuses the accepted native boot by exact image
identity; no further IPL or compiler run is claimed for a document correction.
Only PDF page 1 changed; it was rendered/reviewed again and pages 2-8 checked
unchanged. The identity table below names this final downloadable container.

The Operator reports normal shutdown, closed native I/O/ownership, removed
consumer processes/terminal/leases and temporary media. The personal running
ADRIAN-PD035 guest, disk and reservation remain unchanged. Only concise results,
the current release input/assets and requested manual screenshots are retained.

Four actual wide terminal screenshots are included. The coordinator rendered
and visually checked all eight PDF pages, verified portable Markdown/image paths,
ZIP CRC and all hashes independently, and checked the package's exact disk
identity. The cREXX packager compiles without warnings, rejects the personal
workspace path before writes, and passes its positive packaging path.

Models 3/4 and monochrome retain host-layout/check coverage in this increment.
Earlier physical3215 and advanced card/printer execution retain their stated
guest scope: no batch converter/DD/executor policy changed, LINE skips the
screen handoff, and focused host LINE/ownership/refusal checks cover that
dependency boundary. No fresh physical3215/monochrome/card job or full-family
conformance is claimed for this download.

## Selected identities and publication

| Input or deliverable | SHA256 |
| --- | --- |
| Pristine image, 7,583,838 bytes | `dceda57737481efef19771045b52790a95ce68461da0f5a3c527156aa59b0a86` |
| Final ZIP, 8,450,811 bytes | `2279823b16670cce05ad19695849b1fff9f3bc6420351e2db3bbdd86b5b06ec9` |
| cREXX package manifest | `a52dcb3a27f3c09928bb8492cf7674f75d504d3d3892f5a96bedcc9dca1c685c` |
| KCORE | `7696145531b53911c252bc29eaec6f775418e5e2a5484e156c072c7f5f04e53b` |
| Command processor | `6403cfa7ba781fcbdf77b590b1a6994447b85f436f6baf97c0de2d259ad173e8` |
| EDIT | `3ae5b5a2aba5b2ac538ca51542630bc6224106465267b9a7b094d290d87bc55d` |

`developer.json` identifies all installed tools, runtime libraries, generated
procedures and profile inputs. It contains no private host paths or user source.
The portable package recipe and lifecycle are documented in the
[development guide](../development/CREXX-DEVELOPMENT.md#portable-3270-preview-packaging).
The non-`v*` tag selects this explicit preview without dispatching a general
host-tool release. Hosted source CI is distinct from this native image/archive
acceptance; unchanged review and bounded guest results are reused by identity.
The runtime source's normal four-host build/package CI passed on 4 November
([run 41850631074](https://github.com/adesutherland/z-pdos/actions/runs/41850631074)).

The archive includes the image, portable Hercules configuration, notices,
SHA256SUMS, startup guide, PDF/Markdown walkthrough and its real screenshots.
Limits remain two program members, FB80 source, one synchronous execution
stream and selecting terminal geometry before IPL. Managed-fleet adoption,
upstream emulator publication and a general host-tool release are separate.
