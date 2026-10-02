# Optional traditional macro provider

The first provider increment is implemented and host-qualified for the PDPCLIB
consumer. [The checkpoint](../qualification/PDPCLIB-CHECKPOINT.md) records exact inputs, independent
review and QA. Complete MVSSUPA assembly, conditional assembly and service
definitions remain separate gates.

The bootstrap reader and engine remain independent of macros. A capable host
can select a separate streaming provider that reads `mf_records` and emits the
existing `mf_statements` contract. The provider never opens source paths, uses
a host heap, retains the expanded compilation unit or supplies an IBM service.
Its code and small qualification fixtures are original project material.

## First language gate

The first gate selects MACRO/prototype/MEND definitions, invocation-label
substitution, positional parameters, keyword defaults and overrides, explicit
empty values, quote/parenthesis-aware argument lists, parameter substitution,
period concatenation and nested calls. Empty operands remain different from a
missing keyword that uses its default. Commas inside quoted values or balanced
parentheses do not split a macro argument.

For example, this original definition needs no service library:

```text
         MACRO
&LABEL   LOADVALUE &REG,&SOURCE
&LABEL   L     &REG,&SOURCE
         L     &REG,0(&REG)
         MEND
DEMO     CSECT
FIRST    LOADVALUE 4,16(1)
         END FIRST
```

Its two instructions have independently expected bytes `58401010 58404000`.
The macro definition is the input; generated expansion files are not maintained
replacement sources. PDPCLIB's separately licensed LDVAL/LDADD/STVAL definitions
can be tested as reference consumers without copying them into this provider.

The maintained original CLI fixture includes load/address/store definitions and
a keyword override. Run it on a capable host using a fresh output path:

```sh
build/tools/assembler/mf-classic-as --macros \
  assembler/tests/fixtures/macros.asm build/example-macros.obj
build/tools/assembler/test_macro_deck build/example-macros.obj
```

For the separately supplied, pinned source-owned definitions, the optional
[reference consumer recipe](../../../pdpclib/scripts/check-macros.crexx) takes an assembler,
the standalone checker, an external MVSSUPA path and a test-owned build directory.
It neither imports definitions into the source tree nor uses an expanded
replacement assembly file. That external consumer is additional to the ordinary
public test suite, which uses only original fixtures.

The provider now accepts open-code COPY and automatic library macro lookup
through `mf_macro_set_library`. The host owns library records and handles;
the provider bounds nesting, detects active-member cycles, fingerprints copied
comments and definitions, and closes every opened handle. Independent in-memory
and CLI fixtures exercise lookup, nesting, CP037 members, missing/cyclic inputs,
capacity, I/O failures and changed unused records across replay. COPY inside
a macro definition still requires definition-time insertion and is rejected.

