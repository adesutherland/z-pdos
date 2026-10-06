# TSO31 native image in shared U after fresh IPL, 6 October 2026

The diagnostic successor now reads the pinned unchanged TSO31 RXVM native
record stream from a checked `TSO31.RXVM` F/18452 dataset on a disposable
3390 image. A 64-byte stage envelope beginning `PDTSO31` and version byte
`0x01` records raw size, block count and FNV.
K checks the first-extent boundary on every record, validates all 76 blocks,
the complete raw payload, trailing zero padding and native AMODE31/RMODE ANY
directory before building an executable image in private K real storage.
The materializer writes no U address until its checks pass.

K places the 1,094,960-byte materialized image at U `0x07000000` with 268
page-rounded, independently backed pages. The existing CMS24 fixed image at
`0x20000` and two CMS31 images at `0x03000000` and `0x05000000` remain mapped
in the same U ASCE. K has no mapping in U's 24-bit placement gap. The guest
receipt checks every TSO31 PTE, entry `0x07000000`, all image bytes through
FNV `3ce51566`, the raw FNV `5db419ae`, and 76 checked disk blocks. K
releases both private raw and materialization buffers after copying into U.
Its C31 service is 60,926 bytes inside the current 64 KiB fixture reservation.

The final diskless core gate passed 89 checks at
`build/pdos/tso31-shared-u-final-build/run/receipt.json`. A fresh IPL passed
124 checks at `build/pdos/tso31-shared-u-final-ipl/run/receipt.json`, including
unchanged CMS31 IOQUAL and CMS24 IO24 results, the restored 13,914,112-byte
low U placement gap, DAT purge accounting, disabled-wait completion and disk
immutability. The source core SHA-256 was
`7aefd1a3ed5ea65de54705e4261d02bb8a550c343a1d378fbd7aac552f1da518`.
The disposable disk SHA-256 was
`d564869269b11e9eedbbc86596f635bac1620e77bc86ac58683701cc16486aae`
before and after IPL. The profile used one model-2064 ESAME CPU, 256 MiB real
storage, 3390 `01B9`, 3270 `0009`, local GNU binutils 2.47, maintained
Classic C/Assembler/Linker and local Hercules. Guest events determined
completion; elapsed time was only a failure watchdog.

This proves checked transport, materialization and placement of one native
TSO31 image beside CMS images. It does **not** execute RXVM or supply its
MVS-style service API. TSO64 image loading, native TSO24, TSO-to-TSO calls,
persistent CMS output and the normal replacement image remain
unqualified. The 64 KiB C31 fixture reservation now has limited headroom;
further services need an explicit K capacity/layout change, not use of low U
virtual storage.
