# z/PDOS 0.2.1 console design review

9 October 2026. Three proposals for the retained-history and usable-console checkpoint. The boards are design artifacts, not screenshots of implemented or qualified behaviour. Sample CMS31 RXVM output and return values illustrate a completed call; they are not a qualification receipt. No product source was changed.

I recommend **01 Workbench**. Program output has a clear home, and the shell retains a useful record of the commands that produced it. At 80 columns I use a vertical arrangement; at 132 columns we gain a real shell sidebar without shrinking the command editor. Ledger suits repeated command comparison. Focus suits longer output and has the least flexible split.

The main designs deliberately use standard 3270 colour. The monochrome views show compatibility with terminals that do not advertise colour. They are not the proposed default on a colour terminal.

## Read the boards

Open `index.html` for all three options. Each standalone `*-board.svg` contains a 24x80 colour preview, a 24x80 monochrome proof, a 27x132 colour preview and the ordered teletype projection. Matching `.txt` files preserve every row and column, including trailing spaces. `.json` files expose every cell, display role and illustrated cursor position. The cursor outline represents the emulator's cursor, not an extra screen character.

All terminal characters are ASCII. Typography outside the terminal frame belongs to the review board only. Inside each frame, each glyph is placed at the centre of one fixed cell. No gradient, proportional terminal typography, Unicode border, animation, spinner or fake progress is proposed.

## Exact layout measurements

Coordinates below are one-based presentation-space rows and columns. Column 1 is normally reserved for a non-display field attribute, and the final column is normally blank. Input continuation rows are the explicit exception: a 3270 field wraps through those cells.

| Option | Model 2: 24x80 | Model 5: 27x132 | Main tradeoff |
| --- | --- | --- | --- |
| 01 Workbench | Header/status rows 1-2; program heading 3; output 4-15 (12 rows, 78 columns); output position 16; shell heading 17; recent command/result 18-19 | Header/status 1-2; headings 3; program output 4-21, columns 2-86 (18x85); shell history 4-21, columns 90-131 (18x42); position 22; command heading 23 | Good output capacity and command context; 80-column view shows only the latest shell command pair |
| 02 Ledger | Header/status 1-2; shell heading 3; history 4-9 (6x78); program heading 10; output 11-17 (7x78); position 18; command heading 19 | Header/status 1-2; headings 3; shell history 4-22, columns 2-45 (19x44); program output 4-22, columns 49-130 (19x82); command heading 23 | More command context; smallest program viewport at 80 columns |
| 03 Focus | Header/status 1-2; frame/title 3; output 4-16, columns 4-77 (13x74); frame/position 17; shell summary 18; command heading 19 | Header/status 1-2; frame/title 3; output 4-20, columns 4-129 (17x126); frame/position 21; shell summary 22; command heading 23 | Widest output, explicit visual enclosure; a compact shell summary replaces a persistent shell history viewport |

All options share the same command dock and controls:

- Model 2: prompt at row 20, column 2; the input attribute occupies column 4; editable data starts at column 5, spans the rest of row 20 and all of row 21, and has 156 cells. The next protected attribute is at row 22, column 1.
- Model 5: prompt at row 24, column 2; input attribute at column 4; editable data starts at column 5 and has 256 cells, ending at row 25, column 128. A protected end attribute occupies row 25, column 129. The rest of row 25 is protected.
- Model 2 status is row 22 and PF labels are rows 23-24. Model 5 status is row 26 and PF labels are row 27.
- **PF2 Edit** is a submitted-AID action that expands the command editor to a dedicated 256-cell field on both geometries. It preserves the draft and leaves a return action visible. The normal 80-column dock does not pretend to expose 256 editable cells. Existing PCOMM `&`/`MORE>` command-fragment semantics remain independently available.
- If a captured draft will not fit after geometry changes, the UI must select the expanded editor before redraw. It must not silently truncate or place unseen editable bytes beyond a smaller field.

A deliberate split at 100 columns chooses the two-column layout only when the shell can retain at least 40 columns and output at least 50 columns. Below 100 columns, Workbench and Ledger stack. Additional rows enlarge the output/history viewport; the command/status/PF dock stays anchored to the bottom. Model 3/4 and negotiated oversize follow the same measured policy later. These boards qualify neither live geometry notification nor oversize support.

