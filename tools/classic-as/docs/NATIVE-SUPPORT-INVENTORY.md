# PDPCLIB and PDOS native-support inventory

30 September 2026. This is a read-only review of the two stages after
`entry31.asm` in the approved [consumer sequence](PLAN.md). The assembler and
its maintained tests live here. The source paths below refer to local Lab
reference files; none of their source, private macro libraries or expansions
has been imported by this review. No source was changed, assembled, linked or
executed for this inventory.

After this inventory, I chose to maintain PDPCLIB in
[`z-pdos/pdpclib`](../../../pdpclib/README.md). Its source record and change
ledger own the subsequent import, fixes and explicit profiles. The source hashes
and supplied default switches below remain the historical inventory inputs.

## Exact baseline and notices

I use the **untouched local upstream source files and supplied PDPTOP** as the
baseline for this inventory. I do not select the prepared three-patch TSO
candidate described in [CONSUMER-INVENTORY.md](CONSUMER-INVENTORY.md). Its source
manifest would need a separate review before qualification. A target-specific
replacement PDPTOP or service-definition library is also a distinct dependency
variant, even if the principal `.asm` files remain unchanged.

All paths in this table are relative to
`/Users/adrian/CLionProjects/mainframe-lab/`.

| Input | SHA-256 | Observed notice |
| --- | --- | --- |
| `vendor/pdos/pdpclib/mvssupa.asm` | `27758af986baae46fb726fe9bb35ed993898b98d547e315587328940c2f41e31` | Lines 4–7 credit Paul Edwards and release to the public domain; lines 27–45 retain Gerhard Postpischil's changes, and later source retains contributor annotations |
| `vendor/pdos/pdpclib/pdptop.mac` | `e2a2873397bd0452e409053d1d4b91bf70ee2580ef634c379b511847afe4a843` | Lines 3–4 credit Paul Edwards and release to the public domain; the header identifies this as a customized S/380 version |
| `vendor/pdos/s370/ploadsup.asm` | `4bb345cfe1cc663177cd890add07d659323f85cd03f1e6c2bfb240ba2ad9cd98` | Lines 4–5 credit Paul Edwards and release to the public domain |
| `vendor/pdos/s370/pdossup.asm` | `bc4d429b75ae41e73fcf04e25ef7503acd764c76517e1cc2f80f1c39ea1d4505` | Lines 4–5 credit Paul Edwards and release to the public domain |

The three `.asm` hashes match the migrated consumer inventory. These are file
identities, not an independently verified upstream Git revision. The vendor
snapshot has no separate Git identity established by this review.

The file notices support reuse of this supplied material on its stated
public-domain basis; preserve the complete notices and contributor attribution
when bringing a component here and update `LICENSES.md`. The assembler's MIT
grant does not relicense these files or extend to an external macro library.
The files call IBM-named service and mapping macros, but no such macro's source,
terms or expansion has been inspected here. Those dependencies are **not cleared
for import** by the parent files' notices.

## COPY and target configuration

The only active COPY member found in these three compilation units is PDPTOP:
MVSSUPA line 556, PLOADSUP line 16 and PDOSSUP line 16. The only matching
source-owned member found in the two reviewed component directories is the
PDPCLIB `pdptop.mac` pinned above. A build must explicitly resolve and hash that
member; the statement alone does not identify a macro-library search path.

The supplied member sets these globals unconditionally:

| Switch | Supplied value | Consequence in the reviewed source |
| --- | --- | --- |
| `COMP` | `GCC` | GCC parameter-value convention in LDINT; distinct from the retained IBM C branch |
| `ZSYS` | `S380` | Application mode-switch branches emit BSM and below-line stubs |
| `XSYS` | `ZARCH` | OS branches choose extended interrupt-vector offsets and subchannel I/O |
| `ZAM64` | `YES` | Extended PSW control words select the source's AM64 case |
| `STEPD` | `YES` | Application/OS mode-switch setup remains active |
| `OUTM` | `M` | Move-mode I/O selection |
| `OS` | `UNKNOWN` | Does not select the optional MVS-specific setting |

