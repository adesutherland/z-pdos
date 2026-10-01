# Frozen PDIO1 qualification runtime

These 44 files are the exact canonical SourceForge PDPCLIB input selection
from the repaired PDIO1 source tree. They preserve the kernel/runtime baseline
used for the 1 October cREXX guest qualification. [The OS source record](../../../os/pdos/SOURCES.md)
owns their provenance, complete selected-source manifests and recovery patch.

The top-level maintained library uses a different GitHub-mirror revision,
with its own consolidated changes. These files are a frozen qualification
reference, not a second destination for new shared library fixes. Reconcile
that lineage into an explicit maintained PDOS target with separate validation
before replacing this baseline in the OS build.

The OS preparation recipe copies this selection to its own
`build/pdos/.../source/pdpclib/`. Ordinary library profiles continue to use
the top-level source. The frozen selection keeps all 21 headers, 13 runtime
C units, four startup/support assembly inputs, four source-owned macros and
two runtime documents. Unrelated ports, VSE routines and inherited tool
recipes are outside this selection.

[LICENSE](LICENSE) retains the canonical PDPCLIB dedication and fallback
permission; individual files retain their author notices. External IBM
service/mapping macro implementations are not supplied by those terms.
