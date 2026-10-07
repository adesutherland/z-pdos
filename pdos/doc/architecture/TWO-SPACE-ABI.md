# Two-space successor: executable ABI inventory

This is the P0 audit for the [K/U successor](TWO-SPACE-POC.md). It is a
frozen version-1 executable contract for PD-003 P0, accepted 6 October 2026.
The normal replacement image remains P5/P6 work. P0 freezes the required ABI from unchanged binary/source evidence
and qualified one-ASCE behavior; the named successor positive and negative
controls are acceptance work for P2 and P3. An unknown form keeps its P0
entry open and blocks the corresponding P2/P3 implementation decision.
The package, disk stage, mapped image and executed guest are distinct
evidence layers. The local stages named below are ignored build outputs;
their hashes identify the unchanged bytes used by the diagnostic guest.
The CMS stage files are under `build/pdos/cms-module-recipe-c/media/`;
the TSO disk stages are under
`build/pdos/tso24-beta3-ipl-verified/media/`. Both are disposable local
paths, so a fresh reproduction verifies the hashes rather than trusting
the directory names.

The TSO beta 3 package is
`CREXX-v1.0.0-beta.3-mainframe.zip`, SHA-256
`fd11ae260bba169653126a861ecae545cee9bcfbe1fad4083c4a913241e3aedd`.
The checked local copy is in `build/pdos/tso24-release-beta3/`.
The CMS stages below are the pinned v2 inputs of the local source-built
recipe. Do not substitute a later binary without recording its identity
and repeating affected gates.

## Unchanged program inputs

| Profile and exact stage SHA-256 | Observed placement and entry | Positive and negative evidence; open contract |
| --- | --- | --- |
| CMS24 RXVM MODULE `04550b86d7bdfa3c9060d9fa154875adf3ccb677d95ba13290836ddc02a8da9e` | Fixed U `0x20000`, page-rounded end `0x1bb000`; SVC 202 with flagged 24-bit parameters; U stack at `0xf01000`–`0xffffff` with an unmapped guard page at `0xf00000`. | [P2](../qualification/TWO-SPACE-P2-2026-10-07.md) qualifies unchanged RXVM/IO24, exact input/output and durable records. [P3](../qualification/TWO-SPACE-P3-2026-10-07.md) qualifies native CMS24 PROGRAM calls and caller restoration. Normal-image CMS24 acceptance remains P6. |
| CMS31 RXVM MODULE `a08f3431407f8efec6b05f1e0580e3f12b76f9865f561dac15b5a90135223ff9`; RXC `a61f9ba543c2a471bae9551d6c9b55b121e5bbfae3a98f95797ea1712c1bfde8`; RXAS `c8b6eb5ce785eb3413c8f02528949f1f5643a016a519370ca2188e308926c910` | Relocatable RXVM at U `0x03000000` and `0x05000000`; lowcore `0x14` SYSREF, SVC 205 FST veneer, SVC 204 CMSCALL subset, 3 MiB image C stack and 64 MiB runtime heap. | [P2](../qualification/TWO-SPACE-P2-2026-10-07.md) qualifies unchanged RXVM/IOQUAL/library and selected native files/input. [P3](../qualification/TWO-SPACE-P3-2026-10-07.md) qualifies native PROGRAM, 31-to-24 calls and parent file preservation. Normal RXC/RXAS workloads remain P6. |
| TSO24 RXVM beta 3 disk stage `1335bd64e276ed5fc76a6198704fdde67f7e1f0748dea2d5f25ab81a53bf666a` | AMODE24/RMODE24 image at U `0x400000`, 268 pages; EBCDIC `-v` tail, 1 MiB stack, 256-byte terminal buffer and 4 MiB heap below 16 MiB; SVC 120 storage and SVC 93 TPUT. | [P2](../qualification/TWO-SPACE-P2-2026-10-07.md) retains unchanged native version, storage, real low-collision and exhaustion controls. [P3](../qualification/TWO-SPACE-P3-2026-10-07.md) qualifies LINK and wider callers reaching a 24-bit child. Native TSO24 workload/file qualification remains a separate P6 profile. |
| TSO31 RXVM beta 3 stage `9fffaba3c586c1f361c7ecb87167fb1b8acbb569c267b621187fe95305bce506` | AMODE31/RMODE ANY image at U `0x07000000`, 268 pages; EBCDIC tail, 1 MiB stack, 64 MiB heap, SVC 120 and SVC 93 TPUT with R0 length/R1 low-31-bit address. | [P2](../qualification/TWO-SPACE-P2-2026-10-07.md) qualifies unchanged native RXVM/IOQUAL/library, files and input. [P3](../qualification/TWO-SPACE-P3-2026-10-07.md) qualifies LINK and native PDPCLIB parent cursors across child return/fault. Full normal workloads remain P6. |
| TSO64 ANY RXVM beta 3 stage `bdc2d14439f1a3c90409313a9693672c61187b78277197d4dc2b5634dc19bdd6` | AMODE64/RMODE ANY image at U `0x09000000`, 188 pages; U CVT/SFT and SVC 233 plus PR veneer for IARV64 stacking PC, 128 MiB high heap, SVC 93 TPUT. | [P2](../qualification/TWO-SPACE-P2-2026-10-07.md) qualifies unchanged version and 128 MiB GETSTOR/DETACH. [P3](../qualification/TWO-SPACE-P3-2026-10-07.md) qualifies native LINK and return/mode restoration. Full terminal/file/compiler workloads remain P6. |
| TSO64 HIGH beta 3 compiler, assembler and VM pairs | [The six unchanged XMI and RDW hashes](../qualification/TWO-SPACE-TSO64-HIGH-BOUNDARY-2026-10-06.md) pin low AMODE64/RMODE ANY launchers `LAC65O`, `LAU65O`, `LAVM65O` (directory `0x11`) and high AMODE64/RMODE64 bodies `RXCH`, `RXASH`, `RXVMH` (directory `0x31`). All six entry offsets are zero. | [P3](../qualification/TWO-SPACE-P3-2026-10-07.md) enters all three unchanged launcher/body pairs, returns RC0 and exact version lines, and proves LOAD/DELETE plus 128 MiB heap ownership. The [host AL8 materializer](../qualification/TWO-SPACE-TSO64-HIGH-LOADER-2026-10-06.md) remains independent supporting evidence. Full normal HIGH compiler/assembler/terminal/file workloads remain P6. |

