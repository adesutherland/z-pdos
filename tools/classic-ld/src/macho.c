/******************************************************************************
 * @file            macho.c
 *
 * Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish, use, compile, sell and
 * distribute this work and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions, without
 * complying with any conditions and by any means.
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "ld.h"
#include "bytearray.h"
#include "macho.h"
#include "xmalloc.h"

#include "macho_bytearray.h"

#if defined (NO_LONG_LONG) && ((ULONG_MAX >> 16) >> 16) < 0xffffffff
#define NO_64_BIT_INT
#endif

#define PAGE_SIZE 0x4000
#define NUM_SEGS 4

#define FLOOR_TO(to_floor, floor) ((to_floor) / (floor) * (floor))

#if 0
#define section_in_text_seg(section) ((section)->flags & SECTION_FLAG_READONLY)
#define section_in_data_seg(section) (!((section)->flags & SECTION_FLAG_READONLY))
#else
/* __TEXT is always readonly,
 * so chained fixups cannot
 * be applied to it.
 * Ideally __const
 * and other readonly sections
 * which need chained fixups
 * would be in __DATA_CONST
 * which would be later turned readonly
 * but this is simpler.
 */
#define section_in_text_seg(section) ((section)->flags & SECTION_FLAG_CODE)
#define section_in_data_seg(section) (!((section)->flags & SECTION_FLAG_CODE))
#endif

/* New format is position independent (PIE)
 * (but GOT can be avoided)
 * and contains many commands.
 * The old format is position dependent
 * (so it must be loaded on the desired base address)
 * and has only LC_SEGMENT_64s,
 * LC_SYMTAB and LC_UNIXTHREAD commands.
 * Both old and new format can be used for x64
 * while ARM64 requires the new format.
 */
static int new_format = 1;

/* First version for ARM64. */
#define MINIMUM_ARM64_MINOS_VERSION 0x000b0000LU /* "11.0.0" */
#define DEFAULT_ARM64_MINOS_VERSION MINIMUM_ARM64_MINOS_VERSION
/* First version with chained fixups. */
#define DEFAULT_X64_MINOS_VERSION 0x000a0800LU /* "10.8.0" */
static unsigned long minos_version;
#define USE_BUILD_VERSION ((minos_version) > 0x000a0800LU)

#define SDK_VERSION 0x000d0300LU /* "13.3.0" */

static struct section *first_data_section;
static address_type first_data_section_alignment;

static int check_cputype (unsigned long cputype, const char *filename)
{
    switch (cputype) {
        case CPU_TYPE_x86_64:
            if (ld_state->target_machine == LD_TARGET_MACHINE_X64
                || ld_state->target_machine == LD_TARGET_MACHINE_UNKNOWN) {
                ld_state->target_machine = LD_TARGET_MACHINE_X64;
            } else {
                ld_error ("%s: cputype is not CPU_TYPE_x86_64", filename);
                return 1;
            }
            break;
        
        case CPU_TYPE_ARM64:
            if (ld_state->target_machine == LD_TARGET_MACHINE_AARCH64
                || ld_state->target_machine == LD_TARGET_MACHINE_UNKNOWN) {
                ld_state->target_machine = LD_TARGET_MACHINE_AARCH64;
            } else {
                ld_error ("%s: cputype is not CPU_TYPE_ARM64", filename);
                return 1;
            }
            break;

        default:
            ld_error ("%s: unrecognized cputype %#lx", filename, cputype);
            return 1;
    }

    return 0;
}

static unsigned long get_cputype (void)
{
    switch (ld_state->target_machine) {
        case LD_TARGET_MACHINE_X64: return CPU_TYPE_x86_64;
        case LD_TARGET_MACHINE_AARCH64: return CPU_TYPE_ARM64;
        default: return 0;
    }
}

address_type macho_get_base_address (void)
{
#if !(defined (NO_LONG_LONG) && ((ULONG_MAX >> 16) >> 16) < 0xffffffff)
    return 0x100000000;
#else
    return 0;
#endif
}

address_type macho_get_first_section_rva (void)
{
    return PAGE_SIZE;
}

void macho_before_link (void)
{
    /* Mach-O groups sections into segments,
     * so they need to be sorted according
     * to which segment they belong to.
     */
    struct section *section, *next_section;
    struct section *text_sections = NULL;
    struct section **last_text_section_p = &text_sections;
    struct section *data_sections = NULL;
    struct section **last_data_section_p = &data_sections;
    struct section *bss_sections = NULL;
    struct section **last_bss_section_p = &bss_sections;

    for (section = all_sections; section; section = next_section) {
        next_section = section->next;
        section->next = NULL;
        if (section_in_text_seg (section)) {
            *last_text_section_p = section;
            last_text_section_p = &section->next;
        } else if (!section->is_bss) {
            *last_data_section_p = section;
            last_data_section_p = &section->next;
        } else {
            *last_bss_section_p = section;
            last_bss_section_p = &section->next;
        }
    }
    *last_text_section_p = data_sections;
    if (data_sections) {
        *last_data_section_p = bss_sections;
    } else {
        *last_text_section_p = bss_sections;
    }
    all_sections = text_sections;

    first_data_section = *last_text_section_p;
    if (first_data_section) {
        first_data_section_alignment = first_data_section->section_alignment;
        first_data_section->section_alignment = PAGE_SIZE;
    }
}

static int log_base2 (address_type val)
{
    int ret;

    if (!val) return 0;

    for (ret = 0, val >>= 1; val; val >>= 1, ret++) {}

    return ret;
}

static size_t num_function_starts_symbols;
static address_type *function_starts_addresses_p;

static void function_starts_symbol_callback (struct symbol *symbol)
{
    if (!symbol->part || !(symbol->part->section->flags & SECTION_FLAG_CODE)) return;
    num_function_starts_symbols++;
}

static void function_starts_symbol_callback2 (struct symbol *symbol)
{
    if (!symbol->part || !(symbol->part->section->flags & SECTION_FLAG_CODE)) return;
    *function_starts_addresses_p = symbol_get_value_no_base (symbol);
    function_starts_addresses_p++;
}

static int function_starts_address_compare (const void *a, const void *b)
{
    if (*(const address_type *)a < *(const address_type *)b) return -1;
    if (*(const address_type *)a > *(const address_type *)b) return 1;
    return 0;
}

struct part_reloc {
    struct section_part *part;
    struct reloc_entry *reloc_entry;
};

static int part_reloc_compare (const void *a, const void *b)
{
    const struct part_reloc *apr, *bpr;
    address_type arva, brva;

    apr = a;
    bpr = b;
    arva = apr->part->rva + apr->reloc_entry->offset;
    brva = bpr->part->rva + bpr->reloc_entry->offset;

    if (arva < brva) return -1;
    if (arva > brva) return 1;
    return 0;
}

struct chained_fixup_starts {
    unsigned short *page_starts;
    unsigned short page_count;
};

static void calculate_chained_fixups (struct chained_fixup_starts *cfs,
                                      int doing_data)
{
    /* As the relocations are not sorted by RVA in object files,
     * they must be sorted now.
     */
    struct section *section;
    struct section_part *part;
    struct part_reloc *part_rels, *p;
    size_t i;
    size_t num_relocs = 0;
    address_type seg_rva = 0;

    if (doing_data) {
        for (section = all_sections; section; section = section->next) {
            if (section_in_data_seg (section)) {
                seg_rva = section->rva;
                break;
            }
        }
    }

    for (section = all_sections; section; section = section->next) {
        if ((doing_data && !section_in_data_seg (section))
            || (!doing_data && !section_in_text_seg (section))) continue;
        
        for (part = section->first_part; part; part = part->next) {
            const struct reloc_entry *relocs;
            
            relocs = part->relocation_array;
            for (i = 0; i < part->relocation_count; i++) {
                if (relocs[i].howto != &reloc_howtos[RELOC_TYPE_64]) continue;
                
                num_relocs++;
            }
        }
    }

    if (!num_relocs) {
        part_rels = NULL;
        return;
    }

    p = part_rels = xmalloc (sizeof *part_rels * num_relocs);

    for (section = all_sections; section; section = section->next) {
        if ((doing_data && !section_in_data_seg (section))
            || (!doing_data && !section_in_text_seg (section))) continue;

        for (part = section->first_part; part; part = part->next) {
            struct reloc_entry *relocs;
            
            relocs = part->relocation_array;
            for (i = 0; i < part->relocation_count; i++) {
                if (relocs[i].howto != &reloc_howtos[RELOC_TYPE_64]) continue;
                
                p->part = part;
                p->reloc_entry = &relocs[i];
                p++;
            }
        }
    }

    qsort (part_rels, num_relocs, sizeof *part_rels, &part_reloc_compare);
    {
        address_type max_rva;

        max_rva = part_rels[num_relocs - 1].part->rva + part_rels[num_relocs - 1].reloc_entry->offset;
        max_rva = ALIGN (max_rva, PAGE_SIZE);
        cfs->page_count = (max_rva - seg_rva) / PAGE_SIZE;
    }
    cfs->page_starts = xmalloc (sizeof *(cfs->page_starts) * cfs->page_count);

    for (i = 0, p = part_rels;
         (i < cfs->page_count) && (p < part_rels + num_relocs);
         i++) {
        address_type p_rva = p->part->rva + p->reloc_entry->offset;

        if (seg_rva + (i + 1) * PAGE_SIZE <= p_rva) {
            cfs->page_starts[i] = DYLD_CHAINED_PTR_START_NONE;
            continue;
        }
        
        cfs->page_starts[i] = p_rva & (PAGE_SIZE - 1);
        for (; p < part_rels + num_relocs; p++) {
            address_type result;

            {
                uint_fast64_t field8;
                bytearray_read_8_bytes (&field8, p->part->content + p->reloc_entry->offset, LITTLE_ENDIAN);
                result = field8;
            }

            result -= ld_state->base_address;
#ifndef NO_64_BIT_INT
            result &= 0xfffffffff;
            if (p + 1 != part_rels + num_relocs) {
                address_type rva1, rva2;
                rva1 = p->part->rva + p->reloc_entry->offset;
                rva2 = p[1].part->rva + p[1].reloc_entry->offset;

                if (FLOOR_TO (rva1, PAGE_SIZE) != FLOOR_TO (rva2, PAGE_SIZE)) {
                    bytearray_write_8_bytes (p->part->content + p->reloc_entry->offset,
                                             result,
                                             LITTLE_ENDIAN);
                    p++;
                    break;
                }

                result |= (((rva2 - rva1) / 4) & 0xfff) << 51;
            }
#endif
            
            bytearray_write_8_bytes (p->part->content + p->reloc_entry->offset, result, LITTLE_ENDIAN);
        }
    }

    free (part_rels);
}

