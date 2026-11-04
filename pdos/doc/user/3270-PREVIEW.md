# z/PDOS 0.2.2: 3270 cREXX development preview

Paul Edwards created PDOS. This preview of the maintained z/PDOS fork adds
the fullscreen source editor, common Programs/Source/Build/Run/Output frame,
native build progress and the shared card-reader/printer batch service.
It includes matching October-qualified TSO31 RXC, RXAS, RXLINK, RXVM and
RXBIN libraries. See the component notices in `notices/`.

Extract the entire archive into a directory you own. Run Hercules there:

```text
hercules -f hercules.cnf
```

Connect a 3270 client to `0009@127.0.0.1:3270`, selecting model 5
(27 rows by 132 columns) and IBM1047 before IPL. Connect the optional
line console as `000A@127.0.0.1:3270`. At the Hercules console:

```text
ipl 01b9
```

In DX3270's connection dialog use Host `127.0.0.1` and Port `3270` separately;
do not include `0009@` in Host. The sole configured 3270 device selects 0009.
The `0009@host:port` notation above is suitable for c3270.

At the PDOS command prompt enter `PROGRAMS`. The supplied user workspace is
empty. Follow **WALKTHROUGH.pdf** to create HELLO in the fullscreen editor,
save, build, review output and run it. The equivalent **WALKTHROUGH.md** and
actual terminal screenshots are included in the archive.

The client must support standard 3270 Read Buffer. The walkthrough identifies
the tested client; older DX3270 1.7.5 build 1 lacks that support. PF keys and
Reset/Clear mappings depend on your client. Model 2 is also supported; select
the desired model before IPL. Changing models live is a known limitation.

Keep this working disk between sessions: saved programs, executables and
reviewed results persist. EXIT from the PDOS console commits state and reports
K SHUTDOWN. After that, stop and quit Hercules normally. Never copy or replace
a live writable disk. Verify the extracted files with SHA256SUMS before first
use; the original downloadable archive remains your clean installation input.

This bounded development profile admits two program members and FB80 source
records. A wide display still saves at most 80 bytes per source record. The
machine uses one z/Architecture CPU and 512 MiB. Live cross-model negotiation,
general scheduling, wider filesystem capacity and complete 3270-family
conformance remain outside this preview. The release page records the exact
operator qualification and image identity.

The 3505 reader uses `reader.cards`; the 1403 printer writes `printer.txt`.
The line console and advanced BATCH/SUBMIT/JOBS/OUTPUT/PRINT routes remain
available alongside the forms. The image includes no IBM guest system,
private laboratory disks, credentials or saved user programs.
