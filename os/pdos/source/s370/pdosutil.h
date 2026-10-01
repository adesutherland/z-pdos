/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pdosutil - utilities used by PDOS and possibly PLOAD             */
/*                                                                   */
/*********************************************************************/

int findFile(int ipldev, char *dsn, int *c, int *h, int *r);
int fixPE(char *buf, int *len, int *entry, int rlad, int capacity);
/* Return the native directory's AMODE code: 0=24, 1=64, 2=31, 3=ANY. */
int fixPEMode(char *buf, int *len, int *entry, int rlad, int capacity,
              int *module_amode, int *module_rmode_any);
int fixPEHigh(char *buf, int *len, int *entry_offset,
              unsigned int base_low, int capacity);