void macho_write (const char *filename)
{
    FILE *outfile;
    unsigned char *file;
    size_t file_size;
    unsigned char *pos;
    unsigned char *content;
    size_t content_offset;

    struct section *section;

    unsigned char *function_starts = NULL;
    size_t function_starts_size = 0;

    struct mach_header_64_internal hdr64 = {0};
    struct chained_fixup_starts chained_fixups[NUM_SEGS] = {{NULL}};
    unsigned long symoff = 0;
    uint_fast64_t data_segment_offset = 0;

    if (first_data_section) {
        first_data_section->section_alignment = first_data_section_alignment;
    }

    if (!(outfile = fopen (filename, "wb"))) {
        ld_error ("cannot open '%s' for writing", filename);
        return;
    }

    file_size = SIZEOF_struct_mach_header_64_file;
    
    hdr64.magic = MH_MAGIC_64;
    hdr64.cputype = get_cputype ();
    if (hdr64.cputype == CPU_TYPE_x86_64) {
        hdr64.cpusubtype = CPU_SUBTYPE_I386_ALL;
    }
    hdr64.filetype = MH_EXECUTE;
    if (new_format) {
        hdr64.flags = MH_NOUNDEFS | MH_DYLDLINK | MH_TWOLEVEL | MH_PIE;
    } else {
        hdr64.flags = MH_NOUNDEFS;
    }

    if (!minos_version) {
        if (hdr64.cputype == CPU_TYPE_ARM64) {
            minos_version = DEFAULT_ARM64_MINOS_VERSION;
        } else {
            minos_version = DEFAULT_X64_MINOS_VERSION;
        }
    } else if (hdr64.cputype == CPU_TYPE_ARM64
               && minos_version < MINIMUM_ARM64_MINOS_VERSION) {
        ld_warn ("minimum Mach-O minos version for ARM64 is %lu.%lu.%lu",
                 (MINIMUM_ARM64_MINOS_VERSION >> 16) & 0xffff,
                 (MINIMUM_ARM64_MINOS_VERSION >> 8) & 0xff,
                 MINIMUM_ARM64_MINOS_VERSION & 0xff);
        minos_version = MINIMUM_ARM64_MINOS_VERSION;
    }
    
    hdr64.ncmds++;
    hdr64.sizeofcmds += SIZEOF_struct_segment_command_64_file;

    hdr64.ncmds++;
    hdr64.sizeofcmds += SIZEOF_struct_segment_command_64_file;
    for (section = all_sections; section; section = section->next) {
        if (section_in_text_seg (section)) {
            hdr64.sizeofcmds += SIZEOF_struct_section_64_file;
        }
    }

    hdr64.ncmds++;
    hdr64.sizeofcmds += SIZEOF_struct_segment_command_64_file;
    for (section = all_sections; section; section = section->next) {
        if (section_in_data_seg (section)) {
            hdr64.sizeofcmds += SIZEOF_struct_section_64_file;
        }
    }

    hdr64.ncmds++;
    hdr64.sizeofcmds += SIZEOF_struct_segment_command_64_file;

    if (new_format) {
        hdr64.ncmds++;
        hdr64.sizeofcmds += SIZEOF_struct_dyld_chained_fixups_command_file;

        hdr64.ncmds++;
        hdr64.sizeofcmds += SIZEOF_struct_dyld_exports_trie_command_file;

#define DYLIB_STRING "/usr/lib/libSystem.B.dylib"
        hdr64.ncmds++;
        hdr64.sizeofcmds += ALIGN (SIZEOF_struct_dylib_command_file + sizeof (DYLIB_STRING), 8);

        hdr64.ncmds++;
        hdr64.sizeofcmds += SIZEOF_struct_function_starts_command_file;

        /* DYSYMTAB references SYMTAB and DYSYMTAB is required, so SYMTAB is required too. */
        hdr64.ncmds++;
        hdr64.sizeofcmds += SIZEOF_struct_symtab_command_file;

        hdr64.ncmds++;
        hdr64.sizeofcmds += SIZEOF_struct_dysymtab_command_file;

        hdr64.ncmds++;
        if (USE_BUILD_VERSION) {
            hdr64.sizeofcmds += SIZEOF_struct_build_version_command_file;
        } else {
            hdr64.sizeofcmds += SIZEOF_struct_version_min_command_file;
        }

#define DYLINKER_STRING "/usr/lib/dyld"
        hdr64.ncmds++;
        hdr64.sizeofcmds += ALIGN (SIZEOF_struct_dylinker_command_file + sizeof (DYLINKER_STRING), 8);

        hdr64.ncmds++;
        hdr64.sizeofcmds += SIZEOF_struct_source_version_command_file;

        hdr64.ncmds++;
        hdr64.sizeofcmds += SIZEOF_struct_data_in_code_command_file;

        hdr64.ncmds++;
        hdr64.sizeofcmds += SIZEOF_struct_main_command_file;
    } else {
        hdr64.ncmds++;
        hdr64.sizeofcmds += SIZEOF_struct_symtab_command_file;

        hdr64.ncmds++;
        hdr64.sizeofcmds += (SIZEOF_struct_unixthread_command_file
                             + SIZEOF_struct_x86_thread_state64_t_file);
    }

    file_size += hdr64.sizeofcmds;

    /* Some empty space between commands and contents is needed. */
    file_size = all_sections ? all_sections->rva : PAGE_SIZE;
    content_offset = file_size;
    for (section = all_sections; section; section = section->next) {
        if (section_in_text_seg (section)) {
            if (!section->is_bss) {
                /* The segments are loaded into memory whole
                 * and the loader does not individually load and align sections,
                 * so (section_rva - segment_rva) == (section_offset - segment_offset)
                 * must be true (except for bss sections).
                 */
                file_size = ALIGN (file_size, section->section_alignment);
                file_size += section->total_size;
            }
        }
    }
    file_size = ALIGN (file_size, PAGE_SIZE);
    for (section = all_sections; section; section = section->next) {
        if (section_in_data_seg (section)) {
            if (!section->is_bss) {
                file_size = ALIGN (file_size, section->section_alignment);
                file_size += section->total_size;
            }
        }
    }
    file_size = ALIGN (file_size, PAGE_SIZE);

    if (new_format) {
        {
            /* dyld_chained_fixups. */
            file_size += SIZEOF_struct_dyld_chained_fixups_header_file;
            file_size += SIZEOF_struct_dyld_chained_starts_in_image_file + (NUM_SEGS - 1) * 4 + 4;

#ifdef GENERATE_TEXT_CHAINED_FIXUPS
            /* Currently gives "zsh: bus error"
             * when output executable produced
             * from Mach-O input is run.
             */
            calculate_chained_fixups (&chained_fixups[1], 0);
            if (chained_fixups[1].page_count) {
                file_size += SIZEOF_struct_dyld_chained_starts_in_segment_file;
                file_size += chained_fixups[1].page_count * 2 - 2;
                file_size = ALIGN (file_size, 4);
            }
#endif

            calculate_chained_fixups (&chained_fixups[2], 1);
            if (chained_fixups[2].page_count) {
                file_size += SIZEOF_struct_dyld_chained_starts_in_segment_file;
                file_size += chained_fixups[2].page_count * 2 - 2;
                file_size = ALIGN (file_size, 4);
            }

            file_size += SIZEOF_struct_dyld_chained_starts_in_image_file;
        }

        {
            /* dyld_exports_trie, just copied from working executable. */
            file_size += 0x980;
        }

        {
            /* LC_FUNCTION_STARTS contains addresses of symbols in code sections
             * in special format:
             *  first entry/address is ULEB128 encoded offset from base address (RVA)
             *  all following entries/addresses are ULEB128 encoded offset from the previous entry
             *  (current RVA - previous RVA)
             * This means the addresses must be sorted in ascending order.
             */
            num_function_starts_symbols = 0;
            symbols_for_each_global (&function_starts_symbol_callback);
            {
                size_t i;
                unsigned char *p;
                address_type prev_rva;
                address_type *addresses = xmalloc (sizeof *addresses * num_function_starts_symbols);

                function_starts_addresses_p = addresses;
                symbols_for_each_global (&function_starts_symbol_callback2);
                qsort (addresses, num_function_starts_symbols, sizeof *addresses, &function_starts_address_compare);

                /* Excessive but quick estimate
                 * of how much space can function_starts take up in the worst case.
                 * + 7 bytes so the function starts size
                 * can be rounded up to multiple of 8.
                 */
                function_starts = xmalloc (num_function_starts_symbols
                                           * (sizeof (address_type) * CHAR_BIT / 7 + 1)
                                           + 7);
                prev_rva = 0;
                p = function_starts;
                for (i = 0; i < num_function_starts_symbols; i++) {
                    address_type diff = addresses[i] - prev_rva;
                    
                    while (diff) {
                        *p = diff & 0x7f;
                        diff >>= 7;
                        if (diff) *p |= 0x80;
                        p++;
                    }

                    prev_rva = addresses[i];
                }
                /* It is necessary to pad function starts
                 * to multiple of 8 to allow aligned access
                 * to content of commands following after function starts
                 * (for example, one OS version requires aligned symbol table).
                 * ULEB128 0 does not mean another function start
                 * and is allowed as padding.
                 */
                memset (p, 0, 7);
                p = function_starts + ALIGN (p - function_starts, 8);
                file_size += function_starts_size = p - function_starts;

                free (addresses);
            }
        }

        {
#define MH_HEADER_SYMBOL_NAME "__mh_execute_header"
            file_size += SIZEOF_struct_nlist_64_file;
            file_size += 4 + sizeof MH_HEADER_SYMBOL_NAME;
        }
    }

    file = xmalloc (file_size);
    memset (file, 0, file_size);

    pos = file;
    content = file + content_offset;

    write_struct_mach_header_64 (pos, &hdr64);
    pos += SIZEOF_struct_mach_header_64_file;

    {
        struct segment_command_64_internal seg;
        unsigned char *saved_pos;

        seg.cmd = LC_SEGMENT_64;
        seg.cmdsize = SIZEOF_struct_segment_command_64_file;
        memset (seg.segname, 0, sizeof (seg.segname));
        strcpy (seg.segname, "__PAGEZERO");
        seg.vmaddr = 0;
        seg.vmsize = ld_state->base_address;
        seg.fileoff = 0;
        seg.filesize = 0;
        seg.maxprot = 0;
        seg.initprot = 0;
        seg.nsects = 0;
        seg.flags = 0;
        write_struct_segment_command_64 (pos, &seg);
        pos += SIZEOF_struct_segment_command_64_file;

        seg.cmd = LC_SEGMENT_64;
        seg.cmdsize = SIZEOF_struct_segment_command_64_file;
        memset (seg.segname, 0, sizeof (seg.segname));
        strcpy (seg.segname, "__TEXT");
        seg.vmaddr = ld_state->base_address;
        seg.vmsize = 0;
        seg.fileoff = 0;
        seg.filesize = 0;
        seg.maxprot = PROT_READ | PROT_EXECUTE;
        seg.initprot = PROT_READ | PROT_EXECUTE;
        seg.nsects = 0;
        seg.flags = 0;
        saved_pos = pos;
        pos += SIZEOF_struct_segment_command_64_file;
        
        for (section = all_sections; section; section = section->next) {
            if (section_in_text_seg (section)) {
                struct section_64_internal sect = {{0}};
                
                seg.cmdsize += SIZEOF_struct_section_64_file;
                seg.nsects++;

                memset (sect.sectname, 0, sizeof (sect.sectname));
                memcpy (sect.sectname,
                        section->name,
                        (strlen (section->name) >= sizeof (sect.sectname))
                        ? sizeof (sect.sectname)
                        : strlen (section->name));
                if (sect.sectname[0] == '.') {
                    /* Seems "." prefix causes problems
                     * and "__" must always be used as prefix.
                     */
                    memmove (sect.sectname + 2, sect.sectname + 1, sizeof (sect.sectname) - 2);
                    sect.sectname[0] = sect.sectname[1] = '_';
                }
                memset (sect.segname, 0, sizeof (sect.segname));
                strcpy (sect.segname, "__TEXT");
                sect.addr = ld_state->base_address + section->rva;
                sect.size = section->total_size;
                sect.align = log_base2 (section->section_alignment);
                if (section->flags & SECTION_FLAG_CODE) {
                    sect.flags = 0x80000000 | S_REGULAR;
                } else if (strcmp (section->name, "__cstring") == 0) {
                    sect.flags = S_CSTRING_LITERALS;
                } else {
                    sect.flags = 0;
                }

                seg.vmsize = ld_state->base_address + section->rva + section->total_size - seg.vmaddr;
                if (!section->is_bss) {
                    content = file + ALIGN (content - file, section->section_alignment);
                    sect.offset = content - file;
                    section_write (section, content);
                    content += section->total_size;
                } else {
                    sect.offset = 0;
                }

                write_struct_section_64 (pos, &sect);
                pos += SIZEOF_struct_section_64_file;
            }
        }
        content = file + ALIGN (content - file, PAGE_SIZE);
        seg.vmsize = ALIGN (seg.vmsize, PAGE_SIZE);
        seg.filesize = content - file - seg.fileoff;
        
        write_struct_segment_command_64 (saved_pos, &seg);

        seg.cmd = LC_SEGMENT_64;
        seg.cmdsize = SIZEOF_struct_segment_command_64_file;
        memset (seg.segname, 0, sizeof (seg.segname));
        strcpy (seg.segname, "__DATA");
        seg.vmaddr = 0;
        seg.vmsize = 0;
        seg.fileoff = content - file;
        seg.filesize = 0;
        seg.maxprot = PROT_READ | PROT_WRITE;
        seg.initprot = PROT_READ | PROT_WRITE;
        seg.nsects = 0;
        seg.flags = 0;
        saved_pos = pos;
        pos += SIZEOF_struct_segment_command_64_file;
        
        for (section = all_sections; section; section = section->next) {
            if (section_in_data_seg (section)) {
                struct section_64_internal sect = {{0}};

                if (!seg.vmaddr) {
                    seg.vmaddr = ld_state->base_address + section->rva;
                }
                
                seg.cmdsize += SIZEOF_struct_section_64_file;
                seg.nsects++;

                memset (sect.sectname, 0, sizeof (sect.sectname));
                memcpy (sect.sectname,
                        section->name,
                        (strlen (section->name) >= sizeof (sect.sectname))
                        ? sizeof (sect.sectname)
                        : strlen (section->name));
                if (sect.sectname[0] == '.') {
                    /* Seems "." prefix causes problems
                     * and "__" must always be used as prefix.
                     */
                    memmove (sect.sectname + 2, sect.sectname + 1, sizeof (sect.sectname) - 2);
                    sect.sectname[0] = sect.sectname[1] = '_';
                }
                memset (sect.segname, 0, sizeof (sect.segname));
                strcpy (sect.segname, "__DATA");
                sect.addr = ld_state->base_address + section->rva;
                sect.size = section->total_size;
                sect.align = log_base2 (section->section_alignment);
                if (section->flags & SECTION_FLAG_LOAD) {
                    sect.flags = 0;
                } else {
                    sect.flags = S_ZEROFILL;
                }

                seg.vmsize = ld_state->base_address + section->rva + section->total_size - seg.vmaddr;
                if (!section->is_bss) {
                    content = file + ALIGN (content - file, section->section_alignment);
                    sect.offset = content - file;
                    section_write (section, content);
                    content += section->total_size;
                } else {
                    sect.offset = 0;
                }

                write_struct_section_64 (pos, &sect);
                pos += SIZEOF_struct_section_64_file;
            }
        }
        content = file + ALIGN (content - file, PAGE_SIZE);
        seg.vmsize = ALIGN (seg.vmsize, PAGE_SIZE);
        seg.filesize = content - file - seg.fileoff;

        data_segment_offset = seg.fileoff;
        
        write_struct_segment_command_64 (saved_pos, &seg);

        seg.cmd = LC_SEGMENT_64;
        seg.cmdsize = SIZEOF_struct_segment_command_64_file;
        memset (seg.segname, 0, sizeof (seg.segname));
        strcpy (seg.segname, "__LINKEDIT");
        seg.vmaddr = seg.vmaddr + seg.vmsize;
        seg.vmsize = file_size - (content - file);
        seg.fileoff = content - file;
        seg.filesize = 0;
        seg.maxprot = PROT_READ;
        seg.initprot = PROT_READ;
        seg.nsects = 0;
        seg.flags = 0;

        seg.vmsize = ALIGN (seg.vmsize, PAGE_SIZE);
        seg.filesize = file_size - seg.fileoff;
        
        write_struct_segment_command_64 (pos, &seg);
        pos += SIZEOF_struct_segment_command_64_file;
    }

    if (new_format) {
        {
            struct dyld_chained_fixups_command_internal dyld_chained_fixups_cmd;

            dyld_chained_fixups_cmd.cmd = LC_DYLD_CHAINED_FIXUPS;
            dyld_chained_fixups_cmd.cmdsize = SIZEOF_struct_dyld_chained_fixups_command_file;
            dyld_chained_fixups_cmd.dataoff = content - file;

            {
                struct dyld_chained_fixups_header_internal chain_hdr;
                unsigned char *starts; 
                unsigned char *saved = content;

                chain_hdr.fixups_version = 0;
                chain_hdr.imports_counts = 0;
                chain_hdr.imports_format = DYLD_CHAINED_IMPORT;
                chain_hdr.symbols_format = 0;
                chain_hdr.padding = 0;

                content += SIZEOF_struct_dyld_chained_fixups_header_file;
                chain_hdr.starts_offset = content - saved;

                /* dyld_chained_starts_in_image. */
                starts = content;
                bytearray_write_4_bytes (content, NUM_SEGS, LITTLE_ENDIAN);
                bytearray_write_4_bytes (content + 4, 0, LITTLE_ENDIAN);
                bytearray_write_4_bytes (content + 8, 0, LITTLE_ENDIAN);
                bytearray_write_4_bytes (content + 12, 0, LITTLE_ENDIAN);
                bytearray_write_4_bytes (content + 16, 0, LITTLE_ENDIAN);
                bytearray_write_4_bytes (content + 20, 0, LITTLE_ENDIAN);
                content += 24;

#ifdef GENERATE_TEXT_CHAINED_FIXUPS
                if (chained_fixups[1].page_count) {
                    struct dyld_chained_starts_in_segment_internal chained_in_segment;
                    int i;
                    struct chained_fixup_starts *cfs = &chained_fixups[1];
                    
                    bytearray_write_4_bytes (starts + 8, content - starts, LITTLE_ENDIAN);
                    chained_in_segment.size = SIZEOF_struct_dyld_chained_starts_in_segment_file;
                    chained_in_segment.page_size = PAGE_SIZE;
                    chained_in_segment.pointer_format = DYLD_CHAINED_PTR_64_OFFSET;
                    chained_in_segment.segment_offset = 0;
                    chained_in_segment.max_valid_pointer = 0;
                    chained_in_segment.page_count = cfs->page_count;
                    chained_in_segment.page_start = cfs->page_starts[0];
                    chained_in_segment.size += chained_in_segment.page_count * 2 - 2;
                    chained_in_segment.size = ALIGN (chained_in_segment.size, 4);
                    write_struct_dyld_chained_starts_in_segment (content, &chained_in_segment);
                    content += SIZEOF_struct_dyld_chained_starts_in_segment_file;

                    for (i = 1; i < cfs->page_count; i++) {
                        bytearray_write_2_bytes (content, cfs->page_starts[i], LITTLE_ENDIAN);
                        content += 2;
                    }
                    content = file + ALIGN (content - file, 4);
                    free (cfs->page_starts);
                }
#endif

                if (chained_fixups[2].page_count) {
                    struct dyld_chained_starts_in_segment_internal chained_in_segment;
                    int i;
                    struct chained_fixup_starts *cfs = &chained_fixups[2];
                    
                    bytearray_write_4_bytes (starts + 12, content - starts, LITTLE_ENDIAN);
                    chained_in_segment.size = SIZEOF_struct_dyld_chained_starts_in_segment_file;
                    chained_in_segment.page_size = PAGE_SIZE;
                    chained_in_segment.pointer_format = DYLD_CHAINED_PTR_64_OFFSET;
                    chained_in_segment.segment_offset = data_segment_offset;
                    chained_in_segment.max_valid_pointer = 0;
                    chained_in_segment.page_count = cfs->page_count;
                    chained_in_segment.page_start = cfs->page_starts[0];
                    chained_in_segment.size += chained_in_segment.page_count * 2 - 2;
                    chained_in_segment.size = ALIGN (chained_in_segment.size, 4);
                    write_struct_dyld_chained_starts_in_segment (content, &chained_in_segment);
                    content += SIZEOF_struct_dyld_chained_starts_in_segment_file;

                    for (i = 1; i < cfs->page_count; i++) {
                        bytearray_write_2_bytes (content, cfs->page_starts[i], LITTLE_ENDIAN);
                        content += 2;
                    }
                    content = file + ALIGN (content - file, 4);
                    free (cfs->page_starts);
                }

                chain_hdr.imports_offset = content - saved;
                chain_hdr.symbols_offset = content - saved;
                bytearray_write_4_bytes (content, 0, LITTLE_ENDIAN);
                bytearray_write_4_bytes (content + 4, 0, LITTLE_ENDIAN);
                content += 8;

                write_struct_dyld_chained_fixups_header (saved, &chain_hdr);
            }
            
            dyld_chained_fixups_cmd.datasize = content - file - dyld_chained_fixups_cmd.dataoff;
            
            write_struct_dyld_chained_fixups_command (pos, &dyld_chained_fixups_cmd);
            pos += dyld_chained_fixups_cmd.cmdsize;
        }

        {
            struct dyld_exports_trie_command_internal dyld_exports_trie;

            dyld_exports_trie.cmd = LC_DYLD_EXPORTS_TRIE;
            dyld_exports_trie.cmdsize = SIZEOF_struct_dyld_exports_trie_command_file;
            dyld_exports_trie.dataoff = content - file;

            {
                /* Just copied from working executable. */
                unsigned char *saved = content;
                
                content[0] = 0;
                content[1] = 1;
                content += 2;
                strcpy ((char *)content, MH_HEADER_SYMBOL_NAME);
                content += sizeof (MH_HEADER_SYMBOL_NAME);
                content[0] = content - saved + 1;
                content[1] = 2;
                content += 2;

                content = saved + 0x980;
            }
            
            dyld_exports_trie.datasize = content - file - dyld_exports_trie.dataoff;
            
            write_struct_dyld_exports_trie_command (pos, &dyld_exports_trie);
            pos += dyld_exports_trie.cmdsize;
        }

        {
            struct dylib_command_internal dylib;

            dylib.cmd = LC_LOAD_DYLIB;
            dylib.cmdsize = ALIGN (SIZEOF_struct_dylib_command_file + sizeof (DYLIB_STRING), 8);
            dylib.name_offset = SIZEOF_struct_dylib_command_file;
            dylib.timestamp = 2;
            dylib.current_version = 0x05276403;
            dylib.compatibility_version = 0x00010000;
            
            write_struct_dylib_command (pos, &dylib);
            strcpy ((char *)pos + SIZEOF_struct_dylib_command_file,
                    DYLIB_STRING);
            pos += dylib.cmdsize;
        }
        
        {
            struct function_starts_command_internal function_starts_cmd;

            function_starts_cmd.cmd = LC_FUNCTION_STARTS;
            function_starts_cmd.cmdsize = SIZEOF_struct_function_starts_command_file;
            function_starts_cmd.dataoff = content - file;

            memcpy (content, function_starts, function_starts_size);
            content += function_starts_size;
            free (function_starts);
            function_starts_cmd.datasize = content - file - function_starts_cmd.dataoff;
            
            write_struct_function_starts_command (pos, &function_starts_cmd);
            pos += function_starts_cmd.cmdsize;
        }

        {
            struct symtab_command_internal symtab = {0};

            symtab.cmd = LC_SYMTAB;
            symtab.cmdsize = SIZEOF_struct_symtab_command_file;
            symtab.symoff = symoff = content - file;
            symtab.nsyms = 0;

            {
                struct nlist_64_internal nlist;

                nlist.n_strx = 4;
                nlist.n_type = N_SECT | N_EXT;
                nlist.n_sect = 1;
                nlist.n_desc = 0x10;
                nlist.n_value = ld_state->base_address;

                write_struct_nlist_64 (content, &nlist);
                content += SIZEOF_struct_nlist_64_file;
                symtab.nsyms++;
            }

            symtab.stroff = content - file;
            
            {
                memset (content, '\0', 4);
                content += 4;
                strcpy ((char *)content, MH_HEADER_SYMBOL_NAME);
                content += sizeof MH_HEADER_SYMBOL_NAME;
            }
            
            symtab.strsize = content - file - symtab.stroff;
            
            write_struct_symtab_command (pos, &symtab);
            pos += symtab.cmdsize;
        }
    } else {
        struct symtab_command_internal symtab = {0};

        symtab.cmd = LC_SYMTAB;
        symtab.cmdsize = SIZEOF_struct_symtab_command_file;
        
        write_struct_symtab_command (pos, &symtab);
        pos += symtab.cmdsize;
    }

    if (!new_format) {
        struct unixthread_command_internal unixthread = {0};
        struct x86_thread_state64_t_internal thread_state = {0};

        unixthread.cmd = LC_UNIXTHREAD;
        unixthread.cmdsize = (SIZEOF_struct_unixthread_command_file
                              + SIZEOF_struct_x86_thread_state64_t_file);
        unixthread.flavor = 0x4;
        unixthread.count = 0x2a;
        
        write_struct_unixthread_command (pos, &unixthread);

        thread_state.rip = ld_state->base_address + ld_state->entry_point;
        write_struct_x86_thread_state64_t (pos + SIZEOF_struct_unixthread_command_file,
                                           &thread_state);
        pos += unixthread.cmdsize;
    }

    if (new_format) {
        {
            struct dysymtab_command_internal dysymtab = {0};

            dysymtab.cmd = LC_DYSYMTAB;
            dysymtab.cmdsize = SIZEOF_struct_dysymtab_command_file;

            dysymtab.nextdefsym = 1; /* The MH_HEADER symbol. */
            dysymtab.iundefsym = 1;
            
            write_struct_dysymtab_command (pos, &dysymtab);
            pos += dysymtab.cmdsize;
        }

        if (USE_BUILD_VERSION) {
            struct build_version_command_internal bld_cmd;

            bld_cmd.cmd = LC_BUILD_VERSION;
            bld_cmd.cmdsize = SIZEOF_struct_build_version_command_file;
            bld_cmd.platform = 1;
            bld_cmd.minos = minos_version;
            bld_cmd.sdk = SDK_VERSION;
            bld_cmd.ntools = 1;
            bld_cmd.unknown1 = 3;
            bld_cmd.unknown2 = 0x03590100;
            
            write_struct_build_version_command (pos, &bld_cmd);
            pos += bld_cmd.cmdsize;
        } else {
            struct version_min_command_internal version_min_cmd;

            version_min_cmd.cmd = LC_VERSION_MIN;
            version_min_cmd.cmdsize = SIZEOF_struct_version_min_command_file;
            version_min_cmd.version = minos_version;
            version_min_cmd.sdk = SDK_VERSION;
            
            write_struct_version_min_command (pos, &version_min_cmd);
            pos += version_min_cmd.cmdsize;
        }
    
        {
            struct dylinker_command_internal dylinker;

            dylinker.cmd = LC_LOAD_DYLINKER;
            dylinker.cmdsize = ALIGN (SIZEOF_struct_dylinker_command_file + sizeof (DYLINKER_STRING), 8);
            dylinker.name_offset = SIZEOF_struct_dylinker_command_file;
            
            write_struct_dylinker_command (pos, &dylinker);
            strcpy ((char *)pos + SIZEOF_struct_dylinker_command_file,
                    DYLINKER_STRING);
            pos += dylinker.cmdsize;
        }

        {
            struct source_version_command_internal src_cmd;

            src_cmd.cmd = LC_SOURCE_VERSION;
            src_cmd.cmdsize = SIZEOF_struct_source_version_command_file;
            src_cmd.version = 0;
            
            write_struct_source_version_command (pos, &src_cmd);
            pos += src_cmd.cmdsize;
        }

        {
            struct data_in_code_command_internal data_in_code_cmd;

            data_in_code_cmd.cmd = LC_DATA_IN_CODE;
            data_in_code_cmd.cmdsize = SIZEOF_struct_data_in_code_command_file;
            data_in_code_cmd.dataoff = symoff;
            data_in_code_cmd.datasize = 0;
            
            write_struct_data_in_code_command (pos, &data_in_code_cmd);
            pos += data_in_code_cmd.cmdsize;
        }

        {
            struct main_command_internal main_cmd;

            main_cmd.cmd = LC_MAIN;
            main_cmd.cmdsize = SIZEOF_struct_main_command_file;
            main_cmd.entryoff = ld_state->entry_point;
            main_cmd.stacksize = 0;
            
            write_struct_main_command (pos, &main_cmd);
        }
    }

    if (fwrite (file, file_size, 1, outfile) != 1) {
        ld_error ("writing '%s' file failed", filename);
    }
    
    free (file);
    fclose (outfile);
}

