# Two-space kernel: first contract and machine proof

The next kernel direction is **one supervisor address space (K) and one shared
application address space (U)** on one CPU for a single user. A small AMODE64
assembler nucleus enters K on interruptions and dispatches services. Most OS
services can continue as Classic C with 31-bit pointers in K; their code and
data use K virtual addresses below 2 GiB. All applications, including AMODE24,
AMODE31 and AMODE64 programs, use the same U translation. Application-to-
application calls therefore retain ordinary in-space pointers when the target
ABI permits them. This is a design contract and a bounded machine proof, not
a replacement disk-boot OS or a general CMS/TSO compatibility claim.
After slice 7 qualification, we will review whether CMS and TSO should
instead have separate U ASCEs, using measured collisions, low-memory
headroom and cross-personality call behavior. The shared-U proof does not
settle that design choice.

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
The later [guest lowcore result](../qualification/TWO-SPACE-CMS31-LOWCORE-2026-10-06.md)
installs that U-owned page with independent real backing and key 8. It
contains the CMS31 SYSREF pointer and SVC 205 veneer; the guest reads them
through U DAT. The veneer and any CMS/TSO lowcore switching still need
service and cross-personality qualification.
The later [CMS31 FST result](../qualification/TWO-SPACE-CMS-FST-2026-10-06.md)
calls the veneer in U, scans a bounded K VTOC catalogue and returns an
FST through U lowcore storage. Absent and present cases pass fresh IPL;
the [later CMS31 IOQUAL run](../qualification/TWO-SPACE-CMS31-IOQUAL-2026-10-06.md)
adds selected record operations. Personality switching remains separate work.
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

Applications share one U map while active, but the design does not require
every application to remain resident simultaneously. The ordinary loader
keeps existing applications mapped when a valid interval is available. If
there is no suitable virtual interval or real backing for the module's
AMODE/RMODE and requested storage, it returns a placement/storage error;
it never silently places a 24-bit module or its required storage above
16 MiB. Completed applications release their intervals. An explicitly
nested call may use a reversible suspend/restore policy for a conflicting
fixed origin, with the caller's state and return code checked on restoration.
The loader needs a live interval inventory
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
The guest builds the tables during bootstrap. A checked single-CPU map/unmap
API and `PTLB` callback exist, but DAT-on runtime mutation remains an
integration gate.

The real, DAT-off island now has SVC, program, external, I/O and machine-check
new PSWs. It keeps two bounded context frames, each with all 16 full-width
GPRs, the exact old PSW, the full CR1 and an interruption class. The nucleus
loads K's ASCE, enters K64, and returns through the real island to the saved
ASCE, PSW and key. A nested SVC while K DAT is enabled returns to its outer
service. One selected U translation fault resumes at an explicit U recovery
address with a result code; unknown program faults and machine checks enter a
bounded wait rather than retrying the faulting instruction. External and I/O
entry paths were exercised with planted old PSWs; real asynchronous device
delivery and acknowledgement remain unqualified.

The C31 gate walks U's full-width region-first translation through a K-only
table alias, validates every page and its access direction, rejects range
wrap, and preflights a transfer of at most 256 bytes before copying through
K-owned storage. K has a `0x08000000`–`0x08ffffff` virtual aperture to the
named 16 MiB real profile; U cannot address that aperture. The Classic C31
service receives a K descriptor, never a raw U pointer. It dispatches SVC
202 and 204 as separate CMS fixture personalities and SVC 1 as the TSO
fixture personality. These return values are probe values, not IBM API
implementations. A forged high pointer, unwritable code page, oversized
request and wrapped address are refused. Valid read/write transfers cross
U pages. K code, data, tables and the real aperture are absent from U.

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
| K DAT tables | 200,704 bytes, 49 real 4 KiB frames | No U mapping |
| U DAT tables | 94,208 bytes, 23 real 4 KiB frames | No U mapping |
| K64 nucleus | One real code page in the fixture | `0x0100000000000000` in K |
| Classic C31 service and trampoline | Six backed service pages, one separate trampoline page | `0x02000000`–`0x02006fff` in K |
| K-only channel workspace | 64 KiB below 16 MiB real, outside both DAT pools | K real aperture alias; no U mapping |
| U application pages below 16 MiB | Two real pages | `0x20000` and `0x21000` in U |
| Other U application pages | Four real pages | `0x02000000`, `0x110000000`, `0x110001000`, `0x110002000` in U |

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
records the original machine gate. The subsequent
[production slices 3/4 record](../qualification/TWO-SPACE-SLICES3-4-2026-10-05.md)
records the interruption and transfer extension.

