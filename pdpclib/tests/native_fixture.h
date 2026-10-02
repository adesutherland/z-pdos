#ifndef PDPCLIB_QA_NATIVE_FIXTURE_H
#define PDPCLIB_QA_NATIVE_FIXTURE_H
#include "stdio_namespace.h"
#include "../src/stdio.h"
#include "../src/stdlib.h"
#include "../src/string.h"
#include "../src/errno.h"
struct pdpqa_counts {
    int opens;
    int closes;
    int reads;
    int writes;
    int dcb_queries;
    int points;
    int unexpected;
    int last_mode;
    unsigned int last_ttr;
    int fail_open;
    int fail_malloc;
    int allocations;
    int svc99_allowed;
    int svc99_calls;
    int svc99_fail_first;
};
extern FILE *__stderr_ptr;
extern struct pdpqa_counts pdpqa_calls;
void pdpqa_reset(void);
int pdpqa_seeded_open(FILE *stream, int requested_mode);
int pdpqa_allocate(int output, int format);
void pdpqa_deallocate(void);
void *pdpqa_text_unit(int index);
int pdpqa_text_pointer(void *encoded, int index, int last);
int pdpqa_failure(const char *condition, int line);
#endif
