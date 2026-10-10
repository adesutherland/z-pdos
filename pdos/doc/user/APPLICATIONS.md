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

## Small record line editor

`EDIT [dataset]` opens a bounded native text file, or starts an empty buffer.
The model is inspired by CMS EDIT. It is original C90 code, with editing and
buffers in U. The same commands and exact submitted bytes work on the
Workbench and plain teletype/LLM consoles. `HELP` lists:

```text
PRINT [first [last]]
TOP
BOTTOM
LOCATE literal
INPUT [after]
INSERT after text
DELETE first [last]
CHANGE line /old/new/
UNDO
SAVE AS dataset
QUIT
QUIT DISCARD
```

Line numbers begin at one; `INSERT 0 text` inserts before the first line.
`INPUT` inserts after the current line until a single `.` is submitted; `..`
inserts a literal dot. An empty submission creates an empty variable record.
`CHANGE` replaces the first literal occurrence on the named line. `UNDO`
swaps the previous editing state; consecutive UNDO commands toggle the change.
One INPUT group is one undo unit. Invalid/full edits preserve the buffer.
`QUIT` refuses unsaved changes. `QUIT DISCARD` deliberately abandons them.

The first editor supports **4096 logical records, each at most 256 bytes**.
It preserves trailing spaces and FB padding. The file service supports PS
FB/VB files in a single contiguous cylinder whose data and EOF are on
the first track. It refuses spanned formats, PDS members and larger files.
This is a small interim tool; the fullscreen editor remains a later project.

The installed Hercules 3215 provider accepts at most **148 input characters
plus CRLF**. EDIT can load, display and preserve 256-byte file records on that
console, but a single interactive input line must fit the provider's smaller
buffer. A provider overflow returns an I/O error and exits the application;
the shell remains available. The 3270 input profile accepts the full 256 bytes.

Saves require a **separate empty preallocated dataset on a mounted non-IPL
volume**. All records, destination geometry and physical track capacity are
checked before writing. Overlong records are refused. Saving shorter records
to FB adds native spaces to LRECL; VB retains exact lengths, including zero.
Data blocks, EOF and VTOC completion are checked, with immediate readback.
An I/O failure can leave a partial destination; the original and the editing
buffer remain available. There is no atomic replacement or retry-overwrite.

Use `VOLSER:dataset` to access a registered volume without changing SELECT:

```text
MOUNT 01BB FTX001
SELECT FTX001
ALLOC DRAFT.TEXT VB 260 2640 1
SELECT PDOS00
EDIT EXAMPLE.REXX
CHANGE 7 /Hello/Welcome/
SAVE AS FTX001:DRAFT.TEXT
QUIT
EDIT FTX001:DRAFT.TEXT
PRINT 1 10
QUIT
```

The allocator currently works only on the supported 100-cylinder exchange
layout. `VOLUMES` shows registered serials. File names and volume serials are
case-insensitive in these C tools; text and literals retain their exact case
and native IBM1047 bytes. No UTF-8 conversion is performed by the editor.

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
