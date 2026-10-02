# Build and use z/PDOS

Run from the repository root with cREXX, CMake utilities, Clang, shasum and
the maintained Classic compiler, assembler and linker:

```sh
crexx -nokeep compiler/scripts/build.crexx --args test mvs
crexx -nokeep scripts/build.crexx --args test
crexx -nokeep pdos/scripts/build.crexx --args check build/pdos/my-check
crexx -nokeep pdos/scripts/build.crexx --args compile build/pdos/my-compile
crexx -nokeep pdos/scripts/image.crexx --args build/pdos/my-image /absolute/hercules/bin
```

The image output directory must be new. Hercules utilities must provide
dasdload, cckd2ckd and ckd2cckd. The recipe compiles 17 C units, assembles six
handwritten modules and links fresh PLOAD, PDOS and PCOMM without native objects.
It runs the actual load-module reader against independent flat links at two
bases, checks entry/modes/relocations/padding and rejects malformed controls.
The source-function and loader/image host checks use ASAN/UBSAN.

PDPCLIB supplies the shared runtime with explicit pdos-zarch configuration.
Current sources are staged under ignored build/ without patches. Build receipts
record live source identities. Small pre-repair function fixtures retain the
old write/console failure controls; positive checks extract current functions.

The recipe constructs a new 100-cylinder 3390 with target-encoded CONFIG.SYS,
source-described IPL records and checked dataset bytes. Compression readback
checks every guest byte, allowing only the Hercules container serial to vary;
five corruption controls must fail. Output is media/pdos00.cckd with input,
output and version receipts. Creating it does not access a running guest.

The existing [0.1 guest record](../qualification/QUALIFICATION.md) remains exact
historical qualification. A changed source/tool candidate requires appropriate
affected checks. Guest operation belongs to Mainframe Lab and its leases.
See [the build contract](../development/BUILD-CONTRACT.md),
[dependencies](../architecture/DEPENDENCIES.md) and [known issues](../BACKLOG.md).
