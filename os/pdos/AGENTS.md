# Repaired z/PDOS source

Read README.md, SOURCES.md, CHECKPOINT.md and BUILD-PLAN.md before changes.
The OS files preserve the selected PDIO1 source used by the accepted guest.
The kernel C implementation is 31-bit and low-resident; its support code
uses standard z/Architecture and runs qualified 64-bit applications.

- Preserve upstream public-domain notices and the original patch identities.
  Original project automation, repairs and tests follow the root MIT grant.
- OS source is maintained here; the runtime is maintained only in `pdpclib/`.
  Select the `pdos-zarch` configuration without copying a second runtime into
  source control. The exact earlier qualified input is preserved by commit
  `d62b109ed986bd455732484173ff4ebe0533045a`, not a second maintained tree.
- Source hashes identify the current input. A new source change is a new
  candidate and needs its own source/patch record and affected qualification.
- Use build.crexx, recover.crexx and inventory.crexx from the repository root.
  Native listings, objects, media, private accounts and raw guest receipts stay
  outside Git. Do not import IBM macro implementations or native expansions.
- as370 is excluded. Compiler output and source macros must use the independent
  assembler. Do not hide missing directives or services with no-op expansion.
- Compile-to-text, independent object checks, linking, disk construction, boot
  and cREXX qualification are separate gates. Keep generous 31/64-bit application
  heaps and the qualified high mappings; historical 24-bit limits are separate.
- Shared guest operation belongs to Mainframe Lab and requires its operator
  guide and leases. Source checks do not authorise changing a running image.
