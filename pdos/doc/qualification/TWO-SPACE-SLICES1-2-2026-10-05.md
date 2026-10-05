# Two-space production slices 1/2: checked handover and guest DAT, 5 October 2026

We retained the existing one-ASCE z/PDOS release route and qualified a
successor bootstrap on a fresh, disposable 100-cylinder 3390 image. The
successor is still the bounded K64/C31/shared-U machine fixture, not a booted
replacement OS or a CMS/TSO application run.

## Inputs and route

The macOS arm64 host source-built PLOAD and the base image through
`pdos/scripts/image.crexx`. Mainframe Classic C 3.4.6 compiled the C31 stage,
real-frame ledger, fixture map and sparse DAT builder. Classic Assembler
0.1.0-bootstrap and Classic Linker (PDLD 0.19 lineage) produced the native
`PDOS.SYS` staging module. Classic Assembler also encoded the privileged
single-CPU `PTLB` callback. GNU Binutils 2.47.20260726 assembled and linked
the position-independent real launcher with `-march=z900`. The source core was
built by the current Steps 3/4 recipe and passed its 26 diskless machine
checks before packaging.

The checked disk contains an ordinary, unchanged `COMMAND.EXE` PCOMM dataset
and a separate `KCORE.BIN` successor package. The version-2 package has six
U/18452 records and 110,712 bytes. It carries 14 nonzero bare-core pages plus
the launcher; its K/U DAT pools and ASCE slots are zero. The header binds the
16 MiB real-memory profile, 2 MiB final image, entry, record count and CRC32
values. The C31 stage checks the DSCB first extent, complete record framing,
sequence, duplicate pages, padding, CRCs and real-frame layout before copying
or entering the core. CRC32 is an accidental-damage check, not a signature.

The stage reserved real 0–4 MiB and the 15–16 MiB stack during loading,
reserved final-core real 0–2 MiB during copy/execution, and selected a
staging source at real `0x400000` and a one-page launcher at real `0x600000`.
Its final-core ledger
reserved supplied pages and selected DAT pools at real `0x100000` (K) and
`0x140000` (U). The guest C31 builder produced 139,264 K table bytes (34
4 KiB frames) and 94,208 U table bytes (23 frames). K-only 31-bit virtual
aliases at `0x05000000` and `0x05040000` expose both completed table pools;
the high K64 nucleus read each alias after DAT was enabled. An unmap/remap cycle
called the assembled `PTLB` callback twice before DAT was enabled. The
launcher then copied the completed core to real zero and entered its DAT-off
AMODE64 bootstrap PSW.

## Checks and result

Hercules 4.9.1.0-SDL ran one ESAME model-2064 CPU with 16 MiB real storage and
the disposable 3390 at `01B9`. A fresh `ipl 01B9` passed all 35 recorded
checks, including the four 24/31/64-bit SVC crossings, nested U call, Classic
C31 results, forged high-pointer rejection, final U isolation fault, guest
ASCE values, K reads of both table aliases, and byte-for-byte guest DAT
comparison with the host reference.
The checked CKD/CCKD construction read back all PLOAD, PDOS, COMMAND, CONFIG
and KCORE source bytes. The disk SHA-256 was identical before and after IPL.
The fixture still maps only 8,192 U virtual bytes below 16 MiB, with no K or
DAT-table pages in U; this is placement headroom, not a CMS24 heap guarantee.

| Input or output | SHA-256 |
| --- | --- |
| Source K64/C31/shared-U core | `31a4f0a7f4e3c7f401d19d064713f86d7a2b1c510d2d55a9b789653fac22583f` |
| Bare core in package, before guest DAT | `682d365940a17876153cebf05629465858aad069652a66e0977d3da81b759dfb` |
| `KCORE.BIN` package | `8f922e4d33fb7e5d44d7414784cab311969cd0550c4cdeccb865d17bf78213de` |
| Native C31 stage | `e77b0b5ec51de6560f9e42704da0193779abdeaa2196096cd33961a13a388387` |
| Checked CCKD and IPL disk | `b5affa7ba9950b574b41aef9d09fc8aba34c227d1d1efafb93d43ef754871019` |

The positive build receipt is under ignored
`build/pdos/two-space-ipl-slices12-artifact-final/run/receipt.json`; the package and
source/tool hashes are in its parent output directory. The source-core
machine receipt is under ignored
`build/pdos/two-space-next-slices12-final/run/receipt.json`.

For a negative control we altered one byte of the first data payload in a
separate package without updating the core CRC recorded in its header. We
built and checked a separate 3390 image, then IPLed it. The C31 stage stopped
with diagnostic
`0x5453001c` (package CRC failure) before the launcher or core ran. The disk
remained unchanged and Hercules reported no program-interruption loop. That
candidate's receipt is under ignored
`build/pdos/two-space-ipl-corrupt-alias-final/run/receipt.json`.

## Limits

This proves the named 16 MiB, one-CPU handover and guest table construction.
The final core still has fixed real-zero linked placement. The table pools are
reserved as contiguous blocks inside that final image; the unused reserved
pool frames have not yet been returned to a running kernel allocator. The
map/unmap API and `PTLB` callback are host-checked and exercised by the guest
stage while DAT is off, so actual DAT-on mutation, persistent K ownership and
reclamation remain open. The fixture handles only its selected SVCs and
expected fault. It does not run the CMS/TSO loader, general OS services or
additional interruption classes. These are the subsequent PD-003 gates.
