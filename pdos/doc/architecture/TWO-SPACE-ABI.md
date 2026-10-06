# Two-space successor: executable ABI inventory

This is the P0 audit for the [K/U successor](TWO-SPACE-POC.md). It is a
working executable contract, not a claim that the normal replacement image
exists. P0 freezes the required ABI from unchanged binary/source evidence
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
| CMS24 RXVM MODULE `04550b86d7bdfa3c9060d9fa154875adf3ccb677d95ba13290836ddc02a8da9e` | Fixed U `0x20000`, page-rounded end `0x1bb000`; SVC 202 with flagged 24-bit parameters; U stack at `0xf01000`–`0xffffff` with an unmapped guard page at `0xf00000`. | [Fresh IO24 IPL](../qualification/TWO-SPACE-CMS24-IO24-2026-10-06.md) ran `-v` and library-free IO24, exact output and RC 0. Missing-file and bad-address controls passed; a checked overlay restored the earlier image. Native CMS-to-CMS command linkage remains open. |
| CMS31 RXVM MODULE `a08f3431407f8efec6b05f1e0580e3f12b76f9865f561dac15b5a90135223ff9`; RXC `a61f9ba543c2a471bae9551d6c9b55b121e5bbfae3a98f95797ea1712c1bfde8`; RXAS `c8b6eb5ce785eb3413c8f02528949f1f5643a016a519370ca2188e308926c910` | Relocatable RXVM at U `0x03000000` and `0x05000000`; lowcore `0x14` SYSREF, SVC 205 FST veneer, SVC 204 CMSCALL subset, 3 MiB image C stack and 64 MiB runtime heap. | [Fresh IOQUAL IPL](../qualification/TWO-SPACE-CMS31-IOQUAL-2026-10-06.md) ran RXVM `-v` and IOQUAL with exact output/RC and separate cursors. Missing library, corrupt file envelope and bad U return buffer have negative controls. RXC/RXAS execution and general command linkage remain open in this successor. |
| TSO24 RXVM beta 3 disk stage `1335bd64e276ed5fc76a6198704fdde67f7e1f0748dea2d5f25ab81a53bf666a` | AMODE24/RMODE24 image at U `0x400000`, 268 pages; EBCDIC `-v` tail, 1 MiB stack, 256-byte terminal buffer and 4 MiB heap below 16 MiB; SVC 120 storage and SVC 93 TPUT. | [Fresh beta 3 IPL](../qualification/TWO-SPACE-TSO24-BETA3-NATIVE-2026-10-06.md) returned RC 0 and one exact line. A real collision returned loader RC 4 without publishing an image; release/retry succeeded. TSO file/input and native TSO-to-TSO calls remain open. |
| TSO31 RXVM beta 3 stage `9fffaba3c586c1f361c7ecb87167fb1b8acbb569c267b621187fe95305bce506` | AMODE31/RMODE ANY image at U `0x07000000`, 268 pages; EBCDIC tail, 1 MiB stack, 64 MiB heap, SVC 120 and SVC 93 TPUT with R0 length/R1 low-31-bit address. | [Fresh native IPL](../qualification/TWO-SPACE-TSO31-NATIVE-2026-10-06.md) returned RC 0 and the exact version line. The first ASCII-tail probe failed; an incorrect TPUT register assumption was rejected and corrected. General file/input and calls remain open. |
| TSO64 ANY RXVM beta 3 stage `bdc2d14439f1a3c90409313a9693672c61187b78277197d4dc19bdd6` | AMODE64/RMODE ANY image at U `0x09000000`, 188 pages; U CVT/SFT and SVC 233 plus PR veneer for IARV64 stacking PC, 128 MiB high heap, SVC 93 TPUT. | [Fresh native IPL](../qualification/TWO-SPACE-TSO64-NATIVE-2026-10-06.md) returned RC 0 and exact output; GETSTOR/DETACH left no high-U mapping. Unsupported IARV64 forms return an error. File/input/call services remain open. |
| TSO64 HIGH beta 3 compiler, assembler and VM pairs | [The six unchanged XMI and RDW hashes](../qualification/TWO-SPACE-TSO64-HIGH-BOUNDARY-2026-10-06.md) pin low AMODE64/RMODE ANY launchers `LAC65O`, `LAU65O`, `LAVM65O` (directory `0x11`) and high AMODE64/RMODE64 bodies `RXCH`, `RXASH`, `RXVMH` (directory `0x31`). All six entry offsets are zero. | [One-ASCE release qualification](../qualification/QUALIFICATION.md) ran HIGH compiler/assembler/VM and LOAD/DELETE. The successor [AL8 materializer](../qualification/TWO-SPACE-TSO64-HIGH-LOADER-2026-10-06.md) matches all three bodies on the host and the ANY parser rejects RMODE64. The source-backed bounded SVC 8/9 subset is below. No HIGH successor guest entry exists. Exact launcher-to-body parameter and return linkage remains open. |

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

