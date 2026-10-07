# From source to a z/PDOS 0.2 image

The default `image.crexx` recipe builds the protected K64/C31 kernel and U
PCOMM, then constructs a fresh 100-cylinder 3390. Maintained source supplies
all mainframe code and macros. No prebuilt mainframe objects, proprietary
compiler/assembler/binder or IBM macro library is required. Run recipes from
the repository root; outputs belong under ignored `build/pdos/`.

## Source and tools

| Input | Owner and purpose |
| --- | --- |
| PLOAD and native load reader | `pdos/src/pload.c`, `ploadsup.asm`, `pdosutil.c`, `install-ipl.c`. |
| K64 nucleus | `twospace_normal.S`, `twospace_entry64.inc`, `twospace_nucleus64.inc`, built by GNU s390 assembler/linker under the z900 instruction ceiling. |
| C31 K and handover | `twospace_boot.c`, `twospace_service.c`, DAT, gate, memory, placement, invocation, channel, native-file and console source; maintained Classic C/Assembler/Linker. |
| U command processor | `pcomm.c`, `twospace_ui.c`, native entry and checked K service linkage. |
| Shared runtime and macros | Maintained `pdpclib/src/`, selected `pdos-zarch` modules and Classic linkage interfaces; no copied runtime or patch stack. |
| Host orchestration/checks | cREXX, CMake, native C tools, make/Bison/Flex for Classic C, Clang sanitizers, Python for explicit binary/emulator interfaces, and `shasum`. |
| Disk utilities | Hercules `dasdload`, `cckd2ckd`, `ckd2cckd`; the emulator is a separate guest-test input. |

## Build sequence

1. `two-space-normal.crexx` builds the existing bootstrap/runtime links through
   `one-space-image.crexx`. That producer is also the explicit legacy-image
   route; its intermediate PDOS does not become the 0.2 kernel.
2. `two-space-command.crexx` builds U PCOMM and its C presentation/native stage.
3. `two-space-next.crexx ... normal` builds the K64 nucleus, protected C31
   services and sparse K/U tables into the normal 4 MiB core. Its normal route
   performs host checks and creates the core; it does not start Hercules.
4. `two-space-ipl.crexx ... command` builds the C31 handover, packs the core,
   stages U PCOMM and the durable store, installs explicit IPL records, and
   compares the disk after compressed readback. The bare route does not start
   a guest or require a cREXX application package.

The final disk is `<work>/image/media/pdos00.cckd`. It contains PLOAD.SYS,
PDOS.SYS, CONFIG.SYS, COMMAND.EXE, KCORE.BIN, U.COMMAND and PDOS.STORE. The
normal build root carries combined `inputs.sha256`, `outputs.sha256` and
`versions.log`; base/kernel/image manifests retain detailed producer inputs.
The root VERSION and guest `zpdos-version.h` must agree.

`package-image.crexx` packages that K/U disk, its producer manifests, licence,
attribution and operator/qualification guides. The hosted Linux build supplies
GNU s390 Binutils explicitly and packages this same default route. Host tool
packages are built on four hosts; that does not establish a guest run on each.

## Evidence and limits

Compile, assembly, native reconstruction, disk checks and guest execution are
separate results. The [P6 record](../qualification/TWO-SPACE-P6-2026-10-07.md)
qualifies the reviewed K/U workloads and media. The
[0.2.0 release record](../qualification/RELEASE-0.2.0.md) distinguishes the exact
operator candidate from hosted release disk containers and host packages.
A different container hash alone does not prove changed native payloads;
compare the recorded source, core and dataset bytes before reusing a result.

The explicit legacy producer retains its original 17 C units, six handwritten
modules and three links, documented by the dated 0.1 qualification reports.
Earlier GCCMVS/IBM ASMA90/IEWL comparisons and frozen archives are provenance,
not current build prerequisites. See the [user guide](../user/README.md) for
commands and the [licence map](../../../LICENSES.md) for component terms.
