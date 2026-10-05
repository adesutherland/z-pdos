# Checked CMS and TSO fixtures

The 0.1.1 source recipe can move a bounded set of logical tape fixtures onto
a disposable 3390 exchange volume and export exact guest results. Keep the
release ZIP and its tape images unchanged. The ZIP member bytes must equal the
extracted tape image, and the manifest pins both SHA-256 values.

## Supported input

| Source medium | Supported logical form | Destination |
| --- | --- | --- |
| CMS VMFPLC2 HET | Named `filename filetype A1`, fixed records through `F lrecl B` or variable records through `V S`; Hercules `vmfplc2` performs the extraction | PS FB or VB on a new exchange CKD volume |
| z/OS standard-label AWS | One `VOL1`/`HDR1`/`HDR2` fixed-record dataset, data file, `EOF1`/`EOF2`, and final tape marks; HDR2 geometry must match the manifest | PS FB on a new exchange CKD volume |

CMS file name/type/mode and TSO dataset names remain explicit in the
manifest. Neither tape format is a z/PDOS disk format. The adapter preserves
the logical bytes; `encoding: "ibm1047"` identifies text for the operator,
while `encoding: "binary"` means opaque bytes. There is no implicit trimming,
newline conversion or binary-to-text conversion. CMS variable files use
two-byte length framing at the `vmfplc2` interface; z/PDOS VB uses BDW/RDW
blocks. The recipe verifies logical records across that framing change.

The current disk staging and `RCOPY` command are bounded to one-cylinder
sequential extents whose contents and EOF fit on the first track. They reject
other geometry. Other CMS tape layouts, multi-file z/OS labels, compressed
standard-label tapes, PDS members and large files need separate adapters.

## Manifest

Use absolute paths for `archive.file` and each input `tape`. Map every
fixture ID to its tape member; several fixtures may use one tape. This
example shows the fields;
replace the paths and zero hashes with real values:

```json
{
  "format": "pdos-fixtures-v1",
  "volume_serial": "FIXT01",
  "archive": {
    "file": "/absolute/path/release.zip",
    "sha256": "0000000000000000000000000000000000000000000000000000000000000000",
    "members": {"SOURCE": "cms/input.het"}
  },
  "inputs": [{
    "id": "SOURCE",
    "medium": "cms-vmfplc2-het",
    "tape": "/absolute/path/input.het",
    "tape_sha256": "0000000000000000000000000000000000000000000000000000000000000000",
    "source": {"name": "IOBAD", "type": "CREXX", "mode": "A1", "recfm": "V"},
    "target": "SOURCE.IN",
    "recfm": "VB", "lrecl": 80, "blksize": 800,
    "encoding": "ibm1047",
    "records_sha256": "0000000000000000000000000000000000000000000000000000000000000000",
    "payload_sha256": "0000000000000000000000000000000000000000000000000000000000000000"
  }],
  "outputs": [{
    "target": "SOURCE.OUT", "same_as": "SOURCE",
    "recfm": "VB", "lrecl": 80, "blksize": 800,
    "encoding": "ibm1047"
  }]
}
```

For an AWS input, use `medium: "zos-sl-aws"` and a source such as
`{"dataset":"TSO.FIXTURE","volser":"FTX001"}`. Use FB geometry matching
HDR2. `records_sha256` covers each record prefixed by an unsigned four-byte
big-endian length. `payload_sha256` covers the records concatenated without
framing. Thus an empty logical record differs from an empty file even though
their payload hash is the same. A mismatch stops preparation before there is
an exchange candidate.

The draft may use the zero hashes shown above. Inspect the unchanged ZIP and
tapes to create a **new** pinned manifest with the measured archive, tape,
record and byte hashes:

```text
crexx -nokeep pdos/scripts/fixtures.crexx --args inspect /absolute/path/draft.json /absolute/path/pinned.json /absolute/path/HERCULES_BIN
```

`inspect` checks that each ZIP member equals its extracted tape, verifies the
named logical file and its geometry, and prints record counts and hashes.
Review the pinned manifest, then use it for `prepare`. `prepare` checks every
hash again; it never silently refreshes a changed tape or ZIP.

## Operator sequence

Run from the z-pdos repository root. `HERCULES_BIN` is the absolute directory
containing the Hercules disk utilities and `vmfplc2`:

```text
crexx -nokeep pdos/scripts/fixtures.crexx --args prepare /absolute/path/pinned.json build/pdos/fixture-run /absolute/path/HERCULES_BIN
```

The recipe validates archive and tape hashes, decodes logical records, builds
a fresh 100-cylinder exchange image, reads its contents back, and prints the
path to `exchange.cckd`. With the guest stopped, attach a copy of that image
at `01BA` and boot the checked z/PDOS source image. Use the shared guest lease
procedure when operating a managed guest. Connect a leased 3270 console with
an `s3270` script port, then run:

```text
crexx -nokeep pdos/scripts/fixtures.crexx --args run SCRIPT_PORT build/pdos/fixture-run build/pdos/fixture-console
```

`run` requires a fresh IPL prompt. It executes the checked command list:
mount and select the exchange disk, allocate each output dataset, `RCOPY`
each input to its output, return to IPL and unmount. Each command must finish
with `PCOMM END ... RC=0`; the console trace stays in the run directory.
`ALLOC` creates the outputs on the guest. A native TSO-style program may be
run against the selected exchange volume where its binary and services are
already qualified; [program conformance](CONFORMANCE.md) owns that test.
Unchanged CMS MODULE execution uses the separate checked
[CMS route](README.md#running-applications); it is not part of this fixture
copy script.

Exit PCOMM normally, stop Hercules, and run the export check against the
stopped exchange image:

```text
crexx -nokeep pdos/scripts/fixtures.crexx --args verify /absolute/path/stopped-exchange.cckd build/pdos/fixture-run build/pdos/fixture-export /absolute/path/HERCULES_BIN
```

`verify` requires a fresh export directory. It checks that every input
dataset stayed unchanged and every output has the expected ordered logical
records and exact bytes. It writes `.records` (four-byte length-framed),
`.bin` (concatenated bytes), and `receipt.json` with hashes. Console success
alone is insufficient for acceptance.

The [phase 2 result](../qualification/PHASE2-2026-10-04.md) records the
guest and stopped-media proof and its limits.
