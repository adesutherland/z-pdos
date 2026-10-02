/* Compile the maintained source; these adapters exist only for host QA. */
#include "stdio_namespace.h"
#include "../src/stdio.h"
/* stdio.c undefines these macro names before defining the helper functions.
   Compiler symbol labels preserve the host namespace without editing it. */
int (getc)(FILE *stream) __asm__("pdpqa_getc");
int (putc)(int c, FILE *stream) __asm__("pdpqa_putc");
int (getchar)(void) __asm__("pdpqa_getchar");
int (putchar)(int c) __asm__("pdpqa_putchar");
int (feof)(FILE *stream) __asm__("pdpqa_feof");
int (ferror)(FILE *stream) __asm__("pdpqa_ferror");
#define malloc pdpqa_malloc
#define free pdpqa_free
/* These historical services have no declaration in upstream mvssupa.h. */
void __adcba(void *handle, unsigned int *parm, unsigned int *data);
void __apoint(void *handle, unsigned int *ttr);
#include "../src/stdio.c"

int pdpqa_seeded_open(FILE *stream, int requested_mode)
{
    myfile = stream;
    fnm = "dd:input";
    modeType = requested_mode;
    inseek = 0;
    err = 0;
    osfopen();
    return err;
}

int pdpqa_allocate(int output, int format)
{
    err = 0;
    modeType = format;
    filedef("INPUT   ", "INPUT.DAT", output);
    return err;
}

void pdpqa_deallocate(void)
{
    fdclr("INPUT   ");
}

void *pdpqa_text_unit(int index)
{
    return &tu[index];
}

int pdpqa_text_pointer(void *encoded, int index, int last)
{
    unsigned long expected = (unsigned long)&tu[index];
    if (last) expected |= 0x80000000UL;
    return (unsigned long)encoded == expected;
}
