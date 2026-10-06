# Native invocation gate in the diagnostic successor, 6 October 2026

This is the next bounded P1 result after the
[ownership component](TWO-SPACE-INVOCATION-2026-10-06.md). It is a guest
proof of the K descriptor on three unchanged TSO RXVM paths, not a
production launcher or a complete native-call implementation.

The diagnostic U fixture begins an explicit K invocation immediately before
entering each loaded TSO24, TSO31 and TSO64 ANY image, then ends it after
the application returns. The private SVC 235/236 pair is a fixture control,
not a public TSO ABI. K saves all caller GPRs, PSW, ASCE and key in its
descriptor and gives each entry a nonreused token. Native SVC 120 uses the
active descriptor's runtime owner; new allocations are recorded by K handle
and removed from that ledger on successful FREEMAIN. The selected TSO64
IARV64 GETSTOR/DETACH subset uses the same owner and records its high
allocation. TPUT selects the active descriptor's terminal receipt. The
saved caller PC and AMODE still check that a request came from the declared
image, but do not choose its personality or allocation owner.

The source-built Classic C31 service is 74,902 bytes in the 128 KiB K-only
slot, SHA-256
`4692a6fd003da1e55b4e0089617b1cd56ef866555b901bef63de7fe39547fb96`.
The diskless `two-space-next.crexx` run at
`build/pdos/invocation-gate-3/run/receipt.json` passed 89/89 checks. The
fresh `two-space-ipl.crexx` run at
`build/pdos/invocation-gate-ipl-2/run/receipt.json` passed 134/134 checks,
including unchanged CMS24 RXVM and IO24, CMS31 RXVM and IOQUAL, and TSO24,
TSO31 and TSO64 ANY RXVM. Each TSO begin and end status was zero; the
recorded tokens were 1, 2 and 3. The host exit was zero and the run did not
time out. Disk SHA-256 before and after IPL was identically
`3697b0e222ba5157748aefe17b37711667d78557033e9937e11cf20bf9aa61f5`.
The source core SHA-256 was
`04b5e95fac6db7284320b1fd32030a0ee39c88e8e53bf6261e9f22cff6fa54d8`.

The profile was one model-2064 ESAME CPU, 256 MiB real storage, 3390 01B9
and 3270 0009. The run used the local Classic compiler, assembler and linker
under `build/compiler/mvs/gcc/` and `build/tools/`, GNU binutils 2.47 under
the Mainframe Lab build tree, and Hercules SHA-256
`080616bd946278ecf08bd7715663592a9bb5bc0619632bd7f2b0c0dba6f8ff77`.
The eight unchanged CMS and TSO stage hashes are pinned in the
[ABI inventory](../architecture/TWO-SPACE-ABI.md). The recipe verifies every
stage hash before IPL. The result is a disposable checked 3390 image.

An earlier fresh run failed because the fixture placed the TSO31 begin in
the CMS31 parameter setup. That aborted subsequent CMS and TSO checks; it
was corrected before the passing diskless and fresh-IPL runs above. This
failure was in the diagnostic fixture, not a changed application binary.

The descriptor is still activated by a private U fixture request. A normal
K launcher must own entry and reject a caller's assertion of personality.
Image, file, lowcore and terminal lifetimes are not yet connected to the
ledger. The current cleanup callback refuses a live allocation instead of
releasing it, so fault unwind, nested application calls and asynchronous
completion remain open. P1 is incomplete; P2–P6 are not qualified by this
result.
