# Invocation-owned READ MODIFIED across a child, 6 October 2026

The diagnostic guest now starts a TSO31 parent invocation after its checked
image is loaded, shows a distinct owner prompt, and waits for a fourth real
3270 AID. K submits READ MODIFIED and records a pending I/O handle under the
parent token. The guest observes phase two before it enters a nested child.
The child cannot cancel the parent's read. Ending the child restores its
caller; ending the parent clears the subchannel, completes and forgets the
pending handle, releases the terminal lease and ends the frame. A fresh read
then polls pending in phase one, with no stale attention from the parent's
request. K uses the clear completion event to release the shared channel
workspace. The terminal driver's wall bounds only detect a stalled run.

The diskless Classic C31 and machine gate at
`build/pdos/owned-inflight-1/run/receipt.json` passed 96/96 checks. The
source-built fresh 3390 IPL at
`build/pdos/owned-inflight-ipl-1/run/receipt.json` passed 152/152.
The guest recorded parent phase 2, owner screen RC 0, child cancellation
RC 8, child and parent end RC 0, and fresh poll/phase `1/1`. The K image
lease counts remained `1, 2, 1, 0`. Source core SHA-256 was
`a0064c86efe157a33b2ecbcdb7b74f63c25fccad568163103db3cc75b5fac760`.
Disk SHA-256 was
`7f07f264ed5bc69f0c8c5a30ffb567ec14bc2040a5fc0c652f6fa1c118883bbe`
both before and after IPL. Host exit was zero without timeout.

The machine was one model-2064 ESAME CPU, 256 MiB real, 3390 `01B9` and
3270 `0009`, with the pinned toolchain and unchanged stages in the
[invocation gate](TWO-SPACE-INVOCATION-GATE-2026-10-06.md). This is a nested
K descriptor probe around one cached image. It does not show two native
applications calling one another or prove a physically late hardware
completion after the owner has exited. Recoverable child-fault unwind and
normal K-controlled launch remain open P1/P3 work.
