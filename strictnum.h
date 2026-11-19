/* strictnum.h -- strict, portable integer parsing for C and C++.
 *
 * Version 1.0.1
 * Project: strictnum; public single-header API: strictnum.h.
 *
 * Drop this one file into your project and #include it. No build system,
 * no library to link, no configuration needed.
 *
 *     #include "strictnum.h"
 *
 *     sn_u16 port;
 *     if (sn_parse_u16_z("8080", &port) == SN_OK) { ... }
 *
 * WHAT "STRICT" MEANS
 *   - The WHOLE input must be a number. "12abc", " 12", "12 " are errors.
 *   - Only an optional leading '-' (signed types only) followed by digits.
 *     No '+', no whitespace, no "0x"/"0b"/"0" prefixes, no digit separators.
 *   - Leading zeros are digits: "007" is 7 and "08" is 8 (never octal).
 *   - Signed parsers accept "-0" (the value is 0).
 *   - Unsigned parsers reject '-' entirely (even "-0"); they never wrap.
 *   - Out-of-range values are errors, never clamped.
 *   - Locale independent: <ctype.h>, <errno.h> and <locale.h> are not used.
 *   - On any error the output variable is left untouched.
 *   - If an input has BOTH a syntax problem and an out-of-range magnitude,
 *     the syntax error is reported ("99999999999999999999x" -> SN_ERR_SYNTAX).
 *
 * PORTABILITY
 *   Written in the common subset of C89 and C++98; also valid C99/C11/C17/
 *   C23 and C++11..C++23. Uses <stddef.h> and <limits.h>, plus <stdint.h>
 *   when available. Does not depend on signed bitwise representation,
 *   the size of `long`, or ASCII letter ordering. Fallback signed types
 *   must provide the exact numeric ranges required by the fixed-width API.
 *   Signed result construction does not perform signed overflow.
 *
 * CONFIGURATION (define before including)
 *   STRICTNUM_NO_STDINT       Do not use <stdint.h>; derive the fixed-width
 *                             types from <limits.h> instead (C89 compilers).
 *   STRICTNUM_USE_LONG_LONG   With STRICTNUM_NO_STDINT: use `long long` for
 *                             the 64-bit types (a common C89 extension).
 *   STRICTNUM_NO_64           Omit the 64-bit functions entirely.
 *
 * FEATURE MACROS (defined by this header)
 *   SN_HAS_STDINT             1 if <stdint.h> is used, otherwise 0.
 *   SN_HAS_STDINT_32          1 if exact 8/16/32-bit stdint types are used.
 *   SN_HAS_STDINT_64          1 if <stdint.h> provides exact 64-bit types;
 *                             independent of STRICTNUM_NO_64.
 *   SN_HAVE_64                Defined if the 64-bit parser API is available.
 *
 * LICENSE: MIT. See the end of this file.
 */
#ifndef STRICTNUM_H
#define STRICTNUM_H

#include <stddef.h>
#include <limits.h>

#define STRICTNUM_VERSION_MAJOR 1
#define STRICTNUM_VERSION_MINOR 0
#define STRICTNUM_VERSION_PATCH 1

/* ------------------------------------------------------------------------
 * Compiler glue
 * --------------------------------------------------------------------- */

#ifdef __cplusplus
#  define SN_DETAIL_CAST(T, x) static_cast<T>(x)
#else
#  define SN_DETAIL_CAST(T, x) ((T)(x))
#endif

/* Private linkage policy: every definition has internal linkage.  Avoid an
 * unused attribute here: Clang diagnoses calling functions marked unused
 * under -Wused-but-marked-unused.  The GNU inline spelling is used for the
 * C89 extension path where available. */
#if defined(__cplusplus) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
#  define SN_DETAIL_FN static inline
#elif defined(__GNUC__) || defined(__clang__)
#  define SN_DETAIL_FN static __inline__
#elif defined(_MSC_VER)
#  define SN_DETAIL_FN static __inline
#else
#  define SN_DETAIL_FN static
#endif

/* ------------------------------------------------------------------------
 * Fixed-width types
 * --------------------------------------------------------------------- */

