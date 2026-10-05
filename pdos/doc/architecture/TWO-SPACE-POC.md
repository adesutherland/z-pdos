# Two-space kernel: first contract and machine proof

The next kernel direction is **one supervisor address space (K) and one shared
application address space (U)** on one CPU for a single user. A small AMODE64
assembler nucleus enters K on interruptions and dispatches services. Most OS
services can continue as Classic C with 31-bit pointers in K; their code and
data use K virtual addresses below 2 GiB. All applications, including AMODE24,
AMODE31 and AMODE64 programs, use the same U translation. Application-to-
application calls therefore retain ordinary in-space pointers when the target
ABI permits them. This is a design contract and a bounded machine proof, not
an implemented disk-boot kernel or a general CMS/TSO compatibility claim.

## Step 1: address and transition contract

| Area | K translation | U translation |
| --- | --- | --- |
| Prefix/real interruption area | A dedicated real low-storage island receives SVC and program interruptions with DAT off. K also maps it at the same low virtual address while enabling DAT. | No kernel island mapping is needed. Hardware saves old PSWs in the real prefix area. |
| 0–16 MiB virtual | K may place its own C31 text, state and stacks here if required. | App code, data, stack and 24-bit allocation. No K body, heap, DAT tables or interrupt entry page occupies this range in U. |
| 16 MiB–2 GiB virtual | Main home for K's C31 services and data, independently backed. | Preferred placement for relocatable CMS31 and TSO31 modules and their generous heaps. |
| Above 2 GiB | AMODE64 nucleus data as needed, and real backing for K structures at independently chosen addresses. | AMODE64 images, heaps and stacks; placement is independent of K. |

The ASCEs are independent 64-bit values in CR1. An application runs in problem
state with a nonzero PSW key. On SVC, the new PSW enters a real-addressed,
DAT-off assembler entry in supervisor state. That entry saves all 16 full-width
GPRs and the old PSW, loads K's full CR1 with `LCTLG`, enables DAT, and invokes
the K dispatcher. Returning disables DAT in the real island, loads U's CR1,
restores the full GPRs and loads the saved application PSW. No transition code
needs to be virtually common to K and U. The real island and prefix storage
still consume *real* memory. Program, external, I/O and machine-check entries,
nesting, interrupt masks and recovery remain design and integration work.

**24-bit budget.** The first machine fixture maps only two U pages below the
line: its code at `0x20000` and data at `0x21000`. Thus 8,192 of 16,777,216
low virtual bytes are mapped, 16,769,024 are unmapped, and **zero kernel pages
are mapped in U's 24-bit range**. That is a fixture measurement, not an
application heap promise. The production CMS31 adapter currently uses a
lowcore word at `0x14`; retaining that ABI may require one U-owned 4 KiB
compatibility page at virtual zero. With only that page committed, the design
starts from 16,773,120 low virtual bytes available for application placement
before any program, stack, heap or guard allocations. We must inventory every
CMS/TSO lowcore and parameter convention before deciding the final minimum.
The checked CMS24 RXVM is fixed at `0x20000`–`0x1ba6c0` (1,681,088 image
bytes, 1,683,456 page-rounded bytes). It must not collide with a kernel
virtual mapping or a second fixed module. Its usable heap must be measured
separately from 31/64-bit heaps; storage above the line does not repair a
24-bit allocation failure.

The service surface keeps distinct existing personalities. TSO programs use
the selected MVS-style SVC, LOAD/LINK/ATTACH, storage, file and terminal
conventions. CMS31 currently uses CMSCALL/SVC 204 and SVC 205 plus the `0x14`
SYSREF convention; CMS24 uses its selected SVC 202 convention. The dispatcher
must preserve each caller's AMODE, GPR high halves, return code and parameter
layout. A C31 service must never treat a U virtual pointer as a directly
dereferenceable K pointer. The AMODE64 nucleus must validate range and length,
copy through bounded buffers or a controlled U access window, then pass C31
addresses to Classic C. High U pointers require a 64-bit entry contract;
silently truncating them to a C32 pointer is forbidden. This contract does
not assert that every IBM service is already implemented.

