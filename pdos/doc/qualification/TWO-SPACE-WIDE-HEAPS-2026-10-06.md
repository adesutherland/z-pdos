# Two-space concurrent wide heaps, 6 October 2026

The 64 MiB successor now exercises real U storage, beyond the earlier DAT
capacity probe. A U64 fixture requests a 16 MiB U31 heap through the
selected conditional SVC 120 convention and a 32 MiB U64 heap through
internal diagnostic SVC 223. Both remain allocated at once. The U program
writes and reads the first and last byte of each, frees both, and verifies
their return codes. It also rejects a stale second free and a request with
a nonzero high half in its length. SVC 223 accepts only AMODE64 callers and places storage
starting at `0x0000000120000000`; it is not an IBM compatibility API. It
keeps the full address in the K descriptor and return register rather than
truncating it through Classic C31.

The fresh IPL mapped the unchanged CMS24 and CMS31 RXVM images and the
backed CMS24 stack before the wide allocations. The U31 heap landed at
`0x0340b000` because the live CMS31 placement occupies part of the lower
31-bit range. The U64 heap landed at `0x0000000120000000`. Guest U tables
grew to 319,488 of 393,216 bytes. The host checked both heap endpoints
were unmapped after free and counted every live map/unmap purge. This is
evidence that these *specific* simultaneous allocations fit while both
CMS images are mapped; the RXVM programs themselves were not entered.

The diskless host gate passed 89 checks at
`build/pdos/two-space-wide-heaps-build4/run/receipt.json`. The fresh
disposable 3390 IPL passed 111 checks at
`build/pdos/two-space-wide-heaps-ipl2/run/receipt.json`. The latter used
one model-2064 ESAME CPU, 64 MiB real storage, 3390 `01B9`, 3270 `0009`,
the pinned CMS24/CMS31 images, the named local GNU/Classic build tools and
Hercules. The core SHA-256 was
`6c4367a2147e519513adbf1e785538ea4a92720e425e4d75bd7eae19d5d6ab01`;
the disposable disk SHA-256 after IPL was
`bd29b12f1737899663d46b5ad9cb7b1d6236664628a0d4e60a70d4262e17e69f`,
unchanged from before IPL. Completion was observed through the guest
disabled-wait event after the terminal input gate; elapsed limits served
only as stall watchdogs.

This does not qualify unchanged CMS/TSO application execution, IBM high
storage calls, maximum heap size, or the replacement image route. The
constrained 24-bit virtual placement gap remains 13,914,112 bytes after
the CMS24 image and reserved stack; wide heaps use independent U virtual
regions and real backing.
