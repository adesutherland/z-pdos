# Checked TSO24 image and low-only failure after fresh IPL, 6 October 2026

The diagnostic two-space successor now reads the selected, unchanged
AMODE24/RMODE24 TSO24 RXVM native stream from a separate `TSO24.RXVM`
F/18452 dataset on its disposable 3390. The `PDTSO24` version-one envelope
records the 1,379,654-byte raw stream, 75 blocks and FNV `add8ef0d`.
K checks the dataset extent and every record, validates the whole envelope
and native directory, and materializes the image in private K real storage
before publishing any U mapping.

The 1,082,128-byte image is fixed for this diagnostic call at U `0x400000`,
with 265 separately backed pages beginning at real `0xa97000`. The full
pre-execution image FNV is `e3810260`, and every page translation matches
the checked backing. CMS24 RXVM remains at its distinct fixed origin
`0x20000`; CMS31, TSO31 and TSO64 images remain in the same U ASCE. The
TSO24 image consumes 1,085,440 bytes of the previously measured
13,914,112-byte low placement gap, leaving 12,828,672 bytes in that gap
before its stack, heap or parameter storage is requested. No K virtual
page is introduced into low U.

The U caller then requested a full 16 MiB of low-only storage through the
selected SVC 120 form. K returned RC 4 with a zero address. It did not
promote the request above 16 MiB, evict another image or change the DAT.
The later independent high-U heap workload still passed. This is an
explicit guest control for insufficient **appropriate** 24-bit virtual
storage despite available wider-mode storage.

The diskless gate passed 89/89 checks at
`build/pdos/tso24-low-error-build2/run/receipt.json`; the Classic C31
service is 65,590 bytes in the 128 KiB K-only slot. The fresh IPL passed
128/128 checks at `build/pdos/tso24-low-error-ipl/run/receipt.json`,
including unchanged CMS24 RXVM/IO24, CMS31 RXVM/IOQUAL, TSO31 RXVM `-v`
and TSO64 ANY RXVM `-v`, pointer/fault/DAT controls and shutdown. Its DAT
purge count was `0x42eba`. The source core SHA-256 was
`6fb9b851291cb3659261d9c11d4799f81525a337296a7dcf0dcd895d32b8fb86`.
The disposable disk SHA-256 was
`1fafb016c19d7fb10e60f2736751eb242ec2985bc97b7e56862b1ac7e320f815`
before and after IPL. The profile was one model-2064 ESAME CPU, 256 MiB
real storage, 3390 `01B9`, 3270 `0009`, local GNU binutils 2.47,
maintained Classic C/Assembler/Linker and local Hercules. Guest completion
events determined success; time limits only detected stalls.

This proves checked transport and simultaneous *image* placement when room
exists. It does not execute the TSO24 entry, allocate its 1 MiB stack and
4 MiB heap, qualify its TSO services, or establish full runtime headroom.
The ordinary loader must return a clear error if a later 24-bit image,
stack or heap cannot fit in appropriate virtual and real storage. The
released one-ASCE image remains selected.
