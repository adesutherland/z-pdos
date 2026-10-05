# Two-space production slices 3/4: interruption and U-buffer gate, 5 October 2026

We extended the successor fixture from the checked slices 1/2 handover and
guest-built DAT. This remains a bounded, one-CPU, 16 MiB proof. The released
z/PDOS image still selects the one-ASCE C32 kernel.

## Implementation and machine profile

The real DAT-off island now has new PSWs for SVC, program, external, I/O and
machine check. Two bounded frames retain every 64-bit GPR, the old PSW, CR1,
interruption class and SVC number. The high K64 dispatcher returns through
that island to the saved address space, addressing mode and key. A nested SVC
from K runs while K DAT is enabled and resumes its outer service. The selected
U page-translation fault returns to an explicit U recovery continuation with
`-4`; unexpected program faults and machine checks take a bounded wait.

Product `twospace_dat.c` now walks a full-width U virtual address through the
K-only U table alias. Product `twospace_gate.c` checks length, 64-bit range
wrap, each translation, real-frame bounds and access direction before a copy
of up to 256 bytes through K-owned C31 storage. The named fixture uses a
K-only `0x08000000`–`0x08ffffff` virtual aperture for its 16 MiB real profile
and an explicit per-frame read/write policy. Its Classic C31 endpoint receives
only a K descriptor. SVC 202, 204 and 1 select distinct CMS24, CMS31 and TSO
*probe* results; they are not complete IBM service adapters.

The macOS arm64 host used Mainframe Classic C 3.4.6, Classic Assembler
0.1.0-bootstrap, Classic Linker (PDLD 0.19 lineage), GNU Binutils
2.47.20260726 with `-march=z900`, and Hercules 4.9.1.0-SDL. The guest was one
model-2064 ESAME CPU with 16 MiB real storage; the IPL case used a disposable
100-cylinder 3390 at `01B9`. Source, tool, binary, disk and run identities
are in ignored `build/pdos/two-space-s34-final6/run/receipt.json` and
`build/pdos/two-space-s34-ipl-final6/`.

## Checks and observations

The cREXX diskless recipe compiled and linked the C31 service/gate/DAT code
with Classic tools, ran C89 DAT, placement and transfer controls under
ASAN/UBSAN, including a 256-byte cross-page copy and a malformed table
control, then passed all 66 Hercules checks. Nine U service requests
crossed from AMODE24, AMODE31 and AMODE64. Full-width GPR sentinels survived
every crossing. The guest observed one nested K SVC, one recoverable U program
fault, a valid read spanning two U pages, a write to U data, and refusals for
a forged high address, a U code-page write, a wrapped address and length 257.
The CMS24, CMS31 and TSO probe results remained separate. The fixture
retained only two mapped U pages below 16 MiB, leaving 16,769,024 low U
virtual bytes unmapped before application allocation.

External and I/O entry paths returned from planted old PSWs during bootstrap;
these are synthetic entry tests, not asynchronous device interruptions. A
second diskless run selected the machine-check entry synthetically and reached
its fail-stop marker `5` without a program-interruption loop. A third run
selected an unexpected U fault PC and reached fail-stop marker `-1` rather
than retrying it. The normal IPL did not inject a machine check.

The checked `KCORE.BIN` package contained 16 nonzero core pages, seven U/18452
records and 129,164 bytes, with no completed DAT tables or ASCEs. The guest
C31 stage reserved the zero-filled context and service frames as well as
supplied pages before choosing table pools. It built K and U ASCEs at real
`0x100000` and `0x140000`; the final K tables used 200,704 bytes/49 frames
and U used 94,208 bytes/23 frames. The fresh IPL passed all 73 checks,
including exact host/guest DAT comparison, full service/fault observations,
checked handover and an unchanged disk hash.

| Artifact | SHA-256 |
| --- | --- |
| Source core | `61530949fd5d5892ca29bb4ceeee7a9e9ff632664d8c185d332ec4fcc660fbf0` |
| Bare core in package | `7da9fa2fbdeeeb03880e6227243a2a898957068a67e145c5d01377aa7e0d1f71` |
| `KCORE.BIN` | `ad4b10480f561b18974c05d0554666476fdbe91dac04276715d79937b4b56545` |
| Native C31 IPL stage | `6aafcea7f129383b4e0991d9c6278f7028f978c0e885ad77963f21e934232d5c` |
| Checked CCKD IPL disk | `ca399f172391c5e5602a7440934e01cd8f05b95631cc0d78bc538d2237e77cfc` |

Reproduce with fresh ignored `build/pdos/` directories, using the exact tool
paths for this host:

```sh
crexx -nokeep pdos/scripts/two-space-next.crexx --args \
  build/pdos/two-space-s34-new "$GNU_AS" "$GNU_LD" \
  "$CLASSIC_CC" "$CLASSIC_AS" "$CLASSIC_LD" "$HERCULES"
crexx -nokeep pdos/scripts/two-space-ipl.crexx --args \
  build/pdos/two-space-s34-ipl-new \
  build/pdos/two-space-s34-new/run/image.core \
  "$GNU_AS" "$GNU_LD" "$HERCULES_BIN_DIR"
```

## Boundary

This qualifies the fixture's five-class routing and bounded transfer path,
not a complete interruption or IBM API implementation. Actual asynchronous
external/I/O delivery and device acknowledgement, hardware machine-check
injection, deeper nesting, SMP, dynamic U frame rights, DAT-on map changes,
running-K frame reclamation and larger real-memory apertures remain open.
No unchanged CMS or TSO application binary ran; the SVC selections are probe
conventions. The shared-U loader, real OS services and low-memory budget
against actual applications are later [PD-003](../BACKLOG.md#pd-003-two-space-supervisor-and-shared-application-memory)
gates.
