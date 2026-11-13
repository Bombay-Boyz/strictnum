/* Read strict "key=value" records from stdin and parse typed values.
 *
 *   printf 'workers=8\ntimeout_ms=2500\nmask=ff\noffset=-42\n' | ./kv
 *
 * The reader counts bytes itself, rejects embedded NUL bytes, drains
 * overlong physical lines, handles CRLF, and reports stream errors.
 */
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "../strictnum.h"

#define LINE_CAP 256u

enum line_result {
    LINE_ERROR = -1,
    LINE_EOF = 0,
    LINE_OK = 1,
    LINE_TOO_LONG = 2,
    LINE_HAS_NUL = 3
};

/* Read one physical line. The returned length excludes LF and optional CR.
 * A too-long line is fully drained so its suffix is never treated as a new
 * record. Embedded NUL is reported separately from ordinary line content. */
static int read_line(FILE *in, char *buf, size_t cap, size_t *len)
{
    size_t n = 0;
    int ch, saw_byte = 0, too_long = 0, has_nul = 0;

    if (buf == NULL || cap < 2u || len == NULL)
        return LINE_ERROR;

    while ((ch = fgetc(in)) != EOF) {
        saw_byte = 1;
        if (ch == '\n')
            break;
        if (ch == 0)
            has_nul = 1;
        if (n + 1u < cap)
            buf[n++] = (char)(unsigned char)ch;
        else
            too_long = 1;
    }

    if (ch == EOF && ferror(in))
        return LINE_ERROR;
    if (!saw_byte && n == 0u && !too_long && !has_nul)
        return LINE_EOF;

    if (ch == '\n' && n > 0u && buf[n - 1u] == '\r')
        --n;
    buf[n] = '\0';
    *len = n;

    if (too_long)
        return LINE_TOO_LONG;
    if (has_nul)
        return LINE_HAS_NUL;
    return LINE_OK;
}

static int key_is(const char *k, size_t klen, const char *name)
{
    return strlen(name) == klen && memcmp(k, name, klen) == 0;
}

static void bad(unsigned long lineno, const char *what, sn_status st, int *had_errors)
{
    fprintf(stderr, "line %lu: %s: %s\n", lineno, what, sn_status_string(st));
    *had_errors = 1;
}

int main(void)
{
    char line[LINE_CAP];
    unsigned long lineno = 0;
    int had_errors = 0;
    int lr;

    for (;;) {
        size_t len = 0, klen, vlen;
        const char *val;
        char *eq;
        sn_status st;

        lr = read_line(stdin, line, sizeof line, &len);
        if (lr == LINE_EOF)
            break;
        if (lineno < ULONG_MAX)
            ++lineno;
        if (lr == LINE_ERROR) {
            fprintf(stderr, "input error while reading stdin\n");
            return 1;
        }
        if (lr == LINE_TOO_LONG) {
            fprintf(stderr, "line %lu: record exceeds %u bytes\n", lineno,
                    (unsigned)(LINE_CAP - 1u));
            had_errors = 1;
            continue;
        }
        if (lr == LINE_HAS_NUL) {
            fprintf(stderr, "line %lu: embedded NUL byte\n", lineno);
            had_errors = 1;
            continue;
        }
        if (len == 0u || line[0] == '#')
            continue;

        eq = (char *)memchr(line, '=', len);
        if (eq == NULL) {
            fprintf(stderr, "line %lu: missing '='\n", lineno);
            had_errors = 1;
            continue;
        }
        klen = (size_t)(eq - line);
        val = eq + 1;
        vlen = len - klen - 1u;

        if (key_is(line, klen, "workers")) {
            sn_u8 v;
            st = sn_parse_u8(val, vlen, &v);
            if (st == SN_OK) printf("workers    = %u\n", (unsigned)v);
            else bad(lineno, "workers", st, &had_errors);
        } else if (key_is(line, klen, "timeout_ms")) {
            sn_u32 v;
            st = sn_parse_u32(val, vlen, &v);
            if (st == SN_OK) printf("timeout_ms = %lu\n", (unsigned long)v);
            else bad(lineno, "timeout_ms", st, &had_errors);
        } else if (key_is(line, klen, "mask")) {
            sn_u32 v;
            st = sn_parse_u32_base(val, vlen, 16, &v);
            if (st == SN_OK) printf("mask       = 0x%lx\n", (unsigned long)v);
            else bad(lineno, "mask", st, &had_errors);
        } else if (key_is(line, klen, "offset")) {
            sn_i32 v;
            st = sn_parse_i32(val, vlen, &v);
            if (st == SN_OK) printf("offset     = %ld\n", (long)v);
            else bad(lineno, "offset", st, &had_errors);
        } else {
            fprintf(stderr, "line %lu: unknown key\n", lineno);
            had_errors = 1;
        }
    }

    if (ferror(stdin)) {
        fprintf(stderr, "input error while reading stdin\n");
        return 1;
    }
    return had_errors ? 1 : 0;
}
