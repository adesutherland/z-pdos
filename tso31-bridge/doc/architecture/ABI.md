# Original TSO31 entry bridge consumer

I selected this entry bridge as the first real consumer for Mainframe Classic
Assembler. It is a synchronous, non-reentrant TSO/E adapter, with one private
invocation and no IBM Language Environment dependency. It is not a z/PDOS
service implementation or a general macro library.

See [the dated checkpoint](../qualification/CHECKPOINT.md) for exact host assembly, target C
compilation and link results. The subsequent Mainframe Classic Linker repair
closes the inherited nonzero-origin address audit for the retained package.
Guest execution and rebuilding the complete native library remain open.

`reference-entry31.asm` preserves the exact project-authored reference selected
for this work. Its SHA-256 is
`ad226ed2c1dd647fa22e532dc5f40662295d9b90b20e20ee1ae7d0af3fc983c8`.
The reference's first comment identifies its project ownership. Its calls to
IBM service macros contain no macro definitions or private expansions. I approved
extracting this original source and the new variant under the project's MIT
licence. IBM manuals, macros and service implementations are not included.

`entry31.asm` is the maintained assembler input. We deliberately replace only
the service forms below with original explicit register setup and SVC operations.
The [service notes](SERVICES.md) explain their public interface facts. The variant
retains every callback, cleanup path, native file call, character translation
table and service-vector field from the selected reference. It is a separately
identified source variant; it is not unmodified IBM-macro source assembly.

| Reference form | Maintained source operations | Limited contract |
| --- | --- | --- |
| `GETMAIN RC,LV=1048576,LOC=ANY` | R0 length, R1 zero, R15 `GMANY`, SVC `SVCMEM` | Conditional 1 MiB allocation; subpool zero, caller key, full 31-bit address |
| `GETMAIN RC,LV=256,LOC=BELOW` | R0 256, R1 zero, R15 `GMBELOW`, SVC `SVCMEM` | Conditional terminal buffer allocation below 16 MiB |
| `GETMAIN RC,LV=(0),LOC=ANY` | Preserve R0; clear R1; select `GMANY`; SVC | Callback's requested byte count; same limited allocation options |
| `FREEMAIN RC,LV=...,A=(...)` | R0 exact original size, R1 address, R15 `FMCOND`, SVC | Conditional release of the same subpool-zero allocation |
| `TPUT (9),(8)` | R0 from R8, R1 from below-line R9, SVC `SVCTERM` | EDIT, WAIT, NOHOLD and NOBREAK defaults; existing callback still returns zero |
| `TGET (9),(7)` | R0 from R7, R1 from below-line R9 with input flag, SVC `SVCTERM` | EDIT and WAIT defaults; preserve native status and returned byte count |
| `DYNALLOC` | SVC `SVCDYN`, using the already constructed R1 pointer list | Existing DD-name unallocation request only |

The variant adds six local EQU definitions and explanatory comments. It removes
the reference's private-laboratory wording from its own header; the exact
reference retains its original comments. There are no conditional/unconditional,
arbitrary subpool/key, branch-entry, access-register, 64-bit, terminal-list-form,
or generalized DYNALLOC replacements. A new service form requires an explicit
source change and an independent test.

## Invocation and object contract

```text
mf-classic-as --profile s370 tso31-bridge/src/entry31.asm entry31.obj
```

The instruction subset includes BASR and therefore needs the selected S/370
profile. The program's AMODE 31 and RMODE ANY metadata describe its required
execution environment separately; an arbitrary historical S/370 machine does
not acquire 31-bit addressing by selecting an assembler instruction profile.

The object must preserve `LABTSO`, its section-relative entry, local A-type
callback/table addresses, and external V-type references to `ELFPOC`, `@@AOPEN`,
`@@AREAD`, `@@AWRITE`, `@@ACLOSE` and `@@DYNAL`. Ordinary explicit code and the
literal pool must remain below the displacement reach of their declared USING.
Output comparison tests inspect binary records and interface fields without
using assembler encoding helpers. Host assembly does not qualify native linking,
execution, service availability or code residence.

The maintained source produces one 1,168-byte section: 1,072 bytes of text and
96 bytes of reserved storage. Its deck has seven ESD identities and 13 A/V
relocations. `test_deck.c` independently checks those identities, section modes,
entry, every reservation, relocation location/target/addend, service register
setup, native file linkage and callback save/restore instructions. It also checks
the complete literal pool and all 256 preserved translation-table octets.
`test_source.c` checks all 237 original executable statements against the
maintained variant, allowing exactly the nine listed replacements and six EQU
definitions. The exact reference hash remains a separate byte-preservation
check.

## Caller and runtime prerequisites

The entry expects native TSO linkage: R1 addresses the one-element argument
pointer list, its pointer's flag bit is masked, and the pointed data starts with
a two-byte length. R13 supplies the original 72-byte native save area, R14 the
native return address and R15 the entry address. The bridge saves the native
registers, obtains a 1 MiB C stack and a 256-byte below-line terminal buffer,
then calls `ELFPOC` with R2 argument bytes, R3 argument length, R4 service-vector
address, R14 return address and R15 stack top. The stack top leaves 96 bytes for
the existing 31-bit ELF caller area. The C result is in R2 and is converted to
the native R15 result at cleanup.

`abi.h` describes the existing version-4 vector for a 31-bit, big-endian C target
with four-byte integers, pointers and function pointers. Ordinary host C type
sizes do not establish that ABI. Compile `target_layout.c` with the selected
target compiler before linking: its C89 type assertions require four-byte
integers and pointers, vector offsets 0, 4, 8, 12, 16, 20 and 24, and total size
28. A failed assertion stops the target path. Passing these layout checks still
requires separate big-endian object and register-linkage verification.
`test_abi.c` checks the small caller's allocation, output and cleanup behavior
with ordinary host callbacks; it does not establish the target layout or ABI.
Each returning callback preserves C's R6–R15
through a single static save area. Calls must not nest or run concurrently.
READLINE returns EBCDIC bytes; PUTLINE accepts the reference's ASCII-to-IBM1047
table and its existing 132-byte limit. The buffer copy and translation behavior
is preserved, including translating the full 132-byte output workspace.

FILECALL operations 0–4 enter the external PDPCLIB native routines through R1
parameter-list/R13-save-area/R14-return/R15-entry linkage, and convert their R15
result to R2. Operation 5 constructs the existing SVC 99 DD-name unallocation;
only names allocated by this runtime belong there. These native routines are
unresolved prerequisites, not stubs supplied by this component.

The coordinator owns selection and qualification of the existing downstream
linker/packager and the exact C compiler/ELF-to-classic route. The ELF-style C
entry must not be silently treated as a native PDPCLIB C entry. A later small C
caller link/run needs that exact producer, the six resolved external identities,
native file support, TSO storage/terminal/dynamic-allocation services, and a
separately qualified guest execution. No guest action is part of this source
and host-object increment.