The member also issues AMODE ANY and RMODE ANY before the principal source's
unnamed CSECT. The current seed accepts AMODE 24/31 and RMODE 24/ANY, and requires
an existing section for mode directives. Supporting this header requires
explicit pre-section mode semantics and a documented interpretation of AMODE
ANY; it cannot silently be treated as the seed's AMODE 31.

Presetting globals before COPY does not override the unconditional SETC
assignments. A narrow S/370 configuration would require a declared alternate
member or an explicit maintained configuration change, with its own hash and
byte expectations. The directory name `s370`, the application's `S380` switch,
actual community S/380 and the selected machine ISA must remain separate.

## PDPCLIB MVSSUPA semantics

The operation-field scan was used only to locate statements. Continuation cards,
macro prototypes, quoted operands and remarks were then reviewed at the points
below; token counts are not support or coverage counts.

The source contains its own traditional macros before COPY: AMUSE, GAMOS,
GAMAPP, FUNEXIT, FUNHEAD, GAM24, GAM31, LDVAL, LDADD, LDINT, QBSM, MAPSUPRM,
STVAL, DYNPAT, FIXWRITE, OSUBHEAD, OSUBRET, OPENCALL and OBRAN. Their contracts
require positional/keyword/default arguments, label substitution, argument
sublists, variable-symbol concatenation and periods, `SYSNDX` labels, local and
global character/boolean/arithmetic variables, SETA/SETB/SETC, AIF/AGO/ANOP and
MEXIT. FUNHEAD/FUNEXIT share global state across calls; this cannot be implemented
as stateless text substitution. K', N' and T' attributes and substring selection
occur in macro conditions. The macro arithmetic uses multiplication/division
and boolean/string comparisons in addition to ordinary expression arithmetic.

Assembly features needed beyond the seed include:

| Source evidence | Required meaning |
| --- | --- |
| FUNHEAD lines 212–216; MAPSUPRM lines 377–409; EQU examples at 1376, 3533 and 4788 onward | ENTRY generation; ordinary symbol length/type attributes; EQU's optional length/type operands; AL1/AL2 and expression-valued character lengths |
| DYNPAT lines 467–490 | Zero-duplication DC alignment, A and Y nominal-value lists, two-byte Y fields and service text-unit constants |
| Lines 1667–1698, 1892–1899, 2535–2536 | LTORG; unnamed CSECT restoration after DSECT; trailing comma as an empty operand; backward ORG inside emitted templates and operandless ORG restoration |
| Lines 3517–3519 and 3905–3921 | Explicit SS length zero in EX templates; length must be encoded as zero, not decremented and wrapped to 255 |
| Lines 4758–4783 and 4882–4966 | DSECT layouts, expression-valued DS lengths/repeats, DS nominal operands such as D'0', and overlay/high-water layout |
| Instructions throughout; examples CLM at 4674/4680, STCK at 3322 and BSM at 4725 | Selected instruction additions and aliases, including CLM, STCK, BSM and conditional register-branch aliases |
| MVC/CLC operands without an explicit length; L' references around 1931–1935 | Implied SS lengths and ordinary-symbol attributes, with a defined connection to the layout pass |
| Macro bodies and ordinary code with PUSH/POP USING and DROP with an empty operand | Addressability-stack save/restore and clearing the active USING set |

Binary self-defining operands such as `B'1100'`, CNOP, literal pools and
continuation cards also require support. TITLE/PRINT/SPACE must be recognized
as presentation controls without manufacturing object bytes. Hexadecimal
nominal values can contain commas (`TAPEIOB DC X'42,00,00,00'` at 4931). No direct packed P constant or
floating-point DC requirement was established by this review; DS D does not
require floating-point constant conversion. The exact selected expanded source
must close the final instruction and constant inventory.

