# Run a native program conformance script

This is the current source's bounded PDIO1 operator route. It creates a new
100-cylinder 3390 candidate from an untouched base image, installs checked
native load streams and optional simple VB PDS members, builds `CONFORM.BAT`,
and checks the stopped disk after compression. A leased 3270 run then produces
one PASS/FAIL receipt with each command, expected and actual return code, and
required output text. The published 0.1.0 image does not contain the new PCOMM
result markers, console continuation or screen repair; build a current image
with the [source image recipe](README.md#build-a-fresh-disk-from-source) first.

## Inputs

Work from the repository root. Give `prepare` an **absolute** stopped base
CCKD path, an absolute UTF-8 JSON manifest path, a fresh `build/pdos/` output
directory and the absolute directory containing Hercules `dasdload`,
`cckd2ckd` and `ckd2cckd`. Input files must exist and carry their exact
SHA-256 in the manifest. The base has the standard `PLOAD.SYS`, `PDOS.SYS`,
`CONFIG.SYS`, `COMMAND.EXE` layout. The installer never edits the base.

The manifest has these arrays:

| Key | Required fields | Meaning |
| --- | --- | --- |
| `programs` | `name`, absolute `file`, `sha256`, `cylinders` | Native RDW stream installed unchanged as `NAME.EXE`, fixed 18452-byte physical blocks; 1–16 cylinders. |
| `datasets` | `name`, `kind`, `organization`, `record_format`, `lrecl`, `blksize`, `cylinders` | `kind` is `empty`; this creates the output/fixture dataset before checked member installation. |
| `members` | `dataset`, `member`, `kind`, absolute `file`, `sha256` | Optional `binary` or UTF-8 `text` member in a manifest-created empty PO/VB dataset. Binary records split at `LRECL−4`; text lines become IBM1047 logical records. |
| `checks` | `id`, `command`, `expected_rc`, `output_contains` | Ordered external commands; each output phrase must appear during that command. Optional `responses` is an ordered list of `{ "prompt": "...", "text": "..." }` pairs for terminal input. |

`programs` and `checks` must be nonempty. A check command must name a program
in `programs`. Commands must be at most 198 IBM1047 bytes; checks and members
are bounded to one native batch and a simple one-block PDS directory. An
`output_contains` phrase must differ from the command text. The supported PDS
subset uses single extents, ordinary members and VB records. Supply the
application's *actual* loader companions, library and fixture datasets; a
TSO-style launcher or XMIT file alone does not make a CMS MODULE runnable.
Use an explicit empty `output_contains` list for a command judged by its
return code and separate output readback. A phrase may appear in different
checks: the judge searches only between that check's numbered `PCOMM BEGIN`
and `PCOMM END` markers.

A small manifest shape (replace paths, hashes and package-specific names):

```json
{
  "programs": [
    {"name":"RXVM","file":"/absolute/package/RXVM.rdw","sha256":"64 lowercase hex digits","cylinders":8}
  ],
  "datasets": [
    {"name":"APP.RXBIN","kind":"empty","organization":"PO","record_format":"VB","lrecl":255,"blksize":27998,"cylinders":8}
  ],
  "members": [
    {"dataset":"APP.RXBIN","member":"HELLO","kind":"binary","file":"/absolute/package/HELLO.rxbin","sha256":"64 lowercase hex digits"}
  ],
  "checks": [
    {"id":"HELLO","command":"RXVM -l APP HELLO","expected_rc":0,"output_contains":["HELLO RESULT"]}
  ]
}
```

The example shows the schema, not a qualified RXVM package. Use the actual
package's command and companion load modules, and hash each file with
`shasum -a 256` before filling the manifest. A response may be an empty string;
prompt and response text currently cannot contain tab, quote or backslash.

## Prepare and run

```sh
crexx -nokeep pdos/scripts/conformance.crexx --args prepare \
  /absolute/base/pdos00.cckd /absolute/package/checks.json \
  build/pdos/my-candidate /absolute/hercules/bin
```

Preparation refuses an existing output directory. It records input/output
hashes, exact native batch bytes, a locked manifest, logs and CKD readback in
that directory. `candidate.cckd` is the only disk to boot. Keep the original
base unmounted and unchanged. If this is a shared guest, follow its operator's
lease and version procedure; do not copy or edit a mounted image. For a
standalone instance, copy `candidate.cckd` as `pdos00.cckd` into a disposable
Hercules directory with the standard configuration, connect a 3270 terminal,
then IPL `01B9`.

For an automated console, launch `s3270` with a script port and the correct
terminal/codepage. The script port is a local controller port, separate from
the Hercules 3270 listener. For example, with a local listener on 3270:

```sh
s3270 -model 3278-2 -codepage cp1047 -scriptport 3498 0009@127.0.0.1:3270
```

At a **freshly booted** PCOMM prompt, before other external commands, run:

```sh
crexx -nokeep pdos/scripts/conformance.crexx --args run \
  3498 build/pdos/my-candidate build/pdos/my-result
```

The result directory must be new. `run` refuses a visible prior PCOMM result or
completion marker, captures a fresh `console.trace`, submits `CONFORM`, sends
responses only after their prompts, waits for completion and an unlocked
prompt, then writes `result.json` and `judge.log`. Each check carries its ID,
command, expected/actual RC, missing output phrases and PASS/FAIL; the receipt
also hashes the manifest, batch and trace. A nonzero recipe exit or `FAIL`
needs investigation. A timeout preserves the guest and trace; inspect them
before rerunning. Use a freshly booted candidate for a second run.

After recording results, end PCOMM with `EXIT`, observe the Hercules wait at
`0444`, stop/quit Hercules normally and read the stopped disk if checking
guest-written files. Remove only this disposable candidate/result and its
temporary managed version when they are no longer needed. Keep the base and
the accepted managed version intact.

## Scope

The installer preserves exact incoming native bytes and checks logical PDS
member records. It does not provide general TSO or CMS allocation commands,
CMS MODULE execution, arbitrary PDS layouts, or fixture export through this
native conformance route. The separate checked CMS exchange-disk and
`CMS CHECK`/`CMS RUN` path is described in the
[stage 3 record](../qualification/STAGE3-2026-10-05.md).
