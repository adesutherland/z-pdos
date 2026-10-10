# Application command environments: 0.2.2 development candidate

10 October 2026. The shared U/C dispatcher and bounded CMS31/TSO31 native
interfaces are implemented and qualified on model 5, model 2 and line primary.
This is a qualified development checkpoint, not a release or managed adoption.
The [architecture guide](../architecture/COMMAND-ENVIRONMENTS.md) owns the
current design and explicit compatibility limits.

## Review and frozen inputs

The complete source review preceded guest dispatch. It covered native
parameter fencing, LOAD/DELETE, entry mode, complete context preservation,
private stack/argument ownership, nested return/fault cleanup, shared shell
policy, input cursor preservation and build selection. Resolved findings
included the AMODE31 LOAD entry marker for BASSM, FINDENVB's R0 output,
separate child RC/service status, defined reason outputs, rejection of a
nonzero internal pointer high half and avoiding an unverified ABEND code.

The first candidate's independent relocated-artifact preflight found that
Classic Assembler aligns V constants to a fullword, inserting two bytes
between the descriptor signature and pointer table. K expected offset 10
instead of 12. F1 was revoked before any guest start or IPL. The repair uses
explicit assembler alignment and shared C offsets, with a retained C89
sanitizer check of the actual relocated table and native veneer bytes.

Repeat the focused checks from the product root, with fresh output paths:

```sh
clang -std=c89 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined \
  -I pdos/src pdos/tests/two-space/command_router.c \
  pdos/src/twospace_command_router.c -o ABS_ROUTER_CHECK
ABS_ROUTER_CHECK
clang -std=c89 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined \
  -I pdos/src pdos/src/twospace_tso.c \
  pdos/tests/two-space/command_descriptor.c -o ABS_DESCRIPTOR_CHECK
ABS_DESCRIPTOR_CHECK ABS_FROZEN_U_COMMAND_STAGE
```

Final review also inspected the emitted fault fixture. GCC transformed its
null write into a store followed by the runtime abort hook. The corrected
fixture uses a volatile write to explicitly unmapped U page 0x2000; its
emitted code contains that store and no abort call. Only that native package
was rebuilt. The compatibility pages are at 0 and 0x10000.

Freeze `command-environments-f4` is recorded in
`build/pdos/command-environment-f4/freeze.json`, SHA-256
`6b26e9ac72c0951130cc5736b2514c2671fdafc87be5c222d38d1a4b1b7a5a84`.
It pins the complete K/U build inputs, new SDK clients, unchanged native
clients, bootstrap and installer, review result and bounded guest matrix.

| Frozen output | SHA-256 |
| --- | --- |
| `env-kernel6/normal-final/image.core` | `6a3b45247ab767c6d4106ccd5f5d97c54044781dc99d50e054d8c430266e0356` |
| `env-command5/u-command.bin` | `1681ed4dbecbe75fef6f691c6050110fbf3f82027eb45be10d5b722966d8521a` |
| Unchanged CMS31 ENVFIX MODULE | `e9e37d92540a304abc21e2d2ad80e2a794606f30b7776d39515c0810a59237fb` |
| Unchanged TSO31 ENV31 XMIT | `bfa29910968fdfc48b8e7f3e8edf9fbd3ce67e2544e8f73b14ea2c3dacb7e007` |
| New CMS31 PDOS client MODULE | `c7aa4efc5ea517557700c5cb8230dc94f74486bc46dd5dd81a5e97db52511b99` |
| New TSO31 PDENV31 XMIT | `55fea800e1966878b8da4a8ec6bd509e1df4fed1943445eece515c2f67691911` |
| TSO31 RC7 child XMIT | `80a7b0942db059ed7622cb22101c9aac58687ab2c67fdd8d1bc7720a3d144647` |
| TSO31 deliberate-fault child XMIT | `3b3729b13d66289de312e63052e2e035c64d40ff57ed29ddf206be92c8a2aee2` |

The new SDK packages are under `build/command-environment/pdos2` in
`mainframe-cross-sdk`; original native clients are under `freeze5`. Their
outputs were checksum verified before dispatch. The original installed
0.1.0 SDK remains unchanged. The optional recipe's output-manifest addition
followed the build; it does not change the compiled adapters or fixtures.
The corrected fault package is under `build/command-environment/pdos-fault2`.

## Host evidence

The C89 command parser passes Clang warnings-as-errors and Address/Undefined
Behavior sanitizers. It checks exact command text, double spaces, case and
quotes, CMS file IDs, quoted TSO names, missing/unknown commands, invalid
length/NUL/newline controls, shell-state fallbacks and a dated IBM TOD vector.
The kernel recipe passes its required DAT, shared-U placement/invocation,
whole-span gate, storage ownership, CMS envelope/cursor and normal boot
checks. Its C service bank uses 469,390 bytes of 2 MiB, leaving 1,627,762 bytes.
PCOMM compiles,
assembles and links through Classic C/Assembler/Linker and passes the native
envelope packer: 114 records, 133,318 raw bytes, eight F18452 blocks.

