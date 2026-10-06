# Two-space CMS24 full-stage guest checkpoint, 6 October 2026

The successor fixture was rebuilt with the GNU binutils 2.47 s390-linux-gnu
assembler/linker, the local Mainframe Classic C compiler, assembler and
linker, and Hercules on the single-CPU ESAME model 2064, 16 MiB, 3390
profile. The source-built PLOAD/PDOS disposable IPL included the pinned
v2 `CMS24.RXVM` stage from
`build/pdos/cms-module-recipe-c/media/cms24-RXVM.bin` (SHA-256
`04550b86d7bdfa3c9060d9fa154875adf3ccb677d95ba13290836ddc02a8da9e`).

K found the checked first extent, read 92 complete F/18452 records into a
temporarily reserved, K-only high-real buffer, validated the full MODULE
framing and FNV, and released the reservation. The guest result recorded
zero from SVC 213 and 92 blocks; the full fresh IPL machine gate passed.
The diskless gate passed with the same service source and correctly reported
the absent dataset. The new IPL harness used the observed 3270 ready screen
and the guest's disabled-wait event to decide when to send input and save the
core. A 60-second watchdog detects a stalled guest without making successful
execution depend on a fixed pause or poll count.

The source-built guest remains a diagnostic 16 MiB fixture. It has not
mapped or executed RXVM in U, run unchanged CMS or TSO applications, or
selected this kernel as normal `PDOS.SYS`. Those are separate PD-003 gates.
