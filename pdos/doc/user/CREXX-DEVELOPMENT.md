# Create, build and run a cREXX program

The PD-035 development profile installs matching TSO31 RXC, RXAS, RXLINK and
RXVM, their runtime libraries, and the shared batch procedures. It is a locally
guest-qualified source candidate, separate from the public beta.3 release image.
The existing console, card reader/printer route and advanced BATCH forms remain
available.

Enter `PROGRAMS` on a 3270 terminal. Type `HELLO` in **Member**, then choose
**E** or PF2 to edit it. The editor opens the source member in the configured
workspace. Enter this program, including the blank lines:

```rexx
options levelb
import rxfnsb

main: procedure = .int
  arg args = .string[]
  i = .int
  say "Hello from PDOS"

  do i = 1 to args.0
    say args[i]
  end
  return 0
```

The cursor starts in the first source field for a new member. Type across the
source rows. A later populated row creates the intervening blank records;
there are no page guards to insert. `I` in a prefix inserts a blank after that
row and `D` deletes it. PF2 saves; `END` saves and returns to Programs. PF3
refuses to discard unsaved changes. `CANCEL` followed by Enter deliberately
discards the buffer.

If a page is refused, its typed fields remain visible. Correct the invalid
prefix or field and press Enter; accepted changes form one undo unit. Navigation
and saving wait for correction. If the terminal geometry changes, restore it
before correcting the retained page, or enter `CANCEL` to discard. A save error
leaves the buffer available and explains capacity, format, changed/busy target
or uncertain publication. Retry an uncertain SAVE to reconcile the same content.

Back in Programs:

1. Choose **B** or PF4 to build. Successful compile, assemble and link steps
   publish the executable to your separate bytecode collection.
2. Choose **O** or PF6 to review diagnostics. PF7/PF8 page through output;
   PF3 returns. Output is retained until deliberately reviewed and replaced.
3. Enter `one two` in **Arguments**, then choose **R** or PF5. RUN invokes only
   RXVM; it supplies the VM switches for you.
4. Choose **O** to read the program output. Edit, build and run again as needed.

The same controller is available from the console, including a physical 3215:

```text
EDIT HELLO
BUILD HELLO
OUTPUT
RUN HELLO one two
OUTPUT
```

`EDIT member LINE` forces the same line interface on a 3270. Bare `EDIT LINE` and `EDIT HELP` retain their editor meanings; use a full dataset(member) for members with those reserved names.

On a line terminal, EDIT uses `INPUT 0`, one source record per input line, and
`.` to finish entry; enter an empty input line for a blank source record. Then
`END` saves and exits. `PRINT` reviews source and `HELP` lists editing commands.
The qualified 3215 input route accepts printable ASCII; the source model is
UTF-8 and the native file boundary is IBM1047.

After OUTPUT has successfully read a result, the next explicit BUILD/RUN can
replace **that exact selected result**. Unreviewed output blocks another job.
The controller never automatically retries submission or silently removes
another batch job. After restart, acknowledged results are selected only when
the saved identity and immutable native step description match; review them
again. An interruption before that selection was durably saved needs advanced
`JOBS`, `OUTPUT id` and deliberate `PURGE id` recovery. This is an uncertainty
boundary, not an instruction to submit the job again.

RUN checks the saved source token, framed executable bytes and installed tool
profile against a successful build. Changing source requires BUILD. A failed
compile, changed source during a build, failed publication or storage refusal
preserves the previous executable. A tool-profile update also requires rebuilding;
source and the prior executable are retained.

This first profile has **two named programs**. Its eight journal slots cover
spool, console transcript, build work, registry and two source/executable pairs.
Cancelling an unsaved new member does not reserve a program name. Other journal
users can exhaust storage earlier; the controller reports refusal and does not
reclaim unrelated data. Source uses FB80; records wider than 80 columns are
refused on save. Executable publication is limited to 64 KiB including native
record/block framing. The Arguments field accepts up to 60 characters, subject
to the native 100-byte PARM limit and checked JCL representation.

**D Details** returns to the console with native job/step status. `BATCH`,
`JOBS`, `STATUS id`, `OUTPUT id`, `PRINT id` and explicit dataset(member) editing
remain the advanced routes. User collections and runtime libraries are separate.
This is a small single-user workspace, not ISPF or a general multiuser scheduler.
