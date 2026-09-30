# Classic assembler consumer inventory

30 September 2026. I approved an original C89 `mf-classic-as` seed and parallel
implementation/QA in `/Users/adrian/CLionProjects/z-pdos`. Maintained product
code and its own architecture, interfaces, user and bootstrap guides live there.
This migrated record pins consumer inputs for subsequent expansion; no source or
macro library was imported into the product from this inventory.

## Exact local inputs

| Input | SHA-256 | Role |
| --- | --- | --- |
| `runtime/tso/entry24.asm` | `b023f50f9cafcca085f5f8bd4b54cc2cef95ba707c311c21fa74d067f127b3ca` | 24-bit entry/stdio/storage adapter |
| `runtime/tso/entry31.asm` | `ad226ed2c1dd647fa22e532dc5f40662295d9b90b20e20ee1ae7d0af3fc983c8` | 31-bit, low-residence adapter |
| `runtime/tso/entry31-any.asm` | `0ce72e69ab14246cbc7ff0b9c935748f85bba66eb43b9c20c2d269d1c495594e` | 31-bit RMODE ANY adapter with extra mode checks |
| `runtime/tso64/entry64.asm` | `05e2475f2556f7d0abbd3be288129b7eb12c6c19f5f7fde87464cc20bbf2d0ac` | Real 64-bit application adapter |
| `runtime/tso64/entry64-any.asm` | `e0d201ee678bf8f2e322098ec4a51fb812eb28a705e1e8efe3ce5023ecdd4c3f` | Wider residence variant |
| `runtime/tso64/entry64-high.asm` | `49fcd9aaa2bb41adbeb482aef1de741f4afb211c5db85d0d85d09c3e4008a2d2` | High-residence variant and load/delete path |
| `vendor/pdos/pdpclib/mvssupa.asm` | `27758af986baae46fb726fe9bb35ed993898b98d547e315587328940c2f41e31` | Unchanged PDPCLIB upstream native support |
| `vendor/pdos/s370/ploadsup.asm` | `4bb345cfe1cc663177cd890add07d659323f85cd03f1e6c2bfb240ba2ad9cd98` | Loader native support |
| `vendor/pdos/s370/pdossup.asm` | `bc4d429b75ae41e73fcf04e25ef7503acd764c76517e1cc2f80f1c39ea1d4505` | OS native support |

The retained PDPCLIB preparation reference identifies PDOS revision
`0fe81209e78d022b40301f86f97c7f4d3e406d0a`, Paul Edwards's public-domain file
notice and contributor attribution. Its checked TSO candidate applies the three
retained patches and adds `entry31.asm` and `pdptop.mac`; that prepared variant
has its own source manifest. Qualification must choose that exact variant or
the untouched upstream source explicitly. The vendor directory is not an
independent Git checkout; the file hashes above, not a parent Git revision,
identify these local inputs.

The retained newlib configuration reference pins newlib
`4.6.0.20260123`; file notices and `COPYING.NEWLIB` remain authoritative.
The existing generic-C proof is a selected 58-object integer/stdio/allocator
subset, not all newlib or libm. Its native adapter path and libc C compilation
must not be conflated.

## Features and next consumer

The 24/31-bit entry adapters use ordinary RR/RX/RS/SI/SS instructions, branch
aliases, BASR, USING/DROP, symbol arithmetic, literals/LTORG, constants, mode
metadata and storage/terminal/dynamic-allocation definitions. This is a suitable
first small real consumer after the seed. The basic object writer must retain
section, entry and relocation meaning through a compatible downstream linker.
Macro/service definitions must have separately reviewed provenance or original
source-owned simplified counterparts; no private native expansion is an import.

MVSSUPA adds multiple sections/entries, DSECT, traditional macros, COPY,
conditional assembly, ORG and assorted layout/service dependencies. The loader
and OS support add privileged instructions, PSWs, channel words and selected
later-machine operations. A whitespace token scan alone cannot inventory these
sources reliably: continuations, macro prototypes and comments need semantic
review. This record intentionally does not turn such a scan into coverage counts.

The 64-bit adapters require selected real z/Architecture instructions and an
explicitly qualified object/relocation route. A classic 80-byte writer supporting
24-bit record addresses and four-byte RLD fields is not that wider route merely
because absolute AD constants are eight bytes.

The GNU `.s` adapters under `runtime/cms/` and `runtime/pdos390/`, and the
GNU `.S` PDOS64 context proof, remain distinct ELF source inputs. No automatic
retagging or GNU assembler rewrite is authorised. Classic compiler-generated
assembly also needs an exact producer/input pin before whole tool rebuilding
can be claimed.

The [approved plan](PLAN.md) separates initial seed,
whole-consumer assembly, native hosting and source-to-IPL. Existing ASMA90/XF
qualification and accepted runtime evidence are retained; no guest runs are
repeated for this local original implementation.
