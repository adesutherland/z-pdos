# Release preparation: 2 October 2026

The runner and packaging changes were prepared locally on `develop`, based on
`96c7c7c11ef76fd34a6dd33a511a8a4e1b7ad894`. Adrian authorised publication of
`v0.1.0` on 2 October. The release candidate is assembled in a clean isolated
checkout, preserving the development checkout. The maintained procedure is
[BUILD-AND-RELEASE.md](BUILD-AND-RELEASE.md); component qualification gaps remain
in their existing backlogs.

## Local checks

The host is Apple Silicon macOS with Apple Clang, CMake and local cREXX
`1.0.0-beta.3+local.g5949ef27efd8`. The hosted bootstrap instead pins the
published beta-3 runtime and each platform's exact archive digest.

- Both MVS and CMS Classic C variants rebuilt and passed their existing
  code-generation and consolidation checks.
- All 91 existing CTests passed. The additional Windows CRT argument-quoting
  check passed separately after it was added.
- Both staged, extracted-ZIP and expanded-PKG MVS/CMS toolchains compiled,
  assembled and linked the fullword fixture in a relocated path containing
  spaces. All 28 independently expected bytes matched. Unsupported `-c` and
  missing-private-compiler controls returned the expected failure.
  Deliberately modifying an extracted package file also failed its inventory
  check before any compiler was executed; the fixture was restored afterwards.
- An unsigned development macOS PKG and portable ZIP were constructed. The
  expanded PKG contains the four intended relative command symlinks and the
  complete private compiler payload. No administrator installation was done
  on the development Mac.
- The real Windows NSIS interface compiled with a deliberately synthetic
  fixture. That check proves construction, not Windows execution or signing.
- The native cREXX helper used by NSIS for local Windows signing compiled.
  Missing Apple settings fail before creating a temporary keychain.
- A fresh base disk was built in `build/pdos/release-qa` with the existing
  maintained recipe and the Lab's SDL Hercules utility installation. All 23
  fresh objects, three real-loader checks and complete disk payload/CCW,
  dataset and compressed-disk readback controls passed. The distributed
  image archive's extracted file hashes matched.
- `actionlint` and `git diff --check` passed.

Detailed local logs and generated development packages are under ignored
`build/`, including `release-qa.log`, `release-installed-qa.log`,
`release-package-qa.log`, `release-extracted-qa.log`,
`release-pkg-payload-qa.log`, `release-nsis-qa.log` and
`release-image-qa.log`.

## Checks still required

The workflow must run on macOS ARM64/Intel, Linux x64 and Windows
x64. Actual Apple payload/Installer signing, notarization, stapling and
Gatekeeper installation remain unrun for z/PDOS. Native Windows compiler
build/execution and install/reinstall/PATH/uninstall acceptance also remain
unrun at preparation. Local SimplySign signing has not been exercised on genuine Windows
z/PDOS payloads. Linux's distro Hercules utilities still require their hosted
image checks.

This work performed no new guest boot or application run and changed no shared
guest disk. Existing exact guest qualification stays in the PDOS component's
qualification record. Tag publication requires the complete hosted matrix;
local development packages are not release artifacts.
