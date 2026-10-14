"""Binary fixture interface: fresh named data and bounded empty PDS staging."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

TRACK = 56832

def records(raw, c, h):
    start = 512+(c*15+h)*TRACK
    if raw[start:start+5] != b"\0"+struct.pack(">HH", c, h):
        raise ValueError("CKD track header")
    at = start+5
    result = {}
    while raw[at:at+8] != b"\xff"*8:
        cc, hh, n, kl, dl = struct.unpack_from(">HHBBH", raw, at)
        if (cc, hh) != (c, h) or n in result or at+8+kl+dl > start+TRACK:
            raise ValueError("CKD record bound")
        result[n] = (bytes(raw[at+8:at+8+kl]), bytes(raw[at+8+kl:at+8+kl+dl]), at)
        at += 8+kl+dl
    return result

def track(raw, c, h, items):
    out = bytearray(b"\0"+struct.pack(">HH", c, h))
    for n, (key, data) in sorted(items.items()):
        out += struct.pack(">HHBBH", c, h, n, len(key), len(data))+key+data
    out += b"\xff"*8
    if len(out) > TRACK:
        raise ValueError("fixture exceeds CKD track")
    at = 512+(c*15+h)*TRACK
    raw[at:at+TRACK] = out.ljust(TRACK, b"\0")

def prepare(media):
    (media/"DD-FB.bin").write_bytes(bytes(range(64)))
    (media/"DD-GUARD.bin").write_bytes(b"PD033-GUARD-KEEP!")
    entries = ["PD3.FB SEQ DD-FB.bin TRK 1 1 0 PS FB 32 64",
               "PD3.GUARD SEQ DD-GUARD.bin TRK 1 1 0 PS F 16 16",
               "PD3.OUT EMPTY TRK 1 1 0 PS FB 32 64",
               "PD3.LIB EMPTY CYL 1 1 0 PO VB 20 64 1",
               "PD3.FBLIB EMPTY CYL 1 1 0 PO FB 32 32 1"]
    ctl = media/"ctl.txt"
    source = ctl.read_text().splitlines()
    if any(x.startswith("PD3.") for x in source):
        raise ValueError("fresh DD fixture required")
    index = next(i for i, x in enumerate(source) if x.startswith("PDOS.STORE "))
    ctl.write_text("\n".join(source[:index]+entries+source[index:])+"\n")

def stage(work, herc):
    disk = work/"media/pdos00.cckd"
    flat = work/"dd-fixture.ckd"
    subprocess.run([str(herc/"cckd2ckd"), "-q", "-cyls", "100", str(disk), str(flat)], check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    raw = bytearray(flat.read_bytes())
    before = bytes(raw)
    allowed = set()
    for dsn, member in (("PD3.LIB", False), ("PD3.FBLIB", True)):
        found = []
        for c in (1, 2):
            for h in range(15):
                for key, data, at in records(raw, c, h).values():
                    if key == dsn.ljust(44).encode("cp037"):
                        full = key+data
                        if len(full) != 140 or full[44] != 0xf1:
                            raise ValueError("fixture DSCB")
                        found.append((full, at+8))
        if len(found) != 1:
            raise ValueError("unique fixture dataset required")
        full, address = found[0]
        c, h, ec, eh = struct.unpack_from(">4H", full, 107)
        if full[82:84] != b"\x02\0" or c*15+h+1 > ec*15+eh:
            raise ValueError("bounded PDS extent")
        initial = records(raw, c, h)
        if any(any(data) for n, (_, data, _) in initial.items() if n):
            raise ValueError("only empty private PDS may be initialized")
        body = ("INPUT".ljust(8).encode("cp037")+b"\0\x01\x01\0") if member else b""
        body += b"\xff"*8+b"\0"*4
        directory = (struct.pack(">H", 2+len(body))+body).ljust(256, b"\0")
        track(raw, c, h, {0: initial[0][:2], 1: (b"\xff"*8, directory), 2: (b"", b"")})
        allowed.update(range(512+(c*15+h)*TRACK, 512+(c*15+h+1)*TRACK))
        lstar = b"\0\0\x02"
        if member:
            mc, mh = divmod(c*15+h+1, 15)
            old = records(raw, mc, mh)
            if any(any(data) for n, (_, data, _) in old.items() if n):
                raise ValueError("member fixture track not empty")
            track(raw, mc, mh, {0: old[0][:2], 1: (b"", bytes(range(96, 128))), 2: (b"", b"")})
            allowed.update(range(512+(mc*15+mh)*TRACK, 512+(mc*15+mh+1)*TRACK))
            lstar = b"\0\x01\x02"
        raw[address+98:address+101] = lstar
        allowed.update(range(address+98, address+101))
    if any(a != b and i not in allowed for i, (a, b) in enumerate(zip(before, raw))):
        raise ValueError("unrelated fixture bytes changed")
    flat.write_bytes(raw)
    output = work/"dd-fixture.cckd"
    subprocess.run([str(herc/"ckd2cckd"), "-q", "-bz2", "-cyls", "100", str(flat), str(output)], check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    output.replace(disk)
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"two-space"))
    from store_check import inspect
    report = inspect(flat)
    (work/"store-before.json").write_text(json.dumps(report, indent=2)+"\n")
    (work/"dd-fixture.json").write_text(json.dumps({"input_sha256": hashlib.sha256(before).hexdigest(),
        "flat_sha256": hashlib.sha256(raw).hexdigest(), "disk_sha256": hashlib.sha256(disk.read_bytes()).hexdigest(),
        "datasets": ["PD3.FB", "PD3.GUARD", "PD3.OUT", "PD3.LIB", "PD3.FBLIB"],
        "guard_hex": b"PD033-GUARD-KEEP!".hex(), "physical_fb_hex": bytes(range(64)).hex(),
        "physical_member_hex": bytes(range(96, 128)).hex()}, indent=2)+"\n")
    flat.unlink()

if __name__ == "__main__":
    if sys.argv[1] == "prepare":
        prepare(Path(sys.argv[2]))
    elif sys.argv[1] == "stage":
        stage(Path(sys.argv[2]), Path(sys.argv[3]))
    else:
        raise ValueError("selected fixture phase required")
