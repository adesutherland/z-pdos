/* Simulate MinGW's function-backed error-table macros, after its declarations. */
#include_next <stdlib.h>
extern int *mfqa_sys_nerr(void);
extern char **mfqa_sys_errlist(void);
#undef sys_nerr
#undef sys_errlist
#define sys_nerr (*mfqa_sys_nerr())
#define sys_errlist (mfqa_sys_errlist())
