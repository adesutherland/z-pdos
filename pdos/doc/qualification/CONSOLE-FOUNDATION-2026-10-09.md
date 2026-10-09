# 0.2.1 console and K/U foundation review

9 October 2026. The reviewed foundation passes its selected one-CPU guest
matrix. The complete 0.2.1 driver and presentation work remains in progress
under [PD-024](../BACKLOG.md#pd-024-complete-3270-family-driver-and-shared-screen-service)
and [PD-025](../BACKLOG.md#pd-025-k-and-u-service-communication-foundation).
There has been no release or managed-image adoption.

The later [PD-025 acceptance](IO-FOUNDATION-2026-10-09.md) completes the shared
I/O and normal-IPL performance work described as outstanding in this earlier
checkpoint. Its frozen candidate and 537-assertion matrix supersede this
checkpoint for the shared foundation; PD-024 remains in progress.

I treat communication between K and U as a prerequisite for the console,
panels and complete 3270-family interface. A request must finish through a
checked return or an actual device completion. Elapsed time must never make
an operation successful.

## Main findings and repairs

The old channel wait helper set the wrong PSW byte for WAIT. Its intended
wait was therefore not the architectural enabled wait. Both wake handlers
also cleared that wrong byte, and external wake could return to K service C
with I/O enabled. The repaired paths set and clear PSW bit 14 correctly,
restore K's I/O-masked status-processing discipline and preserve interrupted
U PSWs. A planted wait and a deliberately broken external-return mask now
exercise those rules rather than accepting an unexecuted observation.

Console submission could acknowledge status before SSCH and discard input
attention. Output and read submission now route pending status explicitly;
combined attention and completion are kept distinct. Primary attention is
retained during a monitor wait. Its owner is the lease that unlocked the
input field, including across nested calls. A child cannot take a parent's
pending record. Unowned input is explicitly quarantined. Line Read Inquiry
uses its own input rules rather than 3270 field/AID handling.

The older asynchronous diagnostic controls and v1 console shared the physical
workspace with different admission rules. Those test controls are now confined
to the legacy console profile. V1 rejects them before they can start or clear
I/O; native CMS/TSO interfaces remain available. Both writers guard pending
legacy reads before copying, and legacy cancellation shares the quarantine
flag. The native early-input probe attempts the old start and clear controls
while the parent's record is pending and verifies that both are rejected.

Failed output, sense and cancelled reads must observe channel-clear
completion before their workspace is reused. Failed clear leaves the
workspace quarantined. The native raw receive path now uses the advertised
16-KiB capacity instead of a hidden 4-KiB limit.

Large K/U spans are preflighted as a whole and copied page by page. Later
permission failures cannot publish an earlier part of the destination. The
existing 256-byte gate remains compatible. An 8-KiB, two-page control makes
four permission callbacks. The normal one-CPU profile keeps the U mapping
stable throughout preflight and copy; this is not an SMP contract.

The console retains confirmed protected cells and sends ordinary Write
deltas. It commits its cache after checked completion and invalidates it
after failed I/O or raw activity. Input clearing, keyboard restoration and
cursor positioning are separate from background output.

## Qualification

The machine is standard z/Architecture/ESAME, one model-2064 CPU and 256 MiB
real storage, with fresh disposable 3390 media. K uses the GNU-built 64-bit
nucleus and Classic C31 services; U uses native C31 clients, an unchanged
HIGH cREXX child and a U64 capability probe. Source, tool, image and receipt
identities are in [the machine-readable record](CONSOLE-FOUNDATION-2026-10-09.json).

| Selected guest gate | Checks | Result |
| --- | ---: | --- |
| Final K/U foundation, context, permissions, faults, cancellation and negative controls | 106 | Pass |
| Model 5 raw input: 1,386 fields, every address and byte, 5,547-byte record | 19 | Pass |
| Model 2 with monitor input handoff | 18 | Pass |
| Final model 5 with early 256-byte input and rejected legacy start/clear | 20 | Pass |
| Final primary 3215 with early 148-byte input and rejected legacy start/clear | 18 | Pass |

The total is 181 checks: 144 freshly exercised on the final candidate and
37 reused after dependency review. Raw transfer, handoff, geometry and channel
support retain their tested implementation. The final changes strengthen
common reservation/quarantine rules and segregate legacy diagnostic controls.
Fresh final-candidate gates exercise both console classes and the old
diagnostic profile, including rejection of legacy start/clear with input pending.

Terminal traces independently show 79 ordinary body updates after the first
output frame, without erase, cursor insertion, keyboard restoration or MDT
reset. The early-input trace shows the full Enter record sent before the
HIGH child starts and before the parent's Read Modified. Native execution
checks its exact contents and successful return. Durable transcript
readback and disk content outside the transcript store pass.

C89 ASan/UBSan checks cover the family codecs, console and channel. The
review checks extract the actual endpoint functions for 38 deterministic
submission, status, ownership, sense, wait and cancellation controls. An
independent field-cell model also checks full-width rendering, cursor and
attribute preservation, failed-output cache handling and recovery on models
2–5. Injecting a cursor order into a protected delta fails that control.

The final diagnostic console runs measure 1,000 checked U64 capability calls
using a buffer above 4 GiB. Mean cost is 55.846 microseconds on the line
profile and 55.725 microseconds on model 5, including loop overhead under
Hercules emulation. STCK measures duration; return values and a fixed
operation count decide correctness. Its conversion follows the TOD clock
definition in [IBM Principles of Operation](https://www.ibm.com/docs/en/module_1678991624569/pdf/SA22-7832-14.pdf?cp=HW11W).

These are diagnostic-console measurements. Normal IPL additionally calls
`service_observe` after each service and rescans storage metadata. The
measurements do not establish normal-image latency or isolated native bulk
copy time. Those costs remain part of PD-025 before release selection.

## Remaining work and order

First measure normal-IPL request and bulk-copy costs, account for storage
observations, and keep unnecessary bookkeeping out of the service hot path.
Then build the checked K device-session and immutable-record/event interface,
including complete chained binary transfers, capability queries, device
exceptions and printer endpoints. Geometry changes and raw mode transitions
must use monotonic generations before U clients rely on stale-request
rejection.

The retained history, clipping, stacking, focus and panel compositor follow
that foundation. U chooses application layouts and actions; PCOMM remains a
consumer. The [complete family contract](../architecture/CONSOLE-STANDARD.md)
continues to include partitions, graphics, programmed symbols, DBCS, text
facilities and printers. Protocol framing is not device conformance.
Physical-device execution and unsupported emulator facilities remain
explicitly unqualified. Full unchanged 0.2.0 workload regression is still
required before selecting a 0.2.1 release candidate.
