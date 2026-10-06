# Invocation-owned CMS input cursors, 6 October 2026

This bounded P1 checkpoint gives each open CMS input cursor an invocation
token as well as a filename and 24/31-bit profile. A child requesting the
same file receives a separate cursor and K real buffer; a suspended parent's
cursor remains private. K records each newly opened input handle in the
invocation ledger, checks the active owner before FINIS releases it, and
releases unclosed input buffers on normal return. The diagnostic cursor
probe retains its explicit token-zero path. Output buffers remain a separate
shared diagnostic facility and are not covered by this checkpoint.

The Clang C89 cursor test passed under address and undefined-behavior
sanitizers, including same-name, same-profile lookups under different tokens.
The diskless source-built machine gate passed 89/89 checks at
`build/pdos/invocation-cmsfiles-1/run/receipt.json`. The fresh source-built
3390 IPL at `build/pdos/invocation-cmsfiles-ipl-1/run/receipt.json` passed
142/142 checks, including unchanged CMS31 IOQUAL and CMS24 IO24. The source
core SHA-256 was
`acd9f7dda9d7ddce5a06d40177433f38abdf76a81d6ffa68b03fb7bcc4901bd5`.
The disk SHA-256 was
`5e1c5f69a3eeda0987b6afd8aa1ba54b32a1af1d049000c9d617c686e8937e3b`
both before and after IPL. The host exited zero without a timeout. The
machine was one model-2064 ESAME CPU, 256 MiB real, 3390 `01B9`, and 3270
`0009`. The exact compiler/assembler/linker and unchanged application stage
identities are those in the [CMS invocation
gate](TWO-SPACE-CMS-INVOCATION-2026-10-06.md) and [ABI
inventory](../architecture/TWO-SPACE-ABI.md).

The IPL is a regression of current unchanged paths. A native nested child
opening the same name, a child fault and a failed channel start still need
guest controls. This checkpoint does not finish P1, normal K launch or P2's
persistent output behavior.
