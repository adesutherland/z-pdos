# Mainframe Classic C

This component maintains the GCC 3.4.6 / cc370 compiler lineage. Read
`README.md`, `SOURCES.md`, `CHECKPOINT.md` and `ROADMAP.md` before changing it.
The inherited compiler and changes to it retain GCC's GPL terms; the root MIT
licence does not relicense them. Keep the compiler separate from the original
assembler and from the maintained PDPCLIB and linker components.

Do not import or invoke as370. The native development launcher currently
permits preprocessing, syntax checking and assembly-text generation only.
Do not enable its assemble/link path until the independent assembler accepts
the compiler output and the affected link/load and guest path is qualified.

Use `build.crexx` for the native host recipe and retain original development
automation in cREXX. Inherited configure, Makefiles and tests keep their native
interfaces. Compiler flags must preserve the Darwin ARM64 instruction-generator
calling convention repair. Do not apply old generated host config headers to a
fresh configure build.

Update source provenance and the consolidated recovery patch when changing
`source/`. Keep private archives, mail, guest material and detailed build logs
under ignored `build/` or outside the repository. Record scope accurately:
passing code-generation tests is not target execution or a qualified profile.