Both CMS and TSO SDK adapter sanitizer controls pass. All four new native
packages pass the relevant encoded ELF and MODULE/XMIT preparation checks.
The earlier real CMS20 18-check and z/OS 1.5 19-check evidence is reused only
for the exact unchanged ENVFIX/ENV31 binaries. Neither real guest is rerun
for this PDOS campaign.

## Guest matrix and acceptance boundary

The operator owns the disposable normal-IPL candidate. The profile is one
ESAME CPU, 512 MiB real, a 16 MiB core, 3390 media and a capture-only monitor.
The producer input tree is the retained 0.2.1 release image; the current
frozen K/U version is 0.2.2. Configuration makes a separately hashed core
derivative. The managed guest image is preserved.

CMS ENVFIX runs first with `ENVINPUT DATA A`, followed by TSO ENV31 with
`LABA01.ENVCMD.INPUT`. The TSO client binary is unchanged; the path argument
uses its existing direct allocation route, rather than the real-z/OS run's
preallocated DD. CMS input retains the two ASCII records including LF; TSO
input uses two IBM1047 VB records without embedded LF.

The new CMS and TSO PDOS clients use that same input. They check shared
VERSION/ECHO, an absent command, a real TSO31 child returning RC7, contained
hardware fault, later command success, unbuffered second input read and a
128 KiB caller guard. Final combined replay covers model 5, model 2 and line
primary, shell/media regression, normal shutdown, stopped STORE/non-STORE
readback and actual process/socket/disk-handle closure.

All three F4 profiles pass the selected client paths. Application
registration/callbacks, complete
CMS/TSO command implementations, a real Rexx processor, Classic whole-library
clients and actual cREXX ADDRESS integration remain open. This campaign
qualifies the selected existing-environment C client paths and native PDOS
vocabulary; it does not close PD-026's source-script objective.

## First guest failure and reviewed repair

F2's first normal model-5 CMS31 fixture reached its header, then reported
`ENV FAIL input-open` and observed native RC16. PCOMM returned to its prompt;
the guest did not freeze. The later fixtures were held.
The unchanged SDK encodes explicit filemode A as C1 40. K's existing native
file lookup required C1 F1 and rejected it before media lookup. Native CMS
resolves A to A1. The repaired STATE service canonicalizes that default before
lookup and returns the resolved mode in its FST. The client bytes and A
arguments remain unchanged. This is a native file-service compatibility
repair exposed before command invocation, not a passing environment result.

The actor's failure cleanup attempted EXIT without first observing keyboard
readiness, then batched CPU stop and capture commands. Hercules rejected
savecore because the CPU had not yet stopped. No memory capture or orderly
guest shutdown is claimed for F2. The emulator/controllers exited and the
candidate disk hash was unchanged; the managed image was preserved.
The repaired actor requires a successful unlocked keyboard query and accepted
EXIT actions. Capture requires a fresh echoed PSW and complete `Manual Mode`
response from Hercules's actual `console.log`; elapsed time does not select
readiness. Thirteen focused controls pass, including a real cold Hercules
capture with no IPL, devices or DASD. Coordinator checks independently reject
a stale stopped response and hold failed or pending keyboard queries. The
frozen actor has SHA-256
`c28ec76336dc3af55a90edbe2fbe3c937436f5362130e3e0ad161f657f0dd524`.
F3 review checked all 413 inputs and eight outputs before dispatch. F2 remains
a failed candidate with an unqualified capture. Private F2 evidence is
under `/Users/adrian/MainframeLab/private/pdos-command-environment-20261010/f2-model5`.

## F3 replay findings

F3's model-5 replay reached every fixture's RC0 and PASS summary, all shell
regression commands, EXIT and K SHUTDOWN. Independent review nevertheless
found that the TSO client's second launch of ENVCHILD omitted the child's
output marker. The native return remained RC7. The command route had reused
the checked mapped image without refreshing its mutable executable/runtime
state, including newlib stdout after the prior exit. The repair calls the
existing validated `load_refresh` before launching a program, as PCOMM's
normal ATTACH route does. The binary fixtures and expected output markers
remain unchanged; RC alone is insufficient acceptance evidence.

F3's capture also failed. Hercules's PSW display reports `Disabled Wait`
before considering its CPU stop state, so `Manual Mode` is not a general
stop oracle. Source review of the pinned Hercules implementation confirms
that `ipending` reports its actual CPU run state; savecore checks that same
state. The actor repair requires a fresh echoed `ipending` and CP00 STOPPED
response. F3 has an orderly shutdown but no qualified memory capture, and
model 2/line were held. The next reviewed freeze must include both repairs.

