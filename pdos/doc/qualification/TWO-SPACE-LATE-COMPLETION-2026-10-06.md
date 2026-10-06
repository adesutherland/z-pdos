# Retired terminal completion and new owner, 6 October 2026

This P1 diagnostic checkpoint retires a parent's in-flight READ MODIFIED
through checked CSCH/TSCH completion, then begins a distinct TSO31 invocation
and its phase-one terminal read. Private SVC 248 presents the retired
`(invocation token, I/O handle)` to the K completion ledger. The old token
must be stale; the new owner's token, terminal lease and phase must remain
unchanged. The request is also rejected when there is no new owner. The guest
then polls the new read, sees phase one without stale AID, cancels it and ends
the invocation. No delay decides any of these results.

`build/pdos/late-completion-1/run/receipt.json` passed 99/99 diskless checks.
The source-built fresh 3390 IPL in
`build/pdos/late-completion-ipl-1/run/receipt.json` passed 156/156. The
no-owner injection returned 8; the new invocation began with token 9; its
injected old completion and end returned 0; the retry poll and phase both
reported 1. The failed-start, nested child ownership, pending-I/O clear and
normal application checks remained passing. Host exit was zero, with no
timeout. The boot DAT checksums and disk image were unchanged.

The machine was one model-2064 ESAME CPU with 256 MiB real storage, 3390
`01B9` and 3270 `0009`. Classic C, Assembler and Linker came from this
z-pdos checkout; GNU Binutils 2.47 and Mainframe Lab Hercules built and ran
the fixture. The unchanged CMS24/CMS31 and TSO24/31/64 ANY stage hashes are
recorded in the [ABI inventory](../architecture/TWO-SPACE-ABI.md). The booted
core SHA-256 was
`66e78393328ddae43c49218dc50c9762ef20b2db1e21d32f48461e7e08b46258`.
The 3390 SHA-256 was
`718ed9fa843d6e158de3e88dba30264d81073576faf8174bdfb79e3d98e61f14`
before and after IPL. Hercules SHA-256 was
`080616bd946278ecf08bd7715663592a9bb5bc0619632bd7f2b0c0dba6f8ff77`.

SVC 248 is a deterministic K ledger injection. It does not emulate a
physically late device status after CSCH, prove all channel-status orderings,
or replace the native CMS/TSO call tests. P1 remains open.
