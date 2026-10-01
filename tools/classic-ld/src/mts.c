/******************************************************************************
 * @file            mts.c
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
#include <ctype.h>

#include "ld.h"
#include "tebc.h"
#include "bytearray.h"
#include "xmalloc.h"
#include "mainframe.h"

#define RECORD_SIZE 80
#define MAX_TXT_IN_BYTES 56
#define MAX_RLD_IN_BYTES 56
#define MAX_RLD_RELOCS (MAX_RLD_IN_BYTES / 8)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

static size_t num_undefined_symbols;
static struct symbol **undefined_symbols;

address_type mts_get_base_address (void)
{    
    return 0;
}

static unsigned char *write_ebcdic_str_no_null (unsigned char *pos, const char *str)
{
    size_t i;

    for (i = 0; str[i]; i++) {
        pos[i] = tebc (str[i]);
    }

    return pos + i;
}

static unsigned char *terminate_record (unsigned char *file, size_t *record_i_p)
{
    unsigned char *pos = file + (*record_i_p) * RECORD_SIZE - 4;
    char text[5];

    sprintf (text, "%04lu", (unsigned long)*record_i_p);
    *record_i_p += 1;

    return write_ebcdic_str_no_null (pos, text);
}

static void undefined_symbol_callback (struct symbol *symbol)
{
    if (!symbol_is_undefined (symbol)) return;
    num_undefined_symbols++;
}

static void undefined_symbol_callback2 (struct symbol *symbol)
{
    if (!symbol_is_undefined (symbol)) return;
    undefined_symbols[num_undefined_symbols++] = symbol;
}

static unsigned char *write_symbols (unsigned char *file,
                                     unsigned char *pos,
                                     size_t *record_i_p,
                                     size_t total_section_size_to_write)
{
    unsigned short esdid;

    /* Symbol table,
     * first symbol is SD symbol with program name,
     * other symbols are undefined ER symbols.
     */
    pos[0] = 0x2;
    write_ebcdic_str_no_null (pos + 1, "ESD");
    bytearray_write_2_bytes (pos + 10, MIN (3, 1 + num_undefined_symbols) * 16, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 14, 1, BIG_ENDIAN);
    pos += 16;
    
    for (esdid = 1; esdid - 1 < 1 + num_undefined_symbols; esdid++) {
        if (esdid % 3 == 1 && esdid != 1) {
            pos = terminate_record (file, record_i_p);

            pos[0] = 0x2;
            write_ebcdic_str_no_null (pos + 1, "ESD");
            bytearray_write_2_bytes (pos + 10,
                                     MIN (3, num_undefined_symbols - (esdid - 2)) * 16,
                                     BIG_ENDIAN);
            bytearray_write_2_bytes (pos + 14, esdid, BIG_ENDIAN);
            pos += 16;
        }
        
        if (esdid == 1) {
            size_t i;

            /* 8 bytes long EBDIC uppercase program name. */
            for (i = 0; i < 8 && ld_state->output_filename[i]; i++) {
                if (i > 0 && ld_state->output_filename[i] == '.') break;
                pos[i] = tebc (toupper ((unsigned char)ld_state->output_filename[i]));
            }
            pos[8] = ESD_DATA_TYPE_SD;
            bytearray_write_3_bytes (pos + 9, ld_state->base_address, BIG_ENDIAN);
            bytearray_write_3_bytes (pos + 13, total_section_size_to_write, BIG_ENDIAN);
        } else {
            size_t i;
            struct symbol *symbol = undefined_symbols[esdid - 2];

            symbol->value = esdid;

            /* 8 bytes long EBDIC uppercase symbol name. */
            for (i = 0; i < 8 && symbol->name[i]; i++) {
                pos[i] = tebc (toupper ((unsigned char)symbol->name[i]));
            }
            pos[8] = ESD_DATA_TYPE_ER;
            bytearray_write_3_bytes (pos + 9, 0, BIG_ENDIAN);
        }
        pos += 16;
    }
    
    pos = terminate_record (file, record_i_p);

    return pos;
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

