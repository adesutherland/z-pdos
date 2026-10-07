# z/PDOS 0.2.0 release record

7 October 2026. Version 0.2.0 selects the completed PD-003 K64/C31 kernel
and shared-U architecture. [P6](TWO-SPACE-P6-2026-10-07.md) retains its
pre-release source/workload qualification; this record owns the separate
release build and operator acceptance.

| Gate | Status |
| --- | --- |
| Version and current documentation | Passed: current architecture/capability guides reconciled; 97 current/reference guides scanned, local links resolved. |
| Code review and source freeze | Passed for release plumbing and version-only product delta; source/input freeze recorded before guest acceptance. |
| Fresh local K/U image and host package checks | Pending. |
| Exact mainframe operator acceptance and managed replacement | Pending; previous accepted version retained. |
| Four-host release matrix and downloadable K/U image | Pending. |
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
packaging. The kernel/service implementation remains the reviewed P6 code.
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