### CMS31 RXC and RXAS entry trace

The original beta 3 CMS31 MODULEs in the checked
`build/pdos/cms-module-contract-c.json` have the same linked origin and entry
`0x02200000` as RXVM. RXC has 127 records and 132,588 relocation entries;
RXAS has 69 records and 28,708. Their original MODULE SHA-256 values are
`9d2ff332113a5a6c9adafb8e9b338f894492958ee3e871f276cb2b6d242a5f04`
and `fdc5d5ef44fcd6ea75d345dffbaa4012ae850ed0f33d3f8b80d85b1f82a07a01`.
The staged disk bytes have the separate hashes in the input table above.
At offset 148 in each staged container, the first 64 image bytes of RXC,
RXAS and RXVM have the same SHA-256,
`207a7dbf62aa5f5ff0b060a3fe6a2b3739a22a41e7253fd5dec3e8193cd640f8`.
This directly checks that the three unchanged images start with the same
startup prologue; it does not establish their whole-file service behavior.

The qualified one-ASCE `pdosCmsCommand` in `pdos/src/pdos.c` validates the
MODULE and relocations, then builds a blank-padded, eight-byte-token PLIST
with an `FF` fence. Its bounded PCOMM path allows at most 14 tokens in the
child list, including the module name. It enters the child with R1=PLIST,
R13=save area, R14=return and R15=relocated entry in AMODE31. The maintained
`pdpclib/src/cmsstart.asm` startup source takes the PLIST and returns the C
application result in R15 through its saved return linkage. The
[one-ASCE fresh compiler chain](../qualification/STAGE3-2026-10-05.md) ran
unchanged RXC, RXAS and RXVM at RC 0, including file and library use; its
negative controls include malformed RXC source and malformed RXAS input.

This establishes the selected entry shape and baseline behavior. It does
not show that the successor can run RXC/RXAS. P2 must cover the compiler
and assembler's reached CMSCALL/file verbs and exact record outputs; P3 must
load each original module without moving a live image or its lowcore state.

### TSO64 HIGH SVC 8/9 source trace

The maintained one-ASCE handler in `pdos/src/pdos.c` (`pdos64HighName`,
`pdos64HighRead`, `pdos64HighService`) is the source for this selected PDOS
compatibility subset; it is not a claim about every z/OS LOAD/DELETE form.
The low launcher enters SVC 8/9 in AMODE31 after SAM31. The saved PSW must
confirm AMODE31. GPR0 supplies a low 31-bit pointer to an eight-byte uppercase
member name, padded with trailing blanks; the name maps to the private
`<member>.EXE` dataset. LOAD requires GPR1's low 31 bits to be zero. On
success K returns R15=0, a full-width AMODE64 tagged entry in GPR0 and the
loaded size rounded to doublewords in GPR1. On failure the initial R15 is 8
and no image is published. DELETE names the same member under the same
invocation owner, clears the image and returns R15=0; an owner/name mismatch
returns R15=4. The one-ASCE handler permits one live HIGH body globally; the
successor's placement ledger must support coexistence when virtual and real
storage fit, while preserving the caller-visible register/status contract.

P3's native launcher trace confirms that DELETE's name argument is a
31-bit register interface: its upper GPR0 word contains an arbitrary value
left by the returned C body (`9ffffffd` or `1` in the frozen controls).
DELETE uses the checked low word after confirming the saved AMODE31 PSW;
that upper word is not part of the pointer. The selected LOAD entry checks
its zero upper word and zero low-31-bit GPR1 before staging any body.
HIGH's allocation callback uses the same 88-byte IARV64 layout as ANY,
with positive segment counts through 128, rounded from its actual request.
K assigns these objects to the active invocation, including newly loaded
images; neither a fixed RXVM owner number nor its code address selects the
owner.

The release's frozen launcher objects are `LAC65O.obj`
`4914828d08a646e4a7a114a584ace52495757d35f101d5a0098fa2b25a164d7e`,
`LAU65O.obj`
`a65328f4de99bf23b552632d9c2fb3278fa7f5bc54e7b289b0b10be81e05065d`
and `LAVM65O.obj`
`6e7b6408865f645c15f19948f4fbd9d3ad88c0f3f7f2ded71bee3cb7c770c177`.
Their frozen source, Mainframe Lab
`crexx-release/inputs/native/entry64-high.asm`, has SHA-256
`49fcd9aaa2bb41adbeb482aef1de741f4afb211c5db85d0d85d09c3e4008a2d2`.
The release recipe selected these objects as the three low launchers.
The unchanged RDW members materialize under `TSTIMAGE64ANY` to 16,872-byte
images, all with entry offset zero and the same first 48 instruction bytes.
Each image contains exactly one EBCDIC `HIGHNAME` for its corresponding
`RXCH`, `RXASH` or `RXVMH`, and none of the other two names. This connects the
frozen source path to the selected unchanged launchers beyond a symbolic
member name.

The source saves the full caller GPRs, captures the entry PSW, switches to
AMODE31 and obtains below-line TPUT/TGET storage plus a 64 KiB work area.
It issues LOAD, checks the AMODE64 entry tag, dereferences the launcher's
incoming parameter list and passes the target a halfword argument length
and data pointer, plus a low service table. It switches to AMODE64 for BASR,
then to AMODE31 for DELETE and work-area release. The target C result in R2
is saved as the launcher's R15 result. LOAD or setup failures return 40 or
20, a bad entry state returns 36, and a failed DELETE replaces the result
with 44. The static launcher save areas forbid overlapping or nested
execution of the *same* launcher image; P3 must preserve that constraint
while allowing distinct live images to coexist when storage fits.