## Step 5: bounded 3390 IPL and remaining integration gate

The existing PLOAD reads a native `PDOS.SYS` load module into a fixed one-MiB
slot at real 2 MiB and allocates its heap at real 1 MiB. The first successor
IPL proof uses that loader unchanged, with a small Classic C31 staging module
as `PDOS.SYS`. The stage reads a compact sparse bundle from the image's
`COMMAND.EXE` dataset into real 4–6 MiB and puts a position-independent
launch stub at real 8 MiB. After all disk I/O completes, that stub copies the
2 MiB bootstrap core to real zero and enters its DAT-off AMODE64 PSW. The
copy overwrites PLOAD and its former heap only after neither runs again.
This is safe lifetime reuse of real frames in the fixture; it is not a
general physical-frame allocator or the final image layout. The U virtual
`0x20000` CMS24/application range remains independent of those real addresses.

The checked 100-cylinder 3390 IPL uses the normal source-built PLOAD, the
Classic C31 staging module, existing IPL vectors and a 19-record/350,588-byte
sparse package. It boots the same K64/C31/shared-U fixture that previously
ran through `loadcore`, and all machine checks pass after actual disk IPL.
The [Step 5 qualification note](../qualification/TWO-SPACE-STEP5-2026-10-05.md)
records the exact image and guest result. The dataset names and fixed real
addresses are fixture details, not a compatibility or ABI commitment.

At this Step 5 checkpoint the tables were prebuilt by product C source on the
host. The later guest-construction and interruption/service checkpoints are
described here. The boot crossing does not by itself establish a replacement
operating system.

## Production slices 1 and 2: checked handover and guest DAT

The successor now uses a separate `KCORE.BIN` dataset. `COMMAND.EXE` remains
the ordinary PCOMM application. The version-2 package contains a 40-byte
header, bounded sparse 4 KiB core pages, one position-independent launch stub,
a terminator, sequence checks and CRC32 values for the bare core and stub.
Every record is a fixed U/18452 block. The stage reads only within the
dataset's DSCB first extent and checks the entire package before calling the
launcher. CRC32 detects accidental package damage; this is not authenticated
boot. The disk package has zeroed DAT pools and ASCE slots, which prevents a
host-built table image from satisfying the guest-construction check.

The C31 stage uses a supplied-storage real-frame ledger over the named 16 MiB
profile. It reserves real 0–4 MiB for the live PLOAD/stage interval during
loading, the 15–16 MiB PLOAD/stage stack, and 0–2 MiB for the final core
during copy and execution. It allocates the 2 MiB staging source and one-page
launcher outside both live intervals;
the current first-fit result is real 4–6 MiB and real 6 MiB respectively.
The launcher is position-independent and receives its checked source, length
and entry through a descriptor patched by the guest. PLOAD and the C31 stage
are dead before the launcher overwrites their real frames. The linked fixture
still chooses the final real destination at zero; the transient staging
locations are allocated, not built into the package or launcher.

A second ledger reserves every supplied final-core page, including mapped
zero-filled interruption, context and service frames, then allocates
separate contiguous K/U DAT table pools in unused final-core frames. The C31
guest stage runs the product sparse region-first builder there and writes both
ASCEs into the core control page. In the checked 16 MiB profile the chosen
pool origins are real `0x100000` and `0x140000`. The current mappings use 49 K
and 23 U 4 KiB table frames. K-only C31 virtual aliases at `0x05000000` and
`0x05040000` make both completed pools reachable after DAT is enabled. The K64
entry still runs at virtual `0x0100000000000000`, while U has no K or table
mappings. `TSDATTACH` can reopen a completed pool through its K alias using the
recorded real origin, ASCE and used size. A source-level map/unmap API
requires a purge callback once a table is marked live. The guest stage calls
a Classic-assembled single-CPU `PTLB` callback for an unmap/remap cycle before
it enables DAT, then boots the final table set. This verifies the guest
builder, callback execution and final tables; a DAT-on runtime mutation by K
is a separate integration check before services depend on it.

