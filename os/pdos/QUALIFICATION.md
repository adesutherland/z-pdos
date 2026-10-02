# z/PDOS 0.1 qualification

2 October 2026. **The Classic-built OS boots and runs the unchanged released
cREXX TSO31, TSO64 ANY and TSO64 HIGH packages.** Their actual compiler chains,
interactive checks, diagnostics and stopped-disk readback pass. TSO24 was
tested but is not qualified: its AMODE24/RMODE24 directory is rejected before
dispatch. A genuine low-residence application path is in [the roadmap](ROADMAP.md).

The selected source is committed as
[`f9b7b1f`](https://github.com/adesutherland/z-pdos/commit/f9b7b1f2ea89fd492fef205db662bbdfa4164585).
[qualification-0.1.json](qualification-0.1.json) retains the exact source,
tool, base-image, native-input and output identities and successful results.
It contains no application sources, executables or disk images.

## Producer and machine

All 17 C units and six support modules were built afresh on Apple Silicon
macOS 26.6.2 with Mainframe Classic C 3.4.6 (MVS variant), the independent
Classic Assembler and Classic Linker. No native objects or as370 were used.
The compiler flags are `-Os -DZARCH -U__CMS__ -D__MVS__ -D__PDOS390__
-S -I. -I../pdpclib -DUSE_MEMMGR`. The selected `pdos-zarch` kernel profile
uses C32 in AMODE31/RMODE24, with standard z/Architecture support under
the z900 instruction ceiling. PCOMM is AMODE31/RMODE ANY.

Normal and sanitizer builds produce byte-identical 23 objects, three RDW
load modules and six independently linked flat images. The actual OS loader
reconstructs the modules at bases zero and 2 MiB, checking entry, declared
mode, relocations and padding, and rejecting malformed controls. Source
recovery reproduces all 81 selected files. Repaired-function checks,
complete IPL/dataset/compression readback and five media corruption controls
pass. Focused PDPCLIB/interface suites pass 11/11 normally and with sanitizers;
restoring the old SVC99 list format fails both STDIO controls. Unchanged
assembler core evidence is reused from the earlier complete build checkpoint.

The managed Lab guest uses Hercules 4.9.1.0-SDL, standard z/Architecture,
one CPU, 4,096 MiB real storage, a 100-cylinder 3390 at 01B9 and an IBM1047
3278-2 console at 0009. Guest operation followed the existing Lab operator
guide and lease procedure. This establishes the selected emulator/guest
route; it is not a physical-hardware or complete z/OS service qualification.

| Base program | Bytes | SHA-256 |
| --- | ---: | --- |
| PLOAD.SYS | 129,440 | `1d56baf898cf3aaa496e74efd90d597e626bd45be1f7c1eee941aa78f5484c9b` |
| PDOS.SYS | 181,320 | `94ddd6d188716c3d69b1b6462662f6862e3035711593be8ce1f9f409d560da1e` |
| COMMAND.EXE | 105,408 | `8afd1c8ed07f095b50cf8671d8713e6ada37df2abbb09f70a5fef724536b40cd` |

## Unchanged application qualification

Inputs are the already-built cREXX `v1.0.0-beta.3` packages from source commit
`ae1607b8e145174422cee7f3e73fbcc37a65226c`. Package SHA256SUMS passed.
XMIT transport framing alone was removed; native load bytes and supplied
application inputs were unchanged. Temporary command aliases used the Lab's
existing `RXC64`/`RXA64`/`RXV64` batch recipe. Their directory attributes,
rather than the aliases, establish the tested modes.

| Released package | Native directory / placement | Runtime heap | Supplied and fresh VM | Actual input |
| --- | --- | ---: | --- | --- |
| TSO31 | AMODE31/RMODE ANY, above 16 MiB | 64 MiB | 9 PASS / 0 FAIL / 2 SKIP, RC 0 | 10 PASS / 0 FAIL / 1 SKIP, RC 0 |
| TSO64 ANY | AMODE64/RMODE ANY, below 2 GiB | 128 MiB | 9 PASS / 0 FAIL / 2 SKIP, RC 0 | 10 PASS / 0 FAIL / 1 SKIP, RC 0 |
| TSO64 HIGH | AMODE64/RMODE ANY launchers; AMODE64/RMODE64 bodies loaded at `0x110000000` | 128 MiB | 9 PASS / 0 FAIL / 2 SKIP, RC 0 | 10 PASS / 0 FAIL / 1 SKIP, RC 0 |

Each profile passed the version check, native RXC compilation, RXAS assembly
and execution of the fresh RXBIN. The interactive run supplied all four
requested lines, including the empty line, preserved spaces and accented
character. Each of the three help commands returned 0; expected compiler,
assembler and VM error cases returned 2, -1 and -1. HIGH LOAD/DELETE completed
for each invocation. The append check remains skipped because the frozen
native runtime supports read/write modes only; this is not an append claim.

After normal guest shutdown, readback checked every staged input, kernel,
native stream padding and batch delimiter: 16 inputs for TSO31, 16 for ANY
and 19 for HIGH. All five output fixtures passed for each profile:

- The generated assembly has all 3,108 records and 138,311 exported UTF-8/LF
  bytes, matching the frozen host reference. Its SHA-256 is
  `935083d29f0339856b838748d14ffd7b39fee70d077d5e74926689bc2dd64fee`.
- The fresh RXBIN has valid VB framing and passed the actual guest VM run.
  Its different module name does not imply byte identity with supplied IOQUAL.
- TEXT contains `Native café! [] ^`, an empty record and `end`.
- BYTES contains exactly the ordered 256 bytes `00` through `FF`; SHA-256
  `40aff2e9d2d8922e47afd4648e6967497158785fbd1da870e7110266bf944880`.
- EMPTY is EOF-only with zero data records, and MISSING remains absent.

A bounded HIGH write-CKD trace captured three full-track failures with status
0E00 and zero residual. Each retry used the same count, buffer address and
shown data prefix on the next head and returned 0C00. Complete stopped
assembly readback confirms that no output block was lost. Device tracing
was then disabled.

The TSO24 RXVM's actual AMODE24/RMODE24 module was delivered unchanged and
tested. The loader reports `unsupported direct-load mode: AMODE24 RMODE24`;
PDPCLIB returns its documented ATTACH failure `0x04806004`. It is not executed
in a different mode and does not inherit its earlier TSO qualification.

## Accepted working image

The managed current version is `zpdos-0.1`. It was booted afresh from the
checked base image after qualification and returned a usable PCOMM prompt.
Its disk contains PLOAD.SYS, PDOS.SYS, CONFIG.SYS and COMMAND.EXE only.
Temporary cREXX installations, source/export files and qualification media
are removed; canonical released inputs remain at their existing owner.
One managed OS image and its standing emulator remain. Git holds the source
and this successful qualification record, not duplicate application code.
