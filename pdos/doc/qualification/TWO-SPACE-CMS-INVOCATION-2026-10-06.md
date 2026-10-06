# CMS invocation ownership in the diagnostic successor, 6 October 2026

This bounded P1 gate extends the [TSO invocation
gate](TWO-SPACE-INVOCATION-GATE-2026-10-06.md) and [allocation
unwind](TWO-SPACE-INVOCATION-REAP-2026-10-06.md). The private fixture
SVC 235 now names either CMS24 or CMS31, with a checked fixed or relocated
image base. K creates a CMS descriptor with the selected image owner,
runtime owner, full caller state and nonreused token. CMS24 SVC 202 and
CMS31 SVC 204 use the active descriptor and validate the caller against
that image's actual mapped size and AMODE. The CMS31 lowcore SVC 205 veneer
requires an active CMS31 descriptor. Native SVC 120 allocations can be
recorded against the same invocation and reaped by the existing checked
cleanup path. Program counter and mode validate a declared invocation;
they no longer select its CMS personality.

The guest fixture surrounds five unchanged or existing CMS executions:
CMS24 RXVM `-v`, a CMS31 overlay child with the FST veneer, CMS31 RXVM
`-v`, the second relocated CMS31 RXVM running IOQUAL, and CMS24 IO24.
All five recorded begin/end status zero, with tokens 1 through 5. The
unchanged TSO24/31/64 ANY entries and their K owner checks still pass.
The high U diagnostic fixture now puts its existing wide-heap helper at
U `0x110003000`, backed by real `0x1f000`, because the call receipts no
longer fit in the first U high code page. K maps no code or state into
24-bit U as a result. The host gate treats the new helper as read-only and
still verifies an unmapped next page.

`build/pdos/invocation-cms-5/run/receipt.json` passed 89/89 diskless
machine checks. The disposable fresh 3390 IPL at
`build/pdos/invocation-cms-ipl-3/run/receipt.json` passed 141/141 checks,
including unchanged CMS24 RXVM and IO24, CMS31 RXVM and IOQUAL, and
TSO24/31/64 ANY RXVM. The Classic C31 service is 76,006 bytes,
SHA-256
`c59ce7334db31e11b13093aa9790ecf0596ed38877054b3b0e95898192a56163`.
The source core SHA-256 was
`3d5c29e6e3dd30e5934c8b669ac3e3a28c4502c5ecc08d5beed10b7001eebab7`.
The disk SHA-256 before and after IPL was identically
`c1b1814fded944a56f633aa618aabef2d901c9f81163e5ca0a2e65ae2e2932fc`.
The profile was one model-2064 ESAME CPU, 256 MiB real storage, 3390
01B9 and 3270 0009, with the same source-built Classic toolchain, local
GNU binutils 2.47, Hercules and pinned unchanged stages as the [prior
gate](TWO-SPACE-INVOCATION-GATE-2026-10-06.md). The host exited zero and
the run did not time out.

Two failing development IPLs exposed fixture sequencing and addressing
errors: the FST veneer was exercised by the overlay child before any CMS
descriptor existed, then a high U PC-relative load targeted a constant
in low U beyond its supported displacement. The passing run surrounds
that child and loads the second-image base from a high U local constant.
No unchanged application binary was modified.

The fixture still requests entry through a private U-callable SVC. A
production launcher must create descriptors from K-controlled program
selection. CMS file handles, output records, lowcore backing and terminal
leases are still profile-wide, not invocation-owned. General nested
application call, fault unwind and pending I/O cleanup remain open. This
does not close P1 or qualify P2–P6.