static struct part_reloc addend_part_reloc;

static void apply_addend_reloc (struct reloc_entry *reloc,
                                const struct section_part *part)
{
    if (!addend_part_reloc.reloc_entry) return;

    if (part != addend_part_reloc.part) {
        ld_warn ("%s: ARM64_RELOC_ADDEND is in different"
                 " section than relocation it applies to",
                 addend_part_reloc.part->of->filename);
    }

    if (reloc->offset != addend_part_reloc.reloc_entry->offset) {
        ld_warn ("%s: ARM64_RELOC_ADDEND has different"
                 " offset than relocation it applies to",
                 addend_part_reloc.part->of->filename);
    }

    reloc->addend = addend_part_reloc.reloc_entry->addend;

    addend_part_reloc.reloc_entry = NULL;
}

static void translate_relocation (struct reloc_entry *reloc,
                                  const struct relocation_info_internal *input_reloc,
                                  struct section_part *part)
{
    unsigned int size, pcrel, type;
    
    if ((input_reloc->r_symbolnum >> 27) & 1) {
        reloc->symbol = part->of->symbol_array + (input_reloc->r_symbolnum & 0xffffff);
    } else {
        if (input_reloc->r_symbolnum == 0) {
            ld_internal_error_at_source (__FILE__, __LINE__,
                                         "local input_reloc->r_symbolnum %#lx is not yet supported",
                                         input_reloc->r_symbolnum & 0xffffff);
        }
        reloc->symbol = part->of->symbol_array + part->of->symbol_count - (input_reloc->r_symbolnum & 0xffffff);
    }

