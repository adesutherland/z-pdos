/******************************************************************************
 * @file            xmit.c
 *
 * Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish, use, compile, sell and
 * distribute this work and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions, without
 * complying with any conditions and by any means.
 *****************************************************************************/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "ld.h"
#include "tebc.h"
#include "bytearray.h"
#include "mainframe.h"
#include "xmit.h"

address_type xmit_get_base_address (void)
{    
    return 0;
}

static void str_tebc (void *dst, const char *src)
{
    char *p = dst;

    while (*src) {
        p++[0] = tebc (src++[0]);
    }
}

size_t xmit_calculate_segments (size_t file_size, size_t start_file_size)
{
    size_t diff = file_size - start_file_size;

    while (diff) {
        size_t change;
        
        change = (diff >= 253) ? 0xff : (diff + 2);
        
        diff -= change - 2;
        start_file_size += change;
    }
    
    return start_file_size;
}

unsigned char *xmit_write_segments (const unsigned char *pos, unsigned char *start_pos)
{
    size_t diff = pos - start_pos;
    int first = 1;

    while (diff) {
        size_t change;

        memmove (start_pos + 2, start_pos, diff);

        start_pos[1] = 0;
        if (first) {
            first = 0;
            start_pos[1] |= SEGMENT_FIRST_REC_SEG;
        }
        
        change = (diff >= 253) ? 0xff : (diff + 2);
        
        if (change < 0xff) {
            start_pos[1] |= SEGMENT_LAST_REC_SEG;
        }

        start_pos[0] = change;
        
        diff -= change - 2;
        start_pos += change;
    }
    
    return start_pos;
}

/* Name of (temporary?) file, can probably be anything 8.3. */
#define TMP_FILE_NAME "TEMP01"
#define TMP_FILE_SUFFIX "ZZZ"

static char timestr[21];

static size_t header_record_size;
static size_t iebcopy_control_record_size;
static size_t inmcopy_control_record_size;
static size_t data_control_record_size;

size_t xmit_calculate_start (void)
{
    {
        time_t t;
        const struct tm *tm;

        const char *epoch = getenv ("SOURCE_DATE_EPOCH");

        if (epoch != NULL) {
            char *end;
            long long value;

            errno = 0;
            value = strtoll (epoch, &end, 10);
            if (errno != 0 || epoch == end || *end != '\0'
                || value < 0 || (long long)(time_t)value != value) {
                fprintf (stderr, "invalid SOURCE_DATE_EPOCH: %s\n", epoch);
                exit (EXIT_FAILURE);
            }
            t = (time_t)value;
            tm = gmtime (&t);
        } else {
            time (&t);
            tm = localtime (&t);
        }
        if (tm == NULL) {
            fprintf (stderr, "cannot format XMIT timestamp\n");
            exit (EXIT_FAILURE);
        }
        sprintf (timestr, "%i%02i%02i%02i%02i%02i",
                1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday,
                tm->tm_hour, tm->tm_min, tm->tm_sec);
    }
    

    /* XMIT control records. */
    header_record_size += (SIZEOF_struct_segment_header_file + 6 + 7 + (6 + sizeof "ORIGNODE" - 1)
                           + (6 + sizeof "ORIGUID" - 1) + (6 + sizeof "DESTNODE" - 1)
                           + (6 + sizeof "DESTUID" - 1) + (6 + strlen (timestr)) + 7);
    iebcopy_control_record_size = (SIZEOF_struct_segment_header_file + 10 + (6 + sizeof "IEBCOPY" - 1)
                                   + 10 + 8 + 7 + 10 + 10 + 8 + 9
                                   + (8 + sizeof TMP_FILE_NAME - 1 + sizeof TMP_FILE_SUFFIX - 1));
    inmcopy_control_record_size = (SIZEOF_struct_segment_header_file + 10 + (6 + sizeof "INMCOPY" - 1)
                                   + 10 + 8 + 10 + 10 + 8);
    data_control_record_size = SIZEOF_struct_segment_header_file + 6 + 10 + 8 + 8 + 8;
    
    return header_record_size + iebcopy_control_record_size + inmcopy_control_record_size + data_control_record_size;
}

