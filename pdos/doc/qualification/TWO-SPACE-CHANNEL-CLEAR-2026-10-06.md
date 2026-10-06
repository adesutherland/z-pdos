# Event-verified subchannel clear, 6 October 2026

The K C31 channel wrapper now has `TSCCLEAR`. It issues CLEAR SUBCHANNEL and
returns success only after TEST SUBCHANNEL reports a status with the clear
function bit. IBM *z/Architecture Principles of Operation*,
[SA22-7832-14](https://www.ibm.com/docs/en/module_1678991624569/pdf/SA22-7832-14.pdf),
specifies opcode `B230`, the asynchronous clear function and the SCSW clear
bit. The maintained Classic assembler does not yet accept the CSCH mnemonic,
so the privileged wrapper emits the independently checked four instruction
bytes `B2300000` directly. This is confined to the assembler channel stub.
The architectural clock bounds a stalled wait; it never decides that a clear
succeeded. A nonoperational subchannel returns failure immediately.

The source-built diskless gate at `build/pdos/channel-clear-2/run/receipt.json`
passed 90/90 checks, including the absent-subchannel failure control. A
private idle-terminal clear gate passed after the normal 3270 read; the fresh
3390 IPL at `build/pdos/channel-clear-ipl-1/run/receipt.json` passed 143/143
checks. The source core SHA-256 was
`27bf3cb5cbc18694c00c0ee2c30f09ab8eb5260ae6380b29291dcacb08be375b`.
The disk hash was
`b81d2742ace42b4ff01d37040c669b256c60fe026b81e8dd5f05b3be54374257`
both before and after IPL. The host exited zero without a timeout. The
machine was one model-2064 ESAME CPU, 256 MiB real, 3390 `01B9` and 3270
`0009`, using the pinned toolchain and unchanged application stages in the
[CMS invocation gate](TWO-SPACE-CMS-INVOCATION-2026-10-06.md).

This proves the helper and an idle clear completion, not cancellation of a
live READ MODIFIED, late completion isolation or invocation-owned terminal
leases. Those guest cases remain required for P1 and P4.
