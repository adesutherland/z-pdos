# Synchronous application command environments

PCOMM is the shell. `CMS` and `TSO` retain their selected native command
contracts; `PDOS` is the separately named PDOS command vocabulary. An
application must query its availability before using it. This increment
implements the bounded CMS31/TSO31 invocation and discovery paths needed by
the ELF/newlib fixtures. It does not supply a Rexx language processor.

Command parsing and presentation live in `twospace_command_router.c`, a
stateless U/C module linked into the resident PCOMM image. PCOMM calls that
same module for VERSION, ECHO, TIME, DIR, DEVICES and VOLUMES. Stateful shell
controls and the other shell file utilities retain their existing helpers.
The router does not borrow PCOMM's command buffer, C stdio state or open files.

K validates native parameter lists, pointers, lengths, personality and mode
in `twospace_environment.inc`. It saves the caller's complete execution
context, supplies private arguments and a 64 KiB C stack, enters the resident
U dispatcher, and restores the caller on its explicit completion SVC. This
is synchronous. Timers do not select readiness or completion. All device,
file, storage and child invocation operations use the existing checked K
services and ownership records.

The PCOMM entry contains a versioned `TCM1` descriptor. K checks each relocated
entry against the mapped image before exposing it. TSO LOAD/DELETE of IRXINIT,
IRXSUBCM, IKJEFTSR and PDOSCMD uses invocation-owned reference counts; the
returned entry addresses carry the AMODE31 marker for native BASSM callers.
The descriptor is an internal resident interface, not a new application ABI.
Its signature is at entry offset 6; the fullword-aligned five-pointer table
starts at offset 12 and ends at offset 32. Shared C constants define these
offsets. The artifact check materializes the actual linked image and checks
the pointers and native veneer instruction bytes, including assembler padding.

CMS31 accepts the selected SUBCOM availability query and CMSCALL/SVC204
CALLTYP SUBCOM with its fenced parameter list and four-word extended list.
CMS and PDOS are available; unsupported names are absent. The query does not
return a general CMS SCBLOCK. CMS commands in this slice are QUERY TIME,
STATE filename filetype filemode, and a controlled unknown-command result.
STATE checks actual media or an already owned open file without advancing
the caller's cursor. Its output reports presence, rather than a full CMS
STATE directory listing.

TSO31 implements read-only IRXINIT FINDENVB discovery, the selected IRXSUBCM
QUERY parameter shape, and six-parameter synchronous IKJEFTSR invocation.
FINDENVB returns an opaque PDOS context in both its output parameter and R0;
it is not a traversable IBM ENVBLOCK. QUERY returns a 32-byte descriptive
entry for TSO or PDOS. Its routine name is descriptive; no IRXHST handler or
callback registration is supplied. TSO commands are TIME and quoted LISTDS
of a full dataset name, with actual VTOC presence lookup. LISTDS reports
presence or absence rather than a full TSO catalog/attribute listing.

The native PDOS environment supports VERSION, ECHO, TIME, DIR, DEVICES and
VOLUMES. Other names launch a checked TSO31 application synchronously through
the existing nested invocation service. Each new program launch refreshes
the validated executable image, including writable runtime state, before
entry; a cached mapping does not retain a previous run's closed stdio state.
Launch parameters are native
length-prefixed command text. This route is bounded to 198 bytes and an
eight-byte program name; it does not claim arbitrary shell syntax or all
PCOMM commands. CMS/TSO built-in discovery and command text accepts up to
32767 bytes; console writes preserve those bytes in chunks of at most 256,
each delivered as a logical output line.

A nonzero child RC is a completed command. TSO facility RC0/4 exposes a valid
command RC; service failures return facility RC12 and invalidate that result.
CMS has one native return and maps a contained child fault to RC-4. K restores
the parent after a child fault, preserving the open input cursor and parent
allocations. No unverified ABEND code is invented. Invocation-owned context,
stack and argument storage is freed explicitly or reaped on application exit.

The fixtures, review/freeze and guest evidence belong in the dated
qualification record. Application registration/callbacks, general command
tables, CMS24/TSO24/TSO64 environment adapters and an actual cREXX ADDRESS
bridge remain later work under PD-026. The existing whole-library Classic
client prerequisite remains PDPCLIB PCL-002.

Native layout references are IBM's [IRXINIT parameters](https://www.ibm.com/docs/en/zos/2.5.0?topic=irxinit-parameters),
[IRXSUBCM parameters](https://www.ibm.com/docs/en/zos/2.5.0?topic=irxsubcm-parameters)
and [IRXSUBCT mapping](https://www.ibm.com/docs/en/zos/3.1.0?topic=information-irxsubct-mapping).
These define the native subset; they are not evidence of full Rexx support.
