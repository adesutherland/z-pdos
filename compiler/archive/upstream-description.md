# cc370

A **host-native cross-toolchain for MVS 3.8j** (TK4-, TK5, MVS/CE): compile,
assemble, link and package MVS programs entirely on the host (macOS / Linux), so
the only thing that touches the mainframe is the finished load module.

```sh
cc370 hello.c -o hello                       # -> hello : an MVS load module (LMOD)
cc370 hello.c -o hello -flinker-output=xmit  # -> hello + hello.xmit : compile+assemble+link+package
```

`-o` always writes the load-module member; `-flinker-output=xmit` *additionally*
emits `hello.xmit` (and `=iebcopy` an `hello.iebcopy` unloaded-PDS image). That
`hello.xmit` ships to MVS and is installed into a load library with one `RECV370`
step — no IFOX00, no IEWL, no IEBCOPY, no JCL round-trip.

## The tools

| Tool | Role | Built from |
|------|------|------------|
| **cc370** | the driver + C → i370 HLASM compiler (`cc370/gcc/`) | a GCC 3.4.6 fork for the `i370-ibm-mvspdp` target (`TARGET_PDPMAC`) |
| **as370** | `.s`/`.asm` → OS/360 object deck — byte-identical to IBM's IFOX00 | `as370/src/as370.c` |
| **ld370** | object decks → MVS load module (replaces IEWL) + automatic library call + `-iebcopy`/`-xmit` host→MVS transport | `ld370/src/ld370.c` |
| **ar370** | object decks → `.a` archive with an ESD symbol index | `ar370/src/ar370.c` |
| **file370** | read-only inspector — identify + analyze any of the formats above (`file`/`objdump` for the toolchain) | `file370/src/file370.c` |
| **xmit370** | host directory → TSO TRANSMIT file holding a source PDS (samplib, JCL, macros); also lists + extracts XMIT files | `xmit370/src/xmit370.c` |