#if defined(STRICTNUM_NO_STDINT)
#  define SN_HAS_STDINT 0
#elif defined(__has_include)
#  if __has_include(<stdint.h>)
#    define SN_HAS_STDINT 1
#  else
#    define SN_HAS_STDINT 0
#  endif
#elif defined(_MSC_VER) && _MSC_VER >= 1600
#  define SN_HAS_STDINT 1
#elif defined(__GNUC__) && \
      ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
#  define SN_HAS_STDINT 1
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#  define SN_HAS_STDINT 1
#elif defined(__cplusplus) && __cplusplus >= 201103L
#  define SN_HAS_STDINT 1
#else
#  define SN_HAS_STDINT 0
#endif

#if SN_HAS_STDINT
#  include <stdint.h>
#  if defined(INT8_MAX) && defined(UINT8_MAX) && \
      defined(INT16_MAX) && defined(UINT16_MAX) && \
      defined(INT32_MAX) && defined(UINT32_MAX)
#    define SN_DETAIL_STDINT_8_32 1
#    define SN_HAS_STDINT_32 1
#  else
#    define SN_DETAIL_STDINT_8_32 0
#    define SN_HAS_STDINT_32 0
#  endif
#  if defined(INT64_MAX) && defined(UINT64_MAX)
#    define SN_DETAIL_STDINT_64 1
#    define SN_HAS_STDINT_64 1
#  else
#    define SN_DETAIL_STDINT_64 0
#    define SN_HAS_STDINT_64 0
#  endif
#else
#  define SN_DETAIL_STDINT_8_32 0
#  define SN_DETAIL_STDINT_64 0
#  define SN_HAS_STDINT_32 0
#  define SN_HAS_STDINT_64 0
#endif

#if SN_DETAIL_STDINT_8_32
typedef int8_t   sn_i8;
typedef uint8_t  sn_u8;
typedef int16_t  sn_i16;
typedef uint16_t sn_u16;
typedef int32_t  sn_i32;
typedef uint32_t sn_u32;
#else /* derive 8/16/32-bit types from <limits.h> */
#  if SCHAR_MIN == (-127 - 1) && SCHAR_MAX == 127 && UCHAR_MAX == 255
typedef signed char    sn_i8;
typedef unsigned char  sn_u8;
#  else
#    error "strictnum: cannot find exact 8-bit types"
#  endif

#  if SHRT_MIN == (-32767 - 1) && SHRT_MAX == 32767 && USHRT_MAX == 65535
typedef short          sn_i16;
typedef unsigned short sn_u16;
#  else
#    error "strictnum: cannot find exact 16-bit types"
#  endif

#  if INT_MIN == (-2147483647 - 1) && INT_MAX == 2147483647 && UINT_MAX == 4294967295U
typedef int            sn_i32;
typedef unsigned int   sn_u32;
#  elif LONG_MIN == (-2147483647L - 1L) && LONG_MAX == 2147483647L && ULONG_MAX == 4294967295UL
typedef long           sn_i32;
typedef unsigned long  sn_u32;
#  else
#    error "strictnum: cannot find exact 32-bit types"
#  endif
#endif

#ifndef STRICTNUM_NO_64
#  if SN_DETAIL_STDINT_64
typedef int64_t  sn_i64;
typedef uint64_t sn_u64;
#    define SN_HAVE_64 1
#  elif defined(STRICTNUM_USE_LONG_LONG)
#    if (defined(LLONG_MAX) && LLONG_MAX == 9223372036854775807LL) || \
        (defined(__LONG_LONG_MAX__) && __LONG_LONG_MAX__ == 9223372036854775807LL)
typedef long long          sn_i64;
typedef unsigned long long sn_u64;
#      define SN_HAVE_64 1
#    else
#      error "strictnum: long long is not a verified 64-bit type"
#    endif
#  elif LONG_MIN == (-LONG_MAX - 1L) && ((LONG_MAX >> 31) >> 31) == 1 && ((ULONG_MAX >> 31) >> 31) == 3
typedef long           sn_i64;
typedef unsigned long  sn_u64;
#    define SN_HAVE_64 1
#  endif
#endif /* !STRICTNUM_NO_64 */

/* Widest unsigned type used internally for accumulation. */
/* The accumulator must be at least as wide as the largest public type.
 * sn_u32 is sufficient when 64-bit APIs are disabled: the pre-multiply
 * limit check guarantees every performed intermediate fits in that type. */
#ifdef SN_HAVE_64
typedef sn_u64 strictnum_detail_umax;
#else
typedef sn_u32 strictnum_detail_umax;
#endif

