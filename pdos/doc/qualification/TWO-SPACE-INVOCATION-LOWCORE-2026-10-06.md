# Nested invocation lowcore snapshot, 6 October 2026

When a child enters after the CMS31 compatibility lowcore has been mapped,
K allocates one 4 KiB K real frame, saves the current U lowcore page there,
and records that frame as the child's `TSV_LOWCORE` resource. The child sees
the clean compatibility template. On child return, the K invocation cleanup
restores the parent's bytes and releases the K real frame before the parent
resumes. A failed save allocation leaves the parent's lowcore untouched.
This adds no low U mapping and preserves the 24-bit application interval.

The first fresh IPL failed one check: top-level entry reset the CMS FST
veneer page after the diagnostic catalogue had prepared its result. The
captured FST was zero despite the unchanged CMS workload otherwise running.
The implementation now resets only on nested child entry, when there is a
parent snapshot to restore. It does not claim isolation between unrelated
top-level invocations yet.

The final Classic C31 and diskless machine gate at
`build/pdos/invocation-lowcore-3/run/receipt.json` passed 91/91 checks.
The fresh source-built 3390 IPL at
`build/pdos/invocation-lowcore-ipl-2/run/receipt.json` passed 144/144
checks, including the CMS31 FST catalogue, unchanged CMS and TSO entries,
and a real nested K descriptor control. That control observes a clean child
byte, writes a different child byte, and observes the parent's original
byte after child return. The source core SHA-256 was
`6a8ae00edcb864e73d794809cec59f431178c3ad1669a113caceb2ffa94fa761`.
The disk hash was
`000e0e453c79dfb49a9cc651489944e75c441475f10295c47556e88415a2589f`
before and after IPL. The host exited zero without a timeout on one
model-2064 ESAME CPU, 256 MiB real, 3390 `01B9` and 3270 `0009`, using
the pinned toolchain and unchanged stages in the [CMS invocation
gate](TWO-SPACE-CMS-INVOCATION-2026-10-06.md).

The nested control is a K descriptor and U byte probe; it is not an
unchanged native application calling a second native application. Fault
unwind, top-level personality transition, terminal lease and pending I/O
cleanup remain separate P1/P3 gates.
