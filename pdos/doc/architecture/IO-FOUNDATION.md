# Shared K/U and channel I/O foundation

PD-025 owns this one-CPU contract and is complete in the
[9 October local qualification](../qualification/IO-FOUNDATION-2026-10-09.md).
Every maintained K physical-I/O provider uses the same operation engine in
`twospace_io.c` through the native adapter in `twospace_io.inc`. Console,
disk, tape, native files, loader reads and durable store callbacks select
device semantics above that engine. The explicitly absent-subchannel assembly
probe remains an isolated negative fixture. The earlier IPL/handover driver
runs before the K/U foundation is established.

U requests enter the common SVC/context boundary. Full-width U spans are
validated completely before publication and copied page by page. Native
24/31-bit adapters retain their address limits; their large file transfers
use the same span implementation. The small 256-byte interface remains
compatible for parameter lists.

## Operation lifetime

The supplied K pool binds each low-real workspace once. A live request holds
its workspace and subchannel, owner and monotonically allocated ticket.
Another workspace cannot start on that subchannel while the binding is live.
Builders reject occupied or quarantined storage before clearing or writing
the channel program. Output data is copied only after a builder succeeds.

A request progresses through prepared, active and done states. Only accepted
SSCH enters active. Pending status before submission is retained as an
unsolicited notification and routed separately; it cannot complete the new
request. The console can defer prompt preparation when an actual input
notification is already owned by that field.

TSCH is performed against the bound subchannel. Primary and secondary status
are combined without substituting secondary CPA/count fields for the original
transfer result. Ending status must identify the expected final CCW and a
valid residual. Attention remains a separate notification even when combined
with completion. The device validator then decides record lengths, EOF,
printer/terminal controls and other protocol meanings.

Owner/ticket mismatches reject stale completion and cancellation attempts.
Asynchronous reads retain completed buffers until their consumer releases
them. Synchronous callback data remains valid until the next accepted K
operation, matching the existing dataset callback contract.

An error after accepted submission must quiesce the operation before reuse.
Clear confirmation is checked by both the native provider and common engine.
Failed clear leaves the binding, occupied flag and quarantine intact. The
failing operation status remains in its descriptor while IRB carries the
clear receipt. Quiescing a notification-only legacy read follows the same
binding and clear rules. No caller can treat a timeout as successful I/O.

## Waiting and performance

K service C checks status with I/O masked. The assembly wait atomically enables
interruptions and WAIT. Its wake-notification entry leaves TSCH to the common
engine and preserves the operation's original watchdog deadline across
unrelated wakes. Interactive input may remain idle indefinitely. Watchdogs
only report stalled submitted work.

Storage observations use real and virtual ledger generations. Unchanged
services update invocation depth without rescanning the ledgers. Generations
saturate rather than wrap; saturated ledgers force a scan.

Equal-size keyless CKD reads can group up to three records from one track in one
channel program. The chain seeks once, performs bounded search/TIC/read
sequences and validates each count/header and exact length before publishing
the batch. Read-ahead uses separate K storage, does not cross a track or the
caller's checked extent, and is invalidated by writes, attention and failed
I/O. Last-track, short, keyed, missing and incomplete batches use the
existing single-record path. An actual No Record Found sense condition can
advance to the next checked track; equipment or other failure cannot.

The normal-IPL benchmark owns a U64 64-KiB allocation, verifies fixed request
and channel-submission counts, compares grouped records with single reads,
and measures page-wise copies. Clock observations are cost measurements,
never readiness or correctness tests. Qualification records distinguish
tested emulator behavior from physical-device execution.

## Scope

The supplied pool has eight bindings. Monotonic 32-bit tickets fail closed on
exhaustion rather than wrap. Full-span copying is bounded to 32 MiB; native
file/store limits and 24/31-bit address limits remain explicit.

One CPU keeps U mappings stable during preflight and copy. This is not an SMP
memory or scheduling contract. The general session, event API and complete
3270-family facilities remain PD-024 work and must consume this foundation.
Architecture facts follow IBM *z/Architecture Principles of Operation*,
command-mode subchannel status and I/O instructions, and the CKD reference
formats. Exact source, tools, native inputs and positive/negative results
belong in the dated PD-025 qualification record.