    reloc->offset = input_reloc->r_address;

    size = 1U << ((input_reloc->r_symbolnum >> 25) & 3);
    pcrel = (input_reloc->r_symbolnum >> 24) & 1;
    type = input_reloc->r_symbolnum >> 28;

    if (ld_state->target_machine == LD_TARGET_MACHINE_X64) {
        switch (size) {
            case 8:
                if (!pcrel && type == X86_64_RELOC_UNSIGNED) {
                    reloc->howto = &reloc_howtos[RELOC_TYPE_64];
                } else goto unsupported;
                break;
            
            case 4:
                if (pcrel) {
                    if (type == X86_64_RELOC_SIGNED) {
                        reloc->howto = &reloc_howtos[RELOC_TYPE_PC32];
                        if (reloc->symbol->flags & SYMBOL_FLAG_SECTION_SYMBOL) {
                            /* For relocations with section symbols
                             * the field is already relocated
                             * as if entire executable
                             * was only the object file,
                             * so it must be unrelocated
                             * using section address saved earlier.
                             */
                            unsigned long field;
                            
                            bytearray_read_4_bytes (&field, part->content + reloc->offset, LITTLE_ENDIAN);
                            field -= reloc->symbol->part->rva - reloc->offset - 4;
                            bytearray_write_4_bytes (part->content + reloc->offset, field, LITTLE_ENDIAN);
                        }
                    } else if (type == X86_64_RELOC_BRANCH) {
                        reloc->howto = &reloc_howtos[RELOC_TYPE_PC32];
                    } else if (type == X86_64_RELOC_GOT_LOAD) {
                        /* Variables are typically accesed through x64 GOT using:
                         * 48 8b 05 00 00 00 00 mov rax, QWORD PTR [rip+0x0]
                         * The MOV can modified into LEA by changing 0x8b to 0x8d:
                         * 48 8d 05 00 00 00 00 lea rax, [rip+0x0]
                         * and GOT is thus avoided.
                         */
                        if (part->content[reloc->offset - 2] != 0x8b) {
                            ld_internal_error_at_source (__FILE__, __LINE__,
                                                         "%s: not yet supported x64 GOT instruction",
                                                         part->of->filename);
                        }
                        part->content[reloc->offset - 2] = 0x8d;
                        
                        reloc->howto = &reloc_howtos[RELOC_TYPE_PC32];
                    } else if (type == X86_64_RELOC_GOT) {
                        /* This relocation needs pointer from GOT
                         * to variable but not to directly access the variable
                         * but to add the pointer to value in register
                         * in one instruction:
                         * 48 03 05 00 00 00 00 add rax, QWORD PTR [rip+0x0]
                         * This seems impossible to do without GOT,
                         * so instead call to generated helper function
                         * must be used:
                         * 90 nop
                         * 90 nop
                         * e8 00 00 00 00 call helper
                         * ...
                         * helper:
                         * 51 push rcx
                         * 48 8d 0d 00 00 00 00 lea rcx, [rip+0x0]
                         * 48 01 c8 add rax, rcx
                         * 59 pop rcx
                         * c3 ret
                         */
                        address_type addend;
                        size_t helper_offset;
                        
                        if (part->content[reloc->offset - 1] != 0x05
                            || part->content[reloc->offset - 2] != 0x03
                            || part->content[reloc->offset - 3] != 0x48) {
                            ld_internal_error_at_source (__FILE__, __LINE__,
                                                         "%s: not yet supported x64 GOT instruction",
                                                         part->of->filename);
                        }
                        part->content[reloc->offset - 3] = 0x90;
                        part->content[reloc->offset - 2] = 0x90;
                        part->content[reloc->offset - 1] = 0xe8;
                        {
                            unsigned long field;
                            
                            bytearray_read_4_bytes (&field, part->content + reloc->offset, LITTLE_ENDIAN);
                            addend = field;
                            field = part->content_size - reloc->offset - 4;
                            bytearray_write_4_bytes (part->content + reloc->offset, field, LITTLE_ENDIAN);
                        }
                        
                        helper_offset = part->content_size;
                        part->content_size += 13;
                        part->content = xrealloc (part->content, part->content_size);
                        memcpy (part->content + helper_offset,
                                "\x51"
                                "\x48\x8D\x0D\x00\x00\x00\x00"
                                "\x48\x01\xC8"
                                "\x59"
                                "\xC3",
                                13);
                        reloc->offset = helper_offset + 4;
                        bytearray_write_4_bytes (part->content + reloc->offset, addend, LITTLE_ENDIAN);
                        
                        reloc->howto = &reloc_howtos[RELOC_TYPE_PC32];
                    } else if (type == X86_64_RELOC_SIGNED_1
                               || type == X86_64_RELOC_SIGNED_2
                               || type == X86_64_RELOC_SIGNED_4) {
                        /* The "_N" part needs to be subtracted from the field
                         * and, unlike with PE/COFF IMAGE_REL_AMD64_REL32_1 etc.,
                         * the assembler already did it
                         * but for unknown reason decided
                         * to inform the linker of it too,
                         * so it should be treated as
                         * regular RIP-relative relocation.
                         */
                        reloc->howto = &reloc_howtos[RELOC_TYPE_PC32];
                    } else goto unsupported;
                } else {
                    if (type == X86_64_RELOC_UNSIGNED) {
                        reloc->howto = &reloc_howtos[RELOC_TYPE_32];
                    } else if (type == X86_64_RELOC_SUBTRACTOR) {
                        /* Must be followed by X86_64_RELOC_SUBTRACTOR
                         * but there is no explanation why exactly,
                         * so no checking is done.
                         */
                        reloc->howto = &reloc_howtos[RELOC_TYPE_32_SUB];
                    } else goto unsupported;
                }
                break;

            default: goto unsupported;
        }
    } else if (ld_state->target_machine == LD_TARGET_MACHINE_AARCH64) {
        switch (size) {
            case 8:
                if (!pcrel && type == ARM64_RELOC_UNSIGNED) {
                    reloc->howto = &reloc_howtos[RELOC_TYPE_64];
                } else goto unsupported;
                break;
            
            case 4:
                if (pcrel) {
                    if (type == ARM64_RELOC_BRANCH26) {
                        reloc->howto = &reloc_howtos[RELOC_TYPE_AARCH64_CALL26];
                        apply_addend_reloc (reloc, part);
                    } else if (type == ARM64_RELOC_PAGE21) {
                        reloc->howto = &reloc_howtos[RELOC_TYPE_AARCH64_ADR_PREL_PG_HI21];
                        apply_addend_reloc (reloc, part);
                    } else if (type == ARM64_RELOC_GOT_LOAD_PAGE21) {
                        /* Variables are typically accesed through GOT using:
                         * 09 00 00 90 adrp x9, #GOT_SLOT_PAGE21
                         * 29 01 40 F9 ldr x9, [x9, #GOT_SLOT_PAGEOFF]
                         * 29 01 40 F9 ldr x9, [x9]
                         * ADRP and first LDR put a pointer into x9
                         * and then later instructions use the pointer.
                         * One other linker avoids GOT by modifying the above into:
                         * 1F 20 03 D5 nop
                         * 1F 20 03 D5 nop
                         * C9 1E 03 58 ldr x9, #PC_relative_variable_address
                         * What generates faster code but is more complex
                         * to support in linker
                         * (as it changes the instructions after relocated fields)
                         * and has smaller range.
                         * Simpler solution to support is:
                         * 09 00 00 90 adrp x9, #variable_PAGE21
                         * 29 01 00 91 add x9, x9, #variable_PAGEOFF
                         * 29 01 40 F9 ldr x9, [x9]
                         * This way ADRP and ADD create a pointer
                         * and put it into x9 without needing GOT.
                         */
                        /* ADRP needs no change. */
                        reloc->howto = &reloc_howtos[RELOC_TYPE_AARCH64_ADR_PREL_PG_HI21];
                    } else goto unsupported;
                } else {
                    if (type == ARM64_RELOC_PAGEOFF12) {
                        /* Same problem as with COFF IMAGE_REL_ARM64_PAGEOFFSET_12L
                         * except "add" is under ARM64_RELOC_PAGEOFF12 type
                         * too.
                         */
                        if (part->content[reloc->offset + 3] == 0x39) {
                            /* strb w... */
                            reloc->howto = &reloc_howtos[RELOC_TYPE_AARCH64_LDST8_ABS_LO12_NC];
                        } else if (part->content[reloc->offset + 3] == 0x79) {
                            /* strh w... */
                            reloc->howto = &reloc_howtos[RELOC_TYPE_AARCH64_LDST16_ABS_LO12_NC]; /*strh*/
                        } else if (part->content[reloc->offset + 3] == 0xB9) {
                            /* str w... */
                            reloc->howto = &reloc_howtos[RELOC_TYPE_AARCH64_LDST32_ABS_LO12_NC];
                        } else if (part->content[reloc->offset + 3] == 0xF9
                                   || part->content[reloc->offset + 3] == 0xFD) {
                            /* str x... or str d... (64-bit floating register) */
                            reloc->howto = &reloc_howtos[RELOC_TYPE_AARCH64_LDST64_ABS_LO12_NC];
                        } else if (part->content[reloc->offset + 3] == 0x3D) {
                            /* str q... (128-bit floating register) */
                            reloc->howto = &reloc_howtos[RELOC_TYPE_AARCH64_LDST128_ABS_LO12_NC];
                        } else {
                            reloc->howto = &reloc_howtos[RELOC_TYPE_AARCH64_ADD_ABS_LO12_NC];
                        }
                        apply_addend_reloc (reloc, part);
                    } else if (type == ARM64_RELOC_GOT_LOAD_PAGEOFF12) {
                        unsigned long field;
                        
                        bytearray_read_4_bytes (&field,
                                                part->content + reloc->offset,
                                                LITTLE_ENDIAN);
                        /* The original instruction is LDST32:
                         * 29 01 40 F9 ldr x9, [x9, #0]
                         * It needs to be converted into ADD:
                         * 29 01 00 91 add x9, x9, #0
                         * by modifying the top 2 bytes.
                         */
                        field &= ~0x68400000;
                        bytearray_write_4_bytes (part->content + reloc->offset,
                                                 field,
                                                 LITTLE_ENDIAN);

                        reloc->howto = &reloc_howtos[RELOC_TYPE_AARCH64_ADD_ABS_LO12_NC];
                    } else if (type == ARM64_RELOC_ADDEND) {
                        reloc->symbol = NULL;
                        reloc->addend = input_reloc->r_symbolnum & 0xffffff;
                        reloc->howto = &reloc_howtos[RELOC_TYPE_IGNORED];
                        addend_part_reloc.reloc_entry = reloc;
                        addend_part_reloc.part = part;
                    } else goto unsupported;
                }
                break;

            default: goto unsupported;
        }
    } else {
unsupported:
        ld_warn ("%s: ignoring not yet supported relocation with size %u, pcrel %u and type %#x",
                 part->of->filename,
                 size,
                 pcrel,
                 type);        
        reloc->howto = &reloc_howtos[RELOC_TYPE_IGNORED];
    }

