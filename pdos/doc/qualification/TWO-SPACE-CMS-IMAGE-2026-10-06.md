# Two-space CMS image materialization checkpoint, 6 October 2026

`TSHIMAGE` validates the complete v2 staged MODULE before writing an image.
It checks the supplied capacity and placement: CMS24 stays at its fixed
origin, while CMS31 uses an aligned base wholly below 2 GiB. It copies each
framed image record and applies the checked CMS31 relocation entries to the
new base. The entry address is derived from the selected base and MODULE
header. The caller supplies distinct destination storage; this API does not
allocate U pages.

The C89 host check, with address and undefined-behavior sanitizers, accepted
the pinned CMS24 RXVM and CMS31 RXVM, RXAS and RXC v2 stages. It rejected
wrong fixed origin, undersized destination, changed payload and invalid
header controls; a CMS31 relocation word matched its source word rebased to
`0x03000000`. The Mainframe Classic C31 compile and the source-built
diskless machine gate passed with the same source.

This is loader-component evidence. K has not mapped a MODULE image in U or
entered unchanged CMS code under the successor; native CMS-to-CMS calls, TSO
applications and the normal replacement image remain open.
