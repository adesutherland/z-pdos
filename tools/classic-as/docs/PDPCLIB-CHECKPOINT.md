# First PDPCLIB language checkpoint

30 September 2026. The approved consumer order is the TSO31 entry adapter,
PDPCLIB MVSSUPA, then PDOS loader/kernel support. The bootstrap completion is
published as `e87652d2bc1efdee4439302be4420a5c3ac8911e`. I approved committing
and publishing this independently reviewed follow-on delivery. It does not
qualify whole MVSSUPA, its external services or a booted OS.

## First complete PDPCLIB target

I selected MVS 3.8 on real System/370 with 24-bit addressing (AMODE 24/RMODE 24)
for the first complete PDPCLIB qualification. It uses traditional MVS linkage
and classic object decks. This is a target decision; the separately maintained
configuration member and exact service-definition sources still need to be
implemented and qualified. The supplied PDPTOP remains a read-only reference
with different defaults. The existing TSO31 entry experiment retains its own
31-bit profile and does not establish this new target's acceptance.

## Ordinary layout slice

The engine now normalizes explicit source SS lengths zero and one to the same
zero encoded field, while the pure machine encoder continues to require an
architectural length of at least one. A single unnamed CSECT can be created and
restored after a DSECT or named section, including a previously implicit unnamed
section. Its location, modes, ENTRY and replay semantics survive restoration;
an unnamed DSECT remains unsupported.

Selected zero-duplication DC forms align and define a label without generating
nominal bytes, implicit externals or fixups. Numeric/address nominals are syntax
checked with bounded numeric atoms but are not evaluated or resolved. Thus a
well-formed ungenerated expression may exceed the target width or refer to an
undefined ordinary symbol. Malformed expressions still fail. Character/hex
nominals retain their lexical checks. The exact forms and omissions are in
[the user guide](USER.md).

The dedicated strict C89 and ASan/UBSan suite passes 1,253 checks. Independent
review decoded a mixed actual CLI deck: restored unnamed PC, no emitted DSECT,
zero-AD labels at offsets 8 and 16, SS template `D20010002000`, exactly two
positive A/V fixups and no nominal-only implicit externals. END identifies
section 1, offset 8. Maximal 64-bit atoms followed by addition/subtraction are
accepted without evaluating an ungenerated nominal; this was corrected after
independent review found unnecessary arithmetic evaluation.

The TSO31 entry deck remains exactly
`492d89c81beb8f85655c67723d1898652c5a7a8f1aec3bffb81ec2132f81d0d5`.
Its 3,248 independent object checks pass with this engine.

## Optional traditional provider

The separate C89 provider implements the first language gate described in
[MACROS.md](MACROS.md), selected by CLI `--macros`. It uses the existing
`mf_records` and `mf_statements` interfaces and the caller's storage. Definition
tables, model/argument arenas and frames are acquired at creation; next/replay
acquire nothing and retain no expanded compilation unit or per-call history.
The bootstrap reader and five-file assembly core have no dependency on it.

The provider and object-integration suites check label/positional/keyword
binding, explicit empty arguments, period delimiters, quoted and nested argument
lists, nested-call ownership, original coordinates, ASCII/CP037 cards and
unsupported syntax. They cover every configured budget, allocation failure,
step/depth exhaustion, source I/O, changed replay including unused definitions
and comments, and invalid output after writer/sink failure, including failure
after END. Thirty thousand repeated macro calls use the same allocator count
and payload as one call; fifty thousand ordinary statements acquire no
additional provider storage. Independent review also exercises ignored sequence
byte changes and nested observer coordinates.

Review found a quoted-argument scanner regression involving a comma inside
`D'A,B'`. It is corrected and covered by new maintained tests and 329 independent
strict-C89/ASan/UBSan controls over positional/default/override/doubled-quote
values and unsupported bare attributes. No concrete finding remains in this
first provider gate. The final provider suite passes 1,225,979 assertions and
the actual assembler/object suite 2,301; the earlier 1,790 unaffected independent
controls remain valid. The coordinator matched all six delivered source/test
identities to the reviewed delivery.