The exact fresh IPL, host comparison and deliberately damaged-package result
are in the [slices 1/2 qualification record](../qualification/TWO-SPACE-SLICES1-2-2026-10-05.md).
Slices 3/4 add the bounded interruption and service gate described above.
The active release kernel remains the one-ASCE C32 system.

## PD-003 storage and fixed-overlay checkpoint

The [storage and overlay record](../qualification/TWO-SPACE-STORAGE-OVERLAY-2026-10-05.md)
extends the same 16 MiB successor with a K-owned page allocator. The K64
nucleus passes a saved-context request to a C31 endpoint through K memory.
Conditional SVC 120 GETMAIN/FREEMAIN is the first real service subset: K
assigns a U virtual interval, backs it with separate real frames, gives them
storage key 8, changes U's live DAT, purges the single CPU's translations and
returns the full address in
R1. A C31 pointer never represents a U virtual address. The fixture's
AMODE24 call obtains U `0x22000` using real `0x200000`; an AMODE64 caller
obtains U `0x02010000`. A 16 MiB request fails truthfully in the 16 MiB
physical profile, without taking 24-bit virtual space. The U callers wrote
their new pages in problem state. Both successful frees restore the original
U PTEs and storage key 0. K, DAT tables and heaps remain absent from U's
low range; physical K and channel reservations still reduce real capacity.

The allocator's search begins at `0x20000` for 24-bit requests and skips the
live interval inventory. With a registered CMS24 RXVM image ending at
`0x1bb000`, the first subsequent page can start there. This retains the
`0x1bb000`–`0x200000` gap that an artificial 2 MiB U floor would lose. The
current guest maps CMS24 RXVM at that fixed origin. The ledger reserves
`0x0`–`0x1ffff` for interfaces and `0xf00000`–`0xffffff` for the
24-bit stack and guard, leaving the contiguous `0x1bb000`–`0xefffff` gap of
13,914,112 low U virtual bytes. The [guarded stack result](../qualification/TWO-SPACE-CMS24-STACK-2026-10-06.md)
now backs `0xf01000`–`0xffffff` with separate real frames and leaves the
first page unmapped. A tiny AMODE24 child writes the last stack page. The
gap is a placement budget; unchanged CMS24/TSO24 heap demand is unmeasured.

Fixed-origin collision handling now has a K-owned page-backing primitive.
It keeps a suspended caller's real frames intact, maps distinct child frames
at exactly the same U address, and restores the caller's mapping after a
nested child. Two levels, original byte preservation and return-code transfer
passed a host control. The direct, nonoverlapping U call in the machine
fixture remains separate. There is no CMS/TSO format loader, relocation pass,
REXX `ADDRESS` integration or actual colliding guest call yet.

The guest bootstrap records CRC32 values for both completed DAT pools before
K runs. The IPL oracle compares these to the host reference, then checks the
post-run U table state separately. This avoids mistaking a legitimate live
map/unmap for a bad bootstrap. That fresh fixture IPL passed 79 checks. The
successor is still a selectable test dataset on its own disposable disk, not
the normal `PDOS.SYS` replacement selected for CMS/TSO applications. The
subsequent channel, dataset and terminal checkpoints extend the fixture;
general file I/O and command handling remain work in slices 5–7.

## Post-handover K channel checkpoint

