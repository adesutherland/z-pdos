# Two-space CMS full-stage parser checkpoint, 6 October 2026

The Classic C31 `TSHVALIDATE` parser checks the v2 staged MODULE envelope,
payload FNV, zero padding, complete record framing, exact image record sizes,
CMS31 load map and ordered relocation entries with in-image targets. It does
not trust an envelope length or record count to authorize an out-of-bounds
read. Host tests compiled the same parser with C89 warnings and address and
undefined-behavior sanitizers. They accepted the unchanged CMS24 RXVM and
CMS31 RXVM, RXAS and RXC staged inputs under
`build/pdos/cms-module-recipe-c/media/` and rejected a changed payload byte.
The inputs came from the pinned Mainframe Classic Tools release archive
SHA-256 `d5e2c8bca5f90d973c01e43a81483f4827fabe534b49417ca4f5508fdef97a3a`;
their stage lock is v2. Earlier v1 stage files have no envelope FNV and are
correctly rejected by this parser.

This is a host parser result. It does not show that K read the whole dataset,
loaded a U image or ran an unchanged CMS application. Those remain PD-003
qualification gates.
