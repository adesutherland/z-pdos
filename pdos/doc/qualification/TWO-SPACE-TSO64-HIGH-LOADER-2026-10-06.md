# Bounded TSO64 HIGH AL8 materializer, 6 October 2026

The successor's Classic C31 native loader now has a separate
`TSTHEADER64HIGH` and `TSTIMAGE64HIGH` path. It accepts only a checked
AMODE64/RMODE64 directory byte `0x31`. The caller supplies a full-width U
base, K-only image staging and a bounded K-only relocation-offset vector.
The materializer copies text into staging, validates AL8 relocation sources,
sizes and target bounds, sorts the offsets, rejects overlapping targets and
overflow, and preflights every old target before applying any relocation.
The caller must publish U pages only after success. Its ANY parser still
rejects HIGH members. Nothing in this path uses low U virtual storage.

The retained `two-space-tso-high.crexx` recipe validates the three exact
unchanged RDW hashes in the [HIGH package inventory](TWO-SPACE-TSO64-HIGH-BOUNDARY-2026-10-06.md),
compares the new image byte for byte with the released `fixPEHigh` loader at
`0x110000000`, and checks a second full-width base by normalizing every AL8
target before comparing all nonrelocated bytes. It checks insufficient image
capacity, insufficient relocation slots, low and unaligned bases, and HIGH
rejection by the ANY parser. Under AddressSanitizer and UndefinedBehaviorSanitizer,
all three members passed:

| Member | Image bytes | Checked AL8 entries |
| --- | ---: | ---: |
| `RXCH` | 3,916,064 | 27,130 |
| `RXASH` | 1,775,648 | 5,577 |
| `RXVMH` | 1,799,472 | 4,941 |

The recipe also compiled, assembled and linked the new source with the local
Classic C31 toolchain under `build/pdos/high-loader-1/`. The diskless K/U
gate at `build/pdos/high-loader-gate-1/run/receipt.json` passed 93/93
checks. A fresh source-built 3390 IPL at
`build/pdos/high-loader-ipl-1/run/receipt.json` passed 147/147 unchanged
selected CMS and TSO checks. Source core SHA-256 was
`0a8965fb6aa1c2a578037006dccf1ff8037c949445873b9565d48129c5a27b07`.
Disk SHA-256 was
`b7bb4d4ac0f09f293a2f022b8aa639ee90489fbe7eadb9668e1f6bb72090618a`
before and after IPL; host exit was zero with no timeout. The machine was
one model-2064 ESAME CPU, 256 MiB real, 3390 `01B9`, 3270 `0009`, using
the pinned toolchain and unchanged application stages in the
[invocation gate](TWO-SPACE-INVOCATION-GATE-2026-10-06.md).
The existing pinned TSO24/31/64 ANY loader recipe also passed its two-base
byte comparisons and malformed-input controls after the shared parser change
under `build/pdos/high-loader-low-regression-1/`.

This is a host materializer and selected-regression result. The HIGH bodies
are not yet staged on the diagnostic disk, mapped by K, entered through the
unchanged launchers or unloaded through native LOAD/DELETE. The normal
successor image and HIGH guest qualification remain P3/P5/P6 work.
