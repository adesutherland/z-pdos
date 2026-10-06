# K-selected invocation image records, 6 October 2026

This bounded P1 result follows the [CMS](TWO-SPACE-CMS-INVOCATION-2026-10-06.md)
and [TSO](TWO-SPACE-INVOCATION-GATE-2026-10-06.md) guest gates. Private
SVC 235 now accepts a K image-record selector in R0, with R1 and R2 zero.
K resolves that selector to the loaded image's personality, AMODE, owner,
runtime owner and checked image extent. A caller cannot assert a CMS/TSO
mode or an image base. The selected diagnostic records are CMS24 fixed (1),
CMS31 first (2), CMS31 second relocated (3), TSO24 (4), TSO31 (5) and
TSO64 ANY (6). These numbers are fixture controls, not a published
application API. K rejects a selector without a loaded image and a zero
image length.

The guest exercised selector 7 and a nonzero R1 with selector 5. Both
returned status 8. A subsequent valid TSO31 begin, one live page and end
still succeeded, showing the rejected requests did not leave an active
frame. All five CMS and three TSO unchanged RXVM entries retained distinct
tokens and successful end status. The selected SVC 120, CMSCALL, IARV64
and TPUT checks remained green.

`build/pdos/invocation-kselect-1/run/receipt.json` passed 89/89 diskless
checks. The disposable source-built 3390 IPL at
`build/pdos/invocation-kselect-ipl-2/run/receipt.json` passed 142/142
checks. The Classic C31 service is 76,030 bytes, SHA-256
`35cbf88790122b9866affe8c89cb6385ad0e2e9355cad366d2f9927f92f8fb06`.
The source core SHA-256 was
`bf220309758ae01b170167711dcea017719816b8245d4b1772109a0cf1268dac`.
The disk SHA-256 before and after IPL was identically
`bb0bbf7bde3c340d36407249000d4860e4000621f143703581a312110130792e`.
The profile was one model-2064 ESAME CPU, 256 MiB real storage, 3390
01B9 and 3270 0009, using the toolchain and pinned unchanged stages of
the [prior CMS gate](TWO-SPACE-CMS-INVOCATION-2026-10-06.md). The host
exited zero without a timeout.

The first IPL with this same guest code reached its disabled wait, but
Hercules could not save its 256 MiB result core because the host disk had
filled with disposable outputs from this turn. We compressed only the
generated `result.core` and CKD round-trip files in our `invocation-*`
build directories, preserving receipts, logs and recoverable `.zst`
copies. The repeat used the same source core and completed normally.

K still has a fixed diagnostic image registry. It has not yet implemented
a normal K-controlled launch from U PCOMM, generic load handles,
same-personality application calls or full resource and fault ownership.
P1 and later checkpoints remain open.
