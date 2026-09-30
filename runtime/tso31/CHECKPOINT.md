# TSO31 entry consumer checkpoint

30 September 2026. I approved qualifying the entry adapter first, then PDPCLIB
and PDOS support. The completed bootstrap was pushed as
[`e87652d2bc1efdee4439302be4420a5c3ac8911e`](https://github.com/adesutherland/z-pdos/commit/e87652d2bc1efdee4439302be4420a5c3ac8911e)
before this work. I approved committing and publishing the independently
reviewed consumer changes described here.

## Source and host assembly

The maintained [entry source](entry31.asm) retains the project-authored
reference's behavior with nine explicit service replacements and six EQU
definitions. The [service notes](SERVICES.md) cite public IBM interface facts;
no IBM macro definition or native expansion was imported. The assembler adds
selected BC/BCR aliases, bounded F/X/V literal pools, implicit V externals and
SS operands with an inferred USING base. These remain separate from the optional
traditional macro provider.

| Input/output | SHA-256 |
| --- | --- |
| Exact reference source | `ad226ed2c1dd647fa22e532dc5f40662295d9b90b20e20ee1ae7d0af3fc983c8` |
| Maintained entry source | `96a0d596232da7f79fdd972fa55c9c33b90f33ec45223b4a3e6883ccde8797df` |
| Classic object deck | `492d89c81beb8f85655c67723d1898652c5a7a8f1aec3bffb81ec2132f81d0d5` |

The actual source assembles under the s370 instruction profile: 263 statements,
58 symbols and 13 fixups. The 55-card, 4,400-byte deck describes one 1,168-byte
LABTSO section, AMODE 31/RMODE ANY, six external identities and seven local A
plus six external V relocations. It reserves 96 bytes without manufacturing
text for them. The original macro-calling reference is rejected as unsupported;
the maintained BASR-using source is rejected under s360. Neither failure leaves
a successful object.

Apple Clang 21.0.0 strict C89/C90 and ASan/UBSan integrated runs each passed all
25 registered checks. The engine consumer suite passed 2,561 assertions; the
unchanged bootstrap engine regression suite passed 2,881. Source comparison
passed 1,571 checks over all 237 original executable statements, and the host C
caller passed 29 behavior checks. Those host callbacks do not prove the target
ABI or native services.

The independent deck checker passed 3,248 checks in both normal and sanitizer
builds. It verifies section/modes, all reserved gaps, all relocations, service
register setup, native file linkage, callback preservation, the complete literal
pool and all 256 translation octets. The implementer rejected 18 deliberately
damaged decks. The coordinator reviewed the source/object contracts and separately
rejected altered section mode, allocation size, translation byte and relocation
target controls on the exact assembled deck.

## Target C layout and caller

The native macOS cross compiler identified itself as GCC 16.2.0; its executable
SHA-256 was `9bb428866e7d1506335d8f294fb0f40929b0223e82c8977772b1188f51743a89`.
The selected binutils 2.47 target assembler produced ELF32, big-endian,
EM_S390 (22) relocatable objects. Target compilation passed every C89 size and
offset assertion in [target_layout.c](target_layout.c), including four-byte
integers/pointers/function pointers and the 28-byte version-4 service vector.

The actual [caller](caller.c) also compiled with strict C89 diagnostics and
`-mexperimental-s370 -m31 -mesa -msoft-float`, freestanding/no-PIC settings.
Its disassembly uses R2/R3/R4 for arguments, BASR R14 for callbacks and R2 for
the return value, consistent with the selected ELF caller convention. This
caller object has not been linked or executed; the package experiment below
uses the separately retained accepted C/newlib program instead.

| Target output | SHA-256 |
| --- | --- |
| Layout assertion object | `c1f3b2927c91d2391d7efd19ed2b06ce6ebef0a87ae44682a970cb05f65366b3` |
| Small C caller object | `4b337d6ac68793d422eb972304bc472efebaddd0c21fa73f8913ff6e0dd0d157` |

The repeatable compile-only recipe is
[tools/check-tso31-target.crexx](../../tools/check-tso31-target.crexx).
It takes the compiler and two explicit tool directories, places the unprefixed
target binutils directory before the compiler's own assembler-search prefix,
and writes only build outputs. It neither rebuilds nor changes the SDK.

## Downstream link experiment

We reconstructed the retained public-domain PDLD reference source revision
`a65eddb9ef4b27a6844f2857db0c98696137612b` with the five existing SDK linker
patches in separate private storage. All 76 patched source identities matched
the retained split-patch proof. Native Apple Clang compiled 39 C units; the
writer SHA-256 is `e3d51fda0d4af44050d4126a8ec9037b5c4248af6cb69f09d3bfda8cf5ce41a8`.
This is a new macOS reference prerequisite, not an imported maintained linker
or a new Linux qualification. Its source and output packages remain outside
this repository.

The original-entry control reproduced the retained accepted XMIT exactly:
243,920 bytes, SHA-256
`3f78f7b714015892dcbcba94fb2eb35bf3421d707026048720f7cfa2a88bca03`.
Replacing only that entry with our deck linked the retained C/newlib and PDPCLIB
native inputs with entry LABTSO, AMODE 31/RMODE 24 and epoch 1790405192. Two
candidate packages were identical: 243,840 bytes, SHA-256
`b98784b1c32e970a085b3ed1d0d2d6220c1e8c3ada6686b1497ee454750596c3`.
The coordinator independently read back those hashes, sizes and repeat identity.

The independent link audit matched all 4,573 emitted relocation positions to
the three decoded inputs: 13 entry, 4,482 C/newlib and 78 native fields. All
entry and C fields, and 77 native fields, contain the correct linked values.
The entry's 1,168-byte span and all six external references survive the link.
The coordinator separately extracted both module images, checked all 13 new
entry address words against fixed expected values, and confirmed the one
inherited discrepancy below. XMIT framing, end-of-member, symbol closure and
AMODE 31/RMODE 24 also pass. This closes the entry-only host link gate.

An independent relocation audit found a nonzero-origin address problem in the
unchanged PDPCLIB input/link path. The candidate maps @@PCLST to `0x30238`,
but its referring field contains `0x34028`, adding the original `0x3df0` origin
twice. Reproducing the accepted control does not qualify that field. The
original-entry control has the same defect: expected `0x30290`, stored
`0x34080`. The source uses that pointer for a write and the IKJPARS prefix-parser
control list; the erroneous linked value would direct the write outside the
module. The earlier DD-oriented core checks do not
exercise that path. Whole-module correctness remains blocked by this inherited
input/linker defect. No guest execution or linker repair was performed.

## Remaining work

Entry source, host object semantics, target C layout and the entry-only host
link gate are checked. We must close the inherited nonzero-origin issue before a whole
linked-image claim or execution gate. The small caller's link/run is also open.
I selected MVS 3.8 / real System/370 / AMODE 24 and RMODE 24 for the first
complete PDPCLIB qualification. Its configuration member still needs to be
implemented. Full assembly also needs ordinary language features, source-owned
traditional macros and independently supplied service/mapping definitions.
[The native-support inventory](../../tools/classic-as/docs/NATIVE-SUPPORT-INVENTORY.md)
records those needs and the later loader/kernel steps. Native assembler hosting
on z/PDOS and source-to-IPL remain separate work.
The [first PDPCLIB language checkpoint](../../tools/classic-as/docs/PDPCLIB-CHECKPOINT.md)
now closes selected ordinary layout changes and the optional substitution
provider. Its final engine still produces this exact entry deck and passes all
3,248 independent checks; unchanged target/link evidence is retained.
