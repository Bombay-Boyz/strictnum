#define SN_API extern
#include "../strictnum.h"
#include "../strictnum.h" /* idempotent include */
#ifndef SN_API
#  error "strictnum.h must not consume an unrelated consumer macro"
#endif
#undef SN_API

#ifdef STRICTNUM_NO_64
#  ifdef SN_HAVE_64
#    error "STRICTNUM_NO_64 must disable the 64-bit parsing API"
#  endif
#endif

#ifdef SN_DETAIL_CAST
#  error "private cast macro leaked from strictnum.h"
#endif
#ifdef SN_DETAIL_FN
#  error "private function macro leaked from strictnum.h"
#endif
#ifdef SN_DETAIL_DEFINE_Z_WRAPPERS
#  error "private wrapper macro leaked from strictnum.h"
#endif
#ifdef SN_DETAIL_STDINT_8_32
#  error "private feature-detection macro leaked from strictnum.h"
#endif
#ifdef SN_DETAIL_STDINT_64
#  error "private 64-bit feature macro leaked from strictnum.h"
#endif
#ifdef SN_DETAIL_UNUSED
#  error "private attribute macro leaked from strictnum.h"
#endif
#ifdef SN_DETAIL_MAKE_SIGNED
#  error "private conversion macro leaked from strictnum.h"
#endif
#if defined(SN_DETAIL_U8_MAX) || defined(SN_DETAIL_U16_MAX) || \
    defined(SN_DETAIL_U32_MAX) || defined(SN_DETAIL_I8_MAX) || \
    defined(SN_DETAIL_I16_MAX) || defined(SN_DETAIL_I32_MAX)
#  error "private range macro leaked from strictnum.h"
#endif
#if defined(SN_DETAIL_U64_MAX) || defined(SN_DETAIL_I64_MAX)
#  error "private 64-bit range macro leaked from strictnum.h"
#endif
#if (SN_HAS_STDINT != 0) && (SN_HAS_STDINT != 1)
#  error "SN_HAS_STDINT must be 0 or 1"
#endif
#if (SN_HAS_STDINT_32 != 0) && (SN_HAS_STDINT_32 != 1)
#  error "SN_HAS_STDINT_32 must be 0 or 1"
#endif
#if (SN_HAS_STDINT_64 != 0) && (SN_HAS_STDINT_64 != 1)
#  error "SN_HAS_STDINT_64 must be 0 or 1"
#endif

int main(void)
{
    sn_i8 i8v = 0;
    sn_i16 i16v = 0;
    sn_i32 i32v = 0;
    sn_u8 u8v = 0;
    sn_u16 u16v = 0;
    sn_u32 u32v = 0;
    if (sn_parse_i8_z("-8", &i8v) != SN_OK || i8v != -8 ||
        sn_parse_i16_z("-16", &i16v) != SN_OK || i16v != -16 ||
        sn_parse_i32_z("-32", &i32v) != SN_OK || i32v != -32 ||
        sn_parse_u8_z("8", &u8v) != SN_OK || u8v != 8u ||
        sn_parse_u16_z("16", &u16v) != SN_OK || u16v != 16u ||
        sn_parse_u32_z("32", &u32v) != SN_OK || u32v != 32u)
        return 1;
#ifdef SN_HAVE_64
    {
        sn_i64 i64v = 0;
        sn_u64 u64v = 0;
        if (sn_parse_i64_z("-64", &i64v) != SN_OK || i64v != -64 ||
            sn_parse_u64_z("64", &u64v) != SN_OK || u64v != 64u)
            return 1;
    }
#endif
    return 0;
}
