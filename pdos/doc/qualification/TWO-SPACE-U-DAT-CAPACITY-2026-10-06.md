# Two-space U DAT capacity, 6 October 2026

The 64 MiB diagnostic profile now gives U a 384 KiB DAT pool at real
`0x180000`–`0x1dffff`. K retains its 512 KiB pool at real `0x100000`–
`0x17ffff`. Independent disk and console channel workspaces move to
`0x1e0000` and `0x1f0000`, still below the 24-bit real limit. The final
2 MiB core contains all four reservations without overlap. No U virtual
address or low 24-bit application placement changes.

The retained host capacity control builds the actual sparse region-first
tables and maps a 16 MiB U31 region at `0x04000000` plus a simultaneous
32 MiB U64 region at `0x0000000120000000`. Its U tables occupy 290,816 of
393,216 bytes, leaving 102,400 bytes for additional table entries. This
is a DAT capacity check, not an application allocation or heap measurement.

The host diskless machine check at
`build/pdos/two-space-upool-384k-regression2/run/receipt.json` passed 87
checks. The fresh disposable 3390 IPL at
`build/pdos/two-space-upool-384k-ipl/run/receipt.json` passed 108 checks,
including guest-built DAT tables, independent channel workspaces, CMS24
and CMS31 image maps, guarded CMS24 stack access, nested overlay restore,
and event-observed completion. Both ran with one model-2064 CPU, 64 MiB
real storage, the named local GNU/Classic toolchains and Hercules. The
IPL source core SHA-256 was
`5f0ee08c8271defae4e3dc755db6bede394871a7b472c541739f10b892659cf4`;
the disposable disk was unchanged after boot. The IPL guest's final U
table use was 122,880 bytes before a wide-heap workload.

The next gate must allocate, touch and release both wide heaps in the
guest while CMS images are mapped. The normal PDOS image still selects
the existing kernel.
