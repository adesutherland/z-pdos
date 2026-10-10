# cREXX TSO31 bundle notices and source identities

The development image consumes the unchanged public
[cREXX v1.0.0-beta.3 mainframe package](https://github.com/adesutherland/CREXX/releases/tag/v1.0.0-beta.3),
SHA256 `fd11ae260bba169653126a861ecae545cee9bcfbe1fad4083c4a913241e3aedd`.
`bundle.json` records the native identities and installed members. The
transport decoder removes only XMIT framing; it requires the previously
qualified native RXC, RXAS and RXVM bytes.

- cREXX: [source commit ae1607b8e145174422cee7f3e73fbcc37a65226c](https://github.com/adesutherland/CREXX/tree/ae1607b8e145174422cee7f3e73fbcc37a65226c).
  `CREXX-LICENSE.txt` retains the MIT notice of Adrian Sutherland, Peter Jacob
  and René Jansen, copied from that commit's LICENSE.
- Runtime: the package's INPUTS.json names the modern Mainframe Cross SDK
  `0.1.0-local-v22` TSO31 64 MiB RXC runtime archive, SHA256
  `eeecfe3a9eeb191c53465243a44634315f4bb1123522481760f0e8c3d3598c2a`.
  `SDK-LICENSE.txt` carries the original adapter's MIT notice.
- Newlib: `NEWLIB-LICENSE.txt` is the selected SDK libc notice collection.
  Individual file notices remain applicable; the collection is not a single
  replacement licence for every library source.
- GCC runtime support: `GCC-GPL-3.0.txt` and `GCC-RUNTIME-EXCEPTION.txt` retain
  the GNU GPLv3 text and runtime library exception from the maintained SDK's
  GCC source. These terms describe the selected application runtime, not the
  Classic C tool's separate GPLv2-or-later lineage.
- The repository's `pdpclib/LICENSE` accompanies the source-built C
  applications and retained PDPCLIB service interfaces.

The source-built OS and original C applications keep their own component and
file notices. Bundling does not relicense an upstream application or runtime.
No IBM software or private laboratory disk is included.