Historical IBM assembler documentation confirms that explicit SS lengths zero
and one both produce a zero length field. This matters for the source's EX
patterns. See [IBM OS assembler length specification](https://archive.decromancer.ca/bitsavers.org/pdf/ibm/360/asm/GC28-6514-8_OS_Assembler_Language_Rel21_197201.pdf).
Zero-duplication DC emits no value while retaining alignment and attributes;
see [IBM OS/VS assembler constant rules](https://www.bitsavers.org/pdf/ibm/370/OS_VS/assembler/GC33-4010-5_OS_VS_DOS_VSE_VM_370_Assembler_Language_Mar79.pdf).

MVSSUPA has one principal unnamed CSECT restored after several internal DSECTs,
with many ENTRY identities and internal code/data labels. This review does not
establish multiple independently named emitted CSECTs. Its copied exit stubs,
work-area patterns and internal A addresses require their existing relocation
and offset relationships to survive assembly and link editing. The source also
contains unguarded optional environment-specific routines: @@ADDNUM (lines
4189–4226) calls a stated 80386-execution API through SVC 120 and emits raw
CODE386 octets; @@GETMSZ emits a raw DIAGNOSE instruction at 4238. Their
bytes can be assembled without adding an x86 encoder or executing those
services. Whole-source assembly does not qualify those environmental APIs.

ORG is not solely a DSECT feature here. CAMLST templates at CAMDUM/CAMLOC move
the location counter back into emitted data; EXCPDCB+84 writes a volume count
and CCW before operandless ORG resumes at the next available location. The
current ordered non-overlay writer cannot simply receive these events as if
they were new gaps. A future implementation needs section high-water state,
correct source-order overlapping definitions, and a deliberately qualified
emission strategy. [IBM ORG semantics](https://www.ibm.com/docs/en/hla-and-tf/1.6.0?topic=statements-org-instruction)
state that ORG changes the location counter without emitting fill bytes.

## External service and mapping dependencies

These are source-call inventories, not imported macro implementations:

- Storage/linkage: GETMAIN, FREEMAIN, SAVE, RETURN, CALL, YREGS and mode helpers.
- MVS/TSO services: OPEN/CLOSE, READ/WRITE/CHECK, GET/PUT, FIND/BLDL, NOTE/POINT,
  EXCP/WAIT/EOV/FREEPOOL, RDJFCB, OBTAIN/CAMLST/LOCATE, DEVTYPE/TRKCALC,
  GETLINE/PUTLINE, WTO/ABEND/SNAP, ATTACH/DETACH/LINK, EXTRACT/DYNALLOC/SWAREQ,
  SHOWCB/MODCB, CALLTSSR/IKJRLSA, parser macros and MODESET.
- Mapping/data definitions: DCB/DCBD, ACB/RPL/EXLST, IEFJFCBN/IECSDSL1,
  IHAPSA/CVT/IKJTCB/IEZJSCB and the further TSO, allocation, UCB, TIOT, DEB,
  PDS and VSAM mappings called near MVSSUPA lines 5037–5059.

List and execute forms are semantically different: MF=L creates parameter
structures; MF=(E,...) generates code using them. Original replacements must
preserve field widths, offsets, alignment, flags and linkage, not emit a generic
SVC for every macro. [IBM macro forms](https://www.ibm.com/docs/en/zos/3.1.0?topic=programming-forms-manipulative-macroinstruction)
provide a public interface reference; the exact target OS/edition still needs
selection before reproducing an MVS 3.x or later layout.

We can independently write register equates, the documented save/return/call
conventions, selected PSA fields, CCW records and a narrowly declared service
interface from public factual descriptions. Source-owned MAPSUPRM/DYNPAT and
manual mappings already in MVSSUPA may be reused under the source notice.
For each IBM-named replacement, retain a primary-source edition and an original
fixture with independently expected bytes or offsets. The source call's name
alone does not establish enough information to recreate its output. Unreviewed
macro libraries and existing private/native expansions remain outside this work.

## PDOS loader and kernel support

PLOADSUP is the smaller next assembly boundary: one unnamed CSECT, INITSYS entry,
register/save/return conventions, `USING PSA,R0`, mode-dependent vector writes,
LCTL, binary ICM mask, a literal pool boundary and four wait PSWs. Its unused
mapping declarations still require explicitly supplied definitions if the
unchanged source is assembled. The seed rejects USING register zero; the
consumer's absolute low-storage addressing needs separately defined semantics.

PDOSSUP adds WRBLOCK, ADISP/GOTRET, DREAD/DWRITE/DCHECK/DNOTPNT/DEXIT, control
register and DAT helpers, GETDEVN, tape and FBA output. The source switches
between SIO and MSCH/TSCH/SSCH/STSCH paths, uses LPSW/LCTL/STOSM/STNSM, and
contains a conditional LPSWE statement at line 345. Its comment explicitly says
that LPSWE is retained as an opcode record and is not currently executed. A
successful assembly must not be promoted into qualification of that path.

With XSYS=ZARCH the source writes 16-byte extended PSWs, while several wait and
dispatch instructions remain LPSW. This inventory records the source as it is;
it does not resolve that runtime architecture boundary or change its PSWs.
A real execution target and the active PSW/load path must be specified before
a boot claim.

Channel definitions require genuine object semantics:

| Construct | Required field/layout behavior |
| --- | --- |
| CCW at PDOSSUP 266–270 and CCW1 at 273–277 | Eight-byte, doubleword-aligned channel words with distinct field ordering and 24-bit versus 31-bit address limits |
| Format-0 CCW references to WRBBCCHH, WRCCHHR and WRSRCH | Three-byte address fields must carry correct section relocation; widening them to the writer's current four-byte RLD changes the command/flags layout |
| Format-1 CCW address at bytes 4–7 | Four-byte storage field with a 31-bit address constraint and separate flags/count fields |
| ORG *-4 / *-5 at 282–289 | Aliases over emitted seek/search data; DS definitions must not overwrite the original bytes or extend the section twice |

See [IBM CCW1 definition](https://www.ibm.com/docs/en/hla-and-tf/1.6.0?topic=statements-ccw1-instruction)
and [IBM format-0 field map](https://www.ibm.com/support/pages/zvm/pubs/cp740/s0ccw.html).
PLOADSUP's wait constants use absolute A words. PDOSSUP's continuation/new-SVC
PSWs additionally contain relocatable A references to code labels; preserve
the mode bits and constant addends independently of section relocation.

## Bounded qualification order and open decisions

1. Close the selected `entry31.asm` checkpoint first, with exact source/dependency
   hashes, independent bytes and relocations, then its separate link/caller gate.
2. Add only the traditional language controls demonstrated above, using original
   small fixtures and source-owned macro definitions. Qualify replay, bounded
   macro state/recursion, origin chains, expression/attribute queries, literal
   pools, CSECT restoration and ORG independently before the full MVSSUPA job.
3. Pin MVSSUPA's target configuration and the exact service-definition library.
   Assemble the selected complete source, verify exported names, template
   offsets and relocation fields, then link and exercise the required library
   services. A subset of services or an alternate member is named as a variant.
4. Qualify PLOADSUP assembly and INITSYS/vector/PSW objects before expanding
   PDOSSUP. Define USING R0 and the exact low-storage mappings, without adding
   unrelated instructions or kernel changes.
5. Add PDOSSUP's selected privileged/channel forms with independent byte tests,
   three-byte relocation and overlay coverage. Then qualify its object/link
   boundary. Source-to-boot and bounded disk/dispatch execution remain distinct
   later acceptance steps on the explicitly selected guest/profile.

Concrete open decisions are the mode/configuration member, historically
appropriate service/mapping definitions with rights and edition provenance,
ORG emission strategy, three-byte relocations and a compatible downstream
linker, and the active OS/PSW profile. The supplied default activates later
machine branches; silently discarding those branches or substituting the
prepared TSO patches would not assemble this untouched baseline. This report
supplies implementation leads and dependency boundaries, not whole-library,
whole-kernel, native-host or source-to-IPL qualification.
