# z/PDOS 0.2.0 release record

7 October 2026. Version 0.2.0 selects the completed PD-003 K64/C31 kernel
and shared-U architecture. [P6](TWO-SPACE-P6-2026-10-07.md) retains its
pre-release source/workload qualification; this record owns the separate
release build, DAT recycling repair and operator acceptance. All ten repaired
acceptance gates passed before managed adoption and publication.

| Gate | Status |
| --- | --- |
| Version and current documentation | Passed: current architecture/capability guides reconciled; 97 current/reference guides scanned, local links resolved. |
| Code review and source freeze | Passed for DAT recycling repair and release plumbing; new exact source/input freeze precedes repaired-image guest qualification. |
| Fresh local K/U image and host package checks | Passed for the repaired image: integrated host, loader/dataset/compression and ZIP inventory checks; unchanged components retain 93/93 host results. |
| Exact mainframe operator acceptance and managed replacement | Passed: ten gates, fourteen fresh phases, 143 ordered cases; reviewed working image adopted while stopped, old managed version retired and runbook committed. |
| Four-host release matrix and downloadable K/U image | Passed: final tagged builds on both macOS hosts, Linux x64 and Windows x64; packaged OS payloads match the accepted build. |
| Tag, publication and final asset verification | Passed: v0.2.0 published; signed Windows replacements, final shared checksums, signatures and exact source archive verified. |

The intended machine is Hercules 4.9.1.0-SDL, model 2064, one z/Architecture
CPU and 256 MiB real storage. Bare release media contain the OS only.
Operator working media add checked unchanged cREXX packages; those images
and the hosted disk container have separate identities.

Accepted capability claims follow P6: CMS31 and TSO31/64 ANY/HIGH fresh
compiler chains and real input, selected native calls and cursor restoration,
models 2–5 and line/monitor consoles, durable output and exact CKD/tape
workflows. CMS24 and native TSO24 are bounded to the library-free RXVM IO24
route. Full-library TSO24, CMS24 compiler/assembler and general CMS/TSO
compatibility are not claimed.

The release preparation changes VERSION, the shared guest version header,
CMake package identity, combined normal-build receipts and hosted K/U image
packaging. The initial candidate's kernel/service implementation remained
the reviewed P6 code.
Required review resolves release plumbing and documentation before freezing
source/input identities and beginning guest acceptance. A regression repair
returns to review/freeze before affected requalification.

## Review before qualification

The reviewed implementation delta from P6 is the guest version header only.
Release plumbing now selects K/U in hosted Linux builds, requires explicit
GNU s390 tools, records combined normal-build manifests, packages the correct
nested disk path and supplies 256 MiB K/U instructions. Version-specific
release notes replace the old one-space publication text. cREXX compilation,
C89 ASan/UBSan focused review checks and whitespace checks pass.

Documentation review covers the root/component READMEs, 97 current/reference
guides and the dated qualification index. Current boot, architecture,
dependencies, profiles, development, media and release guides now agree.
Old kernel/memory descriptions are explicitly historical. The legacy
conformance installer is labelled with its actual one-space acceptance;
K/U uses its checked normal/workload staging. All scanned local document
links resolve. Dated qualification reports retain their original inputs and
claims. Full-library TSO24 is consistently excluded from accepted capability.

The prior P6 code review and unchanged service/loader/console controls remain
applicable. The fresh 0.2.0 base, versioned U PCOMM, operator workloads and
media/lifecycle checks will establish the release candidate separately.

## Initial release build

The reviewed release preparation is commit
`77eddccef0a63254c9a065102f597e0616629799`. Its source/input freeze records
459 identities before operator qualification. Both Classic C variants and
all 93 host tests pass. The fresh normal K/U build and package pass their
loader, dataset, compression and SHA-256 checks.

| Local artifact | SHA-256 |
| --- | --- |
| K core, 4 MiB | `df2499795c4deac44140903e242323085818e6200a142c21c8e43f8c7709c4c0` |
| U PCOMM, version 0.2.0 | `ea2cb6542359a178908bda508ce89c1bee2ff495985a110febc6491545e7c38b` |
| Bare compressed disk | `a6ea674ceb76b8cbba1edc8b838b228e375f89778d32d34c8277d4aed6dd909a` |
| Local image ZIP | `5591e3a3a1a41606b6b9a999066e80ab38e003fe64820f3246d335eb84e2ab23` |

