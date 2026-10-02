# compiler agent guide

Read [the root guide](../AGENTS.md), README.md, UPSTREAM.md, LICENSE,
[AI guidance](doc/ai/README.md), the user and architecture documentation,
and [the backlog](doc/BACKLOG.md). Follow [the shared workflow](../doc/WORKFLOW.md).

Maintain one current product source tree in src/. Keep tests in tests/,
maintained cREXX recipes in scripts/ and generated output in root build/.
The optional archive/ is frozen and excluded from normal builds/required tests.
Record defects and future work only in doc/BACKLOG.md; Open does not authorise
implementation. Preserve behaviour during structural work and repair only
migration regressions. Do not commit, push or publish without session authority.

GCC/i370/cc370 source and derived repairs retain GPL terms and exceptions. Keep the inherited configure/build tree in src/. The original launcher emits assembly text and rejects unsupported driver operations; changing that behaviour is outside a structural migration. Compile MVS and CMS variants with their current fixtures.
