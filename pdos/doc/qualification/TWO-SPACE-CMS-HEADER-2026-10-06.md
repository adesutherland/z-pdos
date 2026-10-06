# Two-space CMS24 MODULE header checkpoint, 6 October 2026

The successor's disposable 3390 image now optionally carries the pinned
unchanged `CMS24.RXVM` stage. The input is the checked staged RXVM MODULE
from the CMS exchange recipe, SHA-256
`313367ea0b4d90aec256720dcbca253116229e717534e0bd194ae23ce927fa88`.
The cREXX IPL recipe verifies that hash before adding the dataset. The normal
five-dataset successor fixture remains available without this extra input.

K's Classic C31 SVC 212 finds `CMS24.RXVM` through the checked VTOC/first
extent path, reads the first F/18452 block through K's low-real channel
buffer, and checks the stage magic, profile, length, record count and MODULE
header placement. The fresh guest returned origin `0x20000`, end
`0x1ba6c0`, entry `0x20000` and 1,681,222 MODULE transport bytes. The
image length is 1,681,088 bytes, or 1,683,456 bytes rounded to pages.
The host C89/ASAN/UBSAN parser also accepted the actual staged CMS31 RXVM
header: origin and entry `0x02200000`, image length 4,238,296 bytes and
three relocation records. Its host check does not establish a guest load.

The diskless machine gate passed 81 checks. The ordinary disk without the
optional module passed 93 fresh IPL checks with a missing-dataset result.
The fresh IPL with the pinned CMS24 stage passed 94 checks, including the K header result, terminal
input/output, dataset and storage services, DAT isolation, fault recovery
and unchanged disk hash. This is one ESAME model-2064 CPU, 16 MiB real,
z900 target ceiling, 3390 at `01B9` and 3270 at `0009`, on macOS arm64
using Mainframe Classic C/Assembler/Linker, GNU Binutils 2.47 and Hercules
4.9.1.0-SDL. Exact source, stage and tool inputs are in ignored
`build/pdos/two-space-s6-cmshead-b/run/receipt.json` and
`build/pdos/two-space-s6-cmshead-final-ipl/inputs.sha256`; the IPL result is in
`build/pdos/two-space-s6-cmshead-final-ipl/run/receipt.json`.

| Artifact | SHA-256 |
| --- | --- |
| Source-built 2 MiB core | `c55976207ec14f455d0505521ee64b786d41ad0a80a881c64de8e86f9296547e` |
| Classic C31 service, 28,934 bytes | `563e37836461e065bd91791f6458820ed986d5f0f8951f91a9f3f3acf14563fd` |
| Checked 3390 CCKD disk | `305d91fc4bfda05d72403a33af955fd2fbaa38bb0b8bbe0120f853aea1a00d5e` |

This checks only the first staged block and MODULE header. The guest has
not consumed every MODULE record, verified the entire payload digest, loaded
the image into U, allocated its stack/heap, or executed an unchanged CMS
application. TSO24 headroom, TSO services, native same-personality calls, collision recovery
and normal successor selection remain open in PD-003 slices 5–7.
