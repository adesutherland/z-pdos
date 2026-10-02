# z/PDOS source origin

Paul Edwards created PDOS. Our maintained z/PDOS descends from his canonical
[SourceForge PDOS repository](https://sourceforge.net/p/pdos/gitcode/ci/a65eddb9ef4b27a6844f2857db0c98696137612b/tree/),
revision a65eddb9ef4b27a6844f2857db0c98696137612b. The verified whole-source
archive pdos-sourceforge-a65eddb9.tar.gz had SHA-256
4bed44b93fbb09f0f40442e29ab6073ced049a1ce4659a341cf89eb16f17ad93.

archive/upstream-a65eddb9.tar.gz freezes the 37 selected original OS files and
archive/upstream-description.txt preserves the upstream description. The
upstream name s370 remains inside that frozen reference; the maintained
implementation is z/PDOS in src/. Archives are outside normal builds/tests.
Their actual checksums are in archive/SHA256SUMS; inherited source notices
and terms remain with the source. [LICENSE](LICENSE) scopes original work.

The repaired PDIO1 source originally qualified in commit
d62b109ed986bd455732484173ff4ebe0533045a incorporated multi-CSECT loader,
native-build, full-width context, native-service, high-loading, checked-write
and console-wrapping repairs. Those original patch identities and the earlier
recovery recipes remain in Git history at fd7df814fff5b181fb18ab30de31872c77404d2c;
they are not the current source-maintenance or build interface.

PDPCLIB owns the shared runtime source. Its canonical runtime merge, preserved
MVS repairs and current source modules are described in
[the library source record](../pdpclib/UPSTREAM.md). OS builds select pdos-zarch
and stage current source below ignored build/. They record current source
identities rather than requiring source to match an old recovery manifest.
The old write/console failures remain small deliberate test fixtures; positive
checks extract the actual current OS functions. No second OS/runtime tree is
maintained in source control.

[The exact 0.1 qualification](doc/qualification/QUALIFICATION.md) and its JSON
retain the original source/tool/image identities and guest results unchanged.
The earlier report's differently recorded outer archive identity is historical;
we verified the selected upstream members against the 4bed44... archive.
No private disk, native object/listing, correspondence or application package
was imported. Future source changes are ordinary src/ edits and Git commits.
