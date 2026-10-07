/* SPDX-License-Identifier: MIT
 * Native SVC3 or U fault leaves the FILE, DD and C heap owned by this child. */
#include <stdio.h>
#include <string.h>
extern void P3EXIT(void);
int main(int argc,char **argv)
{
    FILE *file;char line[16];
    (void)argc;(void)argv;
    file=fopen("D0IO.CUR","r");
    if(!file||!fgets(line,sizeof line,file)||strncmp(line,"ONE",3))return 39;
    P3EXIT();
    return 39;
}
