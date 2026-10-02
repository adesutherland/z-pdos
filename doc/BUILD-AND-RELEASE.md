# Builds, installers and signing

I use GitHub-hosted runners to check the complete Classic host toolchain and
prepare installable downloads from one source revision. Version tags are the
publication decision. Preparing these recipes does not qualify an unrun host,
guest or signing operation.

The workflow builds macOS ARM64, macOS Intel, Linux x64 and Windows x64.
The Linux job also constructs a fresh z/PDOS disk using distro Hercules
utilities. Each host runs both Classic C variants and the complete CTest suite,
then exercises a relocated installed compiler/assembler/linker chain with exact
expected output bytes. Missing cREXX, compiler variants, macros or linker cannot
silently remove required checks. Existing archives are not build dependencies.

The bootstrap runtime is cREXX `v1.0.0-beta.3`, source
`ae1607b8e145174422cee7f3e73fbcc37a65226c`. The four public ZIP filenames and
SHA-256 digests are frozen in `.github/crexx-bootstrap.txt`; the workflow checks
the downloaded bytes before executing them. They are build tools, not bundled
runtime dependencies of the Classic commands.

## Local build and packages

Run from the repository root, with cREXX, CMake, native C tools and make:

```sh
crexx -nokeep scripts/build.crexx --args full-test
crexx -nokeep scripts/release.crexx --args stage macos-arm64 0.1.0-dev.local
crexx -nokeep scripts/release.crexx --args verify macos-arm64 0.1.0-dev.local
PDOS_APPLE_SIGNING=unsigned crexx -nokeep scripts/release.crexx --args package macos-arm64 0.1.0-dev.local
```

Use `macos-x86_64`, `linux-x64` or `windows-x64` on the corresponding host.
Staging and packaging require new output directories under
`build/release/<platform>/`. Clear or move only your own completed output before
another run. The package records its source commit, working-tree dirtiness,
platform, version and signing status in `release.json`. `SHA256SUMS` describes
final package bytes. Local dirty packages are development evidence.

Portable ZIPs contain `bin/mf-classic-cc` (MVS), `mf-classic-cc-cms`,
`mf-classic-as` and `mf-classic-ld`, with `.exe` on Windows. Both compilers find
their private `xgcc`/`cc1` under `libexec/z-pdos/<variant>` relative to the actual
launcher executable, including invocation through installed symlinks. Keep the
whole package together. Compiler operations and target profiles retain their
documented limits; installation does not add implicit assembly or linking.
PDPCLIB headers/macros/source profiles are under `share/z-pdos/pdpclib/source`.
Supply the selected target headers explicitly. There is no host libc sysroot.

Licences and attribution accompany each component. The tagged repository source
is the corresponding source for the inherited GPL compiler, including its
retained notices and exceptions. No private image, key or external manual is
included in Git.

## macOS

The PKG installs the payload under `/usr/local/lib/z-pdos` and four command
symlinks under `/usr/local/bin`. Administrator installation is required.
Portable ZIP users can run the commands from their own extracted directory.

`PDOS_APPLE_SIGNING=required` selects Developer ID signing of all eight native
files, an Installer-signed PKG, Apple notarization with explicit Accepted status,
ticket stapling and Gatekeeper assessment. Any missing setting or failed step
fails packaging. A temporary private keychain and exported certificates are
removed on success and handled failures. The workflow also cleans them after a
cancelled or failed job. The ZIP carries the signed native files; the PKG carries
the stapled ticket. Development/PR builds explicitly select unsigned packages.

Create these repository Actions secrets, matching CREXX:

| Secret | Value |
| --- | --- |
| `APPLE_DEVELOPER_ID_CERTIFICATE_BASE64` | Base64 Application `.p12` with private key |
| `APPLE_DEVELOPER_ID_CERTIFICATE_PASSWORD` | Application export password |
| `APPLE_DEVELOPER_ID_IDENTITY` | Exact Application certificate Common Name |
| `APPLE_DEVELOPER_ID_INSTALLER_CERTIFICATE_BASE64` | Base64 Installer `.p12` with private key |
| `APPLE_DEVELOPER_ID_INSTALLER_CERTIFICATE_PASSWORD` | Installer export password |
| `APPLE_DEVELOPER_ID_INSTALLER_IDENTITY` | Exact Installer certificate Common Name |
| `APPLE_ID` | Developer account email |
| `APPLE_APP_SPECIFIC_PASSWORD` | Generated Apple app-specific notarization password |
| `APPLE_TEAM_ID` | Developer team identifier |

On the Mac, use Keychain Access to select each Developer ID certificate and
its matching private key, then export them separately as password-protected
PKCS#12 (`.p12`) files. A certificate-only `.cer` export cannot sign. Example
local filenames are `~/signing/developer-id-application.p12` and
`~/signing/developer-id-installer.p12`; keep them outside the repository.
The two password secrets contain the corresponding export passwords.

Generate each Base64 value as one continuous string, without quotes or a
trailing newline:

```sh
base64 -i "$HOME/signing/developer-id-application.p12" | tr -d '\r\n' | pbcopy
# Paste into APPLE_DEVELOPER_ID_CERTIFICATE_BASE64.
base64 -i "$HOME/signing/developer-id-installer.p12" | tr -d '\r\n' | pbcopy
# Paste into APPLE_DEVELOPER_ID_INSTALLER_CERTIFICATE_BASE64.
```

