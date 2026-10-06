# Post-start terminal cancellation and retry, 6 October 2026

The diagnostic guest waits for a real 3270 AID, submits READ MODIFIED, and
records terminal phase two before it consumes the result. It then asks K to
clear the subchannel. Only the clear completion event releases the channel
workspace. K redraws a distinct retry prompt, and the terminal driver sends
a second `PING` only after observing that prompt. A new read must return the
second AID record. Guest events advance this test; its clock only fails a
stalled run. The phase and cancellation selectors are private fixture
controls, not application services.

The first fresh IPL exposed an error in the redraw path: the terminal was
already enabled, and repeating ENABLE SUBCHANNEL returned a nonzero condition.
The captured guest reported phase 2, clear RC 0 and redraw RC 23. K now
enables the device only on the first screen request. This corrects the
failure without changing the channel-clear completion rule.

The final diskless Classic C31 and machine gate at
`build/pdos/terminal-retry-3/run/receipt.json` passed 93/93 checks. The
fresh source-built 3390 IPL at
`build/pdos/terminal-retry-ipl-2/run/receipt.json` passed 147/147.
The guest recorded phase 2, clear RC 0, redraw RC 0, restart RC 0, final
read RC 0 and a ten-byte second AID record; the second terminal entry was
observed by the driver. Source core SHA-256 was
`6d5b003740bb2951b6e516e37e97bf3e526c9822f8d4ea6a859a9e655cfacfb3`.
Disk SHA-256 was
`51a56f14d2cdfb6bf19ff10d851c88d04943375bc3b705378e821431ffe65b22`
before and after IPL. The host exited zero without a timeout.

The machine was one model-2064 ESAME CPU, 256 MiB real, 3390 `01B9` and
3270 `0009`, with the pinned toolchain and unchanged stages in the
[invocation gate](TWO-SPACE-INVOCATION-GATE-2026-10-06.md) and
[ABI inventory](../architecture/TWO-SPACE-ABI.md).

This proves a real post-start clear and a fresh retry in one ordering. The
first READ MODIFIED may have completed in the channel before K requested its
clear; the guest observes K phase, not the physical in-flight interval.
Injected late completion after owner exit, failed channel start and
recoverable child fault still need guest controls. The first read here has no
native application invocation; [the separate owner gate](TWO-SPACE-TERMINAL-OWNER-2026-10-06.md)
proves ownership while waiting for attention.
