# Mainframe Classic C backlog

This is the only live roadmap, defect and qualification queue for this component.
Follow [the shared workflow](../../doc/WORKFLOW.md). Existing findings are
recorded here; their repair is outside the 2 October 2026 reorganisation.
Open items require separate implementation authority.

## CC-001: External-name policy

- Type: improvement
- Status: Open
- Target: MVS/CMS cross-compiler and declared Classic workload profiles
- Observation: Long-name aliases retain cross-unit collision and explicit-map concerns.
- Evidence: UPSTREAM.md and existing collision fixtures.
- Acceptance: Define cross-unit collision handling, overflow diagnostics and maps for existing libraries; rebuild cooperating units consistently.

## CC-002: Shared machine selectors and integer-only lowering

- Type: improvement
- Status: Open
- Target: MVS/CMS cross-compiler and declared Classic workload profiles
- Observation: Named profiles remain unsupported by the driver; HFP emission does not satisfy an integer-only profile.
- Evidence: machines/doc/MACHINE-PROFILES.md and src/driver.c.
- Acceptance: Constrain emission and final objects to the named ceiling, with explicit runtime helper closure or truthful rejection.

## CC-003: Complete runtime and optimisation coverage

- Type: qualification
- Status: Open
- Target: MVS/CMS cross-compiler and declared Classic workload profiles
- Observation: The selected z/PDOS workload and consolidation fixtures pass; this does not prove all programs at all optimisation settings.
- Evidence: doc/qualification/CHECKPOINT.md and z/PDOS 0.1 qualification.
- Acceptance: Build runtime/helpers with consistent name policy and qualify realistic applications at each affected optimisation level.

## CC-004: Historical application entry and high residence

- Type: qualification
- Status: Open
- Target: MVS/CMS cross-compiler and declared Classic workload profiles
- Observation: The first historical integer application chain, native entry and code residence need distinct qualification from the z/PDOS kernel build.
- Evidence: Existing compiler checkpoints and shared profile direction.
- Acceptance: Compile, assemble, link and run an unchanged real consumer for each declared entry/residence profile.

## CC-005: Native CMS hosting and 64-bit code generation

- Type: improvement
- Status: Open
- Target: MVS/CMS cross-compiler and declared Classic workload profiles
- Observation: The present backend has 32-bit pointers. CMS host execution and a 64-bit Classic backend are separate future work.
- Evidence: Current source data model and component checkpoints.
- Acceptance: Separate approved backend/hosting designs and their own measured compile-to-guest acceptance.

## CC-006: Later reported compiler patch series

- Type: qualification
- Status: Open
- Target: MVS/CMS cross-compiler and declared Classic workload profiles
- Observation: The later 18-patch source bundle was not present in the examined native deliveries. Do not infer its contents from the count.
- Evidence: doc/qualification/MANUAL-RECONCILIATION.md and UPSTREAM.md.
- Acceptance: Compare an exact attributable source bundle if it becomes available; retain current available-source limits.

## CC-007: Installed toolchain and native Windows host qualification

- Type: qualification
- Status: In progress
- Target: macOS ARM64/Intel, Linux x64 and native Windows x64 host tools
- Observation: Relocatable launchers and the MinGW host configuration are prepared. Local Apple Silicon build, installed MVS/CMS compile/assemble/link and Windows argument-quoting checks pass. Hosted Windows installer construction now passes. The first native compiler build exposed the pinned runtime's raw shell-command interface, which passed only the first word to Bash `-c`; a native adapter and pre-build quote/redirection/status probe are prepared. Complete hosted Windows qualification remains pending.
- Evidence: `src/driver.c`, `src/gcc/config/host-mingw.h`, root `tests/release/`, and `../../doc/BUILD-AND-RELEASE.md`. The shell probe passes on Windows; configure then exposed drive-letter paths misparsed by the inherited Makefile generator. Configure paths now use `cygpath -u`, while native command/output paths retain Windows format. The later compiler build reached parser generation and found missing Bison/Flex; both are now explicit runner dependencies and checked before configure. Native `gengtype` then exposed baked-in MSYS source paths, which receive no shell argv conversion. Its generated path strings now use `cygpath -m` on Windows, while make/configure paths retain MSYS form.
- Acceptance: Both variants build and pass the complete required suite on each declared host; extracted and installed commands produce independently expected bytes from a path containing spaces; missing private tools fail; the hosted Windows installer passes install/reinstall/PATH/uninstall controls.

The earlier first-COPY gap is closed for the selected z/PDOS workload. Exact
XL4/XL8 HFP emission was selected before this migration; it retains target
rounding and widths and does not change the ABI or admit HFP into integer-only
profiles. The remaining application/profile gates are recorded above.

## CC-008: Modern MinGW error-table macros

- Type: host portability
- Status: In progress
- Target: Native Windows x64 compiler hosting
- Observation: MinGW exposes `sys_nerr` and `sys_errlist` through function-backed macros. The inherited libiberty declarations expanded into an invalid function returning an array. Those redundant declarations are now omitted when the CRT defines the macros; ordinary variable-backed hosts retain their declarations.
- Evidence: [Windows failure](https://github.com/adesutherland/z-pdos/actions/runs/37062265650). A CRT-macro fixture reproduces the original compilation failure and exercises error-name lookup with the repair. All 93 required local GCC 16 checks pass.
- Acceptance: Both native Windows compiler variants build and pass their complete required suite, including the CRT-macro regression.

## CC-009: Windows CRT mode-generator output counts

- Type: host portability
- Status: In progress
- Target: Native Windows x64 compiler generators
- Observation: The inherited mode generator uses `printf` `%n` twice to measure comment padding. Windows UCRT disables that conversion by default, leaving the padding count unset. The hosted build failed while reading generated `min-insn-modes.c`; the runtime behavior provides a concrete generator defect to repair.
- Evidence: [Windows run](https://github.com/adesutherland/z-pdos/actions/runs/37065961572) and [Microsoft CRT documentation](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/set-printf-count-output?view=msvc-170). Both sites now use the return value from `printf`. Local MVS/CMS compiler and code-generation checks pass; all three generated outputs match their earlier bytes. A printf-count-disabled probe fails all three original generator paths and passes all three repaired paths with identical output.
- Acceptance: Both native Windows compiler variants and the complete installed-toolchain suite pass on the hosted runner.
