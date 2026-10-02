# PDPCLIB source origin and contributors

Paul Edwards created the Public Domain C Library. Individual source notices
also retain contributors including Dave Edwards, Gerhard Postpischil,
J. Reginato, Chris Langford, Dave Jones and Steve Rhoads. Keep those notices
with their source. [LICENSE](LICENSE) preserves the inherited dedication and
fallback permission; our original MIT grant does not relicense their work.

The first complete import was the 288-file PDPCLIB subtree of
[pdos-project/pdos](https://github.com/pdos-project/pdos/tree/0fe81209e78d022b40301f86f97c7f4d3e406d0a/pdpclib),
revision 0fe81209e78d022b40301f86f97c7f4d3e406d0a. The downloaded whole-source
archive SHA-256 was fe793cbf61583e42fcca3afad19bc77aed0acf06a12c230eed088023d6400c21.
Its 288 members were verified before original maintained changes. The frozen
library subtree is archive/upstream-0fe81209.tar.gz; its own checksum is in
archive/SHA256SUMS and the original member manifest is in archive/.

On 1 October we reconciled the relevant runtime changes from Paul Edwards's
canonical SourceForge PDOS revision a65eddb9ef4b27a6844f2857db0c98696137612b
into this same maintained library. Eighteen differing C/header/notice files
adopted canonical bytes; canonical MVSSUPA was combined with the existing
MVS fixes, and the w+b write-handle initialization path was corrected before
this reorganisation. [The change record](doc/development/CHANGES.md) owns the
integration details. The original selected 44-file runtime remains in Git
commit d62b109ed986bd455732484173ff4ebe0533045a.

src/ owns the complete maintained C, headers, native support, source-owned
macros, explicit interfaces and service configurations. MVSSUPA is factored
into common and profile-specific source modules in src/native/mvssupa/;
src/profiles/*/mvssupa.inputs specifies the normal source selection. Joining
these modules reproduces all five pre-reorganisation variants byte for byte;
no patch is applied. Shared fixes are direct source changes and Git commits.

The frozen upstream archive retains historical build recipes and examples.
Retained implementation ports in src/ are not all qualified here. A later
canonical VSE file had a recorded copyright question and was not imported by
the runtime merge; retain each actual imported file's notices and review them
before a replacement or new port. External IBM macro calls do not include or
license those macro implementations. No native expansions or private assets
were imported. The source version text 4.xx is not a new release 4.00.
