/* Read one signed 32-bit integer per physical line and print the maximum.
 *
 * Input contract:
 *   - LF and CRLF line endings are accepted.
 *   - Embedded NUL bytes are rejected by the explicit-length parser call.
 *   - Lines longer than LINE_CAP - 1 bytes are rejected and fully drained.
 *   - An empty file is an error; blank lines are invalid records.
 *   - The computed result is not published unless the entire input succeeds.
 */
#include <limits.h>
#include <stdio.h>
#include "../strictnum.h"

#define LINE_CAP 128u

enum line_result {
    LINE_READ_ERROR = -1,
    LINE_READ_EOF = 0,
    LINE_READ_OK = 1,
    LINE_READ_TOO_LONG = 2
};

static int read_line(FILE *in, char *buf, size_t cap, size_t *len)
{
    size_t n = 0;
    int ch;
    int saw_input = 0;
    int too_long = 0;

    if (in == NULL || buf == NULL || len == NULL || cap < 2u)
        return LINE_READ_ERROR;

    while ((ch = fgetc(in)) != EOF) {
        saw_input = 1;
        if (ch == '\n')
            break;
        if (n + 1u < cap)
            buf[n++] = (char)(unsigned char)ch;
        else
            too_long = 1;
    }

    if (ch == EOF && ferror(in))
        return LINE_READ_ERROR;
    if (!saw_input)
        return LINE_READ_EOF;

    /* Strip CR only when it is part of a CRLF line ending. A bare CR remains
     * in the byte slice and is rejected as syntax by strictnum. */
    if (ch == '\n' && n > 0u && buf[n - 1u] == '\r')
        --n;

    buf[n] = '\0';
    *len = n;
    return too_long ? LINE_READ_TOO_LONG : LINE_READ_OK;
}

static int max_of_lines(FILE *in, sn_i32 *max_out)
{
    char line[LINE_CAP];
    sn_i32 maximum = 0;
    unsigned long line_number = 0;
    int have_value = 0;

    if (in == NULL || max_out == NULL)
        return -1;

    for (;;) {
        size_t len = 0;
        sn_i32 value;
        sn_status status;
        int result = read_line(in, line, sizeof line, &len);

        if (result == LINE_READ_EOF)
            break;
        if (line_number < ULONG_MAX)
            ++line_number;

        if (result == LINE_READ_ERROR) {
            fprintf(stderr, "input error while reading line %lu\n", line_number);
            return -1;
        }
        if (result == LINE_READ_TOO_LONG) {
            fprintf(stderr, "line %lu: line exceeds %u bytes\n",
                    line_number, (unsigned)(LINE_CAP - 1u));
            return -1;
        }

        status = sn_parse_i32(line, len, &value);
        if (status != SN_OK) {
            fprintf(stderr, "line %lu: %s\n",
                    line_number, sn_status_string(status));
            return -1;
        }
        if (!have_value || value > maximum) {
            maximum = value;
            have_value = 1;
        }
    }

    if (!have_value) {
        fprintf(stderr, "input contains no integer records\n");
        return -1;
    }
    *max_out = maximum;
    return 0;
}

int main(void)
{
    sn_i32 maximum;

    if (max_of_lines(stdin, &maximum) != 0)
        return 1;
    printf("maximum = %ld\n", (long)maximum);
    return 0;
}
