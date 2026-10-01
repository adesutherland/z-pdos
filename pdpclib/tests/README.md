# Focused PDPCLIB host checks

These checks compile the maintained `stdio.c` and `string.h`. They exercise the
C changes consolidated from Mainframe Lab's
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

`pdpqa_malloc` deliberately uses host `calloc` to zero its allocations. In the
inherited MVS path, `fopen` allocates a FILE, `osfopen` reads its `update` flag
when opening ordinary modes, and `fopen3` initializes that flag afterwards.
The consolidated PDOS/390 guard excludes that post-open block.
This fixture controls that state; it does not prove initialization by the
target allocator or repair that separate inherited issue. The seeded-open
check supplies `update = 1` explicitly so the guarded code is exercised.

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
