# Classic compiler and assembler roadmap

1 October 2026. I want the Classic and ELF tool families to use consistent
machine-profile names and instruction limits. I also want us to choose the
compiler/assembler interface deliberately, now that we maintain both tools.
The source consolidation comes first; these architecture choices do not hold
up preserving and building the repaired compiler.

The current available-source checkpoint integrates Mike's v6 series and manually
reconciles the relevant described failures. His later reported 18-patch series
has no available source bundle in the native delivery decks. Keep its inventory
unverified rather than holding up feasible repairs or inventing patch identities.
Compare the full newer series if it becomes available.

## First compiler-to-assembler contract

The current compiler emits HLASM-style text with `COPY PDPTOP`, PDPCLIB
prologue/epilogue macros, `EQU *`, literal pools and address literals. The first
independent assembler attempt stops at `COPY`. That is a source-language gap,
not evidence that System/370 cannot execute the program.

We should inventory actual output and choose a bounded compiler input dialect.
For each missing construct, record the cost and effect of teaching the
assembler, changing compiler emission, or using an explicit compatibility
adapter. Prefer ordinary independently authored assembler facilities when
they also serve PDPCLIB or OS sources. Prefer simpler compiler emission when
it avoids a feature without weakening the machine contract or changing the
calling convention. Keep this decision reviewable; a blanket choice to reduce
compiler capabilities or implement all of HLASM is premature.

Acceptance is a repeatable C fixture that compiles with `mf-classic-cc`,
assembles with `mf-classic-as`, links with `mf-classic-ld` and runs in the named
guest, with exact runtime and guest inputs recorded. Start with integer code
on the historical 24-bit route. Keep as370 out of the implementation and recipe.

## Shared machine profiles

[The shared profile contract](../../architecture/MACHINE-PROFILES.md) records
the ELF SDK names and ceilings. A profile must constrain compiler emission and
assembler acceptance, and its final object must be independently checked.
The broad assembler `s370` selection is not automatically the narrower
`s370-int1` integer contract. Inline assembly and macro expansion need the
same instruction validation as compiler-generated instructions.

The Classic backend currently emits IBM hexadecimal floating-point code;
the SDK's historical integer profiles exclude floating point. Decide the
software-helper lowering and runtime closure, or reject such code in those
profiles. Do not silently admit floating-point instructions under `s370-int1`.

AMODE24/31 entry, residence and service rules need explicit Classic adapters.
GCC 3.4.6's present 32-bit pointer backend cannot claim `tso-zos64-v1` support.
The Classic object identity and ABI must remain distinguishable from the
ELF SDK even when both share a guest and ISA ceiling.

## Subsequent qualification

- Close the external-name policy, including explicit maps for existing runtime
  libraries, overflow diagnostics and cross-unit collision handling.
- Build maintained PDPCLIB and compiler helpers with one consistent compiler
  and name policy, then qualify realistic cREXX applications at each affected
  optimization level.
- Qualify 31-bit entry and high residence separately from the 24-bit route.
- Consider CMS-hosted compiler work after the cross-built toolchain works.
- Treat 64-bit Classic code generation, later ISAs and counterfactual profiles
  as separate backend decisions with their own tests.

## Repaired PDIO1 OS consumer — 1 October

The [repaired PDIO1 OS baseline](../../os/pdos/README.md) is now retained
with its exact original qualified input preserved by Git commit. The active
build uses the single maintained PDPCLIB with merged fixes. All 17 C units compile with this
producer. Its [build plan](../../os/pdos/BUILD-PLAN.md) makes the OS a
concrete consumer for source handling, instruction coverage and full pipeline
qualification. The imported 31-bit kernel also has z/Architecture support;
its profile is distinct from a historical integer application profile.

## Selected floating literal dialect — 1 October 2026

We selected exact XL4/XL8 HFP literals from the backend's existing target
encoder, retaining its rounding and literal widths. This removes duplicated
decimal E/D parsing and rounding from the first assembler target without
changing the machine instructions, ABI or floating representation. The
assembler adds only the reached historical HFP instruction subset. Integer-only
SDK profiles still exclude this code; whole-source object success does not
qualify a shared named profile or guest execution.