The conditional increment supports scalar LCLA/B/C and GBLA/B/C, SETA/B/C,
AIF/AGO/ANOP/MEXIT and SYSNDX. Local state is fresh per macro invocation;
global state persists after compatible redeclaration, but must be declared in
each scope that uses it. Parameter/local collisions and repeated local
declarations fail. SET assignments can implicitly declare a local scalar.
Arithmetic uses checked signed 32-bit values, decimal terms, parentheses and
+/-/*/division. Logical expressions support NOT, AND, OR and EQ/NE/LT/LE/GT/GE;
character comparisons use CP037 order and right-space padding. Quoted character
values preserve case. The expression nesting bound is 32.
Quoted character primaries accept substring notation `(start,count)`, with
one-based arithmetic start and nonnegative count or `*` for the remainder.
Doubled apostrophes count as one character. This selected subset rejects
out-of-range slices instead of silently accepting warning-producing forms;
concatenation and character duplication remain unsupported.

Macro sequence branches can go forward or backward in their own definition;
the step limit bounds loops. Open-code branches currently scan forward within
the supplying record stream. Missing targets fail; backward open-code branching
and cross-member lookahead remain unsupported. Skipped records still contribute
to replay consistency. MEXIT ends the current invocation. SYSNDX uses the
invocation's decimal index, padded to at least four digits, reset on replay.

K' reports immediate parameter text length and N' its top-level sublist count.
The invocation label is also a formal parameter when the prototype declares it;
these immediate attributes include its supplied or omitted text.
Literal positive decimal indices select parameter sublist elements; an absent
element is empty. Scalar variable indexing and dynamic or nested indices remain
unsupported. The selected T' parameter query distinguishes immediate numeric text (N),
omitted text (O) and unknown text (U). It does not infer assembler symbol types
or offer phase-dependent lookahead. General symbol attributes,
continuation cards, COPY inside definitions, arrays, created variables and
cREXX preprocessing remain unsupported. Each scope has at most
`max_model_statements` scalar variables, names at most 64 bytes and character
values at most `max_statement_bytes`; their storage is allocated at creation.

MNOTE with severity 8 through 255 ends expansion with a source error. Lower
severity reporting still needs an explicit diagnostic interface and is rejected;
messages never silently disappear.

## Storage, replay and provenance

Configuration bounds macro definitions, formal parameters per definition,
total model statements, definition bytes, nesting depth, argument bytes per
frame, output-statement bytes and processing steps per pass. Checked size
arithmetic precedes allocation. Definition storage and reusable frames and
workspaces are allocated through `mf_storage`; advancing or replaying the
provider does not acquire per-invocation storage. Many unlabeled calls with
the same definitions and nesting depth must use the same storage as a short
stream. Step and depth limits turn nonterminating recursion into explicit
capacity failure.

Replay resets definition visibility, frame state and counters and rebuilds
definitions in the existing arenas. The provider fingerprints all consumed
records, including unused definitions and comments. At replay EOF it compares
fingerprints and counts and returns MF_REPLAY on a change, even when expanded
instructions would be identical. Like the engine's check, this is a bounded
consistency check rather than a cryptographic source identity guarantee.

A generated statement carries its outermost source invocation coordinate.
An optional synchronous observer exposes its definition coordinate and bounded
nested invocation frames. Borrowed names and frames last only through the
observer call; the observer must copy what it retains. The current single
`mf_origin` in statements cannot preserve a complete historical chain for
delayed symbol-resolution diagnostics. A later shared provenance extension
must make that ownership explicit.

## Subsequent gates

Substrings, dynamic or nested sublist indices, variable arrays and backward open-code replay
belong to later conditional provider gates.
Source-owned shared globals must retain their value when redeclared, while
locals start afresh on each call. Variable assignments must reuse bounded
storage. No optional cREXX backend becomes a bootstrap prerequisite.

The supported N' and K' describe argument structure and substituted text. T'
omission test can also be local to arguments, but general T' and ordinary-symbol
L' require assembler metadata and explicit phase/visibility rules. The query
contract must distinguish known, not yet available and undefined; pass-two
conditions must not silently see pass-one final symbols and change expansion.
Forward lookahead is a separate design, not part of the first provider.

COPY resolution and runtime/OS service/mapping definitions remain separate
dependencies. Each service replacement needs a selected OS edition, public
interface facts and independent field/byte checks. Original simplified
definitions must identify their supported forms. A traditional macro parser
does not grant rights to IBM macro source or establish those service layouts.

A single comma can mark absent prototype operands and absent call operands
for a macro with no formal parameters, including a label-only parameter.
Commas in a macro with formal parameters continue to denote positional null
arguments. Other empty prototype slots remain unsupported. This follows IBM
HLASM [macro instruction prototype](https://www.ibm.com/docs/en/hla-and-tf/1.6?topic=definitions-macro-instruction-prototype).

An open-code forward conditional branch searches labels and operation fields
without checking inactive operands or continuation syntax. Labels inside
skipped macro definitions cannot be open-code branch targets. The selected
target is scanned fully. Every skipped raw record remains part of the replay
fingerprint, so a changed inactive record invalidates output. Macro nesting
in the search is bounded by the configured depth.
