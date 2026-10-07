/* SPDX-License-Identifier: MIT
 * Native PDPCLIB file cursor across LINK return or an unhandled child fault. */
#include <stdio.h>
#include <string.h>
extern int P3LINK(void);
#ifndef P3_EXPECT
#define P3_EXPECT 37
#endif
int main(int argc,char **argv)
{
    FILE *file;char line[16];
    (void)argc;(void)argv;
    file=fopen("D0IO.CUR","r");
    if(!file||!fgets(line,sizeof line,file)||strncmp(line,"ONE",3))return 39;
    if(P3LINK()!=P3_EXPECT)return 39;
    if(!fgets(line,sizeof line,file)||strncmp(line,"TWO",3))return 39;
    /* K reaps the remaining parent FILE, DD and allocations on SVC3. */
    return 37;
}
