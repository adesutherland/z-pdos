# Shared batch package and native reference

The selected Stage 3 package is TSO31, direct MVS PARM entry, and one shared
SDK/cREXX build. The SDK owns version-8 DD-backed standard streams and refuses
terminal callbacks. cREXX owns the four-tool consumer and matching RXBIN
libraries. PDOS owns native DD services and the serial job descriptor.

The disposable package profile selects a 150-cylinder 3390 while retaining
the default eight 2 MiB STORE slots. It needs the space for all original
foreground programs plus separate batch binaries and eleven MiB of libraries.
The default 100-cylinder and existing large-file profiles are separate.
The batch kernel validates actual format-4 geometry and bounds DSCBs by it.
PDS member TTRs are checked against the dataset extent, rather than an
unrelated directory-scan limit. Fresh binary staging preserves all bytes
outside the selected library extent and its last-used-record field.

Each native batch step binds SYSIN/SYSPRINT/SYSTERM. The compiler and assembler
use job-owned temporary assembly data; RXBIN is explicitly retained for later
link/run work. The current sixteen physical DD entries fit compile/assemble/run
in one job; link/run is a second job. Stage 4 owns wider expanded JCL bounds.

The native TSO facility uses SYSTSPRT for selected batch command output.
PDOS emits checked FB/VB records through that binding; it does not relabel
arbitrary TPUT as output. Batch CMS, PDOS operator/media commands and program
launch through this facility are refused. The existing TSO command subset
and native module ABI remain explicit. Interrupt-key dispatch is distinguished
from the established command service by actual interruption disposition and
code, preserving foreground/native ADDRESS and serial cancellation.

The frozen matrix is a C DD/Unicode/binary/missing-DD probe; the four native
tool interfaces; fresh compile/assemble/run and link/run; invalid compiler
source with a skipped dependent step; absent input; normal RC8; partial startup;
native C and cREXX ADDRESS TSO TIME; and actual PARSE compiler exits. Independent
spool/step/DD reports, exact named data and portable RXBIN execution accompany
stopped ownership and process checks. The native reference supplies direct
EXEC PGM jobs and background TSO where its command context is required.
Native TIME text is OS-specific and is assessed as native command output,
not a byte-identical timestamp. Other frozen source/data/native outputs are
compared exactly where their format is shared.

Review, focused controls and exact source/input freeze precede guest runs.
A failure returns to review and a new freeze. The selected package gate passed
on native z/OS 1.5 and PDOS line/model2 on 14 October; the
[qualification summary](../qualification/BATCH-STAGE3-PACKAGE-2026-10-14.md)
records the exact scope. JCL parsing, 3270 forms, managed adoption and release
remain later work.

cREXX's batch build uses the SDK's UTF-8 standard streams directly; its
foreground console path still translates to native IBM1047 itself. Named
text files keep cREXX's existing codec policy. The qualification fixture
therefore supplies IBM1047 source records, matching native in-stream DDs.
This adjustment passed the paired qualification. Retain concise result summaries;
disks, dumps, traces and detailed per-file receipts are temporary working files.

Use normal reusable build locations: the SDK's `build/tso-batch`, cREXX's
`build/mainframe-batch`, and PDOS's `build/pdos/batch-sdk` paths. Clean failed
generated output in place after checking ownership and active consumers.
The Mainframe Operator owns all z/OS and PDOS configurations and operations.
Qualify the reviewed shared package on z/OS first, repairing the SDK or cREXX
there as necessary, then execute the same native bytes on PDOS. Keep the
current candidate only while qualification needs it; do not create numbered
run directories or retain earlier SDK/disk copies for each retry. Build the
current resident PCOMM/router with `two-space-command.crexx` at
`build/pdos/batch-sdk/command`; its TCM1 descriptor is required for native
ADDRESS services. Successful boot/version output from an older shell does
not establish that this interface is installed.
