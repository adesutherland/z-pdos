# TSO64 ANY image in shared U after fresh IPL, 6 October 2026

The diagnostic successor now reads the pinned unchanged TSO64 ANY RXVM
native stream from a separate checked `TSO64.RXVM` F/18452 dataset on its
disposable 3390. The version-1 64-byte `PDTSO64` envelope records raw size,
block count and FNV. K checks the first-extent boundary on every record,
all 45 blocks, complete payload, zero padding and AMODE64/RMODE ANY
directory before materializing the image in private K real storage. The
validated low-resident AL4 relocation path is separate from the unqualified
RMODE HIGH AL8 format.

K maps the 767,728-byte image at U `0x09000000`, with 188 separately backed
pages beginning at real `0x4cc000`. The previously checked TSO31 image at
`0x07000000`, both CMS31 images and the fixed CMS24 image remain mapped in
the same U ASCE. Guest checks compare every TSO64 PTE and the complete image
FNV `94d5943a` with the host materializer. The K receipt confirms 45 stage
blocks, entry `0x09000000`, raw FNV `007eda54`, and successful scratch
release. No U page in the 13,914,112-byte 24-bit placement gap was taken.

The final diskless gate passed 89 checks at
`build/pdos/tso64-any-map-build/run/receipt.json`; the Classic C31 service
is 63,006 bytes in the 64 KiB fixture reservation. The fresh IPL passed all
125 checks at `build/pdos/tso64-any-map-ipl/run/receipt.json`, including
unchanged CMS31 IOQUAL, CMS24 IO24 and TSO31 RXVM `-v`, restored low U
headroom, DAT purge accounting, terminal output and disabled-wait completion.
The source core SHA-256 was
`269ffe96c4278d3fd05ffd49fa74ee0a5b24e90b46e2ac120c90da61f9f32bff`.
The disposable disk SHA-256 was
`3cc6df32db24ae25afc6822dad42d48496bbf3c7972c9c978793456a3cc54cd0`
before and after IPL. The profile used one model-2064 ESAME CPU, 256 MiB
real storage, 3390 `01B9`, 3270 `0009`, local GNU binutils 2.47,
maintained Classic C/Assembler/Linker and local Hercules. Guest events
determined completion; elapsed limits only detected stalls.

This proves checked transport, materialization and placement of one TSO64
ANY image beside the selected CMS/TSO31 workloads. It does not execute that
member, supply its 64-bit native services, qualify RMODE HIGH or TSO24, or
select the successor as the normal operating image. The C31 service slot
has 2,530 bytes remaining, so further service growth requires an explicit
K-only capacity and layout change.
