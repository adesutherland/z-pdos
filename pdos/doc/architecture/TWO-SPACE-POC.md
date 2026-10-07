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
The October 2026 architecture review retains one U ASCE for the first
replacement. Selected CMS and TSO images already coexist in the diagnostic
map. The design permits further applications to coexist whenever their image,
stack and heap fit the appropriate virtual range and real backing. There is
no residency policy that evicts a live application to make room. A load that
cannot meet its placement and storage requirements fails explicitly, especially
for 24-bit requests; completed applications release their allocations.
Separate CMS and TSO U ASCEs remain a contingency if unchanged workloads
demonstrate an unavoidable lowcore conflict, material 24-bit headroom loss,
or failure isolation that the shared map cannot provide at acceptable
complexity. Such a change would need fresh DAT, context and service-gate
qualification; the present proof would not establish it.

P0 through P5 now have accepted results in the [completion plan](../BACKLOG.md#pd-003-completion-plan-6-october-2026).
The [P1 fresh IPL](../qualification/TWO-SPACE-P1-2026-10-06.md) closes active
invocation ownership, full architectural caller state and interruption-driven
completion/cancellation. The [P2 service gate](../qualification/TWO-SPACE-P2-2026-10-07.md) adds native
file/input and durable output/readback. The [P3 native call gate](../qualification/TWO-SPACE-P3-2026-10-07.md)
adds checked co-resident loading, CMS/TSO calls and mode crossings, HIGH
LOAD/DELETE, and caller file-cursor preservation across child return/fault.
The [P4 C console gate](../qualification/TWO-SPACE-P4-2026-10-07.md) qualifies all four
display models, U presentation, the attached and line-only 3215 paths,
exclusive input selection and the durable transcript. The [P5 normal-image gate](../qualification/TWO-SPACE-P5-2026-10-07.md)
adds source-built PLOAD-to-K boot, U PCOMM, CMS/TSO command recovery, attached
capture and independent K emergency/shutdown. Full workload/default selection
remains P6; the published one-ASCE image is unchanged.

## Architecture decisions for the first replacement

These are design choices for the successor, not claims about the released
one-ASCE kernel or completed implementation.

| Concern | Decision |
| --- | --- |
| Application space | Keep one shared 64-bit U ASCE for AMODE24/31/64 CMS and TSO applications. One foreground invocation chain executes at a time. Other live modules may remain mapped when suitable storage exists. K owns the placement ledger and real frames. No K body, heap or tables occupy low U virtual storage. |
| Personality state | Each invocation carries its CMS or TSO service profile, file handles and lowcore contract. K installs the applicable U compatibility page or state on entry and restores the previous state on return; a dormant module's profile does not define the active lowcore. A failed transition leaves the caller's map and profile intact. |
| Application calls | The required contract is a synchronous CMS-to-CMS or TSO-to-TSO application call, with bounded parameters, a return code and restored caller state. The selected native linkage for each personality must be qualified separately. An in-space pointer is usable only when both programs' ABI and address modes permit it. A REXX `ADDRESS` operation is not a requirement of the mainframe cREXX builds. CMS-to-TSO or TSO-to-CMS application calls are optional only if a simple copied-parameter and return-code gate suffices; they cannot rely on shared raw pointers or become a prerequisite for replacement. |
| Command processor | PCOMM, or its successor, is a U application placed above 16 MiB with its stack and heap there when its ABI permits. It is the neutral PDOS command entry point and may ask K to launch either personality with a copied command and receive its return code. K retains boot, fault and emergency console output plus checked terminal and invocation services. The command processor is not part of K merely to remain resident. |
| Terminal and 3270 ownership | K owns the devices, interrupt/completion state, bounded data transfer, one foreground screen owner, one input owner and compatibility line input/output services. The K driver and 3270 data-stream encoder are C31 C, with assembler limited to privileged channel operations. A reusable U presentation library is C, with only target-specific linkage at its boundary. It provides an application header, footer, scrollable output and editable entry area through checked K screen/input requests. PCOMM should use it first. Wrapped applications redraw after a nested child returns. Existing unmodified line-oriented CMS/TSO programs continue through their native terminal calls; the wrapper does not silently impose a full-screen layout on them. A separately attached line monitor can receive the same logical line output while the 3270 remains the primary display. |

The maintained one-ASCE PDOS already accepts PCOMM commands, resolves CMS
MODULEs and TSO load members, enters them and returns their results through
`pdosCmsCommand`, `pdosLoadExe` and the existing ATTACH/completion path in
`pdos/src/pdos.c`. P3 carries that behavior into the K/U translation and
ownership model. The private successor SVC 235/236 and 245/246 fixtures
exercise pieces of that migration; they are not a new command ABI or a
reason to design another launcher.

The terminal contract must distinguish line output from a full-screen lease.
K serializes channel operations and rejects an invalid owner or buffer before
I/O. The U C library owns field layout, scrolling, command history and display
text conversion. K accepts bounded logical screen and field requests and
encodes their device data stream in C; it does not execute an unchecked raw U
stream. A screen lease follows the foreground invocation stack and returns to
its caller, which repaints its frame. This allows a legacy child to
use its normal terminal calls without corrupting a suspended U renderer's
state. A bounded output queue or an explicit busy result is required for any
output that cannot be displayed while another owner holds the screen; the
policy must be selected and tested before asynchronous output is enabled.

The release console's `22*80` output buffer and the successor's 2 KiB console
write limit are implementation limits, not the 3270 architecture. The first
expanded driver target is the standard display models 2 (24×80), 3 (32×80),
4 (43×80) and 5 (27×132), in their qualified default or alternate screen
modes. K exposes a versioned terminal-capability result: device class, usable
rows and columns, active/default/alternate geometry, buffer-address format,
and supported attributes. Optional color and highlighting are used only when
reported; the basic layout works without them. K obtains the result from a
checked device query where the channel terminal supports one; otherwise it
uses an explicit, validated terminal configuration. A 24×80 default is valid
for configured models that support it. It must not stand in for an unknown
active or alternate geometry. U computes output rows, input field and scroll region
from the reported geometry and fails a layout that cannot fit. K checks each
field, address, encoded length, input capacity and low-real channel buffer
capacity before submission. The present 256-byte input limit is a fixture
limit, not a general modified-field capacity. Wider or extended displays,
such as 62×160, require separately qualified addressing and device-query
support. Non-3270 line consoles use a separate backend behind the same
terminal service boundary, not fictitious 3270 rows and columns. The disk and
terminal retain independent K-owned channel workspaces and event-driven
completion.

### Operator line view and attached transcript

The intended normal operator session keeps a scrollable 3270 PCOMM screen and
may attach a second, append-only text view. PCOMM and the selected CMS/TSO
line services produce one ordered stream of logical text and command events;
the 3270 presentation renders it, and K can also deliver it to a separately
configured Hercules 3215 line device reached by a Telnet client. A 3215-C
instead writes to the integrated Hercules console and mixes host/operator
messages with guest output, so it is not the first clean transcript target.
The line view is a monitor, not another application address space or a second
command processor. A line-only primary console is a further configuration of the same service
boundary. The [P4 named-device qualification](../qualification/TWO-SPACE-P4-2026-10-07.md) now proves simultaneous attachment/capture, line-only operation and explicit prompt input selection. Normal-image acceptance remains P5/P6.

Unchanged applications that use the selected CMS or TSO line terminal calls
keep the same ABI and return conventions whether the primary display is a
3270 or a line console. The K terminal service routes those calls and fans
out text records to the attached monitor. It does not expose monitor
availability as an application-visible output failure unless an application
explicitly requests reliable transcript delivery. A versioned capability
query distinguishes the primary device and its active full-screen facilities
from the presence and health of a secondary monitor. A line-only primary
reports full-screen unavailable. A 3270 primary retains full-screen support
when a text monitor attaches. Full-screen requests on a line-only console
return a documented unsupported result rather than being flattened into text.

There is exactly one input owner. The 3270 normally owns command input; the
line connection may take a checked input lease only at a PCOMM command prompt
or a line-input request, with explicit handoff and return. It cannot supply
3270 AID keys or fields for a full-screen application. An attach, disconnect
or late input cannot duplicate, reorder or silently replace a command. K
serializes terminal events and completion; PCOMM publishes sequenced command
IDs, readiness, prompts, separate OS status and application return codes after
primary output is drained. The optional monitor cannot hold that completion
hostage: it either receives the records or its capture is marked incomplete.
A host client waits for those events, not a clock delay or screen coordinates.
Output resembling control records is escaped under a versioned text grammar.
Guest text records retain their defined bytes until a documented host
EBCDIC-to-UTF-8 conversion; non-text data is escaped. Host capture retains
the full transcript beyond the finite 3270 scroll region. A lost monitor,
full queue or incomplete capture is reported as a transcript gap or a
sequence discontinuity on reconnect; qualification fails rather than
accepting silent loss.
K-owned queues and buffers stay outside low U storage.

Raw 3270 screen writes do not inherently carry a meaningful line transcript.
The U presentation library can emit semantic text events for a wrapped
application; an unmodified full-screen program may only yield a screen-change
event in the monitor. This limitation is visible to the operator and to tests.
It does not change that program's 3270 service behavior. IBM's
[CMS CONSOLE interface](https://www.ibm.com/docs/SSB27U_7.2.0/com.ibm.zvm.v720.dmsa6/console.htm)
separates full-screen 3270 operations from LINERD/LINEWRT for line mode and
3215-type devices. IBM also describes
[LINEWRT in full-screen and line-mode CMS](https://www.ibm.com/docs/en/zvm/7.2.0?topic=instructions-linewrt),
and its [TSO full-screen command processor guide](https://www.ibm.com/docs/en/zos/2.5.0?topic=user-writing-full-screen-command-processor)
distinguishes full-screen TPUT from ordinary terminal output. These are
design precedents, not a claim that the selected z/PDOS service subset already
implements every IBM terminal form.
Hercules documents a [Telnet client for a 3215 console](https://hercules-390.github.io/html/hercoper.html)
and an [integrated 3215-C console](https://hercules-390.github.io/html/hercrnot.html);
its [sample configuration](https://github.com/SDL-Hercules-390/hyperion/blob/master/hercules.cnf)
shows a 3270 and 3215-C defined at different addresses. Simultaneous
3270-plus-Telnet-3215 guest service is still a z/PDOS qualification gate.

IBM's [screen-size definitions](https://www.ibm.com/docs/en/gddm?topic=network-pservic-operand-modeent-macro)
give the standard model dimensions. Its [3270 screen-size control](https://www.ibm.com/docs/en/personal-communications/15.0.0?topic=operations-3270-session-screen-size-control)
and [buffer-address description](https://www.ibm.com/docs/en/cics-ts/5.6?topic=stream-set-buffer-address-order)
are reference inputs for the capability and address-encoding checks; the
Hercules channel device still needs its own qualification.

Before selecting the replacement image, qualify the native call parameter and
return conventions separately for CMS and TSO, including mixed address modes,
fixed-origin collision failure and caller restoration after a child fault.
Also qualify personality lowcore save/restore, native terminal ownership
transfer, malformed terminal requests, and the 24-bit placement budget with
PCOMM present. The U presentation library has its own
[delivery item](../BACKLOG.md#pd-021-reusable-3270-application-presentation);
its header, footer, scrolling, entry and child-return repaint must be checked
before claiming that wrapper is available. The diagnostic IPL does not yet
implement it. The separate [operator line-view item](../BACKLOG.md#pd-022-attached-operator-line-view-and-transcript)
owns the secondary device, capture, input handoff and line-only qualification.

Four interface details must be fixed before implementing the remaining calls
and console work. First, name resolution and parameter layouts must follow
the selected unchanged CMS and TSO binaries; the K invocation gate must
distinguish a load/service failure from the child's normal return code.
Second, every invocation needs an owner for image pages, runtime allocations,
file handles and terminal lease, with the same cleanup on return and
recoverable fault. Third, personality must come from that invocation record,
not an inferred program-counter range or a global mutable mode. Fourth,
terminal completion and application wakeup must follow guest events and
explicit states; elapsed time may detect a stall but must not decide whether
an I/O completed. The exact parameter layouts and terminal request structures
remain implementation gates in [PD-003](../BACKLOG.md#pd-003-two-space-supervisor-and-shared-application-memory)
and [PD-021](../BACKLOG.md#pd-021-reusable-3270-application-presentation).
The first [K invocation-ledger component result](../qualification/TWO-SPACE-INVOCATION-2026-10-06.md)
proved token and cleanup ordering. The subsequent [TSO](../qualification/TWO-SPACE-INVOCATION-GATE-2026-10-06.md)
and [CMS](../qualification/TWO-SPACE-CMS-INVOCATION-2026-10-06.md) guest
gates connect the descriptor to selected native service calls, and the
[allocation unwind](../qualification/TWO-SPACE-INVOCATION-REAP-2026-10-06.md)
releases a live page on end. The diagnostic U fixture still requests entry;
K-controlled launch, complete resource ownership, fault unwind and event
delivery remain to be implemented.
The [image-selection gate](../qualification/TWO-SPACE-IMAGE-SELECTION-2026-10-06.md)
then replaced caller-declared personality and placement with a checked K
record; normal command launch and dynamic image handles remain open.
The [image-lease gate](../qualification/TWO-SPACE-IMAGE-LEASE-2026-10-06.md)
counts each invocation's reference to a K-cached mapped image. Returning a
child releases its lease while the parent's image stays mapped. Cache
replacement, eviction and native application re-entry still need policy and
guest proof.
The subsequent [heap](../qualification/TWO-SPACE-INVOCATION-HEAP-2026-10-06.md),
[input cursor](../qualification/TWO-SPACE-INVOCATION-FILES-2026-10-06.md) and
[output staging](../qualification/TWO-SPACE-INVOCATION-OUTPUT-2026-10-06.md)
checks bind selected CMS resources to the active invocation and preflight
releases against its K handle. FINIS-closed output is held for the diagnostic
audit. The unchanged guest workloads pass fresh IPL, but native nested-file
and heap guest controls, personality lowcore switching, terminal leases,
pending I/O cancellation and fault unwind remain open.
The [clear helper](../qualification/TWO-SPACE-CHANNEL-CLEAR-2026-10-06.md)
has a guest-verified CSCH completion on an idle 3270 and a failing absent
subchannel. The [terminal owner gate](../qualification/TWO-SPACE-TERMINAL-OWNER-2026-10-06.md)
then binds a waiting read to its invocation, rejects child cancellation and
clears the subchannel before transferring its lease. The
[post-start retry gate](../qualification/TWO-SPACE-TERMINAL-RETRY-2026-10-06.md)
clears after a real AID started READ MODIFIED and reads a second AID after a
distinct screen event. A physically in-flight cancellation and injected late
status after owner exit still need guest proof.
The [failed-start gate](../qualification/TWO-SPACE-TERMINAL-START-FAILURE-2026-10-06.md)
uses a real AID followed by one deliberately absent SSCH target, then checks
cleanup and a fresh read. It does not exercise a frame-owned pending I/O
handle or arbitrary late status.
The [owned I/O gate](../qualification/TWO-SPACE-OWNED-IO-2026-10-06.md)
does exercise a parent-frame pending READ MODIFIED across a nested child,
followed by parent-end clear and a fresh read without stale attention in the
observed ordering. The [retired-completion
gate](../qualification/TWO-SPACE-LATE-COMPLETION-2026-10-06.md) then presents
the old I/O token to the K ledger while a distinct invocation owns a new
phase-one read; it rejects the old completion and leaves the new owner intact.
Physical late channel status still needs separate proof.
The [controlled-call and child-fault
gate](../qualification/TWO-SPACE-CONTROLLED-CALL-2026-10-06.md) proves a K
entry and return through a registered U fixture and reaps a faulting child's
page. Its fixture addresses and trampoline do not establish native CMS or TSO
application linkage; fault cleanup with pending I/O remains open.
The [nested lowcore gate](../qualification/TWO-SPACE-INVOCATION-LOWCORE-2026-10-06.md)
stores a suspended parent's U compatibility page in a K-owned real frame,
installs the child template, then restores the parent page on child end. It
does not use another low U page. Separate top-level personality reset and
abnormal-exit restoration remain open.

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

The [executable ABI inventory](TWO-SPACE-ABI.md) records the pinned
unchanged CMS and TSO inputs, observed placement, selected service ownership,
positive and negative controls, and remaining P0 gates. It is updated from
actual binary and guest evidence before new K service or invocation forms
are added.

Applications share one U map. They can coexist whenever appropriate virtual
intervals and real backing are available; simultaneous residency of every
possible application is a storage-capacity question, not a prohibition. The
ordinary loader keeps live applications mapped when a valid interval is
available. If there is no suitable virtual interval or real backing for the module's
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
reversible overlay policy that preserves the suspended caller. For a required
same-personality application call, the target executes in U while OS-mediated
loading, files and terminal operations cross to K. The nested-call gate must
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
MODULE or TSO load module, nor does it implement the native CMS or TSO
application-call contracts. The synchronous call and return path is the
first machine gate for those contracts.

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
native same-personality call integration or an actual colliding guest call yet.

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
heap. At that checkpoint K's Classic C31 endpoint recognized the selected
CMSCALL storage and line-write requests by the caller's saved PC within the
checked CMS31 image. The later invocation gate uses an explicit CMS owner
and checks that PC against the selected image.
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
disk persistence, same-personality application calls and arbitrary
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
compatibility or native same-personality application calls.

The [CMS cursor checkpoint](../qualification/TWO-SPACE-CMS-CURSORS-2026-10-06.md)
replaces the single input cursor per personality with eight K-owned slots
keyed by file ID and personality. Two real CMS31 input stages coexist during
fresh IPL and retain separate cursors and real-frame owners. Their storage
is outside U's low virtual range. `FINIS` releases only its named input;
the fixture completion gate releases any remainder. This supports a
necessary file-state condition for nested calls, but a native
application-to-application call has not run.

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
file/input surface, native same-personality application calls, TSO64 or
native TSO24 support.

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
The [fresh-IPL TSO24 map](../qualification/TWO-SPACE-TSO24-MAP-2026-10-06.md)
subsequently installs that member at `0x400000` beside the retained CMS24
image. Its 265 U pages have independent high-real backing, and the checked
low placement gap retains 12,828,672 bytes before TSO24 runtime storage.
A full 16 MiB low-only request returns RC 4 with no address while high-U
work continues. At this mapping checkpoint the native TSO24 entry and its
stack/heap budget were still unqualified.

The later [released beta 3 native TSO24 check](../qualification/TWO-SPACE-TSO24-BETA3-NATIVE-2026-10-06.md)
replaces that earlier local input with the XMI from the public beta 3
mainframe ZIP. The unchanged program returns RC 0, sends a legible EBCDIC
version line through the selected TPUT service, and releases its 1 MiB
stack, 256-byte output buffer, 4 MiB heap and low parameter page. The
diagnostic IPL also rejects a full 16 MiB low-only storage request with RC 4
and no above-line fallback. This proves one bounded native execution case;
general load, command and file behavior remain open.
The later direct collision control places a temporary 3 MiB low allocation
over TSO24's fixed origin. The load returns RC 4 without publishing the
image, then succeeds after that allocation is released. The program's
version call and low runtime storage still pass. The ordinary loader may
keep programs resident when suitable space exists; it has no obligation to
keep all applications loaded at once.

The [K-controlled call proof](../qualification/TWO-SPACE-CONTROLLED-CALL-2026-10-06.md)
adds a private diagnostic transition from K into a registered U child. It
restores the interrupted caller after normal return and one recoverable
child fault, with separate OS status and application RC. Its fixed fixture
entries do not establish native CMS or TSO application-call linkage.

The [same-origin child check](../qualification/TWO-SPACE-NESTED-BACKING-2026-10-06.md)
replaces the mapped RXVM interval with a minimal AMODE31 child, executes its
SVC through K, returns `0x3456` to a U64 caller, restores the parent PTEs,
and verifies its bytes again. Each live page-table replacement is purged on
the one CPU. This proves a mixed-mode memory and gate transition. The later
CMS31 `-v` check runs after the parent image is restored; broader CMS file
and command services and native same-personality application calls remain to be
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
