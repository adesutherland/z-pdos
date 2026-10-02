# Classic C user guide

Run from the repository root with native C development tools, make and cREXX:

```sh
crexx -nokeep compiler/scripts/build.crexx --args test mvs
crexx -nokeep compiler/scripts/build.crexx --args test cms
build/compiler/mvs/gcc/mf-classic-cc -S -O1 hello.c -o build/hello.asm
```

The inherited configure/Makefile system builds directly from compiler/src/
into separate ignored MVS and CMS directories. The original launcher uses
its sibling xgcc/cc1; rebuild it after moving the checkout. It supports -S,
-E, -fsyntax-only and compiler queries. It still rejects object generation,
implicit assembly/linking and unqualified --profile selections.

The MVS target is i370-ibm-mvspdp; CMS is i370-ibm-cms. Both use the retained
PDPCLIB macro convention, HLASM-style assembly text and EBCDIC characters.
int/long/pointers are 32 bits, long long is 64 bits and plain char is unsigned.
This data model alone does not select AMODE31 or a 64-bit ABI. Supply target
headers/runtime explicitly; --without-headers is not a sysroot.

-O0/-O1/-O2/-Os consolidation fixtures pass. Upstream's strict-aliasing and
unit-at-a-time defaults remain; passing selected fixtures is not general
optimised-program qualification. [The checkpoint](../qualification/CHECKPOINT.md)
and [backlog](../BACKLOG.md) retain actual scope and limits.