One `=` or two `==` at the end are both valid Base64 padding. Keep them.
The RAG packager validates Base64 strictly; `Excess data after padding` means
the copied secret contains extra data after its encoded value, rather than
proving that the number of equals signs is wrong.

The current identity examples are
`Developer ID Application: Adrian   Sutherland (S4ESV5CK46)` and
`Developer ID Installer: Adrian   Sutherland (S4ESV5CK46)`; preserve the exact
Common Name, including spaces. Read it in Keychain Access and check validity
there before exporting. `APPLE_TEAM_ID` is `S4ESV5CK46` for these certificates.
For Application signing, `codesign` also accepts the certificate SHA-1 printed
by `security find-identity -v -p codesigning`. The current local fingerprint is
`9AB0CBB6E1C3A4FCF72F3E0DE7BC1B0770A2DD68`; it must match the Application
certificate actually exported. This avoids Common Name spacing mistakes.
`APPLE_ID` is the email used for the developer account, for example
`developer@example.com`. Generate `APPLE_APP_SPECIFIC_PASSWORD` at
[Apple Account](https://account.apple.com/) under Sign-In and Security →
App-Specific Passwords; its format is `xxxx-xxxx-xxxx-xxxx`.
The account procedure is documented by [Apple Support](https://support.apple.com/en-gb/102654).

Trusted branch/manual builds and version tags exercise signing. PRs receive no
signing secrets, including PRs from the same repository. Only pushed version
tags can publish releases. GitHub supplies the release token automatically.

## Windows

The hosted build uses the MSYS2 UCRT64 toolchain, Unix tools for the inherited
configure/make interface, and native Windows cREXX. The configured ADDRESS SHELL
is MSYS2 Bash, rather than the native SYSTEM command processor. The built
Classic tools are native Windows PE executables. MinGW host configuration and
Windows launcher support do not imply completed hosted Windows qualification.

The NSIS installer is per-user under `%LOCALAPPDATA%\Programs\z-pdos`.
It adds only its own `bin` directory to the user PATH, preserves long and
pre-existing entries, and removes only owned files/entries on uninstall.
The small PowerShell helper implements the Windows registry interface; build,
packaging and signing orchestration remain cREXX. Restart terminals after
installation to pick up PATH changes.

CI packages Windows downloads as unsigned. After installing SimplySign and
logging in locally on the Mac, set `PROVIDER` to its PKCS11 configuration file,
`CERTUM_ALIAS` to your certificate alias and optionally `TSA_URL` to the
timestamp service. Requirements: cREXX with native packaging tools, `jsign`,
`osslsigncode`, NSIS and CMake. No Windows key is uploaded to GitHub.

```sh
export PROVIDER=/absolute/path/provider.macos.cfg
export CERTUM_ALIAS=your-token-alias
export TSA_URL=http://time.certum.pl
crexx -nokeep scripts/sign-windows.crexx --args /absolute/path/z-pdos-0.1.0-windows-x64-unsigned.zip 0.1.0
```

For the existing CREXX setup the provider file is
`/Users/adrian/CLionProjects/CREXX/scripts/provider.macos.cfg`:

```text
name=SimplySignPKCS
library=/usr/local/lib/libSimplySignPKCS.dylib
```

Use the certificate alias shown by the configured token, rather than the
Apple identity or a certificate filename. CREXX currently uses
`7DDC0FE9C4D43C9D1D900B39548410F1`. If SimplySign reports
`CKR_FUNCTION_FAILED`, refresh its local login before repeating the signing
step. Provider/alias/timestamp settings describe local Windows signing; they
are not additional GitHub secrets.

Signing verifies the unsigned payload inventory, signs its PE files and helper,
refreshes hashes, signs private NSIS plugins and the generated uninstaller,
then signs/verifies the final installer. The original ZIP remains unchanged.
Final assets and checksums are under `build/release/windows-local-assets`.
There is no upload operation in this local signing script.

## PDOS disk image and release tags

The Linux host builds a fresh `media/pdos00.cckd` using the existing
`pdos/scripts/image.crexx` recipe. Loader, relocation, dataset, compression
readback and corruption controls must pass. The image archive includes a
Hercules example configuration, operator instructions, source/tool identities
and checksums. It contains only the base OS; it does not bundle cREXX applications
or a managed/private guest disk.

The image job records the actual distro Hercules tool versions and hashes.
Its host image checks are distinct from the recorded Hercules 4.9.1 guest
qualification. It does not start or modify a shared Lab guest, and it does not
claim that this freshly produced image has passed a new application guest run.
The exact existing guest evidence remains in `pdos/doc/qualification/`.

`VERSION` and the CMake product version are `0.1.0`. A pushed `v0.1.0` or
`v0.1.0-<prerelease>` tag builds the full matrix. Publication depends on every
host and image check succeeding. Prerelease suffixes create prereleases.
The release contains host installers/ZIPs, the fresh image ZIP and a combined
checksum inventory. Windows downloads remain labelled unsigned until the
separate local signing operation and explicitly authorised upload.

The workflow itself creates neither tags nor development commits.
