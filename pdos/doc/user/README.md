# Run and build z/PDOS

The quickest way to explore the system is the **PDOS image ZIP** from the
[0.2.0 release](https://github.com/adesutherland/z-pdos/releases/tag/v0.2.0).
The Classic tools downloads are separate packages for building mainframe
software on your host computer.

The default source-built K/U route and its accepted scope are described in
[the K/U operator guide](TWO-SPACE.md).

## Boot the supplied image

You need Hercules and a 3270 terminal client on your computer. The recorded
guest qualification used Hercules 4.9.1.0-SDL, one z/Architecture CPU and
256 MiB of real storage. Other emulator versions need their own evidence.

1. Download the image ZIP and release `SHA256SUMS`. Verify the ZIP against the
   release checksum inventory. Extract it and keep an untouched copy.
2. Work in a separate writable copy of the extracted image directory. It
   contains `pdos00.cckd`, `hercules.cnf`, instructions and provenance files.
3. From that directory, start Hercules:

   ```sh
   hercules -f hercules.cnf
   ```

4. Connect your 3270 client to **0009@127.0.0.1, port 3270**, using model 2.
   Connect the configured Telnet 3215 monitor as **000A@127.0.0.1:3270**
   before IPL; see [terminal operation](TWO-SPACE.md).
5. In the **Hercules console**, enter `IPL 01B9`.

The supplied configuration selects:

```text
ARCHMODE z/Arch
CPUMODEL 2064
MAINSIZE 256
NUMCPU 1
CNSLPORT 127.0.0.1:3270
CODEPAGE 819/1047
0009 3270
000A 3215 noprompt
01B9 3390 pdos00.cckd
```

Run from the image directory so the relative disk path resolves correctly.
The 100-cylinder disk contains `PLOAD.SYS`, the `PDOS.SYS` handover,
`CONFIG.SYS`, `COMMAND.EXE`, the protected `KCORE.BIN`, U PCOMM in
`U.COMMAND`, and `PDOS.STORE`. It includes no cREXX application packages
or IBM guest system.

The [0.2.0 release record](../qualification/RELEASE-0.2.0.md) distinguishes
local operator acceptance from the hosted image archive and host packages.
The bundled P6 record describes its exact reviewed K/U workload evidence.
Container hashes and native payload hashes are separate identities.

## At the PCOMM prompt

PCOMM is the command processor. Begin with `HELP` for the first-run route,
`VERSION` for its release version, interface and build-receipt location, and
`DIR` to inspect datasets, creation dates, record formats and extents.
`HELP TSO` and `HELP CMS` explain the command and binary compatibility
boundary. `SHOWRC` toggles an
extra display of command return codes; every external command also prints
numbered `PCOMM BEGIN` and `PCOMM END ... RC=` lines.
Commands for editing raw blocks or initializing disks are development tools
and can change the disk; use only your working copy.

The prompt resembles a PC drive/path prompt, but the storage model is mainframe
datasets. The current `CD` handler does not implement directory changes, and
`REBOOT` is a placeholder. `EXIT` ends the primary command processor and lets
the kernel terminate; it is not a restart command.

PCOMM looks for a named `.BAT` file before forwarding an external command to
the kernel. The current source hides expected missing `AUTOEXEC.BAT` and
`NAME.BAT` probes, while retaining failures for present invalid datasets.
The 3270 entry field is shorter than a full PCOMM command. End a field entry
with `&`, press Enter, and enter the next fragment at `MORE>`; PCOMM joins the
fragments without inserting a space. You can repeat this up to 198 target
characters. End at most 79 characters per field with `&` included. An overlong
command is rejected before dispatch. Native batch lines have the same
198-character limit and use IBM1047 bytes with hex `15` line delimiters, not
a host UTF-8 file copied unchanged. Long output scrolls above a stable prompt
and editable input line; use a complete 3270 ScreenTrace when exact output
matters.

## Running applications

The accepted K/U route covers unchanged CMS31 and TSO31/TSO64 ANY/HIGH
cREXX compiler, assembler and VM chains. CMS24 and native TSO24 have separate
library-free IO24 acceptance; full-library TSO24 and CMS24 RXC/RXAS remain
outside it. CMS uses `CMS CHECK`/`CMS RUN` on the checked exchange disk;
ordinary TSO names use profile-specific native stages. The
[P6 record](../qualification/TWO-SPACE-P6-2026-10-07.md) names the exact inputs,
results and limits.

Use the application's exact packaging and installation instructions; an XMIT
transport or ELF object is not directly executable by the z/PDOS loader.
Application installation must preserve dataset structure and load bytes, and
offline disk updates require the guest to be stopped. The
[conformance candidate guide](CONFORMANCE.md) gives the checked installer and
script runner for the retained one-space disk profile. For K/U installation,
use the checked workload/image staging described in [TWO-SPACE](TWO-SPACE.md)
and the [release acceptance record](../qualification/RELEASE-0.2.0.md).
The base image has no general package manager; application installation and
its acceptance are separate from booting the base OS.

The [exchange disk and tape guide](MEDIA.md) covers the current source's
second CKD volume, guest `ALLOC` and `RCOPY`, and raw HET/AWS tape records.
The CMS route requires an unchanged checked release ZIP, a fresh exchange disk
made by `pdos/scripts/cms.crexx`, and `MOUNT`/`SELECT` of that disk. `HELP CMS`
shows the guest commands and their bounded profiles. Its guest result does not
extend to general CMS commands or every CMS application.
The [fixture guide](FIXTURES.md) covers checked CMS and TSO tape imports and
exact stopped-disk export. The base image includes no fixture packages.

## Build a fresh disk from source

Run these commands from the repository root. You need cREXX, CMake, a native
C toolchain, make, Bison/Flex for the inherited compiler build, Clang and
`shasum`, GNU s390 assembler/linker for the K64 nucleus, plus Hercules utilities `dasdload`, `cckd2ckd` and `ckd2cckd`.
The [host build guide](../../../doc/BUILD-AND-RELEASE.md) covers tool packages
and platform details.

Build the MVS compiler and supporting tools, then give the image recipe a new
output directory and the absolute directory containing the Hercules utilities:

```sh
crexx -nokeep compiler/scripts/build.crexx --args test mvs
crexx -nokeep scripts/build.crexx --args test
crexx -nokeep pdos/scripts/image.crexx --args build/pdos/my-image \
  /absolute/hercules/bin /absolute/s390-as /absolute/s390-ld
```

These are build-and-check recipes for developers: they include their own
component, loader and disk checks. The image recipe already invokes source
checks, compilation, assembly and linking; separate preliminary OS `check` and
`compile` commands are not required. It refuses an existing image output
directory and does not connect to a running guest.

The result is `build/pdos/my-image/image/media/pdos00.cckd`. Combined
input/output hashes and producer versions are at the build root; detailed
manifests are in `base/`, `kernel/` and `image/`. The normal build compiles,
assembles and links the bootstrap, K64/C31 core and U PCOMM, checks sparse
translation/ownership and loader behavior, and compares disk bytes after
compressed readback. It does not start a guest. A bare build needs no
application ZIP. Use `image.crexx ... legacy` for the explicit one-space
producer and its old `<work>/media/` layout.

See [build dependencies](../architecture/DEPENDENCIES.md) for the component
inputs and [development](../development/README.md) for how to change them.
A changed source/tool candidate needs evidence appropriate to that change;
reuse the dated guest record only for the scope it actually establishes.
