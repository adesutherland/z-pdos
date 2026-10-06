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
| Supporting gates | Qualify truthful invalid storage requests before widening the service surface, and correct region-table initialization before expanding mappings. Stage 3 measured less than 1 MiB of low heap headroom after the initial kernel allocation. The two-space contract and diskless machine proof address the first design gate; a booted redesign still needs its own build and guest qualification. | [PD-002](#pd-002-conditional-storage-service-errors), [PD-011](#pd-011-region-first-table-padding), [PD-003](#pd-003-two-space-supervisor-and-shared-application-memory) |

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
names the separate locally tested image. Native TSO24 and broader CMS services
remain separate work.

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
- Status: In progress
- Target: pdos-zarch nucleus, C31 supervisor services and shared AMODE24/31/64 application space
- Observation: The current C32 kernel runs AMODE31 below 16 MiB with a one-MiB linked image slot. Its `PDOS` structure, mostly eager DAT and address-space tables, is 8,802,304 bytes; the initial aligned allocation leaves at most 630,784 bytes before the 12 MiB below-line application pool. Stage 3 exposed an overlap between the CMS24 fixed-origin image and its inherited low stack; the bounded bridge moves that stack to the reserved 15–16 MiB range. The image also covers PLOAD's 1 MiB boot heap. An initial 0.1.1 operator candidate returned from PCOMM but failed in PLOAD's C exit; the repaired z/Architecture kernel reaches its own `0444` wait after DAT is off. A trial with an 8 MiB heap start and 10 MiB application-pool start booted PCOMM but stalled at CMS24 execution; its initial kernel allocation could not fit in that gap, so the trial did not isolate the image-slot limit. Existing 31/64 application loads use above-line storage, but GETMAIN can still use or fall back to `btlmem`. Dropping CMS24 alone would not remove that dependency or bound the kernel heap. Kernel growth requires an independently backed K home for its body and bulky structures, with the real interruption island and 24-bit channel buffers handled separately. Moving the current linked image wholesale would put its 24-bit channel addresses and startup assumptions out of contract.
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
| P0. Freeze the executable contract | Inventory the actual unchanged CMS24/CMS31 and TSO24/TSO31/TSO64 ANY/HIGH packages, load records, entry and return linkage, parameters, lowcore words, line and full-screen terminal calls, storage ranges and command paths. Define the selected service subset, capability queries and return codes; distinguish a loader/service failure from an application's RC. Keep the REXX `ADDRESS` operation and CMS-to-TSO application calls outside the required contract. | An ABI/service table in the architecture guide traces each required native operation to its owning K service or U component, exact unchanged binary fixture and positive/negative control. Record the 24-bit image, stack, heap and largest free low-U interval at named points. No interface is inferred from a synthetic probe alone. |
| P1. Make K state and ownership explicit | Replace program-counter or global-profile inference with an invocation descriptor. Make image pages, U allocations, newly opened file handles, lowcore backing, terminal lease and pending I/O owned by that invocation, except for explicit ABI-defined sharing. Complete interruption nesting, event-driven I/O completion, cancellation and recoverable U-fault unwind without sharing the disk and terminal low-real workspaces. Keep real-frame and DAT-table release checked. | Fresh IPL proves normal return and fault cleanup, including a child fault, failed channel start, late completion and retry. K remains protected, all caller state is restored, and no owned frame, handle or low-U page leaks. Completion events decide success; elapsed time only fails a stalled test. |
| P2. Finish the selected K service surface | Complete the bounded CMS and TSO storage, dataset, file, terminal-input and command services identified in P0. Preserve their distinct ABI layouts, return conventions and encoding. Persist selected output records safely and read them back; validate every full-width U buffer before a C31 service touches it. | Unchanged selected CMS and TSO service probes pass in the diagnostic image with exact output, RC and error behavior. Invalid pointers, short buffers, missing members and failed writes leave no partial state or false success. The 24-bit U placement budget does not shrink because of K code, tables or buffers. |
| P3. Load and call real applications | Use the K placement ledger for fixed and relocatable modules. Implement synchronous CMS-to-CMS and TSO-to-TSO calls with bounded parameters, application RC and separate OS failure status. Restore the caller's AMODE, full registers, lowcore and terminal lease; preserve its file handles and cursors while releasing child-owned handles on return or fault. A fixed-origin nested call may use a checked reversible overlay; ordinary loads preserve every live image. | Native same-personality parent and child pairs run without z/PDOS-specific binary edits and return exact parameters and RCs, including relevant 24/31/64 mode crossings. A colliding or exhausted low interval, or missing real backing, returns a deterministic error without publishing a partial image or moving 24-bit storage above 16 MiB. Co-resident applications remain mapped whenever their required virtual and real storage fits. |
| P4. Deliver the C console and display models | Put the model-aware 3270 and line-device drivers and the 3270 encoder in K C31 C, leaving privileged channel instructions in assembler. Expose checked primary-device capabilities, separate monitor status and logical screen/input requests. Build the reusable U presentation code in C, first for PCOMM and an opt-in app; use explicit ABI adapters for other modes without consuming 24-bit U storage merely for the console. Route unchanged CMS/TSO line calls to the 3270 view and an optional attached Telnet 3215 text monitor. Give that monitor an explicit, exclusive input handoff only at line prompts. Define a versioned append-only transcript with command boundaries and distinct OS and application results. | Qualify model 2 24×80, model 3 32×80, model 4 43×80 and model 5 27×132 with queried or explicit geometry, address encoding, long output, AID/input, header/footer/scroll/entry layout and child-return repaint. On a named guest, qualify simultaneous 3270 display and attached line capture, a line-only primary, input handoff, loss/gap reporting and deterministic event-driven completion. Reject unsupported or ambiguous capabilities and malformed U requests. Full-screen calls on a line-only primary fail clearly; raw 3270 screens are never misreported as complete text. Disk I/O cannot corrupt pending terminal operations. |
| P5. Build a selectable normal image | Integrate PLOAD handover, K64/C31, U PCOMM, native loaders and services into a source-built 3390 image with an explicit successor selection. Retain the current release boot route until the new route passes qualification. Configure the primary terminal and optional text monitor explicitly. Keep K emergency output and shutdown independent of U PCOMM health. | A fresh disk IPL reaches a stable U prompt, launches both CMS and TSO commands, reports each RC, recovers from a failed command, and shuts down without stranded guest or host resources. A second attached terminal can capture the complete text transcript while the 3270 remains usable. Stopped-disk checks verify the image and persistent output after the guest stops. |
| P6. Qualify and select the replacement | Run the unchanged released workload set through the normal successor image: CMS31 RXC/RXAS/RXVM, the bounded CMS24 IO24 path, TSO31 and TSO64 ANY/HIGH compiler/assembler/terminal/file paths, and native TSO24 separately. Exercise nested calls, low-memory exhaustion, fixed collisions, corrupt inputs, bad U pointers, failed I/O, child faults, each terminal model, attached/line-only console modes, long transcripts and monitor disconnect/reconnect across fresh IPLs and repeated start/stop. Compare against the current one-ASCE acceptance without weakening its supported cases. | Record exact binary and image identities, per-case RC/output, low-U free interval and real-frame use, and a repeatable guest completion or failure event. Check that one command produces one ordered text and 3270 result, transcript gaps fail capture qualification, and 24-bit placement is unaffected by the monitor. Watchdogs only detect stalls. Select the successor as default only after all required cases pass on a named machine/toolchain and the operator and architecture guides describe the observed behavior. Publication or release is a separate decision. |

P0 resolves the exact native ABI details before the dependent P2/P3 code is
committed. P1's owner and completion model is a prerequisite for expanding
services or screen output. P3 supplies the invocation and screen-lease stack
used by P4. P5 is an opt-in integration route; P6 changes the default only
after its normal image qualifies. A separate CMS or TSO U ASCE is not part of
this plan unless the recorded shared-U review trigger is met.

The [P0 ABI inventory](architecture/TWO-SPACE-ABI.md) now pins the selected
unchanged inputs and names the untraced forms. The first [P1 invocation
component check](qualification/TWO-SPACE-INVOCATION-2026-10-06.md) proves
token and cleanup ordering on the host and a linked, uncalled Classic C31
module in the diskless fixture. The subsequent [fresh-IPL invocation
gate](qualification/TWO-SPACE-INVOCATION-GATE-2026-10-06.md) executes the
descriptor around unchanged TSO24/31/64 ANY RXVM calls and assigns SVC 120,
IARV64 and TPUT to that active owner. P0 remains incomplete. The private U
fixture still requests entry. A further [fresh-IPL allocation unwind
check](qualification/TWO-SPACE-INVOCATION-REAP-2026-10-06.md) proves that K
releases a live TSO31 page on end. The [CMS invocation
gate](qualification/TWO-SPACE-CMS-INVOCATION-2026-10-06.md) wraps five CMS24
and CMS31 entries, including the overlay child and second relocated RXVM.
Fault-triggered unwind, pending I/O, K-controlled launch and other resource
lifetimes remain open; these checkpoints do not close P1 or qualify P2–P6.

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
- Status: Open
- Target: successor U command processor and opt-in 3270 applications, with a checked K terminal gate
- Observation: The release PCOMM is an application, while the successor currently has only bounded K 3270 reads/writes and selected CMS/TSO line output. The release `22*80` output buffer and the successor's `TSC_MAX_CONSOLE=2048` and `TSC_MAX_INPUT=256` limits are historic source choices. The [architecture decision](architecture/TWO-SPACE-POC.md#architecture-decisions-for-the-first-replacement) places a C31 C device driver and encoder in K and a reusable C presentation library in U. No successor U library, geometry negotiation or general full-screen application contract has been qualified.
- Acceptance: Define a versioned terminal-capability result and bounded logical screen/input requests with one foreground owner, deterministic busy/error results and no unchecked U pointer or U-supplied raw data stream in K. Support checked model 2 24×80, model 3 32×80, model 4 43×80 and model 5 27×132 geometries, including the configured or queried active screen mode, field limits, buffer addressing and sufficiently sized low-real terminal workspaces. Reject unsupported geometry or ambiguous capability data explicitly; keep disk and terminal I/O workspaces separate and completion event-driven. Implement the header, footer, scrollable output and editable entry wrapper in C in U, then use it in PCOMM and one opt-in application. Let the wrapper publish semantic text events to the attached monitor; do not imply that arbitrary raw 3270 streams are readable there. Place the wrapper, PCOMM and their compatible runtime storage above 16 MiB so their own state does not shrink the 24-bit application range. Preserve native line terminal calls for unchanged CMS and TSO programs. Check long output, AID/input handling, encoding, nested-child screen return and redraw, malformed requests, and fresh IPL runs on at least the four named display models without terminal races. Keep K's boot and fault output usable when U cannot run. Qualify non-3270 consoles under [PD-022](#pd-022-attached-operator-line-view-and-transcript), and treat larger extended displays as separate capability qualification.

## PD-022: Attached operator line view and transcript

- Type: improvement
- Status: Open
- Target: successor K terminal service and line backend, U PCOMM command events, and an attached host capture client
- Observation: The release source recognizes 3215 and 3270 console types and PCOMM prints numbered `BEGIN`/`END` markers. Existing exact-output checks depend on a 3270 ScreenTrace. The two-space successor has selected 3270 and native line operations, but no qualified secondary Telnet 3215 device, dual-view fan-out, exclusive input handoff or complete append-only transcript. The [operator line-view decision](architecture/TWO-SPACE-POC.md#operator-line-view-and-attached-transcript) specifies their boundary; it is not an implementation claim. Hercules 3215-C writes to its integrated console and is a separate optional backend, not the clean attached stream.
- Acceptance: Configure one 3270 primary display with a separately attachable Telnet 3215 line monitor, and qualify a line-only primary configuration. Keep unchanged selected CMS/TSO line-call ABIs and RCs; render their text on the scrollable 3270 view and deliver the same ordered logical records to the monitor. Define and version a readable, sequenced transcript grammar for readiness, command ID, input request, output, separate OS status and application RC, screen-only activity and gap/error events. Escape text that resembles framing, preserve defined guest text bytes up to a documented host EBCDIC-to-UTF-8 conversion, and retain output beyond the finite 3270 scroll region. A missing, disconnected or full monitor must not hang normal 3270 work or silently claim a complete transcript; capture qualification fails on a gap or sequence discontinuity. Permit monitor input only through an explicit, exclusive lease at a command or line-input prompt; reject stale or duplicate input and never route monitor input into 3270 AID/field requests. Keep device buffers and queues in K storage, protect U pointers, and drive readiness and I/O completion from guest events rather than sleeps. With named unchanged CMS24/31 and TSO24/31/64 line programs, check output and RC equivalence on 3270 and line-only primary, simultaneous 3270 plus monitor attached before the first command, long output, repeated commands, input prompts, encoding, disconnect/reconnect, failure injection and no loss of 24-bit U capacity. A full-screen request on a line-only primary reports unsupported; a raw full-screen program has no promised text rendering unless it emits semantic events through the wrapper.
