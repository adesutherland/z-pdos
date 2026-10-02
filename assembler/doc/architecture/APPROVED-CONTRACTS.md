# Mainframe Classic Assembler: approved component contracts

30 September 2026. **Approved for local implementation.** This guide records
the logical source-level contracts. The product's [mf_classic.h](../../src/include/mf_classic.h) owns the concrete declarations and its
component documentation owns implemented coverage. No stable binary ABI is
promised. These contracts support the [approved architecture](APPROVED-DESIGN.md).

## Common rules

- Headers and implementations use the C89/C90 common subset. Public core
  headers may use `<stddef.h>` and `<limits.h>`; they do not expose `FILE`,
  POSIX types, cREXX types, C++ classes or platform handles.
- Components exchange explicit lengths, small integer identifiers, statuses
  and pointers borrowed for a stated lifetime. No serialized native C structs,
  implicit NUL termination, host byte order or exactly-32-bit `long` assumption.
- The core is ordinary synchronous code. Callbacks have a caller-owned cookie;
  function tables select an implementation at construction/build time. There
  is no runtime discovery or shared-library loader contract.
- Each assembly has its own context. Borrowed callback data lasts until that
  callback returns; source spans last until the provider advances. Retained
  values/names must be copied into session storage. Error unwinding releases
  session ownership through the supplied storage adapter.
- A status distinguishes success, end of stream, source/semantic error,
  unsupported capability, capacity exhaustion and host I/O failure. Output
  publication and process return codes are driver decisions based on this
  result. No component calls `exit` or uses a nonlocal jump as its API.
- Interfaces are versioned at source level. A backend declares the language,
  object and optional query capabilities it implements. Adding an optional
  capability does not require every bootstrap host to implement it.

## Values and identities

| Value | Logical representation and contract |
| --- | --- |
| Octet | An eight-bit target byte, held in `unsigned char` on supported hosts |
| 32-bit part | Unsigned value constrained to 32 bits, represented by a C unsigned type of at least 32 bits; helpers explicitly constrain arithmetic when the host type is wider |
| Wide value | Two checked 32-bit parts or equivalent octets; signed interpretation and target width are explicit; no mandatory `long long` or `<stdint.h>` |
| Text span | Borrowed UTF-8 pointer and `size_t` byte length; distinguish it from a binary octet span |
| Origin | Source identifier, record/line and column, plus an optional expansion-parent identifier; the host maps source identifiers to names |
| Section/symbol identity | Session-local identifier, distinct from serialized ESD identifiers and target addresses |
| Expression value | Absolute value or a supported symbolic relocation expression; unresolved state is distinct from zero |
| Fixup | Owning section, location, target/reference expression, kind, width and addend; writer must reject unsupported representation |

Internal source text, identifier names, textual resolver names and diagnostic
arguments use UTF-8 independently of the host C execution character set.
Instruction-name tables and parser comparisons must use that contract explicitly.
The bootstrap language accepts the ASCII subset and rejects non-ASCII syntax
with a declared diagnostic. General Unicode libraries, normalisation and Unicode
case folding are not mandatory. A later language profile may support valid UTF-8
names without confusing their byte lengths with character counts or object names.

Raw record input declares its external encoding. The record decoder/card reader
preserves the original fixed columns and record/line coordinates before returning
UTF-8 statement fields. It may use a coordinate map when expansion/diagnostics
need one. UTF-8 byte offsets never substitute for original card columns. The
bootstrap reader's ASCII subset makes the positions coincide for ASCII input;
non-ASCII UTF-8 fixed-card source needs an explicitly selected column convention.

Character constants choose a declared target encoding and are sized after that
conversion. Hex constants preserve explicit bytes. The object writer checks
external names in its target encoding, including its byte limit, and rejects
unrepresentable names. Host file/console adapters map UTF-8 names and rendered
diagnostics to native text. Binary streams remain octets throughout; no adapter
may apply text conversion to an object stream.

IBM bit positions use a most-significant-bit-zero convention in source references.
The machine description must explicitly identify that convention; helpers must
not silently substitute host bit numbering. Checked encode/decode interfaces
agree on field meaning, sign interpretation and instruction-relative bases.

## 1. Storage service

