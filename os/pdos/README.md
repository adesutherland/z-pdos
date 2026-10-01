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
```

`check` verifies current source/patch hashes and tests actual source functions under
ASAN/UBSAN, with the pre-0005 implementation as a failing control. `compile`
prepares the maintained source/runtime selection and compiles all 17 native C
units to assembly text using the recorded flags and the new Classic compiler.
`inventory` counts spelled operations in ten source assembly/macro inputs
and those 17 outputs. Conditional paths and macro definitions remain unexpanded.
These commands do not produce an independently assembled or bootable OS.

For source recovery, supply a checkout or extracted archive containing the
pinned upstream files and a new output directory:

```sh
crexx os/pdos/recover.crexx --args /absolute/pinned-pdos build/pdos/recovered
```

Every selected upstream file is hash-checked before patching. The consolidated
patch must then reproduce the current maintained selection exactly. No mail download,
Lab build tree, proprietary native service or as370 is required for these
source, host-control and compile-to-text checks.

The [build plan](BUILD-PLAN.md) owns the remaining independent assembly,
runtime/name closure, link/image construction, boot and application gates.
The [dependency inventory](DEPENDENCIES.md) makes the existing native build's
IBM assembler/binder and macro interfaces explicit.
