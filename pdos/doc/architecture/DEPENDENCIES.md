# From source to a z/PDOS image

The active build takes maintained source through compilation, assembly, linking
and disk construction. It uses no prebuilt mainframe objects or proprietary
mainframe build tools. The recipes run from the repository root and write
under ignored `build/pdos/`.

The separate [two-space successor](TWO-SPACE-POC.md) uses
`two-space-next.crexx` and `two-space-ipl.crexx` to build a diagnostic K64
core, Classic C31 endpoint and disposable `KCORE.BIN` IPL disk. Those inputs
are outside the active three-link `image.crexx` route below. The latest
[storage checkpoint](../qualification/TWO-SPACE-STORAGE-OVERLAY-2026-10-05.md)
qualifies that fixture, not a replacement `PDOS.SYS` image.

## Source inputs

| Input | Owner and purpose |
| --- | --- |
| Four OS C units | `pdos/src/`: `pload.c`, `pdos.c`, `pcomm.c`, `pdosutil.c`. |
| Thirteen common C units | `pdpclib/src/`: START, STDIO, STDLIB, CTYPE, STRING, TIME, ERRNO, ASSERT, LOCALE, MATH, SETJMP, SIGNAL, MEMMGR. |
| Six handwritten assembler units | OS PLOADSUP and PDOSSUP; PDPCLIB SAPSTART, SAPSUPA, MVSSTART and selected MVSSUPA. |
| Configuration and linkage macros | Selected PDPTOP, PDPMAIN, PDPPRLG and PDPEPIL, plus the original selected service/linkage interfaces under `pdpclib/src/interfaces/`. |
| Host tools | Maintained Classic C MVS, Classic Assembler and Classic Linker; cREXX orchestration, ordinary native build tools, Clang for host checks, and Hercules disk utilities. |

[`compile-inputs.txt`](../../scripts/compile-inputs.txt) names the 17 C units.
[`assembly-inputs.txt`](../../scripts/assembly-inputs.txt) names six handwritten
modules plus four macro/configuration inputs. Those ten input rows do not mean
ten additional objects: the full route produces **23 objects**.

PDPCLIB preparation selects `pdos-zarch` directly from maintained source.
`MVSSUPA` is assembled from its common and profile-specific modules; preparation
does not apply a patch stack. The generated C assembler text uses the selected
PDPCLIB linkage macros, literals, external references and page tables. The
runtime also contains IBM hexadecimal floating-point code, so an integer-only
ELF SDK contract cannot be substituted for this entire library.

## The three links

| Program | Startup and native support | Additional code |
| --- | --- | --- |
| PLOAD | SAPSTART, START, SAPSUPA | Common runtime, PLOAD, PLOADSUP, PDOSUTIL. |
| PDOS | SAPSTART, START, SAPSUPA | Common runtime, PDOS, PDOSSUP, PDOSUTIL. |
| PCOMM | MVSSTART, START, MVSSUPA | Common runtime and PCOMM. |

[`link.crexx`](../../scripts/link.crexx) writes the native load modules and
independent flat links. It exercises the actual loader implementation at bases
zero and 2 MiB and compares reconstructed bytes, entry and mode information.
The loader/kernel use AMODE31/RMODE24; PCOMM uses AMODE31/RMODE ANY.

[`image.crexx`](../../scripts/image.crexx) then stages the flat PLOAD, native
PDOS and PCOMM images and explicitly encoded `CONFIG.SYS`. Hercules `dasdload`,
`cckd2ckd` and `ckd2cckd`, together with the source-owned IPL writer/checker,
construct a fresh 100-cylinder volume. Conversion explicitly preserves that
size. The recipe checks all dataset bytes, decompressed disk readback and
malformed/corrupt controls. It records input/output hashes and tool identities.

## What the evidence establishes

A C compile proves neither assembly nor guest execution. A reconstructed load
module proves neither a successful IPL nor application behavior. Host media
checks show that the intended bytes reached the image; the
[guest qualification](../qualification/QUALIFICATION.md) records boot and real
application results separately.

The [0.1.1 release run](https://github.com/adesutherland/z-pdos/actions/runs/37308481475)
built the four host packages and a fresh Linux-produced image. Its image
checks did not perform a new guest run. The
[local 0.1.1 operator acceptance](../qualification/0.1.1-OPERATOR-UAT-2026-10-05.md)
names a separate source-built and installed image. The earlier source-built
milestone and service work retain their own input identities.

Earlier investigations used GCCMVS 3.2.3, IBM ASMA90/IEWL and identified native
objects. Those are historical comparison inputs, not prerequisites of the
current build. Old names such as PDIO1 and SPSZ1 in records are experiment or
member identities, not additional missing source components. Frozen upstream
IPL material is provenance; the build does not read `archive/`.

See the [build guide](../user/README.md) for commands, the
[build contract](../development/BUILD-CONTRACT.md) for the recorded acceptance
stages, and the [licensing guide](../../../LICENSES.md) for source origins.
