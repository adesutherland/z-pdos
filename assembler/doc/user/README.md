# Assembler user guide

Run from the repository root with a C89/C90 host compiler and CMake; cREXX
runs the development recipe. The resulting assembler has no cREXX dependency.

```sh
crexx -nokeep assembler/scripts/build.crexx --args test
build/assembler/mf-classic-as assembler/tests/fixtures/bootstrap.asm build/example.obj
```

See [USER.md](USER.md) for accepted syntax, profiles, object records and
failure behaviour; [BOOTSTRAP.md](BOOTSTRAP.md) describes the direct C build.
Source headers and implementation are together under `../../src/`.
An existing object output is refused; choose a new path. Native guest hosting
is a separate backlog item.
