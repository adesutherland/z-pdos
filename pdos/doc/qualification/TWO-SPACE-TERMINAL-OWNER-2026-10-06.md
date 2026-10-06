# Invocation-owned terminal read, 6 October 2026

The selected terminal input request now takes a K invocation lease before it
waits for 3270 attention. K records the owner token and, once READ MODIFIED
starts, a separate pending-I/O handle. A nested child cannot cancel or poll
its parent's read. On owner return, K clears the subchannel and waits for the
clear completion event before it releases the lease or the real channel
workspace. An unsuccessful clear quarantines that workspace and prevents a
new read. The clock in the channel helper only detects a stalled clear; it
does not establish successful completion. A completed read releases its I/O
handle, and a failed U copy leaves the result available for its owner to
retry. The private diagnostic cancellation selector is not an application
ABI.

The diskless Classic C31 build and machine gate at
`build/pdos/terminal-owner-2/run/receipt.json` passed 92/92 checks. The
source-built fresh 3390 IPL at
`build/pdos/terminal-owner-ipl-2/run/receipt.json` passed 145/145. The guest
control starts a parent-owned read waiting for attention, observes that a
nested child cannot cancel it, ends the child and parent, and starts and
cancels a read for a new owner. Existing unchanged CMS24, CMS31, TSO24,
TSO31 and TSO64 selected entries passed in the same IPL. Source core SHA-256
was `1ea65ba483532cb96af358bcc7498710790a86c88b2a55a458dcae3b8b85f0cb`.
Disk SHA-256 was
`ae17b9cd19ee403653db2feeda9c20dd92942e9a3f4684e4ad90759eb1a60b3d`
both before and after IPL. The host exited zero without a timeout.

The guest was one model-2064 ESAME CPU, 256 MiB real, 3390 `01B9` and 3270
`0009`. The recipe used the same pinned Classic compiler, assembler, linker,
GNU binutils 2.47, Hercules and unchanged application stage hashes recorded
in the [invocation gate](TWO-SPACE-INVOCATION-GATE-2026-10-06.md) and
[ABI inventory](../architecture/TWO-SPACE-ABI.md).

This guest control covers cancellation while waiting for attention. An
in-flight READ MODIFIED cancellation, late status after cancellation and
retry still require a separate guest control. It also does not prove a native
parent application entering a native child or the abnormal-exit unwind.
