# Two-space guarded CMS24 stack, 6 October 2026

The successor's CMS24 fixed image now receives a real-backed U stack in
the same 24-bit virtual interval already reserved for it. The page at
`0x00f00000` remains unmapped as a guard. K allocates and keys 255 pages
(`0x000ff000` bytes) from `0x00f01000` through `0x00ffffff`, then maps
them in U with live DAT invalidation. The allocation is conditional on the
validated fixed CMS24 image and has rollback paths for failed stack or
image allocation. It does not consume the contiguous placement interval
between RXVM's page-rounded end `0x001bb000` and the guard at `0x00f00000`:
the interval remains 13,914,112 U virtual bytes. This is placement
headroom, not a measured malloc heap.

The same-origin AMODE24 child writes `0x5a` at U virtual `0x00fff000`
and returns to its U64 caller. The IPL oracle walks all 255 stack PTEs,
checks the guard is absent, checks real backing and the child write, and
checks both CMS images remain independently backed. The stack pages also
account for 255 additional single-CPU `PTLB` callbacks. The first new
run failed only the old exact callback-count oracle; the corrected oracle
passed a second fresh IPL of the same source-built image.

The build receipt is at
`build/pdos/two-space-cms24-stack-build/run/receipt.json`. The verified
IPL receipt is at `build/pdos/two-space-cms24-stack-ipl-verified/receipt.json`:
108/108 checks passed, including the separate 3270 terminal input, disk
read, absent-device control, mode24 nested return and disabled-wait
completion. The run used the disposable one-CPU 16 MiB ESAME model-2064
profile, source-built PLOAD and C31 stage, and pinned unchanged v2 CMS24
and CMS31 RXVM stages. Disk SHA-256 before and after IPL was
`9a2a99a65cb08c849163216b241a1e6bca2974ac29983b1a05479e8097898080`;
input core SHA-256 was
`b7fad65bdf100501bac56dfc88f643a9e5199167600dcef1d1cbcd2297914098`.

This proves the stack translation, storage key, write access and guard for
the tiny child. The unchanged CMS24 RXVM image is still mapped but not
entered. Its actual stack and heap demand, native TSO24 placement, wider
31/64-bit heaps and low-memory exhaustion remain unqualified.
