# Linker user guide

Run from the repository root with CMake, a C99 host compiler and cREXX:

```sh
crexx -nokeep linker/scripts/build.crexx --args test
crexx -nokeep linker/scripts/build.crexx --args sanitize
build/linker/mf-classic-ld --version
```

[USER.md](USER.md) describes the reached Classic object and output formats.
The inherited PDLD backends have distinct qualification; importing them does
not establish compatibility with an IBM binder or execution in a guest.
