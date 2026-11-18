/* Full test suite for strictnum.h (requires C99: long long, snprintf).
 *
 * The oracle is deliberately independent of the library: it validates the
 * shape of each input with its own code and then uses strtoll/strtoull only
 * to obtain the value and the overflow verdict.
 *
 *   cc -std=c99 -Wall -Wextra -O1 test_strictnum.c -o t && ./t
 */
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../strictnum.h"
#include "support.h"



/* ---- deterministic PRNG (xorshift64*) -------------------------------- */
static unsigned long long g_rng = 88172645463325252ULL;
static unsigned long long rnd(void)
{
    g_rng ^= g_rng >> 12;
    g_rng ^= g_rng << 25;
    g_rng ^= g_rng >> 27;
    return g_rng * 2685821657736338717ULL;
}

/* ---- reference oracle ------------------------------------------------- */
enum { R_OK = 0, R_EMPTY = 1, R_SYNTAX = 2, R_RANGE = 3 };

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

static int ref_shape(const char *s, unsigned base, int allow_neg)
{
    const char *p = s;
    if (*p == '\0') return R_EMPTY;
    if (allow_neg && *p == '-') ++p;
    if (*p == '\0') return R_SYNTAX;
    for (; *p; ++p)
        if (!ref_is_digit(*p, base)) return R_SYNTAX;
    return R_OK;
}

static int ref_i64(const char *s, unsigned base, long long *v)
{
    int sh = ref_shape(s, base, 1);
    long long r;
    if (sh != R_OK) return sh;
    errno = 0;
    r = strtoll(s, NULL, (int)base);
    if (errno == ERANGE) return R_RANGE;
    *v = r;
    return R_OK;
}

static int ref_u64(const char *s, unsigned base, unsigned long long *v)
{
    int sh = ref_shape(s, base, 0);
    unsigned long long r;
    if (sh != R_OK) return sh;
    errno = 0;
    r = strtoull(s, NULL, (int)base);
    if (errno == ERANGE) return R_RANGE;
    *v = r;
    return R_OK;
}

