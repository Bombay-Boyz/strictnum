int parse_first_translation_unit(void);
int parse_second_translation_unit(void);
int main(void)
{
    return parse_first_translation_unit() && parse_second_translation_unit()
        ? 0 : 1;
}
