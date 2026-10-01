# Using mf-classic-as

The `0.1.0-bootstrap` build produces binary classic object decks, ready for a
separately qualified compatible linker/loader. It does not link or execute code.

```text
mf-classic-as [--profile s360|s370] [--macros] [-I directory] input.asm output.obj
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
| Sections | Named CSECT, one unnamed CSECT created/restored by an unlabeled CSECT, internal named DSECT layout, implicit unnamed section for ordinary statements; no overlays |
| Symbols | ASCII names, case-insensitive identifiers, longer internal labels; bounded caller storage |
| Expressions | Decimal, `X'hex'` wide integers and signed 32-bit `B'bits'` terms, symbols, `*`, parentheses, unary signs, addition/subtraction and supported single-target relocation expressions |
| EQU | Values resolvable in the layout pass; forward EQU chains are unsupported |
| TITLE | One named deck ID up to eight characters; 1..100-character quoted heading. CP037 deck IDs occupy bytes 73..80, space padded, without generated sequence suffixes. Headings/listings are not produced. |
| PRINT | Validated ON/OFF, GEN/NOGEN, DATA/NODATA controls and null operands; no listing output in this component. Labels and other control forms are rejected. |
| DC | H/F integers, AL1/AL2/AL3 absolute and A addresses, absolute eight-byte AD, external V, hexadecimal X and CP037 C/CL character constants; checked duplication and target length |
| DS | Reservation/alignment for the documented constant types, including `0H`, `0F`, `0D`; no emitted bytes for gaps |
| Addressability | USING with up to 15 registers at successive 4096-byte bases, register-zero mappings at offset zero, DROP lists or all; symbolic addresses need a matching USING |
| Visibility and entry | ENTRY, EXTRN, implicit externals for a single-name V constant, and END with an optional section-relative entry |
| Literals | Selected `=H'number'`, `=F'number'`, `=X'hex'`/`=XLn'hex'`, `=A(expression)` and `=V(name)`, explicit LTORG and implicit END pool |
| Modes | AMODE 24/31/ANY; RMODE 24/31/ANY; name associates the declaration with its section, including a later section |

Mode declarations do not create a section. A blank name selects the unnamed
section, which must exist by END; it does not select the current named section.
Each mode may be declared once per section. ANY addressing is represented
separately from AMODE31 and emits the classic ANY flag.

H and F are signed 16- and 32-bit constants. Positive A values can use all 32
bits; absolute AD uses two 32-bit parts, including sign-extended negative values.
Eight-byte relocatable AD is unsupported. C/CL constants are sized after CP037
conversion and padded with target spaces; X constants preserve explicit bytes.

A zero-duplication DC aligns and defines its label without emitting a value,
creating an implicit external or recording a fixup. H/F/A/AD/V may omit their
nominal in this case; C/X may omit it when an explicit length is present.
Omitted plain `0C`/`0X` remain unsupported. A supplied numeric/address nominal
is checked for the selected expression syntax and the 64-bit numeric-atom
limit, but its value is not computed or checked against the field width.
Symbol references in an ungenerated nominal need not resolve. Character/hex
nominals still undergo the documented lexical and explicit-length checks.
This does not yet implement symbol length/type attributes.

Ordinary assembler multiplication/division, ORG and cREXX expansion remain
unimplemented. The optional provider has its separate scalar conditional subset. Traditional definitions require the
optional provider selected below. Unsupported
constructs fail explicitly. The source parser's documented subset is distinct
from the pure encoder's instruction descriptions.

The selected literals have no duplication or nested nominal list. Only X
literals accept explicit length, with checked padding and no truncation. A literals accept the ordinary checked address expression and addend. H/F literals are signed decimal; X literals contain a
nonzero number of hexadecimal digits with left zero padding; V takes one external name. Identity
is case-insensitive source spelling within one pool: differently spelled
numeric values are not automatically merged. Pending literals collect globally
across sections. LTORG emits them in the current real section; END emits the
remaining pool at the end of the first real section. The pool starts on an
eight-byte boundary and groups lengths divisible by 16, then 8, 4, 2, then odd,
preserving encounter order within each group. A 16-byte literal need not start
on a 16-byte boundary. Gaps remain reserved, without emitted fill bytes.

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
| HFP subset | LPDR, LTDR, LCDR, LDR, CDR, ADR, SDR, MDR, DDR: `f1,f2`; STD, LD, CD, AD, SD, MD, DD, STE, LE, AE: `f1,d(x,b)`; historical FPRs 0,2,4,6 only |
| Selected S/370 additions | BASR, BAS, CLM, LRER (long-to-short HFP), STCM, ICM, CS, CDS, MVCL, CLCL |

Branch aliases lower to the ordinary BC/BCR encodings: B, BO, BH/BP, BL/BM,
BNE/BNZ, BE/BZ, BNL/BNM, BNH/BNP, BNO and NOP. Each takes an address operand;
append R for the register form, including BR and NOPR. NOP still requires its
address operand in this subset. Alias handling belongs to the source engine,
so replacement providers can use the same encoder with resolved BC/BCR fields.

Register/mask fields are 0–15, displacements 0–4095 and byte immediates 0–255.
The encoder's character SS length is 1–256; decimal lengths are 1–16 with MP/DP restrictions.
An explicit source length of zero is normalized to one: both encode a zero
length field. This supports EX templates without wrapping zero to 255.
Register-pair restrictions are checked. Values are rejected before narrowing.
The core encoder takes resolved fields; the engine owns expressions and USING.
Write `0(12)` for zero displacement with base register 12. A parenthesized
displacement expression such as `(4+1)` is an expression, and can precede an
explicit suffix as in `(TARGET+4)(,12)`.
For an SS length operand, `AREA(20)` supplies the length and infers a matching
base from USING; `AREA(20,12)` selects base 12 explicitly. The second character
SS operand may also infer its base, as in `XC AREA(20),AREA`. Implied lengths
from symbol attributes remain unsupported.

## Object and failure contract

The writer emits SD/PC/ER/LD ESD identities, TXT bytes, four-byte A/V RLD fixups
and END. Internal and serialized IDs are separate. It preserves section-relative
entry, mode metadata and gaps. Serialized CSECT, exported and external symbol
names must fit eight CP037 bytes without truncation; record addresses/section
lengths must fit 24 bits. Overlays and wider
relocations are rejected. Local symbols and DSECT names remain internal. V names occupy a separate
external-reference namespace and can match a local ENTRY or CSECT name. The
object retains ER plus LD/SD identities and a V relocation for linker resolution;
an ordinary A expression requires a local definition or explicit EXTRN.

An artifact is usable only after successful assembly, writer and sink completion.
Return code 0 means success; 1 means assembly or output failure; 2 means command,
input-open or output-path setup failure. Diagnostics identify source coordinates
where available and a project-owned status category, including malformed source,
unsupported feature, range, duplicate, undefined, capacity, changed replay,
object representation and I/O failure.

The desktop configuration has 64 sections, 4096 symbols, 65536 fixups, 256
literal identities across all pools by default (`--literal-limit 0..65536`
selects an explicit count; zero disables literals), 256 statement bytes and expression depth
32, with a 32 MiB allocation-payload budget.
Allocator metadata and host buffers are additional. Other adapters choose their
own explicit limits; this does not qualify a 24-bit native host's memory fit.

## Optional traditional macros

The ordinary CLI uses the identity reader. A CMake build includes the optional
traditional provider by default; `--macros` selects it for that input. Configure
with `-DMF_TRADITIONAL_MACROS=OFF` to omit its source and library entirely. The
direct bootstrap recipe also omits it. Such builds reject `--macros` with return
code 2; neither build silently expands macros without the option.

The first provider supports inline MACRO/prototype/MEND definitions, invocation
labels, positional arguments, keyword defaults and overrides, explicit empty
arguments, parameter substitution, period delimiters and nested calls. Quoted
commas and balanced parentheses stay inside one argument. Missing positional
arguments are empty; missing keywords keep their literal default. Positional
actuals fill only positional formal slots and must precede keyword actuals.
Definitions cannot be redefined. It accepts both `*` and `.*` comment cards.
A comma on MACRO/MEND is an empty marker operand; a comma on a call represents
two empty positional arguments and is checked against the prototype.

With `--macros`, repeat `-I directory` to select ordered library directories.
The host searches lower-case and original-case member names with `.mac`, `.asm`,
then no suffix. COPY inserts library records; an unknown operation can load a
member containing exactly one matching macro definition. Missing COPY members,
cycles, depth limits, I/O errors and changed dependency records fail explicitly.
The core uses host-supplied record handles and source identities, never paths.
Library diagnostics name the supplying file. Desktop bounds are 16 directories,
256 source identities, 16 simultaneous members and 1,024 path bytes.

The provider accepts the scalar variables, conditional branches and SYSNDX
described in the macro guide. It rejects continuation, COPY inside macro
definitions, variable arrays, general symbol attributes, dynamic or nested sublist indexing, escaped
ampersands and nested definitions. The [macro guide](MACROS.md) gives an original example and the
storage/replay/provenance contract. This is a language subset; it supplies no
IBM service or mapping macros.

Desktop limits are 64 definitions, 16 parameters per definition, 1,024 total
model statements, 256 KiB definition bytes, nesting depth 16, 4,096 argument
bytes per frame, 256 total field bytes per yielded statement and 1,000,000
raw-record/model steps per pass. They share the driver's 32 MiB payload budget.
Definitions, frames and workspaces are acquired at creation; advancing/replaying
does not acquire per-invocation storage. Failures invalidate output through the
same assembler/writer contract.

`PUSH USING` saves the current addressability state without changing it;
`POP USING` restores it. The fixed stack has 16 frames. Underflow and overflow
fail explicitly. `DROP` with no operand or a single comma drops every mapping.
Absolute implicit addresses also use applicable absolute USING mappings.
Forward USING expressions defer resolution to pass two; unresolved mappings
never produce a successful deck. Other PUSH/POP state classes remain unsupported.

DC/DS duplication factors also accept parenthesized absolute expressions whose
layout values are already defined. Forward layout dependencies are rejected.
Repeated address constants reevaluate the location counter for each element.
