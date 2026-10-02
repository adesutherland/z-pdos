# Repository reorganisation — 2 October 2026

The approved component structure is implemented locally. We built each
component after moving it, then checked their integration and the z/PDOS 0.1
output. This work preserves behaviour. Existing functional defects remain
in the owning component's backlog; no functional defect repair is included.
The starting checkout was clean, on develop at
`fd7df814fff5b181fb18ab30de31872c77404d2c`. This record describes the local
migration checks; commit and publication were authorised after the main README
review.

## Maintained layout

| Previous location | Owning component now |
| --- | --- |
| os/pdos/ | pdos/ — z/PDOS |
| pdpclib/ with root-level implementation | pdpclib/src/ |
| tools/classic-as/ | assembler/ |
| tools/classic-cc/ | compiler/ |
| tools/classic-ld/ | linker/ |
| runtime/tso31/ | tso31-bridge/ |
| architecture/ | machines/ |
| Shared tools/*.crexx | scripts/ and tests/integration/, with owned checks in their components |

Each component has README.md, AGENTS.md, LICENSE, UPSTREAM.md, one current
src/ tree, doc/, tests/ and scripts/. Documentation is organised into user,
architecture, ai, development and qualification directories. The common work
queue is component/doc/BACKLOG.md, following [WORKFLOW.md](WORKFLOW.md).
[The root agent guide](../AGENTS.md) enforces ownership, attribution, frozen
archives, source maintenance and the agreed scope of structural migrations.

Upstream/reference archives are explicitly frozen and have recorded checksums.
The independently authored assembler needs no upstream archive. Paul Edwards's
PDOS and PDPCLIB authorship is prominent; component notices and the compiler's
GCC/i370/cc370 contributor history and GPL terms remain with their material.
Historical local patches and recovery recipes are retained by Git history.
Current builds consume maintained source directly and never apply patches.

PDPCLIB's five native MVSSUPA selections now share source modules in
src/native/mvssupa/. Profile input lists concatenate those modules into the
build directory. Every selection reproduces its previous assembler input
byte for byte. The TSO bridge test uses explicit normalized reference
statements, and the OS repair checks use two small deliberate failure fixtures;
their frozen archives are unnecessary for required tests.

Generated outputs are under ignored build/. Documented cREXX commands and
child commands use -nokeep to remove transient compiler intermediates.

## Build and check results

All commands run from the repository root on macOS with native Clang, CMake
and cREXX. The compiler retains its inherited configure/Makefile build.
Component counts include any dependency checks registered by that component;
they are not additional independent totals to sum with the root suite.

| Component or scope | Validation | Result |
| --- | --- | --- |
| Assembler | Standalone build and CTest | 37/37 passed |
| Compiler | Separate MVS and CMS builds; inherited and project code-generation fixtures at -O0, -O1, -O2 and -Os | Both passed |
| Linker | Standalone build and CTest | 33/33 passed |
| PDPCLIB | Five focused host source checks and all five profile-source selections | 6/6 registered tests passed |
| TSO31 bridge | Standalone build, five bridge checks and assembler dependency suite | 42/42 passed |
| z/PDOS | Source failure controls, 17 C units, six native assembly modules, links and fresh media build | Passed |
| Cross-component root build | Complete registered suite before and after migration | 91/91 passed |

With all five archive directories physically removed to ignored temporary
storage, we rebuilt/checked every component and the complete root suite.
The OS check covered its complete 17-unit C compile route; full image checks
are recorded separately below. Every command passed. Archives were restored and every frozen file hash was
verified unchanged. This establishes archive independence for the maintained
build and required test routes.

The final root run also passed 91/91 after adding automatic cREXX intermediate
cleanup. The MVS compiler build/test and OS source failure controls passed
through the updated child-command paths.

## z/PDOS 0.1 equivalence

The fresh image is build/pdos/reorganised-image-2. Compared with the accepted
build/pdos/zpdos-0.1-final input, all 23 objects, all three RDW load modules
and all six flat links are byte-identical. The shipped payload identities are:

| Payload | Bytes | SHA-256 |
| --- | ---: | --- |
| PLOAD.SYS | 129,440 | `1d56baf898cf3aaa496e74efd90d597e626bd45be1f7c1eee941aa78f5484c9b` |
| PDOS.SYS | 181,320 | `94ddd6d188716c3d69b1b6462662f6862e3035711593be8ce1f9f409d560da1e` |
| COMMAND.EXE | 105,408 | `8afd1c8ed07f095b50cf8671d8713e6ada37df2abbb09f70a5fef724536b40cd` |
| CONFIG.SYS | 10 | `cb71c77537873856995ed4e9b7d936da9cf3fcd8abea17c5ab64146330c8a19d` |

Complete raw-disk comparison passes: every guest byte is unchanged, with only
the regenerated Hercules 12-digit container serial allowed to differ. Fresh
IPL/dataset checks, compression readback, five media corruption controls and
sanitized real-loader reconstruction at two bases also pass.

The original [0.1 qualification JSON](../pdos/doc/qualification/qualification-0.1.json)
is unchanged byte for byte. Current build receipts identify current source
rather than requiring old path-based manifests. Guest execution evidence is
reused because the guest inputs are identical; no guest was altered or rerun.
These migration checks establish equivalent host-built input and do not claim
new guest or release qualification.

## Source preservation and remaining work

The source audit found 971 relocated C/header/assembly/macro source and fixture
files unchanged byte for byte. Six test source files changed only their
relative include paths. The shared MVSSUPA decomposition preserves all five
previous generated inputs. Build orchestration, component metadata and
documentation account for the remaining intentional changes.

Detailed local logs and comparisons are under build/reorganisation/. They are
disposable receipts, not maintained product source. Known compatibility,
qualification and operator issues are in the six component backlogs. Their
implementation requires its own agreed scope under the shared workflow.
The migration did not alter the external SDK, Mainframe Lab or a running guest.
Commit and publication of this repository are the subsequent authorised step.
