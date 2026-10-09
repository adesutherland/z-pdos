# K/U source-built image and operator route

The PD-003 kernel has accepted P0–P6 results and is the default
source-built kernel. Final code review and the recorded source freeze precede
its accepted qualification.
Version 0.2.0 selects this route for release builds; the
[release record](../qualification/RELEASE-0.2.0.md) names its operator acceptance.
The earlier 0.1.1 image keeps its recorded one-ASCE boot route.

The published 0.1.1 producer remains available explicitly as
`pdos/scripts/one-space-image.crexx`; `image.crexx ... legacy` selects it.
Legacy results remain separate from K/U qualification.

Use the owning checkout on develop, prepared Classic C/assembler/linker tools,
GNU s390 assembler/linker, Hercules and, for workload qualification, the frozen beta-3 CMS/TSO stage inputs
listed in the [ABI inventory](../architecture/TWO-SPACE-ABI.md). A bare default
image needs no application ZIP:

```sh
crexx -nokeep pdos/scripts/image.crexx --args \
  build/pdos/default-image "$HERCULES_BIN" "$GNU_AS" "$GNU_LD"
```

Alternatively set `ZPDOS_GNU_AS` and `ZPDOS_GNU_LD` to absolute tool paths,
or put the GNU s390 tools on PATH, and omit the last two arguments.
To include the frozen qualification workload inputs, invoke:

```sh
crexx -nokeep pdos/scripts/two-space-normal.crexx --args \
  build/pdos/normal-image "$GNU_AS" "$GNU_LD" "$HERCULES_BIN" \
  "$CMS_STAGE_DIR" "$TSO_STAGE_DIR" 2 3270 monitor
```

All output paths must be fresh. Tool and input directories are absolute.
The last three arguments explicitly select model 2/3/4/5, primary `3270` or
`line`, and optional `monitor` or `none`. Omitting them selects model 2,
3270 at 0009 and the monitor at 000A. The selected normal disk is
`build/pdos/normal-image/image/media/pdos00.cckd`. The source K core and
producer manifests are under that build's kernel directory. With explicit
console arguments, use the configured `normal.core` at the build root for
the guest gate.

A normal guest needs ESAME, one CPU, 256 MiB real storage, that disk at 01B9,
a primary device matching the selected configuration at 0009 and, when
selected, `000A 3215 noprompt`. Connect the terminals before IPL 01B9. K
starts U PCOMM; an ordinary EXIT returns to K, commits the transcript and
prints K SHUTDOWN before disabled wait. The monitor defaults to output capture. Interactive input requires both
configuration mask `0x00000001` at console word 24 and an explicit handoff
by a U application.

The 0.2.1 Workbench development console separates program output and shell
history. Enter clears the submitted command and immediately returns the cursor
to the start of the input field. Its keys are PF1 Help, PF2 Edit, PF5/PF6 command
recall, PF7/PF8 page, PF9 Latest and PF10 Output/Shell. PF10 toggles the focused
pane and its highlighted heading; on colour terminals this is the yellow bar.
PF11 and PF12 have no Workbench action. A panel selected with cursor plus Enter
stays selected when chosen again. Plain teletype operation uses ordered text.
Explicit `CONSOLE INPUT PRIMARY|MONITOR` commands still select shell input;
monitor handoff requires the configured interactive reply path. A capture-only
monitor refuses selection. While interactive monitor input is pending, Clear
on the primary cancels the exact monitor request and restores primary input.
The monitor needs an actual reconnect before it can be selected again.

The opt-in `DISKMAP` C application shows actual mounted DASD allocation: a
capacity bar, cylinder map, reserved/allocated/free track counts and the largest
unallocated run. Enter refreshes, N selects the next registered volume and Q
returns to PCOMM. It uses the read-only K storage service and also prints a
readable teletype view. The current parser supports the maintained 100-cylinder
3390 layout and direct extents; these are allocated tracks, not file contents
or compressed host disk sizes.