    return;
}

#define CHECK_READ(memory_position, size_to_read) \
    do { if (((memory_position) - file + (size_to_read) > file_size) \
             || (memory_position) < file) ld_fatal_error ("%s: corrupted input file", filename); } while (0)

static int read_macho_object (unsigned char *file, size_t file_size, const char *filename)
{
    struct mach_header_64_internal hdr64;
    unsigned long i;

    unsigned char *pos;

    struct object_file *of;
    struct section_part **part_p_array;
    size_t num_symbols = 0;
    size_t num_sections = 0;
    struct section *bss_section = NULL;
    long bss_section_number = 0;

    addend_part_reloc.part = NULL;
    addend_part_reloc.reloc_entry = NULL;

    pos = file;
    CHECK_READ (pos, SIZEOF_struct_mach_header_64_file);
    read_struct_mach_header_64 (&hdr64, pos);
    pos += SIZEOF_struct_mach_header_64_file;

    if (check_cputype (hdr64.cputype, filename)) return 1;

    if (hdr64.filetype != MH_OBJECT) {
        ld_error ("%s: filetype is not MH_OBJECT", filename);
        return 1;
    }

    CHECK_READ (pos, hdr64.sizeofcmds);
    of = NULL;
    for (i = 0; i < hdr64.ncmds; i++) {
        struct load_command_internal load_cmd;

        if (pos - file + SIZEOF_struct_load_command_file
            > SIZEOF_struct_mach_header_64_file + hdr64.sizeofcmds) {
            ld_error ("%s: invalid header sizeofcmds/ncmds", filename);
            return 1;
        }

        read_struct_load_command (&load_cmd, pos);
        if (pos - file + load_cmd.cmdsize
            > SIZEOF_struct_mach_header_64_file + hdr64.sizeofcmds) {
            ld_error ("%s: invalid load command cmdsize", filename);
            return 1;
        }
        
        if (load_cmd.cmd == LC_SYMTAB) {
            struct symtab_command_internal symtab_cmd;

            read_struct_symtab_command (&symtab_cmd, pos);
            if (SIZEOF_struct_symtab_command_file > symtab_cmd.cmdsize) {
                ld_fatal_error ("%s: symtab command cmdsize too small", filename);
            }
            
            if (num_symbols && symtab_cmd.nsyms) {
                ld_fatal_error ("%s: more than 1 non-empty symbol table per object file", filename);
            }
            num_symbols = symtab_cmd.nsyms;
        } else if (load_cmd.cmd == LC_SEGMENT_64) {
            struct segment_command_64_internal segment_cmd;

            read_struct_segment_command_64 (&segment_cmd, pos);
            if (num_sections && segment_cmd.nsects) {
                ld_fatal_error ("%s: more than 1 non-empty segment command per object file", filename);
            }
            num_sections = segment_cmd.nsects;
        }

        pos += load_cmd.cmdsize;
    }

    of = object_file_make (num_symbols + num_sections, filename);

    part_p_array = NULL;
    pos = file + SIZEOF_struct_mach_header_64_file;
    for (i = 0; i < hdr64.ncmds; i++) {
        struct load_command_internal load_cmd;

        read_struct_load_command (&load_cmd, pos);
        
        if (load_cmd.cmd == LC_SEGMENT_64) {
            struct segment_command_64_internal segment_cmd;
            unsigned long j;

            read_struct_segment_command_64 (&segment_cmd, pos);
            if (!segment_cmd.nsects) {
                pos += load_cmd.cmdsize;
                continue;
            }
            
            part_p_array = xcalloc (segment_cmd.nsects + 1, sizeof *part_p_array);

            if (SIZEOF_struct_segment_command_64_file
                + segment_cmd.nsects * SIZEOF_struct_section_64_file
                > segment_cmd.cmdsize) {
                ld_fatal_error ("%s: segment command cmdsize and nsects mismatch", filename);
            }

            for (j = 0; j < segment_cmd.nsects; j++) {
                struct section_64_internal sect_64;
                char *section_name, *segment_name;
                struct section *section;

                read_struct_section_64 (&sect_64,
                                        pos + SIZEOF_struct_segment_command_64_file
                                        + j * SIZEOF_struct_section_64_file);
                section_name = xstrndup (sect_64.sectname, sizeof (sect_64.sectname));
                segment_name = xstrndup (sect_64.segname, sizeof (sect_64.segname));
                section = section_find_or_make (section_name);
                free (section_name);

                if (1LU << sect_64.align > section->section_alignment) {
                    section->section_alignment = 1LU << sect_64.align;
                }

                if (strcmp (segment_name, "__TEXT") == 0) {
                    section->flags = SECTION_FLAG_ALLOC | SECTION_FLAG_LOAD | SECTION_FLAG_READONLY;
                    if (sect_64.flags & S_REGULAR) {
                        section->flags |= SECTION_FLAG_CODE;
                    }
                } else if (strcmp (segment_name, "__DATA") == 0) {
                    if (strcmp (section->name, "__const") == 0) {
                        section->flags = SECTION_FLAG_ALLOC | SECTION_FLAG_LOAD | SECTION_FLAG_READONLY;
                    } else if ((sect_64.flags & 0xff) == S_ZEROFILL) {
                        section->flags = SECTION_FLAG_ALLOC;
                        section->is_bss = 1;
                        bss_section_number = j + 1;
                        bss_section = section;
                    } else {
                        section->flags = SECTION_FLAG_ALLOC | SECTION_FLAG_LOAD | SECTION_FLAG_DATA;
                    }
                } else {
                    /* Segment "__LD" and such. */
                    section->flags = SECTION_FLAG_ALLOC | SECTION_FLAG_LOAD | SECTION_FLAG_READONLY;
                }
                free (segment_name);

                {
                    struct section_part *part = section_part_new (section, of);

                    /* The RVA is going to overwritten later,
                     * it is just necessary to save the section address
                     * for now for adjusting N_SECT symbol values.
                     */
                    part->rva = sect_64.addr;
                    
                    part->alignment = 1LU << sect_64.align;

                    part->content_size = sect_64.size;
                    if (!section->is_bss) {
                        const unsigned char *content_pos = file + sect_64.offset;
                        part->content = xmalloc (part->content_size);

                        CHECK_READ (content_pos, part->content_size);
                        memcpy (part->content, content_pos, part->content_size);
                    }
                    
                    section_append_section_part (section, part);

                    part_p_array[j + 1] = part;
                }
            }
        }

        pos += load_cmd.cmdsize;
    }

    pos = file + SIZEOF_struct_mach_header_64_file;
    for (i = 0; i < hdr64.ncmds; i++) {
        struct load_command_internal load_cmd;

        read_struct_load_command (&load_cmd, pos);
        
        if (load_cmd.cmd == LC_SYMTAB) {
            struct symtab_command_internal symtab_cmd;
            const char *string_table;
            const unsigned char *sym_pos;
            unsigned long j;

            read_struct_symtab_command (&symtab_cmd, pos);
            if (!symtab_cmd.nsyms) {
                pos += load_cmd.cmdsize;
                continue;
            }
            
            string_table = (char *)file + symtab_cmd.stroff;
            CHECK_READ (file + symtab_cmd.stroff, symtab_cmd.strsize);

            sym_pos = file + symtab_cmd.symoff;
            CHECK_READ (sym_pos, SIZEOF_struct_nlist_64_file * symtab_cmd.nsyms);

            for (j = 0; j < symtab_cmd.nsyms; j++) {
                struct nlist_64_internal nlist_64;
                struct symbol *symbol = of->symbol_array + j;

                read_struct_nlist_64 (&nlist_64, sym_pos + j * SIZEOF_struct_nlist_64_file);

                if (nlist_64.n_strx < symtab_cmd.strsize) {
                    if (string_table[nlist_64.n_strx] == '\0') {
                        symbol->name = xstrdup (UNNAMED_SYMBOL_NAME);
                    } else {
                        symbol->name = xstrdup (string_table + nlist_64.n_strx);
                    }
                } else ld_fatal_error ("%s: invalid index into string table", filename);

                symbol->value = nlist_64.n_value;

                if ((nlist_64.n_type & N_STAB)
                    || (nlist_64.n_type & N_PEXT)) {
                    ld_internal_error_at_source (__FILE__, __LINE__,
                                                 "+++%s: not yet supported symbol n_type: %#x",
                                                 filename, nlist_64.n_type);
                }

                if ((nlist_64.n_type & N_TYPE) == N_UNDF) {
                    if (symbol->value) {
                        /* It is a common symbol. */
                        struct symbol *old_symbol = symbol_find (symbol->name);

                        if (!old_symbol || symbol_is_undefined (old_symbol)) {
                            struct section_part *bss_part;
                            
                            if (bss_section == NULL) {
                                bss_section = section_find_or_make ("__bss");
                                bss_section->flags = SECTION_FLAG_ALLOC;
                                bss_section->is_bss = 1;
                                bss_section_number = num_sections ? num_sections : 1;
                            }
                            
                            bss_part = section_part_new (bss_section, of);
                            section_append_section_part (bss_section, bss_part);

                            bss_part->content_size = symbol->size = symbol->value;
                            /* ARM64 cannot access unaligned memory
                             * but there is no way to specify common symbol alignment in Mach-O,
                             * so linker must determine the alignment on its own.
                             */
#if 0
                            /* One way is to set the same alignment as is the size
                             * of the field rounded up to the closest power of two
                             * but at most PAGE_SIZE to avoid wasting memory
                             * because nothing should need greater alignment.
                             */
                            if (bss_part->content_size > PAGE_SIZE) {
                                bss_part->alignment = PAGE_SIZE;
                            } else if (bss_part->content_size
                                       & (bss_part->content_size - 1)) {
                                address_type align = bss_part->content_size;
                                align |= (align >> 1);
                                align |= (align >> 2);
                                align |= (align >> 4);
                                align |= (align >> 8);
                                align |= (align >> 16);
                                align++;
                                bss_part->alignment = align;
                            } else {
                                bss_part->alignment = bss_part->content_size;
                            }
#else
                            /* Simpler way is to assume everything needs
                             * at most 16-byte alignment.
                             * This is the currently used solution
                             * but if greater alignment is needed,
                             * the above code can be enabled.
                             */
                            bss_part->alignment = 16;
#endif
                            symbol->part = bss_part;
                            symbol->value = 0;
                            symbol->section_number = bss_section_number;
                        } else {
                            if (symbol->value > old_symbol->size) {
                                old_symbol->part->content_size = old_symbol->size = symbol->value;
                            }
                            
                            symbol->value = 0;
                            symbol->part = NULL;
                        }
                    } else {
                        symbol->section_number = UNDEFINED_SECTION_NUMBER;
                        symbol->part = NULL;
                    }
                } else if ((nlist_64.n_type & N_TYPE) == N_SECT) {
                    if (nlist_64.n_sect > num_sections) {
                        ld_error ("%s: invalid symbol n_sect: %u", filename, nlist_64.n_sect);
                        symbol->part = NULL;
                    } else {
                        /* N_SECT symbol values are not relative
                         * to section part, unlike in other object formats,
                         * they are absolute, so the address
                         * of section saved before must be subtracted.
                         */
                        symbol->value -= part_p_array[nlist_64.n_sect]->rva;
                    }
                    symbol->section_number = nlist_64.n_sect;
                    symbol->part = part_p_array[nlist_64.n_sect];
                } else {
                    ld_internal_error_at_source (__FILE__, __LINE__,
                                                 "+++%s: not yet supported symbol n_type: %#x",
                                                 filename, nlist_64.n_type);
                }

                if (nlist_64.n_type & N_EXT) {
                    symbol_record_external_symbol (symbol);
                }
            }

            break;
        }

        pos += load_cmd.cmdsize;
    }

    /* Mach-O (just like a.out) relocations use implicit section symbols
     * which do not exist in the symbol table,
     * so section symbols need to be automatically generated by the linker.
     */
    for (i = 1; i < num_sections + 1; i++) {
        struct section_part *part;
        struct symbol *symbol = of->symbol_array + of->symbol_count - i;

        part = part_p_array[i];
        symbol->name = xstrdup (part->section->name);
        symbol->flags |= SYMBOL_FLAG_SECTION_SYMBOL;
        symbol->value = 0;
        symbol->size = 0;
        symbol->part = part;
        symbol->section_number = i;
    }

    /* Relocations are read and processed last
     * to have finished symbol table available
     * for modifying the relocations.
     */
    pos = file + SIZEOF_struct_mach_header_64_file;
    for (i = 0; i < hdr64.ncmds; i++) {
        struct load_command_internal load_cmd;

        read_struct_load_command (&load_cmd, pos);
        
        if (load_cmd.cmd == LC_SEGMENT_64) {
            struct segment_command_64_internal segment_cmd;
            unsigned long j;

            read_struct_segment_command_64 (&segment_cmd, pos);
            if (!segment_cmd.nsects) {
                pos += load_cmd.cmdsize;
                continue;
            }

            for (j = 0; j < segment_cmd.nsects; j++) {
                struct section_64_internal sect_64;

                read_struct_section_64 (&sect_64,
                                        pos + SIZEOF_struct_segment_command_64_file
                                        + j * SIZEOF_struct_section_64_file);

                {
                    struct section_part *part = part_p_array[j + 1];

                    if (sect_64.nreloc) {
                        size_t k;
                        const unsigned char *rel_pos = file + sect_64.reloff;

                        CHECK_READ (rel_pos, sect_64.nreloc * SIZEOF_struct_relocation_info_file);

                        part->relocation_count = sect_64.nreloc;
                        part->relocation_array = xcalloc (part->relocation_count, sizeof *part->relocation_array);

                        for (k = 0; k < sect_64.nreloc; k++) {
                            struct relocation_info_internal relocation_info;
                            read_struct_relocation_info (&relocation_info,
                                                         rel_pos + SIZEOF_struct_relocation_info_file * k);
                            translate_relocation (part->relocation_array + k, &relocation_info, part);
                        }
                    }
                }
            }
        }

        pos += load_cmd.cmdsize;
    }

    free (part_p_array);

    if (addend_part_reloc.reloc_entry) {
        ld_warn ("%s: ARM64_RELOC_ADDEND without following relocation",
                 filename);
    }
    
    return 0;
}

