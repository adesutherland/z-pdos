# Two-space channel completion checkpoint, 6 October 2026

The K-owned synchronous channel helper previously used a fixed count of
`TSCH` polls. It now accepts only the subchannel completion status as
success. `STCK` supplies a roughly 67-second failure watchdog; elapsed time
does not determine when a successful operation is ready. The two aligned TOD
slots occupy the unused K IRB workspace after the architectural IRB, before
the CCWs. All CCW and data addresses remain K-owned low real addresses.

The Classic z900 assembler accepted the revised helper. The diskless
source-built machine gate passed. A fresh, disposable 3390 IPL with pinned
CMS24 and CMS31 RXVM v2 datasets passed the complete successor gate:
CKD reads, 3270 output/input, CMS31 U mapping, nested AMODE31 SVC and parent
restoration all completed on channel status. Hercules console completion and
core capture use observable events with watchdogs for stalls.

This check covers the one-CPU diagnostic fixture and its selected devices.
It does not prove a production channel scheduler, cancellation of a stalled
I/O, multi-CPU behavior, or unchanged application qualification.
