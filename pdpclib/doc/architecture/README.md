# PDPCLIB architecture

PDPCLIB provides both a C library and the native machinery that lets that
library run in a selected environment. Those layers are related but separable.
The z/PDOS OS build consumes the whole selected runtime; the modern ELF SDK
consumes selected native services through its own adapters.

## The layers

```text
C program
  │
  ▼
portable C library       stdio, allocation, strings and other C functions
  │
  ▼
native support          startup, storage, terminal and dataset operations
  │
  ▼
selected service macros parameter lists, linkage and control-block fields
  │
  ▼
execution environment   z/PDOS, or the specifically selected MVS/TSO route
```

The [maintained source](../../src/) owns the common C implementation and
headers. It also contains startup and assembler routines. File operations in
C eventually reach the native support, where record organization, DD names,
DCBs, terminal calls and storage-service conventions become environment-specific.
Successful host C-library fixtures do not validate those native calls on a
mainframe.

There are two startup arrangements in the OS build. **SAPSTART/SAPSUPA** support
the standalone loader and kernel, which receive storage and machine context
directly. **MVSSTART/MVSSUPA** support PCOMM as an application using the selected
MVS-style interfaces supplied by z/PDOS. They share common C library code but
enter and obtain services differently.

## Profiles select maintained source

[`src/profiles/`](../../src/profiles/) holds explicit runtime configurations.
Each `mvssupa.inputs` list selects shared modules from `src/native/mvssupa/`
and any profile-specific modules. Preparation concatenates the selection into
one assembler input and chooses its `PDPTOP` configuration. It does not apply
patches to a frozen source copy.

For example, `pdos-zarch` chooses the services and architecture switches needed
by the current OS. `tso31-sdk-files` retains the selected sequential and
partitioned-dataset paths while rejecting VSAM, IDCAMS and supervisor-mode
switching. A profile is a statement of intended source/service scope, not proof
that every retained path has executed successfully.

The original five selections reproduced their pre-reorganisation bytes at the
recorded checkpoint. Later source changes and the added SDK file selection have
new identities. Use the preparation receipt and the current source revision
when recording a build, rather than reusing an old receipt for a changed tree.
The [user guide](../user/README.md) lists the selections and commands.

## Source-owned macro interfaces

Three interface directories supply the selected forms without distributing an
IBM macro library:

| Interface | Purpose | Contract |
| --- | --- | --- |
| `classic-linkage` | Register names, save/return and call forms used by the maintained source. | [Classic linkage](CLASSIC-LINKAGE.md). |
| `pdos31` | Selected service expansions and control-block fields for the PDOS route. | [PDOS31 interfaces](PDOS31-INTERFACES.md). |
| `tso31` | Selected TSO terminal, EXTRACT and control-block forms used by the SDK file service. | [TSO interfaces](TSO31-INTERFACES.md). |

The native build searches the selected TSO interfaces before PDOS31 when that
profile requires them. Unsupported operand forms fail assembly. Identical macro
names do not imply identical semantics across operating systems, and a field
offset used successfully on PDOS is not automatically a supported z/OS
programming interface.

## The ELF SDK boundary

A modern ELF application's C calls use Newlib and its SDK runtime conventions.
Adapters connect the supported operations to this repository's selected native
assembler service object. This reuse does not change the application's C data
model to Classic C's model or replace its whole runtime with PDPCLIB.

The selected TSO service has below-line, non-reentrant workspace and native
parameter restrictions. TSO31 and both TSO64 entry modes ran sequential
write/read and PDS-read checks on z/OS 1.5. TSO24 dataset I/O faulted during
above-line SWA lookup. Positioning/FBS extend and wider dynamic-allocation,
command and prefix routes need further qualification. The service still uses
some selected PDOS31 mappings and an inherited `TCBFA` test that is not an
intended IBM programming interface.

Keep that result separate from the z/PDOS guest milestone and from general
TSO compatibility. [PCL-007](../BACKLOG.md#pcl-007-qualify-selected-tso-service-interfaces-on-zos-15)
tracks the remaining service work. [PCL-001](../BACKLOG.md#pcl-001-partial-mvs-binary-update-support)
tracks incomplete update/seek semantics. The
[licensing guide](../../../LICENSES.md) and [component licence](../../LICENSE)
explain why reviewing each inherited native port remains necessary.
