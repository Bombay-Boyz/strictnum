/* Header hygiene: strictnum.h must be self-contained, idempotent, and must
 * not leak its internal macros into the including translation unit. */
#include "../strictnum.h"
#include "../strictnum.h" /* second inclusion must be harmless */

#if defined(SN_DETAIL_CAST) || defined(SN_DETAIL_FN) ||                      \
    defined(SN_DETAIL_MAKE_SIGNED) || defined(SN_DETAIL_DEFINE_Z_WRAPPERS) || \
    defined(SN_DETAIL_STDINT_8_32) || defined(SN_DETAIL_STDINT_64)
#error "strictnum.h leaked an internal macro"
#endif

#if !defined(STRICTNUM_VERSION_MAJOR) || !defined(STRICTNUM_VERSION_MINOR) || \
    !defined(STRICTNUM_VERSION_PATCH)
#error "strictnum.h must define its version macros"
#endif

int main(void)
{
    sn_u8 v = 0;
    return sn_parse_u8_z("255", &v) == SN_OK && v == 255u ? 0 : 1;
}
