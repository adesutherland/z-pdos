z/PDOS 0.2.1 adds a retained Workbench console with separate program output
and command history, scrolling, function-key guidance, colour and monochrome
views, and a plain line console for automated/LLM operation. K owns terminal
sessions, buffers, input leases and completion; C applications in U own
presentation. The common I/O path uses interruption-driven completion.

The image includes an original small C record line editor, FIND, HEX, CMP/FC,
DISKMAP and runnable C examples. EDIT preserves fixed/variable records and
supports grouped input, insert/delete/change, literal search, undo and checked
SAVE AS to a separate empty dataset. DISKMAP shows actual allocated/reserved
tracks and the largest free run.

Canonical cREXX v1.0.0-beta.3 TSO31 RXC, RXAS, RXVM and libraries are bundled
from the SHA256-pinned mainframe release ZIP. The documented Hello example
compiles, assembles and runs with arguments on the guest. Full Rexx shell
integration and a fullscreen editor remain later work.

The default machine now has 512 MiB real storage, a 16 MiB core, a 2 MiB K
service bank, a 512 KiB K stack and 3 MiB K/U translation pools. The desktop
assembler profile supplies a 256 MiB host budget with larger symbol, literal,
fixup and macro arenas while retaining checked bootstrap defaults.

Bounded guest qualification includes 21 console runs/333 checks and a separate
68-check application matrix on standard/wide colour, monochrome and line
consoles, including exact saved-file and durable-transcript readback. Release
selection reuses unchanged payload evidence and checks the versioned image
separately. Read the bundled release and application records for identities
and scope; this does not establish complete 3270-family conformance.

Current limits include the 100-cylinder disk layout, eight 2 MiB stored-file
slots, first-track/65536-byte packed record transfers and SAVE AS without
atomic replacement. Hercules's 3215 keyboard provider limits input to 148
characters plus CRLF. DX3270 1.7.5 has recorded code-page, retained-pane repaint
and graph-export defects; x3270/s3270 remains the qualification reference.
The initial 0.2.2 focus is disk layout, large files, recovery and measured I/O
performance.

Classic tools are supplied for macOS Apple Silicon/Intel, Linux x64 and Windows
x64. The tag workflow publishes only after all required hosted builds pass.
macOS packages are Developer ID signed, notarized and stapled. Windows CI
downloads are explicitly unsigned pending the separate local signing route.
SHA256SUMS identifies the final downloads; corresponding source is included.

Paul Edwards created PDOS and PDPCLIB. Component licences, original notices
and canonical cREXX/runtime attribution accompany the downloads. No IBM
operating system, proprietary compiler or private guest disk is included.