Final SHA-256 identities of the provider implementation are
`2b22befc053ed1210facbedffdeee3430788e3b9dc54ca752225c3d48ec02707`
(`macro_cards.c`) and
`4d1e3a50fc2837b42dd3249136701d6229f6660dd74e71a1a6f7a4985fd5b164`
(`macro_provider.c`). Its public header is
`c58c2ea7a37c57c8cb43300ad7c0140fb0bf01f72591e18120eab20ca967c6c9`.

## Actual source-owned macro consumer

[check-pdpclib-macros.crexx](../../check-pdpclib-macros.crexx) takes an
explicit external MVSSUPA source, checks its reviewed SHA-256, and assembles a
test-owned subset using unchanged LDVAL, LDADD and STVAL definitions. Its input
retains the supplied public-domain notice and contributor header. Definitions
remain in the read-only Lab reference and ignored test output; no upstream or
IBM macro definition is maintained in this product. The recipe does not retain
an expanded replacement source.

The reviewed MVSSUPA identity is
`27758af986baae46fb726fe9bb35ed993898b98d547e315587328940c2f41e31`.
Definition ranges are 298–302, 306–309 and 455–459. The consumer invokes LDVAL
with register 4, LDADD with register 5, and STVAL with register 6 and both its
default R14 scratch register and an explicit register-7 override. Its ten
emitted statements produce one 28-byte section, three symbols and no fixups.

Independently expected instruction bytes are:

```text
58401010 58404000 58501014 58E01018 5060E000 5870101C 50607000
```

The standalone checker links no assembler/provider/writer helper and passes
101 record/byte assertions. The coordinator matched every selected input line
and notice to the pinned file, checked the fixed bytes, and rejected four decks
with altered opcode, section size/name or END entry. The subset source SHA-256
is `2fd00cee478d2be510856c8f219e7f1fb97d1ee634c41b9a65a9c5d97357a1e4`;
the 240-byte deck SHA-256 is
`cc3f5b86b416b6dbd6f4326eb24a71c7173456dba40fca80a69853520162cb15`.
With the identity reader selected, this source fails explicitly and publishes
no deck. A build omitting the provider rejects `--macros` before assembly.

## Qualification and remaining gates

The integrated strict-C90 and ASan/UBSan configurations each passed 28 registered
tests. After the final narrow scanner correction, both affected macro suites
and the actual source-owned consumer/checker passed again in both configurations;
the deck identity is unchanged. Unaffected checks retain their passing results.
Three subsequently registered original CLI fixture checks also pass in both
configurations, giving 31 registered tests with passing results. This fixture
requires no Lab reference or external macro library.
The macro-disabled configuration passes 26, and the documented direct C89 build
succeeds. The first ordinary layout and optional provider gates are accepted
as host component slices.

Complete MVSSUPA assembly still requires conditional variables/control flow,
argument sublists and selected attributes, continuation/COPY, ordinary
constant/layout/addressability features, and a deliberately qualified ORG
emission strategy. Source-owned macros and OS/service definitions are separate
dependencies. The supplied PDPTOP unconditionally selects S/380 and
z/Architecture branches; it cannot silently stand for a real S/370 build.
The first complete target is now MVS 3.8 / real System/370 / 24-bit. Its
configuration member and service-definition edition remain open, as recorded
in [the inventory](NATIVE-SUPPORT-INVENTORY.md).

The inherited nonzero-origin PDPCLIB input/link defect in the
[entry checkpoint](../../../runtime/tso31/CHECKPOINT.md) remains unresolved;
whole-image execution has not started. PDOS loader/kernel assembly, native
assembler hosting and source-to-IPL are separate later gates. No SDK, guest,
Lab reference, historical worktree or automation was changed by this slice.