The [6 October channel result](../qualification/TWO-SPACE-CHANNEL-2026-10-06.md)
adds a K-only 64 KiB disk workspace at real `0x180000`–`0x18ffff`. Guest bootstrap
reserves it before assigning DAT pools. Its K alias is part of the existing
real aperture; U never maps it. The Classic C31 endpoint builds a format-1
3390 read chain with real CCW/data addresses, submits it through a bounded
Classic-assembled `SSCH`/`TSCH` helper, and checks the IRB and `VOL1` record.
The 3390 IPL ran this service after K64 had entered its separate ASCE. At
that checkpoint the C31 endpoint used six K-only pages with a trampoline at
K virtual `0x02006000`. No 24-bit application virtual address was used.

The subsequent [K dataset result](../qualification/TWO-SPACE-DATASET-2026-10-06.md)
uses that real channel path to locate `KCORE.BIN` in the checked VTOC and
read its first record after handover. It checks one first extent and the
F/18452 geometry; it has not exposed a general CMS/TSO dataset service.

The [K terminal result](../qualification/TWO-SPACE-TERMINAL-2026-10-06.md)
uses `STSCH`/`MSCH` to enable console 0009, then writes a complete 3270 screen
through a K-owned low-real channel buffer. An independently connected
`s3270` saw `K SERVICE READY` during a fresh IPL. K C31 currently reserves
16 sparse pages at virtual `0x02000000`–`0x0200ffff`, backed by real
`0xa000`–`0xefff` and `0x14000`–`0x1efff`; its trampoline is at K virtual
`0x02010000`, real `0xf000`. These addresses are absent from U. The larger
service did not add a low U page: the diagnostic U map still has only two
pages below 16 MiB.

The [K terminal input result](../qualification/TWO-SPACE-INPUT-2026-10-06.md)
waits for 3270 attention, submits `READ MODIFIED` through the K real buffer,
then copies a bounded AID record to U with `TSGCOPY`. A connected terminal
submitted `PING`; the U caller received its Enter AID and EBCDIC bytes.
The service retains completed data if the destination copy fails, so the
caller can retry with a valid U pointer. This is an input transfer proof,
not a command interpreter. Command dispatch, general CMS/TSO file APIs and
unchanged application loads remain open.

The [disk/console isolation result](../qualification/TWO-SPACE-DUAL-IO-2026-10-06.md)
reserves a second low-real 64 KiB workspace at `0x190000`–`0x19ffff` for
the console. The disk and console have separate real ORB, CCW, IRB and data
areas; an intervening K dataset read no longer overwrites a pending terminal
operation. Both buffers cost real memory, not 24-bit U virtual space.

K's synchronous CKD/3270 helper now judges success from `TSCH` completion
status. A TOD watchdog bounds a stalled operation, replacing its previous
fixed poll count. The [fresh I/O completion result](../qualification/TWO-SPACE-IO-COMPLETION-2026-10-06.md)
covers the one-CPU diagnostic profile.
The [subchannel failure result](../qualification/TWO-SPACE-TSCH-FAILURE-2026-10-06.md)
also checks an absent subchannel in the guest. The assembler branches on the
channel instruction's condition code before changing it: no pending status
can be retried, while a nonoperational subchannel fails immediately. The
terminal state machine abandons that read instead of polling indefinitely.

K's C31 loader reads the checked first extent into a temporary real
allocation sized from the validated MODULE header. It checks the v2 envelope
FNV, zero padding, every image record, and the CMS31 map and relocation
records before exposing an image in U. The scratch allocation is released
after each load. [Host format tests](../qualification/TWO-SPACE-CMS-FULL-STAGE-2026-10-06.md)
cover unchanged CMS24 RXVM and CMS31 RXVM, RXAS and RXC stages; the
[guest dual-map check](../qualification/TWO-SPACE-DUAL-CMS-MAP-2026-10-06.md)
loads pinned RXVM from both profiles in one fresh IPL. CMS24 stays fixed at
`0x20000`; CMS31 is relocated to `0x03000000`. K writes image bytes through
its real aperture, and the host gate checks their U page translations and
distinct real backing. The original 16 MiB fixture was a diagnostic
real-memory profile; it did not establish a production 31/64-bit heap policy.