For a human operator, select the primary 3270 device as
`0009@HOST:PORT` with a terminal model matching the image configuration.
The separate text console is a Telnet connection to the same host and port:
set its negotiated terminal type to `ANSI@000A` to select the 3215 monitor.
For a line-only primary, use `ANSI@0009` and a core configured with `line`;
changing the Hercules device alone does not change the kernel's selection.
The monitor defaults to output capture. Its input is used only when the
configuration permits it and an application explicitly requests handoff.

The accepted normal command route includes:

```text
VERSION
CMS RUN 31 RXVM -v
RXVM -v
MISSING
RXVM -v
EXIT
```

Each external command has numbered BEGIN/END output. A real application
return reports RC. A launch or service failure reports OS status and
`RC=unavailable`; it must not be read as an application RC. CMS RUN selects
its declared mode. Ordinary TSO names search 31-bit, then absent-only 24-bit
and 64-bit fallbacks; profile-specific dataset aliases can select unchanged
members when their names overlap.

For the repeatable disposable gate:

```sh
crexx -nokeep pdos/scripts/two-space-normal-gate.crexx --args \
  build/pdos/normal-gate "$ABSOLUTE_IMAGE_BUILD" "$ABSOLUTE_CORE" "$HERCULES_BIN"
```

Use the fifth argument `bare` for a disk with no application package. The
fifth argument `rootfault` stages the small deliberate U-root
fault fixture instead of PCOMM. That control verifies K emergency output,
owned cleanup, stopped-disk integrity and durable text; it is not a normal
operator command or release asset. [P5 qualification](../qualification/TWO-SPACE-P5-2026-10-07.md)
records the exact accepted build and guest identities.

The native workload recipe is `two-space-workload-image.crexx`; retained
command/input fixtures are in `pdos/tests/two-space/p6/`. CMS compiler imports
use the explicit `A1` collection. Native TSO24 is qualified separately on
its released library-free IO24 path; full-library IOQUAL remains outside that
bounded 24-bit result. Wider cREXX heaps retain 64 MiB (31-bit) and 128 MiB
(64-bit). PCOMM's source-built C arena is 16 MiB, allowing the qualified nested
presentation/HIGH path on the 256 MiB guest.

The primary and monitor use IBM-1047 native text with Hercules
`CODEPAGE 819/1047` for the Telnet 3215 connection. A detected monitor loss or
raw screen produces an explicit gap: its loss-control result can pass while
`capture_qualified` remains false. It must never be reported as complete text.
The [completion plan](../BACKLOG.md#pd-003-completion-plan-6-october-2026)
records the completed P0–P6 checkpoints.

The existing `DEVICES`, `VOLUMES`, `MOUNT`, `SELECT`, `UNMOUNT`, `DIR`,
`ALLOC`, `RCOPY` and `TAPE` commands have a normal K/U route. Attach an
exchange disk or tape to the disposable guest before IPL; the guest command
registers that already attached device. `SELECT` changes native executable
and dataset lookup. An open DD retains its registered device. The bounded
100-cylinder allocator, exact FB/VB copy and raw tape limits are described in
[the media guide](MEDIA.md).

Native CMS and TSO application output and the transcript use the banked
`PDOS.STORE` on the IPL disk. Keep that stopped disk when exporting application
results; the checked `store_check.py` and `workload_output.py` adapters inspect
its records. Selecting a CMS exchange volume does not put these journaled
outputs on that exchange disk. Native sequential `COPY` to an allocated
exchange dataset and native tape transfers write their physical target and
have independent stopped-media verification.

`two-space-media-gate.crexx` reproduces the accepted mounted CMS, fixture
record-copy and tape workflow using the pinned workload and fixture inputs.
The [P6 record](../qualification/TWO-SPACE-P6-2026-10-07.md) identifies those
inputs, exact results and current final-selection status. General CMS/TSO services and unqualified low-level development commands remain
outside this replacement contract. The explicit one-space producer retains
its older operator environment and qualification.
