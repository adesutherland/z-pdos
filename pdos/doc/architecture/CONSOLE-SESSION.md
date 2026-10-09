# Console sessions, events and retained presentation

9 October 2026. This describes the authorized PD-024 session and usable-console
increment. Implementation and exact acceptance remain in the
[backlog](../BACKLOG.md#pd-024-complete-3270-family-driver-and-shared-screen-service)
and dated qualification records. The requested review pause after the usable
console preceded the [testable stage 3 increment](../qualification/CONSOLE-STAGE3-2026-10-09.md).
The complete family/provider coverage audit and release remain open.

## Device and application ownership

The common PD-025 engine owns submitted channel operations. A device session
adds a K-issued, non-reused handle, invocation ownership, advertised
capabilities and a geometry generation. U cannot supply an owner token or use
a suspended parent's handle. Session cleanup releases owned records and
quiesces submitted I/O before storage is reused.

Sessions expose the configured primary, optional line monitor and one verified
local 3287 printer endpoint.
Their provider uses the same physical I/O engine as native applications.
The interface separates advertised capabilities, implemented operations and
qualification state. Query replies are retained as binary records; dimensions
are validated before changing the usable geometry. An unsupported query
retains the configured baseline explicitly. Equipment errors remain errors.

An input event contains the actual AID, an optional cursor address and ranges
for every returned field. A short PA/Clear record does not invent a cursor.
Structured replies and arbitrary binary bytes remain records, not console
text. K owns each immutable record until its consumer explicitly acknowledges
it. Failed U-span validation or copying cannot consume the record. Capacity
and queue exhaustion return a defined status before partial publication.

## Presentation and called programs

K retains semantic output history independently of visible cells, panel
composition and the durable transcript. A frozen viewport stays anchored
while output arrives; returning to Latest resumes following output. History
eviction is bounded and visible. Repaints and scroll actions emit no duplicate
semantic text.

The compositor uses character cells, clipping, stacking and a single focus.
Protected output regions never overwrite input fields or move the cursor.
U applications select panel rectangles, titles and actions through checked
requests. A separate C client must use the same API as PCOMM.

PCOMM presents command context separately from called-program output. A
program panel names the active/completed invocation and preserves its result,
including OS errors and unavailable RC. Nested calls retain their own context.
Header, status and key legends derive from actual state. Execution remains
synchronous on one CPU; presentation does not claim multitasking or progress
that the application has not reported.

Colour and highlighting follow advertised device facilities. Monochrome
retains readable grouping, emphasis and focus. The 24-by-80 baseline and
27-by-132 layout use whole character cells and reserve attribute positions;
negotiated geometry adapts the same rules. Drafts remain terminal-owned; submitted input or an explicit checked editor
snapshot provides the bytes. Host-restored drafts carry MDT so Enter submits
the visible text. The narrow shell summary shortens complete semantic records
visibly; full text remains in history.

## Teletype and LLM use

The line primary and optional monitor expose ordered semantic text, stable
command boundaries and distinct OS/application results. They carry no panel
borders, colour escapes or repaint traffic. Raw screens, truncation and
disconnect/reconnect produce explicit gap/status records.

There is one input owner. A monitor defaults to output capture; input requires
explicit interactive configuration and selection for a prompt. Line clients can execute commands and answer unchanged
native program prompts without depending on screen coordinates or timing.
Presentation changes must preserve exact empty, padded and accented input,
and the existing native command and transcript contracts.

The [Workbench checkpoint](../qualification/CONSOLE-WORKBENCH-2026-10-09.md)
records the live UI sweep, repaired interactions and provider/emulator limits.
