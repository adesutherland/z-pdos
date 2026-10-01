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
| IBM HLASM [LTORG](https://www.ibm.com/docs/en/hla-and-tf/1.6.0?topic=statements-ltorg-instruction) and [literal pools](https://www.ibm.com/docs/en/hla-and-tf/1.6.0?topic=instruction-literal-pool) | Global collection across sections, first-section implicit END pool, eight-byte default pool boundary and descending length groups; selected H/F/X/XL/A/V literals only |
| IBM *OS Assembler Language*, GC28-6514-9, January 1974, p30, [transcribed scan](https://manuals.plus/m/17b6feca60f26515201385a6b4d06b419dad686ab7faea35b82105a555d1ae0b) | Source SS lengths zero and one both select a zero encoded length field; source-language handling remains separate from the pure architectural encoder |
| IBM HLASM [DC duplication factor](https://www.ibm.com/docs/en/hla-and-tf/1.6.0?topic=instruction-subfield-1-duplication-factor) | A zero factor produces alignment and label semantics without a value; a nominal may be omitted; zero-duplication literals are not permitted |
| IBM HLASM [COPY](https://www.ibm.com/docs/en/hla-and-tf/1.6.0?topic=statements-copy-instruction) | Library member insertion, distinct from a macro call; definition-time COPY remains outside the first resolver slice |
| IBM HLASM [scalar declarations](https://www.ibm.com/docs/en/hla-and-tf/1.6?topic=symbols-lcla-lclb-lclc-instructions), [SETA](https://www.ibm.com/docs/en/hla-and-tf/1.6.0?topic=symbols-seta-instruction) and [conditional language](https://www.ibm.com/docs/en/hla-and-tf/1.6.0?topic=reference-macro-conditional-assembly-language-summary) | Selected scoped scalar, signed 32-bit arithmetic and conditional branch semantics; documented provider subset only |
| IBM z/VM [AMODE and RMODE](https://www.ibm.com/docs/en/zvm/7.2.0?topic=modes-amode-rmode-instructions) and HLASM [RMODE](https://www.ibm.com/docs/en/hla-and-tf/1.6.0?topic=statements-rmode-instruction) | Named association anywhere in source; blank association requires an actual unnamed section and does not create it |
| IBM HLASM [ENTRY](https://www.ibm.com/docs/en/hla-and-tf/1.6.0?topic=statements-entry-instruction), Language Reference SC26-4940-09, chapter 5 page 169 | A V nominal may reference an ENTRY in the same source module; explicit EXTRN excludes local definition. Separate ER and LD/SD identities preserve linker resolution and V semantics. |
| IBM *z/Architecture Principles of Operation*, SA22-7832-06, February 2008, [public PDF](https://www.ibm.com/docs/en/SSQ2R2_15.0.0/com.ibm.tpf.toolkit.hlasm.doc/dz9zr006.pdf), p7-78 | CLM RS opcode BD, mask 0..15 and unsigned 12-bit displacement; original encoder and independent expected vectors |
| IBM *z/Architecture Principles of Operation*, SA22-7832-14, [public PDF](https://www.ibm.com/docs/en/module_1678991624569/pdf/SA22-7832-14.pdf?cp=HW11W), chapter 18 instruction descriptions, and the S/360/S/370 editions above | Reached HFP RR/RX opcode and operand facts. The selected historical register subset remains 0,2,4,6; no AFP register facility is enabled. |
| [RFC 3629](https://www.rfc-editor.org/rfc/rfc3629.html) | UTF-8 text contract; ASCII is a valid subset |

Character constants and external names select IBM CP037 explicitly. The
conversion table implements character-code correspondences; it does not use
the host's locale or translate binary output. Parser and machine names use
numeric ASCII values so a future EBCDIC C execution environment can preserve
the same internal representation. That future host remains to be qualified.
