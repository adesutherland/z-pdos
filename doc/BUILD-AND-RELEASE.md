# Install, build and release the Classic tools

The [0.1.1 release](https://github.com/adesutherland/z-pdos/releases/tag/v0.1.1)
contains tools for macOS Apple Silicon, macOS Intel, Linux x64 and Windows x64,
plus a source archive and a separate z/PDOS base disk image.

The [release workflow](https://github.com/adesutherland/z-pdos/actions/runs/37308481475)
passed for source `69b638d38dc4221cbed399d8fde5bf4883673901`: both compiler
variants, the required host suite and a relocated compile/assemble/link check
on all four hosts, plus the Linux source-to-image checks. macOS packages were
signed and notarized. CI built the Windows packages unsigned; the separate
local signing step completed for 0.1.1. The published Windows ZIP and installer
are Authenticode signed and timestamped. The
[local operator acceptance](../pdos/doc/qualification/0.1.1-OPERATOR-UAT-2026-10-05.md)
and downloadable Linux-built base image have distinct image identities.
The released image receipt pins the same PLOAD, PDOS, PCOMM and CONFIG payload
hashes as the locally accepted bare producer. Its CCKD container has a separate
hash and was not itself used for that guest run.

## Choose a download

| Host / purpose | Download | Installation |
| --- | --- | --- |
| macOS Apple Silicon | `macos-arm64-signed.pkg` or `macos-arm64-signed.zip` | PKG installer or portable directory. |
| macOS Intel | `macos-x86_64-signed.pkg` or `macos-x86_64-signed.zip` | PKG installer or portable directory. |
| Linux x64 | `linux-x64.zip` | Extract and use the `bin` directory. |
| Windows x64 | `windows-x64-signed-setup.exe` or `windows-x64-signed.zip` | Signed per-user installer or portable directory. |
| Run the operating system | `pdos-image.zip` | Follow the [OS boot guide](../pdos/doc/user/README.md). |
| Inspect or rebuild the release | `source.tar.gz` | Corresponding maintained source and recipes. |

These are filename suffixes; release assets start with `z-pdos-0.1.1-`.
Verify downloads against the release's `SHA256SUMS` before use. The image ZIP
also has an internal inventory for its extracted files.

The macOS PKG requires administrator installation. It puts the payload in
`/usr/local/lib/z-pdos` and four command symlinks in `/usr/local/bin`. Its
Developer ID signature, notarization and stapled ticket accompany the
installer. The portable ZIP contains signed native files.

The Windows installer defaults to `%LOCALAPPDATA%\Programs\z-pdos`, adds its
own `bin` directory to the user PATH, and removes owned files and PATH entries
on uninstall. Restart terminals after installation. Portable ZIP users on any
host can invoke the tools by their extracted paths; keep the complete package
together when moving it.

## What is installed

| Command | Purpose |
| --- | --- |
| `mf-classic-cc` | MVS C compiler launcher. |
| `mf-classic-cc-cms` | CMS C compiler launcher. |
| `mf-classic-as` | Classic source-to-object assembler. |
| `mf-classic-ld` | Classic object linker and native-format producer. |

Windows commands have an `.exe` suffix. The compiler launchers find their
private `xgcc` and `cc1` under `libexec/z-pdos/<variant>` relative to the
installed executable. Copying only a launcher will not work. The commands do
not need cREXX installed to run.

The compiler supports assembler-text output (`-S`), preprocessing (`-E`),
syntax checks and queries. It rejects implicit assembly/linking and unsupported
profile selections. Target headers and runtime choices must be supplied
explicitly. PDPCLIB source, headers and macros are under
`share/z-pdos/pdpclib/source`; installation does not create a target sysroot.
See [Classic C usage](../compiler/doc/user/README.md),
[assembler syntax](../assembler/doc/user/USER.md) and
[linker formats](../linker/doc/user/USER.md).

Windows private compilers include the pinned GNU libiconv runtime. Its licence,
package identity and matching source/build archive are under
`share/z-pdos/host-libraries/libiconv`. Retain the package's component licences
and attribution; [LICENSES.md](../LICENSES.md) explains their scope.

## Build from the checkout

Run from the repository root with cREXX, CMake, native C development tools,
make, Bison and Flex. The complete development recipe builds both compiler
variants and runs the required host checks:

```sh
crexx -nokeep scripts/build.crexx --args full-test
```

To stage, verify and create a local macOS development package:

```sh
crexx -nokeep scripts/release.crexx --args stage macos-arm64 0.1.1-dev.local
crexx -nokeep scripts/release.crexx --args verify macos-arm64 0.1.1-dev.local
PDOS_APPLE_SIGNING=unsigned crexx -nokeep scripts/release.crexx --args package macos-arm64 0.1.1-dev.local
```

Use the corresponding platform key `macos-x86_64`, `linux-x64` or
`windows-x64` on other hosts. `PDOS_APPLE_SIGNING` controls the macOS packaging
path. Staging and packaging require new output directories under
`build/release/<platform>/`; move or remove only your own previous output.
`release.json` records source commit, working-tree dirtiness, platform, version
and signing status. A local dirty package is development evidence.

For the operating-system image, use the [OS build recipe](../pdos/doc/user/README.md#build-a-fresh-disk-from-source).
It additionally requires Clang, `shasum` and Hercules disk utilities. The four
host-tool packages do not imply a source-to-image build on every host: the
release image job runs on Linux, and the original local source-to-image
milestone ran on Apple Silicon macOS.
The root `VERSION` and `pdos/src/zpdos-version.h` must carry the same release
number. The PCOMM host check compares the guest `VERSION` output with the root
file, and the image receipt hashes both inputs. Guest acceptance remains a
separate gate before selecting a shared image.

## How the hosted release works

The [workflow source](../.github/workflows/build-release.yml) is the maintained
recipe. It builds the four host variants, checks both C compilers and the
required suite, stages each package, and exercises the installed tools after
relocation. Missing required inputs cannot silently remove checks.

The build bootstrap uses cREXX `v1.0.0-beta.3`, source
`ae1607b8e145174422cee7f3e73fbcc37a65226c`. The public ZIP filenames and SHA-256
hashes are pinned in [crexx-bootstrap.txt](../.github/crexx-bootstrap.txt) and
verified before execution. cREXX orchestrates the build; it is not a bundled
runtime dependency of the installed Classic commands.

Windows uses MSYS2 UCRT64 for the inherited configure/make build and native
Windows cREXX. `scripts/windows-shell.c` adapts the pinned runtime's raw shell
command text to Bash's `-c` interface. The tools it builds are native Windows
executables. The package checks account for their private DLL dependencies;
installer filename escaping is owned by `scripts/windows-inventory.crexx`.

Linux also builds a fresh 100-cylinder CCKD disk from source. Loader,
relocation, dataset, compression-readback and corruption checks must pass.
The image archive records actual tool identities and includes a Hercules
configuration and boot instructions. It has only the base OS; no private disk
or cREXX application package is copied into it. These host checks do not start
a guest or establish new application execution.

A pushed version tag such as `v0.1.1` runs the release matrix. Publication waits
for all required host and image jobs; prerelease suffixes produce prereleases.
The workflow creates neither the tag nor a development commit. Trusted branch
and manual builds can exercise signing, but only version-tag pushes publish a
release. Pull requests receive no signing secrets.

## Maintainer reference: macOS signing

`PDOS_APPLE_SIGNING=required` requires Developer ID signing of the native files,
an Installer-signed PKG, an Accepted notarization result, ticket stapling and
Gatekeeper assessment. A missing setting or failed step fails packaging.
Development/PR packages explicitly select unsigned output.

Configure these Actions secrets for the signing account:

| Secret | Value |
| --- | --- |
| `APPLE_DEVELOPER_ID_CERTIFICATE_BASE64` | Base64 Application `.p12`, including its private key. |
| `APPLE_DEVELOPER_ID_CERTIFICATE_PASSWORD` | Application certificate export password. |
| `APPLE_DEVELOPER_ID_IDENTITY` | Exact Application certificate identity. |
| `APPLE_DEVELOPER_ID_INSTALLER_CERTIFICATE_BASE64` | Base64 Installer `.p12`, including its private key. |
| `APPLE_DEVELOPER_ID_INSTALLER_CERTIFICATE_PASSWORD` | Installer certificate export password. |
| `APPLE_DEVELOPER_ID_INSTALLER_IDENTITY` | Exact Installer certificate identity. |
| `APPLE_ID` | Developer account email. |
| `APPLE_APP_SPECIFIC_PASSWORD` | Account's app-specific notarization password. |
| `APPLE_TEAM_ID` | Developer team identifier. |

Export the two certificates with their matching private keys as separate,
password-protected PKCS#12 files. Use each full Base64 value without quotes or
extra text. Certificate identities must match the exports, including spaces.
Keep account values, keys and local export files outside Git.

The script creates a temporary private keychain and cleans it on success and
handled failure; the workflow also cleans up after failed or cancelled jobs.
`scripts/macos-payload.crexx` handles payload hashes and component PKG creation
without re-entering the running cREXX release orchestrator.

## Maintainer reference: Windows signing

CI initially produces unsigned Windows packages. A separate local operation
signs and publishes them; this step completed for 0.1.0 and 0.1.1. For 0.1.1,
the script verified the Authenticode signatures and timestamps, uploaded the
signed ZIP and installer, refreshed `SHA256SUMS`, and removed the unsigned
downloads. No Windows signing key is uploaded to GitHub.

The local signing route requires a logged-in SimplySign token, cREXX, `jsign`,
`osslsigncode`, NSIS, CMake and authenticated `gh`. Set `PROVIDER` to the PKCS11
configuration and `CERTUM_ALIAS` to the token's actual certificate alias.
`TSA_URL` optionally selects the timestamp service.

The 0.1.1 operation used the following command, which **uploads to an existing
release and replaces its unsigned Windows downloads**. Use it for another
release only with that release's own input ZIP, version and authorization:

```sh
export PROVIDER=/absolute/path/provider.macos.cfg
export CERTUM_ALIAS=your-token-alias
export TSA_URL=http://time.certum.pl
crexx -nokeep scripts/sign-windows.crexx --args /absolute/path/z-pdos-0.1.1-windows-x64-unsigned.zip 0.1.1
```

The script checks the unsigned payload inventory, signs the native files and
helper, refreshes hashes, signs private NSIS plugins and the uninstaller, then
signs and verifies the final installer. It leaves the original local ZIP
unchanged and writes final assets under `build/release/windows-local-assets`.
It uploads the signed ZIP and installer, updates `SHA256SUMS`, verifies their
presence, and removes the matching unsigned release assets. It defaults to
`adesutherland/z-pdos`; an optional third argument selects another repository.
It does not alter the tag, version or release notes.

If upload was interrupted after signing, reuse the completed assets:

```sh
crexx -nokeep scripts/sign-windows.crexx --args --upload-only 0.1.1
```
