# Retained console and Workbench ABI

9 October 2026 PD-024 retained console and testable stage 3 development
candidate. Dated qualification owns accepted profiles; broader 3270-family
facilities and release remain open.

SVC 200 uses binary big-endian words, explicit IBM1047 text and K-issued
invocation ownership. Headers and complete U spans are checked before
publication. Status 8 is an invalid request/span/handle, 16 is busy or stale
geometry, 20 is unsupported, and 12 is a physical I/O failure. Timers never
confer readiness or transfer an input lease.

## Sessions

Operations 64–71 are Open, Close, Capabilities, Write, Read, Event, Acknowledge
and Query. Their 64-byte request is:

| Offset | Meaning |
| --- | --- |
| 0, 4 | Version 1, size 64 |
| 8, 12 | K session handle, geometry generation |
| 16, 20 | Flags, architected command/AID |
| 24, 28 | Fragment offset, payload capacity/length |
| 32, 36 | Full-width U payload pointer |
| 40, 44 | Immutable event sequence high/low |
| 48, 52 | Actual length, event type |
| 56, 60 | Cursor address, field count |

Open accepts address 0, the configured primary/monitor or the verified local
printer endpoint. Non-reused handles
belong to the active invocation. Close discards queued records; submitted or
quarantined I/O is quiesced before storage reuse.

Read accepts F2 Read Buffer, F6 Read Modified or 6E Read Modified All. Capacity
0 returns metadata while retaining the complete record. Event copies the head;
only Acknowledge with its exact sequence consumes it. Cursor-valid is output
flag 2; clear output flags before the next operation. Short PA/Clear records
retain an absent cursor. Structured replies, buffers and malformed records
retain their bytes; portable decoders expose field ranges separately.

Write stages contiguous fragments under one command/generation; flag 1 seals
the final fragment. Protocol validation precedes one data-chained submission.
A busy sealed operation can be retried at its final offset with zero bytes and
flag 1. Close abandons rejected sealed records. The staging limits are 131072 output bytes,
65535 input bytes, and 256 text bytes for a line session. These are not provider
transfer guarantees. Hercules completes a 16384-byte record; a 131072-byte
write completes only 32768 bytes and returns status 12, then recovers through
owned Close. Saturated input is event type 7 (truncated), retaining the prefix
and an explicit gap instead of claiming a complete record.

Capabilities copies 128 bytes: legacy geometry at 0, provider operation mask
at 64, qualification mask at 68, record/transfer limits at 72/76, advertised
colour mappings at 80 and Query Reply bitmap at 96. The qualification mask is
reserved at zero; the dated receipt gives actual qualification. Advertisement
is distinct from implemented and qualified operations. Feature bit 2048 means
Read Buffer is available on the selected display provider. The exact captured
DX3270 1.7.5 legacy reply supplies validated geometry and palette; original
bytes remain immutable. Read Buffer returns 20 for this profile. Its Workbench
snapshot uses responding Read Modified and an MDT editor. The standard decoder
remains strict. Hercules rejects local Read Modified All (0E); framing support
does not qualify that provider operation.

## Workbench and input

Operations 96–103 use 32-byte requests. Open (96, flag 0) carries a title at
length 12 and pointer 16/20. Close uses flag 1. A called program attaches to the
existing presentation and cannot replace its parent's controller.

Job Begin (100, flag 0) carries native command text. Flag 3 declares the PCOMM
shell controller. Flag 2 marks an external
call with no payload. Job End (101) supplies RC-valid at 8, RC at 12 and OS status
at 24; external results use K's actual result. Status (98, flag 1) carries volume
context. Flag 0 returns generation, command number, phase, input source and
successful flush/erase counts. Flag 2 supplies continuation prefix length at
12 (0–198) and pending boolean at 24. Control Text (103) selects ordinary output
(flag 0), controller text excluded from the program pane (1), or shell-history
text (2). Each retains ordered semantic teletype/transcript delivery.
Input (99, flag 0 primary or 1 monitor) selects
one source at a completed boundary; called-program changes restore on unwind.
TTY Mode (102) accepts plain mode 0; other modes return 20.

Line Key (3) returns bytes plus AID; PCOMM owns PF5/PF6 recall policy.
Workbench actions are consumed by the controller; unassigned AIDs, including
PF3/PF4 and PA, reach the semantic key caller. Raw session input retains its
complete AID and cursor contract.
Existing Line Read (2) returns Enter submissions. Line Set (4) explicitly replaces the
dock after an AID: cursor offset 8, length/pointer 12/16/20, generation 28.
Pending attention prevents replacement. Ordinary output, scroll and focus
updates never clear the dock, reset its MDT or move its cursor. Injected and
restored drafts set MDT so Enter submits the visible text. Recall or Clear
cancels the joined prefix. Blank/refused input cannot publish an execution
result for a command that did not run.
An Enter submission clears the field and inserts the cursor at its first cell
in the same locked write, before command execution produces any output.

History is independent of viewport width. Frozen views retain their anchor;
eviction is visible. Workbench separates program output and shell context,
side by side on wide screens. PF1 help, PF2 editor, PF7/PF8 page, PF9 Latest,
PF10 toggles output and shell focus, including their focus bar, at actual input
boundaries. Panel selection with cursor plus Enter is idempotent. Workbench
does not assign PF11/PF12; the driver still delivers their raw AIDs to session
clients. Input-source selection uses operation99 or PCOMM's explicit
`CONSOLE INPUT PRIMARY|MONITOR` command. Modified
input takes priority over cursor selection. A monitor read owns its reply and
does not receive screen keystrokes; screen PF legends reflect that state.

