z/PDOS 0.2.0 introduces a protected kernel address space (K) and one shared
application address space (U). A 64-bit assembler nucleus handles interruptions
and dispatches Classic C31 services in K; PCOMM and unchanged native
AMODE24/31/64 applications run in U. The normal machine uses one z/Architecture
CPU and 256 MiB real storage.

The completed P0–P6 qualification covers unchanged CMS31 and TSO31/TSO64
ANY/HIGH cREXX compiler → assembler → VM chains, real input and exact output
readback, selected native application calls, caller restoration, 3270 models
2–5, line/monitor operation and existing disk/tape workflows. CMS24 and native
TSO24 have separate library-free RXVM IO24 results. Full-library TSO24, CMS24
compiler/assembler and general CMS/TSO services remain outside this scope.

The base image contains the OS only; application packages are separate.
The release record distinguishes the locally operator-tested/installed image,
the hosted disk container and four-host tool packages. Read the bundled K/U
operator guide and qualification record before booting.

Classic C (MVS and CMS), Classic Assembler and Classic Linker are packaged for
macOS Apple Silicon/Intel, Linux x64 and Windows x64. macOS PKGs are Developer
ID signed and notarized. Windows CI assets are explicitly unsigned until the
separate local signing operation replaces them. SHA256SUMS identifies final
asset bytes; the source archive contains the corresponding maintained code.

Paul Edwards created PDOS and PDPCLIB. Component licences and upstream notices
accompany the downloads. The build requires no proprietary mainframe compiler,
assembler, binder, IBM macro library or prebuilt mainframe objects. GNU s390
Binutils build the K64 nucleus; no IBM operating system or private guest disk
is included.
