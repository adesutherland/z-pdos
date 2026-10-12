# ADDRESS prerequisite: native PDS output member review

The stopped PDOS TSO qualification wrote RXC/RXAS output under journal member
keys beginning with the generated DD-name tail (`0018MFAD`, `0020MFAD`). Input
FIND for `MFADDB` therefore read the empty physical placeholder. The assembler
accepted that empty input, and linking failed. This is a PDOS native-file defect,
separate from cREXX allocation, the SDK heap size and terminal-controller RSS.

The selected SDK's actual maintained MVSSUPA source and object are unchanged.
A host-only constant CSECT added to a copy of that source derives these values
from assembler symbols: OPENCLOS-IHADCB = 140; ZDDN-IHADCB = 280;
ZMEM-IHADCB = 288; ZMEM-OPENCLOS = 148. The old kernel displacement was 144.
It selected four DD-name bytes followed by four member bytes, matching the
stopped journal exactly. The extra CSECT is never executed in a guest.

`twospace_native_file_abi.h` owns the selected 148-byte contract. PDS output
OPEN uses that constant through the existing checked U-span copy and retains
owned cleanup on invalid input. An empty leading byte is rejected alongside
the existing blank-member check. Sequential datasets and CMS file names do
not use this private TSO work-area offset. This does not establish a generic
ABI for arbitrary vendor native service modules.

The review checks the probe's unique 16-byte ESD/TXT section, complete text
coverage, symbol relationships and explicit kernel-contract match. The complete
current SDK source assembles. The `tso31-lean` source preparation succeeds, but
its complete assembly is rejected for an unsupported VSAM bootstrap feature;
that route is not additional qualification for this selected SDK build.

Before full ADDRESS replay, `native_pds_roundtrip.c` writes distinct binary
patterns to two similarly named eight-character members, closes and reopens
both, replaces the first and verifies the second is unchanged. It checks all
700 bytes, EOF and close results through newlib and the actual MVSSUPA service.
The fresh stopped STORE must contain the exact requested member names and
payloads. Only that focused guest proof admits the expensive affected replay.

Source/core, actual native service inputs, fixture delivery and stopped media
are frozen separately. The runtime's 32-record and PDOS fixture's 23-record
amendments remain explicit inputs. Normal IPL, owned release, caller/resource
recovery and unchanged data outside STORE remain required. No managed image
is modified by this disposable qualification.

## Pre-guest evidence

The selected `tso31-sdk-files-v14` MVSSUPA source has SHA-256
`458c3f8f3ce426bc470c160eb160dfa40b5092153651ed239610c45dfa31a67c`;
its original object has SHA-256
`58efd091aa78765cc4b0f9b00002c96c181bb4be353022c40b0e69b1d783e5c3`
and matches the selected SDK service object exactly. The host-only proof object
has SHA-256
`aac0318baa2677d30b35a5fd44348b3a87095fc8ca9097ae009f3fdfabbf40c1`.
The eight-record symbol/contract proof manifest in
`build/pdos/address-member-layout-reviewed-20261012` passes strict verification;
its SHA-256 is
`87e9bf38a9797bf1ceb80fbb344d1289c5eefe058427985e6e751a8ceebaf7c2`.
The explicit 144-byte negative contract is rejected while preserving the
assembler-derived 148-byte value in the negative-control output.

The retained `build-native-pds-roundtrip.crexx` invokes the existing SDK
consumer with maintained command adapters and the selected source-built
runtime archives, over a verified base SDK. The resulting native service
object must equal the selected campaign service. Before/after source hashes
match. The independently read IOFLT31 member is AMODE31/RMODEANY, 89,186 framed
bytes, SHA-256
`58e3109e5a98365c46c60a80112c1297e15de0e896cadfe4eb457fe5fb89b10e`.
Its 29-record inputs/outputs manifest passes strict verification, SHA-256
`484706db61d0f1291aa5e4ead69c137f0c851dea191a3b57c771e5af51f65d93`.
These checks prove the selected host packaging; they do not prove guest I/O.
