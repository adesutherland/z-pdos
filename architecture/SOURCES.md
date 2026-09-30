# Architecture and format references

The implementation records factual instruction fields, encodings and object
record meanings in original code and tests. It does not bundle manuals, copied
reference tables, library macros or expansions. Public access to a reference
does not establish that all its contents are public domain.

| Reference | Use |
| --- | --- |
| IBM *System/360 Principles of Operation*, A22-6821-6, January 1967, [scan](https://www.bitsavers.org/pdf/ibm/360/princOps/A22-6821-6_360PrincOpsJan67.pdf) | Instruction formats, register and displacement widths, selected opcode facts; IBM bit numbering starts with the most significant bit |
| IBM *System/370 Principles of Operation*, GA22-7000-6, March 1980 | Selected S/370 instruction membership and operand facts; later features do not become S/360 instructions |
| IBM *System/370 Principles of Operation*, GA22-7000-7, March 1981, [scan](https://bitsavers.org/pdf/ibm/370/princOps/GA22-7000-7_IBM_System_370_Principles_of_Operation_8th_ed_198103.pdf) | Optional Branch and Save facility, BAS/BASR; the selected `s370` subset includes this facility and does not describe every S/370 machine |
| IBM *System/360 Assembler Language*, Level E/F, C28-6514-5, December 1967, [scan](https://bitsavers.trailing-edge.com/pdf/ibm/360/asm/C28-6514-5_IBM_System_360_Assembler_Language_Level_E_F_Dec67.pdf) | Fixed-card language, constants, symbols and section conventions; only the documented subset is implemented |
| IBM z/VM 7.4 [Object Module Statements](https://www.ibm.com/support/pages/zvm/pubs/cp740/objstmt.html) | Classic 80-byte ESD/TXT/RLD/END record fields, addressing modes and relocation flags |
| IBM HLASM [LTORG](https://www.ibm.com/docs/en/hla-and-tf/1.6.0?topic=statements-ltorg-instruction) and [literal pools](https://www.ibm.com/docs/en/hla-and-tf/1.6.0?topic=instruction-literal-pool) | Global collection across sections, first-section implicit END pool, eight-byte default pool boundary and descending length groups; selected F/X/V literals only |
| IBM *OS Assembler Language*, GC28-6514-9, January 1974, p30, [transcribed scan](https://manuals.plus/m/17b6feca60f26515201385a6b4d06b419dad686ab7faea35b82105a555d1ae0b) | Source SS lengths zero and one both select a zero encoded length field; source-language handling remains separate from the pure architectural encoder |
| IBM HLASM [DC duplication factor](https://www.ibm.com/docs/en/hla-and-tf/1.6.0?topic=instruction-subfield-1-duplication-factor) | A zero factor produces alignment and label semantics without a value; a nominal may be omitted; zero-duplication literals are not permitted |
| [RFC 3629](https://www.rfc-editor.org/rfc/rfc3629.html) | UTF-8 text contract; ASCII is a valid subset |

Character constants and external names select IBM CP037 explicitly. The
conversion table implements character-code correspondences; it does not use
the host's locale or translate binary output. Parser and machine names use
numeric ASCII values so a future EBCDIC C execution environment can preserve
the same internal representation. That future host remains to be qualified.