The K core is byte-identical to P6. The U PCOMM changes its shared version
identity; the service, loader and console implementations are unchanged.
All four jobs in the [branch build](https://github.com/adesutherland/z-pdos/actions/runs/37639441118)
pass. The hosted Linux image has the same K core, U PCOMM, PLOAD, KCORE
package, configuration and bootstrap command as the local build.

The hosted C31 handover native file differs only in seven module-name bytes
in its directory record, reflecting the build host's path prefix. The actual
PDOS loader materializes both at `0x200000` to identical 125,824-byte images,
entry offset `0x146d4`, AMODE31/RMODE24. Their loaded-image SHA-256 is
`c5b3ef2b619a26095397bb5faca2c921699aa6c0dfe9677ac3a1f683a8c6e928`.
The compressed disk container therefore has its own hash; guest execution
claims remain tied to the operator's exact delivered disk and workload media.

## Mixed-workload regression and repair

The initial operator runs passed bare/repeated boot, CMS31 and TSO31/64
ANY/HIGH fresh chains and input, bounded TSO24 IO24 and exact physical
disk/tape workflows. A later model-5 run combined HIGH version commands,
native call/fault/return controls, U application presentation and CMS31
loading, then returned RC 12 from a valid CMS24 CHECK. Cold CMS24 passed on
the same stopped disk; a reduced mixed sequence reproduced the failure.

The retained failed core shows U DAT pool real `0x280000`, capacity and used
both `0x160000`. No live allocation intersects the fixed CMS24 image; the
largest low-U interval is 15,597,568 bytes and real backing remains available.
The DAT allocator previously advanced a high-water mark and unmap cleared
PTEs without reclaiming empty tables. This exhausts table storage across
successive valid allocations. Early CMS24 preloading is not an accepted fix.

PD-023 recycles empty unlinked tables after the live single-CPU purge, keeps
the ASCE root, preserves the state ABI and reserves each missing table path
before publishing it. Focused C89 ASan/UBSan checks cover 200 valid map/unmap
cycles, live neighbours, interior page/region-table reuse, attachment,
purge-before-reuse and allocation failure without a partial change. Existing
memory, service-gate, normal-boot and simultaneous 64 MiB/128 MiB capacity
controls pass; Classic C31 compile/assembly/link passes. The same positive
churn test fails against the pre-repair allocator. Final source review checks
complete-path reservation, sibling preservation, marker uniqueness, root
retention, attachment, purge ordering, high-water bounds and the unchanged
single-CPU/state ABI. The exact source/input freeze and fresh image precede
affected guest acceptance. The old candidate's
native input and output baselines remain useful comparison inputs, while its
guest results do not qualify the repaired memory manager.

The operator's runbook update is required for acceptance: new command routes,
3270 device/model selection, the separate Telnet 3215 monitor, line-only
configuration, output persistence, K shutdown and host/lease cleanup.

## Repaired source and fresh build

Reviewed repair commit `b2b718e61c8b468d53da382a7228bab650abf30f` was frozen
with 459 source/input SHA-256 identities before qualification. The freeze
receipt SHA-256 is
`0b491b730633b4d998acac56b778b03abfe98eed77c489c610462d5118e9742a`.
The fresh normal source build passes its integrated host, Classic object/link,
loader, dataset and disk-compression controls. All 459 identities were
reverified before handoff. The unchanged component implementations retain their
93/93 host-test result; DAT and its dependent memory/gate/bootstrap checks
were rerun for the repair.

| Repaired local artifact | SHA-256 |
| --- | --- |
| K core, 4 MiB | `952d26167b07b39a5ba8675a622b460b17f2cde1bdbc8e16af8f94bcbf6eebcc` |
| U PCOMM, version 0.2.0 | `ea2cb6542359a178908bda508ce89c1bee2ff495985a110febc6491545e7c38b` |
| Bare compressed disk | `4c9a2813281c52f474b3850859085a6d68438f3924ee59463c8a99c8d963ae44` |
| C31 handover native file | `8cff28f471788a3ee2ad8400bcad1033e7dbf979885b5a9038f0262f4c7f4774` |
| KCORE package | `fd100145682c783883968f7ae5ca29c0b8c897bcfd6519b44415dbeb7690c77e` |
| Local image ZIP | `9848fecd96d7399f45d21512b65c09214903a6f8e010ebfba02ef1501127c695` |

The local ZIP passes its 13-entry internal SHA-256 inventory. The bare default
core is model 2; the operator derives and records model-specific console
configurations from these exact kernel bytes. Operator acceptance passed. Its bounded affected matrix begins with the exact failed mixed
sequence, then covers bare/repeated IPL, CMS31 and TSO31/64 ANY/HIGH chains
and input, bounded CMS24/TSO24 IO24, physical media and durable output,
positive models 2–5, line/monitor handoff and final installed readiness.
Unchanged application identities and exact output baselines are reused;
no new invalid-binary test campaign is part of this repair.

The repaired hosted Linux branch image passes its internal inventory and
matches local PLOAD, CONFIG, bootstrap COMMAND, KCORE and U PCOMM exactly.
C31 handover differs only at the same seven directory module-name bytes;
actual loader materialization is identical: 127,336 bytes, entry `0x146d4`,
AMODE31/RMODE24, loaded SHA-256
`f500f68f68e04d617f376ff4ded69d4199d7e1f63f362e43a6aa07d7dfda2b00`.
The hosted branch ZIP has SHA-256
`110d25ae79eb7aed1954ebdb36c86938c7dc483eb30c73b4bb8e87d71444161d`.
This host comparison does not substitute for the repaired guest acceptance.

All four repaired-source jobs passed in the
[branch build](https://github.com/adesutherland/z-pdos/actions/runs/37651991661):
macOS arm64, macOS x86_64, Linux x64 and Windows x64. Publication was skipped
because this is a branch build. Final version-tag assets remain a separate
post-acceptance gate.

The repaired model-5 core is
`f88ee321abbc505452974d42d80ae9737af8393febb028936e43ad4ff2204f54`,
independently reproduced from the frozen default core. The exact failed mixed
sequence now passes all 17 ordered commands, including CMS24 CHECK/RUN at
RC 0, both native parent/fault returns at RC 37, presentation/input/child
repaint and a final healthy HIGH retry. Normal K shutdown, complete attached
capture, released ownership/depth, durable STORE and unrelated disk records
all pass. Independently reviewed stopped DAT observations: capacity 1,441,792
bytes, high-water extent 196,608, 20 reusable frames (81,920 bytes), active
114,688 bytes and total available 1,327,104 bytes. Reusable interior holes are
counted separately from high-water tail storage. Host processes, handles and
exact leases were verified closed. This closes the original reproduction; all remaining affected application,
media and console acceptance gates also passed.

## Operator acceptance and managed adoption

Independent acceptance review verifies all ten gates in fourteen fresh
fleet-controlled phases and 143 ordered command cases against the frozen
repaired source and adapter. Every passing phase verifies normal shutdown,
complete ordered primary/monitor output, released ownership/depth, durable
transcript, stopped disk integrity and actual host-process/lease cleanup.
The [machine-readable record](release-0.2.0.json) pins each phase's core, disks,
receipt and independent handback hashes.

| Gate | Accepted result |
| --- | --- |
| R1 | Exact mixed failure order, CMS24 CHECK/RUN, native return/fault, model-5 input/repaint and HIGH retry. |
| R2 | Bare version/missing-command result, clean shutdown and repeated IPL of the same disk. |
| R3 | Fresh CMS31 chain and input; bounded CMS24 IO24; 59-command CKD/tape workflow and exact exports. |
| R4/R5 | Fresh TSO31/TSO64 ANY chains and input, five exact output files per profile. |
| R7 | Native TSO24 library-free IO24 input and two exact output files. |
| R8 | Models 2, 3 and 4 with long output, input, nested HIGH/U64 child and caller repaint; model 5 in R1/R6. |
| R9 | Line-only primary and exclusive monitor prompt handoff, complete text and released ownership. |
| R6 | Full HIGH chain/input plus native return/fault, model 5, mounted CMS31/CMS24 and healthy retry. |
| R10 | Repeated installed-image IPL/readiness and normal shutdown; all five HIGH outputs preserved. |

Eight freshly generated assembly/bytecode record-array comparisons match the
accepted P6 references. Seven CMS files, five per wider TSO profile and two
native TSO24 files pass exact byte/record readback. Six physical CKD exports,
including an empty VB record and all 256 binary octets, match expected records.
Host tape ingress and disk-to-tape COPY preserve the 160-byte payload and two
file marks; read-only inputs and the CMS exchange remain unchanged.

One model-2 console preparation omitted its nested HIGH dependency and failed
before input. The physical member inventory identified the omission; corrected
inputs passed models 2–4. Product source, frozen adapter and expected results
were unchanged. Failed whole-phase results remain failed rather than being
included in the fourteen accepted phases.

The retained working image is model 5, one CPU/256 MiB, with unchanged beta-3
HIGH applications and useful generated outputs. Its final IPL CCKD SHA-256 is
`2fcd0407640c9672d903aee3a25074b77c05cd40f02f4395824621feb1ad70dc`;
CMS exchange SHA-256 is
`80543264ed614b8e65225d3bd1698bbb6b516bcaaaf8e8538c4a69af9636c34a`.
These installed media are distinct from the OS-only downloadable image.
The operator runbook now covers K/U commands, model/device selection,
`ANSI@000A` teletype monitor, explicit line primary, storage/output location,
DAT observations, K shutdown and independent host cleanup. Its task-only
changes were reviewed before stopped adoption and selective Lab commit.

Managed adoption is complete. The operator committed the seven reviewed
runbook, registry and adapter paths in private Lab commit
`061378c50021b9972a8994fe105ca956cbdd22ec`. The exact accepted disk is the
current managed 0.2.0 version; the previous 0.1.1 version and 32 identified
operator temporary media paths were retired with receipts. Canonical inputs
and failed diagnostics were retained. All six guests are stopped, process
and lease ledgers are empty, and instance file handles are closed. The final
handback records 31 unrelated working-tree files preserved; no Lab push was
requested. Its SHA-256 is
`c9b0bdefe42c28a9aa10364ba51025eeda64148aa7dec62eba721175b8b21249`.

## Tagged publication

Tag `v0.2.0` selects acceptance-record commit
`83157ae4ad77c53ab2a1f2e9b25297b53bc1527a`. Its implementation and all 459
frozen input identities remain identical to the accepted repair commit;
the additional three paths record acceptance and managed adoption.
All four host jobs and the publisher passed in the
[tagged run](https://github.com/adesutherland/z-pdos/actions/runs/37658640352).
The [0.2.0 release](https://github.com/adesutherland/z-pdos/releases/tag/v0.2.0)
is published. The Windows job includes native install, reinstall, PATH and
uninstall checks; each host validates relocated tools as well as its builds.

The downloaded tagged image ZIP SHA-256 is
`faba11ecb94a5dc6c9282b0276d7ac24967f6269707895f98a4ae2d98ddc0faa`.
All 13 internal inventory entries pass. PLOAD, CONFIG, bootstrap COMMAND,
KCORE and U PCOMM match the accepted local inputs byte for byte. The C31
handover has the same seven host-directory metadata differences as the branch
build and loads through the actual product loader to the identical 127,336
bytes, entry `0x146d4`, AMODE31/RMODE24 and SHA-256
`f500f68f68e04d617f376ff4ded69d4199d7e1f63f362e43a6aa07d7dfda2b00`.
The public ZIP matches the compared tagged artifact, so this evidence binds
the packaged OS to the accepted build without repeating unchanged guest runs.

The downloaded Linux and both macOS portable packages each pass all 422
internal checksums and pin version 0.2.0, the exact tag commit and clean source.
Both macOS installer signatures and stapled notarization tickets validate.
All corresponding GitHub asset digests match the downloaded bytes. The source
archive matches every entry of an independent archive of the exact tag.

Windows CI initially publishes explicitly unsigned portable and installer
assets. Two initial local signing attempts returned PKCS11
`CKR_FUNCTION_FAILED`; refreshing the SimplySign session resolved the token
failure. The retained signing script completed all 16 signing operations,
including the eleven EXE/DLL/PowerShell payload files, three NSIS plugins,
uninstaller and final setup. Each signature was verified before packaging.

The signed portable ZIP and installer replace both unsigned release assets.
Independent verification of the downloaded replacements passes all eleven
payload signatures and the setup signature. The signed ZIP passes all 431
internal checksums and records signed status, version 0.2.0, the exact tag
commit and clean source. All eleven shared release checksum entries and all
twelve GitHub asset digests, including the checksum file itself, match the
actual downloaded bytes.

| Final Windows/checksum asset | SHA-256 |
| --- | --- |
| z-pdos-0.2.0-windows-x64-signed.zip | `239f4b1a0205e889da54c40792aa01754dc556c1fe8125b5961cf24dd12b1074` |
| z-pdos-0.2.0-windows-x64-signed-setup.exe | `896d2a5ce34e39553cde68eb46bf65d18ddd15b34fffe0be3021315b470c1df9` |
| SHA256SUMS | `42ecf0d549fd694f56757f44956ab907d7efeaa2aecd83aa1f72221d8be4968f` |

Final publication evidence is recorded after tagging; the accepted
implementation and input freeze remain unchanged. All release gates are
complete.
