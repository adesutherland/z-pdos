# Build the installed cREXX development profile

PD-035 uses the normal source factory with an explicitly selected four-tool
package. Product source is maintained here; native tools and matching runtime
libraries come from their owning cREXX/SDK pipeline. Guest/image work is performed
by the Mainframe Operator under the fleet lease.

Set `ZPDOS_CREXX_BATCH_PACKAGE` to the absolute batch-package directory and
`ZPDOS_CREXX_BATCH_SHA256` to the independently checked SHA256 of its owning
`batch-inputs.sha256`. Then invoke the ordinary `image.crexx` source recipe.
It selects the 150-cylinder batch K profile and calls `developer-image.crexx`,
which installs the four tools and applications, runtime PDS, common FB80 PROCs,
SITE files, and empty user collections. No public beta download, test probes,
fixture aliases or private Lab writer is used on this route.

For development qualification with already built inputs, the operator can use:

```text
crexx -nokeep pdos/scripts/developer-image.crexx --args \
  build/pdos/developer/image BARE_IMAGE CORE COMMAND APPS \
  BATCH_PACKAGE MANIFEST_SHA256 HERCULES_BIN
```

Use the same stable target for repairs. Check processes, leases and consumers
before removing an owned failed factory output/input directory in place. The
factory refuses an existing destination or developer input directory. **A
handed-over workspace is persistent user data:** start its existing configuration
and disk on subsequent sessions. Do not run the image factory or batch cleanup
against it. Tool/configuration updates go through the operator and preserve
PDOS.STORE, both user collections, WORK and STATE. Install a matching tool/runtime/
PROC set and update DEV.SITE's PROFILE fingerprint together. The registry binds
workspace names independently of that fingerprint, so EDIT remains available and
RUN requires rebuilding against the new profile. There is no automatic in-place
installer or managed-instance adoption in this candidate.

The maintained Hercules template selects one z/Architecture CPU, 512 MiB,
IBM1047 boundaries, model-2 primary 3270, a 3215 console, 3505 card input and
1403 printer output. The operator assigns actual ports, device ownership and
model/line variants through the fleet procedure. Startup never recreates the disk.

The stopped media adapter verifies all eight package payloads against the
explicit manifest, uses the maintained native packer, reads back installed
native modules, initializes only new empty PDS directories, and compares the
compressed CKD records. Flat disks and duplicate factory inputs are temporary.
`developer.json` records input/profile identities and capacity, not an archive
of terminal evidence. Never stage over an existing user disk as a factory retry.

Run focused host checks with:

```text
crexx -nokeep pdos/scripts/developer-check.crexx
crexx -nokeep pdos/scripts/batch-jcl-check.crexx
python3 -B pdos/tests/developer_media.py
```

The first checks exercise the actual editor, developer controller, converter,
job core and Programs field codecs with mocked native file/job execution.
The native compiler, assembler and linker builds remain separate from physical
3270/3215 and persistence qualification. Review the complete change, resolve
findings, record exact source/input freeze and the bounded matrix, and only then
ask the operator to run guests. Retain summaries and requested manual screenshots.
