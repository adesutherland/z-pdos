# Host QA: 30 September 2026

The focused standalone CMake suite passed 33/33 tests in both normal and
AddressSanitizer/UndefinedBehaviorSanitizer builds on native arm64 macOS with
Apple Clang 21.0.0. The linker uses C99. The original fixture generator/checker
uses C90 with `-Wall -Wextra -Werror -pedantic-errors`. An independent reviewer
also passed a separate original mixed-origin deck at image bases zero and
`0x1000` with both binaries.

The retained recipe is:

```sh
crexx tools/classic-ld/check.crexx --args test
crexx tools/classic-ld/check.crexx --args sanitize
```

The 33 tests include fixture generation, twelve link/check pairs and eight
malformed-input failures. Independently expected results cover nonzero target
and source origins, a zero-origin target appended after another section,
nonzero PC origin in a flat binary, split TXT records, sparse gaps, exported
LD identities, positive and negative A4, V references, two image bases, and a
separate caller. MVS checks reconstruct every text byte and inspect RLD
positions, identifiers and signs. Negative controls cover TXT/RLD bounds,
LD before the original origin and invalid or unused ESD identifiers.

The separate review adds a mixed PC/SD/LD fixture, A-minus-A at the same field,
eight-byte A relocation and a zero-origin PC appended after a nonzero SD.
This review reuses unaffected writer checks and does not qualify additional
object formats.

## Retained PDPCLIB package audit

The changed linker was run against the exact previously audited three-object
package. These native reference objects remain outside Git. SHA-256 identifies
the inputs independently of their local paths:

| Input | SHA-256 |
| --- | --- |
| Original maintained TSO31 entry object | `492d89c81beb8f85655c67723d1898652c5a7a8f1aec3bffb81ec2132f81d0d5` |
| Retained C/newlib program object | `4c2f839ec21c027933f604aee1ca11cfc6a494052d36d0ab9a6e4142eafefe03` |
| Retained native PDPCLIB support object | `79c232b808201729d54d62f5ba1017dee5fc68decb6ae4790141cf819509f182` |

The audited normal linker binary, built in `build/classic-ld-origin/`, has
SHA-256 `e3bbcb261f15fe04177bf2bf8671e69bbf6d482e663daa6ecd726a0ca4b8c5ee`.
The focused sanitizer binary in `build/classic-ld-origin-sanitize/` has SHA-256
`044a5e0189fa22dbf7d59a40edbcba3ea33bb7d6c50331e10206fb3996a7f35b`.
These are local Debug build identities, not release identifiers.

The package used `--oformat xmit --amode 31 --rmode 24 -e LABTSO`, member basename
`PLCORE.XMI`, and `SOURCE_DATE_EPOCH=1790405192`. Its SHA-256 is
`dc422d41b8cf1ea3d923303edc8348898998c4527be1a54477dbd9b92467f913`;
the physical output is 243,840 bytes and the padded linked image is 197,200
bytes.

The input `@@PCLST` SD starts at `0x3df0`; its referencing A4 field at input
address `0x3700` contains the nominal value `0x3df0`. The linked field is at
`0x2fb48` and the mapped target is `0x30238`. The corrected value is
`0x3df0 + 0x30238 - 0x3df0 = 0x30238`, replacing the old `0x34028`.

This audit reused the prior complete independently expected image and checked
the exact field delta. All CESD and RLD records, all 4,573 relocation positions
and every image byte outside this four-byte field are unchanged. The complete
XMIT differs at exactly two byte offsets, 200441 and 200442. The input relocation
counts are 13 entry, 4,482 C/newlib and 78 native support. The one previously
failing field now matches, so all 4,573 address fields match the reused
expectations. The local receipt is
`build/review/pdld-origin-investigation/fixed-image-audit.json`.

## Scope limits

This is host reader/linker/package QA. There was no guest run, native library
regeneration or complete compiler/runtime qualification. No service was removed
to avoid the pointer defect. The inherited all-named-SD MVS/XMIT writer and
nonzero-PC MVS map/CESD routes remain outside the qualified route. The inherited
output-member basename rule is documented in the user guide. Other inherited
backends were compiled but not qualified by these tests.