The [64 MiB real-memory result](../qualification/TWO-SPACE-64M-2026-10-06.md)
widens this same single-CPU successor. The K-only real aperture covers
64 MiB through sparse 4 KiB translations. The later
[U DAT capacity checkpoint](../qualification/TWO-SPACE-U-DAT-CAPACITY-2026-10-06.md)
keeps the 512 KiB K table pool and widens the U pool to 384 KiB within the
2 MiB final core. Disk/console buffers remain below the 24-bit real
limit. U virtual placements and the 24-bit free interval do not move.
The retained host check maps simultaneous 16 MiB U31 and 32 MiB U64
regions in 290,816 U table bytes. This proves table capacity and a fresh
IPL. The later [wide-heap guest check](../qualification/TWO-SPACE-WIDE-HEAPS-2026-10-06.md)
allocates, touches and releases a 16 MiB U31 heap and a 32 MiB U64 heap
at once while the CMS24/CMS31 RXVM images are mapped. U DAT use reaches
319,488 of 393,216 bytes. The high-U request uses an internal diagnostic
SVC 223 with a full-width address; the selected conditional SVC 120 path
retains its existing below/above-line register convention. The CMS images
are not yet executed, and these sizes do not define a maximum application
heap policy.

The [256 MiB diagnostic profile](../qualification/TWO-SPACE-256M-HEAPS-2026-10-06.md)
widens the final core to 4 MiB and its K/U pools to 1.5/1.375 MiB. K's
C31 real aperture maps the named 256 MiB profile while its DAT tables
remain K-only. The checked guest builds those tables after IPL, maps both
RXVM images and the CMS24 stack, then allocates, touches and frees a
64 MiB U31 heap and a 128 MiB U64 heap simultaneously. K uses 1,191,936
DAT bytes and U reaches 909,312. The low U24 placement gap stays
13,914,112 bytes. This addresses the SDK's default heap *sizes* with
real-frame and DAT headroom on that profile; it does not run the unchanged
SDK programs, exercise the CMS31 C stack, or establish the service ABI.

The [unchanged CMS31 entry result](../qualification/TWO-SPACE-CMS31-NATIVE-2026-10-06.md)
runs RXVM `-v` with its image-resident 3 MiB C stack and 64 MiB U31
heap. K's Classic C31 endpoint recognizes the selected CMSCALL storage and
line-write requests by the caller's saved PC within the checked CMS31 image.
It copies U parameters through the K gate and records one exact version
line. General CMS file, input and command APIs remain open.
The later [unchanged CMS24 entry result](../qualification/TWO-SPACE-CMS24-NATIVE-2026-10-06.md)
runs the fixed-origin RXVM `-v` from a temporary low U24 bridge and
preserves the same permanent low placement gap after releasing its one
page. The separate SVC 202 path checks CMS24's flagged 24-bit line
address and skips its inline four-byte continuation on return. Both
unchanged RXVM images execute in the same U ASCE in that diagnostic IPL.
This is evidence of a narrow coexistence case; application commands and
TSO binaries have not yet been exercised together.
The [live-screen checkpoint](../qualification/TWO-SPACE-CMS-LIVE-SCREEN-2026-10-06.md)
connects those selected CMS line-write requests to K's real-addressed
3270 workspace. The final version line is observed after guest completion;
the general CMS terminal and file APIs remain open.
The [CMS31 IOQUAL result](../qualification/TWO-SPACE-CMS31-IOQUAL-2026-10-06.md)
reads staged IOQUAL and LIBRARY records through K's checked U gate, then
runs IOQUAL from an independently backed second RXVM relocation at U
`0x05000000`. The guest observes `PASS=8 FAIL=0 SKIP=3` and RC 0 with
the earlier CMS24 and CMS31 RXVM images still mapped. Selected output
records use transient K real buffers and are checked and released at
completion. The positive IPL proves this noncolliding second invocation;
disk persistence, cross-personality `ADDRESS` calls and arbitrary
fixed-origin coexistence remain to be qualified.
The [file-validator checkpoint](../qualification/TWO-SPACE-CMS-FILE-VALIDATION-2026-10-06.md)
separates the C89 envelope and record checks from the K endpoint, exercises
corruption cases on the host and repeats the unchanged IOQUAL guest run.
The [record-transfer checkpoint](../qualification/TWO-SPACE-CMS-TRANSFER-2026-10-06.md)
adds a non-mutating U range probe. The selected CMS31 `STATE` and `RDBUF`
calls preflight their return words and record destinations before side
effects, so a bad second pointer cannot leave a partial record copy.
The [CMS24 IO24 checkpoint](../qualification/TWO-SPACE-CMS24-IO24-2026-10-06.md)
adds profile-specific K file state and runs a fresh fixed-origin RXVM overlay
on unchanged IO24 bytecode. The 24-bit U placement gap remains 13,914,112
bytes after the overlay and temporary call page are released. The whole-gap
check also exposed and corrected K's absent-segment software walk: segment
entries use `0x20`, whereas page-table entries use `0x400`. This is a
selected CMS24/CMS31 coexistence case; it does not establish TSO service
compatibility or real cross-personality application calls.

