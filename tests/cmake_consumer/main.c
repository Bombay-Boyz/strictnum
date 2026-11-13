#include "strictnum.h"

int main(void)
{
    sn_u32 value = 0;
    if (sn_parse_u32_z("4294967295", &value) != SN_OK)
        return 1;
    return value == (sn_u32)~(sn_u32)0 ? 0 : 1;
}
