# Invocation leases on K-cached images, 6 October 2026

The diagnostic K loader keeps checked native images mapped as a cache while
their pages and real backing fit. Each invocation now owns a `TSV_IMAGE`
lease identified by its nonreused token. K counts active leases per checked
image record and releases them through the same cleanup callback as that
invocation's files, allocations and lowcore backing. Ending a child releases
its lease without evicting the suspended parent's image; ending the parent
returns the active count to zero. A zero active count does not imply eviction,
so co-resident modules remain available when storage permits. The cache and
its metadata are K-owned and add no low U mapping.

The diskless Classic C31 and machine gate at
`build/pdos/image-lease-1/run/receipt.json` passed 94/94 checks. The
source-built fresh 3390 IPL at
`build/pdos/image-lease-ipl-1/run/receipt.json` passed 148/148.
The nested guest control observed counts `1, 2, 1, 0` for the same checked
TSO31 image across parent entry, child entry, child return and parent return.
The unchanged selected CMS24/31 and TSO24/31/64 paths passed in the same
run. Source core SHA-256 was
`71b22bb1cc33ae222a2e842f940a1e6460ad747c6b34cca7384718f74e27c1bf`.
Disk SHA-256 was
`3aec0e7939ea16f40991aab1ede50854f17110b0cc0fca5b9a73bcc9796698f1`
both before and after IPL. The host exited zero without timeout on one
model-2064 ESAME CPU, 256 MiB real, 3390 `01B9`, 3270 `0009`, using the
pinned toolchain and unchanged stages in the
[invocation gate](TWO-SPACE-INVOCATION-GATE-2026-10-06.md).

This control is a nested K descriptor probe, not two native applications
using one mutable image simultaneously. The normal loader must reject or
clone a module replacement while an invocation holds a lease, and must
release cache pages transactionally on eviction or storage pressure. Native
same-personality calls, child-fault unwind and image-cache eviction are
separate P1/P3 gates.
