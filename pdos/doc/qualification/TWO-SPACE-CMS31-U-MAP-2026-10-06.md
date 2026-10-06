# Two-space CMS31 U mapping checkpoint, 6 October 2026

I used the same local GNU binutils 2.47 s390-linux-gnu tools, Mainframe
Classic C/assembler/linker and Hercules 4.9.1 single-CPU ESAME model 2064
as the preceding diagnostic IPL. The disposable 3390 disk included pinned
CMS24 RXVM and CMS31 RXVM v2 stages from the checked Mainframe Classic Tools
archive. The CMS31 stage SHA-256 was
`a08f3431407f8efec6b05f1e0580e3f12b76f9865f561dac15b5a90135223ff9`.

After the CMS24 full-stage check, K found `CMS31.RXVM`, read 239 F/18452
blocks into a temporarily reserved K-only real interval, validated the
complete stage and allocated 4,238,296 image bytes in U at `0x03000000`.
The C31 loader copied the image through K's real aperture and applied its
relocations. The stage reservation was released. A 16 MiB core snapshot
showed contiguous U page translations to real `0x200000` onward and matching
source image bytes. The complete fresh IPL machine gate, including terminal
input, isolation, storage and failure controls, passed. An earlier gate run
failed only its old exact U-table-size expectation after the new mapping;
that expectation now checks the initial size separately from subsequent
allocated tables, and the fresh run passed.

Hercules now writes a dedicated console log. The harness waits for the
observed screen and disabled-wait completion before issuing `savecore`; time
limits only detect stalls. The diskless gate passed with the absent-dataset
return code. This is a mapped, relocated MODULE proof, not RXVM execution.
CMS24 fixed-origin loading, CMS and TSO services, application-call semantics,
wide heaps and normal successor selection remain open.
