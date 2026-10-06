# Two-space CMS24 fixed-origin child checkpoint, 6 October 2026

The same source-built, disposable 3390 successor used local Mainframe
Classic C/assembler/linker, GNU binutils 2.47 s390 tools and Hercules
4.9.1 on a one-CPU ESAME model 2064 with 16 MiB. It loaded the pinned
unchanged CMS24 and CMS31 RXVM v2 stages into independent U intervals.

K temporarily replaced the mapped CMS24 RXVM backing at U `0x20000` with
a 12-byte executable child. A U64 caller branched to it. The child selected
AMODE24, entered K with SVC 219, received the mode-24 result, set a return
value of `0x2468`, selected AMODE64 and returned to its U64 caller. The pop
service propagated that value and restored the RXVM PTEs; both the U caller
and host core snapshot observed the original RXVM word afterward. The
CMS31 AMODE31 same-origin child check still passed in that IPL. The full
event-driven IPL gate passed, including CKD, terminal, storage, isolation
and fail-stop controls.

The child is a minimal instruction sequence, not a CMS24 application. No
unchanged RXVM execution, CMS lowcore/file API, native CMS-to-CMS calls, TSO24
qualification or production heap measurement follows from this check.
