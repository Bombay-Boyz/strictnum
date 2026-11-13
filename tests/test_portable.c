/* Portability test for strictnum.h.
 *
 * Written in the common subset of C89 and C++98 so the SAME file can be
 * compiled in every language mode:
 *
 *   cc  -std=c89 -pedantic-errors -DSTRICTNUM_NO_STDINT test_portable.c
 *   cc  -std=c99 ...      cc -std=c11 ...      cc -std=c17 ...
 *   c++ -std=c++98 -x c++ test_portable.c      c++ -std=c++17 -x c++ ...
 *
 * It uses only `long`-based oracles (strtol/strtoul), so it also runs where
 * `long long` does not exist. 64-bit behaviour is checked with fixed vectors.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../strictnum.h"

static unsigned long g_checks, g_fails;

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++g_checks;                                                          \
        if (!(cond)) {                                                       \
            ++g_fails;                                                       \
            if (g_fails <= 25)                                               \
                fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                    \
    } while (0)

/* ---- independent oracle (long-based) ---------------------------------- */
static int ref_is_digit(char c, unsigned base)
{
    static const char lo[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    static const char up[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    unsigned i;
    for (i = 0; i < base; ++i)
        if (c == lo[i] || c == up[i])
            return 1;
    return 0;
}

/* 0 ok, 1 empty, 2 syntax */
static int ref_shape(const char *s, unsigned base, int allow_neg)
{
    const char *p = s;
    if (*p == '\0') return 1;
    if (allow_neg && *p == '-') ++p;
    if (*p == '\0') return 2;
    for (; *p; ++p)
        if (!ref_is_digit(*p, base)) return 2;
    return 0;
}

/* Signed: verdict for the range [lo, hi], computed with strtol. */
static int ref_signed(const char *s, unsigned base, long lo, long hi, long *v)
{
    int sh = ref_shape(s, base, 1);
    long r;
    if (sh) return sh;
    errno = 0;
    r = strtol(s, NULL, (int)base);
    if (errno == ERANGE || r < lo || r > hi) return 3;
    *v = r;
    return 0;
}

static int ref_unsigned(const char *s, unsigned base, unsigned long hi, unsigned long *v)
{
    int sh = ref_shape(s, base, 0);
    unsigned long r;
    if (sh) return sh;
    errno = 0;
    r = strtoul(s, NULL, (int)base);
    if (errno == ERANGE || r > hi) return 3;
    *v = r;
    return 0;
}

#define DIFF_S(TN, T, LO, HI)                                                     \
    static void diff_##TN(const char *s, unsigned base)                           \
    {                                                                             \
        long rv = 0;                                                              \
        int rst = ref_signed(s, base, (LO), (HI), &rv);                           \
        T o1 = (T)42, o2 = (T)42;                                                 \
        int s1 = (int)sn_parse_##TN##_base_z(s, base, &o1);                 \
        int s2 = (int)sn_parse_##TN##_base(s, strlen(s), base, &o2);              \
        int bad = (s1 != rst || s2 != rst);                                       \
        if (!bad) bad = (rst == 0) ? (o1 != (T)rv || o2 != (T)rv)                 \
                                   : (o1 != (T)42 || o2 != (T)42);                \
        ++g_checks;                                                               \
        if (bad && ++g_fails <= 25)                                               \
            fprintf(stderr, "MISMATCH %s base %u \"%s\": want %d, got %d/%d\n",   \
                    #TN, base, s, rst, s1, s2);                                   \
    }

#define DIFF_U(TN, T, HI)                                                         \
    static void diff_##TN(const char *s, unsigned base)                           \
    {                                                                             \
        unsigned long rv = 0;                                                     \
        int rst = ref_unsigned(s, base, (HI), &rv);                               \
        T o1 = (T)42, o2 = (T)42;                                                 \
        int s1 = (int)sn_parse_##TN##_base_z(s, base, &o1);                 \
        int s2 = (int)sn_parse_##TN##_base(s, strlen(s), base, &o2);              \
        int bad = (s1 != rst || s2 != rst);                                       \
        if (!bad) bad = (rst == 0) ? (o1 != (T)rv || o2 != (T)rv)                 \
                                   : (o1 != (T)42 || o2 != (T)42);                \
        ++g_checks;                                                               \
        if (bad && ++g_fails <= 25)                                               \
            fprintf(stderr, "MISMATCH %s base %u \"%s\": want %d, got %d/%d\n",   \
                    #TN, base, s, rst, s1, s2);                                   \
    }

DIFF_S(i8,  sn_i8,  -128L,        127L)
DIFF_S(i16, sn_i16, -32768L,      32767L)
DIFF_S(i32, sn_i32, -2147483647L - 1L, 2147483647L)
DIFF_U(u8,  sn_u8,  255UL)
DIFF_U(u16, sn_u16, 65535UL)
DIFF_U(u32, sn_u32, 4294967295UL)

static void diff_all(const char *s, unsigned base)
{
    diff_i8(s, base); diff_i16(s, base); diff_i32(s, base);
    diff_u8(s, base); diff_u16(s, base); diff_u32(s, base);
}

static void test_enumeration(void)
{
    static const char alpha[] = "019-+ aFx";
    static const unsigned bases[] = { 10, 16, 2, 36 };
    const unsigned long A = (unsigned long)strlen(alpha);
    size_t bi, len, i;
    for (bi = 0; bi < sizeof bases / sizeof bases[0]; ++bi) {
        for (len = 0; len <= 5; ++len) {
            unsigned long total = 1, k;
            for (i = 0; i < len; ++i) total *= A;
            for (k = 0; k < total; ++k) {
                char buf[8];
                unsigned long t = k;
                for (i = 0; i < len; ++i) { buf[i] = alpha[t % A]; t /= A; }
                buf[len] = '\0';
                diff_all(buf, bases[bi]);
            }
        }
    }
}

static void test_small_roundtrips(void)
{
    char buf[64];
    long v;
    for (v = -32768L; v <= 32767L; ++v) {
        sn_i16 o16 = 0;
        sprintf(buf, "%ld", v);
        CHECK(sn_parse_i16_z(buf, &o16) == SN_OK && (long)o16 == v);
        if (v >= -128L && v <= 127L) {
            sn_i8 o8 = 0;
            CHECK(sn_parse_i8_z(buf, &o8) == SN_OK && (long)o8 == v);
        }
    }
    for (v = 0L; v <= 65535L; ++v) {
        sn_u16 o16 = 0;
        sprintf(buf, "%ld", v);
        CHECK(sn_parse_u16_z(buf, &o16) == SN_OK && (long)o16 == v);
        if (v <= 255L) {
            sn_u8 o8 = 0;
            CHECK(sn_parse_u8_z(buf, &o8) == SN_OK && (long)o8 == v);
        }
    }
}

static void test_32bit_vectors(void)
{
    sn_i32 i = 0;
    sn_u32 u = 0;
    CHECK(sn_parse_i32_z("2147483647", &i) == SN_OK && i == 2147483647L);
    CHECK(sn_parse_i32_z("-2147483648", &i) == SN_OK && i == -2147483647L - 1L);
    CHECK(sn_parse_i32_z("2147483648", &i) == SN_ERR_RANGE);
    CHECK(sn_parse_i32_z("-2147483649", &i) == SN_ERR_RANGE);
    CHECK(sn_parse_u32_z("4294967295", &u) == SN_OK && u == 4294967295UL);
    CHECK(sn_parse_u32_z("4294967296", &u) == SN_ERR_RANGE);
    CHECK(sn_parse_u32_base_z("FFFFFFFF", 16, &u) == SN_OK && u == 4294967295UL);
    CHECK(sn_parse_u32_base_z("100000000", 16, &u) == SN_ERR_RANGE);
}

#ifdef SN_HAVE_64
/* Fixed vectors; constants are built from shifts so no 64-bit literal is
 * needed (a 64-bit literal is not valid in strict C89 on 32-bit `long`). */
static void test_64bit_vectors(void)
{
    sn_i64 i = 0;
    sn_u64 u = 0;
    const sn_u64 u_max = (((sn_u64)0xFFFFFFFFUL) << 32) | (sn_u64)0xFFFFFFFFUL;
    const sn_u64 i_max = u_max >> 1;

    CHECK(sn_parse_u64_z("18446744073709551615", &u) == SN_OK && u == u_max);
    CHECK(sn_parse_u64_z("18446744073709551616", &u) == SN_ERR_RANGE && u == u_max);
    CHECK(sn_parse_u64_z("184467440737095516150", &u) == SN_ERR_RANGE);
    CHECK(sn_parse_u64_z("18446744073709551615x", &u) == SN_ERR_SYNTAX);
    CHECK(sn_parse_u64_base_z("FFFFFFFFFFFFFFFF", 16, &u) == SN_OK && u == u_max);
    CHECK(sn_parse_u64_base_z("10000000000000000", 16, &u) == SN_ERR_RANGE);

    CHECK(sn_parse_i64_z("9223372036854775807", &i) == SN_OK &&
          (sn_u64)i == i_max);
    CHECK(sn_parse_i64_z("9223372036854775808", &i) == SN_ERR_RANGE);
    CHECK(sn_parse_i64_z("-9223372036854775808", &i) == SN_OK &&
          i == -(sn_i64)i_max - 1);
    CHECK(sn_parse_i64_z("-9223372036854775809", &i) == SN_ERR_RANGE);
    CHECK(sn_parse_i64_z("-1", &i) == SN_OK && i == -1);
    CHECK(sn_parse_i64_z("-0", &i) == SN_OK && i == 0);
}
#endif

static void test_contract(void)
{
    sn_i32 i = 7;
    sn_u32 u = 7;
    sn_u8  b = 7;
    sn_i8  c = 7;

    CHECK(sn_parse_i32_z("", &i) == SN_ERR_EMPTY);
    CHECK(sn_parse_i32("x",    0,      &i) == SN_ERR_EMPTY);
    CHECK(sn_parse_i32_z("-", &i) == SN_ERR_SYNTAX);
    CHECK(sn_parse_i32_z("+1", &i) == SN_ERR_SYNTAX);
    CHECK(sn_parse_i32_z(" 1", &i) == SN_ERR_SYNTAX);
    CHECK(sn_parse_i32_z("1 ", &i) == SN_ERR_SYNTAX);
    CHECK(sn_parse_i32_z("0x10", &i) == SN_ERR_SYNTAX);
    CHECK(sn_parse_u32_z("-0", &u) == SN_ERR_SYNTAX);
    CHECK(sn_parse_u8_z("-1", &b) == SN_ERR_SYNTAX);
    CHECK(i == 7 && u == 7 && b == 7);

    CHECK(sn_parse_i32_z("-0", &i) == SN_OK && i == 0);
    CHECK(sn_parse_i32_z("007", &i) == SN_OK && i == 7);
    CHECK(sn_parse_i8_z("128", &c) == SN_ERR_RANGE && c == 7);
    CHECK(sn_parse_i8_z("-128", &c) == SN_OK && c == -128);
    CHECK(sn_parse_i32_base_z("zz", 36, &i) == SN_OK && i == 1295);
    CHECK(sn_parse_i32_base_z("-FfF", 16, &i) == SN_OK && i == -4095);

    i = 7;
    CHECK(sn_parse_i32_z("99999999999999999999x", &i) == SN_ERR_SYNTAX);
    CHECK(sn_parse_i32_z("99999999999999999999", &i) == SN_ERR_RANGE);
    CHECK(i == 7);

    CHECK(sn_parse_i32("12345", 3, &i) == SN_OK && i == 123);
    {
        const char emb[4] = { '1', '2', '\0', '3' };
        CHECK(sn_parse_i32(emb, 4, &i) == SN_ERR_SYNTAX);
        CHECK(sn_parse_i32_z(emb, &i) == SN_OK && i == 12);
    }

    CHECK(sn_parse_i32_z(NULL, &i) == SN_ERR_ARG);
    CHECK(sn_parse_i32_z("1", NULL) == SN_ERR_ARG);
    CHECK(sn_parse_i32_base_z("1", 1, &i) == SN_ERR_ARG);
    CHECK(sn_parse_i32_base_z("1", 37, &i) == SN_ERR_ARG);

    CHECK(sn_status_string(SN_OK) != NULL);
    CHECK(sn_status_string((sn_status)99) != NULL);
}

int main(void)
{
    test_contract();
    test_32bit_vectors();
#ifdef SN_HAVE_64
    test_64bit_vectors();
#endif
    test_small_roundtrips();
    test_enumeration();
    printf("portable: %lu checks, %lu failures (64-bit %s, stdint %s)\n",
           g_checks, g_fails,
#ifdef SN_HAVE_64
           "yes",
#else
           "no",
#endif
#if SN_HAS_STDINT
           "yes"
#else
           "no"
#endif
    );
    return g_fails ? 1 : 0;
}