`Storage.acquire(bytes)` returns suitably aligned session storage or a capacity
failure. `Storage.finish()` ends ownership of the session's allocations. An
arena implementation can reset once; a richer adapter can release its retained
allocations. Reallocation, individual free, a filesystem spool and a general
host heap are not mandatory core services.

The configuration sets limits for symbols, sections, literals, expression depth,
source/expanded statement size, expansion depth and retained fixups. Sizes are
checked before multiplication or conversion to `size_t`; failure does not wrap
into a smaller allocation. Session statistics report peak storage requested,
counts and the reached limit. Actual host code/BSS/stack/heap measurements remain
part of hosting qualification, not implied by these logical counters.

Scratch and optional backend storage must be declared in the build's resource
report. A bootstrap backend may require a replayable source or caller-supplied
spool rather than retain every statement. No hidden large fixed global arrays.

## 2. Source and statement providers

| Operation | Contract |
| --- | --- |
| `Records.begin(source)` | Start an identified input; declare character encoding and record convention |
| `Records.next()` | Return one raw length-bearing record with its declared encoding and original coordinates, EOF or an explicit host error |
| `Records.replay()` | Replay the same input; implementation may reset, reopen or read a supplied spool |
| `Records.end()` | Release host input ownership |
| `Statements.prepare(records, resolver, limits)` | Select the identity or macro backend and prepare a stable expanded program |
| `Statements.next()` | Return label, operation, operand spans and origin/expansion provenance |
| `Statements.replay()` | Return the same expanded program for another assembly pass |
| `Statements.end()` | Release provider ownership |

The card reader owns column rules, continuation, decoding to UTF-8 and lexical
field recognition.
The statement provider owns preprocessing. Identity preprocessing and a macro
backend use the same field representation; the assembler does not know which
backend produced a statement. Synthetic/macro statements retain a useful source
location and expansion chain.

Preparation can be lazy for a replayable identity source. A stateful macro backend
must stabilise its output once, or demonstrate equivalent deterministic replay.
Changes between passes are an error. Clocks, environment, include resolution
and user parameters are fixed for the session if that backend exposes them.
These facilities are absent unless explicitly supplied; no core clock is needed.

`Resolver.open(logical_name, role)` returns a source/provider handle for COPY or
macro definitions. `role` distinguishes source inclusion and macro lookup. The
host owns naming and search order; the core does not assume Unix paths, PDS
names, environment variables or an executable-relative macro directory. Resolver
absence is legal until the selected source requires it. Include cycles, missing
members and configured depth/size limits produce useful diagnostics.

The first macro subset cannot query unresolved ordinary-symbol/layout attributes
implicitly. Such queries are either a declared error or use a later explicit
read-only assembly-environment interface with documented phases. They cannot
mutate the assembler's symbol table through a macro callback. cREXX integration
uses this same constraint; a full HLASM-compatible query protocol is not assumed.

## 3. Assembly configuration and session

Configuration identifies four separate choices:

1. machine/profile and permitted instruction features;
2. language subset, reader and preprocessing capabilities;
3. object-format capabilities, address/relocation limits and mode metadata;
4. host storage/I/O implementations and resource limits.

Address mode and residence metadata are explicit target attributes, not guessed
from a mnemonic or from the host pointer size. Invalid combinations fail before
output is accepted. Internal symbol names and external object names have separate
limits and identities.

The logical session operations are `create`, `assemble` and `destroy`.
The driver owns the providers, writer, sinks and storage adapter. It closes
providers and destroys the assembly context before calling `Storage.finish()`.
Destroying a context does not close or reset caller-owned devices. A shared
arena remains valid until all its users have finished; individual components
cannot reset it while another component retains spans or state.
The following sketch illustrates dependency direction, not final structure names:

```c
struct mf_as;
struct mf_as_config;
struct mf_storage;
struct mf_statements;
struct mf_object_writer;
struct mf_diagnostics;
struct mf_as_result;

int mf_as_create(const struct mf_as_config *config,
                 const struct mf_storage *storage,
                 struct mf_as **out);
int mf_as_assemble(struct mf_as *session,
                   const struct mf_statements *source,
                   const struct mf_object_writer *writer,
                   const struct mf_diagnostics *diagnostics,
                   struct mf_as_result *result);
void mf_as_destroy(struct mf_as *session);
```

