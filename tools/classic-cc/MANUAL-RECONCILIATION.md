# Manual compiler reconciliation

1 October 2026. I chose manual consolidation because the different source
versions and native delivery packages do not give us a single automatic merge.
We use actual compiler code and repeatable failures to decide what to keep or
repair. A patch count or a report of a guest run cannot substitute for source
identity or our own qualification.

[SOURCES.md](SOURCES.md) records the pinned upstream, v6 patch reconciliation
and macOS repair. The later native build-86 and build-92 ZIPs download correctly,
but contain no newer cc370 patch source. We reviewed the relevant available
descriptions and recorded the following bounded checks.

| Described failure | Current check and disposition |
| --- | --- |
| Incorrect instruction-byte accounting overruns a 4 KiB code/literal page | v6 fixes combined with the newer upstream direct-jump guard; inherited 246-case page sweep passes. The supplied diagnostic audit measures 372 of 373 return templates, finds no under-reporting and 16 conservative over-counts. The remaining template is the empty same-register extension emission and was inspected. This audit is not final-object or whole-program proof. |
| Base register remains from the source page after computed goto | Forced-label reload repair retained; the independent multi-page dispatch fixture compiles at all four tested optimization levels, excludes R0 as the indirect branch address, emits `BALR` at page breaks and reloads at address-taken targets. No target execution claim. |
| Long comments turn the next card into a continuation | Reproduced after the v6 import: an initialized-variable comment had 103 columns and a common-variable comment had 101. Complete the bounded-comment path for both global and common emission; retain the function-comment repair. All emitted comments in the long-name fixtures now have at most 71 columns. |
| Long function names in comment formatting | A valid 700-character identifier made `cc1` trap in the v6 helper's `vsprintf` into a 512-byte stack buffer. Bound formatting with `vsnprintf`, then emit at most 71 columns. The fixture now compiles at all four tested optimization levels on both targets. |
| Implicit helper calls overwrite previously published call arguments | Retain v6 nontrivial-argument precomputation. A conversion-helper fixture checks that the first outer argument is published after the helper call at optimized levels. |
| GCC store motion deletes repeated outgoing argument stores at `-O2`/`-O3` | Added repeated-call and loop fixtures with a callee that overwrites its incoming argument. The tested `-O0`, `-O1`, `-O2` and `-Os` outputs republish each repeated argument; the focused repeated-call check also passes at `-O3`. These cases do not reproduce a remaining defect, so no general optimization switch was changed. Other source shapes and general `-O3` behavior remain unqualified. |
| Optimized `printf("\n")` loses its EBCDIC newline | Current upstream character/builtin repairs already address this. A focused fixture at `-O1`, `-O2` and `-Os` emits target NEL 21 and calls `PUTCHAR`; the existing EBCDIC fold regressions also pass. |
| Long-name hashing changes runtime linkage | The combined source initially hashed existing DI conversion/divide helpers to new names. Explicit helper aliases now preserve the established eight-byte runtime symbols. Hash arithmetic is fixed to 32 bits, with independently calculated names and cross-unit checks. |

The template diagnostic is a private supplied Python utility used from ignored
input staging; it is not imported into the product. The retained build and
regression orchestration is cREXX, with the inherited upstream test interface
preserved. Exact runs are recorded in [CHECKPOINT.md](CHECKPOINT.md).

Other described cREXX fixes concern its VM, scanners, UTF-8 boundary, imports,
library implementation or large frames. GCCLIB allocation and `fopen` defects
belong to that runtime. They are not patches to the C compiler, and the two
native delivery packages are not imported as replacement product source.
Floating formatting, runtime stack alignment and complete helper closure need
separate runtime/application qualification. as370 capacity and implementation
repairs remain excluded.

The later report's unnamed 18-patch inventory cannot be reconstructed exactly
from its broad summary. We do not claim four invented additional patches, an
exact copy of Mike's private tree or replication of his guest results. The
maintained available-source checkpoint includes the relevant inspected fixes,
the reproduced local repairs and regression coverage for the feasible described
cases. Compiler-to-independent-assembler integration remains the next gate.
