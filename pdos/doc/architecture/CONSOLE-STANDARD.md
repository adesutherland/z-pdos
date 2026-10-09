# 3270 family driver contract

8 October 2026. Accepted development scope for 0.2.1; implementation and
qualification are in progress under [PD-024](../BACKLOG.md#pd-024-complete-3270-family-driver-and-shared-screen-service).

I want the reusable driver to support the complete 3270 family. PCOMM uses
only the facilities its command interface needs. The driver's scope includes
display and printer facilities, graphics, programmed symbols, DBCS, negotiated
geometry and hardware partitions. A missing emulator feature is a device
capability result; it is not a reason to omit its standard interface from PDOS.

## Sources and interpretation

The primary data-stream source is IBM *3270 Information Display System Data
Stream Programmer's Reference*, GA23-0059-07, eighth edition, June 1992,
[public reference copy](https://ruelgnoj.co.uk/3270/3270.pdf). Its chapter and
section identifiers own protocol facts. Appendices C and E distinguish
addressing and the SAA baseline. The host connection reference is
[RFC 1576](https://www.rfc-editor.org/rfc/rfc1576.html) and
[RFC 2355](https://www.rfc-editor.org/rfc/rfc2355.html).

The driver provides checked requests, serialization, binary-preserving
transfer, capability queries, results and recovery. A terminal or printer
executes its own rendering, symbol loading and print operations. PDOS does
not need to emulate that device inside K to drive it. Applications may use
typed C helpers or complete standard data streams through a checked service.
Optional device payloads must not pass through text conversion or the console
line model.

## Coverage contract

| Facility | Owning reference | Driver requirement |
| --- | --- | --- |
| Basic commands and write controls | Chapters 3 and 10 | Cover every command, keyboard/MDT action, result and native-channel mapping. |
| Basic and extended orders | Chapter 4 | Preserve field, character and graphic-escape sequences; bound every operand. |
| Geometry and addressing | Chapter 6 and Appendix C | Query default/alternate/usable dimensions; handle coded 12-bit, binary 14-bit and explicit-partition 16-bit addressing. |
| Input and selection | Chapters 3, 6 and 7 | Return AID, available cursor, all fields, reply modes and special input without losing bytes. |
| Attributes and character sets | Chapters 4, 6 and 12 | Expose queried attributes, programmed symbols and DBCS; preserve their native representation. |
| Partitions and viewports | Chapters 2 and 5 | Support the complete partition lifecycle, window origins, activation, reads and reply modes. |
| Graphics, images and object data | Chapters 5 and 6 | Carry standard controls and binary payloads without treating them as console text. |
| Printers, SCS and IPDS selection | Chapters 5, 6 and 8 | Separate printer endpoints, formatting, completion, errors and recovery from interactive input. |
| Format storage and text facilities | Chapters 5, 6 and 13 | Expose storage, presentation and text operations through the shared framed service. |
| Routing, auxiliary devices and chaining | Chapters 5, 6 and 11 | Retain routes, opaque replies, sequencing and chained transfer semantics. |
| Query and exception records | Chapters 5 and 6 | Retain every reply and status record, including facilities not used by PCOMM. |
| Connection and native transport | Chapters 9 and 10; RFCs above | Keep device/data-stream services separate from the selected link provider; report missing provider capabilities explicitly. |

Every row needs an implementation entry and independent positive, malformed,
unsupported-device and recovery controls. A small model-2 example cannot close
the whole table. Native local-channel qualification, host-protocol checks and
terminal/printer execution are different results. A successful transfer does
not establish that a device printed a page or rendered an object correctly.

## Kernel and application responsibilities

K owns channel operations, buffers, device sessions, leases, queued events,
checked full-width U transfers and completion. Portable C owns the protocol;
assembler handles privileged operations and interruption linkage. Keep raw
device streams and printer jobs independent of the primary text console.
Only K-owned immutable records may back decoded field views across a call.

K also owns the retained console display, bounded scrollback and panel
composition. The U toolkit chooses layouts and application controls. PCOMM
parses commands and uses this toolkit; it is not the driver. Native CMS/TSO
line and raw-screen entry points retain their existing ABI and return rules.
Raw device activity invalidates the corresponding retained display and emits
a non-text transcript event instead of fabricated line output.

The application interface must distinguish device-advertised facilities,
implemented PDOS operations and qualification state. Capability queries are
not conformance certificates. Unsupported functions return a defined result
before publishing partial state. Unknown structured fields remain available
as bounded raw records; interpreting their payload requires an owning codec.

Cursor-aware selection is part of the contract. Keyboard selection and an
emulator's cursor-plus-Enter mouse mapping use the same application action.
Cursor Select and selector-pen semantics are included in driver coverage.
Host handling of continuous mouse motion requires a protocol that supplies
such events; the standard driver must not fabricate them.

Negotiated geometry is part of the initial contract. Live geometry changes
must follow actual provider notifications or requery, retain drafts/history,
change the geometry generation and reject stale requests. Connection-time
negotiation remains useful on clients that require reconnection to change
their size. A GUI font/window resize does not itself establish a new host
presentation space.

## First implementation checkpoint

The K/U communication path is a prerequisite for the complete driver. U calls
the checked synchronous SVC boundary once per request/transaction. K validates
full-width addresses and complete spans before changing state or publishing a
response. The one CPU keeps the U mapping stable during preflight and copy;
this is not an SMP contract. Large transfers use page-wise span services while
the existing 256-byte gate remains compatible.

Device waits consume actual status and operation ownership. Service C runs
with I/O masked between its status check and the atomic enabled WAIT. Both I/O
and external wakes restore that K discipline; interrupted U PSWs retain their
original mask. Timer/watchdog expiry is an error, never a completion condition.

The [console foundation review](../qualification/CONSOLE-FOUNDATION-2026-10-09.md)
records retained-stream and input observations. The subsequent
[PD-025 acceptance](../qualification/IO-FOUNDATION-2026-10-09.md) completes
the shared I/O and normal-IPL performance checkpoint. Attention belongs to the lease that
unlocked its input field, rather than the application active when status is
consumed. Line Read Inquiry remains separate from that field/AID policy.

The pre-versioned asynchronous console diagnostic controls use the legacy
console profile. The v1 profile rejects their start, poll, clear, cancellation
and related test controls before touching its workspace. Both writers reject
a pending legacy read before copying, and both cancellation paths share the
workspace quarantine flag. Native CMS/TSO calls and v1 presentation requests
keep their existing interfaces. The complete driver will use a common checked
session/event interface for asynchronous device work.

Pending attention must survive protected output and prompt preparation,
including attention combined with write completion. The output endpoint must
not discard status before SSCH. Failed output must observe channel clear before
reusing its workspace; a failed clear quarantines that workspace. Review and
deterministic controls for these cases precede frozen guest qualification.

`twospace_3270.c` supplies address codecs, basic command mappings, structured
field framing/building, complete-record input decoding and retained query
metadata. The raw console validator consumes the common order/framing layer;
native channel selection now includes the remaining basic command opcodes.
Structured object and printer data stay opaque in this layer.

This checkpoint is protocol implementation, not complete driver conformance.
The normal fixed console, checked device-session API, automatic query policy,
printer endpoints, retained rendering and panel integration still need their
own implementation and frozen guest acceptance. The backlog owns delivery;
this document owns the accepted interface and coverage requirements.
