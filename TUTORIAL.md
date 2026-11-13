# Tutorial: parsing integers safely with strictnum

This tutorial is a practical introduction to the API. Every C and C++ code
block below is compiled by `tests/check_docs.sh`, so the examples stay aligned
with the real API.

**Contents**

1. [Install](#1-install)
2. [Your first parse](#2-your-first-parse)
3. [Choosing a type](#3-choosing-a-type)
4. [Handling errors](#4-handling-errors)
5. [Command-line arguments](#5-command-line-arguments)
6. [Lines from a file or stdin](#6-lines-from-a-file-or-stdin)
7. [Text that is not NUL-terminated](#7-text-that-is-not-nul-terminated)
8. [Hex, binary and other bases](#8-hex-binary-and-other-bases)
9. [When you want to be less strict](#9-when-you-want-to-be-less-strict)
10. [Using it from C++](#10-using-it-from-c)
11. [Migrating from atoi, strtol and sscanf](#11-migrating-from-atoi-strtol-and-sscanf)
12. [Checklist](#12-checklist)

---

## 1. Install

There is nothing to build or link. Copy `strictnum.h` into your project
(next to your sources, or into an `include/` or `third_party/` directory) and
include it:

```c
#include "strictnum.h"
```

You can include it from as many `.c` files as you like. Every function has
internal linkage. The header uses the appropriate inline spelling where the
language or compiler provides one; there is no "define this macro in exactly
one file" step and no duplicate external definition to debug.

## 2. Your first parse

```c
#include <stdio.h>
#include "strictnum.h"

int main(void)
{
    sn_i32 n;
    sn_status st = sn_parse_i32_z("12345", &n);

    if (st == SN_OK)
        printf("parsed %ld\n", (long)n);
    else
        printf("error: %s\n", sn_status_string(st));
    return 0;
}
```

Three things to notice:

- Every parser has the same shape: `sn_parse_<type>(text, length, &result)`.
- Use the `_z` suffix when the input is a NUL-terminated C string. When you
  already know the byte length, use the explicit-length function instead
  (see [section 7](#7-text-that-is-not-nul-terminated)).
- The return value is a `sn_status`. The result variable is **only written on
  `SN_OK`**, so a failed parse can never leave you holding a half-parsed
  value.

## 3. Choosing a type

Pick the type that matches what the number *means*, and let the library
enforce the range for you.

| You are parsing...        | Use        | Accepts                          |
|---------------------------|------------|----------------------------------|
| A TCP/UDP port            | `sn_u16`   | 0 to 65535                       |
| A percentage, a byte      | `sn_u8`    | 0 to 255                         |
| A count, a size, an index | `sn_u32` / `sn_u64` | 0 to 2^32-1 / 2^64-1    |
| A signed quantity         | `sn_i32` / `sn_i64` | full signed range       |
| A small signed field      | `sn_i8` / `sn_i16`  | -128..127 / -32768..32767 |

Parsing into the exact type means a value like `70000` for a port is rejected
at the door with `SN_ERR_RANGE`, instead of being silently truncated later by
a cast.

Unsigned parsers reject a leading `-` outright. This is deliberate: the
standard `strtoul("-1", ...)` happily returns `ULONG_MAX`.

## 4. Handling errors

There are five status codes. Most code only needs to know "did it work", but
you can react to the kind of failure when it helps the user.

```c
#include <stdio.h>
#include "strictnum.h"

/* Style A: you only care whether it worked. */
static int read_count(const char *text, sn_u32 *out)
{
    return sn_parse_u32_z(text, out) == SN_OK;
}

/* Style B: tell the user what went wrong. */
static void report(const char *text)
{
    sn_i16 v;

    switch (sn_parse_i16_z(text, &v)) {
    case SN_OK:         printf("ok: %d\n", (int)v); break;
    case SN_ERR_EMPTY:  printf("please enter a number\n"); break;
    case SN_ERR_SYNTAX: printf("'%s' is not a number\n", text); break;
    case SN_ERR_RANGE:  printf("'%s' does not fit in 16 bits\n", text); break;
    case SN_ERR_ARG:    printf("internal error: bad arguments\n"); break;
    }
}
```

| Status          | Meaning                                              | Example input     |
|-----------------|------------------------------------------------------|-------------------|
| `SN_OK`         | Parsed. The result was written.                      | `"42"`            |
| `SN_ERR_EMPTY`  | Length zero.                                         | `""`              |
| `SN_ERR_SYNTAX` | Not a plain number.                                  | `"12abc"`, `"-"`, `" 5"`, `"+5"` |
| `SN_ERR_RANGE`  | A valid number that does not fit the type.           | `"300"` as `sn_u8` |
| `SN_ERR_ARG`    | `NULL` output, NULL text for `_z`, NULL text with nonzero length, or base outside 2..36.      | `NULL`            |

`sn_status_string()` gives you a short, static, never-`NULL` message for any
status, suitable for logs and error messages.

If an input is both malformed *and* out of range (for example
`"99999999999999999999x"`), you always get `SN_ERR_SYNTAX`. The same input
therefore always produces the same error, regardless of where the bad
character sits.

## 5. Command-line arguments

The complete program is in [`examples/port.c`](examples/port.c):

```c
#include <stdio.h>
#include "strictnum.h"

int main(int argc, char **argv)
{
    sn_u16 port;
    sn_status st;

    if (argc != 2) {
        fprintf(stderr, "usage: %s PORT\n", argv[0]);
        return 2;
    }

    st = sn_parse_u16_z(argv[1], &port);
    if (st != SN_OK || port == 0) {
        fprintf(stderr, "invalid port \"%s\": %s\n", argv[1],
                st == SN_OK ? "port must be 1..65535" : sn_status_string(st));
        return 1;
    }

    printf("port = %u\n", (unsigned)port);
    return 0;
}
```

Try it:

```text
$ ./port 8080
port = 8080
$ ./port 70000
invalid port "70000": number out of range
$ ./port 80abc
invalid port "80abc": not a valid number
$ ./port -5
invalid port "-5": not a valid number
$ ./port 0
invalid port "0": must be 1-65535
```

## 6. Lines from a file or stdin

A strict parser must receive the exact byte count. Do not use `strlen()` to
measure arbitrary file input: an embedded NUL byte would make `strlen()` stop
early and could cause a valid numeric prefix to be accepted while ignoring
malformed bytes after it. `fgets()` also needs explicit handling for
truncated physical lines and stream errors.

The complete, tested reader is [`examples/max_lines.c`](examples/max_lines.c).
It reads one byte at a time, passes the measured length to `strictnum`,
rejects embedded NUL bytes, drains overlong lines, accepts LF and CRLF line
endings, checks I/O errors, and leaves the output unchanged on failure.
Its policy is explicit: an empty file is an error, and every physical line
must contain one valid integer.

Build and run it:

```sh
cc -std=c99 -Wall -Wextra -Wpedantic -Werror -I. examples/max_lines.c -o max_lines
printf '12\n-4\n31\n' | ./max_lines
```

Expected output:

```text
maximum = 31
```

## 7. Text that is not NUL-terminated
Use explicit lengths for buffers returned by `read()` or `recv()`. Check the
signed return value for an error before converting it to `size_t`; the parser
has no sentinel value that changes the meaning of a length.


Network buffers, memory-mapped files, fixed-width records and `std::string`
slices are not NUL-terminated. Pass the length and the parser reads exactly
that many bytes, no more:

```c
#include <stddef.h>
#include "strictnum.h"

/* A fixed-width record: bytes 8..11 hold a zero-padded number like "0042".
 * The caller must provide at least 12 readable bytes. */
static int read_field(const char *record, sn_u16 *out)
{
    if (record == NULL || out == NULL)
        return 0;
    return sn_parse_u16(record + 8, 4, out) == SN_OK;
}

/* Parse exactly three comma-separated integers, e.g. "10,-20,30".
 * The caller supplies `len` readable bytes. Output is transactional. */
static int parse_triple(const char *s, size_t len, sn_i32 out[3])
{
    sn_i32 parsed[3];
    size_t start = 0, i;
    int field = 0;

    if (s == NULL || out == NULL)
        return 0;
    for (i = 0; i < len; ++i) {
        if (s[i] == ',') {
            if (field >= 2)
                return 0;                           /* too many fields */
            if (sn_parse_i32(s + start, i - start, &parsed[field]) != SN_OK)
                return 0;
            ++field;
            start = i + 1;
        }
    }
    if (field != 2 ||
        sn_parse_i32(s + start, len - start, &parsed[2]) != SN_OK)
        return 0;
    for (i = 0; i < 3; ++i)
        out[i] = parsed[i];
    return 1;
}
```

Two guarantees that make this safe:

- With an explicit length, an embedded NUL byte is an ordinary invalid
  character (`SN_ERR_SYNTAX`), not a terminator.
- The caller must ensure that every byte in the supplied explicit-length
  range is readable. The parser cannot validate pointer bounds. An embedded
  NUL inside that readable range is an ordinary invalid byte.

## 8. Hex, binary and other bases

Every parser has a `_base` variant that takes a base from 2 to 36. Letters
are case-insensitive, and there is **no prefix handling**: `"0x1F"` is not
valid base-16 input, because `x` is not a hex digit.

```c
#include <string.h>
#include "strictnum.h"

/* "#ff8800" -> 0xFF8800 */
/* The input must be a valid NUL-terminated C string. */
static int parse_color(const char *s, sn_u32 *rgb)
{
    if (s == NULL || rgb == NULL || strlen(s) != 7 || s[0] != '#')
        return 0;
    return sn_parse_u32_base(s + 1, 6, 16, rgb) == SN_OK;
}

/* Accept either "0x1F" or "31". Decide the base yourself, explicitly. */
static int parse_maybe_hex(const char *s, sn_u32 *out)
{
    size_t len;
    if (s == NULL || out == NULL)
        return 0;
    len = strlen(s);
    if (len >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        return sn_parse_u32_base(s + 2, len - 2, 16, out) == SN_OK;
    return sn_parse_u32(s, len, out) == SN_OK;
}

/* A permission mask in octal, like "755". */
static int parse_mode(const char *s, sn_u16 *mode)
{
    return sn_parse_u16_base_z(s, 8, mode) == SN_OK;
}
```

Making you choose the base explicitly is the point: the standard `base 0`
auto-detection turns `"010"` into 8, which is almost never what a user typing
a decimal number wants.

## 9. When you want to be less strict

Strictness is the default, but you can loosen it in a few lines by
preprocessing the text. The loosening is then visible in your code instead of
hidden inside a library function.

```c
#include <stddef.h>
#include "strictnum.h"

static int is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/* Accepts surrounding whitespace and one leading '+'. */
static sn_status parse_lenient_i32(const char *s, size_t len, sn_i32 *out)
{
    if (s == NULL)
        return SN_ERR_ARG;
    while (len > 0 && is_space(s[0])) { ++s; --len; }
    while (len > 0 && is_space(s[len - 1])) --len;
    if (len > 1 && s[0] == '+' && s[1] != '-') { ++s; --len; }
    return sn_parse_i32(s, len, out);
}
```

(The `s[1] != '-'` check keeps `"+-5"` invalid instead of quietly becoming
`-5`.)

## 10. Using it from C++

The header is tested in C++98, C++11, C++14, C++17, C++20 and C++23 modes
when the compiler supports those modes. It has no dependencies beyond the C
standard headers. The `std::string` example below works in C++98; the
additional `std::string_view` overload requires C++17. Use `.data()` and
`.size()` so that embedded NUL bytes are treated as errors:

```cpp
#include <string>
#include "strictnum.h"

bool parse_port(const std::string &text, unsigned short &port)
{
    sn_u16 v;
    if (sn_parse_u16(text.data(), text.size(), &v) != SN_OK)
        return false;
    port = v;
    return true;
}

#if __cplusplus >= 201703L
#include <string_view>
bool parse_count(std::string_view text, unsigned long &count)
{
    sn_u32 v;
    if (sn_parse_u32(text.data(), text.size(), &v) != SN_OK)
        return false;
    count = v;
    return true;
}
#endif
```

Use the explicit-length function with `.data()` and `.size()` for
`std::string_view`; do not use a `_z` wrapper unless the view is known to end
at a NUL byte.

## 11. Migrating from atoi, strtol and sscanf

**`atoi` / `atol`.** They return 0 for garbage, cannot report errors, and have
undefined behaviour when the value does not fit. Replace them everywhere:

```c
#include <stdlib.h>
#include "strictnum.h"

static int before(const char *s)
{
    return atoi(s);                 /* "abc" -> 0, "99999999999" -> undefined */
}

static int after(const char *s, sn_i32 *out)
{
    return sn_parse_i32_z(s, out) == SN_OK;
}
```

**`strtol` done correctly.** Getting it right takes several steps, and the
result still accepts leading whitespace and a leading `+`:

```c
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include "strictnum.h"

static int before(const char *s, int *out)
{
    char *end;
    long v;

    if (s == NULL || out == NULL)
        return 0;

    errno = 0;
    v = strtol(s, &end, 10);
    if (end == s || *end != '\0')
        return 0;                               /* nothing parsed, or junk after */
    if (errno == ERANGE || v < INT_MIN || v > INT_MAX)
        return 0;
    *out = (int)v;
    return 1;
}

static int after(const char *s, sn_i32 *out)
{
    return sn_parse_i32_z(s, out) == SN_OK;
}
```

**`sscanf("%d", ...)`.** Overflow is undefined behaviour, trailing text is
silently ignored, and there is no way to tell what went wrong. Replace it with
the matching `sn_parse_*` call on the token you extracted.

## 12. Checklist

- Parse into the narrowest type that matches the meaning of the value.
- Check the status. Do not read the output unless it was `SN_OK`.
- Pass the real length for byte slices; use the `_z` wrappers only for
  NUL-terminated C strings.
- Strip whitespace and line endings by adjusting pointer and length before
  parsing, not by loosening the parser.
- Choose the base explicitly. Handle `0x` yourself if you need it.
- The library does not touch `errno`, locales or global state, so it is safe
  to call from multiple threads.

Questions or surprises? Please open an issue: a confusing result is a bug in
either the code or the documentation, and both are worth fixing.
