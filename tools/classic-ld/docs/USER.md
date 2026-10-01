# Using Mainframe Classic Linker

Build from the repository root:

```sh
crexx tools/classic-ld/check.crexx --args build
build/classic-ld/mf-classic-ld --help
build/classic-ld/mf-classic-ld --version
```

The version identifies the product and inherited implementation:

```text
mf-classic-ld (PDLD 0.19 lineage)
Mainframe Classic Linker
```

Choose the output format explicitly. An original classic object can be linked
to a flat binary image with:

```sh
build/classic-ld/mf-classic-ld --oformat binary --image-base 0 \
  -e ENTRY -Map build/program.map -o build/program.bin build/program.obj
```

For the qualified mixed PC/SD MVS/XMIT route, use `--oformat mvs` or
`--oformat xmit`, an explicit entry symbol, and explicit AMODE/RMODE as required
by the consumer. The inherited XMIT writer derives its member name from the
first eight characters of the output argument. Run in the chosen output
directory with a member basename such as `-o PLCORE.XMI`; passing a directory
path as that argument also places its leading characters in the member name.
Keep the executable and input paths absolute when changing directory.

Set `SOURCE_DATE_EPOCH` to a nonnegative, representable decimal timestamp for
reproducible XMIT metadata. Without it, the inherited writer uses current local
time. Invalid timestamp values fail.

The current host checks cover classic ESD/TXT/RLD/END inputs, nonzero original
origins, split TXT, LD exports, positive and negative A fields and V external
references. The MVS/XMIT checks use a PC identity followed by named SD sections,
as in the retained PDPCLIB package. The inherited all-named-SD writer route and
nonzero-origin PC MVS map/CESD handling remain unqualified. The latter can lose
the PC symbol's local placement during writer indexing even when the flat
binary image is correct. Do not infer support for these routes from the reader
repair or its binary controls.

Other inherited formats and options are listed by `--help`; their presence is
not a new compatibility or qualification claim. Linking, transfer packaging,
guest loading and guest execution are separate steps.
