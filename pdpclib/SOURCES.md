# PDPCLIB source ownership and provenance

30 September 2026. I selected this repository as the maintained home of our
PDPCLIB source and fixes. We imported the complete library from a verified
public source archive, preserving upstream notices and contributor history.
No Lab Git history, guest assets, private macro implementations, native
expansions or application binaries were imported.

## Upstream baseline

- Upstream mirror: [pdos-project/pdos](https://github.com/pdos-project/pdos).
- Exact revision: [`0fe81209e78d022b40301f86f97c7f4d3e406d0a`](https://github.com/pdos-project/pdos/tree/0fe81209e78d022b40301f86f97c7f4d3e406d0a/pdpclib).
- Archive: `https://codeload.github.com/pdos-project/pdos/tar.gz/0fe81209e78d022b40301f86f97c7f4d3e406d0a`.
- Archive SHA-256: `fe793cbf61583e42fcca3afad19bc77aed0acf06a12c230eed088023d6400c21`.
- Imported subtree: 288 files, 1,708,903 bytes before local changes.
- [Upstream file manifest](upstream.sha256): SHA-256 `deaeae93e9038108a02ce8f978332f153f47e700d468144399e05556d06deb00`.

Every file matched the archive byte for byte before the maintained fixes were
applied. The manifest identifies upstream bytes; it is not a checksum assertion
for files after local corrections. The current maintained files are source,
not generated patched copies.

The inherited [pdpclib.txt](pdpclib.txt) records Paul Edwards's public-domain
dedication and fallback permission. It has SHA-256
`01a48cd5adddf0b659c52fdcf2ba152bbe59ba97cf78902f1f7cfb0279c5885a`.
Individual source notices retain contributors including Dave Edwards, Gerhard
Postpischil, J. Reginato, Chris Langford, Dave Jones and Steve Rhoads. Keep those
notices with the files. Our MIT grant for original changes does not relicense
their material or an external IBM macro library.

## Other retained source lineage

The Lab also used the canonical SourceForge PDOS revision
`a65eddb9ef4b27a6844f2857db0c98696137612b`. Its PDPCLIB subtree has 324 files and
differs materially from this mirror. We are not combining those source trees
silently or importing its newer files. Relevant Lab C fixes were reapplied to
this selected baseline using exact unique contexts; OS configuration deltas
became named profiles. Their content hashes and scope are in [CHANGES.md](CHANGES.md).

The separate PDLD linker uses that canonical revision and its own source
record. A library notice does not clear an entire upstream repository, a later
native file or an external macro library for import.
