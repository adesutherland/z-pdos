# CMS31 FST lookup through the U veneer, 6 October 2026

The successor's U-owned lowcore page now carries a callable SVC 205
veneer. A bounded K C31 service reads the checked 3390 VTOC, recognizes
CMS31 two-part dataset names, builds a 40-byte FST in U-owned lowcore
storage at `0x300`, and returns its U pointer in R1 with an advancing
cursor in R0. The service caps cursor and catalogue visits; it does not
hand K pointers to the application. This is a selected CMS31 catalogue
route, not a complete CMS file API.

The no-file fresh IPL passed 116 checks at
`build/pdos/two-space-cms-fst-ipl/run/receipt.json`. With only the two
RXVM MODULE stages installed, the veneer returned the expected absent
status. A second disposable 3390 contained the pinned CMS31 IOQUAL RXBIN
file stage (SHA-256
`9b436a1eb9f54bbc5c8b8b7caae634bd17dcfae6559d584a1a00a87565cfa5d4`).
Its fresh IPL passed 116 checks at
`build/pdos/two-space-cms-fst-cursor-ipl/run/receipt.json`, including
R15 0, R0 cursor 1, R1 `0x300`, the exact `IOQUAL  RXBIN   ` FST name/type,
mode A1 and the selected 256-byte record-cap field. Both runs retained
unchanged CMS24/CMS31 RXVM `-v`, live 3270 output, wide heaps and fault
controls. The diskless host gate passed 89 checks at
`build/pdos/two-space-cms-fst-cursor-build/run/receipt.json`.

The initial positive run found the dataset and filled the FST but returned
the wrong cursor because the nucleus copied descriptor offset 12 (transfer
direction) to R0. The corrected handoff copies offset 8 (length/cursor);
the subsequent fresh IPL passed. The guest completed by disabled wait,
with watchdogs only detecting stalls.

Both runs used one model-2064 ESAME CPU, 256 MiB real storage, 3390 `01B9`,
3270 `0009`, pinned unchanged CMS24/CMS31 RXVM stages, local GNU binutils
2.47, maintained Classic C/Assembler/Linker and local Hercules. For the
positive run, the source core SHA-256 was
`ad5cda8e50b0e955bf916b06298279f0cca0ef35d63d81b7d083d86d91389464`;
the disposable disk SHA-256 was
`3281f24936de7a5ee033c5135452aee32b5d543d4edec85db7e03a4f2d071c3f`
before and after IPL.

This proves a real guest catalogue lookup through the CMS31 lowcore
contract. It does not yet show RXVM opening IOQUAL, reading its RXBIN
records, handling CMS file writes, running native CMS-to-CMS calls, or selecting
the successor as the normal OS image.
