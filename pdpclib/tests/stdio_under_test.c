/* Compile the maintained source; these adapters exist only for host QA. */
#include "stdio_namespace.h"
#include "../stdio.h"
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
#include "../stdio.c"

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
