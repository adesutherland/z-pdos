# Invocation-owned CMS output staging, 6 October 2026

This bounded P1 checkpoint keys each transient CMS output buffer by the
active invocation as well as filename and 24/31-bit profile. K records the
buffer as an owned file handle, verifies that handle before WRBUF, FINIS or
ERASE, and reaps an unfinished output on invocation return. FINIS transfers
a closed buffer to K's diagnostic output store so the existing post-call
audit can read it. A child cannot find the parent's same-name open buffer.
This is an in-memory stage, not a durable file commit.

The Classic C31 target service and the 89-check diskless machine gate passed
at `build/pdos/invocation-cmsoutput-1/run/receipt.json`. The first fresh
3390 IPL passed 142/142 checks at
`build/pdos/invocation-cmsoutput-ipl-1/run/receipt.json`, including CMS31
IOQUAL output and the CMS24 IO24 path. That source core SHA-256 was
`03c3f615b16c46a2ee168784d250fa0c054bb34da7f86f468b6a390260447a09`;
the disk hash before and after was
`20fc70fdf6aee1d384fe7cf3acdec1dcbff5260c3e08ac68f5e5fcbe4fe30700`.
The host exited zero without a timeout on one model-2064 ESAME CPU with
256 MiB real, 3390 `01B9` and 3270 `0009`. Exact toolchain and unchanged
application inputs are recorded in the [CMS invocation
gate](TWO-SPACE-CMS-INVOCATION-2026-10-06.md) and [ABI
inventory](../architecture/TWO-SPACE-ABI.md).

The later WRBUF ownership preflight is qualified by the second build and
fresh IPL recorded below. A nested same-name writer, abnormal return and
stopped-disk durability still need separate guest gates before P1/P2 can
close.

## Exact committed version

The additional WRBUF handle preflight passed the full Classic C31 and
89-check diskless gate at
`build/pdos/invocation-cmsoutput-2/run/receipt.json`. Its fresh IPL passed
142/142 checks at `build/pdos/invocation-cmsoutput-ipl-2/run/receipt.json`.
The source core SHA-256 was
`f1cae738cbba06ab14f82617dd7fa0687538b5629cdf0853d3f2a8fe16d6b31b`;
the disk hash was
`ff526fcba044f5b56f64496dec117d117d1e50551049732ecc7ef69a13b265e5`
before and after IPL. The host exited zero without a timeout on the same
machine and unchanged application stages.
