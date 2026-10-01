# Mainframe Classic Linker architecture

The inherited modules retain their PDLD roles. The current repair changes the
classic reader's coordinate conversion and its relocation metadata.

| Module | Responsibility |
| --- | --- |
| `ld.c`, `libld.c` | Host entry point, command options, input loading and output dispatch |
| `read.c`, format modules | Identify objects/archives and dispatch their readers |
| `symbols.c`, `sections.c`, `ld.h` | Symbol resolution, section/part identity and shared link state |
| `link.c` | Assign section placement and apply generic relocation arithmetic |
| `mainframe.c` | Classic ESD/TXT/RLD/END reader and MVS load-module writer |
| `xmit.c` | XMIT envelope, metadata and physical card-image packaging |
| `map.c` | Human-readable linked section and symbol map |
| `bytearray/`, `ftebc/`, `ftasc/` | Explicit byte-order access and character encoding helpers |

## Classic origin normalization

A classic A constant already contains its target's assembled address. If a
section's original origin is `O` and its linked origin is `L`, relocation adds
`L - O`. The inherited reader retained `O` as a symbol placement and generic
relocation then added the full linked address. An A constant containing `O`
therefore counted that origin twice.

The repair keeps the existing section packing policy and stores each SD/PC's
original origin and declared length separately from its placement in a merged
input part. A single checked conversion maps assembled TXT, LD and RLD
positions:

```text
position in merged part = section placement + assembled address - original origin
positive A value        = nominal value + linked target origin - original target origin
negative A value        = nominal value - (linked target origin - original target origin)
V value                 = nominal value + resolved external symbol address
```

The conversion rejects positions before the original origin and fields beyond
the declared section length. LD uses its owner section; RLD uses its source
section and the actual relocated width. ESD identifiers must identify a real
SD/PC or, for external targets, a named undefined ER. Split TXT records use the
same conversion as the first TXT record.

The existing generic relocation arithmetic receives an A addend of `-O`.
V external references keep the full resolved value. The existing subtraction
relocation operations implement negative fields. MVS/XMIT output preserves
the corresponding signed RLD flag. No new relocation width or object dialect
is added by this repair.

For example, the original fixture has a target origin `0x20`, nominal A value
`0x24` and linked target origin `0x20`. The correct result is `0x24`; adding the
target origin again would produce `0x44`. Another fixture moves a zero-origin
target after a 32-byte source section, checking that original origin and linked
placement remain distinct.

The reader still uses inherited PDLD allocation, symbol resolution and section
packing. It is not the assembler's bounded callback core. The build retains
the inherited C99 requirement and keeps its warning policy local to this
component; the original fixture checker remains strict C90.