/* Maximum values, built without relying on <stdint.h> limit macros or on
 * integer literals wider than 32 bits (both are unavailable in some modes). */
#define SN_DETAIL_U8_MAX   SN_DETAIL_CAST(strictnum_detail_umax, SN_DETAIL_CAST(sn_u8, ~SN_DETAIL_CAST(sn_u8, 0)))
#define SN_DETAIL_U16_MAX  SN_DETAIL_CAST(strictnum_detail_umax, SN_DETAIL_CAST(sn_u16, ~SN_DETAIL_CAST(sn_u16, 0)))
#define SN_DETAIL_U32_MAX  SN_DETAIL_CAST(strictnum_detail_umax, SN_DETAIL_CAST(sn_u32, ~SN_DETAIL_CAST(sn_u32, 0)))
#define SN_DETAIL_I8_MAX   (SN_DETAIL_U8_MAX >> 1)
#define SN_DETAIL_I16_MAX  (SN_DETAIL_U16_MAX >> 1)
#define SN_DETAIL_I32_MAX  (SN_DETAIL_U32_MAX >> 1)
#ifdef SN_HAVE_64
#  define SN_DETAIL_U64_MAX SN_DETAIL_CAST(strictnum_detail_umax, SN_DETAIL_CAST(sn_u64, ~SN_DETAIL_CAST(sn_u64, 0)))
#  define SN_DETAIL_I64_MAX (SN_DETAIL_U64_MAX >> 1)
#endif

/* ------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------- */

typedef enum sn_status {
    SN_OK         = 0, /* parsed; *out was written                              */
    SN_ERR_EMPTY  = 1, /* the input had length 0                                */
    SN_ERR_SYNTAX = 2, /* not a number: stray character, lone "-", sign on an
                          unsigned type, ...                                    */
    SN_ERR_RANGE  = 3, /* well-formed, but does not fit the target type         */
    SN_ERR_ARG    = 4  /* invalid pointer/length combination, or base outside 2..36 */
} sn_status;

/* Explicit-length API:
 *   sn_parse_<type>(s, len, &out)
 *   sn_parse_<type>_base(s, len, base, &out)
 * NUL-terminated API:
 *   sn_parse_<type>_z(s, &out)
 *   sn_parse_<type>_base_z(s, base, &out)
 *
 * Explicit-length functions read exactly len bytes; embedded NUL is syntax.
 * The _z functions scan to the first NUL. They are separate entry points so
 * a length converted from a failed signed I/O result cannot select scanning.
 * Bases are 2..36; letters are case-insensitive; no prefixes are accepted.
 * Types: i8 i16 i32 i64 (signed), u8 u16 u32 u64 (unsigned).
 * SN_HAVE_64 is defined when the i64/u64 API is available.
 */

/* Human-readable, static, never-NULL description of a status code. */
SN_DETAIL_FN const char *sn_status_string(sn_status st)
{
    switch (st) {
    case SN_OK:         return "ok";
    case SN_ERR_EMPTY:  return "empty input";
    case SN_ERR_SYNTAX: return "not a valid number";
    case SN_ERR_RANGE:  return "number out of range";
    case SN_ERR_ARG:    return "invalid argument";
    }
    return "unknown status";
}

/* ------------------------------------------------------------------------
 * Implementation (internal; names starting with strictnum_detail_ are not API)
 * --------------------------------------------------------------------- */

/* Value of digit character c (0..35), or -1. Digits '0'..'9' are guaranteed
 * contiguous by the C standard. Letters are only contiguous in ASCII-like
 * character sets, so other sets (e.g. EBCDIC) take the table path. */
