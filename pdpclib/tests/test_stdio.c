#include "native_fixture.h"
#define CHECK(c) do { if (!(c)) return pdpqa_failure(#c, __LINE__); } while (0)

static int rejects(const char *mode, int expected_errno)
{
    pdpqa_reset();
    CHECK(fopen("dd:input", mode) == NULL);
    CHECK(errno == expected_errno);
    CHECK(pdpqa_calls.opens == 0);
    CHECK(pdpqa_calls.dcb_queries == 0);
    CHECK(pdpqa_calls.points == 0);
    CHECK(pdpqa_calls.unexpected == 0);
    CHECK(pdpqa_calls.allocations == 2); /* Trace FILE and buffer only. */
    return 0;
}

static int supports(const char *mode, int native_mode)
{
    FILE *stream;
    pdpqa_reset();
    stream = fopen("dd:input", mode);
    CHECK(stream != NULL);
    CHECK(pdpqa_calls.opens == 1);
    CHECK(pdpqa_calls.last_mode == native_mode);
    CHECK(pdpqa_calls.dcb_queries == 0);
    CHECK(pdpqa_calls.points == 0);
    CHECK(pdpqa_calls.reads == 0);
    CHECK(pdpqa_calls.unexpected == 0);
    CHECK(stream->update == 0);
    CHECK(fclose(stream) == 0);
    CHECK(pdpqa_calls.closes == 1);
    CHECK(pdpqa_calls.allocations == 2);
    return 0;
}

int main(void)
{
    static const char *ordinary[] = {"r", "rb", "w", "wb"};
    static const char *unsupported[] = {
        "a", "ab", "r+", "w+", "a+", "a+b", "ab+"
    };
    static const char *binary_update[] = {"r+b", "rb+", "w+b", "wb+"};
    FILE *trace;
    FILE *stream;
    FILE seeded;
    char buffer[72];
    unsigned int i;
    int result;

    pdpqa_reset();
    trace = fopen("dd:trace", "wb");
    CHECK(trace != NULL);
    __stderr_ptr = trace;
    for (i = 0; i < sizeof ordinary / sizeof *ordinary; ++i) {
        result = supports(ordinary[i], i < 2 ? 0 : 1);
        if (result != 0) return result;
    }
    for (i = 0; i < sizeof unsupported / sizeof *unsupported; ++i) {
        result = rejects(unsupported[i], 2);
        if (result != 0) return result;
    }
    result = rejects("q", 0);
    if (result != 0) return result;

    for (i = 0; i < sizeof binary_update / sizeof *binary_update; ++i) {
#ifdef __PDOS390__
        result = rejects(binary_update[i], 2);
        if (result != 0) return result;
#else
        pdpqa_reset();
        stream = fopen("dd:input", binary_update[i]);
        CHECK(stream != NULL);
        CHECK(pdpqa_calls.last_mode == (i < 2 ? 18 : 1));
        CHECK(pdpqa_calls.dcb_queries == (i < 2 ? 1 : 0));
        CHECK(pdpqa_calls.points == (i < 2 ? 1 : 0));
        CHECK(pdpqa_calls.unexpected == 0);
        CHECK(fclose(stream) == 0);
        CHECK(pdpqa_calls.allocations == 2);
#endif
    }

    /* Exercise the guarded post-open path even with a preexisting update flag. */
    memset(&seeded, 0, sizeof seeded);
    seeded.update = 1;
    seeded.ungetCh = -1;
    /* Match PDPCLIB's four-byte hidden prefix and suffix buffer contract. */
    seeded.fbuf = buffer + 4;
    seeded.upto = seeded.fbuf;
    seeded.endbuf = seeded.fbuf;
    seeded.szfbuf = sizeof buffer - 8;
    seeded.mode = __READ_MODE;
    pdpqa_reset();
    CHECK(pdpqa_seeded_open(&seeded, 4) == 0);
    CHECK(pdpqa_calls.opens == 1);
#ifdef __PDOS390__
    CHECK(pdpqa_calls.dcb_queries == 0);
    CHECK(pdpqa_calls.reads == 0);
    CHECK(pdpqa_calls.points == 0);
#else
    CHECK(pdpqa_calls.dcb_queries == 1);
    CHECK(pdpqa_calls.reads == 1);
    CHECK(pdpqa_calls.points == 1);
#endif
    CHECK(pdpqa_calls.unexpected == 0);

    /* Synthetic update state independently selects the fseek service path. */
    pdpqa_reset();
    seeded.update = 1;
    seeded.quickBin = 0;
    seeded.lrecl = 8;
    seeded.blocks_per_track = 4;
    seeded.style = 0;
    seeded.bufStartR = 0;
    seeded.upto = seeded.fbuf;
    seeded.endbuf = seeded.fbuf;
    seeded.eofInd = 0;
    CHECK(fseek(&seeded, 8, SEEK_SET) == 0);
#ifdef __PDOS390__
    CHECK(pdpqa_calls.points == 0);
    CHECK(pdpqa_calls.reads == 1);
#else
    CHECK(pdpqa_calls.points == 1);
    CHECK(pdpqa_calls.last_ttr == 0x200U);
    CHECK(pdpqa_calls.reads == 0);
    CHECK(seeded.bufStartR == 8);
#endif
    CHECK(pdpqa_calls.unexpected == 0);

    pdpqa_reset();
    pdpqa_calls.fail_malloc = 1;
    CHECK(fopen("dd:input", "rb") == NULL);
    CHECK(pdpqa_calls.opens == 0);
    CHECK(pdpqa_calls.allocations == 2);
    pdpqa_reset();
    pdpqa_calls.fail_open = 1;
    stream = fopen("dd:input", "rb");
    CHECK(stream == NULL);
    CHECK(pdpqa_calls.opens == 1);
    CHECK(pdpqa_calls.allocations == 2);
    pdpqa_reset();
    CHECK(fclose(trace) == 0);
    CHECK(pdpqa_calls.allocations == 0);
    __stderr_ptr = NULL;
    return 0;
}
