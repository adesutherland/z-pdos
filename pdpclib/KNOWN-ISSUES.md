# PDPCLIB issues outside the current consolidation

## MVS FILE update state before initialization

The focused C host harness found a preexisting state-initialization question:
`fopen` allocates a FILE with `malloc`, `osfopen` reads its `update` field, and
`fopen3` initializes that field later. The MVS control explicitly uses zeroed
fixture allocations; it does not establish initialization by target `malloc`.
The PDOS390-only guarded path excludes that post-open block.

The maintained host tests separately seed the update flag to exercise the
existing MVS behavior and the PDOS390 exclusion. They are not a repair or
qualification of the inherited allocation path. Before full MVS libc
qualification, reproduce the fresh-allocation behavior and define initialization
and reopen semantics with a regression case. This question is separate from
PDLD's confirmed nonzero-origin relocation error.
