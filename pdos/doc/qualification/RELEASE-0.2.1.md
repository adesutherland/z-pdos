# z/PDOS 0.2.1 release selection

I selected 0.2.1 for publication on 10 October 2026. This release combines the
shared I/O repair, retained Workbench console, small C applications and
canonical cREXX bundle with the coordinated 512 MiB machine profile.
Paul Edwards created PDOS and PDPCLIB; the maintained fork retains attribution
and component licence notices.

## Reviewed and qualified scope

The [console record](CONSOLE-STAGE3-2026-10-09.md) owns 21 accepted runs and
333 checks. The [application record](APPLICATIONS-2026-10-10.md) owns the
separate 68-check standard/wide colour, monochrome and line matrix, exact
exchange records, durable transcript readback, 320 MiB allocation/release and
canonical RXC → RXAS → RXVM source execution. Their original source and image
identities remain historical evidence; those images reported version 0.2.0
while development was in progress.

The release changes root VERSION, CMake metadata and the guest version header
to 0.2.1, updates operator/release guides, and includes this release record in
the image archive. No kernel, application-service or C utility behaviour
changes during release selection. Freeze 2 under
`build/pdos/release-021-freeze2` records the reviewed source/tool inputs before
the affected guest check. The fresh source build is
`build/pdos/release-021-image3`, using the existing Classic tools, GNU s390
Binutils 2.47.20260726 and Hercules 4.9.1.0-SDL on macOS arm64.

The K core, KCORE package, PLOAD, handover and configuration compare byte for
byte with the accepted application distribution10. All rebuilt C application
native payloads also compare exactly. Their completed matrix is reused on
that basis. PCOMM differs in exactly five native bytes: its version character
and four checksum bytes. The separate version smoke checks the changed string
and healthy native call/return, with exact stopped-media and transcript checks.

The first smoke used the candidate disk instead of a private copy and waited
for an incorrect EDIT prompt count. VERSION and HELLO completed before the
fixture timeout, but that run is excluded from acceptance. Its used disk and
failed receipt are retained. Recompression of the pristine raw source did not
reproduce the original container hash, so the complete source producer was
rerun rather than changing an old manifest. Image2 stopped in the macOS
compressor's device-close wait; its process sample and log are retained and
its partial output is excluded. Image3 is the completed producer retry.

The accepted private-disk `release-021-smoke3` passes all 18 checks: VERSION
reports 0.2.1, HELP reports the editor capacity, two HELLO calls return OS0/RC0,
caller/ownership/depth and capture checks pass, and normal shutdown commits the
exact transcript. Its compressed input is
`72d3be4a3b8483e5dda3715ac2383522a06d7e0a2901e2f1435b871d72ddba80`;
independent decompression compares every guest-visible CKD record with the
pristine source, with only documented container serial/post-EOT padding
regeneration. The durable transcript SHA256 is
`9a9b72fe9c74529e48f90e252338cebfb99a109c76fbd77028bd0d8c461f182c`.
The completed application matrix is not rerun for this version-only change.

Final selection verifies all 611 frozen source rows and 785 producer manifest
rows. Every guest-visible record in the completed image3 disk compares with
the accepted smoke's pristine source, including canonical cREXX members.
The selected local compressed disk SHA256 is
`b26a0af201c60623a1c653502b6b3c20d01ca16a69030f11745a31a2e8acee4b`.
The source freeze SHA256 is
`195c9038426a93129485bf9bf28f8194752e8ee9e05b2676baae4cc6f3c44c50`;
the unchanged K core is
`544cbffb7db3af50f17da525bb704ae5a12e85e1c13b1cb0f80d78127efff3ae`.

## Publication and artifacts

The annotated `v0.2.1` tag selects the release source. Its
[hosted workflow](https://github.com/adesutherland/z-pdos/actions/workflows/build-release.yml)
builds both Classic compiler variants, the assembler and linker on macOS
arm64/x86_64, Linux x64 and Windows x64, with required host and relocated-tool
checks. Linux builds a fresh bundled disk and verifies loader, datasets and
compressed-container readback. Publication runs only after all four build
jobs succeed and independently verifies the final artifact inventories.

The [release](https://github.com/adesutherland/z-pdos/releases/tag/v0.2.1)
contains host tools/installers, the bundled PDOS image, corresponding source
and SHA256SUMS. macOS publication requires signed, notarized and stapled
packages. Initial Windows CI assets are explicitly unsigned until the separate
local signing operation is completed; do not infer signing from a prior
version. Host-built disk containers and locally guest-tested payloads have
separate identities. The exact tag workflow and post-publication comparison
are recorded after they complete, rather than predicted in this source record.

The initial tag build and diagnostic branch build failed on Linux because
the source-receipt recipe uses ripgrep and the runner did not install it.
The command payload completed; its identity scan returned 127 before K build.
Both macOS jobs and Windows passed on the diagnostic branch, including macOS
signing/notarization. CI now installs ripgrep, and the image entry point checks
it before compilation. This repairs host orchestration only; the accepted
guest payloads and guest evidence remain unchanged. The failed tag has no
published release; final publication selects the repaired source.

## Limits and next version

This release does not close the entire 3270 family or all available-provider
facilities. DX3270 1.7.5 has observed code-page, retained-pane repaint and graph
export defects under PD-031. Hercules's selected 3215 provider accepts 148
input characters plus CRLF. Native record saves use a separate empty non-IPL
target, first-track PS FB/VB and at most 65536 packed bytes; replacement is not
atomic. The default disk remains 100 cylinders and STORE has eight fixed
2 MiB slots with two banks. The [capacity review](../architecture/CAPACITY-REVIEW-2026-10-10.md)
explains which bounds are architectural, checked policies or persistent-layout
constraints.

The initial 0.2.2 focus is PD-030: configurable disk geometry, larger files,
extent allocation/reuse, migration and crash recovery, followed by measured
contiguous/fragmented I/O. Continue through the shared owned completion engine.
Managed-fleet adoption and marketing publication remain separate from release.
