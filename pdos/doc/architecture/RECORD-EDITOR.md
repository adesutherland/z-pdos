# Small C record editor

PD-027 owns the authorised 15 October increment. The maintained EDIT buffer
and operations are shared by a full-screen 3270 client and the line console.
The VM/CE Community Edition MECAFF EE editor, by Hans-Walter Latz, is the
interaction reference (upstream 1694fa893eb962ef1dca156247c6c3cca1750ac8).
This increment is original C90 code; no THE or EE implementation is imported.

The first profile has one buffer, 4096 records and 256 UTF-8 bytes per record.
The file boundary is explicit IBM1047 FB/VB with at most 256 native bytes per
record and 65536 packed bytes. Conversion, complete-field validation and
record-format/LRECL checks precede publication. Unsupported characters,
overlong records and invalid requests are refused rather than substituted.
Blank records, trailing spaces, FB padding and hidden horizontal columns are
preserved. The existing legacy PS SAVE AS route retains its earlier limits.

Operation 38 supplies checked dataset/member read and compare-and-save on the
IPL volume. The dataset must already have a supported DSCB. A new member can
be created in an existing PDS. The same native TSO journal key and two-bank
store are used, so native readers and batch DDs observe the saved member.
The physical PDS and its directory are not overwritten. This is the existing
bounded journal, not STORE-v2 or a general physical PDS update facility.

A SHA256 token binds the dataset/member, attributes, existence and original
native blocks. SAVE also compares the opened record attributes separately,
including when reconciling an identical-content retry. It compares the current token before writing, refuses a
changed or busy source, and verifies inactive-bank payload before publishing
its header. Full/invalid refusals leave the previous data and editing buffer
intact. I/O failure during publication can be uncertain: both generations and
the editing buffer are retained, and a retry reconciles identical committed
content without rewriting it. No host power-loss durability claim follows
from device completion/readback. SAVE AS requires an empty/new destination.

The screen client uses the existing invocation-owned device session, immutable
events, common 3270 address/input codecs and actual capabilities. It draws
numbered rows with separate line-command and source fields. One submitted
page is validated before changing the shared buffer. Read Modified identifies
changed fields; a complete Read Buffer recovers their exact positions,
including interior nulls that Read Modified suppresses. Terminals without
Read Buffer use the line interface. PF3 refuses unsaved
changes; CANCEL/QUIT DISCARD explicitly abandons them. Closing/unwinding the
session returns to the retained console. The line interface uses the same
commands, undo, conversion and file-save semantics.

The BATCH fixes retain the existing layout and input ownership. The current
FORM key owner supplies truthful form status, input guidance and legends;
STEP/DD edits describe output filtering without implying resubmission.

Host checks cover the actual editor model, codec, file framing/CAS/store fault
paths and K request gate. Review and a source/input freeze precede the Operator
matrix: models 2/5, monochrome, real line input, saved-member reopen/restart,
failed-save preservation, console ownership and edited-source batch execution.
