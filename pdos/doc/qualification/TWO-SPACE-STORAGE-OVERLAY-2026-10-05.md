# Two-space storage and overlay checkpoint, 5 October 2026

This checkpoint extends the bounded K64/C31/shared-U successor. It implements
one selected K storage service and the page-backing operation needed for a
fixed-origin nested call. It does **not** complete PD-003 slices 5–7 or select
the successor as the operating system. No unchanged CMS or TSO binary ran in
this image.

## Implementation

`twospace_memory.c` owns a supplied 16 MiB real-frame ledger, a shared-U
interval ledger and the live U DAT map. K reserves real 0–2 MiB for its core,
tables and fixture pages. A requested U interval receives separate real
backing from 2–16 MiB. Allocation first reserves the virtual and real spans,
then maps zeroed pages; failure rolls back the new mappings and reservations.
Free preflights every translation, unmaps and zeroes each backing frame, and
releases both ledgers. The privileged C31 `SSKE` helper gives each new U real
frame key 8 before it is exposed, and returns it to key 0 after unmap. A live
map change calls the single-CPU `PTLB` hook.
No real frame containing K code, tables or context is returned as U storage.

The Classic C31 service now handles the fixture's conditional SVC 120
GETMAIN/FREEMAIN register subset. It reads the saved caller PSW and R15 from
K's context frame, and gives the 64-bit nucleus a K-owned result descriptor;
the nucleus restores the full U address into R1. Below-line requests search
from U `0x20000`, skipping registered images; above-line requests start at U
`0x02010000`. A wider request does not fall back below the 16 MiB line. The
fixture's AMODE24 call received U `0x22000`, backed by real `0x200000`;
the AMODE64 caller received U `0x02010000`. A 16 MiB request exhausted the
named real profile, returning RC 4 and R1 zero. Both successful allocations
were written by their problem-state U callers and then freed. A 64-bit length
with a nonzero high half returned RC 8 instead
of being truncated to 4 KiB. Four live page changes invoked four `PTLB`
callbacks.

The host control registers the actual page-rounded CMS24 RXVM image interval,
U `0x20000`–`0x1bafff` (1,683,456 bytes). With only that image and the
proposed U compatibility page at zero, 15,089,664 of the 16,777,216 low
virtual bytes are unreserved before stack and heap allocations. This is an
interval calculation, **not** a CMS24 guest heap measurement. The fixture
itself registers only its two low U pages at `0x20000` and `0x21000`.
TSO24 image and heap sizes remain unknown. Real-memory headroom is separate
from these virtual counts.

`TSMOVERLAYPUSH` swaps a fixed U interval to distinct child real frames while
retaining the caller's frames. `TSMOVERLAYPOP` restores the exact caller map,
zeros and releases the child frames, and propagates the supplied child RC.
The host control exercises two nested overlays, verifies the original bytes
after both returns, and refuses a free of the suspended caller. The operation
currently requires the child to use precisely the parent's interval. There
is no CMS/TSO MODULE parser, command dispatcher or native application call using
it yet.

## Machine and evidence

The named profile is one model-2064 ESAME CPU, 16 MiB real storage, z900
instruction ceiling and a disposable 100-cylinder 3390 at `01B9`. The host
is macOS arm64. The build uses Mainframe Classic C, Classic Assembler and
Classic Linker for C31 code, GNU Binutils 2.47 for the K64 fixture, and
Hercules 4.9.1.0-SDL. Exact executable and input hashes are in ignored
`build/pdos/two-space-s57-key3/run/receipt.json` and
`build/pdos/two-space-s57-key3-ipl/inputs.sha256`.

The diskless machine gate passed 71 checks. Its host controls include
ASAN/UBSAN allocation rollback, modes 24/31/64, frame rights, no low fallback,
storage-key callbacks, cross-page transfer, nested overlay restore and return
code. The fresh 3390
IPL passed 79 checks. The guest stage recorded CRC32 values of its K and U
DAT pools before launch; they match the host-built reference. The runtime
then made four live page changes. At shutdown both temporary U PTEs are
invalid again, and the final U table pool equals the original reference.
The IPL disk hash did not change.

| Artifact | SHA-256 |
| --- | --- |
| Source-built 2 MiB core | `cba3a45b5e7b623c5eca419bae2582abe400a0378f0e52c6f4e14876b23eadde` |
| Classic C31 service binary, 18,470 bytes | `773b137b0615f96c6b4c1fd6f91091e90b3eae716dcb06a265d96526fbd75caf` |
| Checked `KCORE.BIN` package | `eca35864ec53006b0ab008709c6ccfaf0083aeb25649b6c9cf8817aa71b55e04` |
| Native C31 IPL stage | `232d2b0cc1b118ccef205c74bde04778a1a676c5920dea37eec953ac1bef6c3b` |
| Checked 3390 CCKD disk | `19bf6c92d4950c95a985baec73d87401bc174d9e87ac5632543509e272a42e28` |

Reproduce in fresh ignored `build/pdos/` directories using the exact tool
paths recorded in the receipts:

```sh
crexx -nokeep pdos/scripts/two-space-next.crexx --args \
  build/pdos/two-space-new "$GNU_AS" "$GNU_LD" \
  "$CLASSIC_CC" "$CLASSIC_AS" "$CLASSIC_LD" "$HERCULES"
crexx -nokeep pdos/scripts/two-space-ipl.crexx --args \
  build/pdos/two-space-new-ipl \
  build/pdos/two-space-new/run/image.core \
  "$GNU_AS" "$GNU_LD" "$HERCULES_BIN_DIR"
```

## Replacement boundary

The new storage path is a bounded SVC 120 subset. Channel and dataset,
terminal and command services still live in the one-ASCE kernel. The successor
has no actual CMS/TSO loader, relocation, native application-call gate, normal
replacement-image selector or unchanged-application qualification. Its
external and I/O entry tests are synthetic; 24-bit real channel buffers and
native TSO24 memory remain unaudited. PD-003 slices 5–7 stay open.
