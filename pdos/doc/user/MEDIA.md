# Exchange disks and tape records

This guide describes the shared media commands used by z/PDOS 0.2 K/U and
the retained one-space producer. The [P6 record](../qualification/TWO-SPACE-P6-2026-10-07.md)
qualifies their normal K/U route. Use a disposable image
and keep the original disk and distribution tapes unchanged. A Hercules
attachment is a host action; `MOUNT` and `TAPE MOUNT` register an already
attached device inside z/PDOS.
Stop the guest before changing a Hercules disk or tape image.

## Exchange CKD volume

Attach a separately prepared 3390 CKD image at a free address. It needs a
real `VOL1` label, a six-character volume serial and an ordinary VTOC. For
example, a Hercules configuration can add:

```text
01BA 3390 exchange.cckd
```

At the PCOMM prompt:

```text
DEVICES
VOLUMES
MOUNT 01BA EXCH01
VOLUMES
SELECT EXCH01
DIR
TYPE PROBE.DAT
OTHER -C VERSION
SELECT PDOS00
UNMOUNT EXCH01
```

`DEVICES` lists attached addresses; an address remains `unclassified` until
registered. `MOUNT` checks the volume's actual `VOL1` serial and rejects an
absent address, mismatch or duplicate. `SELECT` changes dataset and native
executable lookup, and the prompt follows the selected serial. PCOMM prints
numbered `BEGIN` and `END ... RC=` lines for the media commands. A nonzero RC
means the command did not complete.

The IPL volume is selected at boot and cannot be unmounted. Up to four CKD
volumes, including IPL, can be registered. Unmount additional volumes in
reverse order after selecting another volume; an open DD or active PDS write
prevents unmount. Native DDs bind to their device at allocation, so selecting
another volume does not redirect an already open DD.

The operator can create a sequential output dataset on the selected exchange
disk, then use a native program or `COPY` to write it:

```text
SELECT EXCH01
ALLOC OUT.DAT FB 80 80 1
COPY PROBE.DAT OUT.DAT
```

`ALLOC name FB|VB|U lrecl blksize cylinders` reports the chosen extent and a
return code. The first allocator deliberately accepts only a 100-cylinder
`dasdload` 3390 exchange layout with a two-cylinder VTOC and no format-5 free
map. It allocates 1–16 contiguous cylinders, refuses the IPL disk, duplicate
names, invalid geometry and damaged or full VTOCs, and initializes EOF before
publishing a format-1 DSCB. Use `FB` or `VB` for the qualified fixture route;
`U` is admitted but has not been exercised by that route.

For an exact FB/VB logical-record copy, use `RCOPY source target` after
`ALLOC`. It checks matching geometry and an empty destination, then preserves
each physical block, including an isolated zero-length VB record. The current
bounded command accepts separate one-cylinder PS datasets whose data and EOF
fit on the first track. Ordinary `COPY` is a byte-stream command; it does not
preserve an isolated empty VB record through the current C library.

For a simple fixed 80-byte input fixture, a `dasdload` control can include:

```text
EXCH01 3390-1 100
SYSVTOC VTOC CYL 2
PROBE.DAT SEQ probe.dat CYL 1 1 0 PS F 80 80
```

After the guest exits and Hercules stops, extract `OUT.DAT` from the stopped
image and compare its logical record bytes with the expected fixture. Do not
infer a successful export from console text alone. The exchange volume uses
z/OS-style CKD datasets; it does not turn a CMS minidisk into a z/PDOS disk.
The [fixture guide](FIXTURES.md) covers checked CMS and TSO tape imports,
guest commands and exact output export.

## Raw virtual tape

Attach an unchanged HET or AWS tape image as a Hercules 3420 device, read-only
for incoming media. Keep output on a separate writable image. For example:

```text
0560 3420 input.het RO
0561 3420 output.het
```

Register one tape at a time, then explicitly rewind before reading from the
start:

```text
TAPE MOUNT 0560
TAPE STATUS
TAPE REWIND
TAPE READ
TAPE SCAN
TAPE OFF
```

`TAPE READ` consumes one physical record and prints its byte length, FNV-1a
32-bit digest and first 16 bytes in hex. A zero-length result marks a tape
file boundary. `TAPE SCAN` consumes records to the next file mark and reports
the record and byte totals. It stops at 100,000 records or 512 MiB. The raw
I/O buffer is 32,767 bytes per record. Check an input tape's SHA-256 on the
host and compare guest observations with a host tape map or extracted records.

For a separate initialized output image, register it explicitly writable:

```text
TAPE MOUNT 0561 WRITE
TAPE REWIND
TAPE WRITE 00010203FEFF
TAPE MARK
TAPE MARK
TAPE REWIND
TAPE READ
TAPE OFF
```

`TAPE WRITE` takes 1–40 bytes as even-length hex. It and `TAPE MARK` reject a
read-only registration. Stop Hercules before extracting the output tape and
compare its exact records and file marks with the expected bytes. `TAPE OFF`
only releases the z/PDOS registration; it does not detach the host image.

Native allocation accepts `TAP:0560` only for the registered tape address.
A short native read through PCOMM `COPY TAP:0561 OUT.DAT` also passed guest and
stopped-disk byte checks. The reverse `COPY PROBE.DAT TAP:0561` wrote one
80-byte record to a separate writable image and passed stopped-tape readback.
The K/U P6 native COPY round trip also preserves one 160-byte physical tape
record as two FB80 logical disk records. Larger application records need
their own qualification. The inherited
`TAV:` path is not admitted by this route.

Tape transport preserves the original distribution image. A CMS VMFPLC2 HET
and a z/OS standard-label AWS have different logical layouts. These commands
report physical records and file marks. The [fixture recipe](FIXTURES.md)
decodes a bounded logical subset on the host and verifies z/PDOS output
records after guest shutdown. It does not make CMS MODULEs executable.

Native CMS/TSO application output and transcripts use banked `PDOS.STORE`
on the IPL disk, even when an exchange volume is selected. Native sequential
COPY and RCOPY to exchange datasets write their physical target. Verify the
stopped store and physical media separately.

The [P6 record](../qualification/TWO-SPACE-P6-2026-10-07.md) states the current
K/U results; [phase 2](../qualification/PHASE2-2026-10-04.md) retains the earlier
one-space media proof.
