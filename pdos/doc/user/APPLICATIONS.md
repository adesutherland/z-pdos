# Applications in the 0.2.1 image

The normal source image builds the U31 C applications from this repository and
bundles the qualified cREXX v1.0.0-beta.3 TSO31 compiler, assembler and VM.
The [application qualification record](../qualification/APPLICATIONS-2026-10-10.md)
owns the tested development image; the [release record](../qualification/RELEASE-0.2.1.md)
owns final version selection and publication.

`CONSOLE STATUS` shows the active geometry, address format, feature mask,
monitor state, next input source and capture-gap count. `CONSOLE INPUT PRIMARY`
returns the next prompt to the primary. Monitor input needs an explicitly
interactive core configuration and an online monitor. The default monitor
captures output. PF1 through PF10 are used; F10 switches output/shell focus.

## Small C record editor

This section describes the selected PD-027 development candidate, after the
committed application baseline. It does not update a published or managed
image. See the [current qualification](../qualification/EDITOR-2026-10-15.md).

`EDIT [dataset(member)] [LINE]` opens printable IBM1047 FB/VB text, or starts
an empty buffer. A 3270 opens a fullscreen editor; a line console uses commands.
`LINE` also selects commands on a 3270. This is original C90 code over the
existing EDIT model. VM/CE Community Edition's MECAFF EE by Hans-Walter Latz
is the interaction reference; no EE or THE implementation is imported.

Type over numbered source rows. The two-character prefix accepts `I` to insert
a blank record after that row, or `D` to delete it. Enter applies the complete
submitted page. Invalid, duplicate, overlapping or protected fields refuse the
page without partially changing the buffer. Hidden horizontal columns stay at
their original byte positions. A submitted page is one undo unit.
Fullscreen requires the existing Read Buffer facility to recover exact field
positions; terminals without it use `EDIT name LINE`.

| Key | Action |
| --- | --- |
| PF1 | Toggle help |
| PF2 | SAVE |
| PF3 | QUIT; refuse unsaved changes |
| PF7 / PF8 | Page up / down |
| PF9 | UNDO; repeated presses toggle |
| PF10 / PF11 | Left / right by 40 native columns |

The command field and line console share these commands:

```text
PRINT [first [last]]
TOP / BOTTOM
FIND literal / LOCATE literal
UP [rows] / DOWN [rows] / LEFT [columns] / RIGHT [columns]
INPUT [after]
INSERT after text
DELETE first [last]
CHANGE line /old/new/
UNDO
SAVE
SAVE AS dataset(member)
END
QUIT
CANCEL / QUIT DISCARD
```

Line numbers start at one. `INSERT 0 text` inserts before the first record.
`INPUT` inserts after the current line until `.`; `..` inserts a literal dot.
An empty INPUT creates an empty variable record. One INPUT group is one undo
unit. `CHANGE` replaces the first literal occurrence on the named line.
`END` saves and exits. `CANCEL` deliberately discards the unsaved buffer.
In fullscreen mode PRINT positions the page at the requested first record;
the line route prints the requested range.

The buffer holds **4096 records, each at most 256 UTF-8 bytes**. File and 3270
boundaries use IBM1047, at most 256 native bytes per record and 65536 packed
native bytes per file. Conversion is strict: unsupported characters, controls,
invalid UTF-8 and overlong records are refused. Trailing spaces, blank VB
records and FB padding are preserved. Line commands support printable ASCII
literals; direct 3270 source fields also preserve printable IBM1047 Latin-1.
FB saves pad shorter lines to LRECL with IBM1047 spaces.

The installed Hercules 3215 accepts at most **148 input characters plus CRLF**.
Loaded records up to the editor limit can be displayed and preserved there;
interactive commands must fit the provider limit. Use printable ASCII input:
Hercules can replace host non-ASCII bytes with a dot before PDOS receives them.
The qualification adapter rejects those bytes before transmission; manual
clients must use ASCII for this route. A terminal I/O error ends
the application, so SAVE before deliberately disconnecting a line session.
No unsaved crash recovery is claimed.

On the **IPL volume**, SAVE uses the existing native TSO two-bank journal.
The dataset must already exist with PS FB/VB attributes, or be a supported PDS
with a member name. A missing member opens empty and SAVE creates it in the
journal. Native readers and batch DDs see the saved member. This does not
rewrite the physical PDS directory, enlarge STORE or create a new dataset.