The P2/P3 controls are: accept each unchanged launcher/body pair with exact
entry, length and application RC; reject a high pointer or non-AMODE31 name,
nonzero LOAD GPR1, malformed member name, missing/invalid member, failed
AL8 relocation and cross-owner DELETE without publishing or deleting another
image. Check the transferred halfword length/data pointer/service table,
caller registers and R15 after the high body exits, followed by owner cleanup.
The source and unchanged-member checks select this contract; the successor
guest has not yet exercised it.

### Selected native application-call linkage

The maintained PDPCLIB `pdpclib/src/stdlib.c` has no CMS `system()` call:
its `__CMS__` branch currently returns zero without launching a child. Its
`__MVS__` branch passes request type 2 for TSO to `__system`. In
`pdpclib/src/native/mvssupa/common-03.asm`, `@@SYSTEM` maps type 1 to a
synchronous ATTACH/WAIT/DETACH and type 2 to a TSO command-processor request;
the PDOS selection rejects type 2. The one-ASCE `pdos/src/pdos.c` has a
selected SVC 42 ATTACH and SVC 3 child-completion path, but these source
facts do not establish that either maintained `system()` branch supplies the
required CMS-to-CMS or TSO-to-TSO contract on PDOS.

For the first replacement we select the following IBM-compatible application
call forms. This selects an interface for P3; it does not claim that the
successor already accepts it or that every IBM option is supported.

| Caller | Selected call | Bounded K/U contract |
| --- | --- | --- |
| CMS24 | SVC 202 from below 16 MiB | R1 points to a 24-bit tokenized EBCDIC PLIST whose first eight-byte token names a CMS MODULE. K checks the complete list before selecting a CMS child. The child receives the CMS24 call-type byte and returns its application RC in R15. |
| CMS31 | CMSCALL through SVC 204 | R1 points to a 31-bit tokenized EBCDIC PLIST. Select `CALLTYP=PROGRAM` with no extended PLIST; K checks the fence and permitted COPY/FENCE flags. A CMS31 caller may reach a CMS24 child with a checked, temporary below-line PLIST copy; if suitable low U storage is unavailable, the call fails without entering the child. The child returns its application RC in R15. |
| TSO24/31 | LINK through SVC 6 | The first subset names a load member by EPLOC and passes a checked parameter-address list through R1, with an optional ERRET. K selects the native image, enters it synchronously in its declared AMODE and returns to the caller's AMODE. |
| TSO64 | LINKX through SVC 6 | The same bounded member and parameter semantics apply. The service accepts an AMODE64 caller but requires control parameter addresses and passed addresses below 2 GiB for this first compatibility subset. A high RMODE64 body still uses its separate checked LOAD/DELETE route when requested by its unchanged launcher. |

