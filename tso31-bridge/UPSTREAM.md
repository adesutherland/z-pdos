# TSO31 bridge source origin

The entry bridge and ABI are original project-authored material by Adrian
Sutherland, extracted under MIT for the first Classic Assembler consumer.
The exact original reference has SHA-256
ad226ed2c1dd647fa22e532dc5f40662295d9b90b20e20ee1ae7d0af3fc983c8
and remains frozen in archive/reference-entry31.asm.

src/entry31.asm is the maintained version, with the recorded nine explicit
service replacements and six EQU definitions. The adapter source and its
contract are maintained through normal Git changes. Required tests use the
237 normalized original statement expectations in tests/fixtures and the
independent deck/ABI checks; they do not read the archive.

No IBM macro definitions, native expansions or service implementations are
included. Public interface facts are cited in doc/architecture/SERVICES.md.
The exact host/link checkpoint is in doc/qualification/CHECKPOINT.md.