The assembly engine owns pass sequencing, sections, symbol resolution, layout,
literals and addressability. Expression evaluation returns explicit relocation
information as well as the numeric value. Unsupported expression/fixup classes
fail; substituting zero and emitting a successful deck is not an implementation.

For the bootstrap subset, a layout pass followed by a validated emission pass is
the intended starting algorithm. Additional passes require a documented reason
and a convergence bound. Diagnostics do not disappear because another pass is
run, and a prior fatal error cannot be erased by final output completion.

## 4. Machine lookup, encoder and future decoder

`Machine.lookup(profile, mnemonic)` identifies an instruction/alias or rejects
it. `Machine.describe(id)` supplies format and operand constraints. These
descriptions are original project data with edition-specific references.

`Encode(profile, instruction, typed_operands)` returns the validated instruction
octets and length. It accepts registers, immediate values and resolved address
fields; it does not evaluate source expressions, select a USING, open a file,
allocate persistent assembler state or alter a symbol table. Input range errors
are distinguishable from unsupported instruction/profile errors.

`Decode(profile, octets)` is a future sibling returning instruction identity,
length and typed fields. ISA profile resolves architecture-dependent opcode
meanings and preferred aliases. The decoder does not execute the instruction.
CPU execution uses a separate state/memory/exception interface outside this
proposal. Shared encode/decode round trips supplement independent test vectors.

## 5. Object writer and byte sink

The assembly engine supplies final section and symbol information, then emission
events. The object writer's logical operations are:

| Operation | Contract |
| --- | --- |
| `begin(module, sections, symbols, modes)` | Validate the requested object profile and initialize the selected writer |
| `text(section, offset, octets)` | Emit bytes at a section-relative position; ordered streaming must not erase legal gaps or source-supported overlays |
| `gap(section, offset, length)` | Represent reserved/unemitted storage without inventing text bytes |
| `fixup(reference)` | Preserve location, target, width and relocation meaning, or reject it |
| `entry(reference, mode)` | Preserve the declared entry section/offset and supported mode attributes |
| `finish(valid)` | Complete only if assembly, writer and sink all succeeded; otherwise invalidate the output |

The first selected representation is the supported classic 80-octet
ESD/TXT/RLD/END object contract. It needs an independently authored writer.
Format widths and external-name limits are checked, including limitations that
remain even for wider instruction profiles. Eight-byte constants and eight-byte
external relocations are separate capabilities. A future format is a separate
writer and qualification decision, not an automatic extension of this one.

The sink writes sequential byte spans and reports partial writes/failures.
It exposes begin/finish/discard semantics; it need not support seek, rename or
delete. A driver that cannot retract already written bytes must leave failed
output explicitly incomplete/unusable and return failure. Consumers may use
only successfully completed artifacts. A desktop driver can stage and rename
files to implement stronger publication behaviour without changing the core.

The writer does not link external modules, search archives, generate IPL tracks
or supply a missing ABI. Those remain separate tool/component contracts.

## 6. Diagnostics and inspection

`Diagnostics.report(code, severity, origin, arguments)` receives a stable
project-owned diagnostic identity with typed context. Host adapters render
messages and source names. Independent implementation does not require copying
IBM diagnostic wording or matching its exact identifiers and return-code scale.
The retained ASMA90 diagnostic identities remain reference expectations; the new
assembler has a documented mapping to its own failure categories.

Listings and symbol/statement/addressability exports are optional observers of
checked events. Omitting them must not change assembly semantics. Diagnostics
needed for correctness remain present even in a small build. A discarded or
unimplemented statement cannot count as an assembly success.

## Replacement and conformance

Replacement implementations run a common contract suite: empty/EOF/error input,
identical replay, source/macro locations, storage exhaustion, range failures,
object-width failures, partial writes and incomplete-output handling. Tests
also check that optional capabilities are absent cleanly in a bootstrap build.

The bootstrap/standard builds must match section, bytes, relocation and entry
meaning for their overlapping supported language/profile combinations.
Serialized incidental ESD numbering can differ when references remain correct.
Use original expected bytes and private retained native reference evidence as
independent controls. A host component test does not qualify native z/PDOS
hosting, complete runtime assembly, linking or source-to-IPL.
