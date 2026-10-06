# Bounded native TSO loader core, 6 October 2026

The two-space successor now has a C89 native load-module inspector and
AMODE31/RMODE ANY materializer independent of the released `pdosutil` loader.
The code follows Paul Edwards's public-domain format handling but uses no
host allocator, `printf`, C runtime service, or U pointer. It validates the
bounded record envelope and directory mode before writing a K-owned image,
checks CESD section ranges and RLD targets, and requires the termination
record immediately before the native trailer. The caller must keep this
scratch image private until all checks pass; a rejected input can have
partly written scratch bytes, never a published U mapping.

The inputs were the unchanged source-built beta 3 native RDW streams:
`RXV31.rdw` SHA-256
`77b689a3c64bad63996031654dfc353d357b9c2616da01c65a9ea3e2c09c2cc9`
and `RXV64.rdw` SHA-256
`dc5210b04cabf93d22942b0955ccf9bf63f7e0d4334a88e2a04e8705484339b6`.
The 1,395,270-byte TSO31 stream has 1,429 native records, AMODE31/RMODE ANY
directory flags `0x12`, and a zero image-relative entry. The materializer
produced 1,094,960 image bytes. At bases `0x05000000` and `0x07000000`, its
FNV values were `f4b21310` and `3ce51566`, respectively. The host control
also compared all 1,094,960 bytes at each base directly with the current
released loader on the same input. The 821,446-byte TSO64 ANY stream
has 425 records and flags `0x11`; only its header was inspected here. Malformed
length, magic, mode, entry, section range, truncation and capacity controls
were rejected in the sanitizer-backed host test.

The repeatable check is `pdos/scripts/two-space-tso.crexx` with those two
absolute pinned input paths and the maintained Classic tool paths. Its final
output is under `build/pdos/tso-successor-reviewed/`; it passed the host
ASAN/UBSAN test and assembled and linked the same loader source with Classic
C/Assembler/Linker for the 31-bit z/Architecture target. This was a host and
target-build proof, not a guest IPL. It did not map the image in U, invoke the
TSO program, provide its MVS-style SVCs, load TSO64, or qualify TSO24.
