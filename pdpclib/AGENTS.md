# pdpclib agent guide

Read [the root guide](../AGENTS.md), README.md, UPSTREAM.md, LICENSE,
[AI guidance](doc/ai/README.md), the user and architecture documentation,
and [the backlog](doc/BACKLOG.md). Follow [the shared workflow](../doc/WORKFLOW.md).

Maintain one current product source tree in src/. Keep tests in tests/,
maintained cREXX recipes in scripts/ and generated output in root build/.
The optional archive/ is frozen and excluded from normal builds/required tests.
Record defects and future work only in doc/BACKLOG.md; Open does not authorise
implementation. Preserve behaviour during structural work and repair only
migration regressions. Do not commit, push or publish without session authority.

This is the sole maintained library source. Keep shared fixes in src/ and explicit service/profile differences in src/profiles/ and src/native/. Build profile selection joins maintained modules; do not restore patch-based preparation. Retain all Paul Edwards and contributor notices. Selected z/PDOS qualification does not qualify other library ports.
