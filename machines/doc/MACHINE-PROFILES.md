# Shared machine-profile direction

This is the shared-selector design recorded on 1 October 2026. For the
current distinction between machine, ABI, load mode and service selections,
start with [Profiles and interfaces](../../doc/PROFILES.md).

I want Mainframe Classic Tools and the Mainframe ELF SDK to
use the same names and hardware instruction ceilings for the same machine
profiles. A shared machine name does not imply a shared object format, ABI,
character model or runtime. Those are explicit tool-family contracts.

This table is aligned to Mainframe ELF SDK revision
`5bb30294a4d420c16cfbb141e08db25496e6a659`, its `toolchain/profiles/` records
and its then-current `tools/s370_check.c` / `tools/audit_s370.crexx`. Those
paths describe the pinned revision, not the SDK's later source layout. The
[SDK repository](https://github.com/adesutherland/mainframe-elf-sdk) owns
its maintained definitions. Reconcile changes against the pinned definitions
before enabling a Classic selector.

| Shared name | Address mode | Hardware instruction ceiling | Classic status |
| --- | --- | --- | --- |
| `vm370-4381-v1` | 24 | `s370-int1`, historical 4381, no Hercules-only extensions | Planned; CMS C-to-assembly-text build exists, no complete Classic chain |
| `cms20-esa31-v1` | 31 | `s370-int1-bsm`; `BSM` in the explicit CMS bridge | Planned; Classic entry, services and residence unqualified |
| `tso-zos24-v1` | 24 | `s370-int1`; native z/OS service glue separately audited | Planned; MVS C-to-assembly-text build does not qualify z/OS services |
| `tso-zos31-v1` | 31 | `s370-int1`; direct OS services live in native bridges | Planned; Classic entry and high residence unqualified |
| `tso-zos64-v1` | 64 | z900, software float, no vector extension | Unsupported by the imported 32-bit-pointer backend |
| `vmkernel` | 24 | `s370-int1`, freestanding; no SVC in C objects | Planned; component-specific ABI/services and helper closure required |

The SDK's superseded `vmce-cms-kernel-v1` is historical, not an additional
active target. The existing PDPCLIB `mvs38-s370-24` target remains a distinct
MVS 3.8 service profile; it must not be relabelled `tso-zos24-v1`.

`s370-int1` is the SDK's selected historical integer subset, rather than every
instruction available on System/370. The pinned early audit admits arithmetic,
loads/stores, logical and byte operations, shifts, branch-and-link operations
and their historical extended mnemonics. Floating-point instructions, newer
addressing/ISA extensions and optional emulator facilities are excluded.
SVC policy is profile- and code-role-specific; CMS bridge permission must not
permit direct SVC in unrelated C objects. `s370-int1-bsm` adds the bridge's BSM
permission without broadening compiler ISA selection.

The assembler's existing `s360` and `s370` profiles describe its currently
implemented architecture subset. They are not selectors for the shared
machine profiles. Before enabling one, we need a common instruction contract,
compiler output audit, assembler checks after macro expansion and an independent
final-object check. No named Classic profile is currently qualified or enabled.

Classic C presently uses the inherited PDPCLIB macro ABI with 32-bit C
pointers and EBCDIC character values. The ELF SDK historical profiles use
`s390-ilp32-r1-v1`, ELF objects and their recorded character/runtime contracts;
the z900 profile uses LP64. Linking between them requires an explicit ABI
adapter and cannot follow from sharing a machine-profile name.

The [Classic roadmap](../../compiler/doc/BACKLOG.md) owns implementation
decisions and acceptance gates. Assembler-language coverage and CPU instruction
availability are separate decisions.

The [repaired PDIO1 OS](../../pdos/README.md) is an additional kernel workload
on standard z/Architecture, with AMODE31/RMODE24 kernel metadata and native
64-bit application context support. Its privileged support instructions and
hexadecimal-floating runtime require an explicit kernel code-role contract;
the SDK's historical integer application profiles do not grant that coverage.
No new kernel selector is enabled by importing the source. The
[OS build plan](../../pdos/doc/development/BUILD-CONTRACT.md) owns that contract and its gates.
