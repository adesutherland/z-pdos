# Full caller PSW in the invocation ledger, 6 October 2026

The z/Architecture interruption island saves a 16-byte old PSW: an eight-byte
mask at context offset `0x80` and an eight-byte instruction address at `0x88`.
The K invocation ledger previously retained only the mask. It now copies both
halves, all 16 full-width GPRs, the ASCE and the key when SVC 235 begins an
invocation. The private SVC 244 fixture query returns the saved instruction
address only while that invocation is active.

The host C89 invocation control checks distinct high and low address words
across parent suspension, child return and parent resumption. In a fresh
guest, the returned address equals the assembler label immediately after the
parent's SVC 235. The same query returns RC 8 after the parent frame is reaped.
This checks the actual machine old PSW, rather than only a copied host struct.

`build/pdos/full-psw-probe-2/run/receipt.json` passed 97/97 diskless checks.
`build/pdos/full-psw-probe-ipl-1/run/receipt.json` passed 153/153 after a
fresh source-built 3390 IPL. The machine was one model-2064 ESAME CPU,
256 MiB real, 3390 `01B9` and 3270 `0009`. The Classic C, Assembler and
Linker were the maintained z-pdos build binaries; GNU Binutils 2.47 and the
Mainframe Lab Hercules installation were the fixture assembler, linker and
guest. The unchanged CMS24/CMS31 and TSO24/31/64 ANY stage hashes are pinned
in [the ABI inventory](../architecture/TWO-SPACE-ABI.md).

The booted core SHA-256 was
`6755576a08f6413963def16a650ee8ad2e715d14fb289ec70c72778fc88da5a9`.
The stopped disk SHA-256 was
`4cd5d9aa81ab9be83a16fecafc278260dbf20b6164701b5c9d372df9c4645cd7`
both before and after IPL. Host exit was zero, with no timeout.

SVC 235 still surrounds a U-side manual call. K has not yet transferred
control into the child or restored this saved continuation on a child fault.
The existing fixed-PC recoverable fault fixture is separate. General
recoverable invocation unwind, normal K-controlled launch and unchanged
same-personality application calls remain open P1/P3 acceptance gates.
