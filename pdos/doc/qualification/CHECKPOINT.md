# z/PDOS 0.1 source and build checkpoint

2 October 2026. **One maintained OS/runtime selection builds, boots and
passes the unchanged cREXX TSO31, TSO64 ANY and TSO64 HIGH qualification.**
[QUALIFICATION.md](QUALIFICATION.md) records the actual guest results and
scope. TSO24 is tested but requires the low-residence path in
[BACKLOG.md](../BACKLOG.md).

This is the historical 0.1 checkpoint before the repository reorganisation.
Its source-layout and recovery-patch identities describe that accepted input;
current builds use pdos/src/ and pdpclib/src/ directly. The unchanged
qualification-0.1.json retains the original receipt. [The migration record](../../../doc/REORGANISATION-20261002.md)
records the equivalent new layout and checks.

The source and reached service repairs are committed as
[`f9b7b1f`](https://github.com/adesutherland/z-pdos/commit/f9b7b1f2ea89fd492fef205db662bbdfa4164585).
Git preserves the original exact PDIO1 input at
[`d62b109`](https://github.com/adesutherland/z-pdos/commit/d62b109ed986bd455732484173ff4ebe0533045a)
and the earlier complete host-build checkpoint at
[`6aaaba8`](https://github.com/adesutherland/z-pdos/commit/6aaaba84757c2b00d7edb3a06a73dc4edf9c8d57).
There is no second maintained runtime or OS source tree. That checkpoint selection
contains 37 OS and 44 runtime/header/macro files, 1,401,728 bytes.

| Checkpoint record | SHA-256 |
| --- | --- |
| Maintained source manifest | `d164be3e35af7804f6af111ff011bffa7904c8145434d9e71ce952be942b6b8d` |
| Selected canonical upstream manifest | `53b98cece9972a9589d54407de4094119a5a40a1f725cbc2d3b85a3765d2d813` |
| Prepared-layout manifest | `9d4121d33d1700045cce1d1b6e2f631f8cb2f386f4c952dfe6d4e4080e559ca1` |
| Consolidated recovery patch | `5c70af3bae08256005c8ced34f0d3bd213102148825590e1f79abdcde44a4bff` |

At this checkpoint, clean selected upstream source plus the consolidated
recovery patch reproduced all 81 files exactly. That recovery workflow is now
retired in Git history. Current preparation selects the maintained PDPCLIB
source modules and explicit pdos-zarch configuration without applying patches.

The C32 kernel/runtime now run in AMODE31; native AMODE64 tasks retain their
full-width contexts. Console and callback returns preserve the selected mode,
and PCOMM's RMODE ANY metadata matches its above-16-MiB loader placement.
Implicit standard-stream DDs are accepted as terminal allocations. Shared
PDPCLIB SVC99 helpers flag the last actual text-unit pointer. The direct
loader has a checked 8-MiB capacity and rejects unsupported addressing/residence
combinations before dispatch. Conditional FREEMAIN returns success in R15
after freeing storage. These are generic reached OS/library contracts.

Normal and sanitizer source-to-image builds pass; 23 objects, three RDW
modules, six flat links and every guest disk byte are identical, allowing
only Hercules's regenerated container serial. Real loader reconstruction at
two bases, complete dataset/IPL/compression readback, malformed-load controls
and five media corruption controls pass. Focused normal/sanitizer suites pass
11/11; the old SVC99 list format fails both STDIO controls. The earlier broad
assembler results are reused because no assembler core changed in this increment.

The base disk has 85,248,512 bytes, SHA-256
`82e18020e9c3120de66bb0a160ca701b2e67a274ecad58ee23a4b8fca9574acf`.
The normal compressed image has SHA-256
`b7577800d08f91ed71f2bd0c5787f740a17bad9005291c24bb7c1a58b1e0a763`.
[qualification-0.1.json](qualification-0.1.json) retains exact tool and payload
identities. The maintained build recipe is `pdos/scripts/image.crexx`; generated objects,
listings, disks and raw operator receipts remain outside Git. No as370 is used.