## Colour and monochrome

The colour names are standard 3270 values; the RGB values below are illustrative emulator palette approximations, not a request for arbitrary host-side colours. The emulator controls the actual palette and background. Boards use black for the presentation-space background.

| Role | Workbench | Ledger | Focus |
| --- | --- | --- | --- |
| Restrained header | Turquoise | White | Turquoise |
| Program heading/frame | Turquoise | Yellow | Turquoise |
| Program text | White | Green | Green |
| Shell context | Green | Yellow command / white result | White summary |
| Active command / PF labels | Yellow | Yellow | Yellow |
| Status | White | White | White |
| Error state | Red, with explicit OS error text | Red, with explicit OS error text | Red, with explicit OS error text |

Preview palette: green `#58e878`, turquoise `#62e2e2`, white `#f2f2f2`, yellow `#f4e76b`, red `#ff6262`. Red is reserved for a present fault; an ordinary nonzero application RC uses explicit text and yellow. A normal completion is labelled `COMPLETED OS=OK RC=0`; colour does not establish successful execution.

In monochrome, protected headings, status and actions use basic bright intensity; body text uses normal intensity. State words, field names, separators and row positions carry meaning without colour. No dim-text facility is required. The protected/plain-field arrangement can express the fallback without extended attributes. Optional character colour orders do not consume presentation cells; basic field attributes do.

Workbench and Ledger fit comfortably within the existing 64-field envelope. Focus needs 62 fields at model 5 if its bright border, normal body and bright border are three basic fields per interior row. Its blank columns 3 and 130 hold the body/right-border attributes; do not place output into them. If the renderer cannot safely provide that topology, keep the whole frame/body normal intensity or prefer Workbench. The Focus frame is a costed alternative, not decorative free space.

The current `TTCENCODE` accepts only basic input/protected/bright attributes. Colour therefore requires the checked optional-attribute rendering path and advertised capability handling; these boards do not imply that path already exists. Do not infer colour support from a terminal model number or emulator brand.

## Keyboard, focus and input ownership

The footer shows actions available in the current context. The preview is a completed call with PCOMM ready for its next command.

| Submitted key | Meaning in the preview |
| --- | --- |
| Enter | Capture all modified command fields, then submit to PCOMM |
| PF1 | Show contextual help; the help view offers a return action |
| PF2 | Expand the command editor; capture the draft before changing fields |
| PF7 / PF8 | Page the focused retained output/history viewport up/down |
| PF9 | Move the focused viewport to Latest |
| PF10 | Select the called-program output viewport |
| PF11 | Select the shell history/command context |
| PF12 | Open physical input-source selection: PRIMARY or MONITOR; show the current consumer and pending selection |

Focus is a viewport/action selection, not permission to take input from another invocation. A cursor-plus-Enter mapping can select a protected panel heading only when submitted at an actual read boundary and when that action is visible. An Enter with a modified draft takes the command action instead. This design relies on AID, cursor and modified fields; it assumes no per-keystroke or continuous mouse callbacks. PA and unsupported keys receive a defined no-action/help result; this proposal assigns no cancellation policy to them.

The status names **physical source and consuming invocation separately**. `Input: PRIMARY / PCOMM` means the next/current line read belongs to PCOMM on the primary terminal. While a called program owns a line read, show `Input: PRIMARY / RXVM`, `Waiting for program input`, and `RC=PENDING`. PF12 selects the source for the next eligible read; it does not steal an outstanding lease. Monitor input is useful for LLM operation. Lease transfer must follow the accepted service contract, not a purely visual focus change.

No generic Exit, Stop or Cancel action is shown. Those need an explicit policy owned by PCOMM or the application. The system uses synchronous execution on one CPU: an `ACTIVE` state reports the invocation state, with OS/RC pending. It is not a promise that PF actions or a shell command can execute while the child computes. Input/actions become serviceable at an actual I/O/read boundary.

## Retention and draft safety

K owns bounded chronological history, immutable event records, viewport position and composition. U chooses this layout, labels and application actions. C supplies the protocol/UI; assembler handles privileged linkage. A program-output panel is a view of records attributed to the called invocation. The shell panel is a view of shell command/result records. Neither is a second copy of the transcript or a separate execution engine.

