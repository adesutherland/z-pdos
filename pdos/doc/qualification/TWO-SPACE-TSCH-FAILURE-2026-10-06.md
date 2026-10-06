# Two-space subchannel failure discrimination, 6 October 2026

The K channel helper now preserves the condition code from `TSCH`, `STSCH`
and `SSCH` until it branches. Earlier `TSCPOLL`, `TSCDEV` and `TSCSTART`
executed `SR R15,R15` before the branch. That instruction set CC0 and could
make a nonoperational subchannel appear successful. This was a source defect;
the existing positive console/disk runs could not detect it.

For `TSCH`, CC0 means status pending and CC1 means no status pending. CC3
means the subchannel is not operational. The helper now returns status only
for CC0, continues polling only for CC1, and returns failure for CC3 or the
undefined CC2. The synchronous `TSCIO` loop likewise stops immediately on
those invalid results. `TSCSTART` reports a failed `SSCH` rather than erasing
its condition code, and `TSCDEV` does not read stale SCHIB bytes after a
failed `STSCH`. The C31 terminal state machine clears its active read phase
when polling reports a device failure. The architectural meanings are from
[IBM z/Architecture Principles of Operation, SA22-7832-14, TEST SUBCHANNEL](https://www.ibm.com/docs/en/module_1678991624569/pdf/SA22-7832-14.pdf?cp=HW11W).

Diagnostic SVC 222, used only by the two-space fixture, probes absent
subchannel `0x0001ffff`. It checks that `TSCPOLL` returns failure without a
deadline, `TSCDEV` rejects a stale SCHIB sentinel, and both `TSCSTART` and
`TSCIO` reject the start. The initial new control failed on the old
condition-code ordering, then passed after the repair. This is a deterministic
device-absence result; elapsed time is not used to decide success.

The final source-built diskless run passed 87 checks at
`build/pdos/two-space-tsch-complete-build/run/receipt.json`. The fresh
disposable 3390 IPL passed 107 checks at
`build/pdos/two-space-tsch-complete-ipl/run/receipt.json`, including the
absent-subchannel control, the interleaved disk/console operation, real
terminal input, both staged CMS maps, and disabled-wait completion. That
run used the same one-CPU 16 MiB ESAME model-2064/z900-ceiling profile and
pinned v2 CMS24/CMS31 RXVM stages as the preceding disk/console result.
The disk SHA-256 remained
`12965b414df225669b67545592d84e34cfd7fa6c88cf9d6042a5401dcb864d29`;
the input core SHA-256 was
`fe55b431129268dfe3c5c75288063b54057de46109da75cff78ff405ae148ed5`.
Tool and source identities are in the retained ignored build receipts.

This verifies synchronous positive I/O and a nonoperational-subchannel
negative path. It does not establish hot unplug during an in-flight CCW,
request cancellation, per-device queues, real asynchronous interrupt
acknowledgement, multi-CPU coordination, or a production terminal wait policy.
