/* Semantic libFuzzer harness for strictnum.h.
 *
 * Build with:
 *   clang -std=c99 -O1 -g -fsanitize=fuzzer,address,undefined \
 *     tests/fuzz_strictnum.c -o fuzz_strictnum
 *
 * The reference path first validates the complete byte slice independently,
 * then delegates numeric conversion/range detection to strtoll/strtoull.
 * Every public type is checked for status, value, and unchanged-on-error.
 */
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../strictnum.h"

#define FUZZ_MAX_INPUT 256u

enum ref_status { REF_OK = 0, REF_EMPTY = 1, REF_SYNTAX = 2, REF_RANGE = 3 };

static int ref_digit(unsigned char c)
{
    static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    static const char upper[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int i;
    for (i = 0; i < 36; ++i) {
        if (c == (unsigned char)digits[i] || c == (unsigned char)upper[i])
            return i;
    }
    return -1;
}

static int ref_shape(const char *s, size_t n, unsigned base, int is_signed)
{
    size_t i = 0;
    if (n == 0u)
        return REF_EMPTY;
    if (is_signed && s[0] == '-')
        i = 1u;
    if (i == n)
        return REF_SYNTAX;
    for (; i < n; ++i) {
        int d = ref_digit((unsigned char)s[i]);
        if (d < 0 || (unsigned)d >= base)
            return REF_SYNTAX;
    }
    return REF_OK;
}

static int ref_signed(const char *s, size_t n, unsigned base,
                      long long lo, long long hi, long long *value)
{
    char text[FUZZ_MAX_INPUT + 1u];
    long long v;
    int shape = ref_shape(s, n, base, 1);
    if (shape != REF_OK)
        return shape;
    memcpy(text, s, n);
    text[n] = '\0';
    errno = 0;
    v = strtoll(text, NULL, (int)base);
    if (errno == ERANGE || v < lo || v > hi)
        return REF_RANGE;
    *value = v;
    return REF_OK;
}

static int ref_unsigned(const char *s, size_t n, unsigned base,
                        unsigned long long hi, unsigned long long *value)
{
    char text[FUZZ_MAX_INPUT + 1u];
    unsigned long long v;
    int shape = ref_shape(s, n, base, 0);
    if (shape != REF_OK)
        return shape;
    memcpy(text, s, n);
    text[n] = '\0';
    errno = 0;
    v = strtoull(text, NULL, (int)base);
    if (errno == ERANGE || v > hi)
        return REF_RANGE;
    *value = v;
    return REF_OK;
}

#define CHECK_SIGNED(NAME, TYPE, MINIMUM, MAXIMUM)                              \
    do {                                                                         \
        TYPE got = (TYPE)42;                                                     \
        long long expected = 0;                                                  \
        int want = ref_signed(text, n, base, (long long)(MINIMUM),               \
                              (long long)(MAXIMUM), &expected);                  \
        int actual = (int)sn_parse_##NAME##_base(text, n, base, &got);           \
        if (actual != want) abort();                                             \
        if (want == REF_OK) {                                                    \
            if ((long long)got != expected) abort();                             \
        } else if (got != (TYPE)42) {                                             \
            abort();                                                             \
        }                                                                        \
        if (no_embedded_nul) {                                                   \
            TYPE nts = (TYPE)42;                                                 \
            int nts_status = (int)sn_parse_##NAME##_base_z(text, base,    \
                                                           &nts);                \
            if (nts_status != want) abort();                                     \
            if (want == REF_OK) {                                                \
                if ((long long)nts != expected) abort();                        \
            } else if (nts != (TYPE)42) {                                        \
                abort();                                                         \
            }                                                                    \
        }                                                                        \
    } while (0)

#define CHECK_UNSIGNED(NAME, TYPE, MAXIMUM)                                     \
    do {                                                                         \
        TYPE got = (TYPE)42;                                                     \
        unsigned long long expected = 0;                                         \
        int want = ref_unsigned(text, n, base,                                   \
                                (unsigned long long)(MAXIMUM), &expected);       \
        int actual = (int)sn_parse_##NAME##_base(text, n, base, &got);           \
        if (actual != want) abort();                                             \
        if (want == REF_OK) {                                                    \
            if ((unsigned long long)got != expected) abort();                    \
        } else if (got != (TYPE)42) {                                             \
            abort();                                                             \
        }                                                                        \
        if (no_embedded_nul) {                                                   \
            TYPE nts = (TYPE)42;                                                 \
            int nts_status = (int)sn_parse_##NAME##_base_z(text, base,    \
                                                           &nts);                \
            if (nts_status != want) abort();                                     \
            if (want == REF_OK) {                                                \
                if ((unsigned long long)nts != expected) abort();               \
            } else if (nts != (TYPE)42) {                                        \
                abort();                                                         \
            }                                                                    \
        }                                                                        \
    } while (0)

static void check_one_base(const uint8_t *data, size_t size, unsigned base)
{
    char text[FUZZ_MAX_INPUT + 1u];
    size_t n = size > FUZZ_MAX_INPUT ? FUZZ_MAX_INPUT : size;
    int no_embedded_nul;
    if (n != 0u)
        memcpy(text, data, n);
    text[n] = '\0';
    no_embedded_nul = memchr(text, '\0', n) == NULL;

    CHECK_SIGNED(i8, sn_i8, INT8_MIN, INT8_MAX);
    CHECK_UNSIGNED(u8, sn_u8, UINT8_MAX);
    CHECK_SIGNED(i16, sn_i16, INT16_MIN, INT16_MAX);
    CHECK_UNSIGNED(u16, sn_u16, UINT16_MAX);
    CHECK_SIGNED(i32, sn_i32, INT32_MIN, INT32_MAX);
    CHECK_UNSIGNED(u32, sn_u32, UINT32_MAX);
#ifdef SN_HAVE_64
    CHECK_SIGNED(i64, sn_i64, INT64_MIN, INT64_MAX);
    CHECK_UNSIGNED(u64, sn_u64, UINT64_MAX);
#endif
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    sn_i32 sentinel = 17;
    /* Exercise argument-error paths on every fuzz input as well as byte parsing. */
    if (sn_parse_i32(NULL, 1u, &sentinel) != SN_ERR_ARG || sentinel != 17 ||
        sn_parse_i32("1", 1u, NULL) != SN_ERR_ARG ||
        sn_parse_i32_base("1", 1u, 1u, &sentinel) != SN_ERR_ARG ||
        sn_parse_i32(NULL, 0u, &sentinel) != SN_ERR_EMPTY || sentinel != 17)
        abort();
    unsigned first = size == 0u ? 10u : 2u + (unsigned)(data[0] % 35u);
    unsigned second = size == 0u ? 16u : 2u + (unsigned)(data[size - 1u] % 35u);
    check_one_base(data, size, first);
    if (second != first)
        check_one_base(data, size, second);
    return 0;
}