`cc370` is the single front-end; `as370`/`ld370`/`ar370` are ordinary binaries it
drives (and which you can also run standalone); `file370` is a standalone
inspector that reads the formats the others write, and `xmit370` packs the
non-code half of a distribution — sample JCL, parameter members, macros — into
the same TSO TRANSMIT envelope `ld370` ships load modules in. The C library it links against is
**[libc370](https://github.com/mvslovers/libc370)**, installed into the cc370
sysroot — see *Sysroot* below.

## Build & install

GCC 3.4.6 is old K&R-ish C; a modern clang/gcc host must be told not to error on
it. The top-level `Makefile` wraps the whole thing:

```sh
make            # build the whole toolchain: cc370 + as370/ld370/ar370 + man pages
make tools      # only the three tools (fast; skips the slow compiler build)
make compiler   # just the driver (cc370) + the compiler proper (cc1), out-of-tree in build/
make install    # build (if needed) + install everything under $(PREFIX) (default ~/.local)
```

Knobs: `PREFIX` (default `~/.local`), `TRIPLE` (default `cc370`),
`VERSION` (default `1.0.0`). After `make install` you get:

```
<prefix>/bin/cc370                          the driver
<prefix>/bin/{as370,ld370,ar370}            symlinks -> ../cc370/bin/* (PATH access)
<prefix>/libexec/cc370/1.0.0/cc1            the compiler proper (driver-private)
<prefix>/libexec/cc370/1.0.0/{as,ld,ar}     symlinks beside cc1; the driver's tooldir
<prefix>/cc370/bin/{as370,ld370,ar370}      the real tool binaries
<prefix>/cc370/{include,lib,macros}         the libc370 sysroot (headers, libc.a, crt*.o, macros)
<prefix>/lib/cc370/1.0.0/                    empty: GCC's libsubdir, but required (see below)
```

The target name is `cc370` (a `config.sub` alias for the real `i370-ibm-mvspdp`
backend); nothing user-facing carries the old triple. Everything for the target
lives in one `cc370/` tree; only the user-facing binaries sit on `PATH`. The
empty `lib/cc370/1.0.0/` is GCC's *libsubdir*: it holds no libgcc (we ship none),
but the driver locates the whole `cc370/` sysroot — both `<stdio.h>` and `-lc` —
via a path relative to it, so it must exist. Builds on x86-64 and ARM64.

### Sysroot — where cc370 finds the libc

cc370 searches `<prefix>/cc370/{include,lib}` by default (the `cc370` component is
the target name, from `-dumpmachine`). [libc370](https://github.com/mvslovers/libc370)
drops its headers, `libc.a`, the `crt0/1/m.o` startfiles and the assembler macros
all under `<prefix>/cc370/` (`include`, `lib`, `macros`); as370, whose real binary
lives in `<prefix>/cc370/bin`, finds the macros via its `<exedir>/../macros`
default. After that the toolchain is self-contained:

```sh
make -C ../libc370 install                   # populate the sysroot once
cc370 hello.c -o hello -flinker-output=xmit  # no -I, no -L, no -lc needed
```

## Optimization: `-O1`, and `-Os` as experimental

**`-O1` is the validated level** and mbt's default.

**`-Os` is experimental.** It makes code about 5 % smaller across the ecosystem
(rexx370 8.5 %). Three defects that kept it from even assembling are fixed:

- an unconditional `B` at the end of a code page could land behind the next
  page's `USING` (#575);
- unit-at-a-time dropped `static` tables held only by a global pointer and
  moved top-level `asm` ahead of the code (#590) -- now off by default;
- strict aliasing reorders code that casts between control-block layouts, for
  0.1 % of size (#592) -- now off by default.

With them, all 1,017 ecosystem sources that compile also assemble at
`-Os`. Validated on MVS so far: one run of the rexx370 suite, with
`-Os` test modules (64 tests, batch and TSO, 5,685 assertions, 0 failures;
mvsdev JOB01477). Modules the tests load from the production LINKLIB were
still `-O1`, and no other project has been run. **A project that adopts `-Os`
validates it with its own tests.** An earlier note that `-Os` "miscompiles the
rexx parser (loops → S322)" has no recorded measurement and did not reproduce.

`-O2`/`-O3` are untested and not supported. They get the same two defaults;
`-funit-at-a-time` / `-fstrict-aliasing` still turn the passes back on.

## Layout

```
cc370/             the GCC 3.4.6 fork — the compiler (driver + cc1)
  gcc/config/i370/   the i370/mvspdp target backend — check here first for codegen bugs
  gcc/ libiberty/ intl/ include/ config/   GCC core + build deps, largely upstream
as370/   src/ include/ tests/   as370 — host-native MVS assembler  (as370/README.md)
ld370/   src/ tests/            ld370 — host-native linker (replaces IEWL)
ar370/   src/                   ar370 — .a archiver (ld370 autocalls against it)
file370/ src/                   file370 — format inspector (file/objdump for the formats above)
xmit370/ src/ tests/            xmit370 — TSO TRANSMIT for source PDSes (samplib, JCL, macros)
common/  src/ include/          format primitives shared by the tools (CP037, CKD, NETDATA)
docs/                           object / load-module / unload / xmit formats + roadmap
Makefile                        make (whole toolchain) / make tools / make compiler
```

Deep dives: [`CLAUDE.md`](CLAUDE.md) (architecture + gotchas),
[`as370/README.md`](as370/README.md), and `docs/` (the on-the-wire formats).

## Provenance

The compiler is a fork of GCC **3.4.6** — the last GCC to carry the i370 machine
definition — for the `i370-ibm-mvspdp` target (`TARGET_PDPMAC`: HLASM/EBCDIC,
`COPY PDPTOP` / `PDPPRLG` / `PDPEPIL`, [libc370](https://github.com/mvslovers/libc370)-compatible
calling conventions), slimmed to that one target.

It descends from the gccmvs / [i370-gcc](https://github.com/linas/i370-gcc) line
(Paul Edwards, Dave Pitts, and others). Specifically, this tree is a snapshot of
`mvslovers/i370-gcc` @ `feature/ebcdic-char-constants` (commit `0710af0c`),
brought in as v2.0 because the upstream PRs to linas went unanswered — so the
compiler is developed against the ecosystem's own requirements from here on. The
earlier GCC 3.2.3-based generation is the frozen `mvslovers/c2asm370` repo
(**v1.x**), kept as a fallback for projects still on the old name.

What this snapshot carries over stock GCC 3.4.6:

- **EBCDIC charset (CP037 + ecosystem NEL):** char constants and string literals
  map to CP037; `\n` maps to EBCDIC NEL `0x15` (byte-identical to the ecosystem
  httpd `cp037_atoe`/`cp037_etoa`, so it matches mvsMF upload and HTTP output);
  `\x..`/`\NNN` escapes stay binary-literal.
- **Codegen for IFOX00:** fixed-column assembler text, valid MVS symbol names for
  private statics, signed 32-bit literals; the `asm()`-extern address-of fix.
- **Host portability:** builds on macOS and Linux, x86-64 and ARM64, modern
  clang/gcc-14 hosts.
- **Slimmed to i370:** the tree was cut from ~20 640 to ~1 100 tracked files
  (other GCC targets, other-language front ends, the test suite, other-language
  runtimes), then the leftover build cruft (changelogs, contrib, stale objects).

### Building the compiler by hand

`make compiler` wraps this; the equivalent direct invocation (the GCC 3.4.6 fork
needs the old-C `-w`/`-Wno-*` flags on a modern host):

```sh
CF="-g -O0 -fcommon -std=gnu89 -Wno-implicit-int -Wno-implicit-function-declaration \
    -Wno-int-conversion -Wno-error -Wno-return-type -Wno-deprecated-non-prototype"
mkdir build && cd build
CFLAGS="$CF" CFLAGS_FOR_BUILD="$CF" ../cc370/configure \
  --target=i370-ibm-mvspdp --enable-languages=c \
  --disable-threads --disable-nls --disable-shared --without-headers
make all-gcc CFLAGS="$CF" CFLAGS_FOR_BUILD="$CF"   # -> build/gcc/{cc1,xgcc}
```

Maintained by the [mvslovers](https://github.com/mvslovers) community.
