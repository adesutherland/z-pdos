# Optional traditional macro provider

The first provider increment is implemented and host-qualified for the PDPCLIB
consumer. [The checkpoint](PDPCLIB-CHECKPOINT.md) records exact inputs, independent
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
build/classic-as/tools/classic-as/mf-classic-as --macros \
  tools/classic-as/tests/fixtures/macros.asm build/example-macros.obj
build/classic-as/tools/classic-as/test_macro_deck build/example-macros.obj
```

For the separately supplied, pinned source-owned definitions, the optional
[reference consumer recipe](../../check-pdpclib-macros.crexx) takes an assembler,
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

The first gate explicitly rejects continuation cards, conditional
controls and variables, argument sublist indexing, SYSNDX and assembler-state
attributes. Passing an invocation through as an unexplained no-op would hide a
missing build dependency. Ordinary instructions and directives emitted by the
provider still pass through the same engine/profile/object checks.

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

Scalar local/global variables, SETA/SETB/SETC, AIF/AGO/ANOP/MEXIT, argument
sublists, substrings and SYSNDX belong to the next conditional provider gate.
Source-owned shared globals must retain their value when redeclared, while
locals start afresh on each call. Variable assignments must reuse bounded
storage. No optional cREXX backend becomes a bootstrap prerequisite.

N' and K' can describe argument structure and substituted text. A selected T'
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
