# Repaired z/PDOS — PDIO1

This component maintains the repaired OS source used for the 1 October 2026
cREXX beta 3 HIGH qualification. PDIO1 is the 31-bit, RMODE24 kernel with
standard z/Architecture support, native 31/64-bit application contexts,
high application loading, checked DASD writes and bounded 3270 wrapping.
The [source record](SOURCES.md) identifies the upstream and retained changes;
the [checkpoint](CHECKPOINT.md) separates the accepted Lab baseline from
the new local compiler results.

The OS source lives in `source/s370/`. Its 44 runtime/header/macro inputs
come from the one maintained [PDPCLIB](../../pdpclib/README.md), using the
explicit `pdos-zarch` configuration. [runtime-inputs.txt](runtime-inputs.txt)
defines that selection. The original qualified input is preserved by commit
[`d62b109`](https://github.com/adesutherland/z-pdos/commit/d62b109ed986bd455732484173ff4ebe0533045a).
The merged runtime is a new build candidate and needs its own guest acceptance.
Unrelated runtime ports, the upstream saved emulator log and private native
artifacts are outside this import. The retained upstream batch/configuration
files describe historical routes; the active preparation recipe is cREXX.

Run from the repository root with cREXX, CMake file utilities, Clang and
`shasum`. Build [Classic C](../../tools/classic-cc/README.md) before compiling:

```sh
crexx os/pdos/build.crexx --args check
crexx tools/classic-cc/build.crexx --args build mvs
crexx os/pdos/build.crexx --args compile
crexx os/pdos/inventory.crexx --args build/pdos/pdio1
crexx os/pdos/assemble.crexx --args build/pdos/classic
```

`check` verifies current source/patch hashes and tests actual source functions under
ASAN/UBSAN, with the pre-0005 implementation as a failing control. `compile`
prepares the maintained source/runtime selection and compiles all 17 native C
units to assembly text using the recorded flags and the new Classic compiler.
`inventory` counts spelled operations in ten source assembly/macro inputs
and those 17 outputs. Conditional paths and macro definitions remain unexpanded.
`assemble.crexx` repeats compilation and independently assembles all 17 C
units using the documented language subset and an explicit literal bound.
`assemble-support.crexx` assembles all six handwritten modules with the explicit
z900 kernel ISA ceiling and the named PDOS service configuration.

Build a complete new private disk image after building both Classic tools
with `crexx tools/build.crexx --args test`. Supply an existing Hercules
utilities directory containing `dasdload`, `cckd2ckd` and `ckd2cckd`:

```sh
crexx os/pdos/image.crexx --args build/pdos/my-image /absolute/hercules/bin
```

The output directory must be new. The recipe checks source identities and the
actual repaired-function controls, compiles 17 C units, assembles six support
modules, and links PLOAD, PDOS and PCOMM without native objects. PDOS's actual
load-module reader reconstructs each RDW module against a separately linked
flat image at bases zero and 2 MiB; entry, AMODE31/RMODE24, final padding and
malformed-input controls are checked. The host checks run under ASAN/UBSAN.

The recipe places fresh programs and a target-encoded `CONFIG.SYS` on a
100-cylinder 3390, installs source-described IPL1/2 CCWs, and compares every
dataset byte and final zero padding to the linked inputs. Compression readback
checks every guest byte; only Hercules's regenerated 12-digit container serial
may differ. Five one-byte corruptions must fail, and existing image outputs
are preserved. `media/pdos00.cckd` is the compressed candidate;
`outputs.sha256`, `inputs.sha256`, logs and version receipts identify the run.
These generated files stay outside Git. No running guest is accessed.

For a sanitizer assembler/linker run, first run `crexx tools/build.crexx
--args sanitize`, then add `sanitize` as the third recipe
argument. The compiler source and selected OS/runtime inputs are the same.
Disk construction establishes a built candidate; boot and application
qualification are still separate gates.

For source recovery, supply a checkout or extracted archive containing the
pinned upstream files and a new output directory:

```sh
crexx os/pdos/recover.crexx --args /absolute/pinned-pdos build/pdos/recovered
```

Every selected upstream file is hash-checked before patching. The consolidated
patch must then reproduce the current maintained selection exactly. No mail download,
Lab build tree, proprietary native service or as370 is required for these
source, host-control, compile, assembly, link and disk-build checks.

The [build plan](BUILD-PLAN.md) owns the separate build, boot and application gates.
The [dependency inventory](DEPENDENCIES.md) makes the existing native build's
IBM assembler/binder and macro interfaces explicit.
