# cREXX operator workflow review - 16 October 2026

## Outcome and scope

The editor/batch foundation is qualified, but the normal installed experience
is not yet the requested create/edit -> BUILD -> RUN workflow. The immediate
priority is one integrated development configuration and a small developer
surface over the existing services. [PD-035](../BACKLOG.md#pd-035-installed-crexx-development-profile-and-operator-workflow)
owns the proposed implementation and acceptance; this review does not start it.

Adrian authorised the baseline commit before this review. Product baseline is
`bafc481fce6733fd80d1c91e88594d98a31c2683` on `develop`; Lab baseline is
`abf3a6fc` on `main`. Both commits are local; publication and managed adoption
remain separate. The earlier 175/175 result is reused for its named service,
editor, form and terminal cases. It does not prove the broader installed user
journey reviewed here.

The coordinator and an independent GPT-6.1 Sol High subagent reviewed the
maintained source, guides and all nine actual PDOS manual screenshots. The
coordinator independently reproduced three editor cases with the actual C
controller/parser in a temporary host harness: typing into the second visible
row of an empty file returns 8; I on its initial new row returns 8; source edits
plus an invalid prefix return 8 with the old model unchanged. The driver then
repaints that old model. The temporary source/binary were removed. No new guest
qualification or product code change is claimed by this review.

## Findings

### P1: The installed configuration and the Lab fixture are different products

The normal bundle installs beta.3 RXC, RXAS and RXVM, plus LIBRARY, RXCEXITS and
IOQUAL, with source/assembler/bytecode PDS collections. It does not install the
four-tool batch environment, its RXLINK, SITE or common PROCLIB. Current batch
admission also requires RXFNSC and RXFNSG. A user following the fixture manual
therefore needs substantial hidden preparation.

Sources: `scripts/bundle-media.py:18-23,94-119,153`,
`src/twospace_submit.c:95-108`, `src/procs/BATCH.SITE:2-9`.

Supply one maintained, reproducible development profile with mutually matching
tools, runtime libraries, compiler exits, procedures, site bindings and writable
user collections. The normal installation must not depend on LABA01.B33,
BTEST31 aliases, TEFPROBE, a private Lab checkout or a prior qualification disk.
Do not mix the existing beta.3 bundle with newer batch components by filename.

The Mainframe Operator's current metadata/manifest inventory confirms:

| Selection | Present | Limit |
| --- | --- | --- |
| Managed local instance | Accepted 0.2.0 source `b2b718e`, 256 MiB, model 5 | New EDIT/batch facilities have not been adopted; disk contents were not reread. |
| Normal source bundle | Current source-built EDIT and pinned beta.3 three-tool package | Missing the batch environment described above. |
| Qualified development inputs | Current K/U/EDIT match the accepted handback identities; all twenty batch package artifacts match the recorded manifest | The disposable qualification image was retired; the current reconstruction route depends on private fixtures. |

The four-tool development package is recorded at cREXX source `47ee4e40`,
with matching LIBRARY/RXFNSC/RXFNSG/RXCEXITS. It is distinct from the pinned
public beta.3 bundle. A portable product profile needs an explicit validated
package input produced by its owning build pipeline, or an approved matching
delivered archive. It cannot silently combine those two package selections.
Select the qualified 512 MiB TSO31 K/U profile with its generous application
storage; do not inherit the older managed instance's smaller configuration.
No new guest run or managed-image change followed this inventory.

### P1: BUILD and RUN are not separate user actions

The batch recogniser exposes BATCH/SUBMIT/JOBS/STATUS/OUTPUT and operator
controls, but no developer BUILD/RUN actions. Its default CRXCG compiles,
assembles and runs with temporary output. CRXLGO adds linking but also deletes
its generated program after RUN. Neither leaves the executable requested by a
later separate RUN.

Sources: `src/twospace_submit.c:278-284`,
`src/twospace_batchui.c:49,64-80`, `src/procs/CRXCG.jcl:17-26`,
`src/procs/CRXLGO.jcl:27-35`, `src/procs/CRXCL.jcl:20-27`.

A developer controller should make BUILD produce persistent bytecode and RUN
execute that selected artifact through a VM-only job. Reuse the existing
converter/admission/job/DD services and CRXC/CRXCL behavior. Add the missing
run-only route; do not build another scheduler or compiler invocation path.
Publish a new artifact only after successful build outcomes. A failed rebuild
must preserve the previous good artifact and clearly mark it as belonging to an
older source; RUN must not silently present it as a build of changed source.

### P1: A rejected page discards the text just typed

TESINPUT validates atomically and leaves the accepted model unchanged on an
invalid prefix/field. The driver reports a generic refusal and repaints the
old model. Several valid source changes entered alongside one mistyped prefix
therefore disappear from the screen. The saved file is protected, but the
phrase "buffer retained" does not mean the submitted page draft was retained.

Sources: `src/apps/edit_screen.c:78-100`, `src/apps/edit.c:83-89`.

Keep a rejected page draft available for correction while preserving the
accepted model. Identify the offending field, keep the other typed text,
position the cursor at the error and require an explicit discard to abandon
that draft. A regression check must cover multiple typed changes plus one bad
prefix, correction, acceptance, undo and subsequent save.

### P2: New-file entry presents controls that do not work as advertised

All body prefix/source fields are editable, including rows beyond EOF. Only
count+1 is labelled `new`; text farther below EOF is refused. The advertised
I command on that initial new row is refused too. Untouched NULL rows do not
create blank records, so consecutive entry with an intervening empty line can
also fail. Initial focus is the command field rather than the first source row.

Sources: `src/apps/edit_screen.c:26,35-49,58-61,87-98`.

Give an empty member a clear New program presentation and useful initial
cursor. Define predictable multiline entry including blank lines, mark EOF,
and protect any rows that cannot accept input. Make tab order correspond to
real editable fields. INPUT remains a shared line-mode alternative, not an
undocumented prerequisite for creating a fullscreen file.

### P2: The primary help route is stale and partly broken

HELP EDIT describes the older exchange-only SAVE AS behavior and advertises
EDIT HELP. The application interprets HELP as a source name and tries to open
it. The primary help does not explain the delivered fullscreen member saves,
END/CANCEL or entry to a cREXX development workflow.

Sources: `src/pcomm.c:832-836,864-865,877-888`,
`src/apps/edit.c:109-114`, `src/twospace_submit.c:291-294`.

Update the ordinary HELP routes alongside the UI. Recognise any advertised
command; describe supported behavior in terms a fresh operator can use.

### P2: The first screen exposes batch machinery instead of a program task

BATCH asks for DECK, PROC, SRC, OUT and PARAMS, with STEP/DD output filters on
the Submit page. These are display rows manipulated through a shared command
field, not ordinary tab-to-edit form fields. The 80-column Workbench also
retains SHELL/RECENT COMMAND and diagnostic labels around the form. Job hashes
and numeric states precede some step results, pushing LINK/RUN onto another
page. The output view initially requires step/DD knowledge.

Sources: `src/twospace_batchui.c:95-111,184-198,226-249`,
`src/twospace_submit.c:153-167`; actual `submit-ready`, both `jobs-*` and
`run-output` manual screenshots.

Provide a compact Programs surface with a selected member, Edit, Build, Run,
Arguments and Output. Use actual editable fields/row selection and truthful
focus/PF guidance. Show success/failure and the failed stage first; select the
current run's program output automatically. Keep fingerprints, raw DDs,
procedure cards and BATCH available under Details/Advanced. Preserve the
existing console and line/automation commands over the same controller.

### P2: Program arguments leak interpreter syntax

ARGS quotes its text but passes it straight into RXVM's PARM, so the current
walkthrough requires ARGS -a SHARED. A program-arguments field should not need
the user to know the VM switch.

Sources: `src/twospace_batchui.c:139-144`, `src/procs/CRXLGO.jcl:30`,
`doc/user/BATCH.md:68-70`.

The developer Arguments field and RUN command should accept application
arguments, quote once through the checked builder and supply the runtime
separator internally. Preserve the advanced raw-parameter interface.

### P2: The ordinary second build requires spool housekeeping

TJBSTART refuses another run while a completed job remains. A second build
can appear to fail before its compiler starts, with status 4 and no useful
explanation at the developer surface.

Sources: `src/twospace_job.c:85-90,144`, `doc/user/BATCH.md:37-42`.

Offer an explicit review/replace action for that selected previous result,
before submitting anything. Preserve exactly-once submission and no-replay
recovery. Do not silently purge unreviewed output, automatically rerun jobs,
or present another NEW/PREVIEW attempt as a repair.

### P2: Workspace, save feedback and capacity need a user contract

A new member can be saved in an existing supported PDS; the editor cannot
create its containing dataset. Batch output is deliberately forbidden in the
runtime RXBIN, load and procedure collections. STORE currently has eight
stored-file slots shared with other persistent state. Therefore tools alone
are not a usable workspace, and an unbounded Programs list would overpromise.
SAVE currently combines several refusals in one numeric message.

Sources: `src/apps/edit_session.c:30-62`, `src/twospace_submit.c:122-129`,
`src/twospace_store.h:16`, `doc/user/APPLICATIONS.md`.

Provision separate user source and user bytecode PDS collections, a selected
capacity budget and visible capacity/refusal feedback. Protect runtime,
procedure and tool collections. Explain changed/in-use targets and capacity
limits while retaining the truthful uncertain-publication warning for I/O12.
Full disk/extent expansion stays in PD-030; safe refusal remains required.

## The page guards

`/* editor page guard NN */` is ordinary source text inserted by
`tests/editor/probe.c:24-37` and `tests/editor/support.py:33-39`. The fixture
adds twenty-two comments to a thirteen-record program to exercise paging and
source preservation. These are not protection rules or editor controls.

Keep that deliberate fixture in tests. The normal example and primary user
walkthrough should instead create a short clean program from an empty member.
Record real editor field protection and unsaved-data safeguards separately.

## Proposed operator experience

On a prepared development profile:

```text
EDIT HELLO
    type a short program, including blank lines
    PF2 Save; PF3 Return (or END to save and return)
BUILD HELLO
RUN HELLO one two
OUTPUT
```

These short developer commands are proposed, not implemented at the baseline.
Names resolve within the configured user workspace. The existing fully
qualified dataset/member route stays available. SAVE AS remains an explicit
choice; a new-member template must not overwrite an existing program.

The same actions should be available in a 3270 Programs form, with optional
source template, selected member, visible build/run status and a plain
Arguments field. No procedure names, DD syntax, interpreter switches or JCL
preview should be required in the normal route. Advanced BATCH remains useful.

An outline of the proposed primary screen is:

```text
cREXX Programs                         Workspace: USER
Command ===>

Action  Member    Source       Build          Last run
        HELLO    Saved        Current        RC 0

N New    E Edit    B Build    R Run    O Output    D Details
PF1 Help    PF3 Console
```

Member and Arguments are actual input fields, with an unambiguous cursor and
tab order. Edit returns to the selected program. Build reports its native
stages and Run shows the selected program's output without manual STEP/DD
filter commands. The screenshot scaffold's raw revision, input owner and
capture diagnostics belong in Details/Console status rather than the normal
program view. Their underlying capture/ownership semantics remain intact.

A suggested dataset split is `PDOS.USER.CREXX` for user source,
`PDOS.USER.RXBIN` for user bytecode and `PDOS.CREXX.RXBIN` for protected runtime
libraries. These are proposal names, not a claim about an installed profile.
The image recipe supplies the real datasets and matching site configuration.
The initial supported language/profile remains explicitly cREXX Level B/TSO31.
Once handed to a user, the workspace is persistent user data: starting a
session or rebuilding tool inputs must not overwrite it. Qualification media
cleanup must not treat a real user's working disk as disposable evidence.

## Implementation order and acceptance

PD-035 owns the staged work: integrated profile, editor corrections, BUILD/RUN
controller, then direct output/status and operator rehearsal. Do not add
shortcuts on top of the Lab fixture and call that an installation.

The decisive acceptance is a fresh supported profile: create a new member
without fixture seeding, type a program with blank lines, save, BUILD, RUN with
arguments, edit/rebuild/run again, recover from a source error without running
stale output, review prior results, and restart with saved source/artifact
available. Exercise actual 3270 fields and the line route through the Mainframe
Operator after whole-change review and a new source/input freeze. Reuse the
unchanged service checks; retain concise results only.