/* ---- differential checkers, one per type ----------------------------- */
#define DIFF_S(TN, T, LO, HI)                                                        \
    static void diff_##TN(const char *s, unsigned base)                              \
    {                                                                                \
        long long rv = 0;                                                            \
        int rst = ref_i64(s, base, &rv);                                             \
        T o1 = (T)42, o2 = (T)42;                                                    \
        int s1, s2, bad;                                                             \
        if (rst == R_OK && (rv < (LO) || rv > (HI))) rst = R_RANGE;                  \
        s1 = (int)sn_parse_##TN##_base_z(s, base, &o1);                        \
        s2 = (int)sn_parse_##TN##_base(s, strlen(s), base, &o2);                     \
        bad = (s1 != rst || s2 != rst);                                              \
        if (!bad) bad = (rst == R_OK) ? (o1 != (T)rv || o2 != (T)rv)                 \
                                      : (o1 != (T)42 || o2 != (T)42);                \
        ++g_checks;                                                                  \
        if (bad && ++g_fails <= 25)                                                  \
            fprintf(stderr, "MISMATCH %s base %u \"%s\": want %d, got %d/%d\n",      \
                    #TN, base, s, rst, s1, s2);                                      \
    }

#define DIFF_U(TN, T, HI)                                                            \
    static void diff_##TN(const char *s, unsigned base)                              \
    {                                                                                \
        unsigned long long rv = 0;                                                   \
        int rst = ref_u64(s, base, &rv);                                             \
        T o1 = (T)42, o2 = (T)42;                                                    \
        int s1, s2, bad;                                                             \
        if (rst == R_OK && rv > (HI)) rst = R_RANGE;                                 \
        s1 = (int)sn_parse_##TN##_base_z(s, base, &o1);                        \
        s2 = (int)sn_parse_##TN##_base(s, strlen(s), base, &o2);                     \
        bad = (s1 != rst || s2 != rst);                                              \
        if (!bad) bad = (rst == R_OK) ? (o1 != (T)rv || o2 != (T)rv)                 \
                                      : (o1 != (T)42 || o2 != (T)42);                \
        ++g_checks;                                                                  \
        if (bad && ++g_fails <= 25)                                                  \
            fprintf(stderr, "MISMATCH %s base %u \"%s\": want %d, got %d/%d\n",      \
                    #TN, base, s, rst, s1, s2);                                      \
    }

DIFF_S(i8,  sn_i8,  (-127 - 1), 127)
DIFF_S(i16, sn_i16, (-32767 - 1), 32767)
DIFF_S(i32, sn_i32, (-2147483647L - 1L), 2147483647L)
#ifdef SN_HAVE_64
DIFF_S(i64, sn_i64, (-((long long)(((sn_u64)~(sn_u64)0) >> 1)) - 1LL), (long long)(((sn_u64)~(sn_u64)0) >> 1))
#endif
DIFF_U(u8,  sn_u8,  255ULL)
DIFF_U(u16, sn_u16, 65535ULL)
DIFF_U(u32, sn_u32, 4294967295ULL)
#ifdef SN_HAVE_64
DIFF_U(u64, sn_u64, (unsigned long long)((sn_u64)~(sn_u64)0))
#endif

static void diff_all(const char *s, unsigned base)
{
    diff_i8(s, base);  diff_i16(s, base); diff_i32(s, base);
    diff_u8(s, base);  diff_u16(s, base); diff_u32(s, base);
#ifdef SN_HAVE_64
    diff_i64(s, base); diff_u64(s, base);
#endif
}

/* ---- 1. exhaustive enumeration of all short strings ------------------- */
static void test_enumeration(void)
{
    static const char alpha[] = "019-+ aFx";
    const unsigned long A = (unsigned long)strlen(alpha);
    static const unsigned bases[] = { 10, 16, 2, 36 };
    size_t bi;
    for (bi = 0; bi < sizeof bases / sizeof bases[0]; ++bi) {
        const size_t maxlen = (bases[bi] == 10 || bases[bi] == 16) ? 6 : 5;
        size_t len;
        for (len = 0; len <= maxlen; ++len) {
            unsigned long total = 1, k;
            size_t i;
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

/* ---- 2. boundaries, with and without leading zeros -------------------- */
static void test_boundaries(void)
{
    static const char *dec[] = {
        "0", "-0", "1", "-1", "127", "128", "-128", "-129", "255", "256",
        "32767", "32768", "-32768", "-32769", "65535", "65536",
        "2147483647", "2147483648", "-2147483648", "-2147483649",
        "4294967295", "4294967296",
        "9223372036854775807", "9223372036854775808",
        "-9223372036854775808", "-9223372036854775809",
        "18446744073709551615", "18446744073709551616",
        "99999999999999999999", "184467440737095516150",
        "18446744073709551615x", "99999999999999999999x", "-99999999999999999999x"
    };
    static const char *hex[] = {
        "7f", "80", "-80", "-81", "ff", "100", "7fff", "8000", "ffff", "10000",
        "7FFFFFFF", "80000000", "FFFFFFFF", "100000000",
        "7fffffffffffffff", "8000000000000000", "-8000000000000000",
        "-8000000000000001", "ffffffffffffffff", "10000000000000000"
    };
    size_t i, z;
    for (i = 0; i < sizeof dec / sizeof dec[0]; ++i) {
        for (z = 0; z <= 25; ++z) {
            char buf[96];
            const char *p = dec[i];
            size_t o = 0;
            if (*p == '-') buf[o++] = *p++;
            memset(buf + o, '0', z); o += z;
            {
                const size_t n = strlen(p) + 1u;
                if (o > sizeof buf || n > sizeof buf - o) abort();
                memcpy(buf + o, p, n);
            }
            diff_all(buf, 10);
        }
    }
    for (i = 0; i < sizeof hex / sizeof hex[0]; ++i) {
        for (z = 0; z <= 25; z += 5) {
            char buf[96];
            const char *p = hex[i];
            size_t o = 0;
            if (*p == '-') buf[o++] = *p++;
            memset(buf + o, '0', z); o += z;
            {
                const size_t n = strlen(p) + 1u;
                if (o > sizeof buf || n > sizeof buf - o) abort();
                memcpy(buf + o, p, n);
            }
            diff_all(buf, 16);
        }
    }
}

/* ---- 3. random strings vs the oracle ---------------------------------- */
static void test_random_strings(void)
{
    static const unsigned bases[] = { 10, 16, 8, 36 };
    static const unsigned char invalid[] = {
        '/', '_', '+', ' ', '\t', '\x01', 0x80u, '\x7f'
    };
    int iter;
    for (iter = 0; iter < 400000; ++iter) {
        const unsigned base = bases[rnd() % 4];
        const size_t len = 1 + (size_t)(rnd() % 26);
        char buf[40];
        size_t i = 0, k;
        if (rnd() % 4 == 0) buf[i++] = '-';
        for (k = i; k < len + i; ++k) {
            const unsigned r = (unsigned)(rnd() % 40);
            if (r < base && r < 36)
                buf[k] = "0123456789abcdefghijklmnopqrstuvwxyz"[r];
            else
                memcpy(&buf[k],
                       &invalid[r % (unsigned)(sizeof invalid / sizeof invalid[0])],
                       sizeof buf[k]);
        }
        buf[len + i] = '\0';
        diff_all(buf, base);
    }
}

/* ---- 4. exhaustive round trips for small types ----------------------- */
static void test_small_roundtrips(void)
{
    char buf[64];
    int v;
    for (v = -128; v <= 127; ++v) {
        sn_i8 o = 0;
        snprintf(buf, sizeof buf, "%d", v);
        CHECK(sn_parse_i8_z(buf, &o) == SN_OK && o == v);
    }
    for (v = -32768; v <= 32767; ++v) {
        sn_i16 o = 0;
        snprintf(buf, sizeof buf, "%d", v);
        CHECK(sn_parse_i16_z(buf, &o) == SN_OK && o == v);
        snprintf(buf, sizeof buf, v < 0 ? "-%x" : "%x", (unsigned)(v < 0 ? -v : v));
        o = 0;
        CHECK(sn_parse_i16_base_z(buf, 16, &o) == SN_OK && o == v);
    }
    for (v = 0; v <= 255; ++v) {
        sn_u8 o = 0;
        snprintf(buf, sizeof buf, "%d", v);
        CHECK(sn_parse_u8_z(buf, &o) == SN_OK && o == v);
    }
    for (v = 0; v <= 65535; ++v) {
        sn_u16 o = 0;
        snprintf(buf, sizeof buf, "%d", v);
        CHECK(sn_parse_u16_z(buf, &o) == SN_OK && o == v);
        snprintf(buf, sizeof buf, "%X", (unsigned)v);
        o = 0;
        CHECK(sn_parse_u16_base_z(buf, 16, &o) == SN_OK && o == v);
        snprintf(buf, sizeof buf, "%x", (unsigned)v);
        o = 0;
        CHECK(sn_parse_u16_base_z(buf, 16, &o) == SN_OK && o == v);
    }
}

/* ---- 5. random round trips for wide types, all bases ----------------- */
#ifdef SN_HAVE_64
static void fmt_u64(unsigned long long v, unsigned base, int upper, char *out)
{
    const char *d = upper ? "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                          : "0123456789abcdefghijklmnopqrstuvwxyz";
    char tmp[72];
    size_t n = 0, i;
    if (v == 0) tmp[n++] = '0';
    while (v) { tmp[n++] = d[v % base]; v /= base; }
    for (i = 0; i < n; ++i) out[i] = tmp[n - 1 - i];
    out[n] = '\0';
}

static void test_wide_roundtrips(void)
{
    char buf[80];
    unsigned base;
    int i;
    for (base = 2; base <= 36; ++base) {
        for (i = 0; i < 3000; ++i) {
            unsigned long long m;
            sn_u64 u = 0;
            sn_i64 sv = 0;
            switch (i) {
            case 0: m = 0; break;
            case 1: m = 1; break;
            case 2: m = (unsigned long long)((sn_u64)~(sn_u64)0); break;
            case 3: m = (unsigned long long)(((sn_u64)~(sn_u64)0) >> 1); break;
            case 4: m = (unsigned long long)(((sn_u64)~(sn_u64)0) >> 1) + 1ULL; break;
            default: m = (rnd() >> (rnd() % 64)) & (unsigned long long)((sn_u64)~(sn_u64)0); break;
            }
            fmt_u64(m, base, i & 1, buf);
            CHECK(sn_parse_u64_base_z(buf, base, &u) == SN_OK && u == m);
            if (m <= (unsigned long long)(((sn_u64)~(sn_u64)0) >> 1)) {
                CHECK(sn_parse_i64_base_z(buf, base, &sv) == SN_OK && sv == (long long)m);
            } else {
                CHECK(sn_parse_i64_base_z(buf, base, &sv) == SN_ERR_RANGE);
            }
            memmove(buf + 1, buf, strlen(buf) + 1);
            buf[0] = '-';
            if (m <= (unsigned long long)(((sn_u64)~(sn_u64)0) >> 1) + 1ULL) {
                const long long want = (m == (unsigned long long)(((sn_u64)~(sn_u64)0) >> 1) + 1ULL)
                    ? (-((long long)(((sn_u64)~(sn_u64)0) >> 1)) - 1LL) : -(long long)m;
                CHECK(sn_parse_i64_base_z(buf, base, &sv) == SN_OK && sv == want);
            } else {
                CHECK(sn_parse_i64_base_z(buf, base, &sv) == SN_ERR_RANGE);
            }
            CHECK(sn_parse_u64_base_z(buf, base, &u) == SN_ERR_SYNTAX);
        }
    }
}
#endif /* SN_HAVE_64 */

static void test_32bit_random_roundtrips(void)
{
    char buf[48];
    int i;
    for (i = 0; i < 200000; ++i) {
        const sn_u32 bits = (sn_u32)rnd();
        const sn_i32 a = bits <= 2147483647UL
            ? (sn_i32)bits
            : (sn_i32)(-2147483647L - 1L + (long)(bits - 2147483648UL));
        const sn_u32 b = (sn_u32)rnd();
        sn_i32 x = 0;
        sn_u32 y = 0;
        snprintf(buf, sizeof buf, "%ld", (long)a);
        CHECK(sn_parse_i32_z(buf, &x) == SN_OK && x == a);
        snprintf(buf, sizeof buf, "%lu", (unsigned long)b);
        CHECK(sn_parse_u32_z(buf, &y) == SN_OK && y == b);
    }
}

/* Every public type must apply the same pointer/base/empty-input contract. */
#define CHECK_ARGUMENT_CONTRACT(TN, T) do { \
    T v = (T)7; \
    CHECK(sn_parse_##TN(NULL, 0u, &v) == SN_ERR_EMPTY && v == (T)7); \
    CHECK(sn_parse_##TN(NULL, 1u, &v) == SN_ERR_ARG && v == (T)7); \
    CHECK(sn_parse_##TN##_z(NULL, &v) == SN_ERR_ARG && v == (T)7); \
    CHECK(sn_parse_##TN("1", 1u, NULL) == SN_ERR_ARG); \
    CHECK(sn_parse_##TN##_base("1", 1u, 1u, &v) == SN_ERR_ARG); \
    CHECK(sn_parse_##TN##_base_z("1", 1u, &v) == SN_ERR_ARG); \
} while (0)

/* ---- 6. API contract ------------------------------------------------- */
static void test_contract(void)
{
    sn_i32 i = 7;
    sn_u32 u = 7;
    sn_u8 b = 7;
    sn_i8 c = 7;
    const char embedded[] = { '1', '2', '\0', '3' };

    CHECK_ARGUMENT_CONTRACT(i8, sn_i8);
    CHECK_ARGUMENT_CONTRACT(i16, sn_i16);
    CHECK_ARGUMENT_CONTRACT(i32, sn_i32);
    CHECK_ARGUMENT_CONTRACT(u8, sn_u8);
    CHECK_ARGUMENT_CONTRACT(u16, sn_u16);
    CHECK_ARGUMENT_CONTRACT(u32, sn_u32);
#ifdef SN_HAVE_64
    CHECK_ARGUMENT_CONTRACT(i64, sn_i64);
    CHECK_ARGUMENT_CONTRACT(u64, sn_u64);
#endif

    CHECK(sn_parse_i32_z("", &i) == SN_ERR_EMPTY);
    CHECK(sn_parse_i32("x", 0, &i) == SN_ERR_EMPTY);
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
    CHECK(sn_parse_u8_z("000000000000000000000000000255", &b) == SN_OK && b == 255);
    CHECK(sn_parse_i8_z("-128", &c) == SN_OK && c == -128);
    CHECK(sn_parse_i8_z("127", &c) == SN_OK && c == 127);
    CHECK(sn_parse_i8_z("128", &c) == SN_ERR_RANGE && c == 127);
    CHECK(sn_parse_i32_base_z("zz", 36, &i) == SN_OK && i == 1295);
    CHECK(sn_parse_i32_base_z("ZZ", 36, &i) == SN_OK && i == 1295);
    CHECK(sn_parse_i32_base_z("-FfF", 16, &i) == SN_OK && i == -4095);
    CHECK(sn_parse_u32_base_z("101", 2, &u) == SN_OK && u == 5);
    CHECK(sn_parse_u32_base_z("2", 2, &u) == SN_ERR_SYNTAX);

    i = 7;
    CHECK(sn_parse_i32_z("99999999999999999999x", &i) == SN_ERR_SYNTAX);
    CHECK(sn_parse_i32_z("99999999999999999999", &i) == SN_ERR_RANGE);
    CHECK(sn_parse_i32_z("-99999999999999999999", &i) == SN_ERR_RANGE);
    CHECK(i == 7);

    CHECK(sn_parse_i32("12345", 3, &i) == SN_OK && i == 123);
    CHECK(sn_parse_i32("12345", 1, &i) == SN_OK && i == 1);
    CHECK(sn_parse_i32(embedded, 2, &i) == SN_OK && i == 12);
    CHECK(sn_parse_i32(embedded, 4, &i) == SN_ERR_SYNTAX && i == 12);
    CHECK(sn_parse_i32_z(embedded, &i) == SN_OK && i == 12);

    i = 7;
    CHECK(sn_parse_i32_z(NULL, &i) == SN_ERR_ARG);
    CHECK(sn_parse_i32(NULL, 0, &i) == SN_ERR_EMPTY);
    CHECK(sn_parse_i32_z("1", NULL) == SN_ERR_ARG);
    CHECK(sn_parse_i32_base_z("1", 0, &i) == SN_ERR_ARG);
    CHECK(sn_parse_i32_base_z("1", 1, &i) == SN_ERR_ARG);
    CHECK(sn_parse_i32_base_z("1", 37, &i) == SN_ERR_ARG);
    CHECK(sn_parse_i32_base_z("1", (unsigned)-1, &i) == SN_ERR_ARG);
    CHECK(i == 7);

    /* The explicit length describes a complete readable byte slice. */
    {
        const char p[6] = { 'a', 'b', '\0', 'c', 'd', 'e' };
        sn_u32 w = 7;
        CHECK(sn_parse_u32_base(p, sizeof p, 16, &w) == SN_ERR_SYNTAX && w == 7);
        CHECK(sn_parse_u32_base(p, 2, 16, &w) == SN_OK && w == 0xAB);
    }

    CHECK(strcmp(sn_status_string(SN_OK), "ok") == 0);
    CHECK(strcmp(sn_status_string(SN_ERR_EMPTY), "empty input") == 0);
    CHECK(strcmp(sn_status_string(SN_ERR_SYNTAX), "not a valid number") == 0);
    CHECK(strcmp(sn_status_string(SN_ERR_RANGE), "number out of range") == 0);
    CHECK(strcmp(sn_status_string(SN_ERR_ARG), "invalid argument") == 0);
    CHECK(strcmp(sn_status_string((sn_status)99), "unknown status") == 0);
}

#ifdef SN_HAVE_64
static void test_64bit_contract(void)
{
    sn_i64 i = 7;
    sn_u64 u = 7;
    CHECK(sn_parse_i64_z("-9223372036854775808", &i) == SN_OK);
    CHECK(sn_parse_i64_z("9223372036854775808", &i) == SN_ERR_RANGE);
    CHECK(sn_parse_u64_z("18446744073709551615", &u) == SN_OK);
    CHECK(sn_parse_u64_z("18446744073709551616", &u) == SN_ERR_RANGE);
    CHECK(sn_parse_u64_z("18446744073709551615x", &u) == SN_ERR_SYNTAX);
}
#endif

/* ---- 7. locale independence ------------------------------------------ */
static void test_locale(void)
{
    static const char *names[] = { "de_DE.UTF-8", "fr_FR.UTF-8", "tr_TR.UTF-8",
                                   "C.utf8", "" };
    size_t k;
    int tried = 0;
    for (k = 0; k < sizeof names / sizeof names[0]; ++k) {
        sn_i32 v = 0;
        if (!setlocale(LC_ALL, names[k])) continue;
        ++tried;
        CHECK(sn_parse_i32_z("1234", &v) == SN_OK && v == 1234);
        CHECK(sn_parse_i32_z("1.234", &v) == SN_ERR_SYNTAX);
        CHECK(sn_parse_i32_z("1,234", &v) == SN_ERR_SYNTAX);
        CHECK(sn_parse_i32_base_z("I", 36, &v) == SN_OK && v == 18); /* Turkish dotless-i trap */
        CHECK(sn_parse_i32_base_z("i", 36, &v) == SN_OK && v == 18);
    }
    setlocale(LC_ALL, "C");
    printf("  (locale test ran under %d installed locale(s))\n", tried);
}

static void report_section(const char *name, unsigned long failures_before)
{
    printf("  %-16s %s\n", name, g_fails == failures_before ? "ok" : "FAILED");
}

int main(void)
{
    unsigned long before;
    printf("strictnum %d.%d.%d test suite\n", STRICTNUM_VERSION_MAJOR,
           STRICTNUM_VERSION_MINOR, STRICTNUM_VERSION_PATCH);
    before = g_fails; test_contract(); report_section("contract", before);
    before = g_fails; test_boundaries(); report_section("boundaries", before);
    before = g_fails; test_small_roundtrips(); report_section("small types", before);
#ifdef SN_HAVE_64
    before = g_fails; test_wide_roundtrips(); report_section("wide types", before);
    before = g_fails; test_64bit_contract(); report_section("64-bit contract", before);
#else
    printf("  wide types       skipped (64-bit API disabled)\n");
#endif
    before = g_fails; test_32bit_random_roundtrips(); report_section("32-bit random", before);
    before = g_fails; test_random_strings(); report_section("random strings", before);
    before = g_fails; test_enumeration(); report_section("enumeration", before);
    before = g_fails; test_locale(); report_section("locale", before);
    printf("%lu checks, %lu failures\n", g_checks, g_fails);
    return g_fails ? 1 : 0;
}
