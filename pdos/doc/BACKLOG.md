# z/PDOS backlog

This is the only live roadmap, defect and qualification queue for this component.
Follow [the shared workflow](../../doc/WORKFLOW.md). Items distinguish
implemented behavior, recorded qualification and source-review findings. An open source-review item does not imply a reproduced guest failure.

## Ordered delivery roadmap

I want the operator route to lead the work: load a program, run a script against
it and see whether it conforms. Moving fixtures in and out comes next, with
CMS and TSO record and naming conventions handled explicitly. Then I want
z/PDOS to run the CMS cREXX programs as well as the already qualified TSO
ones. These are ordered outcomes, not claims that the current release provides
them or authorisation to implement every backlog item.

| Order | Outcome and first checkpoint | Items |
| --- | --- | --- |
| 1 | **Load, run and judge.** Start with an end-to-end TSO31 or TSO64 cREXX check from the base image using unchanged native program bytes. Make program installation, native batch delivery, per-command results and cleanup one repeatable operator flow. A human must also be able to enter commands longer than the 80-column 3270 field without silent truncation, read a stable prompt and input line, and tell which command produced each result. Bound executable reads to their dataset extent before accepting arbitrary staged packages. | [PD-013](#pd-013-repeatable-program-conformance-run), [PD-009](#pd-009-temporary-package-lifecycle), [PD-004](#pd-004-batch-file-delivery), [PD-005](#pd-005-command-length-diagnostics), [PD-007](#pd-007-results-and-scrollback), [PD-008](#pd-008-operator-diagnostics), [PD-006](#pd-006-console-input-and-prompts), [PD-012](#pd-012-direct-executable-reads-need-an-extent-boundary) |
| 2 | **Move fixtures both ways.** Make a second CKD volume discoverable, mountable and selectable while protecting IPL. Let the operator allocate a bounded output dataset on the selected exchange volume. Expose sequential tape record I/O using unchanged mounted images. Import/export CMS and TSO fixtures with explicit naming, framing and encoding, then check exact guest results. | [PD-018](#pd-018-multiple-dasd-volumes), [PD-020](#pd-020-safe-guest-dataset-allocation), [PD-019](#pd-019-virtual-tape-io), [PD-014](#pd-014-cms-and-tso-fixture-exchange) |
| 3 | **Run CMS cREXX binaries.** The current source's checked unchanged-MODULE route passed [local guest qualification](qualification/STAGE3-2026-10-05.md): CMS31 RXVM and a fresh RXC → RXAS → RXVM chain, plus the separate fixed-origin CMS24 RXVM IO24 subset. This does not qualify native TSO24 or the full historical CMS24 compiler chain. A TSO build of the same source does not establish CMS binary compatibility. | [PD-015](#pd-015-cms-crexx-binary-contract), [PD-016](#pd-016-cms31-crexx-execution), [PD-017](#pd-017-cms24-crexx-execution), [PD-001](#pd-001-amode24rmode24-application-loading) |
| Supporting gates | Qualify truthful invalid storage requests before widening the service surface, and correct region-table initialization before expanding mappings. Stage 3 measured less than 1 MiB of low heap headroom in the earlier one-space layout. PD-003 P0–P6 now qualify the protected K/U replacement, including native boot and workloads. | [PD-002](#pd-002-conditional-storage-service-errors), [PD-011](#pd-011-region-first-table-padding), [PD-003](#pd-003-two-space-supervisor-and-shared-application-memory) |

The current TSO31, TSO64 ANY and TSO64 HIGH guest results can be reused for
unchanged bytes and services. A new installer, fixture path, loader or CMS
service needs its own affected host, disk and guest checks. The Lab's private
stopped-disk tools and qualification scripts are useful prototypes; the
supported operator path belongs with this product and must not depend on
private paths, accounts or image versions.

Step 1 is implemented, locally guest-qualified and manually accepted in the
current source; see
the [operator guide](user/CONFORMANCE.md) and
[4 October evidence](qualification/STEP1-2026-10-04.md). Order 2 is implemented
and locally guest-qualified in the bounded source profile; see the
[media guide](user/MEDIA.md), [fixture guide](user/FIXTURES.md) and
[phase 2 evidence](qualification/PHASE2-2026-10-04.md). Order 3 is implemented
and locally guest-qualified for the bounded CMS31 and CMS24 workloads recorded
in the [stage 3 result](qualification/STAGE3-2026-10-05.md). These routes are
in the 0.1.1 release; the
[operator acceptance](qualification/0.1.1-OPERATOR-UAT-2026-10-05.md)
names the separate locally tested image. P6 now adds the protected K/U route and bounded native TSO24 IO24. Full-library
TSO24, CMS24 compiler/assembler and broader CMS/TSO services remain separate
work. See the [0.2.0 record](qualification/RELEASE-0.2.0.md) for its release gates.

The selected 13 October operator increment is
[PD-033](#pd-033-card-reader-batch-jobs-and-3270-job-forms): preserve the current
console, add a 3505/1403 native batch route shared with z/OS/Hercules, and
provide 3270 job forms over the same service. Its bounded device/job proofs
can use admitted existing media; retained spool/history and larger workloads
depend on PD-030's selected storage slice.

### Phase 2 delivery contract

1. **DASD discovery and selection.** `DEVICES` shows attached channel addresses
   without treating every address as CKD. `MOUNT address expected-volser` reads
   the real VOL1 label and refuses a mismatch or duplicate; `VOLUMES` shows
   mounted volumes; `SELECT volser` changes dataset and executable lookup.
   IPL stays protected and selected by default. An allocated DD pins its
   device; `UNMOUNT` refuses the IPL, selected and open volumes, and releases
   closed DD bindings. The first implementation supports four CKD volumes
   with LIFO unmount.
2. **Guest output allocation.** `ALLOC` creates a named sequential
   dataset on the selected exchange volume from an explicit record format,
   logical/block length and bounded space request. Refuse IPL, duplicates,
   unsupported formats, insufficient free space and invalid VTOC state
   without publishing a partial dataset. Report the chosen extent and
   return code. Existing-dataset `COPY`, record-preserving `RCOPY`, and native
   programs can then use it. This first allocator is confined to the
   100-cylinder `dasdload` 3390 exchange layout without a format-5 free map.
3. **Tape record I/O.** An operator attaches an unchanged AWS or HET image to
   Hercules, registers its device read-only with `TAPE MOUNT address`, and
   reads physical records through `TAPE READ` or an allocated `TAP:address` DD.
   Output uses a separate, explicitly writable tape image. Raw record,
   file-mark, rewind and output round trips have local guest evidence, as
   do short native `TAP:` read and write transfers. Larger native transfers
   and release-image interpretation still need their own checks;
   the container alone does not identify CMS or z/OS logical contents.
4. **Fixture interpretation and proof.** A manifest pins archive and image
   SHA-256, medium, format, logical name, source/destination, record length,
   code page and expected record/byte hashes. VMFPLC2 CMS HET and z/OS
   standard-label AWS are distinct adapters. Preserve their original tape
   bytes in the release ZIP. Stage disposable output on exchange CKD, run the
   checked mount/allocation/record-copy list, then export and compare logical
   records and exact binary bytes. Run an unchanged native program separately
   where its binary contract is supported. CMS MODULE execution has its
   separate order 3 qualification; a tape read alone is transport evidence,
   not a CMS or z/OS program pass.

## PD-001: AMODE24/RMODE24 application loading

- Type: defect
- Status: Open
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: TSO24 is rejected before dispatch; z/PDOS 0.1 qualifies the wider application routes.
- Evidence: doc/qualification/QUALIFICATION.md and qualification-0.1.json. The [successor TSO24 host loader check](qualification/TWO-SPACE-TSO24-LOADER-2026-10-06.md) is a separate partial result.
- Acceptance: Place code/save areas/parameters below 16 MiB, dispatch in the declared mode and qualify unchanged TSO24 RXVM I/O with its separate storage budget. Preserve resident applications when suitable storage exists; return a clear failure when the required low virtual interval or real backing does not exist. Never silently promote a 24-bit request above the line or evict an unrelated application.

## PD-002: Conditional storage-service errors

- Type: qualification
- Status: Open
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: Successful GETMAIN/FREEMAIN paths do not establish all invalid-request results.
- Evidence: The existing storage-service and guest checks.
- Acceptance: Check invalid requests and truthful failures without weakening unchanged-binary acceptance.

## PD-003: Two-space supervisor and shared application memory

- Type: improvement
- Status: Done
- Target: pdos-zarch nucleus, C31 supervisor services and shared AMODE24/31/64 application space
- Observation at the 0.1.1 checkpoint: The C32 kernel ran AMODE31 below 16 MiB with a one-MiB linked image slot. Its `PDOS` structure, mostly eager DAT and address-space tables, is 8,802,304 bytes; the initial aligned allocation leaves at most 630,784 bytes before the 12 MiB below-line application pool. Stage 3 exposed an overlap between the CMS24 fixed-origin image and its inherited low stack; the bounded bridge moves that stack to the reserved 15–16 MiB range. The image also covers PLOAD's 1 MiB boot heap. An initial 0.1.1 operator candidate returned from PCOMM but failed in PLOAD's C exit; the repaired z/Architecture kernel reaches its own `0444` wait after DAT is off. A trial with an 8 MiB heap start and 10 MiB application-pool start booted PCOMM but stalled at CMS24 execution; its initial kernel allocation could not fit in that gap, so the trial did not isolate the image-slot limit. Existing 31/64 application loads use above-line storage, but GETMAIN can still use or fall back to `btlmem`. Dropping CMS24 alone would not remove that dependency or bound the kernel heap. Kernel growth requires an independently backed K home for its body and bulky structures, with the real interruption island and 24-bit channel buffers handled separately. Moving the current linked image wholesale would put its 24-bit channel addresses and startup assumptions out of contract.
- Evidence: Machine contract, existing 0.1 qualification, and the [stage 3 guest record](qualification/STAGE3-2026-10-05.md).
- Direction: The [two-space contract and proofs](architecture/TWO-SPACE-POC.md) select one protected supervisor ASCE and one shared application ASCE. A small AMODE64 assembler nucleus crosses between them; Classic C services remain C31 in the supervisor space. The Steps 3/4 fixture executes a high K64 nucleus through sparse region-first DAT built by target-compatible C source, calls a Classic C31 service, returns to 24/31/64-bit callers, and runs a nested application call in shared U. A C31-compatible placement ledger rejects fixed-origin collisions and finds a free relocatable interval. The [Step 5 disk proof](qualification/TWO-SPACE-STEP5-2026-10-05.md) first booted that fixture through source-built PLOAD. The [slices 1/2 guest-DAT proof](qualification/TWO-SPACE-SLICES1-2-2026-10-05.md) adds a dedicated checked dataset, real-frame handover ledger and guest-built K/U tables. The [slices 3/4 proof](qualification/TWO-SPACE-SLICES3-4-2026-10-05.md) adds bounded five-class entry routing, two context frames, recoverable U faults and checked U transfers through K-owned C31 buffers. These component and PoC IPL results do not change the active one-ASCE release kernel.
- Acceptance: Separate the kernel image limit from its heap address, reserve independently backed K storage for DAT and kernel structures, eliminate PLOAD's boot-heap overlap, and audit 24-bit channel buffers plus GETMAIN low requests and fallback. Extend the real interruption island for every required interruption class; implement full-width context, PSW/key, K/U transition, bounded U-buffer access and C31 service dispatch with Classic tools. Inventory CMS lowcore/API conventions and preserve TSO SVC contracts without consuming unnecessary U low virtual pages. Add collision-checked relocatable/fixed module loading and a nested application-command gate, including a fixed-origin collision control. Qualify fresh IPL, CMS24/CMS31, TSO31/TSO64, low-memory budgets and failure recovery on named images. The Step 5 PoC IPL, [Steps 3/4 result](qualification/TWO-SPACE-STEPS3-4-2026-10-05.md), Step 2 fixture, stage 3 stack move, low-heap guard and direct shutdown wait are partial evidence, not completion.
- Call and console scope: The required nested gate is native CMS-to-CMS and TSO-to-TSO invocation with parameters, return code and caller restoration. Mainframe cREXX does not supply a REXX `ADDRESS` requirement. Cross-personality application calls are optional only when simple. Keep PCOMM in U, K's checked terminal and emergency output in K, and track the opt-in U 3270 wrapper under [PD-021](#pd-021-reusable-3270-application-presentation).

### PD-003 initial proof slices

These numbered slices identify the original bootable proof checkpoints and
their dated results below. The completion plan after this table owns the
order of remaining implementation. The one-ASCE release route remains
selected until the replacement has its own binary and guest qualification.

| Slice | Work and acceptance checkpoint |
| --- | --- |
| 1. Real-memory handover | Give the successor core its own versioned dataset and bounded image header. Reserve PLOAD, C31 stage, staging, launcher and final-core real frames by lifetime; check dataset extent, record sizes, integrity and overlap before the final copy. The image builder and stopped-disk checker must read back all source bytes. The fixture may retain its linked final real origins, but transient staging addresses must be selected from the ledger. |
| 2. Guest DAT | Carry no completed DAT tables or ASCEs in the disk package. Run the product sparse region-first builder in the C31 guest stage, assigning K and U table pools from reserved final-core frames. Compare guest-built tables and ASCEs against the existing host reference after a fresh IPL. Then add checked live map/unmap and frame-release operations with single-CPU translation invalidation; prove reuse and isolation before service migration. |
| 3. K64 interruption path | Cover SVC, program, external, I/O and machine-check entries, nesting, full GPR/PSW/key preservation, and recoverable application faults. A malformed caller must not strand the machine in an interruption loop. |
| 4. K/U service gate | Validate full-width U ranges, page translations, length, access direction and failures; copy through bounded K buffers. Never pass a raw U virtual pointer to Classic C31. Keep CMS and TSO personalities distinct. |
| 5. Services in K | Move the selected storage, channel/dataset, terminal and invocation services into K; keep the PCOMM command processor in U. Audit 24-bit real channel buffers and every below-line GETMAIN fallback. Keep K code, DAT and heaps out of U's low virtual area; measure actual CMS24/TSO24 image, stack and heap headroom. |
| 6. Shared-U loader and calls | Load fixed and relocatable CMS/TSO modules with a live interval inventory and checked AMODE/RMODE. Allow applications to coexist whenever their image and runtime storage fit the proper virtual range and real backing; never evict a live image as a normal placement policy. Fail a load explicitly if appropriate storage is unavailable, especially below 16 MiB. Never silently promote AMODE24/RMODE24 storage. Reuse storage after application completion. Qualify separate native CMS-to-CMS and TSO-to-TSO calls with bounded parameters, a return code and caller restoration. An explicitly selected fixed-origin suspend/restore path must verify caller state and return code. REXX `ADDRESS` and cross-personality application calls are not replacement gates. |
| 7. Replacement selection | Build a normal 3390 image selecting the successor explicitly. Qualify fresh IPL, unchanged CMS24/CMS31 and TSO31/TSO64 binaries, native TSO24 separately, low-memory exhaustion, bad service pointers, collisions, nested calls, faults and shutdown on named profiles before changing the default route. |

The [architecture review](architecture/TWO-SPACE-POC.md#architecture-decisions-for-the-first-replacement)
retains one shared U ASCE for the first replacement. The capacity of the
proper virtual range and real backing determines whether applications can
coexist; the loader must preserve live images and return a clear failure when
the requested storage is unavailable. Revisit separate CMS and TSO U ASCEs
only if unchanged workloads expose an unavoidable lowcore conflict, material
24-bit headroom loss, or a failure-isolation need that cannot be met in this
map without excessive complexity. A change requires its own machine proof.

### PD-003 completion plan, 6 October 2026

This is the remaining implementation sequence for the first normal two-space
image. It uses the selected K plus one shared U architecture and incorporates
[PD-021](#pd-021-reusable-3270-application-presentation) and
[PD-022](#pd-022-attached-operator-line-view-and-transcript). The diagnostic IPL
has already proved selected DAT, storage, channel and unchanged RXVM paths;
those proofs are inputs, not completion of a normal bootable replacement.
Record each checkpoint as a coherent change and, when authorized, a local
commit with its affected host, target-build and fresh-guest evidence. Pin the
machine profile, toolchain, unchanged application bytes and image hashes in
each dated qualification record. Do not rerun unaffected checks merely to
repeat an earlier result.

| Checkpoint | Implementation | Reviewable acceptance |
| --- | --- | --- |
| P0. Freeze the executable contract | Inventory the actual unchanged CMS24/CMS31 and TSO24/TSO31/TSO64 ANY/HIGH packages, load records, entry and return linkage, parameters, lowcore words, line and full-screen terminal calls, storage ranges and command paths. Define the selected service subset, capability queries and return codes; distinguish a loader/service failure from an application's RC. Keep the REXX `ADDRESS` operation and CMS-to-TSO application calls outside the required contract. | An ABI/service table in the architecture guide traces each required native operation to its owning K service or U component and exact unchanged binary fixture. Derive layouts and call conventions from unchanged binaries, their source and qualified one-ASCE behavior; mark unknowns explicitly and resolve them before the corresponding P2/P3 implementation. Name the positive and negative successor controls to run in P2/P3. Record the baseline 24-bit image, stack, heap and largest free low-U interval. No interface is inferred from a synthetic probe alone. |
| P1. Make K state and ownership explicit | Replace program-counter or global-profile inference with an invocation descriptor. Make image pages, U allocations, newly opened file handles, lowcore backing, terminal lease and pending I/O owned by that invocation, except for explicit ABI-defined sharing. Complete interruption nesting, event-driven I/O completion, cancellation and recoverable U-fault unwind without sharing the disk and terminal low-real workspaces. Keep real-frame and DAT-table release checked. | Fresh IPL proves normal return and fault cleanup, including a child fault, failed channel start, late completion and retry. K remains protected, all caller state is restored, and no owned frame, handle or low-U page leaks. Completion events decide success; elapsed time only fails a stalled test. |
| P2. Finish the selected K service surface | Complete the bounded CMS and TSO storage, dataset, file, terminal-input and command services identified in P0. Preserve their distinct ABI layouts, return conventions and encoding. Persist selected output records safely and read them back; validate every full-width U buffer before a C31 service touches it. | Unchanged selected CMS and TSO service probes pass in the diagnostic image with exact output, RC and error behavior. Invalid pointers, short buffers, missing members and failed writes leave no partial state or false success. The 24-bit U placement budget does not shrink because of K code, tables or buffers. |
| P3. Load and call real applications | Use the K placement ledger for fixed and relocatable modules. Implement synchronous CMS-to-CMS and TSO-to-TSO calls with bounded parameters, application RC and separate OS failure status. Restore the caller's AMODE, full registers, lowcore and terminal lease; preserve its file handles and cursors while releasing child-owned handles on return or fault. A fixed-origin nested call may use a checked reversible overlay; ordinary loads preserve every live image. | Native same-personality parent and child pairs run without z/PDOS-specific binary edits and return exact parameters and RCs, including relevant 24/31/64 mode crossings. A colliding or exhausted low interval, or missing real backing, returns a deterministic error without publishing a partial image or moving 24-bit storage above 16 MiB. Co-resident applications remain mapped whenever their required virtual and real storage fits. |
| P4. Deliver the C console and display models | Put the model-aware 3270 and line-device drivers and the 3270 encoder in K C31 C, leaving privileged channel instructions in assembler. Expose checked primary-device capabilities, separate monitor status and logical screen/input requests. Build the reusable U presentation code in C, first for PCOMM and an opt-in app; use explicit ABI adapters for other modes without consuming 24-bit U storage merely for the console. Route unchanged CMS/TSO line calls to the 3270 view and an optional attached Telnet 3215 text monitor. Give that monitor an explicit, exclusive input handoff only at line prompts. Define a versioned append-only transcript with command boundaries and distinct OS and application results. | Qualify model 2 24×80, model 3 32×80, model 4 43×80 and model 5 27×132 with queried or explicit geometry, address encoding, long output, AID/input, header/footer/scroll/entry layout and child-return repaint. On a named guest, qualify simultaneous 3270 display and attached line capture, a line-only primary, input handoff, loss/gap reporting and deterministic event-driven completion. Reject unsupported or ambiguous capabilities and malformed U requests. Full-screen calls on a line-only primary fail clearly; raw 3270 screens are never misreported as complete text. Disk I/O cannot corrupt pending terminal operations. |
| P5. Build a selectable normal image | Integrate PLOAD handover, K64/C31, U PCOMM, native loaders and services into a source-built 3390 image with an explicit successor selection. Retain the current release boot route until the new route passes qualification. Configure the primary terminal and optional text monitor explicitly. Keep K emergency output and shutdown independent of U PCOMM health. | A fresh disk IPL reaches a stable U prompt, launches both CMS and TSO commands, reports each RC, recovers from a failed command, and shuts down without stranded guest or host resources. A second attached terminal can capture the complete text transcript while the 3270 remains usable. Stopped-disk checks verify the image and persistent output after the guest stops. |
| P6. Qualify and select the replacement | Run the unchanged released workload set through the normal successor image: CMS31 RXC/RXAS/RXVM, the bounded CMS24 IO24 path, TSO31 and TSO64 ANY/HIGH compiler/assembler/terminal/file paths, and native TSO24 separately. Exercise nested calls, low-memory exhaustion, fixed collisions, corrupt inputs, bad U pointers, failed I/O, child faults, each terminal model, attached/line-only console modes, long transcripts and monitor disconnect/reconnect across fresh IPLs and repeated start/stop. Compare against the current one-ASCE acceptance without weakening its supported cases. | Record exact binary and image identities, per-case RC/output, low-U free interval and real-frame use, and a repeatable guest completion or failure event. Check that one command produces one ordered text and 3270 result, transcript gaps fail capture qualification, and 24-bit placement is unaffected by the monitor. Watchdogs only detect stalls. Select the successor as default only after all required cases pass on a named machine/toolchain and the operator and architecture guides describe the observed behavior. Publication or release is a separate decision. |

P0 freezes the required native ABI subset before dependent P2/P3 code is
committed. The successor guest controls named by P0 are executed and accepted
in P2/P3, so P0 does not depend on an implementation it is meant to guide.
Any newly discovered ABI form reopens the relevant P0 entry before code for
that form is accepted. P1's owner and completion model is a prerequisite for
expanding services or screen output. P3 supplies the invocation and
screen-lease stack used by P4. P5 is an opt-in integration route; P6 changes
the default only after its normal image qualifies. A separate CMS or TSO U
ASCE is not part of this plan unless the recorded shared-U review trigger is
met.

The [P0 ABI inventory](architecture/TWO-SPACE-ABI.md) now pins the selected
unchanged inputs and now freezes the selected native forms.
I now authorise completing P0 through P6 in that order, with a local commit
at each completed checkpoint. The earlier bounded PCOMM diagnostic work is
supporting implementation evidence; it does not replace this sequence. Each
step is closed only against its own acceptance. Publication remains a separate
decision. Required failure controls stay bounded to the named plan cases.

| Current checkpoint | Status | Accepted result / next gate |
| --- | --- | --- |
| P0 | Done | [Frozen native contract and inventory](architecture/TWO-SPACE-ABI.md), [P0 source/object qualification](qualification/TWO-SPACE-P0-2026-10-06.md), native call fixtures and versioned PDOS I/O/result byte layouts. |
| P1 | Done | [Complete ownership, caller context and interruption-driven return/fault cleanup](qualification/TWO-SPACE-P1-2026-10-06.md); 159-check fresh IPL, including a child fault with a submitted owned read and clean retry. |
| P2 | Done | [Selected native services, input and durable output/readback](qualification/TWO-SPACE-P2-2026-10-07.md); 169-check fresh IPL, unchanged CMS/TSO31 probes, exact input, failed-write preservation and complete runtime release. |
| P3 | Done | [Native loading/calls, mode crossings, HIGH and caller file preservation](qualification/TWO-SPACE-P3-2026-10-07.md); 187/190/189-check fresh IPL matrix, exact RC/parameters, both personalities' return/fault cursor controls and co-resident backing. |
| P4 | Done | [C display/device models, U presentation, monitor and durable transcript](qualification/TWO-SPACE-P4-2026-10-07.md); all four model IPLs, line-only, input handoff, loss/reconnect, raw-screen gap, PCOMM and a full-width U64 request. |
| P5 | Done | [Selectable source-built normal K/U image](qualification/TWO-SPACE-P5-2026-10-07.md); stable U PCOMM, CMS/TSO commands, explicit OS failure, complete attached capture, durable output and independent K emergency/shutdown. |
| P6 | Done | [Reviewed replacement qualification and source-default selection](qualification/TWO-SPACE-P6-2026-10-07.md): unchanged workloads, bounded native TSO24, calls/faults, four models, monitor controls, 47-command media workflow, exact exports and reviewed-image repeated/normal/fault shutdown. Local commit; publication remains separate. |

The later [HIGH launcher linkage check](qualification/TWO-SPACE-TSO64-HIGH-LOADER-2026-10-06.md)
ties all three frozen low launchers to their corresponding high bodies and
records their source-level parameter and return path. The P0 audit now
freezes the native application-call fixtures and selected terminal/file forms. Their successor guest passes belong to P2/P3.

The first [P1 invocation
component check](qualification/TWO-SPACE-INVOCATION-2026-10-06.md) proves
token and cleanup ordering on the host and a linked, uncalled Classic C31
module in the diskless fixture. The subsequent [fresh-IPL invocation
gate](qualification/TWO-SPACE-INVOCATION-GATE-2026-10-06.md) executes the
descriptor around unchanged TSO24/31/64 ANY RXVM calls and assigns SVC 120,
IARV64 and TPUT to that active owner. P0 was incomplete at that checkpoint;
the private U fixture still requested entry. A further [fresh-IPL allocation unwind
check](qualification/TWO-SPACE-INVOCATION-REAP-2026-10-06.md) proves that K
releases a live TSO31 page on end. The [CMS invocation
gate](qualification/TWO-SPACE-CMS-INVOCATION-2026-10-06.md) wraps five CMS24
and CMS31 entries, including the overlay child and second relocated RXVM.
The [K image-selection check](qualification/TWO-SPACE-IMAGE-SELECTION-2026-10-06.md)
derives personality, AMODE and owners from a checked loaded-image record and
rejects invalid or caller-placed entries. Fault-triggered unwind, pending
I/O, a normal K-controlled launch and other resource lifetimes remain open;
these checkpoints do not close P1 or qualify P2–P6.
The [TSO64 HIGH boundary inventory](qualification/TWO-SPACE-TSO64-HIGH-BOUNDARY-2026-10-06.md)
pins all three unchanged launcher/body pairs and prevents the ANY loader from
accepting an RMODE64 directory. HIGH AL8 materialization, native LOAD/DELETE
linkage and successor guest execution remain open.
The [HIGH AL8 materializer gate](qualification/TWO-SPACE-TSO64-HIGH-LOADER-2026-10-06.md)
now compares all three unchanged high bodies with the released loader at two
full-width U bases, and Classic C31 compilation passes. Staging and entering
HIGH in the guest, native LOAD/DELETE and normal-image qualification remain
open.
The [K image-lease gate](qualification/TWO-SPACE-IMAGE-LEASE-2026-10-06.md)
counts nested invocation references to a checked mapped image and releases
them as frames unwind, while leaving a reusable cache mapping in place.
Checked replacement of a leased image, eviction and native same-image calls
remain open.
The [CMS heap ownership check](qualification/TWO-SPACE-INVOCATION-HEAP-2026-10-06.md)
adds per-invocation CMS31 heap state and a pre-free K handle check for CMS,
SVC 120 and IARV64. The unchanged guest paths passed fresh IPL, but a nested
CMS heap and child-fault guest control remain open.
The [CMS input ownership check](qualification/TWO-SPACE-INVOCATION-FILES-2026-10-06.md)
keys open cursors by invocation token and reaps unclosed input buffers on
normal return. A nested same-file guest control and output ownership remain
open.
The [CMS output ownership check](qualification/TWO-SPACE-INVOCATION-OUTPUT-2026-10-06.md)
keys transient output buffers by invocation, reaps unfinished buffers, and
keeps FINIS-closed data in a K diagnostic store. Durable output and nested
same-name guest controls remain open.
The [subchannel clear check](qualification/TWO-SPACE-CHANNEL-CLEAR-2026-10-06.md)
adds event-verified CSCH/TSCH completion and an absent-device failure path.
An in-flight terminal cancellation and late-completion retry are still P1
gates.
The [nested lowcore check](qualification/TWO-SPACE-INVOCATION-LOWCORE-2026-10-06.md)
saves the parent's U compatibility page in K real storage and restores it on
child return. Its fresh IPL also caught and corrected an overbroad top-level
reset that erased a prepared CMS FST. Unchanged native parent/child calls,
fault unwind and independent top-level personality transitions remain open.
The [terminal ownership check](qualification/TWO-SPACE-TERMINAL-OWNER-2026-10-06.md)
binds a pending read to its invocation, prevents a nested child from
cancelling it, and clears the subchannel before giving it to another owner.
The fresh IPL passes the waiting-for-attention case. In-flight cancellation,
late completion and retry remain open.
The [post-start cancellation check](qualification/TWO-SPACE-TERMINAL-RETRY-2026-10-06.md)
now uses a real AID, clears after READ MODIFIED was submitted, redraws and
reads a second AID. It caught and fixed repeated subchannel enable on redraw.
Physical in-flight ordering, injected late completion after owner exit,
failed start and child-fault unwind remain open P1 cases.
The [failed-start gate](qualification/TWO-SPACE-TERMINAL-START-FAILURE-2026-10-06.md)
now injects an absent subchannel at SSCH after a third real AID, checks RC 12,
and verifies that a fresh read can start and cancel. Invocation-owned pending
I/O failure and late completion after owner exit remain separate gates.
The [owned pending-I/O gate](qualification/TWO-SPACE-OWNED-IO-2026-10-06.md)
now enters a real AID under a parent token, begins READ MODIFIED, suspends it
across a nested child, and checks parent-end clear plus a fresh pending read.
The [retired-completion gate](qualification/TWO-SPACE-LATE-COMPLETION-2026-10-06.md)
injects that retired ledger handle under a different terminal owner and
checks that the new read remains in phase one. Physical late channel status
and fault cleanup with pending I/O remain open P1 gates.
The [full caller-PSW gate](qualification/TWO-SPACE-CALLER-PSW-2026-10-06.md)
now retains both old-PSW halves in the invocation frame and checks the saved
instruction address against the actual SVC continuation in a fresh IPL. This
is required state for the later controlled-call fixture; SVC 235/236 still
only bracket direct U diagnostic entry.
The [K-controlled call gate](qualification/TWO-SPACE-CONTROLLED-CALL-2026-10-06.md)
now enters registered synthetic U children from a saved K frame, restores
the caller on normal return and a protected-page child fault, separates OS
status from application RC, and reaps a child-owned U page. Native same-
personality calls, nested controlled children and fault cleanup with pending
I/O remain open; this does not close P1 or P3.

Slices 1 and 2 now have a checked bootstrap implementation for the named
single-CPU, 16 MiB fixture. The table builder, bounded image format and
real-frame ledger run in the guest. The map/unmap API and `PTLB` callback are
exercised during guest startup before DAT is enabled; both table pools have
K-only C31 virtual aliases exercised after DAT is enabled. Slices 3 and 4
now pass a fresh IPL of that fixture, including nested SVC, U fault recovery,
full-width U page walks, bounded transfers, CMS/TSO probe selectors and a
synthetic machine-check fail-stop control. Real asynchronous external/I/O
handling, machine-check hardware injection and general frame reclamation by
the running K nucleus remain open. That checkpoint used 16 MiB real storage.
The final-core placement is still the fixture's linked real-zero image;
actual CMS/TSO application compatibility remains unqualified.

The [storage and overlay checkpoint](qualification/TWO-SPACE-STORAGE-OVERLAY-2026-10-05.md)
adds a K C31 conditional SVC 120 subset, live DAT map/free with PTLB and no
above-to-below fallback. The fresh diagnostic 3390 IPL passes its storage
and restored-table checks. The host control exercises two nested fixed-origin
backing swaps and caller restoration. At that checkpoint, slice 5 still
needed channel/dataset, terminal and command services plus the 24-bit
channel-buffer and real CMS24/TSO24 budget audits. Slice 6 still needs actual CMS/TSO format loading,
relocation and same-personality application calls. Slice 7 still needs an explicitly selected
normal replacement image and unchanged CMS/TSO guest qualification. The
release route remains the one-ASCE kernel.

The [native TSO31 checkpoint](qualification/TWO-SPACE-TSO31-NATIVE-2026-10-06.md)
now runs unchanged RXVM `-v` in the diagnostic shared U ASCE beside the
qualified CMS24/CMS31 calls. Its selected SVC 120/93 storage and output
surface uses K-owned real backing and bounded U copies, with no lasting
24-bit U gap consumption. Slice 5 still needs general command, TSO file and
terminal input services plus native TSO24 budgeting. Slice 6 still needs
native CMS-to-CMS and TSO-to-TSO application calls and conflicting-module qualification.
Slice 7 still needs TSO64/native TSO24 and the explicitly selected normal
successor image with full regression and failure controls.
The [TSO64 ANY loader-core check](qualification/TWO-SPACE-TSO64-ANY-LOADER-2026-10-06.md)
also passes a two-base host comparison and Classic C31 target link.
The [TSO64 ANY fresh-IPL map](qualification/TWO-SPACE-TSO64-ANY-MAP-2026-10-06.md)
now closes the selected transport and placement checkpoint. Native 64-bit
entry and service behavior remain open.
The [K service-capacity checkpoint](qualification/TWO-SPACE-K-SERVICE-CAPACITY-2026-10-06.md)
reserves 32 K-only C31 pages with guest-built DAT and moves the K trampoline
above them. It leaves the U 24-bit gap and U31 heap minimum unchanged.
The [native TSO64 ANY checkpoint](qualification/TWO-SPACE-TSO64-NATIVE-2026-10-06.md)
now runs unchanged RXVM `-v` through the selected IARV64 stacking PC-cp and
TPUT paths. K validates the U parameter list, owns its 128 MiB high-U heap,
and observes its DETACH before a separate wide-heap workload. This closes
the narrow version-call gate, not the broader TSO command/file service,
RMODE HIGH, abnormal-exit cleanup or native TSO24 gates.
The [TSO24 host materializer checkpoint](qualification/TWO-SPACE-TSO24-LOADER-2026-10-06.md)
matches the existing native loader at two below-line bases and proves
that full low U placement fails without falling into 31-bit space. It does
not yet run TSO24 after IPL or measure actual application headroom.
The [fresh-IPL TSO24 map](qualification/TWO-SPACE-TSO24-MAP-2026-10-06.md)
now checks and installs the image beside CMS24 when room exists and returns
RC 4 for an impossible 16 MiB low-only request while high-U work succeeds.
At that mapping checkpoint, native entry and its actual stack/heap budget
remained unqualified; static image coexistence alone did not qualify them.
The later [released beta 3 TSO24 native checkpoint](qualification/TWO-SPACE-TSO24-BETA3-NATIVE-2026-10-06.md)
runs the exact public TSO24 image in AMODE24, displays its EBCDIC version
line, and releases its 1 MiB stack, 256-byte output buffer, 4 MiB heap and
parameter page. The full low-only demand returns RC 4 with no above-line
fallback. This closes the selected native version-call and measured runtime
budget gate. General TSO file/input and command services, native TSO-to-TSO calls,
and the normal successor image remain open.
The same [beta 3 qualification](qualification/TWO-SPACE-TSO24-BETA3-NATIVE-2026-10-06.md)
subsequently adds a guest loader-collision control: a live low allocation
covering the fixed origin causes RC 4 with no image published; releasing it
allows the unchanged member to load and run. The optional TSO24-absent IPL
still passes. This directly exercises the rule that appropriate storage
shortage is an error, not a reason to evict or move a 24-bit application.

The [6 October K channel checkpoint](qualification/TWO-SPACE-CHANNEL-2026-10-06.md)
adds a reserved 64 KiB low-real K workspace and a bounded post-handover
`VOL1` read through real-addressed CCWs. This removes the virtual-equals-real
assumption for that one channel operation, but the remaining slice 5 service
and application-budget criteria above stay open.
The [channel completion checkpoint](qualification/TWO-SPACE-IO-COMPLETION-2026-10-06.md)
uses `TSCH` status for success and an architectural-clock watchdog only for
stalls, removing the fixed poll-count race from the diagnostic channel path.

The [bounded K dataset checkpoint](qualification/TWO-SPACE-DATASET-2026-10-06.md)
reads `KCORE.BIN`'s first F/18452 block through a checked VTOC/first-extent
lookup. The [K terminal checkpoint](qualification/TWO-SPACE-TERMINAL-2026-10-06.md)
adds a real-addressed 3270 write visible in an independently connected
terminal after fresh IPL. The [terminal input checkpoint](qualification/TWO-SPACE-INPUT-2026-10-06.md)
adds attention-gated `READ MODIFIED` and a bounded K-to-U AID record copy.
The [disk/console isolation checkpoint](qualification/TWO-SPACE-DUAL-IO-2026-10-06.md)
gives each device a reserved low-real workspace and passes a disk read
between terminal-read start and completion in the fresh IPL. It removes
cross-device buffer reuse; per-device scheduling and cancellation remain open.
The [subchannel failure checkpoint](qualification/TWO-SPACE-TSCH-FAILURE-2026-10-06.md)
repairs condition-code clobbering in the channel assembler and proves that
an absent subchannel fails immediately while positive CKD/3270 IPL still
passes. Hot-unplug recovery and asynchronous I/O acknowledgement remain open.
General CMS/TSO file I/O and command services remain open. The diagnostic C31 service uses 16 sparse
K-only pages and no additional low U virtual page. This does not complete
slice 5 or select the replacement image.

The [CMS24 header checkpoint](qualification/TWO-SPACE-CMS-HEADER-2026-10-06.md)
adds a pinned unchanged RXVM stage to a disposable image and validates its
first block through K C31 after IPL. The later [full-stage guest checkpoint](qualification/TWO-SPACE-CMS-FULL-GUEST-2026-10-06.md)
reads and validates all 92 records of the v2 CMS24 stage under K and uses
observable guest completion rather than a fixed IPL pause.
The [image materialization checkpoint](qualification/TWO-SPACE-CMS-IMAGE-2026-10-06.md)
adds a C89 fixed CMS24 copy and CMS31 relocation pass over validated staged
MODULE bytes. The [dual CMS mapping checkpoint](qualification/TWO-SPACE-DUAL-CMS-MAP-2026-10-06.md)
now connects both pinned RXVM images to live U allocations in one fresh IPL,
with dynamic K stage reservations and a measured 13,914,112-byte contiguous
24-bit placement gap between the CMS24 image and reserved stack range.
The [guarded CMS24 stack checkpoint](qualification/TWO-SPACE-CMS24-STACK-2026-10-06.md)
backs that stack range with 255 U pages while preserving the same gap; a
tiny AMODE24 child writes its last page. Actual unchanged CMS24/TSO24
heap and stack demand remains to be measured.
The [same-origin guest backing checkpoint](qualification/TWO-SPACE-NESTED-BACKING-2026-10-06.md)
then swaps an eight-byte AMODE31 child over the mapped RXVM interval,
executes its nested SVC from U, restores the parent pages and propagates its
return value to a U64 caller. It proves the mixed-mode memory and executable
transition needed for a colliding nested call. Neither RXVM executes yet;
full CMS linkage, general file/command services, native CMS-to-CMS calls, TSO programs,
unchanged application heap requirements and normal successor selection
remain unqualified.
The [64 MiB real-memory checkpoint](qualification/TWO-SPACE-64M-2026-10-06.md)
widens the single-CPU diagnostic K aperture while keeping both channel
buffers low-real and all K tables out of U. Its fresh IPL passed; at that
checkpoint large 31/64-bit allocations were still unmeasured.
The [CMS24 child checkpoint](qualification/TWO-SPACE-CMS24-CHILD-2026-10-06.md)
proves the same reversible collision at the fixed 24-bit RXVM origin,
including a real AMODE24 SVC and return to a U64 caller. It remains a
minimal instruction test, not an unchanged CMS24 application run.
The [U DAT capacity checkpoint](qualification/TWO-SPACE-U-DAT-CAPACITY-2026-10-06.md)
widens the U table pool to 384 KiB and retains a simultaneous 16 MiB U31
plus 32 MiB U64 mapping control. The later
[guest wide-heap checkpoint](qualification/TWO-SPACE-WIDE-HEAPS-2026-10-06.md)
allocates, accesses and releases both heaps while the CMS24/CMS31 RXVM
images are mapped. The guest uses 319,488 U DAT bytes and leaves both heap
endpoint pages unmapped after free. Internal SVC 223 is only a provisional
high-U fixture entry; actual application storage and CMS/TSO service ABI
qualification remain open.
The maintained SDK defaults are 64 MiB for CMS31/TSO31 and 128 MiB for
TSO64, with a further 3 MiB CMS31 stack. The earlier 64 MiB real fixture
could not qualify those unchanged workloads. The
[256 MiB memory checkpoint](qualification/TWO-SPACE-256M-HEAPS-2026-10-06.md)
now supplies enough K/U DAT capacity and real-frame headroom for simultaneous
64/128 MiB *diagnostic* heaps while keeping 24-bit U placement unchanged.
The SDK defaults are recorded by the owning Mainframe ELF SDK in
`docs/compiler/SDK.md`. The CMS31 C stack, unchanged application runs and
their service ABIs still need qualification.
The [first native CMS31 checkpoint](qualification/TWO-SPACE-CMS31-NATIVE-2026-10-06.md)
enters unchanged RXVM `-v` from U64, exercises its 3 MiB image-resident C
stack and 64 MiB U31 heap, and returns RC 0 through K's checked CMSCALL
subset. Its one output line is captured in K diagnostic storage. This
establishes one actual CMS31 execution path, while CMS file/input/command
services, live 3270 application output, CMS24 execution, TSO application
execution and native same-personality application calls remain open.
The [first native CMS24 checkpoint](qualification/TWO-SPACE-CMS24-NATIVE-2026-10-06.md)
enters the unchanged fixed-origin RXVM `-v` through a temporary low-U
bridge. Its distinct SVC 202 handler copies the flagged 24-bit `TYPLIN`
buffer through K and advances past the historical four-byte inline error
continuation. CMS24 and CMS31 each execute and return RC 0 in one fresh IPL.
The bridge's one allocated 24-bit U page is released, so the permanent
13,914,112-byte low placement gap does not shrink. General CMS file/input
services and native same-personality application calls remain open.
The [live CMS screen checkpoint](qualification/TWO-SPACE-CMS-LIVE-SCREEN-2026-10-06.md)
routes the selected CMS24 and CMS31 application line writes through K's
checked 3270 channel workspace. The final version line was observed on an
independently connected terminal after guest completion. General multi-line
console, input and file services remain open.
The [CMS31 lowcore checkpoint](qualification/TWO-SPACE-CMS31-LOWCORE-2026-10-06.md)
maps one U-owned, key-8 compatibility page at virtual zero with the SDK's
`0x14` SYSREF pointer and SVC 205 veneer. Guest U reads prove that this is
separate from K's real prefix. It costs no page in the 13,914,112-byte
post-image low placement gap. Actual SVC 205 lookup and cross-personality
lowcore behavior remain open.
The [CMS31 FST checkpoint](qualification/TWO-SPACE-CMS-FST-2026-10-06.md)
adds a bounded VTOC lookup through that veneer. Separate fresh IPLs check
the absent result and the pinned IOQUAL RXBIN FST with exact R0/R1/R15
handoff. The [later IOQUAL checkpoint](qualification/TWO-SPACE-CMS31-IOQUAL-2026-10-06.md)
runs a fresh second CMS31 RXVM relocation on pinned IOQUAL and LIBRARY.
Selected checked file reads and transient K-owned writes yield
`PASS=8 FAIL=0 SKIP=3`, RC 0, in a fresh IPL; a separate run proves
that absent LIBRARY prevents second-app dispatch. Persistent output,
CMS24 file services, full TSO binaries in the successor, native
same-personality application calls and the normal replacement image remain open.
The [CMS file validation checkpoint](qualification/TWO-SPACE-CMS-FILE-VALIDATION-2026-10-06.md)
extracts the staged envelope checks into host-tested C89 source and reruns
the full CMS31 IOQUAL fresh IPL with the same 119 guest checks.
The [CMS transfer preflight checkpoint](qualification/TWO-SPACE-CMS-TRANSFER-2026-10-06.md)
preflights both U destinations of `RDBUF` before either copy, keeps its cursor
unchanged on failure and repeats the full positive guest IPL. A guest-level
malformed CMS record request remains an acceptance gap.
The [CMS24 IO24 checkpoint](qualification/TWO-SPACE-CMS24-IO24-2026-10-06.md)
runs a pristine fixed-origin RXVM overlay on unchanged IO24 RXBIN in the
same U ASCE as both CMS31 images. Its 6/0/1 guest result, two exact transient
output hashes, restored parent, and 13,914,112-byte post-run 24-bit gap pass
122 fresh-IPL checks. A DAT software-walker correction makes absent segment
entries return `TSD_MISSING`. Persistent output, TSO applications and native
same-personality calls remain open.

The [CMS input cursor checkpoint](qualification/TWO-SPACE-CMS-CURSORS-2026-10-06.md)
keeps eight K-owned file slots keyed by profile and CMS file ID. A fresh IPL
holds two actual CMS31 files open at once, verifies distinct cursors and real
owners, and reruns unchanged CMS24/CMS31 workloads. It prepares nested file
use but does not qualify a native application-to-application call or TSO
execution in the successor.

The [native TSO loader-core checkpoint](qualification/TWO-SPACE-TSO-LOADER-2026-10-06.md)
materializes the pinned unchanged TSO31 load-module bytes at two bases with
the same image hashes as the released loader, under host sanitizers, and
builds the code with Classic C31. It validates the pinned TSO64 ANY directory
but does not materialize or run that image. K staging, U mapping, TSO service
dispatch and fresh guest execution remain open.

The [TSO31 shared-U map checkpoint](qualification/TWO-SPACE-TSO31-MAP-2026-10-06.md)
now reads the pinned native stream through K's checked CKD path, validates
its staged records, materializes it privately and maps all 268 pages at
U `0x07000000` beside both CMS31 images and CMS24. A fresh IPL passes 124
checks. TSO31 execution and its MVS-style services remain open, as do TSO64
materialization, native TSO24 and the normal replacement image. The C31
fixture service uses 60,926 of its reserved 65,536 bytes; any expansion must
be K-only and explicitly budgeted rather than consuming low U placement.

## PD-004: Batch-file delivery

- Type: improvement
- Status: Done in 0.1.1
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: The conformance recipe now encodes bounded UTF-8 script commands into IBM1047 raw U/18452 records with hex-15 delimiters. It rejects unsupported manifest geometry and overlong lines, then checks exact batch readback.
- Evidence: [4 October host and guest result](qualification/STEP1-2026-10-04.md); [operator guide](user/CONFORMANCE.md).
- Acceptance: Document framing, diagnose unsupported input and provide a repeatable text-import route.

## PD-005: Command-length diagnostics

- Type: defect
- Status: Done in 0.1.1
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: A trailing `&` joins 3270 field entries at `MORE>` up to 198 characters. Console and batch records beyond that limit are rejected without dispatch.
- Evidence: [PCOMM boundary control](../tests/pcomm-operator.crexx) checks both routes at 198/199 characters; the [guest result](qualification/STEP1-2026-10-04.md) includes a human entry beyond 80 columns.
- Acceptance: Let a human submit a command longer than the 80-column console field through a documented continuation or short-script route, up to the tested command limit. Diagnose an overlong console or batch command before executing any truncated fragment; test both routes at and beyond their limits.

## PD-006: Console input and prompts

- Type: defect
- Status: Done in 0.1.1
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: The 3270 writer now keeps a row and column across fragmented writes, wraps output above the editable field and presents a distinct `MORE>` continuation prompt.
- Evidence: [ASAN/UBSAN console control](../tests/io-repair.c) and the [guest operator result](qualification/STEP1-2026-10-04.md).
- Acceptance: Echo a typed line together, retain one stable prompt and its editable field through normal and long output, and show a distinct continuation prompt when a command spans console entries.

## PD-007: Results and scrollback

- Type: improvement
- Status: Done in 0.1.1
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: External commands now print numbered BEGIN/END markers with RC. The runner requires a fresh prompt, captures a complete ScreenTrace and judges ordered results against the locked manifest; a missing expected output fails.
- Evidence: [4 October fresh and deliberately corrupted trace results](qualification/STEP1-2026-10-04.md).
- Acceptance: Identify each submitted command and its completion code unambiguously through long output, including the command following a failed one. Keep the latest result readable at a fresh prompt and retain a complete trace for automated checks.

## PD-008: Operator diagnostics

- Type: improvement
- Status: Done in 0.1.1
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: The kernel hides expected missing PCOMM `.BAT` allocation probes, while a present invalid dataset and other allocation failures remain visible. `HELP TSO`, `HELP CMS` and `VERSION` explain the command boundary and build identity. The direct loader already announces unsupported addressing/residence.
- Evidence: [4 October guest result](qualification/STEP1-2026-10-04.md), [operator guide](user/README.md) and prior [0.1 qualification](qualification/QUALIFICATION.md).
- Acceptance: Distinguish expected probes and announce unsupported addressing/residence before dispatch.

## PD-009: Temporary package lifecycle

- Type: improvement
- Status: Done for the bounded conformance flow in 0.1.1
- Target: pdos-zarch C32 kernel and native application contexts
- Observation: Preparation creates a fresh disposable candidate from an untouched base, checks it offline, and documents selection, stop, readback and removal. Three local guest candidates were selected only while stopped and removed after returning to the accepted image.
- Evidence: [operator guide](user/CONFORMANCE.md) and [4 October handback](qualification/STEP1-2026-10-04.md).
- Acceptance: Preserve the base OS, leave one working image and remove duplicate temporary installations.

## PD-010: Hosted fresh-image delivery

- Type: qualification
- Status: Done
- Target: Fresh base-OS CCKD delivery built on the Linux GitHub runner
- Observation: The distro converter expands the 100-cylinder disk to the full 1,113-cylinder device by default. Conversion now specifies `-cyls 100` explicitly. The transport checker also retains identical legacy zero serial bytes, while still rejecting non-digit serial changes and all guest-byte changes. The 0.1.1 Linux release job completed source-to-image, loader/dataset/compression checks and image packaging. Packaged host checks do not establish fresh guest execution.
- Evidence: [0.1.1 release run 37308481475](https://github.com/adesutherland/z-pdos/actions/runs/37308481475), source `69b638d38dc4221cbed399d8fde5bf4883673901`, and the published `z-pdos-0.1.1-pdos-image.zip`; [release guide](../../doc/BUILD-AND-RELEASE.md).
- Acceptance: Linux builds the fresh disk from source, passes loader/dataset/compression controls, records its actual utility identities and delivers an archive whose extracted files match their inventory. Keep any new guest qualification explicitly separate.

## PD-011: Region-first table padding

- Type: defect found by source review
- Status: Open; runtime impact not reproduced
- Target: `pdos-zarch` DAT setup
- Observation: In `pdosInitAspaces`, the loop following initialization of `region1[0]` writes invalid entries to `region2[1..511]` again, leaving the corresponding `region1` entries uninitialized. The enclosing `PDOS` allocation uses `malloc`, and `pdosDefaults` does not clear it. The documented low/high windows use the first region-first entry, so this finding does not establish failure of the existing qualified workloads.
- Evidence: [Current source](../src/pdos.c), `pdosInitAspaces`, `main` and `pdosDefaults`; documentation review on 4 October 2026. No code change or execution was performed for this finding.
- Acceptance: Initialize unused entries in the intended table, check table contents independently, and qualify invalid translations outside the mapped region without changing accepted mappings.

## PD-012: Direct executable reads need an extent boundary

- Type: robustness issue found by source review
- Status: Done in 0.1.1
- Target: Disk path in `pdosLoadExe`
- Observation: The direct loader now validates a single selected extent, bounds track reads and requires an EOF before accepting the image. A malformed RXC64 executable without EOF was rejected; a following PCOMM command ran normally.
- Evidence: [Current source](../src/pdos.c), `pdosLoadExe` and [4 October malformed-disk guest result](qualification/STEP1-2026-10-04.md).
- Acceptance: Validate the selected extent, stop reads at its boundary and require the appropriate termination. Preserve valid native loads and reject a missing-EOF image without reading an adjacent dataset.

## PD-013: Repeatable program conformance run

- Type: improvement
- Status: Done in 0.1.1
- Target: z/PDOS operator commands, package staging, PCOMM batch execution and result capture
- Observation: The product-level manifest and cREXX recipe now stage checked native programs, simple VB members and raw batch on a fresh disk, read them back, run a leased 3270 script with live replies, and emit per-command and overall PASS/FAIL. A check may judge a silent command by its RC and may reuse an output phrase in another check because the judge searches each numbered command interval separately. The 0.1.1 base image has no application package or in-guest runner; the host recipe is in the source archive.
- Evidence: [4 October qualification](qualification/STEP1-2026-10-04.md), [operator guide](user/CONFORMANCE.md), and the [product recipe](../scripts/conformance.crexx).
- Acceptance: From an untouched base image, a checked native program package and a declarative test script, provide one documented product-level flow that creates a disposable candidate, loads the program without changing its native bytes, runs ordered commands and reports each command's identity, return code, expected result and overall PASS/FAIL. Verify fresh output rather than accepting stale data; retain input/output hashes and guest evidence in a portable receipt. Demonstrate a positive and an intentionally failing cREXX case, an interactive-input case, a command longer than 80 columns, and cleanup that preserves the base image. Do not require a private Lab path or manual interpretation of a screen snapshot.

## PD-014: CMS and TSO fixture exchange

- Type: improvement
- Status: Done
- Target: z/PDOS host/disk fixture tooling and the selected cREXX record-file contract
- Observation: The product recipe inspects a release ZIP and unchanged tape images to pin SHA-256 and logical-record hashes, strictly rechecks them, decodes bounded CMS VMFPLC2 HET and standard-label TSO AWS records, builds a fresh exchange CKD, runs checked guest `ALLOC`/`RCOPY` commands, and exports exact logical records after shutdown. It keeps CMS file name/type/mode and TSO dataset/label rules separate. The route does not execute CMS MODULEs.
- Evidence: [Phase 2 guest and stopped-disk results](qualification/PHASE2-2026-10-04.md), [fixture guide](user/FIXTURES.md), and `pdos/scripts/fixtures.crexx` with its bounded binary helper. The unchanged beta-3 CMS31 HET `IOBAD CREXX` fixture and a synthetic standard-label AWS both passed exact readback.
- Acceptance: Define a portable fixture manifest with explicit logical name, destination/source, record format and length, text encoding or binary mode, and expected hashes. Import and export the cREXX text, blank/empty, and ordered 00–FF binary cases through the supported CMS and TSO fixture routes and z/PDOS; compare logical records and binary bytes after execution. Create output datasets through PD-020 rather than requiring the operator to prebuild every output slot. Reject unsupported framing, record length, missing datasets and ambiguous conversions without partially accepting a candidate. Check unaffected disk content and keep image changes offline. State any CMS or TSO record forms outside the supported subset.

## PD-018: Multiple DASD volumes

- Type: improvement
- Status: Done
- Target: z/PDOS CKD discovery, mount registry, DD bindings and direct loader
- Observation: Phase 1 bound native datasets and executable loads to the IPL subchannel even though `DIR` already used a separate current-device field. The current phase 2 source provides explicit address discovery, VOL1-checked mount, volume selection and protected unmount, then routes native DD, PDS and executable I/O through the selected or bound device. `ALLOC` now creates bounded sequential outputs on a selected exchange image.
- Evidence: [phase 2 guest and stopped-disk checks](qualification/PHASE2-2026-10-04.md) and [operator guide](user/MEDIA.md).
- Acceptance: On a stopped, disposable two-3390 image, mount a labeled exchange volume without changing the IPL disk; reject absent, wrong-label and duplicate addresses. Select it, list and read its datasets, run a native program or batch from it, write and read back output there, return to IPL, and refuse unsafe unmount. Verify unchanged IPL bytes and complete cleanup. Show accurate prompt and return codes.

## PD-020: Safe guest dataset allocation

- Type: improvement
- Status: Done
- Target: selected exchange CKD volume, VTOC and PCOMM allocation command
- Observation: `ALLOC` now checks the selected non-IPL 100-cylinder exchange volume's VOL1, VTOC layout and occupied extents, initializes EOF before publishing a new sequential DSCB, reads that DSCB back and reports the chosen extent. FB80 and VB outputs then passed guest writes and exact stopped-disk readback. Duplicate, malformed geometry and IPL requests returned RC 8. The inherited `pdosNewF` remains separate.
- Evidence: [phase 2 guest record](qualification/PHASE2-2026-10-04.md), `pdos/src/pdos.c` (`pdosAllocateDataset`), and the [media guide](user/MEDIA.md).
- Acceptance: Offer `ALLOC` with explicit dataset name, PS record format, logical/block length and bounded cylinder request on a selected non-IPL CKD volume. Discover and reserve a real free extent, update the VTOC consistently, initialize an empty dataset, and report volume, extent and return code. Reject duplicate names, unsupported geometry, full or malformed VTOCs, insufficient space, IPL requests and open conflicting DDs before publication. On an injected write failure, leave no discoverable partial dataset. Guest-qualify allocation followed by native program output and stopped-disk byte readback, and show the command in `HELP`. Keep a simple path for the operator while retaining exact geometry in the qualification receipt.

## PD-019: Virtual tape I/O

- Type: improvement
- Status: Done
- Target: Hercules virtual 3420 attachments, z/PDOS raw record service and native `TAP:` DD
- Observation: Inherited `RDTAPE`/`WRTAPE` and old pseudo-dataset names exist, but phase 1 had no mounted tape, no native DD admission and no qualified record/error semantics. The current phase 2 source registers one attached tape read-only by default, reports physical record length/hash/prefix, reads across file marks, writes to a separate explicitly writable image, and admits a matching `TAP:` DD. The inherited `TAV:` path is outside this delivery.
- Evidence: [phase 2 guest and stopped-tape checks](qualification/PHASE2-2026-10-04.md) and [operator guide](user/MEDIA.md). Short native `TAP:` read and write transfers passed; larger application records remain unqualified.
- Acceptance: Mount unchanged checked HET and AWS images on a leased guest, read records across file marks with explicit rewind and error/EOF behavior, compare guest observations with host records, and reject wrong/unregistered devices. On a separate writable output image, write records and file marks, stop the guest, export and compare exact records and hashes. Do not claim VMFPLC2 or standard-label interpretation from raw tape transport alone.

## PD-015: CMS cREXX binary contract

- Type: qualification
- Status: Done
- Target: unchanged CMS24 and CMS31 cREXX MODULE packages and the z/PDOS loader/service boundary
- Observation: The 0.1.1 source pins the unchanged beta 3 CMS24/CMS31 MODULE bytes, record and relocation contracts, and distinct CMS service surfaces. Checked exchange-disk staging and `CMS CHECK` reject malformed format and address inputs before dispatch. The local guest proof names its own installed image; the downloadable base image was built separately.
- Evidence: [Stage 3 local guest qualification](qualification/STAGE3-2026-10-05.md), `pdos/scripts/cms.crexx`, `pdos/scripts/cms-module.py`, and Mainframe Lab's tagged beta 3 CMS qualification.
- Acceptance: Pin the exact unchanged CMS cREXX package bytes and inventory their loader records, address/mode, entry/return, SVC/CMSCALL/DIAGNOSE, file naming/record, console and storage requirements against current z/PDOS behavior. Run a bounded loader/service probe with explicit rejection of unsupported operations. Record the selected adapter or environment design, its owner and the specific prerequisites for PD-016 and PD-017; do not treat a repackaged or recompiled TSO executable as a CMS pass.

## PD-016: CMS31 cREXX execution

- Type: improvement
- Status: Done
- Target: `cms20-esa31-v1` cREXX MODULEs on the selected z/PDOS machine
- Observation: The unchanged CMS31 RXVM/RXAS/RXC MODULEs run through the checked 0.1.1 guest path. Supplied and fresh IOQUAL, live input, library/imports, bounded heap, failure cases and exact stopped-disk outputs passed locally. The downloadable base image contains the adapter, but was not the exact image booted for this result.
- Evidence: [Stage 3 local guest qualification](qualification/STAGE3-2026-10-05.md), the pinned PD-015 contract and complete private 3270 traces.
- Acceptance: On a named z/PDOS build and machine, load the unchanged CMS31 RXVM MODULE and run supplied and fresh cREXX bytecode with the selected console and file fixtures, exact outputs and return codes. Then qualify unchanged CMS31 RXAS and RXC through a fresh RXC → RXAS → RXVM chain with the required library/import cases, bounded heap and failure controls. Preserve the separate CMS 20 result and identify unsupported CMS services explicitly.

## PD-017: CMS24 cREXX execution

- Type: improvement
- Status: Done
- Target: `vm370-4381-v1` cREXX MODULEs on a separately qualified low-address z/PDOS path
- Observation: The unchanged fixed-origin CMS24 RXVM MODULE ran the library-free IO24 subset at `0x20000`, with live input and exact stopped-disk output. Missing-file and invalid-mode controls returned expected errors; an altered-origin MODULE was rejected with RC 8. Native TSO24 loading remains open under PD-001. The downloadable base image contains this adapter, but the local CMS24 proof names its own installed image.
- Evidence: [Stage 3 local guest qualification](qualification/STAGE3-2026-10-05.md), the pinned PD-015 contract, and [PD-001](#pd-001-amode24rmode24-application-loading).
- Acceptance: After PD-015, load the unchanged historical CMS24 RXVM MODULE at its required origin and run its qualified library-free IO24 subset with actual input, exact binary/text readback, expected return codes and negative service/address controls. Keep its constrained heap separate from the wider profiles. The distinct native TSO24 path remains PD-001; a full historical CMS24 compiler chain requires separate evidence and is not implied by this subset.

## PD-021: Reusable 3270 application presentation

- Type: improvement
- Status: Done
- Target: successor U command processor and opt-in 3270 applications, with a checked K terminal gate
- Observation at the initial PD-021 checkpoint: PCOMM was an application, while the successor then had only bounded K 3270 reads/writes and selected CMS/TSO line output. The release `22*80` output buffer and the successor's `TSC_MAX_CONSOLE=2048` and `TSC_MAX_INPUT=256` limits are historic source choices. The [architecture decision](architecture/TWO-SPACE-POC.md#architecture-decisions-for-the-first-replacement) places a C31 C device driver and encoder in K and a reusable C presentation library in U. No successor U library, geometry negotiation or general full-screen application contract has been qualified.
- Evidence: [P4 C console and presentation qualification](qualification/TWO-SPACE-P4-2026-10-07.md) and [P6 normal-image qualification](qualification/TWO-SPACE-P6-2026-10-07.md) close the selected models, U presentation, PCOMM and opt-in application contract. The observation above describes the earlier checkpoint.
- Acceptance: Define a versioned terminal-capability result and bounded logical screen/input requests with one foreground owner, deterministic busy/error results and no unchecked U pointer or U-supplied raw data stream in K. Support checked model 2 24×80, model 3 32×80, model 4 43×80 and model 5 27×132 geometries, including the configured or queried active screen mode, field limits, buffer addressing and sufficiently sized low-real terminal workspaces. Reject unsupported geometry or ambiguous capability data explicitly; keep disk and terminal I/O workspaces separate and completion event-driven. Implement the header, footer, scrollable output and editable entry wrapper in C in U, then use it in PCOMM and one opt-in application. Let the wrapper publish semantic text events to the attached monitor; do not imply that arbitrary raw 3270 streams are readable there. Place the wrapper, PCOMM and their compatible runtime storage above 16 MiB so their own state does not shrink the 24-bit application range. Preserve native line terminal calls for unchanged CMS and TSO programs. Check long output, AID/input handling, encoding, nested-child screen return and redraw, malformed requests, and fresh IPL runs on at least the four named display models without terminal races. Keep K's boot and fault output usable when U cannot run. Qualify non-3270 consoles under [PD-022](#pd-022-attached-operator-line-view-and-transcript), and treat larger extended displays as separate capability qualification.

## PD-022: Attached operator line view and transcript

- Type: improvement
- Status: Done
- Target: successor K terminal service and line backend, U PCOMM command events, and an attached host capture client
- Observation at the initial PD-022 checkpoint: The release source recognizes 3215 and 3270 console types and PCOMM prints numbered `BEGIN`/`END` markers. Existing exact-output checks depend on a 3270 ScreenTrace. The two-space successor has selected 3270 and native line operations, but no qualified secondary Telnet 3215 device, dual-view fan-out, exclusive input handoff or complete append-only transcript. The [operator line-view decision](architecture/TWO-SPACE-POC.md#operator-line-view-and-attached-transcript) then specified their boundary. Hercules 3215-C writes to its integrated console and is a separate optional backend, not the clean attached stream.
- Evidence: [P4 console qualification](qualification/TWO-SPACE-P4-2026-10-07.md) and [P6 normal-image qualification](qualification/TWO-SPACE-P6-2026-10-07.md) close the attached monitor, line-only primary, exclusive prompt handoff, durable transcript and loss/reconnect controls.
- Acceptance: Configure one 3270 primary display with a separately attachable Telnet 3215 line monitor, and qualify a line-only primary configuration. Keep unchanged selected CMS/TSO line-call ABIs and RCs; render their text on the scrollable 3270 view and deliver the same ordered logical records to the monitor. Define and version a readable, sequenced transcript grammar for readiness, command ID, input request, output, separate OS status and application RC, screen-only activity and gap/error events. Escape text that resembles framing, preserve defined guest text bytes up to a documented host EBCDIC-to-UTF-8 conversion, and retain output beyond the finite 3270 scroll region. A missing, disconnected or full monitor must not hang normal 3270 work or silently claim a complete transcript; capture qualification fails on a gap or sequence discontinuity. Permit monitor input only through an explicit, exclusive lease at a command or line-input prompt; reject stale or duplicate input and never route monitor input into 3270 AID/field requests. Keep device buffers and queues in K storage, protect U pointers, and drive readiness and I/O completion from guest events rather than sleeps. With named unchanged CMS24/31 and TSO24/31/64 line programs, check output and RC equivalence on 3270 and line-only primary, simultaneous 3270 plus monitor attached before the first command, long output, repeated commands, input prompts, encoding, disconnect/reconnect, failure injection and no loss of 24-bit U capacity. A full-screen request on a line-only primary reports unsupported; a raw full-screen program has no promised text rendering unless it emits semantic events through the wrapper.

## PD-023: Recycle empty DAT tables

- Type: defect
- Status: Done
- Target: live shared-U mapping and release of native application storage
- Observation: The 0.2.0 operator's mixed HIGH/call/model-5/CMS31 sequence exhausted the U DAT pool before a subsequent valid CMS24 CHECK. The retained image had `used == capacity == 0x160000`, no live allocation overlapping the fixed CMS24 image and ample free real storage. A cold CMS24 run passed on the same stopped disk. `TSDUNMAP` removed page mappings but never recycled their empty tables.
- Repair: Unlink empty tables bottom-up, purge live single-CPU translation before reuse, recycle their pool frames and trim the high-water tail. Reserve a complete new path before changing any table. Keep the pool capacity, ASCE root and bootstrap/state ABI unchanged.
- Evidence: [0.2.0 release record](qualification/RELEASE-0.2.0.md); focused C89 ASan/UBSan DAT churn, neighbour preservation, attachment, purge ordering and atomic-full controls pass, as do memory/gate/boot/capacity checks and Classic C31 compilation. The reviewed/frozen repaired image then passed ten operator acceptance gates in fourteen fresh phases and 143 ordered cases, including the exact failed order and full HIGH/CMS24 sequence. All output/capture, DAT recycling and host/lease cleanup gates passed before managed adoption.
- Acceptance: The reviewed/frozen repaired image passes the exact previously failing mixed workload, normal application/memory/console paths affected by map/free, durable output and clean shutdown. Record bounded table use and exact image/input identities before release or managed promotion.

## PD-024: Complete 3270 family driver and shared screen service

- Type: improvement
- Status: In progress
- Target: K C31 device services, portable 3270 codecs, U presentation clients and PCOMM; retain the native CMS/TSO ABI.
- Observation: The 0.2.0 logical console erases/repaints on each line, retains only visible rows and drops cursor/AID information at its application wrappers. Its raw validator rejects most standard orders and structured fields. Configured models 2–5 do not establish negotiated geometry, hardware partition, graphics, programmed-symbol, DBCS or printer support.
- Direction: The complete 3270 family remains the roadmap, including printers and every capability-defined facility. The stage 3 delivery decision is to implement facilities that we can test on available providers and leave untestable facilities here in the backlog. PCOMM uses its required subset through the reusable API. C owns policy, composition and protocol; assembler remains the privileged linkage.
- Original authorized steps 1–2: Complete the device/session, capability/geometry, immutable event and binary-transfer API, then the usable retained console with scrollback and panels. Pause for the user's review after that second step, before advanced-family integration. The console must provide a distinct called-program output panel, useful status, contextual function-key legends, a restrained header and capability-aware colour with a monochrome fallback. The teletype/LLM interface remains first-class: ordered semantic output, explicit command/result and gap records, stable prompts and one input owner. Three exact-cell design boards support the user's layout choice; this changes presentation requirements without narrowing the full-family scope.
- Steps 1–2 review checkpoint: [Workbench qualification](qualification/CONSOLE-WORKBENCH-2026-10-09.md) records the selected Workbench, full UI sweep and repairs, 13 passing bounded guest runs, direct DX3270 model2/5 use and remaining emulator character defects. This was the requested review pause; the later stage 3 authorization below resumes testable integration. Release remains open.
- Manual review recovery gap (9 October): A preview with an output-capture-only teletype allowed PF12 to select monitor input. The saved Workbench state showed MONITOR; after the primary display was cleared, primary Clear did not restore it while the selected line reply was outstanding. A controlled preview reset restored the prompt. Input handoff needs an explicit interactive-consumer policy and a primary-terminal escape that cancels/quiesces the owned line read before returning input and repainting; test it with no monitor reply and queued primary AIDs, without timers deciding ownership. The cause of the initial display clear is unconfirmed; F10 itself only selects output focus. The tested capture-only policy and no-reply primary recovery are closed by the stage 3 result below; broader driver conformance remains open.
- Manual review refinements (9 October): The submitted field now resets its locked cursor immediately; PF10 toggles output/shell focus and the focus bar. Workbench uses PF1–PF10, leaving PF11/PF12 unassigned; raw session AIDs and explicit input-source APIs remain. Pane selection stays idempotent. Reviewed freeze27/native26/command7/app6 passed five fresh guest runs and91 checks across narrow/wide colour, monochrome, normal PCOMM editor and plain line operation. Removing PF12 does not close the unattended-monitor recovery gap. See the Workbench qualification record and paired feedback receipt; the later stage 3 authorization below resumes work after that checkpoint.
- Stage 3 authorized (9 October): Resume with testable facilities: explicit interactive-monitor policy and interruption-driven primary return, extended text reply-mode helpers and provider checks, basic local 3287 printer sessions/output, and a separate C application plotting actual mounted DASD allocation. Use only the focused checks needed for each slice; complete the whole code review and source/input freeze before one final integrated QA pass. A failing final case returns its repair to review and affected rechecking. Managed adoption, release/publication and emulator upstream changes remain separate decisions.
- Stage 3 accepted (9 October): [The qualification report](qualification/CONSOLE-STAGE3-2026-10-09.md) and paired receipt select 21 passing guest runs and 333 checks. Native11 repeats the affected early-input, retained panels, PCOMM editor/child return, normal/raw monitor recovery and raw-session Close/native-input paths after reviewed repairs. Other source-specific graph/VTOC, printer, DBCS, geometry, line and native-I/O checks are reused with an explicit dependency review. Direct DX3270 single-Clear recovery and normal shutdown are recorded separately. Full presentations now publish their input-field owner only after completion; physical output defers behind pending attention. No timer determines input ownership. The service fits its current bank with 6,674 bytes of headroom; further family growth requires a bank-capacity review. The full-family item remains In progress and this increment is a development source checkpoint, not a released or adopted image.
- Approved 0.2.1 finish (10 October): Close the facility/provider coverage audit for the shipped basic display/session/input surface, fix any supported-path defects it finds, add a user-facing CONSOLE STATUS/capability view, and install DISKMAP through the standard source/release image recipe rather than only the stage 3 operator fixture. Verify an untouched distribution image on screen and line profiles, publish usable console/monitor/emulator instructions and retain the current clean screenshots as development material until release selection. A bounded formatted 3287/pagination slice is worthwhile if the actual installed route and independent output oracle can be demonstrated; broader DBCS input/editing and advanced family facilities remain explicitly scoped by that audit. Review the current 6,674-byte K service-bank headroom before further K additions. The separate later editor and source-script integration directions are PD-026/027.
- Full available-provider closure remains Open: The accepted stage 3 increment does not establish that every facility testable with the installed infrastructure is finished. Formatted printer orders are still deliberately rejected by T27PRWRT, and the DBCS guest proof covers two IBM930 output characters/raw preservation rather than an input/editing/codepage matrix. A facility-by-facility provider and coverage audit must distinguish unfinished implementation/qualification from a demonstrated unavailable device feature. The deferred categories below are not blanket findings that x3270/pr3287/DX3270 cannot support them.
- Deferred stage 3 facilities: Complete hardware partition/window lifecycle and 16-bit partition execution, programmed-symbol loading, GOCA/image/object rendering, format-storage/text processors, printer SCS/IPDS and auxiliary routes/chains require a demonstrated provider and independent result check. They remain Open under this item; portable framing or a receiving socket alone cannot qualify them. Basic DBCS byte preservation can be tested with the installed x3270 engine; conversion and broader DBCS editing remain backlog work until a named codepage/input matrix can be verified.
- Evidence: The [driver contract](architecture/CONSOLE-STANDARD.md) names the primary references, coverage groups and implementation checkpoint. `twospace_3270.c` supplies common wire services and `console-family-check.crexx` owns focused checks. The [9 October foundation review](qualification/CONSOLE-FOUNDATION-2026-10-09.md) records native retained-output, early input, raw 5,547-byte input and handoff results; full family/device conformance remains open.
- Acceptance: Close the contract's complete coverage matrix with independent wire vectors, malformed/error/recovery controls, checked K sessions, query/geometry generation, all applicable addressing and input modes, raw binary transfers, display/partition/graphics/DBCS/programmed-symbol and printer operations. Qualify stable ordinary updates, draft/cursor preservation, navigable bounded history, clipping/stacking/focus and a separate opt-in C application. Preserve unchanged CMS31, TSO31/64 ANY/HIGH, bounded CMS24/TSO24 IO24, line/monitor and durable-transcript behavior. Record exact reviewed/frozen inputs and separate host, compile/assemble/link, native-channel, device/emulator and release outcomes. Missing physical/emulator facilities remain explicitly unqualified, never claimed from successful framing alone.

## PD-025: K and U service communication foundation

- Type: improvement
- Status: Done
- Target: SVC/context return, full-width U transfers and K channel/event services on the one-CPU profile; prerequisite for PD-024.
- Observation: The 0.2.0 wait helper and wake handlers addressed the third PSW byte instead of the second for the architectural WAIT bit; the strengthened fresh machine control stopped at the planted wait before reaching its service cases. The external-wake path also did not mask I/O before returning to service C as the I/O-wake path did. Console output acknowledged pending status before SSCH, potentially discarding attention, and failed output did not prove quiescence before workspace reuse. Large spans repeated translation/permission walks for each 256-byte fragment. A rejected native image selector created two compatibility mappings before returning its error; bounded purge provenance identified that mutation.
- Direction: Use synchronous checked K/U requests and complete-span preflight, explicit operation ownership and actual completion events. Keep I/O masked between checking status and entering an atomic enabled wait, preserve interrupted U context, retain early/combined attention, and quarantine a workspace after failed clear. Timers detect stalls only. Keep the existing small gate compatible and measure useful work separately from device latency.
- Evidence: The [shared I/O acceptance record](qualification/IO-FOUNDATION-2026-10-09.md) records 537 passing guest assertions across 24 fresh phases, focused C89 checks, exact disk/tape readback and reviewed source/input freezes. All supported post-handover K physical I/O uses the common ownership/completion/cancel engine; native large transfers use common whole-span preflight/copy. Normal-IPL U64 requests average 39.427 microseconds for 1,000 checked calls. Three-record grouping reduces the probe from 120 to 40 submissions and elapsed time by 17.1%; 100 64-KiB roundtrips measure 13.075 MiB/s under one-CPU Hercules. Early input, native callers, file cursors, failed write and healthy retry pass.
- Boundary: This is a locally qualified development checkpoint. Stable U mappings on one CPU, eight supplied bindings, non-wrapping 32-bit tickets, bounded spans and physical-device gaps remain explicit. The bootstrap IPL driver precedes K/U establishment. Full 3270-family/session/panel work remains PD-024; release and managed adoption are separate gates.
- Acceptance: Review/freeze the complete affected source; pass portable and native build checks, deterministic status/cancellation controls and fresh K/U guest context/entry/return, invalid-pointer/permission, matched/late completion, real input/output and fault cleanup. Measure SVC/request and bulk-transfer costs on named inputs without using time as readiness. Preserve native calling conventions and U state, and show that normal, early, unrelated and failed events cannot fabricate success or lose a wakeup. Record the one-CPU limit and any remaining real-device qualification gaps explicitly.


## PD-026: First-class cREXX source-script execution

- Type: improvement
- Status: Open
- Target: U shell/application services, maintained cREXX runtime and optional application packaging; later than 0.2.1
- Observation: Prepared images already run qualified unchanged cREXX bytecode and fresh RXC → RXAS → RXVM chains. The published 0.2.0 base image contains no cREXX package. The approved 0.2.1 development increment bundles pinned TSO31 RXC/RXAS/RXVM and examples; PCOMM still does not offer a single source-script command with a defined import/search/cache and argument contract. Existing CMS/TSO execution is not general Rexx shell integration or an ADDRESS PDOS implementation.
- Direction (10 October): I want later versions to run Rexx source scripts conveniently through cREXX. A proposed REXX name arguments command should use the existing compiler/assembler/VM route, with explicit package, language/version, source encoding, libraries, arguments and result semantics. A later ADDRESS PDOS environment should use U host bindings backed by checked K services. Keep maintained cREXX implementation in its owning repository and consume a pinned, qualified package; the command spelling and packaging contract remain design decisions.
- Command-environment step 1 (10 October): First qualify calling existing command environments. Use a common C fixture with separate CMS31 and TSO31 native adapters, built through both modern GCC/newlib and Classic C/PDPCLIB. Query actual availability and use documented enumeration where supported; execute a benign command and a controlled failing command, verify output/result semantics and preserve caller files, storage and console ownership. Establish the native contract on the named real CMS and TSO guests, then freeze and run those exact binaries unchanged on PDOS. Extend the SDK OS adapters and PDPCLIB native services only as required for this invocation/query slice. PCOMM and a PDOS-specific command entry should share reusable U/C dispatch backed by the existing checked K invocation/I/O services. cREXX's bridge requires separate qualification against its actual packaged runtime.
- Command-environment step 2 (10 October): Application-defined environment registration is later roadmap work and does not gate step 1. Add register/query/remove and application callbacks, including exact command text, nonzero results, nested calls and cleanup/ownership controls. Preserve the native CMS command and TSO Rexx language-processor lifetimes through their distinct binary adapters. This enables future editor macro environments such as THE; the first fixture does not create a custom environment or exercise a custom callback.
- Native client checkpoint (10 October): The owning Mainframe ELF SDK now has a common C fixture, CMS31 SUBCOM/CMSCALL adapter and TSO31 read-only IRXINIT/IRXSUBCM discovery with IKJEFTSR command invocation. Native CMS20 and z/OS 1.5 runs establish the selected query, command success/error and caller file/storage preservation paths. Direct IRXHST calls from generic C failed and are retained as negative controls; invoking other Rexx environments remains unsupported pending a proper runtime contract. PDPCLIB PCL-002 records missing whole-library Classic CMS31/TSO31 build/ABI prerequisites. PDOS implements the bounded native interfaces and a shared stateless U/C dispatcher. Unchanged native clients and separate CMS/TSO PDOS clients pass model5, model2 and line qualification: 189 checks, real child RC7/output, contained hardware faults and caller file/storage continuation. Native A-default resolution and repeat-launch mutable image refresh were repaired. The [PDOS review/freeze record](qualification/COMMAND-ENVIRONMENTS-2026-10-10.md) and the SDK's libc/doc/COMMAND-ENVIRONMENTS.md own exact inputs and limits. Application registration and the actual cREXX ADDRESS bridge remain later work.
- Evidence: PD-003/016 and the normal K/U workload qualification establish the existing native execution and compile/run paths. The console stage 3 qualification preserves native output, input and child return, but does not qualify a source-script command or a Rexx ADDRESS host environment.
- Acceptance: One documented command compiles, assembles and runs a source program, passes exact arguments and imports, distinguishes compilation/loading/OS errors from application RC, and preserves console ownership and caller state. Qualify file read/write, interactive and line/LLM input, explicit text/binary records, failed compilation and repeat execution without stale outputs. Define the supported cREXX language level instead of implying unrestricted classic Rexx compatibility. Qualify ADDRESS callbacks separately when added; no macro or VM policy moves into K.

## PD-027: U-space text editor, assessing a reduced THE profile

- Type: improvement
- Status: Open
- Target: A separate U editor application and shared 3270/line presentation API; later than 0.2.1
- Observation: PDOS now has retained panels/history and checked device/input sessions, but no interactive file editor. The maintained THE CREXX fork separates its logical editor model from curses and LLM drivers. Its supported platforms are currently macOS, Linux/POSIX and native Windows, with C99 code, runtime-loaded drivers and a crexxsaa macro bridge; none of those facts establishes a PDOS port.
- Direction (10 October): I want an editor in a later release, possibly a reduced build profile of our THE fork. VM/370 CMS EDIT and VM/CE's MECAFF EE are also relevant precedents/candidates. EE is a C fullscreen editor written by Hans-Walter Latz and released to the public domain; assess its CMS record-file/console dependencies and fsio/fullscreen adapter against our checked PDOS services before choosing the port. Its existing 3270 support may reduce the display work, but there is no PDOS port result yet. Assess reuse in the owning THE repository, add a block-mode 3270 driver and explicit record-file/platform adapter, and select a named mainframe compiler/runtime. Start with open/edit/find/change/save/undo and useful XEDIT/THE commands; retain a semantic line/LLM route. A statically selected driver and optional features can bound the first profile without maintaining a copied editor fork in PDOS. cREXX macros and ADDRESS THE can follow the source-script/host integration in PD-026.
- Evidence: THE's maintained README and doc/architecture.md describe the driver-neutral editor/LLM model; doc/crexx.md describes the existing compile/cache and ADDRESS THE bridge. [VM/CE V1R1.2](https://www.vm370.org/node/4) identifies the shipped MECAFF EE tools and source. The [MECAFF upstream](https://github.com/devhawala/mecaff), reviewed at 1694fa893eb962ef1dca156247c6c3cca1750ac8, supplies separate eecore/eecmds/eescrn/fsio modules; the core uses CMS file-state/open/read/write/erase and rename operations, so file replacement needs explicit adaptation as well as fullscreen I/O. PD-024 owns the PDOS presentation/input contracts. There is no compile/link/guest proof of THE on PDOS yet.
- Acceptance: Record compiler/runtime/API closure and licensing/source ownership before selecting the port. Run the editor entirely in U, translate real submitted 3270 fields/AIDs/cursor into logical edits, and preserve K-owned session/buffer lifetimes. Open, edit, save and reopen supported FB/VB text without silent record truncation or encoding loss; exercise undo, failed saves, search/change, scrolling and models 2/5 plus monochrome and line/LLM use. Exit and fault recovery must return the intact Workbench and release every owned resource. Preserve THE's GPL notices and keep its maintained source/profile in the owning editor repository. Embedded cREXX/macros need separate native host-bridge qualification.


## PD-028: Interim line editor, useful C utilities and examples

- Type: improvement
- Status: Done (bounded development qualification)
- Target: Portable U31 C applications, supported record-file services and normal image packaging; proposed 0.2.1 addition
- Observation: The console and DISKMAP show the new application services, but the default image has no file editor or small documented C application/example collection. Standard VM/370 CMS EDIT provides a useful line-command model; VM/CE's EE is a separate fullscreen candidate under PD-027. Paul Edwards also distributed uemacs and edlin in his Seasik C utility collection. These historical ports are not qualified on our protected K/U console.
- Direction (10 October): I want useful utilities and C examples, with a small interim line editor while the fuller editor is assessed. Recommend a bounded CMS-style EDIT application supporting INPUT/INSERT, PRINT, DELETE, CHANGE, LOCATE, a small undo facility, SAVE AS and explicit exit/discard behavior. Use the same semantic command/line interface on the Workbench and teletype/LLM consoles; keep buffers and editing in U. Assess reusable line-editor sources and their exact terms before import, or implement the small portable core directly in the owning app tree. The small original C90 implementation was authorized on 10 October; the fullscreen port remains separate.
- Utility/example scope: Include DISKMAP in the standard image, then a small literal FIND utility, HEX dump and CMP/FC comparison. Existing TYPE/DIR/COPY/RCOPY commands remain useful and should be documented rather than duplicated. Ship short C90 examples for Hello/arguments, record-aware file read/write and owned panels/key events, with named cross-build recipes and executable guest results. These examples do not imply a self-hosted C compiler.
- File boundary: Initial saves should use a separate supported/preallocated output dataset until safe replacement is qualified. Preserve logical FB/VB records, blank lines, trailing spaces, explicit native encoding and declared LRECL; reject unsupported formats/overlong records before mutation rather than silently truncate. The current allocator's exchange-volume restriction and distinction between ordinary byte-stream COPY and record-preserving RCOPY must be visible in the workflow. Do not assume POSIX rename or CMS file-erase/rename semantics exist on the current K route.
- Evidence: [IBM VM/370 EDIT Guide, Release 2](https://www.bitsavers.org/pdf/ibm/370/VM/370/Release_2/GC20-1805-3_VM370_EDIT_Guide_Rel_2_Mar75.pdf) documents the command model. [Paul's 2019 software announcement](https://forum.osdev.org/viewtopic.php?t=35061) names uemacs/edlin in the Seasik utilities. The current media guide and PD-018/020 plus native I/O qualification own the supported record-file/volume boundaries; PD-024 owns console/input/session behavior.
- Acceptance: Compile/assemble/link from maintained sources and the declared PDPCLIB profile, run the actual applications on models 2/5, monochrome and a plain line primary, and return an intact Workbench with meaningful OS/application results. Edit and save real source/text fixtures, reopen and independently verify exact stopped-disk records, and exercise failed saves, full/invalid destinations, unsaved exit, empty/padded/long input and command scripts. Start with the required focused checks, review/freeze, then run the combined final application/console qualification. Keep generated binaries, copied upstream references and guest media outside Git; publish only maintained attributed sources and repeatable recipes.

## PD-029: Coordinated machine and K service capacity

- Type: architecture / capacity
- Status: Done (selected 512 MiB profile)
- Target: K C31 service-bank real backing, DAT mapping and bootstrap/native reconstruction
- Observation: The approved application/record-file increment builds a 392,670-byte service in the 393,216-byte bank, leaving 546 bytes. This fits the current reviewed candidate but leaves little room for later K facilities. The adjacent real region backs the kernel stack; increasing a constant alone is unsafe.
- Direction: Review relocation or additional disjoint service backing before the next K expansion. Keep the one current source tree and protected K/U contract; do not spend stack space or weaken build bounds to fit new code.
- Acceptance: Record a reviewed real/virtual storage map, boot/DAT/load changes and generous disjoint stack headroom; qualify reconstruction, boundary failures and guest workloads for the selected layout before increasing the supported bank.

- Approved scope (10 October): Implement the reviewed 512 MiB real/16 MiB core profile, contiguous 2 MiB service, 512 KiB stack, 3 MiB K/U DAT pools, 512-entry ledgers/resources and 4096-line U editor. Move the trampoline and coordinate packaging, boot and binary adapters; use the desktop assembler profile under AS-007. See [the capacity review](architecture/CAPACITY-REVIEW-2026-10-10.md).

- Evidence for PD-028/029: [The 10 October application qualification](qualification/APPLICATIONS-2026-10-10.md) selects 68 passing guest checks across models 2/5, monochrome and a plain line primary, with exact exchange/other IPL records and durable transcript readback. Each runs the pinned cREXX source chain and verifies/releases 320 MiB. The final K service has 1,642,938 bytes spare. The line provider's 148-character input boundary is explicit and its overflow recovery is separate evidence. This closes the selected development slice; release and fleet adoption remain separate.

## PD-030: Larger configurable disks and extent-aware stored files

- Type: architecture / capacity / performance
- Status: In progress
- Target: Default media geometry, PDOS.STORE directory/banks, native record transfers
- Observation: The default disk has 100 cylinders; the durable store has eight fixed 2 MiB slots and duplicated banks. Operation 37 currently supports first-track PS FB/VB and separate empty destinations. Increasing memory or editor capacity does not remove these disk limits.
- Direction: Design configurable media geometry and an extent-aware store with larger files, allocation/reuse and migration/recovery rules. Extend record I/O across tracks/extents through the common owned completion engine, preserving record boundaries and checked physical bounds. Consider contiguous transfer batches and measured channel/start counts; do not change persistent layout by multiplying constants.
- Acceptance: Review version/migration and crash semantics, exact stopped-disk readback and preservation controls; measure fragmented and contiguous transfers and qualify all supported paths. Implementation requires its own selected storage-layout slice.
- Authorization (10 October): Publish 0.2.1, then begin 0.2.2 with disk layout and larger files as the initial focus. [The selected storage design](architecture/STORAGE-EXTENTS.md) keeps native datasets/extents, bounded streaming and an explicit versioned-store migration contract.
- Initial slice: Supplied-storage geometry/range helpers and allocation-map unions no longer assume 100 cylinders. Focused C89 ASan/UBSan controls cover 1024/65535-cylinder declarations, cross-cylinder extents and rejection without partial mutation. The K status caller retains an explicit 100-cylinder admission gate while its 1500-cell buffer and clients remain fixed. Host/Classic C object checks do not qualify larger production disks or file transfers. Dynamic K/U status, media admission, streaming, STORE v2, migration/recovery and measured I/O remain outstanding; the full combined guest QA is at the end.
- Evidence: [Initial geometry slice](qualification/STORAGE-SLICE1-2026-10-10.md) records the reviewed caller guard, host controls and successful Classic compile/assemble/link with explicit unchanged kernel objects. The default guest disk remains 100 cylinders until the coordinated producer/admission/client slice is qualified.
- Second local slice: K owns a dynamically sized read-only map and preflights all U chunks before I/O/publication, rejects header overlap and copies only actual cells. Version 1 retains complete summaries; version 2 queries geometry without a map allocation or repeated VTOC scan. DISKMAP owns its U map, aggregates larger declared volumes into bounded labelled rows and closes panels on errors. [The focused checks and native K/U links](qualification/STORAGE-SLICE2-2026-10-10.md) pass; physical capacity admission, producers, writers, streaming and persistent STORE changes remain outstanding. No new guest run or larger-disk support claim follows from these checks.
- ADDRESS prerequisite (11 October, in progress): The approved current cREXX Classic chain needs a 6.4 MiB G library and up to an 8.6 MiB framed link result. An explicit disposable `large-files` qualification profile will admit 16 MiB ordinary CMS files and journal objects on freshly produced larger media. Its checked layout descriptor, computed bank stride and extent bound must reject old/mismatched media before writes. Keep the released/default 2 MiB STORE profile, eight slots and its readback available; do not upgrade managed disks. This bounded banked profile does not close the selected extent-allocation/streaming STORE-v2 roadmap. Acceptance for this prerequisite: boundary, failed-write/restart and layout-refusal controls; coordinated native core/producer/oracle identities; fresh CMS and TSO real-script chain/readback on PDOS with caller/resource recovery. No guest proof yet.

## PD-031: DX3270 code-page selection and emulator compatibility follow-up

- Type: compatibility / emulator roadmap
- Status: Open
- Target: DX3270 native rendering and configured IBM1047 selection; retain x3270/s3270 reference behavior
- Observation: DX3270 1.7.5 reports CP1047 in its connection panel but renders source brackets as CP037 glyphs in the 10 October model-5 preview. The same dataset and K text decode correctly in the stopped transcript and s3270's `screen-8.txt`. This is separate from the assembler's literal-data boundary under AS-008.
- Native repaint observation: A repeated EDIT PRINT and later command transitions leave parts of the retained panes blank in this preview; PF9 restores the full presentation. Keep this as an observed compatibility defect pending a minimal wire replay and comparison with s3270, rather than assigning a cause from screenshots alone.
- Clear restores the complete native presentation, including DISKMAP, without losing its input request. The native Save Screenshot PNG for that active graph omits most of the shell pane although the window shows it correctly; that export is kept as defect evidence and excluded from the clean marketing set.
- Direction: Reproduce with minimal independent byte vectors, inspect the emulator's selected mapping and session propagation, and consider an upstream patch as previously requested. Keep clean development marketing views separate from defect evidence. Do not alter PDOS file bytes to accommodate a mismatched renderer.
- Acceptance: Named emulator versions render the same IBM1047 byte vectors and field/input records correctly, retain Clear/repaint and key behavior, and complete a native application walkthrough. An upstream offer/publication remains a separate decision.

## PD-032: Native TSO PDS output member identity

- Type: defect
- Status: Done (bounded local qualification)
- Target: K native TSO file OPEN and the maintained SDK MVSSUPA work-area contract
- Observation (12 October): The ADDRESS qualification writes output under DD-tail/member-prefix journal keys instead of the requested eight-character member. Later input FIND reads the empty physical placeholder. RXAS can accept empty input before RXLINK fails; this is file identity, not an allocator failure.
- Repair: Use the symbol-derived `ZMEM-OPENCLOS` displacement, 148 bytes, through the existing checked U-span/owned cleanup route. Reject an empty member. The former literal was 144 bytes. Freeze the native ABI header with each normal core.
- Acceptance: Exact selected-source assembler proof and mismatch rejection; newlib/MVSSUPA binary write/reopen/replace of similarly named members; stopped journal keys/payload readback; affected PDOS CMS/TSO real-script replay and normal resource recovery. Preserve the failed media and earlier receipts.
- Review: [Native PDS member review](qualification/ADDRESS-PDS-MEMBER-REVIEW-2026-10-12.md). This private service work area is qualified against named maintained SDK inputs, not arbitrary vendor implementations. Managed images remain unchanged.
- Evidence: [12 October ADDRESS qualification](qualification/ADDRESS-CREXX-2026-10-12.md) records the native binary member write/reopen/replace, full CMS/TSO B and Classic compiler/runtime chains, independent stopped output/transcript/guard validation and normal final fleet handback. Source/core and runtime/fixture/media freezes remain separate; older failed receipts are preserved.

## PD-033: Card-reader batch jobs and 3270 job forms

- Type: improvement / native compatibility / release qualification
- Status: In progress
- Target: pdos-zarch K/U, Hercules 3505 reader and 1403 printer, native TSO31 batch/DD services, PCOMM and retained 3270 panels
- Observation: The current product has a retained Workbench console, semantic line/teletype delivery, a common owned channel engine, scoped native invocations/DDs and bounded 3287 printing. PCOMM command files do not provide a JCL job service, and the 3287 facility is not a 3505/1403 batch route. Native TPUT/TGET and invocation-local DDs do not establish background-TSO or preallocated job-DD compatibility.
- Direction selected (13 October): Keep the existing console as the human/LLM operator surface. Add the same card-deck/printer transport used for z/OS on Hercules. Card reader, console submission and 3270 Submit/Jobs/Output forms share one job service. Start with serial TSO31 jobs and a strict documented JCL subset for cREXX; preserve existing foreground and native behavior.
- Authorization (13 October): Implement Stage 0 and the Stage 1 card reader/printer proof. Use a disposable qualification image, preserve current console behavior, and review/freeze before guest QA. Stages 2–6, managed-image adoption and publication remain separate increments. The existing PD-030 storage scope is unchanged.
- Authorization (14 October): Commit the Stage 0/1 baseline locally and implement Stage 2 serial descriptors, bounded spool and operator recovery. Baseline commit `70b6f02` contains the selected contracts and device proof. Following acceptance, I authorised the local Stage 2 commit and Stage 3 implementation with prebound native DDs and genuine batch I/O. Push, managed-image adoption and Stages 4–6 remain separate decisions.
- Design: [Batch job contract](architecture/BATCH-JOBS.md). Evidence for reusable components: [owned I/O qualification](qualification/IO-FOUNDATION-2026-10-09.md), [console checkpoint](qualification/CONSOLE-STAGE3-2026-10-09.md), [ADDRESS qualification](qualification/ADDRESS-CREXX-2026-10-12.md) and [storage direction](architecture/STORAGE-EXTENTS.md).
- Stage 3 continuation authorised (14 October): Commit the first native DD checkpoint locally, then qualify dataset/member I/O and implement concatenation and job-owned temporary PASS/DELETE/KEEP support. The first checkpoint is local commit `3676128`. Push, managed-image adoption and the shared SDK/cREXX/native-reference gate remain separate decisions.
- Stage 3 package/reference authorised (14 October): Commit the data checkpoint locally, then implement and qualify one shared SDK/cREXX batch package on PDOS and a named native z/OS reference to complete the selected Stage 3 gate. Preserve the console and distinguish direct batch from background TSO. JCL/PROCs, job forms, managed adoption, push and release remain later decisions.
- Stage 3 package accepted (14 October): The reviewed shared TSO31 SDK/cREXX package passed native z/OS 1.5 first, then all 30 PDOS line and 34 model 2 assertions. Independent durable VB decoding matched all five binary/compiled/linked/ADDRESS/PARSE outputs exactly to z/OS; expected RC 2/8/16 and dependent skip, compiler exits, command output, normal shutdown and resource handback passed. [The result summary](qualification/BATCH-STAGE3-PACKAGE-2026-10-14.md) records the XMIT/CLI repairs, fixture/watchdog corrections and current resident PCOMM/router requirement. Reuse normal SDK `build/tso-batch`, cREXX `build/mainframe-batch` and PDOS `build/pdos/batch-sdk` paths, retaining result summaries only. All guest configurations and operations go through the Mainframe Operator. This closes selected Stage 3; PD-033 remains In progress for Stages 4–6, with managed adoption, push and release separate.
- Overall acceptance: Submit common cREXX JCL through the actual virtual reader on PDOS and a named z/OS/Hercules guest; execute the pinned native package with prebound DDs and real batch I/O; collect complete job/step/DD printer reports and exact dataset outputs; reproduce the same job behavior through the PDOS console and 3270 forms. Distinguish application RC, OS/service failure, execution completion and print delivery. Recover the console and all resources after success, failure and cancellation. Close only the selected qualified profile, with wider services and released artifacts separately identified.
- Publication and continuation authorised (15 October): Commit and publish the accepted Stage 3 source in its owning repositories, then implement Stage 4 JCL, procedures and console submission. Stage 5 forms, managed-image adoption and a release remain separate gates. Guest configurations and qualification continue through the Mainframe Operator.
- Stage 4 accepted (15 October): Baseline `945de26` is committed locally. Shared JCL/PROCs, checked site/DD admission and dataset/reader/framed-console submission pass native z/OS 1.5 first, then full PDOS line/model-2 matrices (33/37 assertions, each 68 commands and 17 jobs). All five output hashes match native z/OS. The final console-label IBM1047 repair passes focused line/model-2 checks (16/20 assertions), with UTF-8 application text and binary boundaries explicit. Independent stopped-state/readback acceptance, normal shutdown, cleanup and exact lease handback close selected S4-AC01–05. [The dated result](qualification/BATCH-STAGE4-2026-10-15.md) records the reviewed freezes, repairs, reuse and limits. Stage 5 forms and Stage 6 adoption/release remain open.
- Stage 0/1 checkpoint (13 October): Initial capacities, entry/reference cases and neutral device ABI are selected. The implemented 3505/1403 subset passes four host control groups, a fresh complete native build and 46 harness checks across model 2 and plain 3215 guest runs. Independent stopped endpoint/record/transcript readback and normal cleanup pass. [The dated result](qualification/BATCH-STAGE1-2026-10-13.md) pins review freezes, repairs, exact inputs and limits. This closes the selected Stage 0 contract and Stage 1 local device proof, not the complete PD-033 item or a release.
- Transport finding: The installed Hercules reader presents EOF for a truncated EBCDIC host file. The host adapter rejects non-80-byte framing before configuration/devinit. Physical partial-transfer rejection is checked in the portable/actual-provider host controls; it is not claimed as a Hercules partial-card guest result.
- Stage 2 checkpoint (14 October): The serial descriptor service, bounded STORE spool, owned-allocation admission, distinct execution/print results, interrupt-key STATUS/CANCEL/CONTINUE and interrupted-job recovery are implemented and locally qualified. Two normal console profiles, exact stopped-disk recovery and two deliberate cuts pass 93 harness checks. Independent endpoint/channel state, durable spool/transcript readback and normal handback pass. [The dated result](qualification/BATCH-STAGE2-2026-10-14.md) pins freezes, test-controller repairs and evidence reuse. This closes the selected Stage 2 gate; native prebound DD/batch runtime, JCL, forms, paired z/OS work and release remain open.

### Delivery sequence and stage gates

The stages below belong to this item; they are not another work queue. Each
implementation stage receives final source review, focused checks and an exact
source/input freeze before expensive guest qualification. A repair returns to
review and a new freeze. Reuse unchanged checks with an explicit dependency
review. A stage passes only when its stated result has been recorded.

| Stage | Deliverable | Gate before advancing |
| --- | --- | --- |
| 0 | Freeze the shared job, console, native-entry, JCL and spool contracts | Reviewable reference jobs and independent expected results; bounded capacities and active-job control behavior agreed in the contract |
| 1 | Actual 3505 reads and 1403 writes on the common I/O engine | Fresh PDOS IPL proves exact input records, readable print records, reload/EOF, errors and unchanged console operation |
| 2 | Serial job/step execution, bounded spool and operator recovery | Two sequential jobs with truthful return/failure states, closed output and no DD, storage, terminal or I/O leaks |
| 3 | Prebound native DDs and genuine batch input/output | Native TSO31 probes and cREXX tools read/write the allocated DDs without consuming operator input; batch ABI and native reference match |
| 4 | Common JCL conversion, procedures and console submission | The cREXX build/run procedures execute through the reader and console; malformed/unsupported jobs are rejected before program execution |
| 5 | 3270 Submit, Jobs and Output forms | The same expanded jobs/results as the other routes, with model/line, ownership, retry and recovery checks |
| 6 | Paired qualification controller and release checkpoint | Exact-package PDOS/z/OS comparison, failure controls, stopped dataset readback and clean-consumer IVP; manual release/install gates remain explicit |

### Stage 0: Freeze contracts and reference jobs

The selected initial bounds, native entry inventory, reference cases and Stage
1 device ABI are pinned in [the Stage 0 contract](architecture/BATCH-STAGE0.md).
Implementation/qualification status is recorded here and in dated receipts.

- Pin the one-CPU K/U development profile, machine/console devices and generous TSO31 heap/storage budget. Inventory current channel, invocation, DD, panel and STORE limits before setting bounded job/step/DD/spool capacities.
- Select direct native batch entry versus a required batch-TSO command launcher using the actual candidate's ABI. Inventory the QSAM/BSAM and GETLINE/PUTLINE forms reached by those entries. Do not assume an EXEC PGM invocation can use a foreground command-processor parameter list.
- Freeze serial dispatch and the active-job K control subset. Preserve ordinary PCOMM behavior at idle; specify status/notices/cancel and busy/queued responses while shared U is executing a native step. Select a bounded recovery mechanism for a CPU-bound step as well as a blocked I/O step; do not imply a second shell or general scheduler.
- Freeze reader framing, converter/PROC grammar, immutable job descriptors, DD inheritance/override/disposition, spool publication and restart behavior. Syntax errors start no programs; resource failure, skipped steps and print errors have separate results.
- Establish the common source procedures: CRXC builds RXBIN; CRXCG builds/runs; CRXCL/CRXLGO must declare retained/temporary linked-RXBIN outputs. Reserve SYSLMOD for native modules and keep any future native wrapper/packager separate.
- Prepare portable jobs for a tiny C batch probe, supplied RXBIN, fresh compile/assemble/run, linking/library lookup, DD records and ADDRESS TSO31. Keep source/data/deck/code-page/hash identities and expected RC/record results independent of the implementation.
- Gate: The reference jobs and selected launchers have an explicit native z/OS proof path, each grammar/ABI feature has an owner, and capacities fit a reviewed spool/media profile. Open service/PROC questions are resolved before code depending on them is accepted.

### Stage 1: Reader and printer device proof

- Add configurable 3505/1403 registration, CCW builders and status/sense validation in K over the common completion engine; give each device its own owned real workspace. Extend the source image/recipe's optional batch device profile with collision checks.
- Read complete 80-byte EBCDIC cards; distinguish EOF, no medium/intervention, short transfer, unit/channel error and late status. Qualify a second deck after reload/attention without rebooting.
- Write bounded 1403 records with explicit line/page control. Keep 3287 behavior and console/monitor ownership independent. Retain incomplete-print errors rather than claiming an accepted channel request was a delivered report.
- Gate: Fresh Classic build/link/image/IPL, exact card hashes, independently checked Hercules print output, absent-device/error/retry controls, workspace cleanup and the affected 3270/line console regressions. No JCL execution claim yet.

### Stage 2: Serial job service and bounded spool

The selected implementation contract is [serial descriptors and spool](architecture/BATCH-STAGE2.md).

- Add K-owned job/step/result descriptors and a serial dispatch gate using current native invocation/unwind services. Exercise an explicit test descriptor before introducing the JCL converter.
- Admit complete job input before queue publication; assign non-reused job identities and separate execution from print state. Preserve step OS status, normal RC, abnormal failure and skip reason; final results follow close/disposition cleanup.
- Use a reviewed bounded single-job spool envelope within admitted storage for the first proof. Preflight its slot/byte usage together with transcript, libraries, temporary files and outputs. Reject exhaustion explicitly and keep the existing STORE layout/default media intact.
- Implement the frozen K status/notices/cancel route, idle console restoration and controlled recovery of CPU-bound, faulting and I/O-blocked steps. Reader/printer operations never own batch terminal input. Hold/release and purge follow checked ownership.
- On restart, retain a committed result or report interrupted execution; do not automatically replay uncertain dataset writes. Reprint retained output without rerunning the job.
- Gate: Two sequential descriptor jobs, normal RC 0/nonzero, load failure, child fault, print failure, quota refusal, cancellation and interrupted-spool controls. Prove terminal/input isolation, return to the current console and no resource leaks by actual state/readback.

### Stage 3: Native DD and batch I/O compatibility

The [first increment contract](architecture/BATCH-STAGE3.md) records the
implemented immutable DD descriptor, step lookup/inheritance and selected
native BSAM dataset/in-stream/DUMMY/SYSOUT adapters. The
[first checkpoint](qualification/BATCH-STAGE3-2026-10-14.md) passes 52 checks
across line and model 2 consoles, with native binary input, nested DD ownership,
exact SYSOUT/printer output and independent resource handback. The
[data contract](architecture/BATCH-STAGE3-DATA.md) and
[second checkpoint](qualification/BATCH-STAGE3-DATA-2026-10-14.md) add locally
qualified PS/PDS-member I/O through the native journal, ordered like input
concatenation and job-owned temporary NEW/OLD/PASS/DELETE/KEEP. Its 99 checks
include both console profiles, earlier DD probes and serial/cancellation
regressions, with independent stopped ownership and exact STORE readback.
The [shared package/native-reference result](qualification/BATCH-STAGE3-PACKAGE-2026-10-14.md)
closes the remaining selected Stage 3 gate on native z/OS 1.5 and both PDOS
console profiles. JCL and job forms are later stages.

- Add a step allocation context above invocation-local DD tables. Bind DDs before entry, support scoped lookup/inheritance and preserve the caller's existing bindings across nested commands and unwind.
- Implement the chosen dataset/member, in-stream, DUMMY, concatenation, temporary-data and SYSOUT adapters with explicit FB/VB framing, EOF, empty-record and encoding behavior. Freeze the selected DCB/SPACE/DISP semantics and protect unrelated datasets.
- Implement the native batch entry/service forms selected in Stage 0. Maintain correct foreground behavior; do not hide an application's TPUT-only background bug by routing all terminal output to spool.
- Treat upstream cREXX/SDK batch-capable entry/runtime work as a separate owned prerequisite. Once available, pin one package for both targets; do not qualify a PDOS-only executable as that shared-package result.
- Gate: Direct batch probe and selected TSO31 cREXX executions on real DD bindings, with exact native reference results. Include missing DD, empty input, ordered binary bytes, reopen/replace, temporary PASS/delete, nested DD visibility, invalid native spans, failed writes and final cleanup. Operator input remains untouched.

### Stage 4: JCL, procedures and console submission

Status: **Done (selected source-development qualification).**

I authorised this stage on 15 October after publishing the accepted Stage 3
checkpoint. The intended outcome is an ordinary shared JCL deck submitted from
cards or the existing console, expanded through common cREXX procedures and
executed by the checked job/DD service. The console keeps its present role;
Stage 5 adds forms over this same implementation.

Continuation authorised (15 October): Commit the checked converter baseline
(`945de26`) and complete Stage 4, including its native-first operator gate.
The selected extension keeps legacy job and 3 KiB DD inputs readable, adds a
version-3 8 KiB DD descriptor with 32 physical entries, and uses a version-4
288 KiB spool envelope within the existing STORE slot. A checked in-place
upgrade validates all older 256 KiB contents before moving their DD regions;
uncertain execution remains interrupted rather than replayed. Version-2 job
descriptors add bounded persisted source/qualified-step metadata without
changing the native eight-byte program/step ABI. These are source-development
changes; managed image adoption, Stage 5 forms and release remain separate.

Acceptance criteria:

1. **S4-AC01 — conversion (Done):** A supplied-storage C89 U converter accepts complete
   IBM1047 cards, selected JOB/EXEC/SET/DD syntax, continuation/quoting and
   explicit end/data framing. It returns checked job/DD descriptors only after
   the whole input validates. Malformed, unsupported and oversized input reports
   the original card/column and starts no program. Host sanitizer controls and
   a Classic compile/assemble check verify the portable core.
2. **S4-AC02 — procedures (Done):** Common CRXC, CRXCG, CRXCL and CRXLGO source procedures
   expand defaults, symbols and step-qualified overrides with truthful RXBIN
   outputs. Missing/recursive procedures, undefined symbols and expansion limits
   fail admission. Independent expanded-job expectations verify each procedure.
3. **S4-AC03 — allocation and search (Done):** Explicit site bindings and checked
   program/procedure library order select actual members. DD concatenation,
   native record attributes, selected SPACE/DISP/temporary semantics and
   COND=(0,NE) retain the Stage 3 ownership and dependent-skip rules. Missing
   libraries and quota failures cannot publish a runnable partial job.
4. **S4-AC04 — submission and control (Done):** Reader, SUBMIT dataset/member and
   explicitly framed console input use that converter and existing job service.
   HELP, JOBS, STATUS, OUTPUT step/DD selection, HOLD/RELEASE, CANCEL and PURGE
   act on checked identities. Truncation, active-job/busy behavior, cleanup and
   print failure preserve the existing console/input ownership contracts.
5. **S4-AC05 — native and guest result (Done):** The Mainframe Operator qualifies the
   shared procedures/decks on native z/OS first and then on PDOS line/model 2.
   Reader and console produce equivalent expanded jobs, step results and exact
   datasets for fresh build/link/go and expected failure/skip cases. Independent
   readback and resource handback close this stage, with summaries only.

Implementation sequence:

Accepted implementation (15 October): Four shared procedures, 32-entry DDs,
checked spool upgrade, source fingerprints, site/catalog admission and all
three submission routes pass their complete selected gates. The 14,653 direct
checks and 49 procedure controls pass in normal and sanitizer builds, alongside
migration/controller/DD/temp and native console-encoding controls, common deck
admission and Classic builds. Native z/OS 1.5 and both full PDOS matrices pass
independent acceptance with exact paired outputs. Reviewed R6 changes only
controller-label encoding in the product and reuses unchanged R5 job/DD proof;
focused line/model-2 UI checks pass, followed by final cleanup and lease handback.
[The Stage 4 result](qualification/BATCH-STAGE4-2026-10-15.md) records exact
inputs and the dependency review. S4-AC01–05 are complete for this profile.
The output-review requirement includes creation-time held native output,
repeat retrieval, PDOS output HOLD distinct from execution RUNHOLD, step/DD
selection, printer retry without rerun, and operator-owned cleanup. The
[batch user guide](user/BATCH.md) and native runbook own the repeatable sequence.
The sequence below is complete; the initial host-only checkpoint follows as history.

1. **S4-01 (Done):** Implement the portable direct-JCL conversion core,
   source diagnostics and independent malformed/bounds controls (S4-AC01;
   foundation for S4-AC03). Keep catalog/program resolution as explicit read-only
   admission callbacks; do not guess DD direction from DISP.
2. **S4-02 (Done):** Add supplied-storage PROC expansion, scoped symbols,
   qualified overrides and common procedure sources (S4-AC02/03), building on
   S4-01. Freeze only supported native semantics and retain precise rejection.
3. **S4-03 (Done):** Integrate reader and console intake/control/output over the
   existing K service, including durable expanded-step/source identity
   (S4-AC03/04). Reconcile the eight-byte descriptor step name with procedure
   qualified display names explicitly; do not truncate or silently collide.
4. **S4-04 (Done):** Complete source review, focused checks and input freeze,
   then request native-first operator qualification (S4-AC05). Repairs return to
   review/refreeze before affected replay; unchanged Stage 3 checks are reused.

Initial S4-01 result (15 October): The [direct converter contract](architecture/BATCH-STAGE4-JCL.md)
and [host result](qualification/BATCH-STAGE4-JCL-2026-10-15.md) record 14,653
assertions in normal and ASan/UBSan builds, no static-analyzer findings, and
Classic C31 compile/assembly. The 22,012-byte supplied workspace returns checked
job/DD descriptors only after complete validation and read-only site admission.
At that checkpoint only S4-AC01's host portion passed; integration and the other
criteria were still open. The accepted complete result above supersedes that boundary.

The initial operator review identified a descriptor capacity gap: the old
job-wide 16 DD entries could not hold four-step CRXLGO (19 before library
concatenations). The selected versioned 32-entry descriptor and checked spool
migration resolve that gap; overflow is still rejected. Retain separate
JCLLIB, native JOBLIB/STEPLIB and cREXX RXBIN library roles; STEPLIB replaces
JOBLIB for its step. Supply in-stream procedure input via caller overrides for
the historical z/OS reference, keep compiler exits enabled by default, and
qualify any IKJEFT01/CALL interpretation explicitly. Stage 3's direct ADDRESS
execution did not qualify that launcher. The older direct-version syntax
fixture now includes the batch entry's required SYSTERM DD.

### Stage 5: Retained 3270 job forms

Status: **In progress**. I authorised the Stage 4 commits and Stage 5
implementation on 15 October. The accepted product checkpoint is `83283b6`;
the matching cREXX repair is `a9c080f54` and Lab operation is `73d131ab`.

The intended outcome is Submit, Jobs and Output forms over the same checked
job/DD service, with the existing console and semantic line route preserved.
The first increment uses the retained panel body and its owned shared editor:
named fields are selected for editing, then previewed before an explicit
submit. It adds no persistent layout or scheduler. The selected three-job
spool remains bounded; larger history stays with PD-030.

Acceptance criteria (all open until their complete checks pass):

1. **S5-AC01 — preparation:** Submit accepts a checked deck/member or a common
   PROC with source, parameters and DD overrides. Preview shows the exact JCL
   and admits it through the Stage 4 converter without publishing a job. Tests
   compare expanded descriptors with the console route and reject invalid or
   oversized edits before mutation.
2. **S5-AC02 — submission identity:** Only explicit submission of a current
   preview may publish. An attempt consumes its token before the service call;
   repeated Enter, repaint or reconnect cannot retry it. Editing invalidates
   preview, and deliberate resubmission requires NEW plus another preview.
   Failed acknowledgement leaves an explicit uncertain result for JOBS review.
3. **S5-AC03 — job/output control:** Jobs selects actual non-reused identities,
   reports execution/print and qualified-step results, and uses checked existing
   actions. Output filters step/DD and pages retained records without consuming
   them. Stale selections, busy actions, invalid filters and failed printing
   preserve results and do not rerun a job.
4. **S5-AC04 — presentation and ownership:** Retained panels adapt to models
   2–5 and monochrome. Explicit application key ownership supplies form AIDs,
   Clear/repaint and editor focus while ordinary Workbench/console behavior
   returns on every exit/error. IBM1047 interface text and UTF-8 application
   text remain distinct; raw SYSOUT is not translated again. The line route
   supports the same state transitions through plain text.
5. **S5-AC05 — checked implementation:** Supplied-storage C89 controller,
   generator, pagination and retry/error controls pass normal and sanitizer
   checks. Classic K/U build/link and complete source review precede the
   recorded guest freeze. Reuse unchanged native/package proof explicitly.
6. **S5-AC06 — guest qualification:** The Mainframe Operator proves actual
   forms-generated JCL and dataset results, models 2–5, monochrome and line,
   repeated actions, invalid/long fields, cursor/AIDs, reconnect/repaint,
   failure/busy/console return and affected nested foreground/ADDRESS ownership.
   Independent stopped state/readback and clean handback close the stage.

Implementation steps:

1. **S5-01 (Done, host increment):** Factor shared prepare/post operations from the
   Stage 4 controller, add supplied-storage form state, common-PROC deck
   generation, preview/retry and job/output models with decisive host controls
   (S5-AC01–03/05).
2. **S5-02 (In progress):** Bind the controller to retained panels, shared editor,
   scoped form-key ownership and semantic line input; preserve shell behavior
   and native encoding (S5-AC04/05).
3. **S5-03 (Done for initial freeze):** Complete normal/sanitizer and Classic build checks, review
   the whole selected increment and record a source/input freeze before any
   operator guest replay (S5-AC05).
4. **S5-04 (Open):** Qualify the remaining model/recovery/identity/result matrix
   through the Mainframe Operator, independently review results and retire
   temporary outputs (S5-AC06). An initial host/source checkpoint does not close
   Stage 5 or authorize managed-image adoption/release.

First source increment: shared prepare/post, all three form controllers,
native panel/key/editor binding and source-built K/U are implemented. Normal
and sanitizer controls, native line re-entry/error cleanup, shared model layouts
and static analysis pass. [The first result](qualification/BATCH-STAGE5-2026-10-15.md)
records reviewed PD033-S5-R1 and the independently accepted 29-check model-2
proof: one actual native CG job, preview/retry guards, physical PF8, filtered
output, explicit print/purge and ordinary console return. Operator cleanup and
exact lease handback are complete. R2's forced revision-exhaustion control and
Classic U build pass; unchanged R1 guest paths are reused by dependency review.
S5-AC01–05 have selected host/source evidence; complete physical model/recovery
acceptance remains open under S5-AC06.

- Build Submit, Jobs and Output as U presentation over the established checked service. Submit validates source/member, PROC, parameters and DD overrides and shows the generated JCL; Jobs exposes execution/print status and controls; Output selects expanded step/DD and pages ordered records.
- Use the existing field, cursor, AID, model, panel and terminal ownership contracts. Preserve Clear/repaint, monochrome, IBM1047 and the semantic teletype path. Display active-job control limitations rather than assuming a concurrent U form controller.
- Bind submission retry tokens and job actions to real identities. Repeated Enter/reconnect must not submit an unintended second job or act on a stale job; deliberate resubmission is distinct. Purge refuses active or owned records.
- Gate: Models 2–5, monochrome and a plain line console; long/invalid fields, cursor/AIDs, reconnect/repaint, retry, busy state, job failure and console return. Compare forms-generated JCL, allocations and results with reader/console execution; include nested foreground/ADDRESS ownership regressions.
- Dependency: A fixed single-job prototype is possible within declared limits. Useful retained job lists, larger spool records and workload history require the selected PD-030 capacity/allocation/recovery slice; this stage must not silently introduce another persistent layout.

### Stage 6: Paired qualification and release checkpoint

- Retain cREXX orchestration under pdos/scripts/ for the product and Mainframe Lab tools/lab.crexx for private fleet coordination. Separate Stage/Submit/Wait/Collect/Assert so a submission acknowledgment cannot pass a run.
- Use the actual 3505/1403 transport for both PDOS and a leased named z/OS/Hercules guest. Configure and verify the guest JES reader/printer route, unique run/job identities and printer-file boundaries. Parse each system's real job/step outcome and normalize only declared presentation differences.
- Pin candidate binaries, runtime/libraries, PROC sources, test inputs, deck code page and expected records. Qualify supplied RXBIN and a fresh compile/assemble/link/run chain, native DD/input/output, current ADDRESS TSO31, expected nonzero RC, missing DD/library, compiler error, cancellation and resource exhaustion.
- Verify exact datasets independently after shutdown/export. Printer translation/pagination is report evidence, not the binary oracle. Assert all required test counts and known skips explicitly; an incomplete log or timeout fails the harness.
- After TSO31 passes, select and qualify affected TSO64 ANY/HIGH cells separately; preserve constrained 24-bit and CMS regression boundaries. Record source/media/runtime freezes and the exact unchanged evidence reused.
- Gate: Documented clean-consumer PDOS IVP through cards/printer and forms, paired z/OS native results, recovery/restart and affected console/media regressions. Update guides and name remaining unsupported features. Manual installation, IBM Binder/RECEIVE validation, release-image selection and publication remain separate release gates.

### Dependencies and ownership

- PD-025 owns the reusable channel/completion foundation; PD-021/022/024 own existing console, panels and text semantics. Extend their checked contracts rather than maintain parallel drivers or bypass terminal leases.
- PD-030 owns storage capacity, extents and persistent migration. Admit a bounded prototype explicitly; retained history and full-library jobs must fit a selected qualified storage slice before being advertised.
- PD-032's scoped native member identity and the ADDRESS/environment ABI remain explicit dependencies of the native DD/command cases. Reuse their exact evidence only if the affected services and package inputs are unchanged.
- Kernel/job/device/file changes belong to pdos/src/; U converter/forms and small probes belong to the same component. Maintained PDPCLIB belongs to pdpclib/src/, application entry/runtime changes to CREXX or mainframe-cross-sdk, and native output/linker fixes to their owning component. Product automation is cREXX; a required binary/media interface helper may use its existing necessary language.
- Keep only this item as the live delivery queue. Design changes update architecture/BATCH-JOBS.md; supported behavior goes in user guides only after qualification; dated receipts own exact results. No private correspondence, credentials, guest disks or native reference listings enter the product repository.
