#include "strictnum.h"
int main(void)
{
    sn_i32 value = 0;
    return sn_parse_i32_z("42", &value) == SN_OK && value == 42 ? 0 : 1;
}
