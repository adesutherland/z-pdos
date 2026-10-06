# K cleanup of a live native allocation, 6 October 2026

This bounded P1 check extends the [native invocation
gate](TWO-SPACE-INVOCATION-GATE-2026-10-06.md). An invocation's K resource
ledger now identifies its U allocation by the storage allocator's unique
handle. On `TSVEND`, the cleanup callback checks that handle and task owner,
then uses the ordinary checked `TSMFREE` path to unmap U pages, clear backing,
release real frames and release the virtual interval. A failed release keeps
the invocation in `REAPING` for retry; it cannot report a successful end.

The private diagnostic SVC 237 gives a TSO31 invocation one mapped page
without issuing FREEMAIN. The U fixture writes to that page and ends the
invocation. The guest recorded begin, allocation and end status zero, token
4 and virtual address `0x02010000`; the stopped-core DAT walk found that
page unmapped. The DAT mutation count increased by exactly two for its map
and unmap. This path exercises the real K allocator and Classic C31 cleanup
in an emulated guest, rather than only the host ledger callback.

The diskless recipe at `build/pdos/invocation-reap-1/run/receipt.json`
passed 89/89 checks. The disposable 3390 fresh IPL at
`build/pdos/invocation-reap-ipl-2/run/receipt.json` passed 136/136 checks,
including the unchanged CMS24/CMS31 and TSO24/31/64 ANY cases. The Classic
service is 75,638 bytes, SHA-256
`29ab36754a5fcc23c848c6792da934f58103483387ab0ee89d73e8add5600203`.
The IPL profile was one model-2064 ESAME CPU, 256 MiB real storage, 3390
01B9 and 3270 0009. The source core SHA-256 was
`4ed6317b88415a8fedc1e7799fbce32ceffc6f012e3a788a3b2aa4b77ca38b43`.
The disk SHA-256 was the same before and after IPL:
`ad475f5a91f1873ea39ab6a3948adb3106d21e36589f8fcc80fa4c3867192c81`.
The unchanged stage identities and toolchain are those recorded in the
[previous gate](TWO-SPACE-INVOCATION-GATE-2026-10-06.md); the recipe checked
the stage hashes again. The host exited zero without a timeout.

The first fresh run observed the correct guest cleanup but failed a stale
test expectation for DAT mutation count. The expectation was corrected by
the observed one-page map/unmap pair; no guest implementation change was
needed for the passing second IPL. This is a diagnostic test of allocation
unwind. Image, file, lowcore and terminal leases, fault-triggered unwind,
pending I/O completion and a normal K launcher remain open. P1 and later
checkpoints are incomplete.