All app images occupy one U map. The loader needs a live interval inventory
for code, data, stacks and heaps, keyed by module and lifetime, with checked
AMODE/RMODE placement and relocation. It must refuse overlap before writing
any page. A relocatable CMS MODULE can carry relocation records from
`RLDSAVE`/`GENMOD`, whereas a default MODULE may be fixed; the current CMS24
RXVM has no relocations. An arbitrary pair of fixed-origin modules cannot
co-reside merely because U has 64-bit addresses. A future nested-command test
must either find nonoverlapping placements or implement an explicit,
reversible overlay policy that preserves the suspended caller. For REXX
`ADDRESS`, the target command is dispatched in U, while OS-mediated loads,
files and terminal operations cross to K. The required nested-call gate must
cover a relocatable pair and a conflicting fixed-origin case, including
caller survival and return code. It is not part of this first machine proof.

The modern ELF SDK is useful for AMODE64 application experiments but is not a
dependency of C31 supervisor services. This proof uses GNU assembler/linker
only to produce a freestanding ELF64 core for Hercules; Classic Assembler now
encodes the full-width `LCTLG`/`STCTG` instructions needed by the eventual
nucleus. Classic C, Classic Linker, the native load format and PLOAD remain
separate integration gates.

## Step 2: diskless machine proof

[`two-space.crexx`](../../scripts/two-space.crexx) assembles the original
[`probe.S`](../../tests/two-space/probe.S), links its fixed real/virtual
segments and invokes the bounded ELF/core adapter. It starts one disposable
Hercules process with no disk, 16 MiB real storage, one model-2064
z/Architecture CPU and a z900 instruction ceiling. The adapter builds two
separate region-third ASCEs and checks exact ELF segment bounds before
`loadcore`/`runtest`. Generated core, DAT tables, logs and receipt stay under
ignored `build/`.

The application begins at `0x20000` in AMODE24, enters SVC, resumes in
AMODE31 and enters SVC again, then runs above 4 GiB in AMODE64 for two more
SVCs. Each crossing records the old PSW, an application value read at virtual
`0x21000`, a different K value read at the *same* virtual address and the
saved full GPR set. The final U read from K's virtual entry address `0x1000`
must raise a page-translation exception in problem state. The host judge
checks the exception code, translation address, mode and all four register
records. This demonstrates distinct translation and a protected absent K
mapping, not production recovery from arbitrary faults.

The exact local result, source identities and limitations are in the
[Step 2 qualification note](../qualification/TWO-SPACE-STEP2-2026-10-05.md).
The current release kernel still has one ASCE and key-zero application
execution as described in this architecture guide. No IPL or CMS/TSO binary
was run by this fixture.

## Steps 3 and 4: small K64 core, C31 services and shared-U calls

The successor proof puts the assembler nucleus at K virtual
`0x0100000000000000` and a separately backed Classic C31 service at K virtual
`0x02000000`. U uses `0x02000000` for a different application module. Each
space has a **region-first ASCE**, loaded as a full 64-bit CR1 value. Sparse
region-first, region-second, region-third, segment and page tables support
every 64-bit virtual bit; the high K nucleus exercises a nonzero region-first
index. The current C32 kernel's eager 8.8 MiB table structure and 4 GiB
aliasing are not part of this design. `twospace_dat.c` builds these tables from
an independently reserved K-accessible real pool, with 4 KiB pages and
big-endian 64-bit entries. Both table pools and backing frames must be
reserved before translation is enabled. Page-table storage and low channel
buffers consume real memory but **no U virtual addresses below 16 MiB**.
Live table mutation will need translation invalidation; this builder is for
bootstrap construction only.

The SVC entry remains a real, DAT-off interruption island. It saves full
registers and the old PSW, loads the K ASCE with `LCTLG`, then branches to
the high K nucleus. That nucleus checks the full U pointer, copies a bounded
request through a K alias into a K-owned C31 buffer, and invokes a Classic
C service through a low K trampoline. It rejects a forged pointer whose low
32 bits match a valid address. This fixture's aliases are fixed to two known
pages; a general service gate will need checked page walks, lengths, overflow,
write direction, access rights and fault recovery. The service returns in
the caller's original 24, 31 or 64-bit mode and key 8. K code/data and DAT
tables are not mapped in U.