SAVE compares a SHA256 token for the opened source with its current content
and attributes. A changed or busy source is refused (16); capacity/admission
refusals occur before publication. The editing buffer stays available after
a failed save. I/O error 12 can mean publication is uncertain: retrying SAVE
reconciles identical committed content, and reopening inspects the current
version. Device completion/readback does not establish host power-loss safety.
SAVE AS requires a separate new or empty target; existing nonempty data is
refused. It adopts the new source only after success.

The earlier non-IPL exchange route remains available for PS first-track files
in its supported 100-cylinder layout, through `VOLSER:dataset`. It can SAVE AS
a separate empty preallocated dataset, with the earlier completion/readback
checks; an I/O error may leave that destination partial. It does not support
replacement SAVE or PDS members. `VOLUMES` shows registered volume serials.

The [design and operation 38 contract](../architecture/RECORD-EDITOR.md) records
the bounded journal scope. Physical qualification and current limits are
reported separately; a host codec check is not a guest qualification result.

## Utilities and C examples

| Command | Purpose |
| --- | --- |
| `DISKMAP` | Allocation/capacity graph; Enter refreshes, N selects next registered volume, Q returns. |
| `FIND literal dataset` | Exact case-sensitive byte search, with record/column and matching text. RC4 means no match. |
| `HEX dataset` | Native record lengths and byte offsets/hexadecimal bytes. |
| `CMP first second` or `FC first second` | Exact logical-record comparison, including empty records, padding and record boundaries. RC4 means different. |
| `HELLO arguments` | Small C90 argument example. |
| `RECIO source target` | C example copying exact logical records to a separate empty target. |
| `PANEL` | Owned panel and submitted key/AID example; Q or PF3 returns. Plain line mode prints the same semantic events. |

FIND, HEX, CMP and RECIO share the bounded native record-file service above.
`TYPE`, `DIR`, `COPY`, `RCOPY`, `ALLOC`, `MOUNT`, `SELECT` and `TAPE` remain
available. Ordinary COPY is a byte-stream operation; use record tools for
isolated empty VB records and exact record boundaries.

`EXAMPLE.HELLO`, `EXAMPLE.RECIO` and `EXAMPLE.PANEL` contain the short C sources
as native VB text. `EXAMPLE.REXX` contains the cREXX source example. The C
cross-build recipe is `pdos/scripts/apps.crexx`, using maintained Classic
C/Assembler/Linker and the `pdos-zarch` PDPCLIB profile. It builds real native
U31 executables; this does not install a self-hosted C compiler.

## cREXX TSO bundle

The unchanged TSO31 package provides `RXC`, `RXAS` and `RXVM`, using the
qualified 64 MiB runtime profile. `CREXX.CREXX`, `CREXX.RXAS` and `CREXX.RXBIN`
are explicit source/assembler/bytecode collections. The bytecode collection
includes matching LIBRARY and RXCEXITS modules and the upstream IOQUAL fixture;
HELLO source is installed. RXC requires RXCEXITS for its certified compiler exits.

```text
RXC -v
RXAS -v
RXVM -v
RXC -l CREXX -i CREXX -o CREXX/HELLO.rxas HELLO
RXAS -l CREXX HELLO
RXVM -l CREXX CREXX/HELLO.rxbin -a one two
```

Compilation and generated outputs use the existing checked TSO services and
banked PDOS.STORE. There are eight 2 MiB stored-file slots; keep output names
stable and inspect capacity before creating many compiler outputs. Source
script convenience and `ADDRESS PDOS` remain separate work under PD-026.
The supported language is cREXX Level B, with the package's documented
compatibility limits; this is not a blanket classic Rexx compatibility claim.

The image recipe downloads the pinned public mainframe ZIP and verifies its
SHA256 and each unchanged native payload. `ZPDOS_CREXX_PACKAGE` may name an
already downloaded matching ZIP. The image has no private Lab dependency.
Licence/source attribution accompanies the packaged image.

`HELLO --memory-check` allocates 320 MiB, verifies one byte in every page
and the last byte, then releases the allocation. This requires the 512 MiB
development profile; it is a capacity example rather than a timing benchmark.
The record-file transfer limit remains 65536 packed bytes and one physical
track, independently of the editor buffer size.
