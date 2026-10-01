# PDPCLIB issues and dispositions

## MVS FILE update state — fixed in the consolidated source

The earlier mirror allocated a FILE with `malloc`, read its `update` flag
in `osfopen`, then initialized that flag in `fopen3`. The canonical upstream
already moved initialization into `checkMode`, before native open. We adopted
that fix once during the 1 October runtime merge.

The merged host fixture seeds FILE allocations with a stale update flag.
Ordinary MVS and PDOS390 opens must clear it before native services and return
with update disabled. Removing early initialization fails those controls;
the MVS trace-open path also fails UBSAN. This resolves the recorded source
initialization concern without claiming complete guest libc qualification.

## MVS binary update support remains partial

Upstream describes `r+b` as an attempted update path and `w+b` as a write-only
fallback. Earlier initialization exposed a regression: the write-only handle
was primed with DCB/read/point services for an update handle. The merged source
excludes `w+b` from that post-open block, preserving the earlier native write
behavior. Both spellings are covered by host tests and a failing guard-removal
control. This does not establish complete MVS update/seek/reopen semantics.

PDOS390 still rejects update modes before native open because SAPSUPA does not
provide that service. Full guest file-mode and reopen qualification remains a
later library gate, separate from PDLD's corrected nonzero-origin relocation.
