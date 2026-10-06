# K-controlled return and child-fault unwind, 6 October 2026

The diagnostic image now uses private SVC 245 to enter one of two K-registered
U fixture entries. K records the caller's complete GPRs, old PSW, ASCE and
owned-image lease, checks problem state, key 8, AMODE64 and the U ASCE, then
changes the saved interruption frame to enter the child. The normal child
returns through a registered U trampoline and private SVC 246. A second child
allocates a U31 page through K and faults on a protected K-only page. The
program-interrupt path routes that child to the same frame cleanup, then
restores the caller's saved continuation. The application RC in R15 is kept
separate from the OS status in R0.

`build/pdos/controlled-call-4/run/receipt.json` passed 98/98 diskless
checks, including the no-image rejection control. The source-built fresh
3390 IPL at `build/pdos/controlled-call-ipl-4/run/receipt.json` passed
155/155. The normal child returned OS status 0 and application RC `0x2468`.
The faulting child returned OS status 12 and no application RC
(`0xffffffff` sentinel). GPRs 3 and 4 were restored after both children;
the image lease count and K controlled-call flag were zero at exit. The
faulting child's U page `0x02010000` was unmapped. The exact DAT purge count
included its map and unmap; the boot DAT checksums remained intact.

The machine was one model-2064 ESAME CPU with 256 MiB real storage,
3390 `01B9` and 3270 `0009`. Classic C, Assembler and Linker came from this
z-pdos checkout; GNU Binutils 2.47 and the Mainframe Lab Hercules install
built and ran the machine fixture. The unchanged CMS24/CMS31 and TSO24/31/64
ANY stages have the SHA-256 values recorded in the
[ABI inventory](../architecture/TWO-SPACE-ABI.md). The booted core SHA-256
was `de13778b68fcf6cbc80061ca887b32a7972d8ab575e594b6efbf75a9cbaf6423`.
Disk SHA-256 was
`c9edb578c6fef61401c0c7a8ac7f66535e59d033cb1692c92b349e34bca57919`
before and after IPL. Host exit was zero without timeout.

The entry addresses and return trampoline are fixture constants, not native
CMS or TSO application linkage. This proves a K-controlled return and one
recoverable child fault under the diagnostic layout. It does not yet prove
unchanged application-to-application calls, nested K-controlled children,
pending-I/O fault cleanup, every program-interruption class, or a normal
successor image. P1 and P3 remain in progress.
