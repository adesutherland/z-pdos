# Two-space Steps 3/4: K64, Classic C31 and shared U, 5 October 2026

We extended the diskless two-ASCE proof with a high AMODE64 assembler nucleus,
a real Classic C31 service, a nested application call and target-compatible
sparse DAT and placement builders. This is local component and disposable
Hercules qualification. The current z/PDOS IPL image still enters its C32,
one-ASCE kernel.

The host was macOS arm64. The target inputs were Mainframe Classic C 3.4.6,
Classic Assembler 0.1.0-bootstrap, Classic Linker (PDLD 0.19 lineage), GNU
Binutils 2.47.20260726 with `-march=z900`, and Hercules 4.9.1.0-SDL. The
Hercules profile used one model-2064 ESAME CPU, 16 MiB real memory, no disk
and no shared guest. The exact binary/tool/source hashes, arguments and
observations are in ignored
`build/pdos/two-space-next-final4/run/receipt.json`. The ELF64 image hash was
`28f6aa99c119aeb67e5304d92b8a0244c83a925c24adb698c0badbc514bf00bd`;
the Classic C31 service binary hash was
`fe62189a2f7832f8c71846cdf3ffffcdb1fb41247b946040b84d23844dd2f4ef`.
The published `origin/develop` tip remained
`faea78b9583d354def64f1fc63b772c0f2b5f581` during this local work.

The retained cREXX recipe compiles the service, DAT and placement sources
with Classic C, assembles/links each with Classic tools, runs C89 host controls
under ASAN/UBSAN, builds the fixture's real DAT tables **from the same product
C source**, and executes one disposable Hercules instance. The host controls
passed high K table indexing, separate K/U roots, duplicate mapping rejection,
atomic exhaustion, fixed-origin collisions, relocatable search, AMODE limits,
64-bit address carry and slot capacity. The Classic objects also linked.

Hercules passed all 24 checks. K's region-first ASCE was `0x10000f` and U's
was `0x14000f`. Sparse table storage used 135,168 real bytes for K and 94,208
for U, backed by 33 and 23 real 4 KiB frames respectively. K executed at
virtual `0x0100000000000000`, with a nonzero region-first index. C31 code ran
at K virtual `0x02000000`; an unrelated U application ran at the same virtual
address with different real backing. U had two mapped pages below 16 MiB and
no private K real frame or K entry page there. Thus 16,769,024 low U virtual
bytes remained unmapped in this fixture. Real free storage is a separate
budget.

The four SVC entries came from AMODE24 at `0x2000e`, AMODE31 at `0x0200000e`,
then AMODE64 at `0x11000000e` and `0x11000002c`, all in problem state/key 8.
The low and high valid U request values reached Classic C31 through K-owned
copies and returned `0x2468ace0`. The second application returned `0x1357` to
its still-running caller. A forged `0x100021000` pointer, whose low bits
matched a valid low U request, returned the rejection code `-3`. A final U
read of K's entry page produced the expected page-translation interruption
`0x0011` in AMODE64. Hercules reported no error.

Reproduce from the z-pdos repository root with a **fresh** `build/pdos/` path:

```sh
crexx -nokeep pdos/scripts/two-space-next.crexx --args build/pdos/two-space-next-new \
  /absolute/path/to/s390-linux-gnu-as \
  /absolute/path/to/s390-linux-gnu-ld \
  /absolute/path/to/mf-classic-cc \
  /absolute/path/to/mf-classic-as \
  /absolute/path/to/mf-classic-ld \
  /absolute/path/to/hercules
```

The proof's K alias list covers only two known request pages, its interruption
island is not a complete OS interrupt/recovery path, and its U modules are
preloaded. The placement ledger prevents collision in the fixture but has not
parsed relocation records or loaded CMS/TSO modules. It does not boot from
3390, run a real CMS/TSO application binary, perform channel I/O, handle SMP,
or prove full CMS/TSO API compatibility. [PD-003](../BACKLOG.md#pd-003-two-space-supervisor-and-shared-application-memory)
retains those acceptance gates. The [architecture contract](../architecture/TWO-SPACE-POC.md)
sets out the boot migration and 24-bit budget.
The subsequent [Step 5 proof](TWO-SPACE-STEP5-2026-10-05.md) boots this same
bounded fixture from a checked 3390 image; it does not qualify CMS/TSO binaries.
