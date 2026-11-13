#include "../../strictnum.h"
int parse_first_translation_unit(void)
{
    sn_u16 value = 0;
    return sn_parse_u16_z("1234", &value) == SN_OK && value == 1234u;
}