IBM's [CMSCALL reference](https://www.ibm.com/docs/SSB27U_7.2.0/com.ibm.zvm.v720.dmsa6/cmscall.htm)
describes user MODULE invocation, 31-bit PLIST addressing and the COPY path
for a 24-bit target. Its `CALLTYP=PROGRAM` form has no extended PLIST; forms
with EPLIST remain outside this first call subset. The [CMS register convention](https://www.ibm.com/docs/en/zvm/7.2.0?topic=control-register-usage)
and [CMSRET](https://www.ibm.com/docs/en/zvm/7.2.0?topic=instructions-cmsret)
put the application result in R15. IBM's [LINK/LINKX reference](https://www.ibm.com/docs/en/zos/3.1.0?topic=module-link-linkx-description)
specifies synchronous entry, target AMODE selection, caller AMODE restoration,
optional parameters and ERRET; [SVC 6](https://www.ibm.com/docs/en/zos/3.1.0?topic=descriptions-svc-6-0a06)
is their supervisor route. These primary sources define the compatibility
shape; the selected private-disk member lookup remains z/PDOS-specific.

K records **OS status** separately from the child's **application RC**. A
normal native call exposes the child's R15 by the selected CMS or TSO
convention. For missing modules, malformed or inaccessible arguments and
capacity failure, K must not publish a partial image. IBM CMSCALL returns
R15 `-3` for a missing command and `-4` for LOADMOD failure; insufficient
save-area or PLIST-copy storage is the documented `0F0` abend, which this
successor must confine to the active invocation and report to PCOMM as an
OS failure. A call into an AMODE24 module with a high PLIST and COPY=NO is
the documented `1CC` failure. The selected implementation must retain the
distinct normal child R15 and failure category, even if both appear in the
native R15 convention.

For LINK/LINKX, IBM's ERRET receives control for an error that would have
abended the task; it does **not** receive input-parameter errors. In the
documented 64-bit register view, the high word of R1 carries the abend
reason and the low word carries the abend code; R15 addresses ERRET. K must
validate the saved ERRET address before branching. Without a valid ERRET,
K terminates only the active application invocation and returns OS failure
to PCOMM, preserving the supervisor and other live U images. P3 must pin
the exact selected missing-member abend/reason and bad-parameter result with
an unchanged LINK/LINKX caller fixture before accepting that handler.

P3 needs one fixed unchanged parent/child binary pair for CMS and another for
TSO, each passing a bounded parameter and observing a nonzero child RC. The
negative controls are missing child, malformed or inaccessible parameters,
occupied fixed origin, insufficient low storage, and child fault. A
success-shaped CMS `system()` return from the current placeholder is not a
positive control. The PDPCLIB `system()` convenience route can be repaired
separately after the macro-level contract passes, without making it the
kernel's only application-call API.

## Service and ownership table

### PCOMM command contract

The first command increment preserves `pcomm.c`'s `system()` calls and the
maintained PDPCLIB batch request type 1 in `@@SYSTEM`. That native adapter
builds an eight-byte EBCDIC member name and a flagged address list to a
halfword-length parameter string, issues SVC 42 ATTACH with EPLOC and ECB,
waits through SVC 1, reads TCBCMP and issues SVC 62 DETACH. The one-space
kernel dispatches `CMS RUN 31 RXVM -v` through `pdosMediaCommand` and
`pdosCmsCommand`; an ordinary TSO member goes through `pdosLoadExe`. Native
completion SVC 3 posts the result and restores the suspended parent. These
are the migration inputs, distinct from application-to-application CMSCALL
and LINK/LINKX forms.

The bounded diagnostic successor must check the complete U ATTACH control
block, EPLOC, ECB and parameter list before changing state. K owns the child
image selection, copied CMS PLIST or TSO argument tail, full caller context,
lowcore save/restore and child resource cleanup. The caller-visible ATTACH,
WAIT, TCBCMP and DETACH behavior remains intact. K retains a separate OS
failure category so a nonzero application RC cannot be mistaken for failed
launch. PCOMM itself is a neutral PDOS invocation, rather than a CMS or TSO
command processor. Its runtime console boundary and any genuinely required
below-line control storage are audited independently of its above-line
image, stack and heap. P2 delivers the selected file services, P3 the native application calls,
and P4 the display, monitor and presentation contract below.

P0 is closed by the source/binary inventory, explicit native byte layouts,
selected PDOS extension contract and frozen native call fixtures below.
Their successor execution is P2/P3 acceptance; every P0 control is assigned
there or to P4. Existing diagnostic mechanisms remain partial implementation
results. A new ABI form reopens its P0 contract entry before implementation
for that form is accepted.

| Native operation | K and U responsibility | Current proof and required control |
| --- | --- | --- |
| CMS24 SVC 202; CMS31 SVC 204/205 | K selects the invocation personality, checks the U lowcore veneer and copies U parameters. The heap, files and lowcore belong to the active invocation and are restored/reaped through its resource ledger. U supplies unchanged call blocks. | [P1](../qualification/TWO-SPACE-P1-2026-10-06.md) owns complete invocation resources/lowcore. [P2](../qualification/TWO-SPACE-P2-2026-10-07.md) qualifies selected storage, file, line/input and FST forms. [P3](../qualification/TWO-SPACE-P3-2026-10-07.md) adds PROGRAM calls and caller file preservation. Wider normal workloads remain P6. |
| TSO SVC 120 and 64-bit stacking PC | K owns storage intervals, full-width U addresses and real frames; it copies the selected IARV64 parameters into K. | Selected 24/31/64 heaps/free, low-space exhaustion RC 4 and unsupported IARV64 error. No silent 24-bit promotion. |
| TSO SVC 93 line output | K validates the U buffer and owns the terminal channel and active invocation. | [P2](../qualification/TWO-SPACE-P2-2026-10-07.md) qualifies selected native TPUT/TGET and exact input. P3 adds the unchanged HIGH version routes. P4 owns full-screen forms, capabilities and model-aware line policy. |
| Diagnostic native invocation SVC 235/236/237 and 244 | K resolves a loaded-image selector to CMS or TSO personality, AMODE, image and runtime owners, then records all full-width GPRs, both old-PSW halves, ASCE, key, nonreused token and owned resources. The U fixture surrounds selected RXVM entries; SVC 237 deliberately leaves one page live for K cleanup. Private SVC 244 reads the saved continuation while its frame is active. | [TSO entry IPL](../qualification/TWO-SPACE-INVOCATION-GATE-2026-10-06.md) proves TSO24/31/64 ANY owner-based SVC 120, IARV64 and TPUT. [Cleanup IPL](../qualification/TWO-SPACE-INVOCATION-REAP-2026-10-06.md) proves a live TSO31 page is released. [CMS entry IPL](../qualification/TWO-SPACE-CMS-INVOCATION-2026-10-06.md) proves CMS24/31 native and relocated image selection and the FST veneer. [Full-PSW IPL](../qualification/TWO-SPACE-CALLER-PSW-2026-10-06.md) compares the saved address with the actual post-SVC label and rejects a query after return. These private controls are not an application ABI; [P3 native adapters](../qualification/TWO-SPACE-P3-2026-10-07.md) now supply controlled native launch; normal boot integration remains P5. |
| Diagnostic K-controlled call SVC 245/246 | K selects a registered U entry, validates the interrupted U state, owns the invocation and its resources, enters through the saved PSW, then restores caller GPRs except result registers, PSW and ASCE on normal return or a U fault. R0 reports OS status separately from the R15 application RC. The current two entries and return trampoline belong to the diagnostic fixture. | [Fresh IPL](../qualification/TWO-SPACE-CONTROLLED-CALL-2026-10-06.md) checks normal RC, fault status, caller GPR restoration, child-page unmap, zero image leases and intact DAT checksums. [P1](../qualification/TWO-SPACE-P1-2026-10-06.md) closes pending-I/O fault cleanup; [P3](../qualification/TWO-SPACE-P3-2026-10-07.md) closes selected native linkage and nested caller restoration. |
| CMS and TSO program invocation | K owns the loader and invocation stack, with copied parameters, OS status and application RC; U PCOMM presents results. | [P3](../qualification/TWO-SPACE-P3-2026-10-07.md) qualifies checked placement, native CMS PROGRAM/TSO LINK, selected mode crossings, separate OS/application results and return/fault cleanup. PCOMM normal-image integration remains P5; CMS system() and REXX ADDRESS are outside this required macro-level contract. |
| CMS/TSO files and persistent output | K owns dataset extents and disk channel buffers. CMS input cursors and transient output buffers are keyed by the active invocation token. Open handles are reaped on return; FINIS publishes closed output to the allocated, banked PDOS.STORE extent. | [P2](../qualification/TWO-SPACE-P2-2026-10-07.md) qualifies selected native file/dataset services, durable commits and stopped-disk readback. [P3](../qualification/TWO-SPACE-P3-2026-10-07.md) qualifies both personalities’ same-name parent/child cursor ownership and return/fault release. |
| 3270 display and optional 3215 monitor | K owns device capability, channel completion, screen and input leases; U C presentation builds fields and command events. | Selected 3270 line output/input, [invocation-owned waiting-read](../qualification/TWO-SPACE-TERMINAL-OWNER-2026-10-06.md), [post-start clear and retry](../qualification/TWO-SPACE-TERMINAL-RETRY-2026-10-06.md), and [retired ledger completion under a new owner](../qualification/TWO-SPACE-LATE-COMPLETION-2026-10-06.md) pass. Physical late-channel status remains open. [P4](../BACKLOG.md#pd-003-completion-plan-6-october-2026), [PD-021](../BACKLOG.md#pd-021-reusable-3270-application-presentation) and [PD-022](../BACKLOG.md#pd-022-attached-operator-line-view-and-transcript) own model, transcript and line-only gates. |

## Low U placement checkpoints

The observed free 24-bit U interval after the CMS24 image is
`0x1bb000`–`0xefffff`, 13,914,112 bytes. The `0x0`–`0x1ffff` interface
reservation and `0xf00000`–`0xffffff` stack/guard explain its endpoints.
This is the largest contiguous virtual interval, not a promise of real
frames or heap capacity. The TSO24 image, stack, terminal buffer and heap
temporarily reduce it; its fresh-IPL exit restored the unmapped gap.
K code, DAT, terminal queues and channel buffers must remain outside U's
low range. Repeat the measurement at PCOMM ready, nested entry, peak live
24-bit demand and return in the normal successor image.

## P0 frozen native layouts and service coverage

The owning source is `pdos/src/pdos.c` for qualified one-space behavior and
`pdpclib/src/native/mvssupa/` for Classic native linkage. The application
adapter source lives in the separate Mainframe ELF SDK: `libc/src/adapters/`
`cms`, `tso` and `tso64`. Source traces are checked against the pinned
application bytes and the one-space [stage-3 CMS result](../qualification/STAGE3-2026-10-05.md)
and [TSO acceptance](../qualification/QUALIFICATION.md). Later SDK source
changes do not silently change these frozen binaries or their contract.

All integer and pointer fields below are big-endian. CMS24 and CMS31 FSCBs
have 44 bytes: command at 0, filename 8, filetype 16, filemode 24 (two
bytes), record number 26 (halfword), buffer pointer 28, capacity 32,
record-format bytes 36, requested record count 38 (halfword), returned
byte count 40. CMS24 strips the defined call-type/address flag byte and
requires the resulting pointer below 16 MiB; CMS31 uses a 31-bit pointer.
A 40-byte FST supplies filemode at 24, record format at 30 and LRECL at 32.
No pointer is dereferenced by C31 until K has checked its full U span.

| Native operation | Required parameters and native result | K owner and acceptance assignment |
| --- | --- | --- |
| CMS24 SVC202 | R1 names a bounded token/FSCB list below 16 MiB. The four-byte inline error continuation follows the SVC. Program call type is 0. R15 is the native result. | CMS invocation adapter and command service. P2 runs IO24; P3 runs the frozen CMS24 parent/child, including reversible fixed-origin overlay. |
| CMS31 SVC204 | R1 names the complete 31-bit list; R0 is zero for PROGRAM without EPLIST. R15 flags 0 select ordinary call; `0000a000` selects COPY/FENCE for a below-line child; `00e00000` selects the reached CMSSTOR route. | CMS invocation, copying and storage services. P2 covers all reached file/input verbs; P3 covers native CMS31 and 31→24 calls. |
| DMSFROSV/DMSFRRSV | 32-byte STPL: command 0, subpool 8, size 16, address-register selector 24, flag bytes 28/29 and subpool code 30. Obtain supplies the same size in R0 and returns address in R1; release supplies R8. Native R15 0 means success, selected error 12 means failure. | Invocation-owned heap, no foreign free. P2 obtains/releases the unchanged CMS31 64 MiB heap. |
| CMS STATE/RDBUF | STATE returns FST pointer at FSCB+28 or R15 28 when absent. RDBUF record 1 restarts the cursor; success writes exact record bytes and count at +40; EOF is 12. | Invocation-owned input cursor and K dataset service. P2 compares compiler source/library reads and nested-cursor controls belong to P3. |
| CMS WRBUF/FINIS/ERASE | WRBUF has an exact record length at +32; native bytes are opaque. FINIS commits closed output before returning 0; a native empty text file is one blank record. Missing file is 28; other selected failure is 12. | Invocation-owned output staging and checked durable dataset writer. P2 reads stopped output records back; P3 preserves parent handles on child completion. |
| CMS FST lookup | U word `14` points to SYSREF, whose +12 entry is the U SVC205 veneer. R0 carries the cursor; the R1 high bit requests continuation; R15 is status and returned R1 is the FST. | K directory cursor with U compatibility-page result. P2 covers RXC/RXAS library resolution. |
| CMS LINEWRT/LINERD | LINEWRT list: command 0, pointer 8, length 12, fence 44. LINERD is 56 bytes: pointer 8, capacity 12, flags `ce/c0` at 36/37, count 40, next-length 44 and fence 48. Input count returns in R0. P2's native input trace reconciles the producer struct with the qualified one-space flags/count path; LINEWRT and LINERD have different fence offsets. | K terminal ownership/completion and bounded copying. P2 preserves actual empty, padded and accented input. |
| CMS TYPLIN/WAITRD | TYPLIN uses pointer `01000000 OR address24` at +8 and `c2800000 OR length` at +12. WAITRD uses the same pointer, bytes `e3/00` at 12/13 and capacity/result halfword at 14. | Distinct CMS24 adapter into the same K driver. P2 runs the unchanged IO24 input/output path. |
| TSO SVC120/10 | R0 size, R1 zero for obtain or address for release, R15 selected request bits: `10` below-line or `30` above-line. Conditional failure is R15 4 with zero returned address. SVC10 is the reached legacy release form with size in low 24 bits. | Invocation-owned storage and checked real/DAT release. P2 preserves 24-bit failure and 31/64-bit headroom. |
| TSO IARV64 PC | U PSA+16→CVT, CVT+772→SFT, SFT+208 plus EX14→stacking PC. The selected 88-byte version-0 plist has request 1/3 at byte 1, conditional flag `80` at byte 5, segment count +8, returned origin +40 or detach origin +56. R15 status and R0 reason are distinct. | K high-memory object owner; full-width origin. P2 runs the unchanged 128 MiB ANY heap. P3 qualifies HIGH's source-backed callback and per-invocation objects. |
| TSO SVC99 | R1 points to a flagged RB-address word; RB is 20 bytes, with length 20 at 0, verb at 1, error/info halfwords 4/6 and TU-list pointer 8. Each TU has key/count/length halfwords then value; final TU pointer has bit 31 set. Selected keys: DD `0001`, DSN `0002`, disposition `0004` value 8, returned DD `0055`, reached compatibility flags `001c/0042/0049`. | Invocation-owned DD bindings, no global mutable application TIOT. P2 covers allocate/free and the full compiler file chain. |
| TSO RDJFCB/OPEN/CLOSE | SVC64, 22 and 20 use R1 control lists with flagged 24-bit DCB addresses. OPEN modes `80` input, `8f` output and selected `83` input/output; unsupported modes fail. An 88-byte DCB has EODAD +32, RECFM/exit-list +36, DDNAME +40, GET/PUT +48, CHECK +52, block size halfword +62, LRECL halfword +82, NOTE/POINT +84. Callback pointers must name checked U veneers. | K file handle, copied DCB state, checked U exit dispatch. P2 ports the reached PDPCLIB forms without exposing K code in U. |
| TSO record callbacks | U write/read veneers issue SVC250/251. R1 points to 20-byte DECB: ECB 0, read/write flags 4/5, length halfword 6, DCB pointer 8, buffer pointer 12, optional record/IOB pointer 16. Completion/error and residual must be written before CHECK/EODAD observes them. | Invocation-owned pending I/O and K-only disk channel buffer. P1 owns completion/unwind; P2 owns exact VB/SEQ/PDS records and durable readback. |
| TSO AOPEN TRKCALC compatibility helper | The unchanged RXVM AOPEN loads U PSA+16, then CVT+232, and calls that entry+12. The qualified one-space `pdossup.asm` TRKCLC returns the conservative buffer-count result R0=3 and R15=0 without interpreting its parameter list. | Unprivileged U compatibility veneer published by K. This preserves the selected one-space result; it does not provide a general device track-capacity calculation. P2's actual unchanged native AOPEN exposed this additional form; owning source and native instruction bytes resolve it before P2 acceptance. |
| TSO file CHECK/WAIT | The unchanged native CHECK issues SVC1 with R0=1 and R1 naming its file ECB. One-space read/write is synchronous and its WAIT is a no-op after completion. | K returns success only for the current invocation's registered file ECB, posted after event-verified I/O and data/residual publication. An unposted or foreign ECB cannot report success. This reached form is resolved from the unchanged RXVM CHECK instructions and the qualified one-space SVC1 path before P2 acceptance. |
| TSO BLDL/FIND/OBTAIN/DEVTYPE | SVC18 uses R0 member/list and signed R1 DCB selector; one-entry BLDL count/entry size precede the name and returned TTR. SVC27 CAMLST SEARCH uses flags `c1000000` followed by DSN, volume and 96-byte DSCB output pointers. SVC24 and the reached JFCB/UCB/TIOT fields remain below-line compatibility objects. | K dataset/member lookup with U-owned compatibility fields. P2 qualifies the selected runtime file paths through unchanged TSO31 RXVM/IOQUAL. Full compiler/assembler workloads remain P6; general MVS catalogue services are outside the subset. |
| TSO TPUT/TGET | The released bridges switch to AMODE24/31 and obtain a 256-byte below-line terminal buffer. R0 is length/capacity; R1 high byte 00/80 selects line output/input. Input count returns in R1 and R15 is status. Upper halves and R2 are not inputs. | K primary-terminal owner. P2 preserves native line bytes; P4 supplies the frozen full-screen subset below. |
| TSO LOAD/DELETE | The unchanged HIGH launcher uses the separate SVC8/9 contract already traced above, with full-width entry in GPR0 and owner-checked unload. | K native loader and invocation image lease. P3 enters all three unchanged HIGH bodies and preserves their R2→launcher R15 return path. |
| PCOMM ATTACH/completion | SVC42 R15 addresses its 28-byte control list: EPLOC 0, ECB 8, optional sharing word 16 and flag 27. The observed dynamic work area leaves optional fields zero; the first subset also accepts the source-selected sharing 78 and flag 8. R1 points to the flagged halfword-length command tail. SVC3 posts TCBCMP; SVC1 observes the ECB; SVC62 releases the parent-owned completion token. | Neutral PDOS parent with K-owned child descriptor. P3 carries this existing behavior into the normal command route; P5 integrates its boot selection. |

The TSO service tables are part of the unchanged producer contract: version
6 uses eight 32-bit words (version, putline, allocate, release, filecall,
finish, readline, inspect-state). Version `6403` has 64-bit function pointers
at 8/16/24/32/40/48, low work pointer at 56, work size at 64 and inspect-state
pointer at 72. The HIGH launcher explicitly reserves a 64 KiB low native
work area. File operations 0–5 select open/read/write/close/DD allocation/DD
release; the LP64 file bridge marshals into those bounded low native lists.
K must preserve these tables and callbacks; it does not substitute a new C
calling convention for the native application interface.

### Native application-call fixture freeze

`pdos/tests/two-space/p0/` owns independent native parent/child source.
`two-space-p0.crexx` assembles it with Classic Assembler and links ordinary
CMS MODULE and MVS native load formats. The CMS pair passes a token
`D003ARG`; the child returns 37 only for that token. The TSO pair passes a
fenced address list to a fullword 37; its child terminates its LINK request
block through native SVC3. None uses private successor launch SVCs.

The CMS24 parent uses SVC202 and its inline continuation; CMS31 uses
SVC204, CALLTYP PROGRAM 0, no EPLIST. The native call list limit is 68
eight-byte tokens including the `ff` fence, while PCOMM's existing command
parser remains limited to 14 child tokens. COPY/FENCE below-line transfer
must complete before entry and be released after return/fault.

For TSO, the [primary SVC6 definition](https://www.ibm.com/docs/en/zos/3.1.0?topic=descriptions-svc-6-0a06)
pins the 12-byte control list: EPLOC fullword at 0, extension flag `80` at
4, zero DCB bytes 5–7, optional ERRET address at 8. R15 addresses that
control list and R1 addresses the optional problem parameter list. The
selected list has at most 16 low-address fullword pointers with a fenced
last pointer; no DE, private DCB, AR/ALET or PLIST8 form is selected.
The 64-bit caller fixture preserves full R14 and uses the same below-bar
control/argument subset. A target AMODE64 receives the primary linkage
mode marker `fffff000`, `fffff002` or `fffff004` in R15; this differs from
the unchanged HIGH launcher's LOAD-entry address/tag protocol.

[LINK/LINKX](https://www.ibm.com/docs/en/zos/3.1.0?topic=module-link-linkx-description)
returns in the caller's AMODE. The selected missing-member result is abend
806 reason 04. A valid ERRET receives reason in R1 high word, abend in its
low word and its entry in R15; bad input lists do not use ERRET. Without a
valid exit, failure terminates only the active invocation and returns K OS
status to PCOMM. Native child RC and OS failure are always separate in K.
These layouts derive from primary references and native source/binaries;
[P3 execution](../qualification/TWO-SPACE-P3-2026-10-07.md) accepts the frozen pairs, selected mode crossings and exact missing-member error exit.

### Terminal and capability subset frozen for P4

The released cREXX runtime reaches line calls, not raw full-screen calls.
P0 therefore distinguishes reached native line ABI from the planned full-
screen and presentation surface. CMS CONSOLE and native TPG forms are not
silently inferred from LINEWRT or SVC93; unsupported forms return a native
error and cannot be logged as successful screen or text output.

The first native full-screen compatibility probe is TSO SVC93 register form,
with below-line buffer, R0 length, R1 high-byte flags `03` output or `83`
input (FULLSCR/WAIT, no USERID). The
[register-form reference](https://www.ibm.com/docs/en/zos/2.5.0?topic=tpg-register-form-tget-tput)
provides that flag layout. K validates and owns the complete raw stream,
serializes it with the screen lease and rejects malformed orders/addresses.
The normal line paths retain their qualified 130/132-byte limits. The
versioned PDOS presentation route supports full-width U pointers without
requiring a resident low-U console image. This is a selected interface,
not a current native full-screen guest qualification.

`twospace_abi.h` freezes neutral PDOS SVC200 version 1: R0 request byte count,
full R1 U address, R2 operation. Operations 0/1/2 query capabilities/write
line/read line; 16/17/18 acquire/write/release screen; 32/33/34 query monitor,
hand off line input and emit transcript. K takes ownership from its active
invocation; a caller cannot nominate another owner. A 64-byte capability
record contains version/size, primary device class/address, active/default/
alternate geometry, address encoding, attributes, features, line/screen
limits and generation. Every offset is in the header. Unknown versions,
short buffers and unreported features return explicit errors.

Line requests have a 32-byte version/size/flags/capacity/full-U-address/
returned-count/AID record, with native EBCDIC bytes. Flags are zero for the
first synchronous subset; empty input returns count zero, not EOF. Screen
coordinates are zero-based and the complete field vector and every text span
are preflighted before any channel start. The line and screen result is R15
OS status; result fields are written only for the accepted operation.

The supported P4 geometries are model 2 24×80, model 3 32×80, model 4 43×80
and model 5 27×132. Query or validated configuration supplies the actual
geometry. A line primary reports zero screen dimensions and no screen
feature. Screen requests have a 32-byte header and up to 64 24-byte field
records (row/column/attributes/length/full U address), with checked native
text and an encoded stream limit of 16 KiB. U C code owns header/footer,
scrolling and entry layout; K C31 code owns the model-aware encoder/device.

A transcript record has a 32-byte version/type/64-bit-sequence/payload-size/
invocation/encoding/flags header. Types are command-begin, native text,
command-end with the separate 32-byte K result, raw screen-change and gap.
Sequence discontinuity and disconnect are visible; raw screens never imply
complete text. A monitor is optional, has no input ownership until an
explicit handoff at a line prompt, and cannot reduce the 24-bit U interval.
P4 qualifies loss, reconnect and every advertised feature on named devices.

### P0 controls assigned to later steps

| Controls | Owning acceptance |
| --- | --- |
| Exact CMS24 IO24, CMS31 RXC→RXAS→RXVM, TSO31/64 ANY/HIGH compiler/file/input paths; native TSO24 separately; exact record output and stopped readback | P2 selected services, then P6 unchanged workload matrix |
| One bad U pointer/short list per native layout class, a missing file/member and one failed durable write; reuse existing corruption-boundary checks | P2/P3; no expanding malformed-binary campaign |
| Frozen CMS24/31 and TSO24/31/64 native parent/child calls, parameter 37/RC37, missing child, mixed-mode return, fixed collision and storage exhaustion | P3 |
| Full-width registers/PSW/ASCE/key, parent lowcore/handles/cursors, child resources on normal return and fault, pending I/O cancellation and stale completion | P1, then the real nested P3 path |
| Four display models, child-return repaint, full-screen request on line primary, optional monitor, line-only primary, explicit input handoff and transcript gap/reconnect | P4 |
| Stable prompt, ordinary commands, independent K emergency/shutdown, unchanged low-U headroom and repeated start/stop on the selected normal disk | P5/P6 |

### Six-profile package identities

The new P0 inventory checks all four CMS MODULEs and thirteen TSO XMI
members against the current pinned ZIP `fd11ae26…`. The CMS MODULE hashes
match the previously qualified package `d5e2c8bc…` despite the different
outer archive identity. All original load bytes remain unchanged.

| TSO package member | Directory byte | XMI SHA-256 | Native RDW SHA-256 |
| --- | --- | --- | --- |
| tso24/RXVM.XMI | `00` | `53d5c9ae539f6f8efa0f76968d9e7eb8a473aca586822384fd3b79ddc5e164e2` | `d9ce6e48f83e0bb4096e14e6bd285d350c8f38bc19cff8671d3d9155a96fcdd0` |
| tso31/RXAS.XMI | `12` | `b1187e8b0da66093bdeda6f76b5b36ad078d508103514475421dd5402105fb39` | `52fb1901572f36de0e7128b668965bd7f8686c0dee37b0c8de5dc613357393f1` |
| tso31/RXC.XMI | `12` | `838434cc8c74e6c60c8c590a2a791dbde0309c7dab311035e0bf27d1a730a236` | `eba277736b4df9ca1c79170ee3b29a0299fe74c7b18568e08a8703abb720a1a0` |
| tso31/RXVM.XMI | `12` | `c63e6acfc799f4ea21ca05cb6d31b1d363c6b0b239e2b24283a6be74ee506e1a` | `77b689a3c64bad63996031654dfc353d357b9c2616da01c65a9ea3e2c09c2cc9` |
| tso64-any/RXAS.XMI | `11` | `5392d6a2181382c729da002aa66f5a6fe129f2d6f62d270002864bbbcb83dc5d` | `b05ce7e8c2b9ab06384c71e181dc35ce1514cc5ad23981ec16987f61e6f5be56` |
| tso64-any/RXC.XMI | `11` | `820bddd54bef7805065f92b5ea3a3f035542ae0f1097ba2772e8c3bd5378108e` | `a302dfcacc07132bfba7b9bb14cb0797e34007fa28f6f04a7d81f6d7bfc64fdc` |
| tso64-any/RXVM.XMI | `11` | `2038c41a7e13a573bba9b16cf4446f1eb3c29f050594ae0ce0e37715095c13a2` | `dc5210b04cabf93d22942b0955ccf9bf63f7e0d4334a88e2a04e8705484339b6` |
| tso64-high/LAC65O.XMI | `11` | `705bf0d375933da8e9a8633985fdd153bb61a2c2d228399b198688308cc2c194` | `0037a2ef67e59e0db0e1ac054c592c0feb00601a211b8882bc2b1c2e897f91ae` |
| tso64-high/LAU65O.XMI | `11` | `4761c85d85f1cdc36080d20ab9d1474a25beafac22d9d79d650a869d371556cd` | `b2d7f4e0dfc070065664dceca2f003be187e309803f1f2ef5067ca57fb7ac8ba` |
| tso64-high/LAVM65O.XMI | `11` | `0418b47b12a76de49317bd60326449c0fde7940afb3d0cfaf6dcba41a0ab261b` | `cd87b17f540952745240462f9c847011a449996e0e08725bc0dee286c4b3e527` |
| tso64-high/RXASH.XMI | `31` | `ce97361a9dad704ceb29b2db3afc262d3fa0c6c19b7b149959120fd6db399a50` | `480cb94ea1f511956ae1df8ef3b632620c4048d690b4a922bacec599025c05f9` |
| tso64-high/RXCH.XMI | `31` | `c5d141f086e2509d0f0222c20f1b7c25aba96f1889e7091e7aeb3fa2f2ec95d1` | `2f0d99baaaf9a6089fcaaa75de2bc945ea92eea16832f581ed9726584f43be0b` |
| tso64-high/RXVMH.XMI | `31` | `61332683a9c47b3b425c484728401f42f17e4fd8075449f2b924f4936d6518b2` | `9d45e418f1ce45253ec0a09dfb3015d856c75bda96aa55cb5bf8dae411ac438f` |

### Frozen 24-bit placement budget

CMS24's 1,681,088-byte image already contains its 64 KiB C stack and selected
512 KiB static heap. They must not be counted twice. The U interface reserve
is 0–`1ffff`; the rounded fixed image is `20000`–`1bafff`; the separate guest
stack/guard reservation is `f00000`–`ffffff` with guard `f00000`–`f00fff`.
The largest free interval after return is `1bb000`–`efffff`, 13,914,112 bytes.

The unchanged TSO24 image uses `400000`–`50bfff`. Its observed live runtime
allocations were stack `1bc000`/1 MiB, terminal `2bc000`/256 bytes (one page),
heap `50c000`/4 MiB, and a temporary 4 KiB parameter/bridge page at `1bb000`.
At that peak the largest remaining contiguous interval is `90c000`–`efffff`,
6,242,304 bytes. Return restores the CMS24-only interval. The raw allocation
receipt is retained with the [released TSO24 checkpoint](../qualification/TWO-SPACE-TSO24-BETA3-NATIVE-2026-10-06.md).
This is virtual placement capacity, not a real-frame guarantee.

CMS31/TSO31 use 64 MiB runtime heaps; TSO64 ANY/HIGH use 128 MiB. Those
budgets remain independent of constrained 24-bit storage. K bodies, tables,
heaps, terminal queues, DAT aliases and low-real channel workspaces never
consume these low U virtual intervals. P5/P6 repeat the measurements with
normal U PCOMM and the real nested workloads present.
