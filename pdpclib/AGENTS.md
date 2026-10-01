# PDPCLIB maintenance

Read the root guide, README.md, SOURCES.md and CHANGES.md. This directory owns
our maintained PDPCLIB source and target variants. Mainframe Lab and SDK copies
are references or consumers, not places to make new library fixes.

- Preserve Paul Edwards's public-domain dedication, fallback permission and
  all contributor notices. New project changes follow the root MIT grant;
  imported source is not relicensed. Do not import IBM service/mapping macro
  implementations or native expansions under the library's licence.
- Change maintained source directly for a shared fix. Record the original Lab
  delta and its qualification limits in CHANGES.md. Retained fixes are audit
  inputs, not patches to reapply to already corrected source.
- Keep deliberate omissions and OS-specific adapters in named profiles.
  In particular, the TSO31 lean parser omission is not the default library and
  does not repair a linker relocation error.
- The first complete target is MVS 3.8 / real System/370 / AMODE 24 and RMODE 24.
  Later z/OS, PDOS/390 and z/Architecture configurations have distinct profiles.
  Never activate SAM31 or later IBM services for the real S/370 target.
- Run focused source/object/link tests for changed paths, with independent
  expected values and failure controls. Retain unaffected guest evidence.
  Host source checks do not qualify complete assembly, guest libc or native
  assembler hosting.
- Scripts retained for development and preparation use cREXX. Private assets
  and generated objects stay outside source control. Read-only Lab/SDK input
  must not be changed during consolidation.

- OS and application builds consume this one maintained library. Merge fixes
  here, including relevant upstream changes from a different source lineage.
  Keep justified configuration/service differences in explicit profiles and
  earlier whole-source versions in Git history. A merged candidate does not
  inherit a previous binary or whole-source guest qualification.