F4 pins the repaired launch route and actor
`b7db0292b71bb421a68bffc4e32735d12c1b3f327e68b9f6e997eb77d571dcd3`.
Sixteen focused actor controls pass, including actual cold and disabled-wait
captures; independent pending-state and stale-response controls also pass.

An additional diagnostic profile run during K6 preparation exposed a stale
U DAT diagnostic alias and two table expectations from the former small
memory layout. The alias now matches the current U pool at 0x05300000; the
expectations use the named profile's pool addresses. The affected diagnostic
run passes all 106 checks, covering native context, file/storage ownership,
hardware I/O wake/late-completion paths and child-fault cleanup. Its I/O mask
negative control fails exactly its intended check while completing normally.
This repairs the diagnostic coverage rather than changing DAT allocation.
The final normal entry was separately reassembled from `twospace_normal.S`;
the checked C bank and DAT builder were reused. F4 verifies all 414 final
layered inputs and eight outputs. Combined terminal qualification passes.

Child-fault proof uses the typed invocation results and the fixed unmapped
write fixture. Legacy diagnostic word 0x4f90 also holds PCOMM's input-FNV
receipt during normal IPL; it is not a zero-based fault counter in this
profile. Its raw value is not used as a qualification oracle.

## Final combined qualification

The operator ran the original commands and unchanged binaries against fresh
F4 images, in the approved order. Coordinator independently reviewed every
stopped transcript, result receipt and complete 512 MiB capture. Each run
passes 18 original CMS checks, 19 original TSO checks and 13 checks for each
new PDOS client: 63 application checks per profile, 189 total.

| Primary terminal | Geometry | Application checks | Console/capture checks |
| --- | --- | --- | --- |
| Model 5 colour Workbench | 27 x 132 | 63 PASS | 20 PASS |
| Model 2 colour Workbench | 24 x 80 | 63 PASS | 20 PASS |
| Line / LLM route | 0 x 0 | 63 PASS | 16 PASS |

The original CMS argument remains `A` in every run. Each PDOS client produces
the real child output exactly once in its own section and exposes RC7 as a
completed command. Each deliberate hardware fault has a separate typed
OS12/reason1 result with invalid command RC. CMS exposes native RC-4; TSO
exposes facility12 with invalid command RC. The caller's later ECHO, native
input continuation, 128 KiB guard and close all pass. Shell media commands,
controlled missing-program status and subsequent commands also pass.

Every run reaches EXIT/K SHUTDOWN, then fresh CPU STOPPED before completed
savecore. Captures have no transcript gap, zero invocation depth and zero
read/screen owners. Stopped STORE payloads match their captured transcripts;
bytes outside STORE are unchanged. Peak observed real allocation is
194,129,920 bytes within the 512 MiB profile; final retained real allocation
is 35,659,776 bytes, including the checked image cache. This is an allocation
observation, not a performance benchmark or a claim that all cache storage
is freed on program exit.

| Profile | Complete stopped-core SHA-256 | Semantic transcript SHA-256 |
| --- | --- | --- |
| Model 5 | `042a1309e13161f794f7b04bf5a496f833c13420548cf5785f336d12d3265226` | `5cbbf983e8dac7e2f5d4d46c013fcf6ea67d39a838611d46f26110461bc3e488` |
| Model 2 | `8a7df2b6cdad8a8537e80a816dba7138477484e10fc55f8cb39fdee819a879db` | `ce180dc2374b62050a89e75da11c7159635dcf4b8ecffa7a98cc6d063088fa00` |
| Line | `0f4281424984452ba579a16c4abb04b2cd020cdb090b07602c0bc792b55e3daf` | `bd45a09e3905a2c05d21023a61c54e73067122960344099777cb9696c6cee6fd` |

Private evidence is under
`/Users/adrian/MainframeLab/private/pdos-command-environment-20261010`:
per-profile actor/operator/coordinator receipts, actual core-offset slices,
full semantic transcripts and stopped-media readbacks. Successful large
scratch captures are retired after independent complete hashing and slice
review. Failed F2/F3 diagnostics remain separate. Source publication was
authorized after qualification on 10 October 2026; this result is not a
release or managed-image adoption.

Final handback passes. Coordinator fetched fresh fleet status and verified
all six guests stopped with empty leases, all eight named test processes
gone, all fleet/test listeners closed and no candidate/managed disk handles.
Actual candidate hashes match their final receipts, and both managed PDOS
disk hashes are unchanged. The managed target remains `zpdos-0.2.0`.
All final frozen K/U/client and operator helper hashes were reread exactly.
`coordinator-final-validation.json` records that independent closure.
