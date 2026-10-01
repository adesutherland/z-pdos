# Component interfaces

The authoritative C declarations are [mf_classic.h](../include/mf_classic.h).
They are source-level contracts, not a stable binary ABI. No native C structure
is serialized into an object deck.
The optional provider has a separate public header,
[mf_classic_macro.h](../include/mf_classic_macro.h), and emits the existing
statement interface without changing the bootstrap core's dependency graph.

| Contract | Meaning and ownership |
| --- | --- |
| `mf_span` | Borrowed numeric UTF-8 bytes with an explicit length; ASCII subset initially |
| `mf_origin` | Source identifier, original line/record and column; the host renders the source name |
| `mf_storage.acquire` | Caller allocator supplies aligned session-lifetime storage; no per-block free or realloc required |
| `mf_records.next/replay` | Supply raw records in declared encoding; borrowed record lasts until next/replay |
| `mf_reader_init` | Bind the reader to caller storage and a record provider; returned statement fields last until next/replay |
| `mf_macro_create/destroy` | Optional fixed-capacity traditional provider; creation acquires session arenas, next/replay reuse them; caller owns record/storage lifetimes |
| `mf_macro_observer.notify` | Synchronous statement/failure observation with borrowed definition/model/invocation frames, outermost first; callback must copy anything retained |
| `mf_statements.next/replay` | Supply identical logical statements and original coordinates for both passes |
| `mf_machine_lookup`, `mf_encode` | Profile lookup and pure encoding of checked typed fields; no expressions or USING state |
| `mf_as_create/assemble/destroy` | Per-assembly context, pass sequencing and diagnostics; context never closes devices or resets shared storage |
| `mf_obj_create/destroy` | Bind classic writer to storage and sink; writer validates object representation independently |
| `mf_sink.begin/write/finish` | Sequential binary output, explicit I/O failure and validity completion |

The status separates normal EOF, source/semantic errors, unsupported features,
capacity exhaustion, I/O failures, range errors, duplicates, unresolved names,
changed replay and object-format errors. Core diagnostics have stable project
categories and borrowed detail spans. The host owns readable messages and
process return codes; IBM diagnostic wording and identifiers are not copied.

An assembly context accepts one assembly attempt. Create a fresh context for
another input, and release session storage only after its users have finished.
`mf_as_config.max_literals` bounds total retained literal identities, including
identities in pools already emitted. Zero disables literal syntax and avoids
allocating its metadata table. Each unique selected literal spelling is copied
into session storage; repeated instruction lines are not retained.
If a provider returns an error, only its original source coordinates are usable;
the engine must not inspect statement spans from that failed call. A non-null
assembly result carries the returned status and marks failed output invalid,
including rejected API arguments. Missing diagnostics callbacks are allowed.

Sections and symbols have session-local IDs. Serialized ESD IDs are assigned
by the writer. A fixup identifies its owning section and location separately
from its target. A/V address kind is distinct from section/external target kind;
an external A reference cannot silently become V. Constant addends are already
in emitted bytes. The first writer supports four-byte relocations only. V-only external names
are separate from ordinary symbols; the symbol array can contain an ER and
LD with the same name and different IDs, or an ER matching a named section.
Explicit EXTRN binds the ordinary namespace and still excludes local definition.
Writer name uniqueness is per exported/external identity class; A and V keep
their distinct relocation kinds when referring to the same eventual address.

An optional `deck_id` callback precedes `begin` for a named TITLE. Initialize
that field to NULL when unsupported; a named TITLE then fails explicitly.
The metadata span is borrowed during the synchronous call. The classic writer
copies and CP037-encodes the ID, bounds it to eight bytes, and space pads the
remaining identification bytes. It emits the same ID on ESD/TXT/RLD/END cards,
without generating a sequence suffix. TITLE names do not define ordinary symbols.

Writer operations are `begin`, `text`, `gap`, `fixup`, `entry`, `finish`, plus
optional `origin` and `deck_id` callbacks initialized to NULL when unsupported. Begin
receives final sections/symbols; later events do not mutate their identities.
Gaps preserve reserved storage without inventing bytes. An explicit origin event flushes pending TXT and resets a section cursor
without writing fill. Later TXT can overlay earlier absolute bytes. The
selected writer rejects origins below the end of any prior relocated field;
this conservative suffix restriction prevents stale RLD from corrupting an
overlay and needs only bounded per-section state. Dummy sections describe layout without emitted storage.
A fixup follows its field's text before a gap or noncontiguous text event in
that section, so the writer can validate written coverage with bounded state.
Local symbols and dummy-section names remain internal and are not serialized.
An entry owns a section and offset. Format widths, name lengths, modes and all
references are checked before successful completion.

Driver cleanup order is: complete/invalidate output, destroy assembly and writer
contexts, release source/provider ownership, then release the storage session.
A sink failure after END bytes have been written still makes the artifact
unusable. Completion status is part of the contract, not merely presence of END.

Replacement providers, allocators and sinks must pass common tests for EOF,
malformed input, changed replay, storage exhaustion, partial/failing output and
invalidation. Richer providers must preserve results for overlapping supported
subsets. Optional resolvers, macro queries, listing observers and richer writers
are future additions with their own contracts.

Macro replay requires a completed EOF. It resets definition visibility, depth,
workspaces and counters, then rebuilds definitions in the existing arenas.
Changes in ignored comments, unused definitions, sequence fields or original
coordinates still fail the raw-record consistency check. A yielded statement
uses its outermost invocation as primary origin; observer frame names and all
spans are borrowed only through callback return. The engine does not retain
those frames for delayed diagnostics. Macro creation failure may have acquired
some blocks; the supplied allocator owns their session cleanup, just as for the
assembly and writer contexts.
