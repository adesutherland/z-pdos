# TSO64 HIGH package and loader boundary, 6 October 2026

The unchanged `CREXX-v1.0.0-beta.3-mainframe.zip` package, SHA-256
`fd11ae260bba169653126a861ecae545cee9bcfbe1fad4083c4a913241e3aedd`,
contains three AMODE64/RMODE ANY launchers and three separate
AMODE64/RMODE64 bodies. Mainframe Lab's existing `pdos64_xmit_unload.py`
removed only XMIT transport framing; its receipt validated each member name
and native directory byte. The ignored extraction and receipts are under
`build/pdos/tso64-high-inventory/`.

| Pair | Launcher XMI / RDW SHA-256 | High body XMI / RDW SHA-256 |
| --- | --- | --- |
| Compiler `LAC65O` → `RXCH` | `705bf0d375933da8e9a8633985fdd153bb61a2c2d228399b198688308cc2c194` / `0037a2ef67e59e0db0e1ac054c592c0feb00601a211b8882bc2b1c2e897f91ae` | `c5d141f086e2509d0f0222c20f1b7c25aba96f1889e7091e7aeb3fa2f2ec95d1` / `2f0d99baaaf9a6089fcaaa75de2bc945ea92eea16832f581ed9726584f43be0b` |
| Assembler `LAU65O` → `RXASH` | `4761c85d85f1cdc36080d20ab9d1474a25beafac22d9d79d650a869d371556cd` / `b2d7f4e0dfc070065664dceca2f003be187e309803f1f2ef5067ca57fb7ac8ba` | `ce97361a9dad704ceb29b2db3afc262d3fa0c6c19b7b149959120fd6db399a50` / `480cb94ea1f511956ae1df8ef3b632620c4048d690b4a922bacec599025c05f9` |
| VM `LAVM65O` → `RXVMH` | `0418b47b12a76de49317bd60326449c0fde7940afb3d0cfaf6dcba41a0ab261b` / `cd87b17f540952745240462f9c847011a449996e0e08725bc0dee286c4b3e527` | `61332683a9c47b3b425c484728401f42f17e4fd8075449f2b924f4936d6518b2` / `9d45e418f1ce45253ec0a09dfb3015d856c75bda96aa55cb5bf8dae411ac438f` |

All launchers have directory byte `0x11`; all bodies have `0x31`.
The native entry offset recorded in each of these six directories is zero.
The released one-ASCE loader uses separate LOAD/DELETE for the high bodies;
its high relocation form uses AL8. A successor materializer must use a
full-width U base, validate every relocation before publishing any image,
and keep its staging and real backing out of low U. It must also preserve the
launcher's parameter, return and unload behavior. These details are a design
input, not evidence that the successor runs HIGH.

The current two-space `TSTHEADER` and `TSTIMAGE64ANY` now reject an RMODE64
directory instead of classifying it as ANY. The host loader gate and diskless
machine gate at `build/pdos/high-boundary-1/run/receipt.json` passed 93/93
checks; the selected unchanged ANY guest paths passed 147/147 in a fresh
3390 IPL at `build/pdos/high-boundary-ipl-1/run/receipt.json`. Source core
SHA-256 was
`53e50695f889059ac53bddfc3ce92bdf609d6268bcfd0ecb735f387eeccf0528`.
Disk SHA-256 was
`a6b25cc16b7baa12c8d0cd2d35d3b33c88611a4765a15b27c2536ec0402f26ae`
before and after IPL; host exit was zero with no timeout. The machine was
one model-2064 ESAME CPU, 256 MiB real, 3390 `01B9`, 3270 `0009`, with the
pinned toolchain and application stages in the
[invocation gate](TWO-SPACE-INVOCATION-GATE-2026-10-06.md).

This checkpoint does not stage HIGH on the diagnostic disk, materialize an
AL8 body, or run a HIGH guest entry. Those remain P0/P3/P6 gates.
