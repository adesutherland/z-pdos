# Documentation

Start with the [project overview](../README.md). These guides describe the
maintained source on `develop`; versioned releases and dated qualification
records identify their own inputs.

| I want to… | Read |
| --- | --- |
| Download and install the Classic tools | [Builds, installers and releases](BUILD-AND-RELEASE.md) |
| Boot the base operating system | [z/PDOS user guide](../pdos/doc/user/README.md) |
| Understand how the OS works | [z/PDOS architecture](../pdos/doc/architecture/README.md) |
| Understand profiles and the two toolchains | [Profiles and interfaces](PROFILES.md) |
| Change the kernel, loader or shell | [OS development guide](../pdos/doc/development/README.md) |
| Understand the C runtime and native services | [PDPCLIB architecture](../pdpclib/doc/architecture/README.md) |
| Use the compiler, assembler or linker | [Classic C](../compiler/doc/user/README.md), [Assembler](../assembler/doc/user/README.md), [Linker](../linker/doc/user/README.md) |
| Check licences and origins | [Licence map and build independence](../LICENSES.md) |
| Find supported hardware assumptions | [Machine descriptions](../machines/README.md) |
| Propose or implement a change | [Shared workflow](WORKFLOW.md) and the component's `doc/BACKLOG.md` |

The [completed P0–P6 record](../pdos/doc/qualification/TWO-SPACE-P6-2026-10-07.md)
qualifies the protected K64/C31 kernel, shared U applications, native calls,
console and disk/tape workflows. The
[0.2.0 release record](../pdos/doc/qualification/RELEASE-0.2.0.md) keeps local
build, operator acceptance and downloadable artifact identities separate.
The [0.2.1 record](../pdos/doc/qualification/RELEASE-0.2.1.md) adds Workbench,
the C editor/utilities, canonical cREXX bundle and the 512 MiB capacity profile,
with separate console/application results and release artifact gates.
Earlier 0.1/0.1.1 qualification reports describe their one-space inputs.

The [2 October reorganisation record](REORGANISATION-20261002.md) and
[release preparation report](RELEASE-PREPARATION-20261002.md) preserve earlier
checkpoints. Their pending work describes those dates; use current guides and
backlogs for present status. [AGENTS.md](../AGENTS.md) contains repository
rules for automated contributors.