The [CMS cursor checkpoint](../qualification/TWO-SPACE-CMS-CURSORS-2026-10-06.md)
replaces the single input cursor per personality with eight K-owned slots
keyed by file ID and personality. Two real CMS31 input stages coexist during
fresh IPL and retain separate cursors and real-frame owners. Their storage
is outside U's low virtual range. `FINIS` releases only its named input;
the fixture completion gate releases any remainder. This supports a
necessary file-state condition for nested calls, but a real `ADDRESS` call
has not run.

The [native TSO loader-core checkpoint](../qualification/TWO-SPACE-TSO-LOADER-2026-10-06.md)
adds a C89 AMODE31/RMODE ANY record and relocation path for K-private
staging. Its output matches the released loader on the unchanged TSO31 RXVM
at two selected U bases. Directory inspection accepts the selected TSO64
ANY header, but high image relocation, K dataset staging, shared-U mapping,
TSO SVC services and actual TSO execution remain integration work. This
host/target-build result does not count as TSO coexistence in the guest.

The [fresh-IPL TSO31 map](../qualification/TWO-SPACE-TSO31-MAP-2026-10-06.md)
now carries that unchanged native record stream through K's bounded 3390
reader and private materialization buffer, then installs 268 U pages at
`0x07000000`. Both CMS31 images and CMS24 stay mapped. The complete guest
image hash matches the host loader's image at that base; staged K buffers
are released and no page of U's low 24-bit placement gap is taken. This
is a genuine same-ASCE image coexistence result, while dispatch and MVS-style
service compatibility remain untested. The current C31 service nearly fills
its 64 KiB fixture slot, so new services require an explicit K layout and
DAT capacity decision.

The [native TSO31 entry result](../qualification/TWO-SPACE-TSO31-NATIVE-2026-10-06.md)
adds one unchanged RXVM `-v` invocation in AMODE31. K keeps the TSO
allocations under a distinct owner and accepts the native SVC 93 TPUT
register contract through its checked U copy gate. The exact version line
appears on a connected 3270 and RXVM returns RC 0, while the unchanged
CMS31 IOQUAL and CMS24 IO24 calls also pass in the same U ASCE. This is a
narrow execution coexistence result. It does not establish a general TSO
file/input surface, CMS-to-TSO `ADDRESS`, TSO64 or native TSO24 support.

The [TSO64 ANY host materializer](../qualification/TWO-SPACE-TSO64-ANY-LOADER-2026-10-06.md)
now checks and relocates the selected low-resident AMODE64 member at two U
bases, byte-for-byte against the released loader. It has not entered the
guest or exercised a 64-bit native service. RMODE HIGH's 64-bit relocation
format remains distinct.

The [TSO64 ANY fresh-IPL map](../qualification/TWO-SPACE-TSO64-ANY-MAP-2026-10-06.md)
now carries that checked member through a separate disk envelope and K
stage, then maps it at low U `0x09000000` beside CMS and TSO31. Its complete
unexecuted image and 188 page translations match the host result. A native
AMODE64 entry and service ABI are the next execution gate. The C31 fixture
slot is near capacity; any expansion belongs in K storage, never the low
24-bit U placement gap.