size_t xmit_calculate_end (size_t file_size)
{
    /* Trailer control record. */
    file_size += SIZEOF_struct_segment_header_file + 6;
    /* TSO RECEIVE reads card images. Preserve each logical segment stream,
     * then finish the physical file on an 80-byte record boundary. */
    file_size = ALIGN (file_size, 80);

    return file_size;
}

unsigned char *xmit_write_start (unsigned char *pos, size_t chunk_size)
{
    /* Header record. */
    pos[0] = header_record_size; /* Record length, 1 byte. */
    pos[1] = SEGMENT_FIRST_REC_SEG | SEGMENT_LAST_REC_SEG | SEGMENT_CONTROL_REC;
    /* Record identifier, 6 bytes. */
    str_tebc (pos + 2, "INMR01");
    /* From byte 8 are text units. */
    pos += 8;
    /* Text units are composed of:
     * key, 2 bytes
     * number of data parts, 2 bytes
     * data parts
     *
     * data parts are composed of:
     * length of data, 2 bytes
     * data
     */
    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMLRECL, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, 1, BIG_ENDIAN); /* Length of following field. */
    bytearray_write_1_bytes (pos + 6, 80, BIG_ENDIAN); /* Logical record length, default 80. */
    pos += 7;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMFNODE, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, sizeof "ORIGNODE" - 1, BIG_ENDIAN);
    str_tebc (pos + 6, "ORIGNODE");
    pos += 6 + sizeof "ORIGNODE" - 1;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMFUID, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, sizeof "ORIGUID" - 1, BIG_ENDIAN);
    str_tebc (pos + 6, "ORIGUID");
    pos += 6 + sizeof "ORIGUID" - 1;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMTNODE, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, sizeof "DESTNODE" - 1, BIG_ENDIAN);
    str_tebc (pos + 6, "DESTNODE");
    pos += 6 + sizeof "DESTNODE" - 1;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMTUID, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, sizeof "DESTUID" - 1, BIG_ENDIAN);
    str_tebc (pos + 6, "DESTUID");
    pos += 6 + sizeof "DESTUID" - 1;
        
    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMFTIME, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, strlen (timestr), BIG_ENDIAN);
    str_tebc (pos + 6, timestr);
    pos += 6 + strlen (timestr);

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMNUMF, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, 1, BIG_ENDIAN);
    bytearray_write_1_bytes (pos + 6, 1, BIG_ENDIAN); /* Number of files. */
    pos += 7;

    /* File utility control record, first for IEBCOPY. */
    pos[0] = iebcopy_control_record_size;
    pos[1] = SEGMENT_FIRST_REC_SEG | SEGMENT_LAST_REC_SEG | SEGMENT_CONTROL_REC;
    str_tebc (pos + 2, "INMR02");
    /* The number of the file in transmission to which this record applies to, 4 bytes. */
    bytearray_write_4_bytes (pos + 8, 1, BIG_ENDIAN);
    pos += 12;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMUTILN, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, sizeof "IEBCOPY" - 1, BIG_ENDIAN);
    str_tebc (pos + 6, "IEBCOPY");
    pos += 6 + sizeof "IEBCOPY" - 1;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMSIZE, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, 4, BIG_ENDIAN);
    /* +++Supposedly file size in bytes but it seems to be too big. */
    bytearray_write_4_bytes (pos + 6, 0x000D747E, BIG_ENDIAN); /* File size in bytes. */
    pos += 10;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMDSORG, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, 2, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 6, 0x0200, BIG_ENDIAN); /* File organization, not sure how to determine. */
    pos += 8;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMTYPE, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, 1, BIG_ENDIAN);
    bytearray_write_1_bytes (pos + 6, 0, BIG_ENDIAN); /* Data set type, not sure how to determine. */
    pos += 7;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMLRECL, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, 4, BIG_ENDIAN);
    bytearray_write_4_bytes (pos + 6, 256, BIG_ENDIAN); /* Logical record length, not sure how to determine. */
    pos += 10;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMBLKSZ, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, 4, BIG_ENDIAN);
    bytearray_write_4_bytes (pos + 6, chunk_size, BIG_ENDIAN); /* Block/chunk size. */
    pos += 10;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMRECFM, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, 2, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 6, 0xC000, BIG_ENDIAN); /* Record format, not sure how to determine. */
    pos += 8;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMDIR, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, 3, BIG_ENDIAN);
    /* +++Perhaps this should be automatically determined? */
    bytearray_write_3_bytes (pos + 6, 0x28, BIG_ENDIAN); /* Number of directory blocks. */
    pos += 9;

    /* Name of the (temporary?) file, contains 2 EBCDIC strings, first for name and another for suffix. */
    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMDSNAM, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 2, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, sizeof TMP_FILE_NAME - 1, BIG_ENDIAN);
    str_tebc (pos + 6, TMP_FILE_NAME);
    pos += 6 + sizeof TMP_FILE_NAME - 1;
    bytearray_write_2_bytes (pos, sizeof TMP_FILE_SUFFIX - 1, BIG_ENDIAN);
    str_tebc (pos + 2, TMP_FILE_SUFFIX);
    pos += 2 + sizeof TMP_FILE_SUFFIX - 1;

    /* File utility control record, second for INMCOPY. */
    pos[0] = inmcopy_control_record_size;
    pos[1] = SEGMENT_FIRST_REC_SEG | SEGMENT_LAST_REC_SEG | SEGMENT_CONTROL_REC;
    str_tebc (pos + 2, "INMR02");
    /* The number of the file in transmission to which this record applies to, 4 bytes. */
    bytearray_write_4_bytes (pos + 8, 1, BIG_ENDIAN);
    pos += 12;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMUTILN, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, sizeof "INMCOPY" - 1, BIG_ENDIAN);
    str_tebc (pos + 6, "INMCOPY");
    pos += 6 + sizeof "INMCOPY" - 1;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMSIZE, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, 4, BIG_ENDIAN);
    /* +++Supposedly file size in bytes but it seems to be too big. */
    bytearray_write_4_bytes (pos + 6, 0x000D747E, BIG_ENDIAN); /* File size in bytes. */
    pos += 10;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMDSORG, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, 2, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 6, 0x4000, BIG_ENDIAN); /* File organization, not sure how to determine. */
    pos += 8;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMLRECL, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, 4, BIG_ENDIAN);
    bytearray_write_4_bytes (pos + 6, chunk_size + 16, BIG_ENDIAN); /* Logical record length, not sure why chunk_size + 16. */
    pos += 10;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMBLKSZ, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, 4, BIG_ENDIAN);
    bytearray_write_4_bytes (pos + 6, chunk_size + 20, BIG_ENDIAN); /* Block/chunk size, not sure why chunk_size + 20. */
    pos += 10;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMRECFM, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, 2, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 6, 0x4802, BIG_ENDIAN); /* Record format, not sure how to determine. */
    pos += 8;

    /* Data control record. */
    pos[0] = data_control_record_size;
    pos[1] = SEGMENT_FIRST_REC_SEG | SEGMENT_LAST_REC_SEG | SEGMENT_CONTROL_REC;
    str_tebc (pos + 2, "INMR03");
    pos += 8;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMSIZE, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, 4, BIG_ENDIAN);
    /* +++Supposedly file size in bytes but it seems to be too big. */
    bytearray_write_4_bytes (pos + 6, 0x000D747E, BIG_ENDIAN); /* File size in bytes. */
    pos += 10;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMDSORG, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, 2, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 6, 0x4000, BIG_ENDIAN); /* File organization, not sure how to determine. */
    pos += 8;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMLRECL, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, 2, BIG_ENDIAN); /* Not sure why previous logical records use length 4 but this one 2. */
    bytearray_write_2_bytes (pos + 6, 256, BIG_ENDIAN); /* Logical record length, not sure how to determine. */
    pos += 8;

    bytearray_write_2_bytes (pos, TEXT_UNIT_KEY_INMRECFM, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 2, 1, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 4, 2, BIG_ENDIAN);
    bytearray_write_2_bytes (pos + 6, 0x1, BIG_ENDIAN); /* Record format, not sure how to determine. */
    pos += 8;

    return pos;
}

unsigned char *xmit_write_end (unsigned char *file, unsigned char *pos, size_t file_size)
{
    /* Trailer control record, last record, confirms transmission. No text units. */
    pos[0] = 8;
    pos[1] = SEGMENT_FIRST_REC_SEG | SEGMENT_LAST_REC_SEG | SEGMENT_CONTROL_REC;
    str_tebc (pos + 2, "INMR06");
    pos += 8;

    /* Pad the last physical card with EBCDIC blanks. */
    while (pos < file + file_size) pos++[0] = tebc (' ');

    return pos;
}