SN_DETAIL_FN int strictnum_detail_digit(unsigned c)
{
    if (c >= SN_DETAIL_CAST(unsigned, '0') && c <= SN_DETAIL_CAST(unsigned, '9'))
        return SN_DETAIL_CAST(int, c - SN_DETAIL_CAST(unsigned, '0'));
#if !defined(STRICTNUM_TEST_FORCE_NONASCII_DIGITS) && ('z' - 'a' == 25) && ('Z' - 'A' == 25)
    if (c >= SN_DETAIL_CAST(unsigned, 'a') && c <= SN_DETAIL_CAST(unsigned, 'z'))
        return SN_DETAIL_CAST(int, c - SN_DETAIL_CAST(unsigned, 'a')) + 10;
    if (c >= SN_DETAIL_CAST(unsigned, 'A') && c <= SN_DETAIL_CAST(unsigned, 'Z'))
        return SN_DETAIL_CAST(int, c - SN_DETAIL_CAST(unsigned, 'A')) + 10;
    return -1;
#else
    {
        static const char lo[] = "abcdefghijklmnopqrstuvwxyz";
        static const char up[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        int i;
        for (i = 0; i < 26; ++i) {
            if (c == SN_DETAIL_CAST(unsigned char, lo[i]) || c == SN_DETAIL_CAST(unsigned char, up[i]))
                return 10 + i;
        }
        return -1;
    }
#endif
}

/* Validate public arguments once. The private length pointer is always the
 * address of a public entry point's local copy. A null data pointer is
 * permitted only for an empty explicit-length slice. */
SN_DETAIL_FN sn_status strictnum_detail_prep(const char *s, size_t *n, unsigned base, const void *out)
{
    if (out == NULL || base < 2u || base > 36u ||
        (s == NULL && *n != 0u))
        return SN_ERR_ARG;
    return SN_OK;
}

/* Length of a valid NUL-terminated string. Public _z wrappers reject NULL
 * before calling this helper. */
SN_DETAIL_FN size_t strictnum_detail_zlen(const char *s)
{
    size_t n = 0;
    while (s[n] != '\0')
        ++n;
    return n;
}

/* Scans s[0..n) as digits in `base`. Succeeds only if every byte is a valid
 * digit and the value is <= limit. The WHOLE input is always scanned, so a
 * syntax error anywhere wins over an earlier out-of-range prefix. */
SN_DETAIL_FN sn_status strictnum_detail_scan(const char *s, size_t n, unsigned base, strictnum_detail_umax limit, strictnum_detail_umax *v)
{
    strictnum_detail_umax acc = 0;
    const strictnum_detail_umax b = SN_DETAIL_CAST(strictnum_detail_umax, base);
    int overflow = 0;
    size_t i;

    if (n == 0)
        return SN_ERR_EMPTY;
    for (i = 0; i < n; ++i) {
        const int d = strictnum_detail_digit(SN_DETAIL_CAST(unsigned char, s[i]));
        if (d < 0 || SN_DETAIL_CAST(unsigned, d) >= base)
            return SN_ERR_SYNTAX;
        if (!overflow) {
            /* acc*b + d <= limit  <=>  acc <= (limit - d) / b   (limit >= 127 > d) */
            if (acc > (limit - SN_DETAIL_CAST(strictnum_detail_umax, d)) / b)
                overflow = 1;
            else
                acc = acc * b + SN_DETAIL_CAST(strictnum_detail_umax, d);
        }
    }
    if (overflow)
        return SN_ERR_RANGE;
    *v = acc;
    return SN_OK;
}

/* Optional '-', then digits. On success *mag is the magnitude (<= posmax,
 * or <= posmax + 1 when negative) and *neg says whether '-' was present. */
SN_DETAIL_FN sn_status strictnum_detail_scan_signed(const char *s, size_t n, unsigned base,
                                 strictnum_detail_umax posmax, strictnum_detail_umax *mag, int *neg)
{
    int is_neg = 0;
    if (n == 0)
        return SN_ERR_EMPTY;
    if (s[0] == '-') {
        is_neg = 1;
        ++s;
        --n;
        if (n == 0)
            return SN_ERR_SYNTAX;
    }
    {
        const sn_status st = strictnum_detail_scan(s, n, base, is_neg ? posmax + 1u : posmax, mag);
        if (st == SN_OK)
            *neg = is_neg;
        return st;
    }
}

/* Builds the signed result from a magnitude without signed overflow and
 * without assuming two's complement: -m == -(m-1) - 1, and m-1 <= TYPE_MAX. */
#define SN_DETAIL_MAKE_SIGNED(T, m, neg) \
    ((neg) ? ((m) == 0 ? SN_DETAIL_CAST(T, 0) : SN_DETAIL_CAST(T, -SN_DETAIL_CAST(T, (m) - 1u) - 1)) : SN_DETAIL_CAST(T, (m)))

/* --- signed ----------------------------------------------------------- */

SN_DETAIL_FN sn_status sn_parse_i8_base(const char *s, size_t n, unsigned base, sn_i8 *out)
{
    strictnum_detail_umax m = 0; int neg = 0;
    sn_status st;
    st = strictnum_detail_prep(s, &n, base, out);
    if (st != SN_OK) return st;
    st = strictnum_detail_scan_signed(s, n, base, SN_DETAIL_I8_MAX, &m, &neg);
    if (st != SN_OK) return st;
    *out = SN_DETAIL_MAKE_SIGNED(sn_i8, m, neg);
    return SN_OK;
}

SN_DETAIL_FN sn_status sn_parse_i16_base(const char *s, size_t n, unsigned base, sn_i16 *out)
{
    strictnum_detail_umax m = 0; int neg = 0;
    sn_status st;
    st = strictnum_detail_prep(s, &n, base, out);
    if (st != SN_OK) return st;
    st = strictnum_detail_scan_signed(s, n, base, SN_DETAIL_I16_MAX, &m, &neg);
    if (st != SN_OK) return st;
    *out = SN_DETAIL_MAKE_SIGNED(sn_i16, m, neg);
    return SN_OK;
}

SN_DETAIL_FN sn_status sn_parse_i32_base(const char *s, size_t n, unsigned base, sn_i32 *out)
{
    strictnum_detail_umax m = 0; int neg = 0;
    sn_status st;
    st = strictnum_detail_prep(s, &n, base, out);
    if (st != SN_OK) return st;
    st = strictnum_detail_scan_signed(s, n, base, SN_DETAIL_I32_MAX, &m, &neg);
    if (st != SN_OK) return st;
    *out = SN_DETAIL_MAKE_SIGNED(sn_i32, m, neg);
    return SN_OK;
}

#ifdef SN_HAVE_64
SN_DETAIL_FN sn_status sn_parse_i64_base(const char *s, size_t n, unsigned base, sn_i64 *out)
{
    strictnum_detail_umax m = 0; int neg = 0;
    sn_status st;
    st = strictnum_detail_prep(s, &n, base, out);
    if (st != SN_OK) return st;
    st = strictnum_detail_scan_signed(s, n, base, SN_DETAIL_I64_MAX, &m, &neg);
    if (st != SN_OK) return st;
    *out = SN_DETAIL_MAKE_SIGNED(sn_i64, m, neg);
    return SN_OK;
}
#endif

/* --- unsigned --------------------------------------------------------- */

SN_DETAIL_FN sn_status sn_parse_u8_base(const char *s, size_t n, unsigned base, sn_u8 *out)
{
    strictnum_detail_umax m = 0;
    sn_status st;
    st = strictnum_detail_prep(s, &n, base, out);
    if (st != SN_OK) return st;
    st = strictnum_detail_scan(s, n, base, SN_DETAIL_U8_MAX, &m);
    if (st != SN_OK) return st;
    *out = SN_DETAIL_CAST(sn_u8, m);
    return SN_OK;
}

SN_DETAIL_FN sn_status sn_parse_u16_base(const char *s, size_t n, unsigned base, sn_u16 *out)
{
    strictnum_detail_umax m = 0;
    sn_status st;
    st = strictnum_detail_prep(s, &n, base, out);
    if (st != SN_OK) return st;
    st = strictnum_detail_scan(s, n, base, SN_DETAIL_U16_MAX, &m);
    if (st != SN_OK) return st;
    *out = SN_DETAIL_CAST(sn_u16, m);
    return SN_OK;
}

SN_DETAIL_FN sn_status sn_parse_u32_base(const char *s, size_t n, unsigned base, sn_u32 *out)
{
    strictnum_detail_umax m = 0;
    sn_status st;
    st = strictnum_detail_prep(s, &n, base, out);
    if (st != SN_OK) return st;
    st = strictnum_detail_scan(s, n, base, SN_DETAIL_U32_MAX, &m);
    if (st != SN_OK) return st;
    *out = SN_DETAIL_CAST(sn_u32, m);
    return SN_OK;
}

#ifdef SN_HAVE_64
SN_DETAIL_FN sn_status sn_parse_u64_base(const char *s, size_t n, unsigned base, sn_u64 *out)
{
    strictnum_detail_umax m = 0;
    sn_status st;
    st = strictnum_detail_prep(s, &n, base, out);
    if (st != SN_OK) return st;
    st = strictnum_detail_scan(s, n, base, SN_DETAIL_U64_MAX, &m);
    if (st != SN_OK) return st;
    *out = SN_DETAIL_CAST(sn_u64, m);
    return SN_OK;
}
#endif

/* --- decimal shorthands ---------------------------------------------- */

SN_DETAIL_FN sn_status sn_parse_i8 (const char *s, size_t n, sn_i8  *out) { return sn_parse_i8_base (s, n, 10u, out); }
SN_DETAIL_FN sn_status sn_parse_i16(const char *s, size_t n, sn_i16 *out) { return sn_parse_i16_base(s, n, 10u, out); }
SN_DETAIL_FN sn_status sn_parse_i32(const char *s, size_t n, sn_i32 *out) { return sn_parse_i32_base(s, n, 10u, out); }
SN_DETAIL_FN sn_status sn_parse_u8 (const char *s, size_t n, sn_u8  *out) { return sn_parse_u8_base (s, n, 10u, out); }
SN_DETAIL_FN sn_status sn_parse_u16(const char *s, size_t n, sn_u16 *out) { return sn_parse_u16_base(s, n, 10u, out); }
SN_DETAIL_FN sn_status sn_parse_u32(const char *s, size_t n, sn_u32 *out) { return sn_parse_u32_base(s, n, 10u, out); }
#ifdef SN_HAVE_64
SN_DETAIL_FN sn_status sn_parse_i64(const char *s, size_t n, sn_i64 *out) { return sn_parse_i64_base(s, n, 10u, out); }
SN_DETAIL_FN sn_status sn_parse_u64(const char *s, size_t n, sn_u64 *out) { return sn_parse_u64_base(s, n, 10u, out); }
#endif

/* NUL-terminated wrappers. Keep these separate from the explicit-length
 * entry points: no sentinel value is overloaded into size_t. */
#define SN_DETAIL_DEFINE_Z_WRAPPERS(TN, T) \
    SN_DETAIL_FN sn_status sn_parse_##TN##_z(const char *s, T *out) \
    { return (s == NULL || out == NULL) ? SN_ERR_ARG : sn_parse_##TN(s, strictnum_detail_zlen(s), out); } \
    SN_DETAIL_FN sn_status sn_parse_##TN##_base_z(const char *s, unsigned base, T *out) \
    { return (s == NULL || out == NULL || base < 2u || base > 36u) ? SN_ERR_ARG : sn_parse_##TN##_base(s, strictnum_detail_zlen(s), base, out); }

SN_DETAIL_DEFINE_Z_WRAPPERS(i8, sn_i8)
SN_DETAIL_DEFINE_Z_WRAPPERS(i16, sn_i16)
SN_DETAIL_DEFINE_Z_WRAPPERS(i32, sn_i32)
SN_DETAIL_DEFINE_Z_WRAPPERS(u8, sn_u8)
SN_DETAIL_DEFINE_Z_WRAPPERS(u16, sn_u16)
SN_DETAIL_DEFINE_Z_WRAPPERS(u32, sn_u32)
#ifdef SN_HAVE_64
SN_DETAIL_DEFINE_Z_WRAPPERS(i64, sn_i64)
SN_DETAIL_DEFINE_Z_WRAPPERS(u64, sn_u64)
#endif
#undef SN_DETAIL_DEFINE_Z_WRAPPERS

/* Do not leak implementation-only macros into the including translation
 * unit. Public configuration and feature macros intentionally remain. */
#undef SN_DETAIL_CAST
#undef SN_DETAIL_FN
#undef SN_DETAIL_STDINT_8_32
#undef SN_DETAIL_STDINT_64
#undef SN_DETAIL_U8_MAX
#undef SN_DETAIL_U16_MAX
#undef SN_DETAIL_U32_MAX
#undef SN_DETAIL_I8_MAX
#undef SN_DETAIL_I16_MAX
#undef SN_DETAIL_I32_MAX
#ifdef SN_HAVE_64
#  undef SN_DETAIL_U64_MAX
#  undef SN_DETAIL_I64_MAX
#endif
#undef SN_DETAIL_MAKE_SIGNED

#endif /* STRICTNUM_H */

/* ----------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2026 sabrinathan nair
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * ---------------------------------------------------------------------- */
