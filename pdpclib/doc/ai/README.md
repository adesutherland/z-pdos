# AI guidance for PDPCLIB

Read [the root agent guide](../../../AGENTS.md), [the component guide](../../AGENTS.md),
[the component overview](../../README.md) and [upstream record](../../UPSTREAM.md).
Use [BACKLOG.md](../BACKLOG.md) for defects, proposals and qualification gaps.
Follow [the shared workflow](../../../doc/WORKFLOW.md).

Maintain product code only in `../../src/`. Do not prepare product fixes as
patch layers or refresh frozen archives. Build and run affected checks after
a change, record their actual scope, and preserve unrelated work.
A reorganisation changes paths and build plumbing; functional defect repair
requires separate scope and must not be bundled into it.
