# z/PDOS 0.2.0 release record

7 October 2026. Version 0.2.0 selects the completed PD-003 K64/C31 kernel
and shared-U architecture. [P6](TWO-SPACE-P6-2026-10-07.md) retains its
pre-release source/workload qualification; this record owns the separate
release build and operator acceptance.

| Gate | Status |
| --- | --- |
| Version and current documentation | Passed: current architecture/capability guides reconciled; 97 current/reference guides scanned, local links resolved. |
| Code review and source freeze | Passed for DAT recycling repair and release plumbing; new exact source/input freeze precedes repaired-image guest qualification. |
| Fresh local K/U image and host package checks | Initial candidate passed: fresh normal build, 93/93 host tests and package checks. Repaired image pending. |
| Exact mainframe operator acceptance and managed replacement | Blocked by PD-023 DAT exhaustion in a valid mixed workload; previous accepted version retained. |
| Four-host release matrix and downloadable K/U image | Branch matrix passed on both macOS hosts, Linux x64 and Windows x64; final tag matrix pending. |
| Tag, publication and final asset verification | Pending. |

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