One AMODE24 application calls a different AMODE31 application directly in U.
The callee invokes the same SVC and returns a code; the caller survives and
continues to an AMODE64 application. The `twospace_placement.c` ledger is a
bounded C31-compatible interval model for a future loader: it records page
reservations with split high/low 32-bit addresses, rejects fixed-origin
collisions, finds a free page-aligned location for a relocatable image and
releases a module's reservations. The fixture refuses a conflicting fixed
module before constructing U's DAT. It does not yet relocate or load a CMS
MODULE or TSO load module, nor does it implement the REXX `ADDRESS` dispatcher.
The synchronous call and return path is the first machine gate for that
dispatcher.

| PoC resource | Real backing | Virtual placement |
| --- | --- | --- |
| K DAT tables | 135,168 bytes, 33 real 4 KiB frames | No U mapping |
| U DAT tables | 94,208 bytes, 23 real 4 KiB frames | No U mapping |
| K64 nucleus | One real code page in the fixture | `0x0100000000000000` in K |
| Classic C31 service and trampoline | Two real pages | `0x02000000`–`0x02001fff` in K |
| U application pages below 16 MiB | Two real pages | `0x20000` and `0x21000` in U |
| Other U application pages | Three real pages | `0x02000000`, `0x110000000`, `0x110001000` in U |

The current fixture leaves 16,769,024 of 16,777,216 low U virtual bytes
unmapped. A production CMS `0x14` compatibility page, images, stacks, heaps
and guards reduce that amount. The measure says nothing about free *real*
storage; a loader must separately count real frames. We will preserve a
generous 31/64-bit heap policy and measure the constrained 24-bit budget
against actual CMS24 and TSO24 images.

This is a microkernel-shaped boundary: the K64 nucleus owns switching,
interrupts, physical-frame and DAT authority, and checked transfer of service
messages. C31 supervisor services run only in K; TSO and CMS personalities
can use distinct adapters there. U holds applications and any in-space call
protocol. No IBM TSO or CMS API is implied merely by this boundary.

The [Step 3/4 qualification note](../qualification/TWO-SPACE-STEPS3-4-2026-10-05.md)
records the actual Classic tool and Hercules checks.

## Step 5: boot integration gate

The current PLOAD reads a native `PDOS.SYS` load module into a fixed one-MiB
slot at real 2 MiB, allocates its heap at real 1 MiB, and enters a one-ASCE
C32 kernel. Its heap overlaps the checked fixed CMS24 RXVM image. Replacing
`PDOS.SYS` with a flat core is not an IPL path: the successor fixture is
currently installed with Hercules `loadcore` and has prebuilt real pages.

The successor boot route must load a compact image into independently
reserved real frames, protect K frames and tables, construct K/U region-first
ASCEs, and enter the K64 nucleus from a real DAT-off island. It must give the
C31 service body a K virtual home below 2 GiB, give U no kernel virtual
mapping, and keep channel-command buffers reachable by 24-bit real addresses.
It then needs program, external, I/O and machine-check entries, nesting and
recovery, a bounded general U-buffer copier, service personality adapters,
and collision-checked CMS/TSO module loading. Finally the normal 3390 image
builder and fresh IPL qualification must select that route explicitly and run
CMS24/CMS31 and TSO31/TSO64 binaries. Until then, Steps 3/4 are a machine
proof and target component build, not a booted replacement kernel.

## Primary architecture and compatibility references

- IBM, *z/Architecture Principles of Operation*, SA22-7832-14:
  [instruction, PSW and DAT definitions](https://www.ibm.com/docs/en/module_1678991624569/pdf/SA22-7832-14.pdf?cp=HW11W).
- IBM, [CMS `GENMOD`](https://www.ibm.com/docs/en/zvm/7.2.0?topic=commands-genmod) and
  [CMS command execution characteristics](https://www.ibm.com/docs/en/zvm/7.2.0?topic=concepts-cms-command-execution-characteristics).
- IBM, [z/OS load modules and relocation](https://www.ibm.com/docs/en/zos-basic-skills?topic=zos-load-modules-executable-programs) and
  [AMODE/RMODE system support](https://www.ibm.com/docs/en/zos/3.2.0?topic=mode-system-support-amode-rmode).
