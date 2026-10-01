/* SPDX-License-Identifier: MIT */
#pragma map(localtime, "LOCTIME")
extern int localtime(int);
extern int codec_stream_decode(int);
int codec_stream_encode(int x)
{
    return codec_stream_decode(x) + localtime(x);
}
static const unsigned char retained_table[] = { 3, 7, 11 };
const unsigned char *retained_pointer = retained_table;
