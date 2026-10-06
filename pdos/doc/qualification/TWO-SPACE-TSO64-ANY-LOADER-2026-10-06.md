# Bounded TSO64 ANY image materializer, 6 October 2026

The successor's C89 native loader now accepts the selected classic
AMODE64/RMODE ANY member through a separate `TSTIMAGE64ANY` entry. This
member is **low resident**: its RLD records use the same 32-bit adjustments
as the checked TSO31 member, although its entry runs in AMODE64. The
independent RMODE HIGH member uses 64-bit relocation fields and remains a
separate loader task. The mode flag is checked before materialization, so a
31-bit member cannot enter the 64-bit route or vice versa.

The pinned unchanged `RXV64.rdw` is 821,446 bytes, 425 records, directory
flag `0x11`, and SHA-256
`dc5210b04cabf93d22942b0955ccf9bf63f7e0d4334a88e2a04e8705484339b6`.
At low U bases `0x09000000` and `0x0b000000`, the materializer produced
767,728 bytes with FNV values `94d5943a` and `1c28a98a`. The host control
compared every byte at both bases with the released `fixPEMode` loader on the
same input. It also rejected the wrong mode, an invalid low origin, short
capacity and truncated input. The existing TSO31 two-base comparison and
malformed controls still pass.

`crexx -nokeep pdos/scripts/two-space-tso.crexx` with the two pinned
absolute RDW paths and maintained Classic tools passed ASAN/UBSAN host
checks and assembled/linked the updated source for C31 under
`build/pdos/tso64-low-final/`. The reference `pdosutil.c` compile emitted
its existing 64-bit-host pointer-to-`int` warning; byte comparisons and
sanitizer checks passed. This is host and target-build evidence. TSO64
staging on 3390, U mapping, full-width dispatch, SVCs and guest execution
remain unqualified.