static unsigned char *write_relocs (unsigned char *file,
                                    unsigned char *pos,
                                    size_t *record_i_p,
                                    size_t num_relocs)
{
    struct section *section;
    size_t this_relocs;

    if (!num_relocs) return pos;

    this_relocs = num_relocs > MAX_RLD_RELOCS ? MAX_RLD_RELOCS : num_relocs;
    num_relocs -= this_relocs;

    pos[0] = 0x2;
    write_ebcdic_str_no_null (pos + 1, "RLD");
    bytearray_write_2_bytes (pos + 10, this_relocs * 8, BIG_ENDIAN);
    pos += 16;
    
    for (section = all_sections; section; section = section->next) {
        struct section_part *part;
        
        for (part = section->first_part; part; part = part->next) {
            size_t i;
            
            for (i = 0; i < part->relocation_count; i++) {
                unsigned char flags;
                struct symbol *symbol;
                
                if (part->relocation_array[i].howto == &reloc_howtos[RELOC_TYPE_IGNORED]) {
                    continue;
                }

                if (part->relocation_array[i].howto == &reloc_howtos[RELOC_TYPE_32]) {
                    flags = 0xC;
                } else if (part->relocation_array[i].howto == &reloc_howtos[RELOC_TYPE_24]) {
                    flags = 0x8;
                } else continue;

                if (this_relocs == 0) {
                    pos = terminate_record (file, record_i_p);
                    if (!num_relocs) return pos;

                    this_relocs = num_relocs > MAX_RLD_RELOCS ? MAX_RLD_RELOCS : num_relocs;
                    num_relocs -= this_relocs;
                    
                    pos[0] = 0x2;
                    write_ebcdic_str_no_null (pos + 1, "RLD");
                    bytearray_write_2_bytes (pos + 10, this_relocs * 8, BIG_ENDIAN);
                    pos += 16;
                }

                symbol = part->relocation_array[i].symbol;
                if (symbol_is_undefined (symbol)) {
                    symbol = symbol_find (symbol->name);
                    if (less_strict_mainframe_matching
                        && symbol_is_undefined (symbol)) {
                        symbol = mainframe_symbol_find (symbol->name);
                    }
                    if (symbol_is_undefined (symbol)) {
                        bytearray_write_2_bytes (pos, symbol->value, BIG_ENDIAN);
                    } else {
                        bytearray_write_2_bytes (pos, 1, BIG_ENDIAN);
                    }
                } else {
                    bytearray_write_2_bytes (pos, 1, BIG_ENDIAN);
                }
                bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
                pos += 4;

                pos[0] = flags;
                bytearray_write_3_bytes (pos + 1,
                                         ld_state->base_address + part->rva + part->relocation_array[i].offset,
                                         BIG_ENDIAN);
                pos += 4;
                this_relocs--;
            }
        }
    }

    pos = terminate_record (file, record_i_p);

    return pos;
}

void mts_write (const char *filename)
{
    FILE *outfile;
    unsigned char *file;
    size_t file_size;
    unsigned char *pos;

    struct section *section;
    size_t total_section_size_to_write = 0;
    size_t record_i = 1;
    size_t num_relocs = 0;
    
    if (!(outfile = fopen (filename, "wb"))) {
        ld_error ("cannot open '%s' for writing", filename);
        return;
    }

    {
        file_size = 0;

        /* Symbol table. */
        symbols_for_each_global (&undefined_symbol_callback);
        undefined_symbols = xmalloc (sizeof *undefined_symbols * num_undefined_symbols);
        num_undefined_symbols = 0;
        symbols_for_each_global (&undefined_symbol_callback2);

        file_size += ALIGN (1 + num_undefined_symbols, 3) / 3 * RECORD_SIZE;

        for (section = all_sections; section; section = section->next) {
            if (section->is_bss) continue;
            
            total_section_size_to_write += section->total_size;
            file_size += (section->total_size / MAX_TXT_IN_BYTES
                          + !!(section->total_size % MAX_TXT_IN_BYTES)) * RECORD_SIZE;
        }

        num_relocs = get_num_relocs ();
        file_size += (num_relocs / MAX_RLD_RELOCS
                      + !!(num_relocs % MAX_RLD_RELOCS)) * RECORD_SIZE;

        /* END record. */
        file_size += RECORD_SIZE;
    }
    
    file = xmalloc (file_size);
    /* Instead of '\0' EBCDIC ' ' should be used to fill empty space. */
    memset (file, tebc (' '), file_size);

    pos = file;

    pos = write_symbols (file, pos, &record_i, total_section_size_to_write);

    for (section = all_sections; section; section = section->next) {
        unsigned char *tmp;
        size_t section_size;
        
        if (section->is_bss) continue;

        tmp = xcalloc (section->total_size, 1);
        section_write (section, tmp);

        section_size = section->total_size;
        while (section_size) {
            size_t this_size = section_size > MAX_TXT_IN_BYTES ? MAX_TXT_IN_BYTES : section_size;

            pos[0] = 0x2;
            write_ebcdic_str_no_null (pos + 1, "TXT");

            bytearray_write_3_bytes (pos + 5, ld_state->base_address + section->total_size - section_size, BIG_ENDIAN);
            bytearray_write_2_bytes (pos + 10, this_size, BIG_ENDIAN);
            bytearray_write_2_bytes (pos + 14, 1, BIG_ENDIAN);
            pos += 16;

            memcpy (pos,
                    tmp + section->total_size - section_size,
                    this_size);
            section_size -= this_size;
            pos = terminate_record (file, &record_i);
        }

        free (tmp);
    }

    pos = write_relocs (file, pos, &record_i, num_relocs);

    /* END record. */
    pos[0] = 0x2;
    write_ebcdic_str_no_null (pos + 1, "END");
    /* Entry point. */
    bytearray_write_3_bytes (pos + 5, ld_state->base_address + ld_state->entry_point, BIG_ENDIAN);
    /* ESD ID of SD for the section with entry point. */
    bytearray_write_2_bytes (pos + 14, 1, BIG_ENDIAN);
    
    pos = terminate_record (file, &record_i);

    {
        size_t i;

        for (i = 0; i < num_undefined_symbols; i++) {
            undefined_symbols[i]->value = 0;
        }
        free (undefined_symbols);
    }

    if (fwrite (file, file_size, 1, outfile) != 1) {
        ld_error ("writing '%s' file failed", filename);
    }
    
    free (file);
    fclose (outfile);
}
