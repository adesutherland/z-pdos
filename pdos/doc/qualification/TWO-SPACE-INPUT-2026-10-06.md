# Two-space K terminal input checkpoint, 6 October 2026

The successor now receives a 3270 Enter record through K after the fresh
3390 IPL. SVC 209 arms a single-user terminal read; SVC 210 polls for device
attention, then K issues a format-1 `READ MODIFIED` CCW using the reserved
real `0x180000`–`0x18ffff` channel workspace. A complete record is copied
through the checked K/U gate into a 256-byte U buffer. Its first four bytes
give the record length, followed by the raw 3270 AID record. An independent
`s3270` client submitted `PING`; U received ten bytes:
`7d 5b e5 11 5b 61 d7 c9 d5 c7` (Enter, cursor and SBA, then the EBCDIC
command). The K service and U caller both returned normally.

The first IPL exposed a 3270 timing detail: a `READ MODIFIED` started before
the user's AID returned a three-byte `NoAID` record. The service now waits
for attention before starting the read, and treats initial IRB status as
pending. A completed input record stays in K until its validated copy to U
succeeds; a failed destination copy can be retried. The latter retry branch
has not yet been exercised in the guest.

The host C89/ASAN/UBSAN channel check passed the real-address CCW and IRB
status controls. The diskless machine gate passed 80 checks with an
unavailable-console result. The fresh disposable IPL passed 92 checks,
including screen output, host input, full-width U copy, earlier dataset and
storage services, DAT isolation and fault recovery. The disk was unchanged.
This is one ESAME model-2064 CPU with 16 MiB real storage, z900 ceiling,
3390 at `01B9` and 3270 at `0009`, on macOS arm64 using Mainframe Classic
C/Assembler/Linker, GNU Binutils 2.47 and Hercules 4.9.1.0-SDL. Exact
producer and tool hashes are in ignored
`build/pdos/two-space-s5-input-final/run/receipt.json` and
`build/pdos/two-space-s5-input-final-ipl/run/receipt.json`.

| Artifact | SHA-256 |
| --- | --- |
| Source-built 2 MiB core | `35403622dfefd301e2bb0b4656d4257ca294639238ebc3bd5e1ada5507d1d028` |
| Classic C31 service, 27,086 bytes | `297bc160cc49d913234d5a26192f6e43bec10244cd04694051fed6e50f010177` |
| Checked 3390 CCKD disk | `f27fe8e7bb57427d79d626d0b1e6885cb9b29161185c6be80771210cafd3338b` |

This proves bounded terminal input transfer in the diagnostic image. There
is no command interpreter or general console API, no CMS/TSO dataset API,
and no unchanged application ran under this successor. Real CMS24/TSO24
image, stack and heap headroom remains unmeasured. PD-003 slices 5–7 remain
open, and the one-ASCE release image stays selected.
