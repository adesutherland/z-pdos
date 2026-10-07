# pdos agent guide

Read [the root guide](../AGENTS.md), README.md, UPSTREAM.md, LICENSE,
[AI guidance](doc/ai/README.md), the user and architecture documentation,
and [the backlog](doc/BACKLOG.md). Follow [the shared workflow](../doc/WORKFLOW.md).

Final code review, resolution of findings and a recorded source/input freeze
must precede expensive guest qualification. A code repair returns the candidate
to review before affected requalification. The later evidence/documentation
acceptance review is separate; reuse unchanged results with recorded identity
and dependency justification. Follow the root guide's review-before-qualification
gate for all PDOS work.

Maintain one current product source tree in src/. Keep tests in tests/,
maintained cREXX recipes in scripts/ and generated output in root build/.
The optional archive/ is frozen and excluded from normal builds/required tests.
Record defects and future work only in doc/BACKLOG.md; Open does not authorise
implementation. Preserve behaviour during structural work and repair only
migration regressions. Do not commit, push or publish without session authority.

The earlier 0.1.1 C32 kernel is AMODE31/RMODE24. The 0.2 source/release default uses a K64 nucleus, protected K C31 services and shared U application space; PD-003 P0–P6 own its accepted scope. Consume only maintained PDPCLIB with the explicit pdos-zarch selection; do not copy another library into source control. Preserve unchanged-binary behaviour and the recorded 0.1 inputs. Guest operation requires Mainframe Lab guides and leases.