## Stage 3 monitor, text and printer facilities

The 32-byte console configuration uses word 24 mask `0x00000001` (bit 0)
to declare an interactive monitor consumer. Other bits remain reserved; zero selects output capture.
Source selection and raw monitor-session reads return20 when input is not
configured, and16 when the endpoint is offline or stale. Capabilities omit
input/handoff when the monitor is capture-only. The line primary remains an
input endpoint independently of this flag.

While an interactive monitor read is outstanding, primary-terminal attention
belongs to that blocked invocation. K cancels and quiesces the exact operation
before changing input. Line Read restarts on the primary; a raw monitor-session
Read returns16 without publishing a record. A failed monitor operation can
also return line input to the primary after successful quiescence. A failed
clear retains quarantine and returns an error. Late monitor input cannot be
reused until the provider's actual reconnect notification clears its stale
state. Clear is the advertised primary escape. Timers never perform handoff.

`T27REPLY` builds tested implicit-partition field, extended-field and character
reply modes. Character requests admit highlighting, foreground and charset
attributes. Returned records remain binary and immutable. Closing a raw
primary presentation restores field reply mode on a provider advertising it
before restoring the Workbench, preserving native line input.

Device Open also admits one local3287 endpoint identified by Sense ID. It has
device class3, feature4096, implemented operation mask0x0f, no input record
capacity and a separate K channel/workspace. It uses the same invocation
ownership, staged transfer and checked-completion engine as other I/O.
`T27PRINT` builds unformatted native text with Start Print and explicit End
Media. Read, Query and unimplemented printer formats return unsupported.
Channel completion establishes transfer; actual print-command capture is a
separate QA result. On the tested non-TN3270E pr3287 route, End Media flushes
a page but does not close the print-command stream; QA closes the actual
printer connection after the completed client result to inspect its output.
Protocol job-end, formatted 3287, SCS/IPDS and additional printer endpoints
remain backlog facilities.

## Read-only DASD allocation and DISKMAP

Operation36 uses a 64-byte version1 request. Input word8 is the mounted-volume
index, word12 is map capacity (0 or1500), and words56/60 are its full U pointer.
The complete writable header and optional map are checked before disk I/O.
On success, word8 becomes the mounted-volume count and word12 its device
address. Cylinders/heads/tracks are at16/20/24, reserved/allocated/free tracks
at28/32/36, largest unallocated track run at40, dataset count at44 and the
six IBM1047 volume characters at48. Map bits1/2 mean reserved/allocated;
reserved extents are counted once in the totals.

This implementation admits the maintained 100-cylinder 3390 layout and its
two-cylinder VTOC, with up to three direct dataset extents. It reads the
declared DSCB count per track; failed reads fail the snapshot. Other layouts
and extent chains return unsupported. These are allocation counts rather
than logical content bytes or compressed host-file sizes.

The optional U31 `DISKMAP` application consumes this API and the existing
panel/layout API. It presents a capacity bar, cylinder map and actual totals,
with Enter refresh, N next registered volume and Q return. Its plain line view
uses the same figures and labels. No periodic polling or timers drive it.

## C panel requests

Operations 128–134 are Create, Update, Text, Show, Hide, Close and Commit.
The 64-byte request has version/size at 0/4, handle 8, expected geometry 12,
reserved flags 0 at 16, row/column/height/width at 20/24/28/32, role 36,
title length/pointer 40/44/48 and text length/pointer 52/56/60.

Coordinates are zero-based. Each panel reserves its preceding cell as a
protected attribute. Roles are normal 0, header 1, shell 2, action 3, error 4
and focus 5. Text is native row-major cells. Panels clip to the output body;
they cannot overlap input, status or keys. Later slots overlay earlier slots.

Ordinary semantic output is retained while primary attention is pending;
physical rendering catches up after the owning input record is consumed.
A completed full presentation establishes the active invocation's editable-field
owner, including when WCC preserves keyboard state. Deltas preserve that owner.
An explicit Commit returns busy 16 in that state and keeps its staged data.

Changes stage until Commit with handle 0. It composes live panels and the
current invocation's staged changes in one physical update. Checked completion
publishes them as live; other owners' staged changes cannot be committed.
Close removes an owned panel; Commit or the next output repaints the exposed
history. Sixteen non-reused handles are reaped at exit. The independent C
qualification client uses this API without PCOMM. Help temporarily hides
presentation layers without changing their live state. Cell painting does not
create semantic output: applications use Line Write for text belonging in the
line/transcript interface.

## Storage and teletype

A 6 MiB K arena contains a 4 MiB history ring, 16384 record descriptors,
current/confirmed surfaces, bounded encoding scratch and staged/live panel text.
Device sessions have separate storage. History eviction does not lose live
teletype delivery. A full 2 MiB journal marks a capture gap and output continues.

Line primary and monitor carry ordered text, PCOMM's existing command/result
markers and prompts. Screen pixels, colour escapes and repaints never enter
this stream. Hercules uses Latin-1/IBM1047 on the wire; it is not UTF-8. Binary
transcripts retain explicit encoding, invocation boundaries, results and gaps.
