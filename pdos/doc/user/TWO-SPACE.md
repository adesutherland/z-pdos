# Selectable K/U source-built image

The PD-003 successor is available as an explicit source build. P0–P5 have
accepted results; P6 workload/default qualification remains open. The
published 0.1.1 image keeps its recorded one-ASCE boot route.

Use the owning checkout on develop, prepared Classic C/assembler/linker tools,
GNU s390 assembler/linker, Hercules and the frozen beta-3 CMS/TSO stage inputs
listed in the [ABI inventory](../architecture/TWO-SPACE-ABI.md). Invoke:

```sh
crexx -nokeep pdos/scripts/two-space-normal.crexx --args \
  build/pdos/normal-image "$GNU_AS" "$GNU_LD" "$HERCULES_BIN" \
  "$CMS_STAGE_DIR" "$TSO_STAGE_DIR" 2 3270 monitor
```

All output paths must be fresh. Tool and input directories are absolute.
The last three arguments explicitly select model 2/3/4/5, primary `3270` or
`line`, and optional `monitor` or `none`. Omitting them selects model 2,
3270 at 0009 and the monitor at 000A. The selected normal disk is
`build/pdos/normal-image/image/media/pdos00.cckd`. The source K core and
producer manifests are under that build's kernel directory. With explicit
console arguments, use the configured `normal.core` at the build root for
the guest gate.

A normal guest needs ESAME, one CPU, 256 MiB real storage, that disk at 01B9,
a primary device matching the selected configuration at 0009 and, when
selected, `000A 3215 noprompt`. Connect the terminals before IPL 01B9. K
starts U PCOMM; an ordinary EXIT returns to K, commits the transcript and
prints K SHUTDOWN before disabled wait. The monitor remains output-only
unless a U application explicitly hands off its next line prompt.

The P5 accepted command route includes:

```text
VERSION
CMS RUN 31 RXVM -v
RXVM -v
MISSING
RXVM -v
EXIT
```

Each external command has numbered BEGIN/END output. A real application
return reports RC. A launch or service failure reports OS status and
`RC=unavailable`; it must not be read as an application RC. CMS RUN selects
its declared mode. Ordinary TSO names search 31-bit, then absent-only 24-bit
and 64-bit fallbacks; profile-specific dataset aliases can select unchanged
members when their names overlap.

For the repeatable disposable gate:

```sh
crexx -nokeep pdos/scripts/two-space-normal-gate.crexx --args \
  build/pdos/normal-gate "$ABSOLUTE_IMAGE_BUILD" "$ABSOLUTE_CORE" "$HERCULES_BIN"
```

The optional fifth argument `rootfault` stages the small deliberate U-root
fault fixture instead of PCOMM. That control verifies K emergency output,
owned cleanup, stopped-disk integrity and durable text; it is not a normal
operator command or release asset. [P5 qualification](../qualification/TWO-SPACE-P5-2026-10-07.md)
records the exact accepted build and guest identities.
