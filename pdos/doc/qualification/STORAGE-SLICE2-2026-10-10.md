# 0.2.2 dynamic DASD status and DISKMAP slice

This local slice removes the fixed 1500-cell K/U status buffers and extends
the read-only allocation view. PD-030 remains In progress. No guest was
started, no factory or shared disk was changed, and larger physical media,
writers and file transfers remain unqualified. The persistent STORE format
is unchanged. Version 0.2.1 remains the published release.

## Implementation and review

K reads the declared format-4 geometry, obtains an owned real-frame scratch
map of exactly that size, validates the complete supported VTOC and releases
the map on all normal success/error paths. The full U capacity is preflighted
in chunks no larger than 128 KiB before disk I/O. Header/payload overlap,
64-bit address overflow, unmapped late chunks, insufficient capacity and
malformed metadata produce no header/map publication. The copy writes only
actual track cells, preserving trailing capacity. Native mode/invocation
checks, the shared U pointer gate, real-frame allocator and interruption-driven
channel engine retain their existing roles; timers confer no ownership.

Version 1 retains the complete allocation summary when capacity is zero,
including the original 100-cylinder client contract. The explicit version-2
geometry query reads one format-4 record and allocates no map; allocation
statistics are absent and zeroed. DISKMAP uses that query before allocating
its U map and requesting a version-1 snapshot. Thus size discovery avoids a
second complete VTOC scan. This is an operation-count design/check result,
not a measured guest throughput claim.

The U graph uses declared geometry and at most three labelled rows of cylinder
groups. It reserves panel space for both the legend and return instructions;
the 24×80 layout uses fewer graph rows when necessary. A partial final group
is represented without reading past the map. The 100-cylinder plain view
retains one cylinder per cell. Maps are released before display/input, and
existing panels are explicitly closed on later refresh, display or input
errors. The application build receipt now includes its DASD header dependency.

Review covered all status callers, compatibility/version semantics, geometry
arithmetic, full-address carry/wrap, preflight before publication, map/panel
ownership, bounded layout, error paths and unchanged writer/physical-profile
limits. The final review resolved the small-screen instruction clipping before
selection. The [storage design](../architecture/STORAGE-EXTENTS.md) and
[operation-36 ABI](../architecture/CONSOLE-WORKBENCH-ABI.md#read-only-dasd-allocation-and-diskmap)
own the current contracts. The VTOC remains at cylinders 1–2, with 15 heads
and up to three direct format-1 extents. Declared geometry does not establish
physical device capacity.

## Focused checks

The selected run is `build/pdos/storage-022-check11` on macOS arm64 with
Apple Clang 21.0.0 (`clang-2100.1.1.101`). Four C89 Clang checks
use `-Wall -Wextra -Werror -pedantic` and ASan/UBSan:

- Original 100-cylinder map and malformed-extent controls.
- 1024/65535-cylinder map arithmetic, union counts, crossing cylinder 99/100,
  rejected late descriptors, capacity refusal and sentinel preservation.
- The actual K status source with deterministic channel, owned-allocation
  and U-span adapters: original summary behavior, geometry-only query,
  983025-cell map, high-address/low-word carry, invalid late chunks, overlap,
  whole-address wrap, short capacity, read failures, bad extents, allocation
  refusal and no partial publication/leaked allocation.
- The actual DISKMAP source with deterministic service and captured line/panel
  adapters: 3/100/1024/65535 cylinders, narrow/wide/line layouts, all grouped
  cylinder ranges including the tail, 24×80 footer space, volume changes,
  refresh failures, panel creation/text errors, input failure, allocation
  refusal and malformed summary refusal, with map/panel cleanup.

The adapters establish service/client logic; they do not execute Hercules
channels, actual U DAT translation or the guest screen. The unchanged pointer
gate/channel objects are explicit 0.2.1 inputs, rather than newly guest-qualified
components. The host fixture suppresses only the macOS SDK's deprecated C89
`sprintf` declaration annotation around the actual target source; buffer and
sanitizer checks remain enabled.

Classic C31 compiles and Classic Assembler's desktop profile assembles the
map module, complete K consumer and U application. The service links with 26
explicit unchanged kernel objects; DISKMAP links with 25 unchanged U runtime
and presentation objects and packs into the native TSO31 stage.

| Output | Selected result |
| --- | --- |
| TDA | 665 assembled statements, 61 symbols, 21 fixups |
| TWOSVC | 54640 statements, 4358 symbols, 3351 fixups; linked service 456318 bytes within the 2 MiB bank |
| DISKMAP | 1430 statements, 111 symbols, 58 fixups; native stage 166068 bytes, 9 FB18452 blocks |

Earlier development attempts encountered macOS header annotations and cREXX
recipe scope/array-reuse errors. Those were repaired before selection; failed
attempts and the mixed input/output receipt are excluded from acceptance.

## Reproduction and exact inputs

From the owning repository, with a fresh output directory and checked prior
0.2.1 objects:

```sh
crexx -nokeep pdos/scripts/storage-check.crexx --args build/pdos/storage-check /absolute/checked-image/kernel /absolute/checked-image
```

The second argument enables the K link; the optional third enables the U link
and native packing. Omit both for focused host and compile/assemble checks.
The maintained cREXX recipe records source/tool/prior-object and output hashes.
For the selected run:

- `inputs.sha256`: 401 entries; SHA256 `199994f0992539e1ca3bf34897f14fd2afe30992411267d60e1c526190b8b006`.
- `outputs.sha256`: 10 entries; SHA256 `699d58130d9704fb20a247ca9c5aad9b1ad660b8e77a10605c36aaba07b8853b`.
- Git parent: `a9adef9b9bfd9fc8ce98454400eacf464094200c`; the selected run used the working-tree source now retained with this report in the local storage checkpoint. Local commit was authorized on 10 October; push and publication are separate actions.

Both manifests were independently reread and verified after selection. The
remaining work is physical geometry admission and coordinated producers/oracles,
extent-aware bounded file streaming, allocation/reuse, a reviewed STORE v2
layout and migration/recovery, then measured contiguous/fragmented I/O and
the full combined guest QA. These checks enable none of those claims.