The table names the selected services exercised so far and the next gates.
An IBM compatibility name does not expand the implemented subset. The
[P0 completion checkpoint](../BACKLOG.md#pd-003-completion-plan-6-october-2026)
remains open until unchanged RXC/RXAS, TSO64 HIGH and same-personality
nested-call paths have their exact parameter, return, terminal and file
forms traced here from source/binary or one-ASCE evidence. Passing those
paths in the successor belongs to P2/P3. Earlier P1 ownership and synthetic
call checkpoints were built before P0 closed; they provide useful mechanism
evidence but do not validate an untraced application ABI.

| Native operation | K and U responsibility | Current proof and required control |
| --- | --- | --- |
| CMS24 SVC 202; CMS31 SVC 204/205 | K selects the invocation personality, checks the U lowcore veneer and copies U parameters. The CMS31 heap now belongs to the active invocation; CMS files and lowcore still need complete frame ownership. U supplies unchanged call blocks. | Selected DMSFROSV/DMSFRRSV, TYPLIN/LINEWRT, STATE/RDBUF/WRBUF/FINIS/ERASE and FST pass. [Per-frame heap and pre-free checks](../qualification/TWO-SPACE-INVOCATION-HEAP-2026-10-06.md) passed the host control and unchanged fresh IPL, but a nested CMS heap guest proof is open. Bad pointers, missing file and malformed envelope return errors. General application command calls remain open. |
| TSO SVC 120 and 64-bit stacking PC | K owns storage intervals, full-width U addresses and real frames; it copies the selected IARV64 parameters into K. | Selected 24/31/64 heaps/free, low-space exhaustion RC 4 and unsupported IARV64 error. No silent 24-bit promotion. |
| TSO SVC 93 line output | K validates the U buffer and owns the terminal channel and active invocation. | Selected TPUT `-v` on TSO24/31/64 ANY. TGET, full-screen forms, capability queries and wider line policy remain open. |
| Diagnostic native invocation SVC 235/236/237 and 244 | K resolves a loaded-image selector to CMS or TSO personality, AMODE, image and runtime owners, then records all full-width GPRs, both old-PSW halves, ASCE, key, nonreused token and owned resources. The U fixture surrounds selected RXVM entries; SVC 237 deliberately leaves one page live for K cleanup. Private SVC 244 reads the saved continuation while its frame is active. | [TSO entry IPL](../qualification/TWO-SPACE-INVOCATION-GATE-2026-10-06.md) proves TSO24/31/64 ANY owner-based SVC 120, IARV64 and TPUT. [Cleanup IPL](../qualification/TWO-SPACE-INVOCATION-REAP-2026-10-06.md) proves a live TSO31 page is released. [CMS entry IPL](../qualification/TWO-SPACE-CMS-INVOCATION-2026-10-06.md) proves CMS24/31 native and relocated image selection and the FST veneer. [Full-PSW IPL](../qualification/TWO-SPACE-CALLER-PSW-2026-10-06.md) compares the saved address with the actual post-SVC label and rejects a query after return. These private controls are not an application ABI; normal K-controlled launch of unchanged native applications remains open. |
| Diagnostic K-controlled call SVC 245/246 | K selects a registered U entry, validates the interrupted U state, owns the invocation and its resources, enters through the saved PSW, then restores caller GPRs except result registers, PSW and ASCE on normal return or a U fault. R0 reports OS status separately from the R15 application RC. The current two entries and return trampoline belong to the diagnostic fixture. | [Fresh IPL](../qualification/TWO-SPACE-CONTROLLED-CALL-2026-10-06.md) checks normal RC, fault status, caller GPR restoration, child-page unmap, zero image leases and intact DAT checksums. Native CMS/TSO entry and return linkage, nested controlled calls and fault cleanup with pending I/O remain open. |
| CMS and TSO program invocation | K owns the loader and invocation stack, with copied parameters, OS status and application RC; U PCOMM presents results. | Loader placement/collision and diagnostic overlays pass. The bounded CMSCALL/SVC 202 and LINK/LINKX forms above are selected, not implemented. The maintained C library's CMS `system()` is a placeholder, and its TSO CP request is rejected on PDOS. Unchanged CMS-to-CMS and TSO-to-TSO parent/child controls remain open. |
| CMS/TSO files and persistent output | K owns dataset extents and disk channel buffers. CMS input cursors and transient output buffers are keyed by the active invocation token. Open handles are reaped on return; FINIS transfers closed output to K's diagnostic store. | CMS selected read/write and transient output pass. The [input](../qualification/TWO-SPACE-INVOCATION-FILES-2026-10-06.md) and [output](../qualification/TWO-SPACE-INVOCATION-OUTPUT-2026-10-06.md) ownership checks passed unchanged IPL; nested same-name guest controls remain open. TSO file services, durable commits and stopped-disk readback remain open. |
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