While a live unsubmitted command is terminal-owned, protected output updates may change only protected cells. They must not rewrite the input field, alter its attribute/topology or MDT, position the cursor, erase the screen, reset MDT, or silently unlock/lock input. The illustrated input contains `HELP CMS`, still unsubmitted. Its bytes are absent from the semantic transcript.

At an actual AID/read boundary, copy the modified fields and cursor from the immutable event into the K draft model before applying focus, editor, paging or geometry actions. Redraw only after capture. Restore only a known captured cursor/draft. A connection-time geometry change without an input read cannot recover never-submitted terminal bytes: preserve the last captured draft and report that limitation explicitly. Do not claim otherwise. Protected asynchronous output that occurs at a valid service boundary preserves a pending draft; this is not general multitasking.

History records preserve hard line breaks and byte/encoding provenance. Wrap only in the viewport, without mutating stored text. Paging away from Latest pins the viewport: show `History` and a numeric count of newly appended records; do not jump to the bottom. PF9 returns to Latest. Long program names/commands shorten visibly with ASCII `...` in headings; their full values remain available in help/details and the transcript. Panel omission due to insufficient geometry is explicit; it does not discard retained records.

Raw native-screen activity invalidates the affected retained view. Show a protected `RAW SCREEN / retained view invalid; refresh required` notice when the shell regains ownership. Do not turn raw screen writes into invented lines. Printer jobs and other binary family facilities remain separate device endpoints. Their later UI must not consume or reinterpret the primary transcript.

## Teletype is a first-class interface

All three layouts have the **same ordered semantic projection**. A spatial rearrangement cannot reorder events or add headers, PF labels, border characters, colour escapes, cursor control, progress refreshes or unsubmitted drafts. Use plain UTF-8 line text and length-delimited text payloads, with stable command tokens and record ordering. The examples are an illustrative textual rendering proposal over the existing transcript event types; `PROGRAM`, `INPUT_OWNER` and `INPUT_REQUEST` are semantic projections/control-status records, not claims of additional implemented wire opcodes.

`COMMAND_BEGIN` identifies the submitted command. Every successful or failed completion has a matching token and a `COMMAND_END`, with OS/service outcome distinct from program RC. A load failure can read `os=LOAD_FAILED rc=NOT_RUN`; an application returning 8 can read `os=OK rc=8`. `ACTIVE` has `rc=PENDING`. A reader can distinguish them without screen-state reconstruction or guessing from a coloured return code.

Text payload bytes are normalized UTF-8 bytes, including the following newline; sample lengths must match that normalized payload. Native bytes remain available with explicit provenance through the binary service. Strip display control characters from this text projection; represent their occurrence as an explicit non-text event instead of injecting terminal escapes.

Required non-text/status examples:

```text
RAW_SCREEN token=0043 bytes=892 encoding=IBM-1047 retained=INVALID
GAP first_missing=0180 last_missing=0192 reason=HISTORY_LIMIT
INPUT_OWNER source=MONITOR consumer=PCOMM generation=8
INPUT_REQUEST source=MONITOR consumer=PCOMM kind=LINE
```

RAW_SCREEN records the occurrence and safe metadata; it does not paste binary data into the text stream. GAP explicitly records loss; it does not fabricate missing output or completion. Input selection is an explicit control/status operation, with a current physical source, consuming invocation and pending-transfer result. The line-oriented interface must expose this operation directly, so the LLM does not need a PF-key UI to obtain input. The concrete textual control grammar belongs to implementation review.

## Review boundary

These designs support approved step 1 (sessions, events, geometry, transfers and recovery) and step 2 (retained history, panels and a usable console). Implement and check one selected design, then pause for review after step 2. The full family including printer endpoints remains approved architectural scope; this board does not authorize advancing to the advanced-family step 3 before that review.

Owning sources read for this proposal: `pdos/doc/architecture/CONSOLE-STANDARD.md`, `pdos/src/twospace_console.{h,c}`, `pdos/src/twospace_3270.h`, `pdos/src/twospace_abi.h`, and PCOMM command/help code. No external source or external original was changed.
