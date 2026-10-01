/******************************************************************************
 * @file            musicsp.c
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
#include <time.h>

#include "ld.h"
#include "tebc.h"
#include "bytearray.h"
#include "xmalloc.h"

#define FIXED_RECORD_LEN 128
#define TIMESTR_LEN 8
#define EXE_HEADER_SIZE 0x200
#define CODE_HEADER_SIZE 0x20

address_type musicsp_get_base_address (void)
{
    /* 0x4a00 + CODE_HEADER_SIZE
     * as the code header is considered part of code.
     */
    return 0x4a20;
}

void musicsp_write (const char *filename)
{
    FILE *outfile;
    unsigned char *file;
    size_t file_size;
    unsigned char *pos;

    struct section *section;
    char timestr[TIMESTR_LEN + 1] = {0};

    size_t total_section_size_to_write = 0;

    if (!(outfile = fopen (filename, "wb"))) {
        ld_error ("cannot open '%s' for writing", filename);
        return;
    }

    {
        time_t timestamp;
        const struct tm *tm;
        size_t i;
        
        timestamp = time (NULL);
        
        tm = localtime (&timestamp);
        i = strftime (timestr, TIMESTR_LEN + 1, "%d%b%y", tm);
        memset (timestr + i, ' ', TIMESTR_LEN - i);
        for (i = 0; i < TIMESTR_LEN; i++) {
            timestr[i] = tebc (toupper ((unsigned char)timestr[i]));
        }
    }

    {
        file_size = EXE_HEADER_SIZE;
        
        file_size += CODE_HEADER_SIZE;
        for (section = all_sections; section; section = section->next) {
            if (section->is_bss) continue;
            
            total_section_size_to_write += section->total_size;
        }
        file_size += total_section_size_to_write;

        /* Executable must be composed of whole fixed records. */
        file_size = ALIGN (file_size, FIXED_RECORD_LEN);
    }

    file = xcalloc (file_size, 1);

    pos = file;
    
    /* Executable header, takes up 512 bytes but only 48 bytes are filled. */
    {
        size_t i;

        /* 8 bytes long EBDIC uppercase program name. */
        for (i = 0; i < 8 && ld_state->output_filename[i]; i++) {
            if (i > 0 && ld_state->output_filename[i] == '.') break;
            pos[i] = tebc (toupper ((unsigned char)ld_state->output_filename[i]));
        }
        memset (pos + i, '\x40', 8 - i);
    }
    /* Unknown, 1 byte. */
    pos[8] = 0x1;
    /* Base address with code header size subtracted, 3 bytes. */
    bytearray_write_3_bytes (pos + 9, ld_state->base_address - CODE_HEADER_SIZE, BIG_ENDIAN);
    /* Unknown, 1 byte. */
    pos[12] = 0x80;
    /* Size of sections with code header, 3 bytes. */
    bytearray_write_3_bytes (pos + 13, total_section_size_to_write + CODE_HEADER_SIZE, BIG_ENDIAN);
    /* Unknown, 1 byte. */
    pos[16] = 0;
    /* Entry point, 3 bytes. */
    bytearray_write_3_bytes (pos + 17, ld_state->base_address + ld_state->entry_point, BIG_ENDIAN);
    /* Unknown, 2 bytes. */
    bytearray_write_2_bytes (pos + 20, 2, BIG_ENDIAN);
    /* Unknown, 2 bytes. */
    bytearray_write_2_bytes (pos + 22, 2, BIG_ENDIAN);
    /* Unknown, 1 byte. */
    pos[24] = 0;
    /* Something very similar to size of sections with code header, 3 bytes. */
    bytearray_write_3_bytes (pos + 25, total_section_size_to_write + CODE_HEADER_SIZE, BIG_ENDIAN);
    /* Unknown, differs between executables, 1 byte. */
    pos[28] = 0x4;
    /* Base address with code header size subtracted, 3 bytes. */
    bytearray_write_3_bytes (pos + 29, ld_state->base_address - CODE_HEADER_SIZE, BIG_ENDIAN);
    /* Unknown, 8 bytes. */
    memset (pos + 32, 0xff, 8);
    /* Timestamp in format "01JAN70 ", 8 bytes. */
    memcpy (pos + 40, timestr, TIMESTR_LEN);
    pos += EXE_HEADER_SIZE;

    /* Code header, 32 bytes. */
    /* Unknown, 1 byte. */
    pos[0] = 0x1;
    /* Base address with code header size subtracted, 3 bytes. */
    bytearray_write_3_bytes (pos + 5, ld_state->base_address - CODE_HEADER_SIZE, BIG_ENDIAN);
    /* Unknown, 1 byte. */
    pos[8] = 0x8;
    /* Size of sections with code header, 3 bytes. */
    bytearray_write_3_bytes (pos + 9, total_section_size_to_write + CODE_HEADER_SIZE, BIG_ENDIAN);
    /* Unknown, 1 byte. */
    pos[12] = 0;
    /* Something very similar to size of sections with code header, 3 bytes. */
    bytearray_write_3_bytes (pos + 13, total_section_size_to_write + CODE_HEADER_SIZE, BIG_ENDIAN);
    /* Unknown, 1 byte. */
    pos[16] = 0x2;
    /* Base address with code header size subtracted, 3 bytes. */
    bytearray_write_3_bytes (pos + 21, ld_state->base_address - CODE_HEADER_SIZE, BIG_ENDIAN);
    /* Unknown, 4 bytes. */
    memset (pos + 24, 0xff, 4);
    /* Unknown, 4 bytes. */
    memset (pos + 28, 0, 4);
    pos += CODE_HEADER_SIZE;
    
    for (section = all_sections; section; section = section->next) {
        if (section->is_bss) continue;

        section_write (section, pos);
        pos += section->total_size;
    }
    
    if (fwrite (file, file_size, 1, outfile) != 1) {
        ld_error ("writing '%s' file failed", filename);
    }
    
    free (file);
    fclose (outfile);
}
