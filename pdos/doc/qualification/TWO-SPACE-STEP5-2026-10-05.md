# Two-space Step 5: bounded fresh 3390 IPL, 5 October 2026

We booted the Steps 3/4 K64/C31/shared-U fixture from a freshly constructed
100-cylinder 3390 disk in a disposable Hercules process. This verifies the
successor's first disk boot crossing. It is **not** a replacement z/PDOS
kernel or a CMS/TSO application qualification.

The macOS arm64 host source-built the existing PLOAD and runtime through
`pdos/scripts/image.crexx`. Mainframe Classic C 3.4.6 compiled the new C31
staging module, and Classic Assembler 0.1.0-bootstrap and Classic Linker
(PDLD 0.19 lineage) assembled/linked it as the native `PDOS.SYS` module.
GNU Binutils 2.47.20260726 assembled/linked an 80-byte, position-independent
real launch stub with `-march=z900`. The source core was the exact
`build/pdos/two-space-next-final4/run/image.core` from the prior machine proof.
The binary packer encoded its 70 nonzero 4 KiB pages and the launch stub in
19 fixed U/18452 records (350,588 bytes). It reconstructed both inputs byte
for byte before media construction.

The image used the normal PLOAD and IPL vectors. The new C31 module was
stored as `PDOS.SYS`, while the compact proof bundle occupied the existing
`COMMAND.EXE` dataset. The module read the bundle into real 4–6 MiB and the
stub at real 8 MiB. After the final read, the stub copied the 2 MiB core to
real zero and entered its DAT-off AMODE64 bootstrap PSW. It safely overwrote
PLOAD and PLOAD's old heap only after both had stopped executing. The
bundle's ASCII signature is compared as an explicit big-endian integer in
target C, since the Classic compiler's execution text is EBCDIC.

The existing checked IPL writer confirmed every source byte and zero padding
byte in PLOAD, PDOS, COMMAND and CONFIG. CKD-to-CCKD-to-CKD roundtrip checks
found only the permitted host container serial difference. The generated
`build/pdos/two-space-ipl-final3/inputs.sha256`, `outputs.sha256`, base image
source manifests and `run/receipt.json` pin the exact sources, tools,
programs, disk and machine observations. Principal SHA-256 values are:

| Item | SHA-256 |
| --- | --- |
| C31 staging module | `beee3ce595afc406d8e9e1d5ae2c7c441a9d85129d7ed73864a247001e4c79fc` |
| Sparse bundle | `117fcc12ca42195d8d064d300a94c9742337280b5cd704a966f8e2965a2f387f` |
| Checked CCKD before/after IPL | `80892ba664d38862dee71f15542ff64f1cfa8a9342ad65e2f32aa5d875b58ceb` |

Hercules 4.9.1.0-SDL ran an ESAME model-2064 CPU with 16 MiB real storage,
the private 3390 at `01B9`, no terminal and no shared guest. The script
issued an actual `ipl 01B9`, waited for execution, then captured memory.
The exact Step 3/4 machine judge passed its 24 checks, and five boot-specific
checks passed: IPL command observed, no program-interrupt loop, Hercules exit
zero, expected source-core marker and unchanged disk. The four SVC crossings,
Classic C31 results, nested U call, forged high-pointer rejection, K64 high
execution and final U isolation fault matched the diskless proof. Hercules
reported no error and exited. The run did not depend on `loadcore`.

Reproduce from the z-pdos repository root with a fresh output directory and
the already qualified Step 3/4 core:

```sh
crexx -nokeep pdos/scripts/two-space-ipl.crexx --args \
  build/pdos/two-space-ipl-new \
  build/pdos/two-space-next-final4/run/image.core \
  /absolute/path/to/s390-linux-gnu-as \
  /absolute/path/to/s390-linux-gnu-ld \
  /absolute/path/to/hercules/bin
```

The booted core is still the deliberately preloaded fixture: its K/U tables
were generated from the product DAT builder on the host and carried on disk.
The target C31 DAT and placement modules compile, assemble and link but do
not yet execute in the IPL guest. The real interruption island handles only
the exercised SVC and expected program fault. There is no general U-buffer
copier, dynamic frame allocator, CMS/TSO loader/adapter, channel-I/O service,
other interruption recovery or application-binary run. The fixed real
staging addresses and use of `COMMAND.EXE` are proof layout choices. The
[PD-003 acceptance gate](../BACKLOG.md#pd-003-two-space-supervisor-and-shared-application-memory)
remains in progress.
