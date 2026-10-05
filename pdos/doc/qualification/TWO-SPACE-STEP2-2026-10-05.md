# Two-space Step 2: diskless machine result, 5 October 2026

We ran a disposable Hercules machine proof for the proposed K/U address-space
contract. This result is **not** a booted z/PDOS kernel or a CMS/TSO
application qualification. The source started from `develop` commit
`faea78b9583d354def64f1fc63b772c0f2b5f581`, with the PoC edits not yet
committed at run time.

The host was macOS arm64. Inputs were GNU Binutils assembler and linker
2.47.20260726, both selecting `-march=z900`, and Hercules 4.9.1.0-SDL with
`ARCHLVL ESAME`, one model-2064 CPU, 16 MiB real storage, no disk and no
shared guest. Binary SHA-256 values recorded in the generated receipt were:
assembler `9944bba7123ccd29f19b44835ebb05fa47347ea6eb4c98a96b40d3a6ec714050`,
linker `a2f91253feb7f046b4d8a8b6e67d455c8a7707ab3bf8452cc906c0bdbc687320`,
Hercules `080616bd946278ecf08bd7715663592a9bb5bc0619632bd7f2b0c0dba6f8ff77`,
and linked ELF `780ca31206db396089aaf7646b52037667adf5e5fbb3684f593127d8a0879f65`.
The generated `build/pdos/two-space-poc8/run/receipt.json` pins the exact
source hashes, command arguments, mappings, checks and observed PSWs.

The K ASCE was `0x100007`, the U ASCE `0x110007`. Independent table pools
used 36,864 and 57,344 real bytes. K mapped its DAT-off entry page at
virtual `0x1000`; U did not map it. U mapped only `0x20000` and `0x21000`
below 16 MiB, giving **zero K pages** in U's low virtual region. At
`0x21000`, U read `aaaabbbbccccdddd` and K read `1111222233334444`
on every SVC. The four captured application PSWs were AMODE24 at
`0x2001a`, AMODE31 at `0x2001e`, then AMODE64 at `0x110000002` and
`0x110000004`, all with DAT on, problem state and key 8. All 16 GPRs
retained their expected full-width values across each return. After the
fourth return, an attempted U read at `0x110000008` of K's `0x1000` entry
page raised the expected program interruption code `0x0011` and recorded the unmapped page
address. The bounded host judge passed every check; Hercules exited 0.

Reproduce from the repository root with a fresh `build/` output directory:

```sh
crexx -nokeep pdos/scripts/two-space.crexx --args build/pdos/two-space-new \
  /absolute/path/to/s390-linux-gnu-as \
  /absolute/path/to/s390-linux-gnu-ld \
  /absolute/path/to/hercules
```

The test deliberately uses GNU ELF64 and direct `loadcore` to isolate the
machine question. Classic Assembler's separately checked z900 encoder now
supports `LCTLG` and `STCTG`, but this fixture does not assemble or link a
native z/PDOS module with Classic tools. It does not test C31 service calls,
program loading, nested applications, channel I/O, all interruption classes,
SMP or a fresh IPL. Those remain gates in [PD-003](../BACKLOG.md#pd-003-two-space-supervisor-and-shared-application-memory).
