/******************************************************************************
 * @file            xmit.h
 *
 * Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish, use, compile, sell and
 * distribute this work and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions, without
 * complying with any conditions and by any means.
 *****************************************************************************/
/* XMIT files are composed of segments with structure:
 * Length of segment (including the header), 1 byte
 * Segment descriptor flags, 1 byte
 * User data segment, 0 - 253 bytes
 */
#define SIZEOF_struct_segment_header_file 2
#define SEGMENT_FIRST_REC_SEG 0x80 /* First segment of original record. */
#define SEGMENT_LAST_REC_SEG  0x40 /* Last segment of original record. */
#define SEGMENT_CONTROL_REC   0x20 /* This is (part of) a control record. */
#define SEGMENT_REC_NUMBER    0x10 /* This is record number of next record. */

#define TEXT_UNIT_KEY_INMBLKSZ 0x0030 /* Block size */
#define TEXT_UNIT_KEY_INMCREAT 0x1022 /* Creation date */
#define TEXT_UNIT_KEY_INMDDNAM 0x0001 /* DDNAME for the file */
#define TEXT_UNIT_KEY_INMDIR   0x000C /* Number of directory blocks */
#define TEXT_UNIT_KEY_INMDSNAM 0x0002 /* Name of the file */
#define TEXT_UNIT_KEY_INMDSORG 0x003C /* File organization */
#define TEXT_UNIT_KEY_INMEATTR 0x8028 /* Extended attribute status */
#define TEXT_UNIT_KEY_INMERRCD 0x1027 /* RECEIVE command error code */
#define TEXT_UNIT_KEY_INMEXPDT 0x0022 /* Expiration date */
#define TEXT_UNIT_KEY_INMFACK  0x1026 /* Originator requested notification */
#define TEXT_UNIT_KEY_INMFFM   0x102D /* Filemode number */
#define TEXT_UNIT_KEY_INMFNODE 0x1011 /* Origin node name or node number */
#define TEXT_UNIT_KEY_INMFTIME 0x1024 /* Origin timestamp */
#define TEXT_UNIT_KEY_INMFUID  0x1012 /* Origin user ID */
#define TEXT_UNIT_KEY_INMFVERS 0x1023 /* Origin version number of the data format */
#define TEXT_UNIT_KEY_INMLCHG  0x1021 /* Date last changed */
#define TEXT_UNIT_KEY_INMLRECL 0x0042 /* Logical record length */
#define TEXT_UNIT_KEY_INMLREF  0x1020 /* Data last referenced */
#define TEXT_UNIT_KEY_INMLSIZE 0x8018 /* Data set size size in megabytes */
#define TEXT_UNIT_KEY_INMMEMBR 0x0003 /* Member name list */
#define TEXT_UNIT_KEY_INMNUMF  0x102F /* Number of files transmitted */
#define TEXT_UNIT_KEY_INMRECCT 0x102A /* Transmitted record count */
#define TEXT_UNIT_KEY_INMRECFM 0x0049 /* Record format */
#define TEXT_UNIT_KEY_INMSECND 0x000B /* Secondary space quantity */
#define TEXT_UNIT_KEY_INMSIZE  0x102C /* File size in bytes */
#define TEXT_UNIT_KEY_INMTERM  0x0028 /* Data transmitted as a message */
#define TEXT_UNIT_KEY_INMTNODE 0x1001 /* Target node name or node number */
#define TEXT_UNIT_KEY_INMTTIME 0x1025 /* Destination timestamp */
#define TEXT_UNIT_KEY_INMTUID  0x1002 /* Target user ID */
#define TEXT_UNIT_KEY_INMTYPE  0x8012 /* Data set type */
#define TEXT_UNIT_KEY_INMUSERP 0x1029 /* User parameter string */
#define TEXT_UNIT_KEY_INMUTILN 0x1028 /* Name of utility program */