int macho_read (unsigned char *file, size_t file_size, const char *filename)
{
    unsigned long magic;
    
    CHECK_READ (file, 4);
    bytearray_read_4_bytes (&magic, file, LITTLE_ENDIAN);

    if (magic == MH_MAGIC_64) {
        if (read_macho_object (file, file_size, filename)) return INPUT_FILE_ERROR;
        return INPUT_FILE_FINISHED;
    }

    return INPUT_FILE_UNRECOGNIZED;
}

#include "options.h"

enum option_index {

    MACHO_OPTION_IGNORED = 0,
    MACHO_OPTION_MINOS_VERSION,
    MACHO_OPTION_OLD_MACHO

};

#define STR_AND_LEN(str) (str), (sizeof (str) - 1)
static const struct long_option long_options[] = {
    
    { STR_AND_LEN("minos-version"), MACHO_OPTION_MINOS_VERSION, OPTION_HAS_ARG},
    { STR_AND_LEN("old-macho"), MACHO_OPTION_OLD_MACHO, OPTION_NO_ARG},
    { NULL, 0, 0}

};
#undef STR_AND_LEN

void macho_print_help (void)
{
    printf ("Mach-O:\n");
    printf ("  --minos-version <a>[.<b>[.<c>]]   Set minos version to a.b.c"
            " (ARM64 default %lu.%lu.%lu, x64 default %lu.%lu.%lu)\n",
            (DEFAULT_ARM64_MINOS_VERSION >> 16) & 0xffff,
            (DEFAULT_ARM64_MINOS_VERSION >> 8) & 0xff,
            DEFAULT_ARM64_MINOS_VERSION & 0xff,
            (DEFAULT_X64_MINOS_VERSION >> 16) & 0xffff,
            (DEFAULT_X64_MINOS_VERSION >> 8) & 0xff,
            DEFAULT_X64_MINOS_VERSION & 0xff);
    printf ("  --old-macho                       Use old position dependent format\n");
}

static void use_option (enum option_index option_index, char *arg)
{
    switch (option_index) {

        case MACHO_OPTION_IGNORED:
            break;

        case MACHO_OPTION_MINOS_VERSION:
            {
                char *p;
                unsigned long a, b, c;

                a = strtoul (arg, &p, 0);
                minos_version = (a & 0xffff) << 16;
                if (*p == '\0') break;
                if (*p != '.') goto bad_minos_version;

                p++;
                b = strtoul (p, &p, 0);
                minos_version |= (b & 0xff) << 8;
                if (*p == '\0') break;
                if (*p != '.') goto bad_minos_version;

                p++;
                c = strtoul (p, &p, 0);
                minos_version |= c & 0xff;
                if (*p == '\0') break;

            bad_minos_version:
                ld_error ("invalid minos version '%s'", arg);
            }
            break;

        case MACHO_OPTION_OLD_MACHO:
            new_format = 0;
            break;

    }
}

void macho_use_option (int option_index, char *arg)
{
    use_option (option_index, arg);
}

const struct long_option *macho_get_long_options (void)
{
    return long_options;
}
