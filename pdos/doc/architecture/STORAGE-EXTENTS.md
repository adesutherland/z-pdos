# Disk geometry, extents and larger files

I selected disk layout and larger files as the initial 0.2.2 focus. We will
extend native datasets and the existing owned I/O services. The current
100-cylinder factory, eight fixed STORE slots and first-track record transfers
are different limits; widening one must not imply that the others have gone.
PD-030 in the component backlog owns implementation and acceptance status.

## Options and selected direction

| Option | Assessment |
| --- | --- |
| Increase fixed disk/file/slot constants | Useful only as an experiment; retains fragmentation, duplicated reservations and whole-file buffers, and changes persistence without defining recovery. |
| Native geometry/extents with bounded streaming and a versioned store | Selected. Fits the loader, record formats and common completion engine; lets allocation, transfer and durable publication evolve independently. |
| Introduce a replacement filesystem | Defer. Requires additional boot, native compatibility, tooling and migration contracts before it addresses the current consumers. |

## Geometry and extent foundation

The portable supplied-storage map reads a declared cylinder count from the
supported format-4 record. The initial subset is a 3390 with 15 heads and the
existing VTOC at cylinders 1-2. A positive 16-bit declaration can describe
3-65535 cylinders; this is a parser boundary, not qualification of every
physical device or volume of that size. Direct format-1 extents remain the
supported descriptor subset; continuation chains and relocated VTOCs need
their own implementation and controls.

Inclusive track coordinates derive from cylinder/head pairs. Validate both
ends, head bounds, order, geometry and every descriptor before modifying any
allocation-map cell. Read-only maps count overlaps as a union. Allocation and
replacement must instead reject ownership conflicts, protect boot/VTOC areas
and prove the destination belongs to the admitted volume.

The first source slice generalizes the map and exposes checked geometry/range
helpers. Its fixed K caller retained 100-cylinder admission. The second local
slice moves K status and DISKMAP together to owned dynamically sized buffers:
version 1 keeps its allocation-summary contract; version 2 queries geometry
with one format-4 read and no map allocation. K preflights the full U capacity
in bounded chunks, refuses header/payload overlap and copies only actual track
cells. K/U error returns release their maps and the client closes its panel.
The graph aggregates large volumes into labelled cylinder groups that fit
the existing screen and plain line layouts. These are host and native-object
checks; the default production media and writers retain their current limits.

Declared geometry does not establish device capacity. Before enabling larger
production media, the producer, installer, volume admission, transfer iterator
and stopped-media oracle must agree on the exact physical profile.

## Bounded streaming and allocation

Large files should transfer through bounded K buffers and record/extent
iterators, rather than allocate the entire file in K or raise a global buffer
limit. Keep logical byte offsets separate from data addresses and per-request
lengths. Portable pairs of full words can represent wider logical offsets;
this does not claim that every legacy dataset metadata field can express them.
Preserve FB padding, VB boundaries, empty records, explicit encoding and EOF.

K owns admitted file state, invocation resources, extents and completion.
U/C owns editor and application policy. Reuse the common event-driven channel
engine for streaming and future contiguous transfer batches. Timers must not
establish readiness or ownership. Measure transferred bytes, channel starts
and elapsed time for both contiguous and fragmented cases before claiming
performance improvements.

Prefer contiguous allocation when the requested ownership fits, then use a
bounded explicit extent list. Reserve and validate the complete ownership
before mutation. Out-of-space refusal must preserve the existing file and
metadata; descriptor limits must report exhaustion rather than truncate.

## Durable store and migration

STORE v2 is proposed, not implemented by the geometry slice. Allocate actual
extents instead of permanently reserving two fixed banks for each of eight
slots. Verify replacement data before publishing a checksummed generation or
root that makes it authoritative. Define exactly which extents both
recoverable roots protect before permitting reuse. A successful readback is
not proof of host power-loss durability.

Migration should copy a validated legacy store into a separate empty
destination, then verify it independently. Preserve the original disk for
rollback; do not silently upgrade it in place. Interrupted writes, corrupt
metadata, insufficient space and reboot selection need explicit controls.
The selected format/version and byte layout must be reviewed before changing
persistent media. A full combined guest QA run follows the small checks for
each implemented slice.

## Disposable ADDRESS large-file qualification profile

The current cREXX Classic libraries require more than the default 3 MiB CMS
input and 2 MiB STORE/output limits. The explicit `large-files` recipe profile
selects 16 MiB ordinary CMS input and STORE objects on a fresh 512-cylinder
3390. It reserves a 325-cylinder STORE extent: eight slots, two 911-block banks
per slot, and one immutable descriptor record, using F18452 records.
`PDSTLAY2` identifies the layout and checks version, block size, bank stride,
slot count and limit before bank access. Old or mismatched media is refused
before writes. This is a bounded bank-layout prerequisite, not the proposed
extent-allocated STORE v2 or an in-place migration.

Both recipes must select the same profile. `two-space-next.crexx` takes
`normal large-files` as its final two arguments; the bare
`two-space-ipl.crexx` route takes `command PCOMM-STAGE large-files`. The native
dataset/channel admission, checked installer, producer and stopped-media
oracle agree on 512 cylinders. The independent operation-37 first-track
transfer retains its existing contract. The stopped console actor selects
100 or 512 cylinders from the independently recorded initial STORE geometry.
Other historical Lab helpers that enforce 100 cylinders need an explicit
larger-profile path before use.

CMS output is padded to F18452 blocks. Its largest fitting framed logical
length is 16,772,868 bytes, including the 64-byte envelope and record lengths;
the remaining 4,348 bytes below 16 MiB do not form a complete output block.
Ordinary-file framing also counts against input capacity. STORE tombstones
retain their key slots. A fresh B/Classic chain plus transcript uses seven
keys per personality, so qualify CMS and TSO on separately fresh STORE state.
EOF-only or labelled blank physical placeholders make generated names visible
to the current directory views; they must never contain an old valid result.

The larger profile still uses whole-file backing. Four CMS output slots can
reserve 64 MiB; eight TSO output slots can reserve 128 MiB per invocation,
in addition to heaps, input files, child images and loader workspace. The
512 MiB machine is a selected qualification profile, not proof that arbitrary
depth with every file open fits. Measure peak storage and caller-resource
recovery during the real script campaign. Managed/default images are unchanged.

The [12 October ADDRESS campaign](../qualification/ADDRESS-CREXX-2026-10-12.md)
now qualifies this bounded prerequisite with two B and two Classic lifetimes
per personality, independent six-output/transcript/guard checks and normal
resource/fleet recovery. Observed full-workflow real-frame peaks are
340,123,648 bytes for CMS31 and 328,568,832 for TSO31 on the 512 MiB machine.
Those observations are not native application heap high-water measurements
or arbitrary maximum-depth capacity proof. Streaming, extent allocation,
slot/tombstone reuse and STORE-v2 migration remain the PD-030 roadmap.
