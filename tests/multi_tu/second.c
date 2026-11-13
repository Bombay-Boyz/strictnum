#define STRICTNUM_NO_64
#include "../../strictnum.h"
int parse_second_translation_unit(void)
{
    sn_i32 value = 0;
    return sn_parse_i32_z("-56", &value) == SN_OK && value == -56;
}
