# Bootstrap build and host services

The core uses C89/C90, eight-bit bytes and `unsigned long` with at least 32 bits.
Its ordinary C files can be compiled and linked without CMake, cREXX, a code
generator, POSIX, threads or a native 64-bit integer type. From the repository
root, a development host can build the desktop adapter directly:

```sh
mkdir -p build/direct
cc -std=c89 -pedantic-errors -Wall -Wextra -Werror -O2 \
  -Itools/classic-as/include \
  tools/classic-as/src/values.c tools/classic-as/src/reader.c \
  tools/classic-as/src/machine.c tools/classic-as/src/assemble.c \
  tools/classic-as/src/object.c tools/classic-as/src/main.c \
  -o build/direct/mf-classic-as
```

On a host without a shell or filesystem, compile the five core files and bind a
different driver. Mandatory services are a replayable statement source, aligned
bounded session storage, a sequential byte sink and structured diagnostics.
Record replay can reset a device, reopen input or read a supplied spool. No
core module assumes a filesystem hierarchy, seek, clock, locale or environment.

The desktop driver uses standard C stdio and a bounded allocator. Actual code,
data/BSS, stack and peak storage need measurement for each later native host.
Do not treat chosen desktop capacities as 24-bit host qualification. Wider
application hosts should retain generous capacity headroom.

For development and all retained unit/integration tests:

```sh
crexx tools/build.crexx --args test
crexx tools/build.crexx --args sanitize
```

The optional CMake configuration selects strict C90 and warnings as errors on
Clang/GCC. The sanitizer build adds address and undefined-behaviour checks. The
cREXX script only orchestrates development tools; it is not an assembler runtime
dependency. If `crexx` is available, CMake registers the additional desktop CLI
contract script; its process adapter uses shell quoting and CMake file helpers.
The C unit suites and independent positive deck checks remain available without
cREXX. Equivalent direct CMake invocation is:

```sh
cmake -S . -B build/classic-as
cmake --build build/classic-as
ctest --test-dir build/classic-as --output-on-failure
```

Tests use original independent instruction vectors and object-record assertions,
with invalid ranges, malformed input, replay changes, capacity exhaustion and
sink failures. Encoding/decoding round trips cannot replace those controls.
This host suite does not establish native z/PDOS hosting or complete source-to-IPL.

The optional traditional provider is a separate C89 library, included by the
development CMake build and selected only by `--macros`. Use
`-DMF_TRADITIONAL_MACROS=OFF` for the small CMake configuration; the direct recipe
above already excludes it. Its public header is `mf_classic_macro.h`. A direct
capable-host build can add `macro_cards.c` and `macro_provider.c` and define
`MF_WITH_TRADITIONAL_MACROS` for the desktop driver. No cREXX runtime, resolver,
heap API or external macro library becomes a core dependency.
