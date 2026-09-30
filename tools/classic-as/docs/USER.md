# Using mf-classic-as

The `0.1.0-bootstrap` build produces binary classic object decks, ready for a
separately qualified compatible linker/loader. It does not link or execute code.

```text
mf-classic-as [--profile s360|s370] input.asm output.obj
mf-classic-as --version
mf-classic-as --help
```

The default instruction profile is `s360`. Source files contain ASCII bytes in
short lines or fixed cards, with LF or CRLF endings. Core record adapters can
also supply CP037 records. Tabs and non-ASCII syntax are rejected. A label starts
in column 1; statements without a label start with a space. Columns 1–71 contain
syntax, column 72 continuation is unsupported, and columns 73–80 are ignored
sequence fields. A `*` in column 1 starts a comment. Spaces inside quoted
constants and doubled apostrophes are preserved; whitespace after an unquoted
operand begins a remark. Do not insert spaces inside ordinary operands.

Output is binary octets; do not apply text/EBCDIC conversion to the deck. The
desktop driver stages output and refuses readable existing or uncheckable targets. Source
and assembly failures leave the requested output absent. A failed final write
removes the new incomplete file. Standard C stdio does not provide exclusive
atomic publication against a concurrent creator, or detect a dangling symbolic
link through its existence check. Callers must own the output pathname and use
an absent regular-file destination. A different host adapter may supply stronger
publication semantics.

## Bootstrap language

| Construct | Initial support |
| --- | --- |
| Sections | Named CSECT, internal DSECT layout, implicit unnamed section for ordinary statements; no overlays |
| Symbols | ASCII names, case-insensitive identifiers, longer internal labels; bounded caller storage |
| Expressions | Decimal and `X'hex'` integers, symbols, `*`, parentheses, unary signs, addition/subtraction and supported single-target relocation expressions |
| EQU | Values resolvable in the layout pass; forward EQU chains are unsupported |
| DC | H/F integers, A addresses, absolute eight-byte AD, external V, hexadecimal X and CP037 C/CL character constants; checked duplication and target length |
| DS | Reservation/alignment for the documented constant types, including `0H`, `0F`, `0D`; no emitted bytes for gaps |
| Addressability | One base per USING statement, DROP, explicit base/index fields; same-section symbolic addresses need a matching USING |
| Visibility and entry | ENTRY, EXTRN and END with an optional section-relative entry |
| Modes | AMODE 24/31; RMODE 24/ANY, checked independently of the instruction profile |

H and F are signed 16- and 32-bit constants. Positive A values can use all 32
bits; absolute AD uses two 32-bit parts, including sign-extended negative values.
Eight-byte relocatable AD is unsupported. C/CL constants are sized after CP037
conversion and padded with target spaces; X constants preserve explicit bytes.

Multiplication/division, literals/LTORG, COPY, ORG, branch aliases, traditional
macro/conditional assembly and cREXX expansion are not implemented. Unsupported
constructs fail explicitly. The source parser's documented subset is distinct
from the pure encoder's instruction descriptions.

## Instruction profiles

`s360` is a selected common S/360 instruction subset, excluding Model 67
extensions. `s370` adds selected S/370 forms, including the optional Branch and
Save facility documented in the March 1981 manual. It does not promise those
facilities on every historical S/370 machine. Mode metadata selects object
attributes; it does not grant a historical CPU a new address mode.

| Format | Selected mnemonics and operand shape |
| --- | --- |
| RR | BALR, BCTR, BCR, LPR, LNR, LTR, LCR, NR, CLR, OR, XR, LR, CR, AR, SR, MR, DR, ALR, SLR: `r1,r2`; SVC: one byte immediate |
| RX | STH, LA, STC, IC, EX, BAL, BCT, BC, LH, CH, AH, SH, MH, CVD, CVB, ST, N, CL, O, X, L, C, A, S, M, D, AL, SL: `r1,d(x,b)` or supported symbolic address |
| RS | STM, LM, BXH, BXLE: `r1,r3,d(b)`; SRL, SLL, SRA, SLA, SRDL, SLDL, SRDA, SLDA: `r1,d(b)` |
| SI | TM, MVI, NI, CLI, OI, XI: `d(b),immediate` |
| SS character | MVC, NC, CLC, OC, XC, TR, TRT, ED, EDMK: `d1(length,b1),d2(b2)` |
| SS decimal | MVO, PACK, UNPK, ZAP, CP, AP, SP, MP, DP: `d1(length1,b1),d2(length2,b2)` |
| Selected S/370 additions | BASR, BAS, STCM, ICM, CS, CDS, MVCL, CLCL |

Register/mask fields are 0–15, displacements 0–4095 and byte immediates 0–255.
Character SS length is 1–256; decimal lengths are 1–16 with MP/DP restrictions.
Register-pair restrictions are checked. Values are rejected before narrowing.
The core encoder takes resolved fields; the engine owns expressions and USING.
Write `0(12)` for zero displacement with base register 12. A parenthesized
displacement expression such as `(4+1)` is an expression, and can precede an
explicit suffix as in `(TARGET+4)(,12)`.

## Object and failure contract

The writer emits SD/PC/ER/LD ESD identities, TXT bytes, four-byte A/V RLD fixups
and END. Internal and serialized IDs are separate. It preserves section-relative
entry, mode metadata and gaps. Serialized CSECT, exported and external symbol
names must fit eight CP037 bytes without truncation; record addresses/section
lengths must fit 24 bits. Overlays and wider
relocations are rejected. Local symbols and DSECT names remain internal.

An artifact is usable only after successful assembly, writer and sink completion.
Return code 0 means success; 1 means assembly or output failure; 2 means command,
input-open or output-path setup failure. Diagnostics identify source coordinates
where available and a project-owned status category, including malformed source,
unsupported feature, range, duplicate, undefined, capacity, changed replay,
object representation and I/O failure.

The desktop configuration has 64 sections, 4096 symbols, 65536 fixups, 256
statement bytes and expression depth 32, with a 32 MiB allocation-payload budget.
Allocator metadata and host buffers are additional. Other adapters choose their
own explicit limits; this does not qualify a 24-bit native host's memory fit.
