/* Host substitutes for native services, not an implementation of those services. */
#include "native_fixture.h"
struct pdpqa_counts pdpqa_calls;
int __tso;
int __runnum;
static int error_number;
static char record[64];
static unsigned short classification[256];
static short uppercase[256];
unsigned short *__isbuf = classification;
short *__toup = uppercase;

int *_errno(void)
{
    return &error_number;
}

void pdpqa_reset(void)
{
    int i;
    int outstanding = pdpqa_calls.allocations;
    memset(&pdpqa_calls, 0, sizeof pdpqa_calls);
    pdpqa_calls.allocations = outstanding;
    error_number = 0;
    for (i = 0; i < 256; ++i) {
        uppercase[i] = (short)i;
        if (i >= 'a' && i <= 'z') uppercase[i] = (short)(i - 'a' + 'A');
    }
}

void *pdpqa_malloc(size_t size)
{
    void *result;
    if (pdpqa_calls.fail_malloc) return NULL;
    /* Supply a stale flag so checkMode must initialize it before osfopen. */
    result = calloc(1, size);
    if (result != NULL && size == sizeof(FILE)) ((FILE *)result)->update = 1;
    if (result != NULL) ++pdpqa_calls.allocations;
    return result;
}

void pdpqa_free(void *ptr)
{
    if (ptr != NULL) --pdpqa_calls.allocations;
    free(ptr);
}

void *__aopen(const char *ddname, int *mode, int *recfm,
              int *lrecl, int *blksize, void **asmbuf, const char *member)
{
    ++pdpqa_calls.opens;
    pdpqa_calls.last_mode = *mode;
    if (strncmp(ddname, "INPUT   ", 8) != 0
        && strncmp(ddname, "TRACE   ", 8) != 0) ++pdpqa_calls.unexpected;
    if (member != NULL) ++pdpqa_calls.unexpected;
    if (pdpqa_calls.fail_open) return NULL;
    *mode |= 0x80 << 16;
    *recfm = 0;
    *lrecl = 8;
    *blksize = 8;
    *asmbuf = record;
    return (void *)1;
}

void __aclose(void *handle)
{
    if (handle != (void *)1) ++pdpqa_calls.unexpected;
    ++pdpqa_calls.closes;
}

int __aread(void *handle, void *buf, size_t *len)
{
    if (handle != (void *)1) ++pdpqa_calls.unexpected;
    ++pdpqa_calls.reads;
    *(unsigned char **)buf = (unsigned char *)record;
    *len = 0;
    return 1; /* Controlled end of file. */
}

int __awrite(void *handle, unsigned char **buf, size_t *len)
{
    (void)buf;
    (void)len;
    if (handle != (void *)1) ++pdpqa_calls.unexpected;
    ++pdpqa_calls.writes;
    return 0;
}

void __adcba(void *handle, unsigned int *parm, unsigned int *data)
{
    if (handle != (void *)1 || *parm != 2) ++pdpqa_calls.unexpected;
    ++pdpqa_calls.dcb_queries;
    memset(data, 0, 7 * sizeof *data);
    data[0] = 4;
}

void __apoint(void *handle, unsigned int *ttr)
{
    if (handle != (void *)1) ++pdpqa_calls.unexpected;
    ++pdpqa_calls.points;
    pdpqa_calls.last_ttr = *ttr;
}

int __dynal(size_t ddn_len, char *ddn, size_t dsn_len, char *dsn)
{
    (void)ddn_len; (void)ddn; (void)dsn_len; (void)dsn;
    ++pdpqa_calls.unexpected;
    return 1;
}

int __idcams(size_t len, char *data)
{
    (void)len; (void)data;
    ++pdpqa_calls.unexpected;
    return 1;
}

char *__getepf(char *dsn)
{
    (void)dsn;
    ++pdpqa_calls.unexpected;
    return NULL;
}

int __svc99(void *request)
{
    struct request_block {
        char length, verb, flags[2];
        short error, information;
        void **list;
    };
    struct text_unit {
        short key, count, length;
        unsigned char bytes[98];
    };
    struct request_block *rb = (struct request_block *)request;
    struct text_unit *unit;
    static const short keys[5] = {1, 2, 4, 0x49, 0x42};
    int i, count;
    if (!pdpqa_calls.svc99_allowed) {
        ++pdpqa_calls.unexpected;
        return 1;
    }
    ++pdpqa_calls.svc99_calls;
    if (rb->length != 20 || (rb->verb != 1 && rb->verb != 2))
        ++pdpqa_calls.unexpected;
    count = rb->verb == 2 ? 1 : 3;
    if (rb->verb == 1) {
        unit = (struct text_unit *)pdpqa_text_unit(2);
        if (unit->bytes[0] == 4 && pdpqa_calls.svc99_allowed == 2)
            count = 5;
        if (unit->bytes[0] != (pdpqa_calls.svc99_calls == 1 ? 8 : 4))
            ++pdpqa_calls.unexpected;
    }
    for (i = 0; i < count; ++i) {
        if (!pdpqa_text_pointer(rb->list[i], i, i == count - 1))
            ++pdpqa_calls.unexpected;
        unit = (struct text_unit *)pdpqa_text_unit(i);
        if (unit->key != keys[i] || unit->count != 1)
            ++pdpqa_calls.unexpected;
        if (i == 0 && (unit->length != 8
            || memcmp(unit->bytes, "INPUT   ", 8)))
            ++pdpqa_calls.unexpected;
        if (i == 1 && (unit->length != 9
            || memcmp(unit->bytes, "INPUT.DAT", 9)))
            ++pdpqa_calls.unexpected;
        if (i == 3 && (unit->length != 1 || unit->bytes[0] != 0x40))
            ++pdpqa_calls.unexpected;
        if (i == 4 && (unit->length != 2 || unit->bytes[0] != 0
            || unit->bytes[1] != 255)) ++pdpqa_calls.unexpected;
    }
    return pdpqa_calls.svc99_fail_first && pdpqa_calls.svc99_calls == 1;
}
