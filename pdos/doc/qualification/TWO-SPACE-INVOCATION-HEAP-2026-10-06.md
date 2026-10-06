# Invocation-owned CMS heap and free preflight, 6 October 2026

This is a bounded P1 ownership checkpoint on `develop`. K's active
invocation frame now owns the CMS31 DMSFROSV allocation, in the same ledger
as native SVC 120 and IARV64 allocations. CMS31 DMSFRRSV requires the active
frame's exact K handle before it calls the lower-level allocator. SVC 120
FREEMAIN and IARV64 DETACH have the same read-only ownership preflight.
An allocation made by a suspended parent is therefore not freed by a child
that happens to share its allocator task ID. CMS31 heap metadata is kept per
invocation stack slot; a child can have its own heap while the parent is
suspended. Normal return reaps an unfreed heap through the existing K cleanup
callback, and a successful end clears the slot.

The Clang C89 host invocation test passed under address and undefined-behavior
sanitizers. It checks that a child cannot claim the parent's handle and that
a suspended parent cannot use its own handle until it resumes. The full
source-built diskless machine gate passed 89/89 checks at
`build/pdos/invocation-cmsheap-1/run/receipt.json`. A fresh source-built
3390 IPL passed 142/142 checks at
`build/pdos/invocation-cmsheap-ipl-1/run/receipt.json`, including the
unchanged CMS and TSO version and CMS IOQUAL paths. The source core SHA-256
was `c200d4d8dab70b8b72983a6d01245e820cf57a5ae1f03c04902155b8230d895d`.
The disk SHA-256 before and after IPL was
`b115e1b541e10804d8474b51d4062ac1283367f27e2f70ca54ec8aad1124c019`.
The host exited zero with no timeout. The machine was one model-2064 ESAME
CPU, 256 MiB real, 3390 `01B9`, and 3270 `0009`. Toolchain and unchanged
application stage identities are pinned by the [CMS invocation
gate](TWO-SPACE-CMS-INVOCATION-2026-10-06.md) and [ABI
inventory](../architecture/TWO-SPACE-ABI.md).

The guest still has no native nested CMS heap request or child-fault case.
The host control proves ledger rejection, and the IPL proves regression of
the current guest paths. It does not establish complete P1 ownership of CMS
file handles, lowcore, terminal leases or pending I/O, nor abnormal-exit
cleanup. Those are separate gates before normal-image selection.
