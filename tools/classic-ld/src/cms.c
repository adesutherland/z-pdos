/******************************************************************************
 * @file            cms.c
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

#include "ld.h"
#include "tebc.h"
#include "bytearray.h"
#include "xmalloc.h"

#define MAX_RECORD_SIZE 65535
#define MAX_RECORD_SYMS (MAX_RECORD_SIZE / 20)
#define MAX_RECORD_RELOCS (MAX_RECORD_SIZE / 4)

/* All records have either Record Length Halfword
 * (2 byte length of record data without the RLH itself)
 * or CDW
 * (2 byte length of record data without the RDW itself, 2 zero bytes)
 * at the beginning.
 * CDW is just RLH with 2 extra zero bytes, so only RLH size needs to change.
 */
static size_t rlh_size = 2;

struct temp_sym {
    struct symbol *symbol;
    address_type address;
};

static int temp_sym_compar (const void *a, const void *b)
{
    address_type aa = ((const struct temp_sym *)a)->address;
    address_type ba = ((const struct temp_sym *)b)->address;

    /* The goal is descdending order, not ascending. */
    if (aa < ba) return 1;
    if (ba == aa) return 0;
    return -1;
}

address_type cms_get_base_address (void)
{    
    return 0x20000;
}

static void write_ebcdic_symbol_name (unsigned char *pos, const char *name)
{
    int i;

    memset (pos, tebc (' '), 8);
    for (i = 0; i < 8 && name[i]; i++) {
        pos[i] = tebc (name[i]);
    }
}

static size_t get_num_relocs (void)
{
    struct section *section;
    size_t num_relocs = 0;
    
    for (section = all_sections; section; section = section->next) {
        struct section_part *part;
        
        for (part = section->first_part; part; part = part->next) {
            size_t i;
            
            for (i = 0; i < part->relocation_count; i++) {
                if (part->relocation_array[i].howto == &reloc_howtos[RELOC_TYPE_32]
                    || part->relocation_array[i].howto == &reloc_howtos[RELOC_TYPE_24]) {
                    num_relocs++;
                }
            }
        }
    }

    return num_relocs;
}

static unsigned char *write_relocs (unsigned char *pos, size_t num_relocs)
{
    struct section *section;
    size_t rel_i = 0;
    
    for (section = all_sections; section; section = section->next) {
        struct section_part *part;
        
        for (part = section->first_part; part; part = part->next) {
            size_t i;
            
            for (i = 0; i < part->relocation_count; i++) {
                unsigned char flags;
                
                if (part->relocation_array[i].howto == &reloc_howtos[RELOC_TYPE_IGNORED]) {
                    continue;
                }

                /* Not sure what meaning exactly does the flags field have, the values are guessed. */
                if (part->relocation_array[i].howto == &reloc_howtos[RELOC_TYPE_32]) {
                    flags = 0x23;
                } else if (part->relocation_array[i].howto == &reloc_howtos[RELOC_TYPE_24]) {
                    flags = 0x22;
                } else continue;

                if (rel_i % MAX_RECORD_RELOCS == 0) {
                    size_t this_relocs = num_relocs > MAX_RECORD_RELOCS ? MAX_RECORD_RELOCS : num_relocs;

                    bytearray_write_2_bytes (pos, this_relocs * 4, BIG_ENDIAN);
                    pos += rlh_size;
                    num_relocs -= this_relocs;
                }

                pos[0] = flags;
                bytearray_write_3_bytes (pos + 1,
                                         ld_state->base_address + part->rva + part->relocation_array[i].offset,
                                         BIG_ENDIAN);
                pos += 4;
                rel_i++;
            }
        }
    }

    return pos;
}

