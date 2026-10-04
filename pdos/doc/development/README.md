# Developing z/PDOS

Read the [architecture](../architecture/README.md) before changing a service or
memory layout. The current contract is a C32 kernel, AMODE31 execution,
standard z/Architecture instructions and selected 31/64-bit applications.
A change to one of these is a design change, even if it is only a small source
edit.

Work on `develop` or an explicitly authorized `hotfix` branch, as described in
[AGENTS.md](../../AGENTS.md). Keep generated output under the root `build/`.
The component's [BACKLOG.md](../BACKLOG.md) is the single queue for defects,
improvements and missing qualification; exact run receipts belong in
qualification records.

## Find the implementation

All paths in this table are relative to the repository root.

| Area | Start here | Important neighbors |
| --- | --- | --- |
| IPL and kernel loading | `pdos/src/pload.c`, `ploadsup.asm`, `install-ipl.c` | `pdos/scripts/image.crexx`; startup/heap addresses and disk records must agree. |
| Initialization and memory | `pdos/src/pdos.c`: `pdosInit`, `pdosInitAspaces`, `ONESPACE` | `pdossup.asm`: `P64PC`; lowcore layout, DAT tables and reserved real frames. |
| Dispatch and program completion | `pdos.c`: `pdosRun`, `pdosDispatchUntilInterrupt`, `pdosProcessSVC`, `RB` | `pdossup.asm`: `ADISP`, `ADISP64`, interruption handlers. |
| Native load-module reconstruction | `pdos/src/pdosutil.c`: `fixPEModeBase` and public wrappers | `linker/src/`; section origins, relocation widths, entry and AMODE/RMODE metadata. |
| Direct / high program loading | `pdos.c`: `pdosLoadExe`, `pdos64HighService`, `pdos64HighRead` | Child ownership, input/image capacities, high slot lifetime and cleanup. |
| DD and dataset services | `pdos.c`: `pdos64SVC99`, `pdos64BindDd`, `pdos64ReadDscb`, `pdos64Pds*` | DCB/TIOT maps and the selected PDPCLIB native caller. |
| Physical disk and terminal I/O | `pdos/src/pdossup.asm`; `pdos.c`: `write3270` and record helpers | Channel status, retry behavior, target buffer addresses and record framing. |
| Command processor | `pdos/src/pcomm.c` | PDPCLIB `system()`/startup path and kernel command handling. |
| C library and service macros | `pdpclib/src/`, `pdpclib/src/profiles/pdos-zarch/`, `pdpclib/src/interfaces/` | Shared component: changes may affect both OS and external SDK consumers. |
| Build and distribution | `pdos/scripts/`, root `scripts/`, `.github/workflows/build-release.yml` | [Source-to-image dependencies](../architecture/DEPENDENCIES.md). |

## Follow a request across the boundary

For a file-I/O change, begin with the application's operation and follow it
through its runtime. PCOMM uses PDPCLIB C plus the selected native support;
a modern ELF application can use Newlib plus an adapter and selected native
services. These are different callers, even when both eventually issue OPEN
or a read/write request.

Check the parameter-list layout, register convention and address width before
changing the kernel handler. Follow the handler into dataset metadata and
channel I/O, then trace the result back to the caller. Record whether failure
is reported in a return register, an error field, a callback or task completion.
A successful return must describe an operation that actually completed.

For a loader change, keep the layers distinct: object cards, linked load-module
records, XMIT transport, disk record framing and in-memory image. A relocation
error and a transport error can look alike at the entry point. Preserve the
module's declared AMODE/RMODE rather than silently changing its contract.

## Contracts that need particular care

- **C32 and full-width state.** Kernel pointers are 32-bit. High addresses must
  cross explicit assembler/parameter interfaces; ordinary casts do not make
  the kernel LP64. Saved registers and PSWs must survive a round trip.
- **Memory ownership.** Low allocation, high heap and high code have separate
  backing ranges. Account for failure, child return, DELETE and repeated runs.
  The high loader currently has one active owner, not a general module cache.
- **Service scope.** Implement the selected form precisely and identify
  unsupported forms. A macro name shared with MVS is not evidence that every
  operand or control-block field has the same behavior.
- **Disk integrity.** Bound reads to the intended dataset and writes to its
  available extent. Check completion before committing directory metadata.
  Preserve binary data and distinguish end-of-file, empty records and errors.
- **Text.** Keep source encoding, EBCDIC execution text, terminal code page and
  application UTF-8 conversion separate. Batch framing uses hex-15 newlines.
- **Portability.** The host creating the disk may be little-endian and have
  64-bit pointers. Write target integers and records explicitly.

The source still contains historical alternatives and incomplete operations.
Start with the selected build path, and describe a proposal separately from
implemented behavior. In particular, increasing a storage limit does not
create a 64-bit C kernel, and `ATTACH`/`WAIT` names do not establish multitasking.

## Evidence for a change

Use the existing [build contract](BUILD-CONTRACT.md) and
[shared workflow](../../../doc/WORKFLOW.md). Match checks to what changed:
compiler/assembler output, link and load reconstruction, disk readback, then
real guest execution where the behavior depends on the machine or service.
Reuse unchanged evidence and retain exact source/tool/profile identities.
A host fixture cannot establish a new guest service or operating-system port.

For guest work, keep an untouched base image and use the execution environment's
ownership/lease procedure. The shared private Lab fleet has its own operating
guide; a source build must not modify an active guest disk. Public qualification
records should contain reproducible inputs and outcomes, without credentials
or account-specific assets.

Documentation-only changes need editorial and source-reference review, not a
rebuild or a repeated guest run. Historical qualification records remain exact
accounts of the inputs actually used.
