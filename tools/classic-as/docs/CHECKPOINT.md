# Local implementation checkpoint

30 September 2026. The user requested a clean pause, migration into this
repository, a local snapshot commit, and then restarted agents working here.
All assembler implementation files were already written here. The five
assembler/naming/design/inventory documents formerly held in the research
repository are now owned here; only a short ownership pointer remains there.
Historical licensed tools, private guest outputs and unrelated changes remain
outside this source tree.

## Saved state before restart

- Original `values.c`, `reader.c`, `machine.c`, `assemble.c`, `object.c` and
  standard-C `main.c` are present, with the public header and own MIT licence.
- Values, reader and encoder unit suites passed strict C89 and ASan/UBSan.
  Independent codec and integer-oracle checks passed. The coordinator also
  compiled and ran the three sanitizer unit executables independently.
- Object writer tests passed 661 checks with strict C89 and ASan/UBSan,
  including name/ID/range validation, written fixup coverage, gaps and failed
  begin/write/completion. The coordinator independently reviewed its records
  and failure handling and ran the sanitizer unit suite.
- The first actual CLI deck passed 149 independent checks across 13 cards:
  two sections, entry offset 2, four A/V relocations, modes and reserved gaps.
  The independent checker rejected nine targeted corruptions during its QA.
- The engine builds with strict C89 and creates the agreed fixture deck.
  Its dedicated unit suite was still being written when paused; it is **pending**.
  Full integrated QA and final documentation review are also pending.
- The migrated snapshot's cREXX/CMake pipeline passed all seven registered
  checks. Engine unit registration is visibly conditional at this checkpoint;
  seven passing checks do not imply completion of that pending suite.

## Restart scope

Work only in this repository. Finish the engine's original unit suite, including
replay change, symbol/section/fixup/expression/storage limits, invalid operands,
unresolved/duplicate names and failing providers/writers. Verify constant widths,
signed bounds and long internal names. Resource use must not grow with the count
of repeated unlabeled statements.

Then run the integrated strict-C89 build, independent deck checker and focused
sanitizer QA. Review CLI output preservation and diagnostics; check that the user
guide describes actual support. Remove the temporary conditional engine-test
registration once its source exists so later builds require the complete suite.
Retained development orchestration is `tools/build.crexx`; direct C compilation
remains documented for hosts without cREXX/CMake.

The implementer owns QA and the coordinator independently validates delivery.
Agents use GPT-6.1 Sol Extra High for the engine/complex work and High for defined
review tasks. No inherited assembler code, opcode table, private macro source,
guest run, OS-service work, compiler repair or public push is part of this seed.
Complete runtime/OS assembly and native z/PDOS hosting remain later gates.