The later [K service-capacity checkpoint](../qualification/TWO-SPACE-K-SERVICE-CAPACITY-2026-10-06.md)
extends the C31 K virtual slot to 32 pages (`0x02000000`–`0x0201ffff`).
Its first 16 pages retain their real backing; the additional 16 use reserved
real `0x80000`–`0x8ffff`. The C31 trampoline moves to K `0x02020000`,
still backed by real `0xf000`. The guest handover reserves even zero-filled
service pages, and the K DAT builder maps them as K-only. U's 31-bit heap
may still start at its own virtual `0x02010000`; the equal numerical address
does not refer to K's service page. The service image remains 63,006 bytes
at this checkpoint, so the added capacity is a mapped and isolated budget,
not an implemented program-call service. No U virtual page below 16 MiB is
consumed.

The [native TSO64 ANY checkpoint](../qualification/TWO-SPACE-TSO64-NATIVE-2026-10-06.md)
now enters the unchanged RXVM `-v` image and returns RC 0. Its IARV64
version-zero stacking program call uses U-owned CVT/SFT lowcore pointers,
an U `SVC 233; PR` veneer, and K-owned ASTE/linkage-stack control storage.
The AMODE64 nucleus handles the PC transition, while the Classic C31
service validates and copies the 88-byte U parameter list and owns the
128 MiB high-U mapping. The native DETACH releases it before a separate
wide-heap workload; the checked U 24-bit gap is unchanged. This is a
selected TSO64 service and version-call result, not general IARV64 or TSO
compatibility. The K C31 service is 64,774 bytes in the 128 KiB slot.

The [TSO24 low-resident loader checkpoint](../qualification/TWO-SPACE-TSO24-LOADER-2026-10-06.md)
materializes the separate source-built AMODE24/RMODE24 RXVM at U `0x20000`
or `0x400000`, byte-for-byte against the existing loader. Its 1,085,440
page-rounded image, 1 MiB stack and 4 MiB heap are 24-bit demands. Host
placement controls reject a full low interval despite free 31-bit space,
and permit reuse after release. This does not yet map or run TSO24 in the
guest; its actual storage budget remains to be measured there.

The [same-origin child check](../qualification/TWO-SPACE-NESTED-BACKING-2026-10-06.md)
replaces the mapped RXVM interval with a minimal AMODE31 child, executes its
SVC through K, returns `0x3456` to a U64 caller, restores the parent PTEs,
and verifies its bytes again. Each live page-table replacement is purged on
the one CPU. This proves a mixed-mode memory and gate transition. The later
CMS31 `-v` check runs after the parent image is restored; broader CMS file
and command services and actual REXX `ADDRESS` behavior remain to be
implemented. The IPL harness waits for the
3270 ready screen and guest disabled-wait event; time limits only detect
stalls.

The [CMS24 child check](../qualification/TWO-SPACE-CMS24-CHILD-2026-10-06.md)
also overlays CMS24 RXVM at its fixed `0x20000` origin. A U64 caller branches
to a small child that enters AMODE24, issues an SVC, returns to AMODE64 and
propagates its value through K's pop service. K restores the original RXVM
mapping. This proves the 24-bit mode transition and reversible low-address
collision in the guest, without claiming RXVM execution.

## Primary architecture and compatibility references

- IBM, *z/Architecture Principles of Operation*, SA22-7832-14:
  [instruction, PSW and DAT definitions](https://www.ibm.com/docs/en/module_1678991624569/pdf/SA22-7832-14.pdf?cp=HW11W).
- IBM, [CMS `GENMOD`](https://www.ibm.com/docs/en/zvm/7.2.0?topic=commands-genmod) and
  [CMS command execution characteristics](https://www.ibm.com/docs/en/zvm/7.2.0?topic=concepts-cms-command-execution-characteristics).
- IBM, [z/OS load modules and relocation](https://www.ibm.com/docs/en/zos-basic-skills?topic=zos-load-modules-executable-programs) and
  [AMODE/RMODE system support](https://www.ibm.com/docs/en/zos/3.2.0?topic=mode-system-support-amode-rmode).
