# Two-space disk and console workspace isolation, 6 October 2026

The single-CPU, 16 MiB successor now reserves two K-only low-real channel
workspaces: `0x180000`–`0x18ffff` for 3390 disk operations and
`0x190000`–`0x19ffff` for the 3270. Both are inside K's real aperture and
outside U's real-frame inventory. The C31 service keeps a separate `TSCSTATE`
for each device. A pending terminal AID/read therefore retains its ORB, CCW,
IRB and data while a dataset service prepares and runs disk I/O.

The supplied-storage boot ledger reserves both 64 KiB ranges before building
DAT; the fixture rejects a table-pool overlap. The host channel control
prepares a console read, builds a disk read in the other workspace, and checks
that the console status/data and both distinct CCW addresses survive. A fresh
source build of the diskless fixture passed 86 machine checks. The guest
fixture calls the K dataset service after terminal-read start and before its
completion poll; the fresh 3390 IPL passed all 106 checks, including disk
completion, terminal input transfer, distinct real ORBs, image backing,
context restoration and the final disabled-wait event.

Inputs and outputs are under ignored build directories:

- `build/pdos/two-space-dual-io-final-build/`, including `run/receipt.json`
  and the source-built core.
- `build/pdos/two-space-dual-io-final-ipl/`, including `run/receipt.json`,
  Hercules log, complete 16 MiB result core and disk readback.

The IPL profile was Hercules 4.9.1, ESAME model 2064 with a z900 instruction
ceiling, one CPU, 16 MiB real storage, source-built PLOAD and C31 stage,
disposable 3390 `01B9`, 3270 `0009`, and the pinned v2 CMS24/CMS31 RXVM
stage files named in `two-space-ipl.crexx`. The receipt records the exact
tool and source hashes. Disk SHA-256 before and after IPL was
`9b69d7dd79a4e2963a53543681c4948af1f71134a7d5166587753d7a7c3e63af`;
the input core SHA-256 was
`2dc32854001346c08e1c6806405f0914f5c87af2e674995f27c7836046ec2b1a`.

The first new IPL oracle incorrectly assumed that U virtual `0x180000` was
unmapped; CMS24 legitimately maps that virtual page to a different real
frame. The corrected check inspects the two real ORBs and separate CCWs. A
subsequent interleaving attempt inserted a second traced SVC and wrote an
eleventh trace record over neighboring diagnostic fields. The final fixture
uses the existing untraced dataset selector for that repeat operation.
These were test-oracle and trace-layout failures, not a demonstrated K
interruption failure.

This proof removes one concrete cross-device memory race. It does not yet
provide per-device request queues, cancellation, multi-CPU serialization,
general CMS/TSO dataset APIs, unchanged application execution, or a selected
normal replacement image. Hardware I/O and external interruption delivery
still require their own qualification; the guest currently uses bounded
subchannel-status polling for synchronous transfers.
