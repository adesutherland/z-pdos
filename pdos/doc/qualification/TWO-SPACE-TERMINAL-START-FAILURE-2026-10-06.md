# Failed terminal start and immediate retry, 6 October 2026

The diagnostic fixture now exposes one private failure injection. After the
3270 driver observes a distinct `K FAIL READY` screen and sends a real AID,
K substitutes a verified absent subchannel number for one READ MODIFIED
SSCH. It keeps the real terminal identity for cancellation. A failed start
must clear or quarantine its channel workspace, release the pending read
state, and return RC 12. The fixture then starts a fresh read and cancels it
successfully. The injection selector and screen code are private controls,
not a CMS or TSO application ABI. Guest AID and channel completion events
advance the test; clock bounds detect only a stall.

The diskless Classic C31 and machine gate at
`build/pdos/start-failure-1/run/receipt.json` passed 95/95 checks. The
source-built fresh 3390 IPL at
`build/pdos/start-failure-ipl-1/run/receipt.json` passed 150/150. The guest
recorded injection RC 0, screen RC 0, initial read start RC 0, failed
READ MODIFIED RC 12, fresh read start RC 0 and cancellation RC 0. The third
terminal input was observed by the driver. Source core SHA-256 was
`e760734c325a7bf771c0a6eb52c5996be149285e992573a5f977a315559f1e11`.
Disk SHA-256 was
`60d357db5eb73a77cdd0a8854b58baa09c8da741122fc57090d1fcfc21831f2a`
both before and after IPL. Host exit was zero without timeout.

The machine was one model-2064 ESAME CPU, 256 MiB real, 3390 `01B9` and
3270 `0009`, using the pinned toolchain and unchanged stages in the
[invocation gate](TWO-SPACE-INVOCATION-GATE-2026-10-06.md). The check does
not claim a spontaneous hardware failure or an invocation-owned pending I/O
operation: this read runs without a native frame. Late completion after an
owner exits and child-fault unwind remain open P1 controls.
