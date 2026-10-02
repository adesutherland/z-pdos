# pdos agent guide

Read [the root guide](../AGENTS.md), README.md, UPSTREAM.md, LICENSE,
[AI guidance](doc/ai/README.md), the user and architecture documentation,
and [the backlog](doc/BACKLOG.md). Follow [the shared workflow](../doc/WORKFLOW.md).

Maintain one current product source tree in src/. Keep tests in tests/,
maintained cREXX recipes in scripts/ and generated output in root build/.
The optional archive/ is frozen and excluded from normal builds/required tests.
Record defects and future work only in doc/BACKLOG.md; Open does not authorise
implementation. Preserve behaviour during structural work and repair only
migration regressions. Do not commit, push or publish without session authority.

The current C32 kernel is AMODE31/RMODE24 with z/Architecture support and qualified wider application contexts. Consume only maintained PDPCLIB with the explicit pdos-zarch selection; do not copy another library into source control. Preserve unchanged-binary behaviour and the recorded 0.1 inputs. Guest operation requires Mainframe Lab guides and leases.
