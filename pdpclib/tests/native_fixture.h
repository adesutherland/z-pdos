#ifndef PDPCLIB_QA_NATIVE_FIXTURE_H
#define PDPCLIB_QA_NATIVE_FIXTURE_H
#include "stdio_namespace.h"
#include "../stdio.h"
#include "../stdlib.h"
#include "../string.h"
#include "../errno.h"
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
};
extern FILE *__stderr_ptr;
extern struct pdpqa_counts pdpqa_calls;
void pdpqa_reset(void);
int pdpqa_seeded_open(FILE *stream, int requested_mode);
int pdpqa_failure(const char *condition, int line);
#endif
