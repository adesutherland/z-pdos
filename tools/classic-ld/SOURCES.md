# Source record

The inherited implementation is PDLD from the public
[PDOS SourceForge repository](https://sourceforge.net/p/pdos/gitcode/), Git
remote `https://git.code.sf.net/p/pdos/gitcode`, at revision
`a65eddb9ef4b27a6844f2857db0c98696137612b`. Its original source notices and
`readme.txt` declare the source and documentation public domain.

The import contains all 76 files in that revision's `pdld/` directory: 67
source files, including 39 C translation units, and nine upstream top-level
files. No private prepared source is the authority for the import.
`UPSTREAM.sha256` records the pristine bytes. The canonical retained archive
`pdos-sourceforge-a65eddb9.tar.gz` has SHA-256
`4bed44b93fbb09f0f40442e29ab6073ced049a1ce4659a341cf89eb16f17ad93`.
All 76 members were also compared byte for byte with the separately retained
`pdos-a65eddb9.tar.gz`, SHA-256
`b8d1cdaaa1c23a4c8335ddd81e3e32a787bc1191032ae73d4825c6e6bd736827`.
The archives themselves remain outside the product repository.

Five existing Mainframe SDK fixes are consolidated in the maintained source.
Their exact patch files remain in `provenance/`; `SDK-BASE.sha256` records the
complete 76-file result before the new origin repair and product naming.
That result was independently compared with the retained SDK-prepared tree.

| Patch | Maintained behavior | SHA-256 |
| --- | --- | --- |
| `0001-pdld-only-multi-csect.patch` | Multiple MVS/XMIT control sections, matching written alignment and chunk records | `c37a36ea6683295d6a219192050c053729e96f9253bd66adf2e4504fe800ea02` |
| `0003-xmit-card-images.patch` | Finish XMIT physical output on an 80-byte card boundary | `469175c52775840f292d32477b630ac58e39485160d99541158e005f224afee7` |
| `0004-xmit-source-date-epoch.patch` | Validated `SOURCE_DATE_EPOCH` and UTC reproducible timestamp | `36365be5c1879676edafc4a74532313df4ba90819f8473537d626b6e04603f16` |
| `0005-section-relative-rld.patch` | Output RLD positions relative to their source CESD | `0a6affa11f8677861509ac4878da018fdb1346000499337e759164386ef6720e` |
| `0006-zero-extended-csect.patch` | Define the enlarged SD tail as zero-filled | `3e44df1009d0f3d99e605cd35172f637a98a049063777ac12408620cff0b31f9` |

The maintained origin repair is original project material in `src/mainframe.c`
and `src/ld.h`. Product command identification changes `src/libld.c`.
The new build recipe, tests and component documentation are original MIT
material; the licence does not relicense inherited code. Existing file-level
notices are preserved, including Paul Edwards's credits in the character
encoding routines. Original changes are identified separately.

The relocation design uses published object-format facts:

- [IBM HLASM external symbol dictionary listing](https://www.ibm.com/docs/en/hla-and-tf/1.6.0?topic=listing-external-symbol-dictionary-esd):
  SD and LD addresses are assembled addresses, and PC carries its beginning
  address.
- [IBM HLASM relocation dictionary listing](https://www.ibm.com/docs/en/hla-and-tf/1.6.0?topic=listing-relocation-dictionary-rld):
  the relocated field has an assembled address and an add/subtract action.
- [IBM OS Linkage Editor F PLM, Y28-6667-0, Release 15, January 1968](https://bitsavers.org/pdf/ibm/360/os/R15-16_May68/plm/Y28-6667-0_Linkage_Editor_F_Rel15_PLM_Jan68.pdf),
  printed page 36: relative A relocation applies the difference between
  assigned and original origins; external absolute relocation uses the assigned
  symbol value.
- [IBM DOS System Control and Service, GC24-5036-6, Release 25, April 1971](https://bitsavers.org/pdf/ibm/360/dos/GC24-5036-6_DOS_System_Control_and_Service_Release_25_Apr1971.pdf),
  printed page 96: TXT contains the assembled address and SD/PC contain original
  section origins.

The original test cards and byte expectations are in `tests/test_origin.c`.
No inherited assembler implementation, opcode table, service macro expansion,
native reference object or guest asset is imported for these tests.
