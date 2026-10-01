# Focused PDPCLIB host checks

These checks compile the maintained `stdio.c`, `string.h` and LP64 limits.
They exercise the canonical runtime fixes and C changes from Mainframe Lab's
`patches/pdos390/0002-standard-s390-native-build.patch`. They do not build a
complete PDPCLIB target library.

Run on a GCC-compatible host compiler:

```sh
cmake -S pdpclib -B /tmp/pdpclib-host-qa -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/pdpclib-host-qa
ctest --test-dir /tmp/pdpclib-host-qa --output-on-failure
```

For host address and undefined-behaviour checks, configure a separate build
with `-DMF_SANITIZERS=ON`. The repository's root CMake build may also include
this directory.

The two stdio executables use `__MVS__` and host builtin varargs. One additionally
defines `__PDOS390__`; the other is an MVS control. Both call the actual source,
with instrumented substitutes for native services. The checks cover:

- Supported text and binary read/write modes and unsupported append/update
  modes, including both binary update spellings.
- Rejection before any native open call, failure propagation for a controlled
  allocation failure and native open failure, and balanced fixture allocations.
- Ordinary opens with a stale allocated FILE update flag, initialized before
  native open; returned update state for supported MVS binary update modes.
- MVS `w+b` and `wb+` retain the native write-only fallback, without a DCB
  query, initial read or update seek through that write handle.
- A preexisting update flag through `osfopen`: PDOS/390 must omit DCB queries,
  initial reads and update seeking. The MVS control must retain them.
- A synthetic update stream through `fseek`: PDOS/390 must avoid `__apoint`;
  the MVS control must call it with the independently expected TTR `0x200`
  for offset 8, record length 8 and four records per track.

The two string checks verify the real header's macro selection and exercise
copy/compare results. PDOS/390 must use the function declarations; the GCC
control must keep the existing explicit builtin macros. This excludes those
header macros for PDOS/390; it does not prevent a target compiler from applying
its own optimizations.

The native-service substitutes never access a guest or perform dataset
allocation. Their handles and records are controlled fixtures. The synthetic
FILE buffer includes PDPCLIB's four-byte hidden prefix and suffix, and its
pushback state is explicitly empty.

`pdpqa_malloc` uses host `calloc`, then seeds FILE allocations with
`update = 1`. The canonical upstream `checkMode` initializes this field before
`osfopen`, so ordinary opens must neither retain the stale flag nor enter the
native update block. The MVS write-only fallback is also excluded from that
block. A separate seeded-open check bypasses `checkMode` to exercise the
existing MVS/PDOS390 service guard directly. This tests the selected source
paths; it does not qualify a target allocator or complete guest I/O.

On LP64 hosts the limits check compares the actual header's `LONG_MAX` and
`ULONG_MAX` against independently computed host-width maxima. It selects the
header's `__LONG64__` branch, which must not truncate `ULONG_MAX` to 16 bits.

The host wrapper namespaces standard stdio entry points. Six functions whose
names are undefined inside upstream `stdio.c` use GCC-compatible symbol labels
in the wrapper. The retained harness and stubs are C90; the wrapper's symbol
labels are a compiler interface for this host fixture. Modern host warnings are
suppressed only for the translation unit that includes inherited `stdio.c`.
The harness and stubs retain strict C90 warnings. This is source-path QA on host
widths, ASCII text, host allocation and varargs. It establishes no mainframe ABI,
EBCDIC execution, assembler services, guest libc, complete linking or OS build.

## Failure controls

On 30 September 2026, each of these edits in a disposable copy made its
corresponding PDOS/390 check fail while the maintained source passed:

- Remove `!defined(__PDOS390__)` from the string builtin guard: the string test
  fails compilation because the macros are present.
- Restore the MVS binary-update branches for PDOS/390: rejection tests fail
  because `fopen` returns a stream.
- Restore the post-open update block for PDOS/390: the seeded-open test detects
  an unwanted DCB query.
- Restore the MVS update-seek block for PDOS/390: the seek test detects an
  unwanted `__apoint` call.

These disposable changes are failure controls, not source patches to retain or
apply. AppleClang 21.0.0.21000101 on macOS passed all four normal and sanitized
checks. Existing guest qualification is unchanged and belongs to its original
record in Mainframe Lab.

On 1 October, the consolidated runtime passed five focused normal and
ASAN/UBSAN checks, plus root profile and source-owned macro checks (7/7).
Removing early update initialization failed the stale-allocation control;
the MVS trace-open path also failed UBSAN with halt-on-error enabled. Removing
the MVS write-handle guard failed the no-query assertion. Restoring the old
unsigned-short cast on `ULONG_MAX` made the LP64 check return 1. These controls
ran in disposable build copies; the maintained source remained unchanged.
