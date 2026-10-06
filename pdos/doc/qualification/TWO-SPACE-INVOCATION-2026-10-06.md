# K invocation ownership component, 6 October 2026

This is a bounded P1 component result for the diagnostic two-space successor.
It is not a native application call or a normal replacement image.

`twospace_invocation.c` and `.h` add a K-owned, single-CPU invocation stack.
Each frame records an explicit CMS/TSO personality, AMODE, distinct image
and runtime owners, parent token and full-width caller GPRs, PSW and ASCE.
The resource ledger covers images, allocations, files, lowcore, terminal
leases and pending I/O. Tokens increase for the whole IPL and never reuse a
released stack slot. A late completion carrying an old token therefore
cannot complete an I/O owned by a later child with the same handle. Cleanup
runs in reverse acquisition order; pending I/O and failed cleanup keep the
child in a reaping state and prevent parent resumption until a completion or
cancel event permits retry. No clock decides that transition.

The C89 sanitizer test exercises a CMS31 parent, CMS24 child and TSO31
successor child, a completion for a suspended parent, duplicate and stale
completions, pending-I/O cleanup retry, parent restoration, resource reuse
and token exhaustion. It passed. The maintained Classic C compiler and
assembler built the same source; the module is linked into the diagnostic
C31 service binary, with no call site yet. The full diskless service recipe
`pdos/scripts/two-space-next.crexx` passed 89/89 existing machine checks at
`build/pdos/invocation-next2/run/receipt.json`, with no timeout and host
exit 0. The Classic service image is 68,934 bytes in the 128 KiB K-only
slot, SHA-256
`9ef5761a6b5baa84ce70815d5a79981f45ccbe3097b994dfc4e7a8d00ef5339a`.
The fixture still has zero K pages in U's low virtual range; its 16,769,024
unmapped low U bytes are a small fixture result, not the CMS24 budget.

The profile was one emulated model-2064 ESAME CPU with source-built Classic
C/Assembler/Linker, local GNU binutils 2.47 and Hercules, using the
diskless service machine. The invocation source SHA-256 was
`fc04ef3c65131d000493eeb3d13b2245f7052ff58666ec193ebfdc36b92b3596`.
The test source SHA-256 was
`87ba0b050fc00512f627c889e7cfc056e346b632aba034dd7f9878a41334420a`.

The ledger is not yet wired to the K dispatch or the real loaders. The
current C31 service still infers native personality from saved PC ranges,
uses fixed fixture task owners, and has no resource cleanup callbacks or
serialized completion queue. The K interruption path must deliver events
to this ledger at a safe point; calling the C module reentrantly from an
interrupt would violate its contract. P1 still requires that integration,
fault and cancellation gates and a fresh IPL. P2–P6 remain open.
