#include <stdio.h>
int pdpqa_failure(const char *condition, int line)
{
    fprintf(stderr, "PDPCLIB host QA line %d failed: %s\n", line, condition);
    return 1;
}
