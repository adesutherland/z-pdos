# 0.2.2 initial geometry and extent slice

This is local development evidence after publication of 0.2.1. PD-030 remains
In progress. The [storage design](../architecture/STORAGE-EXTENTS.md) selects
native extents, bounded streaming and a separately reviewed durable-store
format/migration contract. Larger production disks and file transfers are not
enabled or guest-qualified by this first slice.

The fixed K caller described below was the selected first-slice state.
[Slice 2](STORAGE-SLICE2-2026-10-10.md) supersedes that caller with dynamic
read-only snapshots and updates DISKMAP; it retains this first result as
historical implementation evidence.

The portable supplied-storage allocation map now derives its track count from
the admitted format-4 cylinder declaration. Checked helpers convert inclusive
cylinder/head ranges and reject bad order, out-of-range heads/cylinders and
invalid geometry without modifying outputs. TDAADD validates every descriptor
before publishing any map cells and counts overlaps as a union. The current
fixed-buffer K status caller explicitly retains 100-cylinder admission;
otherwise a smaller declaration could copy uninitialized trailing cells.

Review covers all callers, maximum arithmetic, buffer extent, non-mutating
failure paths and the unchanged 3390/VTOC/direct-format-1 subset. Focused C89
ASan/UBSan checks pass the original 100-cylinder controls, independently
expected 1024-cylinder allocation/free-run counts, a range crossing cylinder
99/100, a rejected late descriptor after an earlier valid one, insufficient
map capacity, malformed geometry, and the maximum 65535-cylinder declaration.
The maximum case uses 983025 cells and preserves sentinel bytes immediately
past the buffer.

The helper and full K consumer compile and assemble through Classic C31.
The selected `build/pdos/storage-022-check4` links their new objects with the
explicit unchanged kernel objects from the qualified 0.2.1 source build. The
linked service is 454790 bytes, within the 2 MiB bank. The helper assembles
665 statements; the consumer assembles 54168 statements. An initial recipe
ordering error attempted the link before the consumer object existed; it was
corrected before selecting check4. That failed attempt is not acceptance.

Reproduce from the owning repository with a fresh output directory:

```sh
crexx -nokeep pdos/scripts/storage-check.crexx --args build/pdos/storage-check /absolute/checked-kernel-object-directory
```

Omit the second argument for the focused host and compile/assemble checks only.
The selected run's `inputs.sha256` records 371 source/tool/prior-object inputs,
SHA256 `b6f640e3b345c21c1c748f5a355909ef0b3f97cd246e40a03878924382a1a3f4`;
`outputs.sha256` identifies the checked binaries and objects. Git parent is
`a9adef9`; the selected run used uncommitted 0.2.2 source, now retained with
this report in the local storage checkpoint. No guest was started and no
shared image or persistent disk layout was changed for this slice.

At this checkpoint, dynamic K/U map storage and compatible status queries, actual media admission,
streaming across tracks/extents, allocation/reuse, STORE v2 migration/recovery
and measured I/O remain separate implementation steps. Small checks precede
the full combined guest QA at the end.
