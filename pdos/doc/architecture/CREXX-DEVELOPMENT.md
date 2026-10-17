# Installed cREXX workspace controller

TDV is U-side policy over the existing TJF converter, native job/DD service and
op38 compare/save interface. Programs, short console commands and line mode
share it. It introduces no second executor, new K ABI, background scheduler or
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
Member, Arguments and Action fields, recovers complete modified fields through
Read Buffer, and closes raw ownership before invoking a child EDIT or shared
controller action. Its Output page displays native records first, wraps long
printer lines and sanitises terminal control bytes. Advanced Details returns to
the console. The existing BATCH forms and console remain independent routes to
the same job service.

Closing a raw session repaints the parent Workbench and creates its semantic
input lease. Before launching EDIT on a 3270, the native adapter explicitly
acquires/releases that parent's screen lease with the existing checked API.
This surrenders the newly repainted input field without closing the Workbench.
A capability or handoff error prevents child launch. Physical line terminals
skip the screen operations. K and its refusal of foreign raw ownership are
unchanged; Programs reopens its own raw session after the child returns.
