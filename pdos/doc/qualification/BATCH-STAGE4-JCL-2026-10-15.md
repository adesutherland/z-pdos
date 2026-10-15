# PD-033 Stage 4 direct converter — 15 October 2026

S4-01's portable direct-JCL core is implemented and host-checked. The complete
Stage 4 acceptance remains open: no PROC expansion, console/reader integration,
native JCL execution or guest acceptance is claimed. The
[contract](../architecture/BATCH-STAGE4-JCL.md) describes the implemented subset
and the [PD-033 backlog](../BACKLOG.md#stage-4-jcl-procedures-and-console-submission)
retains all later criteria and the descriptor capacity gap identified by the
Mainframe Operator's read-only review.

## Source and review

The source base is published z-pdos `4082eb79fdd5df922689d16be18ff90d0f091ddc`.
The working increment adds `pdos/src/twospace_jcl.{c,h}`,
`pdos/tests/batch/jcl.c`, `pdos/scripts/batch-jcl-check.crexx` and SYSTERM in
`pdos/tests/batch/direct-version.jcl`. Its source aggregate is SHA-256
`645d30dc6149da7c8905df5968320ea34a0906787965949a2b6bdf6307707451`.
Hash UTF-8 lines `path + space + SHA256(file bytes) + newline` in sorted path
order for those five files. Documentation is outside this code/input identity.
The existing job/DD core and headers are unchanged from the published base.

The source review covered complete-input admission, character conversion,
continuation/source positions, symbol and PARM bounds, duplicate/concatenated
DDs, read-only callbacks, immutable explicit attributes, error-result clearing,
supplied storage, build selection and the existing descriptor limits. It
corrected library continuation, an explicit library-pointer guard, quoted
continuation blanks, literal ampersands and ambiguous DD * framing. Native
semantics that this increment does not implement are rejected, including
parenthesized PARM and SPACE. Clang static analysis reports no findings.

## Commands and result

`crexx -nokeep pdos/scripts/batch-jcl-check.crexx` passed in the normal fixed
`build/pdos/batch-jcl` directory, including successful replacement/rebuild in
place. The recipe checks consumers and refuses symlink outputs; no disk copy,
guest, configuration, terminal session or device operation is involved.

- Normal C89, `-Wall -Wextra -Werror -pedantic`: **14,653 assertions pass**.
- Same controls with AddressSanitizer and UndefinedBehaviorSanitizer:
  **14,653 assertions pass**, with no sanitizer diagnostics.
- Those controls include **7,680 single-native-byte mutation cases**, plus
  semantic/error cases for exact descriptors, continuation/quoting, symbols,
  JOBLIB/STEPLIB order, records/delimiters, temporaries, truncated input/site refusal,
  resolver attribute tampering and resource limits. Every successful mutated
  deck passes the existing K descriptor validators; rejected decks return no
  public descriptor. Malformed complete-deck controls make no site callback.
- Classic C31 compile and Classic assembler `s370`/desktop assembly pass:
  **4,945 statements, one section, 486 symbols and 204 fixups**. This is an
  object check, not a linked/executed U application result.
- Workspace: **22,012 bytes**, within the 64 KiB selected converter envelope.

Host: macOS arm64, Apple Clang 21.0.0 (clang-2100.1.1.101). Classic tool hashes:

| Normal tool | SHA-256 |
| --- | --- |
| `build/compiler/mvs/gcc/mf-classic-cc` | `2c2d8e8d6ccf6bfa9f3737fecbf2fdfc2b3917558f3692c2b78734eb7016ed13` |
| `build/tools/assembler/mf-classic-as` | `03cbc4b3bd2c8ab8ad77ee90d7eaca16ed66b62abe4792b24e76f9792544fbee` |

No Stage 3 guest replay was needed: K, native DD/runtime behavior and the
qualified package are unchanged. S4-02 must resolve the four-step PROC's
expanded DD capacity before service integration; S4-03 owns console/reader
intake, durable source/step identity and control/output. Final integrated source
review and a new input freeze precede native-first operator qualification.
Keep summaries only and reuse the fixed build path.