void cms_write (const char *filename)
{
    FILE *outfile;
    unsigned char *file;
    size_t file_size;
    unsigned char *pos;

    struct section *section;
    struct object_file *of;
    size_t total_section_size_to_write = 0;
    size_t num_symbols = 0;
    size_t num_relocs = 0;

    if (!(outfile = fopen (filename, "wb"))) {
        ld_error ("cannot open '%s' for writing", filename);
        return;
    }

    {
        /* 80 byte record with loader information. */
        file_size = rlh_size + 80;
        
        for (section = all_sections; section; section = section->next) {
            if (section->is_bss) continue;
            
            total_section_size_to_write = ALIGN (total_section_size_to_write,
                                                 section->section_alignment);
            total_section_size_to_write += section->total_size;
        }

        file_size += total_section_size_to_write;
        file_size += (total_section_size_to_write / MAX_RECORD_SIZE
                      + !!(total_section_size_to_write % MAX_RECORD_SIZE)) * rlh_size;
        /* Loader table,
         * stores symbols with 8 byte name and 3 4 byte fields (so 20 bytes each),
         * sorted in descdending order.
         * It should be the last record in file but when too many symbols exist,
         * it needs to be split into multiple records.
         */
        for (of = all_object_files; of; of = of->next) {
            size_t i;

            for (i = 1; i < of->symbol_count; i++) {
                const struct symbol *symbol = of->symbol_array + i;

                if (!symbol->name && symbol->section_number != i) continue;
                if (symbol_is_undefined (symbol)) continue;
                
                num_symbols++;
            }
        }
        /* +2 symbols, SYSREF and NUCON. */
        file_size += (num_symbols + 2) * 20;
        file_size += ((num_symbols + 2) / MAX_RECORD_SYMS
                      + !!((num_symbols + 2) % MAX_RECORD_SYMS)) * rlh_size;

        /* Relocations have 4 bytes: 1 byte flags and 3 bytes address.
         * They seem to be base relocations,
         * so they do not need to reference the symbol table.
         */
        if (ld_state->emit_relocs) {
            num_relocs = get_num_relocs ();
            file_size += num_relocs * 4;
            file_size += (num_relocs / MAX_RECORD_RELOCS
                          + !!(num_relocs % MAX_RECORD_RELOCS)) * rlh_size;
        }
    }

    file = xcalloc (file_size, 1);

    /* 80 byte record with loader information. */
    pos = file;
    bytearray_write_2_bytes (pos, 80, BIG_ENDIAN);
    pos += rlh_size;
    /* Entry point with base address included. */
    bytearray_write_4_bytes (pos, ld_state->base_address + ld_state->entry_point, BIG_ENDIAN);
    /* Base address. */
    bytearray_write_4_bytes (pos + 4, ld_state->base_address, BIG_ENDIAN);
    /* 2-byte last text record index (1-based)
     * and 2-byte last text record size twice.
     */
    {
        unsigned short last_text_record_i;
        unsigned short last_text_record_size;

        last_text_record_i = (total_section_size_to_write / MAX_RECORD_SIZE
                              + !!(total_section_size_to_write % MAX_RECORD_SIZE)) + 1;
        last_text_record_size = total_section_size_to_write % MAX_RECORD_SIZE;
        bytearray_write_2_bytes (pos + 8, last_text_record_i, BIG_ENDIAN);
        bytearray_write_2_bytes (pos + 10, last_text_record_size, BIG_ENDIAN);
        bytearray_write_2_bytes (pos + 12, last_text_record_i, BIG_ENDIAN);
        bytearray_write_2_bytes (pos + 14, last_text_record_size, BIG_ENDIAN);
    }

    /* Unknown. */
    bytearray_write_4_bytes (pos + 16, 0x125D6, BIG_ENDIAN);
#if 0
    memcpy (pos + 28,
            "\x00\x00\x01\x61\x4C\x91\x00\x08\x00\x00\x00\x00\x00\x08\x00\x00",
            16);
#else
    memcpy (pos + 28,
            "\x00\x00\x01\x61\x4C\x91\x00\xEC\x00\x00\x00\x00\x00\xEC\x80\x00",
            16);
#endif
    {
        unsigned long znumber;
        unsigned char *saved_pos = pos;

        /* Perhaps arbitrary unknown number. */
#if 0
        znumber = 0x284;
#else
        znumber = 0x298;
#endif

        pos += 44;
        pos[0] = 0x2;
        pos[1] = tebc ('E');
        pos[2] = tebc ('S');
        pos[3] = tebc ('D');
        /* Regular ESD has 2 blank bytes
         * and 2 bytes with number of bytes of ESD data
         * at this spot but CMS executable instead has 3-byte znumber
         * (not sure what znumber is) and unknown byte.
         */
        bytearray_write_3_bytes (pos + 8, znumber, BIG_ENDIAN);
        pos[11] = 0xC;
        
        /* Not sure why is ESD identifier of first ESD item 0xB18 or 0xBA4. */
#if 0
        bytearray_write_2_bytes (pos + 14, 0xB18, BIG_ENDIAN);
#else
        bytearray_write_2_bytes (pos + 14, 0xBA4, BIG_ENDIAN);
#endif

        sprintf (pos + 16, "Z%06lx ", (unsigned long)znumber);
        {
            size_t i;
            for (i = 0; i < 8; i++) {
                pos[16 + i] = tebc (pos[16 + i]);
            }
        }

        pos = saved_pos;
    }
    /* Top bit of pos[44] is AMODE31 flag. */
    if (mainframe_get_amode () == 31) pos[44] |= 0x80;
    pos += 80;

    {
        unsigned char *tmp, *p;
        size_t size_to_write;
        
        p = tmp = xcalloc (total_section_size_to_write, 1);
        for (section = all_sections; section; section = section->next) {
            if (section->is_bss) continue;
            
            p = tmp + ALIGN (p - tmp, section->section_alignment);
            section_write (section, p);
            p += section->total_size;
        }

        size_to_write = total_section_size_to_write;
        while (size_to_write) {
            size_t this_size = size_to_write > MAX_RECORD_SIZE ? MAX_RECORD_SIZE : size_to_write;

            /* Record containg content of sections. */
            bytearray_write_2_bytes (pos, this_size, BIG_ENDIAN);
            pos += rlh_size;

            memcpy (pos,
                    tmp + total_section_size_to_write - size_to_write,
                    this_size);
            size_to_write -= this_size;
            pos += this_size;
        }

        free (tmp);
    }

    /* Loader table. */
    {
        unsigned long section_symbol_index = 284;
        size_t sym_i = 0;
        struct temp_sym *temp_syms = xmalloc (num_symbols * sizeof *temp_syms);
        size_t table_syms = num_symbols + 2;

        for (of = all_object_files; of; of = of->next) {
            size_t i;

            for (i = 1; i < of->symbol_count; i++) {
                struct symbol *symbol = of->symbol_array + i;

                if (!symbol->name && symbol->section_number != i) continue;
                if (symbol_is_undefined (symbol)) continue;

                temp_syms[sym_i].symbol = symbol;
                temp_syms[sym_i].address = symbol_get_value_with_base (symbol);
                sym_i++;
            }
        }

        qsort (temp_syms, num_symbols, sizeof *temp_syms, &temp_sym_compar);

        for (sym_i = 0; sym_i < num_symbols; sym_i++) {
            struct symbol *symbol = temp_syms[sym_i].symbol;

            if (sym_i % MAX_RECORD_SYMS == 0) {
                size_t this_syms = table_syms > MAX_RECORD_SYMS ? MAX_RECORD_SYMS : table_syms;
                bytearray_write_2_bytes (pos, this_syms * 20, BIG_ENDIAN);
                pos += rlh_size;
                table_syms -= this_syms;
            }
            
            if (symbol->section_number == symbol - symbol->part->of->symbol_array) {
                /* Section symbols have strange names like ".000284". */
                char name[9];

                /* snprintf() would be better but C90 does not have it. */
                sprintf (name, ".%06lu", section_symbol_index--);
                /* 8 bytes long EBDIC symbol name. */
                write_ebcdic_symbol_name (pos, name);

                /* Not sure what is the difference between those 3 fields.
                 * but the third one seems to be the absolute address of the entire section
                 * while the other two are absolute symbol values.
                 */
                bytearray_write_4_bytes (pos + 8, symbol_get_value_with_base (symbol), BIG_ENDIAN);
                bytearray_write_4_bytes (pos + 12, symbol_get_value_with_base (symbol), BIG_ENDIAN);
                bytearray_write_4_bytes (pos + 16, symbol->part->section->rva + ld_state->base_address, BIG_ENDIAN);
            } else {
                /* 8 bytes long EBDIC symbol name. */
                write_ebcdic_symbol_name (pos, symbol->name);

                bytearray_write_4_bytes (pos + 8, 0, BIG_ENDIAN);
                bytearray_write_4_bytes (pos + 12, symbol_get_value_with_base (symbol), BIG_ENDIAN);
                bytearray_write_4_bytes (pos + 16, 0, BIG_ENDIAN);
            }

            pos += 20;
        }

        /* SYSREF and NUCON symbols. */
        {
            const char *name;

            if (sym_i % MAX_RECORD_SYMS == 0) {
                size_t this_syms = table_syms > MAX_RECORD_SYMS ? MAX_RECORD_SYMS : table_syms;
                bytearray_write_2_bytes (pos, this_syms * 20, BIG_ENDIAN);
                pos += rlh_size;
                table_syms -= this_syms;
            }

            name = "SYSREF";
            write_ebcdic_symbol_name (pos, name);

            bytearray_write_4_bytes (pos + 8, 0x600, BIG_ENDIAN);
            bytearray_write_4_bytes (pos + 12, 0x600, BIG_ENDIAN);
            bytearray_write_4_bytes (pos + 16, 0, BIG_ENDIAN);
            pos += 20;
            sym_i++;

            if (sym_i % MAX_RECORD_SYMS == 0) {
                size_t this_syms = table_syms > MAX_RECORD_SYMS ? MAX_RECORD_SYMS : table_syms;
                bytearray_write_2_bytes (pos, this_syms * 20, BIG_ENDIAN);
                pos += rlh_size;
                table_syms -= this_syms;
            }

            name = "NUCON";
            write_ebcdic_symbol_name (pos, name);

            bytearray_write_4_bytes (pos + 8, 0, BIG_ENDIAN);
            bytearray_write_4_bytes (pos + 12, 0, BIG_ENDIAN);
            bytearray_write_4_bytes (pos + 16, 0, BIG_ENDIAN);
            pos += 20;
        }

        free (temp_syms);
    }

    if (ld_state->emit_relocs) write_relocs (pos, num_relocs);

    if (fwrite (file, file_size, 1, outfile) != 1) {
        ld_error ("writing '%s' file failed", filename);
    }
    
    free (file);
    fclose (outfile);
}
