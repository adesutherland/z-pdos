# Installed cREXX workspace controller

TDV is U-side policy over the existing TJF converter, native job/DD service and
op38 compare/save interface. Programs, short console commands and line mode
share it. It introduces no second executor, background scheduler or
native linking mechanism.

DEV.SITE selects distinct source PDS, executable PDS, sequential WORK and STATE.
All must have the expected organisation and record attributes and must differ
from protected SITE/runtime/procedure/load names. The factory installs SOURCE
as FB80, and OUTPUT/WORK/STATE as VB260/264. SITE's profile fingerprint covers
the matching native tools, runtime libraries and common procedures.

BUILD snapshots the source token, compiles/assembles/links with CRXCL into WORK,
then checks every native step state, OS result and valid RC. It rechecks source,
validates both logical and physical publication sizes, and rereads actual
persisted executable bytes before changing build identity. The registry stores two
triples per program: source token, SHA256 of the **framed logical records**, and
installed-profile hash. It preserves the triple matching the actual old artifact
and adds the candidate triple. A CAS commit of this registry precedes CAS
publication of the executable. Compiler failure touches neither artifact nor
build triples. A successful job's selected-result metadata may still be saved.

After interruption, the old or new executable must match a complete stored
triple. The next build preserves the triple matching actual disk bytes, rather
than assuming which pair is latest. RUN checks all three components together;
equal bytecode from two different source versions is valid if either complete
triple matches. Profile changes preserve workspace identity while invalidating
old RUN eligibility. Failed/uncertain registry publication blocks artifact writes
unless a reread proves the complete intended registry. An uncertain artifact
publication is reconciled only by rereading its framed hash. No path resubmits
native jobs automatically.

STATE is strictly versioned and binds all four workspace names. Its first logical
record is 176 bytes: magic/version, program count, binding hash, source/output
names and last selected job's id/kind/member/immutable step hash. Each
208-byte program record has native member name, triple count/reserved word and
two 96-byte triples. Unused triples are zero. Selected-job recovery checks the
native step count and hash of each step's immutable first 124 bytes. Review
permission is volatile and must be renewed after restart. Unknown jobs remain
available through the existing advanced recovery path.

Preparation validates source/artifact/arguments and fully converts JCL before
replacing output. Only the exact selected, successfully read completed result
may be purged by a new explicit BUILD/RUN. All other jobs block the request.
TJFPOST consumes its prepared result before the single submit attempt; zero-id
or uncertain acknowledgement never authorises replay.

The current eight STORE slots bound the profile to two programs: spool,
transcript, WORK, STATE and two source/executable pairs. Op38's 64KiB logical and
native packed limits also apply; a two-MiB STORE slot does not enlarge them.
The controller never erases unrelated keys to create capacity. PD-030 owns
broader storage/extent work.

Programs opens an invocation-owned raw 3270 session. It renders bounded real
Member, Arguments and Command fields and recovers complete modified fields
through Read Buffer. Source, Build, Run, Output, Details and Help use one
character-cell frame: title on row 0, member/context on row 1, body from row 2,
command on rows-3, notice on rows-2 and PF legend on rows-1. Coordinates are
zero based. Each row reserves its first and last cells for attributes. The
shared C90 `twospace_workspace` helper supplies the frame and native job-step
summary; there is no second executor or simultaneously active editor.

Member and Arguments retain their positions (row 3/column 12 and row 5/column
12). Command starts at rows-3/column 7, matching EDIT. Source retains its
existing prefix/data positions, one buffer, atomic submitted-page validation
and rejected-draft rules. A trailing `WORKSPACE` launch option changes EDIT's
presentation only; standalone EDIT and LINE remain available. The narrow
source view displays 69 native columns, and the 132-column view displays 121.
Full dataset names remain available in Help while the header names the member.

PF1 opens Help; PF3 returns one panel, with Source retaining its refusal to
abandon unsaved changes. PF2 means Edit in Programs and Save in Source.
PF7/PF8 page Output. Build/Run completion retains the real native step summary
and the controller's publication/freshness notice. A refused request with no
acknowledged job returns to Programs instead of presenting an earlier job as
the new attempt. Details reads checked native state without granting Output
review permission. Only an explicit successful Output action followed by a
successful visible Output paint grants that volatile permission. Output wraps
native records within the body and sanitises control bytes in the view only;
the retained records are unchanged.

Before closing raw ownership for an action, Programs installs a checked
invocation-owned presentation context. K can paint the same protected frame
while the native job executes synchronously, using actual step state and
OS/RC fields. Native job completion is labelled Finalising until U completes
its publication checks. Busy frames do not restore the keyboard or advertise
panel keys. There is no percentage estimate, background scheduler or second
interactive shell. The accepting raw panel restores input when it reopens.
An actual child or operator input prompt uses the established Workbench reply
field and its input owner while it waits. This exceptional input view preserves
the existing synchronous prompt contract; normal batch DD input does not
enable terminal input or unlock the busy frame.

EDIT still receives its own raw session. The native adapter retains the
checked parent screen acquire/release handoff; a capability or handoff error
prevents child launch. Programs clears presentation context on every exit,
including errors, and K also clears it on controller unwind. Physical line
terminals skip screen operations and preserve semantic line output. Foreign
raw ownership remains refused. The existing BATCH forms and console remain
independent advanced routes to the same job service. Host checks cover the
shared frame, field codecs, native states and ownership gates; guest
qualification is a separate acceptance step.

## Design constraints and extension

Subpanels are feasible as a shared presentation frame with one active owner.
They do not require simultaneous windows or a resident editor. This matters
because EDIT is a separately loaded native child and the terminal has one raw
input owner. The common frame preserves visual context across that handoff.

The designer's constraints were the existing model2–5 geometries, monochrome
operation, C89/native codepage boundaries, bounded storage, synchronous jobs,
safe member saves, and unchanged line/card/printer interfaces. No UI action
may weaken source freshness, executable publication, result review or terminal
ownership. Busy status must come from observed native state and must not imply
that publication is complete. Retained output bytes are never layout scratch.

A future panel should reuse title/context/body/command/message/keys and the
same Back/Help conventions. Interactive fields belong only to its current raw
owner; a child must receive a checked ownership handoff. New presentation task
identities need a checked ABI extension, and new job actions must use the
existing controller/service policy. The current U payload has little spare
space, so substantial additions need a separately reviewed allocation or
module split rather than silently growing into neighbouring disk records.
