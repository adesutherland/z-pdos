# z/PDOS repository guide

This repository owns z/PDOS, PDPCLIB, Mainframe Classic Assembler, Mainframe
Classic C, Mainframe Classic Linker and the experimental TSO31 entry bridge.
The parent /Users/adrian/CLionProjects/AGENTS.md applies. Read README.md,
LICENSES.md, the owning component's README.md, AGENTS.md, UPSTREAM.md and
relevant user/architecture documentation before editing. Check Git status and
preserve unrelated changes. Do not commit, push or publish without explicit
session authority.

## Branches and checkouts

All work in this repository must happen on `develop` or `hotfix`. These are
the only permitted branch names. Do not create or use feature, release,
temporary or other branches, including in additional worktrees.

Before starting, check the current branch, working-tree status and published
branch tip. Keep the primary checkout current with `origin/develop` after work
is published. Preserve unrelated local changes before updating it.

Remove other branches and their worktrees after preserving any unpublished
work and required generated artifacts. Do not discard unique changes during
cleanup. Release tags are retained; they are not development branches.

## Repository structure and ownership

Each product is a root component: pdos/, pdpclib/, assembler/, compiler/,
linker/ or tso31-bridge/. Its implementation lives in exactly one src/ tree,
including headers, native support, macros, adapters and maintained source
configuration. Edit that source directly; Git commits preserve earlier
versions. Do not maintain fixes as patch layers, refresh consolidated recovery
patches, copy product sources into other components, or use historical build
trees as the authority. Explicit source modules may share implementation and
select legitimate system differences without duplicating full source versions.

Every component has README.md, AGENTS.md, LICENSE and UPSTREAM.md, plus doc/
with user, architecture, ai, development and qualification material. tests/
owns focused tests and small deliberate fixtures; scripts/ owns maintained
component automation. No product implementation belongs at a component root.
Inherited configure/build internals may remain within their owning src/ tree.

Keep component-specific implementation, tests, user guides and build recipes
together. Root scripts/ contains only shared orchestration; root tests/
contains cross-component integration checks. machines/ owns shared hardware
profiles and references. Encoder implementations belong to their tools, and
component ABI, object, encoding and OS-service settings remain explicit.
Generated outputs live under ignored build/ and are not maintained source.
Run cREXX recipes with -nokeep, as shown in the user guides, so their transient
compiler intermediates are removed automatically. Retained orchestration and
CMake checks pass that option to child cREXX commands too.

## Upstream archives, attribution and licences

The optional archive/ directories are frozen upstream/reference baselines.
Do not modify them or routinely add new baselines. Normal builds and required
tests must work with every archive directory absent. Historical local patch
series and recovery recipes are retained by Git, not a live patch workflow.
Reconcile a new upstream contribution directly into src/ through normal
commits and record its exact origin in UPSTREAM.md outside the frozen archive.

Credit original creators prominently in each README and explain the source
lineage in UPSTREAM.md. Paul Edwards created PDOS and PDPCLIB; retain all
other contributor notices. The linker derives from PDLD. The compiler derives
from GCC/i370/cc370 and retains its contributor history and GPL terms. The
assembler is independently authored original project material. A root MIT
grant does not relicense inherited code or external architecture references.
Keep actual component licences, notices and exceptions with their material.

Do not import inherited assembler/emulator implementations or opcode tables
into the original assembler. Use cited primary architecture facts and
independently expected test vectors. Keep private research, guest assets,
credentials, correspondence, downloaded manuals and native reference
objects/listings outside this repository.

## One component backlog and working process

Every component uses only doc/BACKLOG.md for defects, roadmap proposals and
qualification gaps; do not add competing PLAN, ROADMAP or KNOWN-ISSUES queues.
Follow doc/WORKFLOW.md. Give each item a stable component-prefixed ID, type,
status, observation, evidence, affected target and acceptance criteria. Search
existing items first. Use Open, In progress, Blocked or Done; record a concrete
prerequisite for Blocked and acceptance evidence before Done. Merely recording
an item does not authorise implementation.

A structural reorganisation preserves behaviour and changes only source
placement, equivalent source selection, build plumbing and documentation.
Existing functional defects are recorded in the owning backlog and are not
repaired as part of that migration. Repair regressions introduced by the
migration; do not remove failing checks or weaken contracts to make it pass.
Move and build each component in turn, then verify affected integration and
consumer outputs. Record exact distinctions between built, checked, guest
qualified and released.

## Engineering and qualification

PDPCLIB is maintained only in pdpclib/src/. OS/application consumers use its
explicit service profiles. Mainframe Lab keeps references and guest evidence;
the modern SDK is a consumer. Kernel changes belong in pdos/src/; relocation
changes belong in linker/src/; compiler changes belong in compiler/src/.
The current z/PDOS 0.1 route is recorded in pdos/doc/qualification/.

Use portable C89/C90 for the bootstrap assembler core, supplied-storage and
explicit host-service interfaces. No mandatory POSIX, native 64-bit integer,
cREXX runtime or plugin loader belongs in that bootstrap core. Retained new
project automation is cREXX; inherited tool interfaces and build systems keep
their necessary languages. Internal text is UTF-8, the initial parser is ASCII,
binary objects are octets and target character encoding is explicit. Do not
assume the host C execution character set is ASCII.

Keep historical IBM machines, community S/380 extensions and counterfactual
architectures distinct. Sharing a hardware profile name does not establish ABI,
object-format or runtime compatibility. Qualify compile, assembly, link/load,
disk construction and guest execution separately. Record current source
identities in build receipts instead of forcing maintained source to match
an old checkpoint. Reuse unchanged qualification evidence and repeat only
checks affected by a change or concrete concern. Use generous 31/64-bit heaps;
keep constrained 24-bit storage budgets separate.

Shared guest operation requires Mainframe Lab's operator guide and leases.
A host source/image check does not authorise changing a running guest.
Use GPT-6.1 Sol Extra High for coordination and complex technical work;
GPT-6.1 Sol High is suitable for defined implementation/QA. When work is
explicitly delegated, the implementer owns appropriate QA and the coordinator
independently validates delivery.
