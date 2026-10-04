# Run and build z/PDOS

The quickest way to explore the system is the **PDOS image ZIP** from the
[0.1.0 release](https://github.com/adesutherland/z-pdos/releases/tag/v0.1.0).
The Classic tools downloads are separate packages for building mainframe
software on your host computer.

## Boot the supplied image

You need Hercules and a 3270 terminal client on your computer. The recorded
guest qualification used Hercules 4.9.1.0-SDL, one z/Architecture CPU and
4096 MiB of real storage. Other emulator versions need their own evidence.

1. Download the image ZIP and release `SHA256SUMS`. Verify the ZIP against the
   release checksum inventory. Extract it and keep an untouched copy.
2. Work in a separate writable copy of the extracted image directory. It
   contains `pdos00.cckd`, `hercules.cnf`, instructions and provenance files.
3. From that directory, start Hercules:

   ```sh
   hercules -f hercules.cnf
   ```

4. Connect your 3270 client to **127.0.0.1, port 3270**.
5. In the **Hercules console**, enter `IPL 01B9`.

The supplied configuration selects:

```text
ARCHMODE z/Arch
MAINSIZE 4096
NUMCPU 1
CNSLPORT 127.0.0.1:3270
CODEPAGE 819/1047
0009 3270
01B9 3390 pdos00.cckd
```

Run from the image directory so the relative disk path resolves correctly.
The 100-cylinder disk contains `PLOAD.SYS`, `PDOS.SYS`, `CONFIG.SYS` and
`COMMAND.EXE`. It includes no cREXX application packages or IBM guest system.

The release image passed host construction and readback checks. The bundled
qualification document describes the separate earlier source-built guest run;
it is not a claim that every newly generated image has been booted.

## At the PCOMM prompt

PCOMM is the command processor. Begin with `HELP` to see the commands and
`DIR` to inspect datasets. `SHOWRC` toggles display of command return codes.
Commands for editing raw blocks or initializing disks are development tools
and can change the disk; use only your working copy.

The prompt resembles a PC drive/path prompt, but the storage model is mainframe
datasets. The current `CD` handler does not implement directory changes, and
`REBOOT` is a placeholder. `EXIT` ends the primary command processor and lets
the kernel terminate; it is not a restart command.

PCOMM looks for a named `.BAT` file before forwarding an external command to
the kernel. Startup may report an absent `AUTOEXEC.BAT`. These expected probes
and genuine failures are not yet presented as clearly as they should be.
Console commands are limited to 80 columns; batch processing uses a 200-byte
buffer with at most 198 characters before the delimiter. Native batch text
uses EBCDIC with hex `15` newlines, not a host UTF-8 file copied unchanged.
The [backlog](../BACKLOG.md) records these operator limitations.

## Running applications

The recorded application routes are TSO31, TSO64 ANY and TSO64 HIGH cREXX
packages. Their native load bytes were preserved when removing transport
framing and staging them onto the test disk. The HIGH route uses a low launcher
and a separate high-resident body. AMODE24 applications are currently rejected.

Use the application's exact packaging and installation instructions; an XMIT
transport or ELF object is not directly executable by the z/PDOS loader.
Application installation must preserve dataset structure and load bytes, and
offline disk updates require the guest to be stopped. The base image does not
provide a general package manager. The
[qualification record](../qualification/QUALIFICATION.md) describes the actual
cREXX workloads, storage budgets, file results and skips.

## Build a fresh disk from source

Run these commands from the repository root. You need cREXX, CMake, a native
C toolchain, make, Bison/Flex for the inherited compiler build, Clang and
`shasum`, plus Hercules utilities `dasdload`, `cckd2ckd` and `ckd2cckd`.
The [host build guide](../../../doc/BUILD-AND-RELEASE.md) covers tool packages
and platform details.

Build the MVS compiler and supporting tools, then give the image recipe a new
output directory and the absolute directory containing the Hercules utilities:

```sh
crexx -nokeep compiler/scripts/build.crexx --args test mvs
crexx -nokeep scripts/build.crexx --args test
crexx -nokeep pdos/scripts/image.crexx --args build/pdos/my-image /absolute/hercules/bin
```

These are build-and-check recipes for developers: they include their own
component, loader and disk checks. The image recipe already invokes source
checks, compilation, assembly and linking; separate preliminary OS `check` and
`compile` commands are not required. It refuses an existing image output
directory and does not connect to a running guest.

The result is `build/pdos/my-image/media/pdos00.cckd`, with input/output hashes
and producer versions beside the build outputs. Seventeen C units and six
handwritten assembler modules are built afresh. The recipe reconstructs load
modules at two bases, creates target-encoded configuration and IPL records,
and compares every guest disk byte after compression readback. These checks
use the maintained loader and explicit corruption controls.

See [build dependencies](../architecture/DEPENDENCIES.md) for the component
inputs and [development](../development/README.md) for how to change them.
A changed source/tool candidate needs evidence appropriate to that change;
reuse the dated guest record only for the scope it actually establishes.
