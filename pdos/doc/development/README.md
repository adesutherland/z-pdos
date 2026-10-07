# Developing z/PDOS

Read the [architecture](../architecture/README.md) before changing a service or
memory layout. The current contract is a protected AMODE64 nucleus with
Classic C31 services in K, one shared U address space, standard z/Architecture
instructions and selected unchanged AMODE24/31/64 applications.
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
| IPL and kernel loading | `pload.c`, `ploadsup.asm`, `twospace_boot.c`, `install-ipl.c` | `image.crexx`, normal core configuration and KCORE.BIN packing. |
| K entry, dispatch and completion | `twospace_normal.S`, `twospace_entry64.inc`, `twospace_nucleus64.inc`, `twospace_entry.asm` | Full architectural state, K/U ASCE and invocation return/fault handling. |
| DAT, frames and placement | `twospace_dat.c`, `twospace_gate.c`, `twospace_real.c`, `twospace_memory.c`, `twospace_placement.c` | K-private aliases, storage keys, interval ownership and fixed-origin overlays. |
| Native loading and calls | `pdosutil.c`, `twospace_tso.c`, `twospace_cms.c`, `twospace_load.inc`, `twospace_call.inc`, `twospace_high.inc` | Declared modes, image leases, copied parameters, native linkage and caller restoration. |
| Files and durable output | `twospace_file.inc`, `twospace_cmsfile.c`, `twospace_store.c`, `twospace_service.c` | Invocation handles, cursors, DD/device binding and banked store commits. |
| Shared operator media | `media_commands.inc`, `twospace_media.inc`, `media_dscb.h` | One-space `pdos.c` consumes the same algorithms; K supplies private buffers and channel completion. |
| Channel and console | `twospace_channel.c`, `twospace_channel.asm`, `twospace_console.c`, `twospace_console.inc`, `twospace_transcript.c` | Separate low-real workspaces, matching interruptions, terminal/input leases and capture gaps. |
| U PCOMM and presentation | `pcomm.c`, `twospace_pcomm.c`, `twospace_ui.c` and their native linkage | Existing system/ATTACH behavior and checked terminal capability/screen requests. |
| Runtime | `pdpclib/src/`, `profiles/pdos-zarch/`, `interfaces/` | Shared owner; changes may affect SDK consumers. |
| Build and distribution | `pdos/scripts/`, root `scripts/`, `.github/workflows/build-release.yml` | [Dependencies](../architecture/DEPENDENCIES.md). |

Source filenames without a prefix are under `pdos/src/`. The explicit legacy
producer retains `pdos.c`/`pdossup.asm`; it is not the active K service dispatcher.

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
  Native images and HIGH bodies have explicit invocation owners; a live image
  cannot be silently evicted.
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

Complete whole-change code review and focused checks before expensive guest
qualification, then record a source/input freeze and a bounded matrix. A
repair returns to review before affected requalification.

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
