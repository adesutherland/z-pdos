# Compiler source and reconciliation

1 October 2026. This imports the compiler subtree, not the upstream
assembler, linker, runtime, binaries or mail archive.

| Input | Exact identity and use |
| --- | --- |
| Recorded Mainframe Lab baseline | `mvslovers/cc370` revision `fcafc6f570e29209abdc29f151f8d896129f222e`, recorded in Lab's 28 September PDOS bootstrap evidence |
| Current upstream checked on 1 October | [cc370](https://github.com/mvslovers/cc370/tree/ece26349fc1096804e82e5618bb8255054035247), revision `ece26349fc1096804e82e5618bb8255054035247`; compiler subtree `cc370/`, Git tree `6f54ea46dc2c9f92a0b32975485855d8b60cf3eb` |
| Mike Beer's compiler patch series | 22 September v6, 14 numbered patches; private archive SHA-256 `a2e1277017f72fc12e2de555c75e593b2f14c7d205d1cd0c4e6d6315e8c118b0` |
| Mainframe Lab host repair | Darwin ARM64 instruction-generator calling convention, adapted from the maintained GCCMVS macOS repair; generated config headers and the old Unix compatibility object are not needed in the configured cc370 build |

`src/` is the maintained compiler subtree. GCC copyright notices and the
upstream GPL licence are retained. Jan Stein, Dave Pitts, Paul Edwards and the
other contributors retain their source notices; the cc370 continuation and
Mike Beer's repairs are attributed here. Original changes to GPL compiler
files remain under their inherited terms.

## Mike's series

| Patch | Reconciliation |
| --- | --- |
| 0001 | Applied: restore the CMS target header and target recognition |
| 0002 | Applied: correct generated instruction byte accounting |
| 0003 | Applied: re-establish the code base with `BALR` |
| 0004 | Applied: reload the code base at computed-goto targets |
| 0005 | Applied: shorten page, pool and table labels to eight characters |
| 0006 | Applied with integration repairs: long external names receive aliases |
| 0007 | Superseded by upstream `72bac61`: DI multiply/divide/modulo retain real helper calls through `UNSPEC` |
| 0008 | Applied: honor `#pragma map` on the default target path |
| 0009 | Already present upstream: logical DI left shift uses `SLDL` |
| 0010 | Applied: clip assembler comments to 71 columns |
| 0011 | Excluded: as370 implementation repair; that assembler is not part of this product |
| 0012 | Adapted: indirect jumps always use a nonzero address register; retain upstream's newer direct-jump page guard |
| 0013 | Applied: keep private static assembler names consistent during callgraph renaming |
| 0014 | Applied: evaluate nontrivial call arguments before publishing the shared outgoing argument slots at optimization |

The v6 series supersedes Mike's earlier seven-patch attachment. The attachment
was recovered successfully; private correspondence, CMS/native packages and
reference macro assets are not in Git.

## Later source availability

The 24 September build-86 report says the toolchain has 18 cc370 patches.
Both newer native packages were downloaded successfully and their ZIP contents
inspected: five CMS card decks and a memo, with no cc370 source or patch series.
Build 86 has archive SHA-256
`7772d5be9aa8f2067ec2235273fad577d492d1fd97b0e29c201356a427cb0c3f`;
build 92 has
`728e2934783985b76c298dc99c8612c4465f70f9374c653106f68c2ef1cc9358`.
They remain private local inputs, not imported product material. The later
18-patch source series was not found in the relevant mail attachments or local
patch inputs examined. Its contents and relation to current upstream cannot
be inferred from the patch count. The current direction is manual consolidation
of relevant available fixes, with reproduction from descriptions where feasible.
The [manual reconciliation](doc/qualification/MANUAL-RECONCILIATION.md) records that work. A full
newer source bundle would allow a further comparison; its absence does not
prevent the bounded available-source checkpoint.

## Integration repairs

- Use a defined 32-bit DJB2 calculation for long-name aliases on both 32- and
  64-bit hosts. This changes the new alias scheme from Mike's host-width-dependent
  implementation; rebuild all cooperating units and preserve explicit mappings
  for existing libraries. Hashing is not proof against cross-unit collisions.
- Preserve the existing `@@UDIVDI`, `@@UMODDI`, `@@UCMPDI`, `@@FIXDFD` and
  `@@FIXSFD` runtime-helper symbols when long-name hashing is enabled.
- Carry the Darwin ARM64 fixed instruction-generator function-pointer type
  through the `expr.c` helper declarations and definitions as well as `recog.h`.
- Identify the local compiler as Mainframe Classic C while retaining its
  GCC 3.4.6 version and cc370 lineage.
- Update inherited regression expectations for hashed long names and the new
  page labels, retaining collision diagnostics for overlong explicit asm names.
- Complete the described assembler-comment repair for initialized and common
  variables, and bound the formatter before clipping. The previous helper
  crashed `cc1` with a valid 700-character function identifier; the common-variable
  path still emitted a 101-column comment. Both defects are reproduced and fixed.

- Emit raw 32-bit data as representable signed F values, preserving automatic
  alignment and exact bits on wider hosts. PDOS MATH reproduced the failure
  with 2827508273; the assembler must reject that out-of-range signed nominal.
  IBM [fixed-point constants](https://www.ibm.com/docs/en/hla-and-tf/1.6.0?topic=value-fixed-point-constantsf-h)
  documents signed and explicitly unsigned ranges. No target ISA/ABI changes.

- Select exact XL4/XL8 floating literal emission from GCC's existing i370 real
  encoder. This preserves target rounding and word order and avoids a second
  decimal floating converter in the independent assembler; see doc/BACKLOG.md.

## Maintained source and frozen baseline

`src/` contains the complete compiler implementation and original host launcher.
The inherited configure/Makefile build uses that tree directly; no patch or
private attachment is needed. Normal source edits and Git commits supersede
consolidated recovery patches. Earlier patch records remain in repository
history at fd7df814fff5b181fb18ab30de31872c77404d2c.

`archive/upstream-ece26349.tar.gz` freezes the exact public cc370 compiler
subtree and COPYING at the revision named above. Its checksum is in
archive/SHA256SUMS. It retains the upstream notices, GPL text and library/header
terms and exceptions; the wider upstream toolchain description is separately
labelled in the archive. Archives are excluded from normal builds and tests.
The imported compiler lineage includes Jan Stein, Dave Pitts, Linas Vepstas,
Paul Edwards and other source contributors, followed by the cc370 continuation
and Mike Beer's attributable repairs. [LICENSE](LICENSE) scopes original work.

## Numeric escape preservation — 2 October 2026

The retained newline fix maps host LF to EBCDIC NEL (0x15). Its inverse
already mapped target 0x25 to host NEL, but the forward table still mapped
host NEL to 0x15. Consequently both hex `\x25` and octal `\045` string
escapes emitted 0x15. This defect survived the earlier newline tests.
The forward NEL entry now maps to 0x25, completing the existing newline/NEL
exchange and preserving all 256 numeric byte values. Printable CP037 mappings
and the selected newline value remain unchanged. Comments now describe this
contract accurately. The repair was already present before this reorganisation; its exact earlier source and manifests remain in Git history.
