/******************************************************************************
 * @file            z390.c
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
#include "bytearray.h"
#include "xmalloc.h"

address_type z390_get_base_address (void)
{    
    return 0;
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
                if (part->relocation_array[i].howto == &reloc_howtos[RELOC_TYPE_64]
                    || part->relocation_array[i].howto == &reloc_howtos[RELOC_TYPE_32]
                    || part->relocation_array[i].howto == &reloc_howtos[RELOC_TYPE_24]) {
                    num_relocs++;
                }
            }
        }
    }

    return num_relocs;
}

static void write_relocs (unsigned char *pos)
{
    struct section *section;

    for (section = all_sections; section; section = section->next) {
        struct section_part *part;
        
        for (part = section->first_part; part; part = part->next) {
            size_t i;
            
            for (i = 0; i < part->relocation_count; i++) {
                if (part->relocation_array[i].howto == &reloc_howtos[RELOC_TYPE_64]
                    || part->relocation_array[i].howto == &reloc_howtos[RELOC_TYPE_32]
                    || part->relocation_array[i].howto == &reloc_howtos[RELOC_TYPE_24]) {
                    /* Relocation offset, 4 bytes. */
                    bytearray_write_4_bytes (pos, part->rva + part->relocation_array[i].offset, BIG_ENDIAN);
                    /* Relocation size, 1 byte. */
                    bytearray_write_1_bytes (pos + 4, part->relocation_array[i].howto->size, BIG_ENDIAN);
                    pos += 5;
                }
            }
        }
    }
}

void z390_write (const char *filename)
{
    FILE *outfile;
    unsigned char *file;
    size_t file_size;
    unsigned char *pos;

    struct section *section;

    size_t num_relocs;
    size_t total_section_size_to_write = 0;
    int amode = mainframe_get_amode ();
    int rmode = mainframe_get_rmode ();

    if (!(outfile = fopen (filename, "wb"))) {
        ld_error ("cannot open '%s' for writing", filename);
        return;
    }

    file_size = 20;
    for (section = all_sections; section; section = section->next) {
        if (!section->is_bss) total_section_size_to_write += section->total_size;
    }
    file_size += total_section_size_to_write;

    num_relocs = get_num_relocs ();
    file_size += num_relocs * 5;

    file = xcalloc (file_size, 1);

    pos = file;
    /* Executable header, 20 bytes. */
    /* Format version, 4 bytes. */
    memcpy (pos, "\x31\x30\x30\x32", 4); /* ASCII string "1002". */
    /* AMODE 31 'T'rue/'F'alse (in ASCII), 1 byte. */
    if (amode == 31) {
        pos[4] = tasc ('T');
    } else {
        pos[4] = tasc ('F');
    }
    /* RMODE ANY/31 'T'rue/'F'alse (in ASCII), 1 byte. */
    if (rmode == 0) {
        pos[5] = tasc ('T');
    } else {
        pos[5] = tasc ('F');
    }
    /* Reserved, should be ASCII "??", 2 bytes. */
    pos[6] = tasc ('?');
    pos[7] = tasc ('?');
    /* Total size of all sections/code, 4 bytes. */
    bytearray_write_4_bytes (pos + 8, total_section_size_to_write, BIG_ENDIAN);
    /* Entry point offset, 4 bytes. */
    bytearray_write_4_bytes (pos + 12, ld_state->entry_point, BIG_ENDIAN);
    /* Number of relocation entries at the end of file, 4 bytes. */
    bytearray_write_4_bytes (pos + 16, num_relocs, BIG_ENDIAN);
    pos += 20;
    
    for (section = all_sections; section; section = section->next) {
        if (section->is_bss) continue;

        section_write (section, pos);
        pos += section->total_size;
    }

    /* At the end of file there are 5-byte (4-byte offset, 1-byte relocation size) relocation entries. */
    write_relocs (pos);
    
    if (fwrite (file, file_size, 1, outfile) != 1) {
        ld_error ("writing '%s' file failed", filename);
    }
    
    free (file);
    fclose (outfile);
}
