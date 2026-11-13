# strictnum

**`strictnum.h` — strict, fixed-width integer parsing for C and C++.**

```c
#include <stdio.h>
#include "strictnum.h"

int main(int argc, char **argv)
{
    sn_u16 port;
    if (argc != 2) {
        fprintf(stderr, "usage: %s PORT (1..65535)\n", argv[0]);
        return 1;
    }
    if (sn_parse_u16_z(argv[1], &port) != SN_OK || port == 0) {
        fprintf(stderr, "invalid port; expected 1..65535\n");
        return 1;
    }
    printf("port = %u\n", (unsigned)port);
    return 0;
}
```

`strictnum` converts a complete byte sequence to a fixed-width integer, or
returns a specific error. It is intended for configuration values, protocol
fields and other inputs where partial conversion and silent wrapping are
unacceptable.

## Disclosure
This project has been prepared using AI. 

## Design properties

- **Whole-input validation.** `"12abc"`, `" 12"` and `"12 "` are rejected.
- **Canonical syntax.** Signed types accept an optional leading `-`; `+`,
  whitespace, digit separators and automatic prefixes are rejected.
- **No unsigned negation.** Unsigned parsers reject `-`, including `-0`.
- **Checked range.** Overflow is detected before the multiply/add operation.
- **Transactional output.** `*out` is written only when the result is `SN_OK`.
- **Defined error precedence.** A syntax error anywhere in the input takes
  precedence over a range error in an earlier prefix.
- **Locale independent.** No `<ctype.h>`, `<locale.h>` or `errno` dependency in
  the library implementation; no mutable global state or allocation.
- **Explicit lengths.** A caller may parse a non-NUL-terminated byte slice.
- **Constant auxiliary space.** Parsing takes O(n) time and O(1) auxiliary
  space, where n is the supplied input length.

## Install and integrate

### Direct single-header integration

Copy `strictnum.h` into your source tree and include it. No library object or
link step is required.

```c
#include "strictnum.h"
```

Every function definition has internal linkage (`static inline` where the
language/compiler supports it). The header can therefore be included in
multiple translation units without creating duplicate external definitions.
There is deliberately no external-linkage or DLL-export override.

### CMake integration

The project exports a header-only interface target named
`strictnum::strictnum`:

```cmake
find_package(strictnum 1 CONFIG REQUIRED)
target_link_libraries(my_application PRIVATE strictnum::strictnum)
```

CMake 3.15 or newer is required for the package configuration.

To install it locally:

```sh
cmake -S . -B build -DCMAKE_INSTALL_PREFIX="$HOME/.local"
cmake --install build
```

When configuring a consumer, set `CMAKE_PREFIX_PATH` to the installation
prefix if it is not already searched by CMake. Direct header inclusion does
not require CMake; this target is an integration convenience.

## API

```text
sn_status sn_parse_<type>(const char *text, size_t len, sn_<type> *out);
sn_status sn_parse_<type>_base(const char *text, size_t len,
                               unsigned base, sn_<type> *out);
sn_status sn_parse_<type>_z(const char *text, sn_<type> *out);
sn_status sn_parse_<type>_base_z(const char *text, unsigned base,
                                 sn_<type> *out);
const char *sn_status_string(sn_status status);
```

| Type | Range |
|---|---:|
| `sn_i8` | -128 to 127 |
| `sn_i16` | -32768 to 32767 |
| `sn_i32` | -2147483648 to 2147483647 |
| `sn_i64` | -2^63 to 2^63-1, when the 64-bit API is available |
| `sn_u8` | 0 to 255 |
| `sn_u16` | 0 to 65535 |
| `sn_u32` | 0 to 4294967295 |
| `sn_u64` | 0 to 2^64-1, when the 64-bit API is available |

The decimal functions use base 10. The `_base` functions accept bases 2 through
36; alphabetic digits are case-insensitive. Prefixes such as `0x` and `0b`
are not consumed automatically.

### Input and length contract

- The explicit-length functions always inspect exactly `len` bytes. A caller
  must provide at least that many readable bytes; embedded NUL is a syntax
  error, not a terminator.
- The `_z` functions are separate NUL-terminated entry points. They scan to
  the first NUL and reject a null pointer with `SN_ERR_ARG`.
- There is no overloaded length sentinel. Never convert a possibly negative
  I/O return value to `size_t`; check it before calling an explicit-length
  parser.
- A null pointer with explicit length zero is an empty input and returns
  `SN_ERR_EMPTY`; a null pointer with nonzero length returns `SN_ERR_ARG`.
- The library cannot validate whether an arbitrary pointer actually refers to
  `len` readable bytes. That remains a caller precondition.
- `out` must point to a writable object of the corresponding `sn_*` type.
- Migration from the pre-release sentinel API: replace
  `sn_parse_i32(text, SN_NTS, &value)` with `sn_parse_i32_z(text, &value)`;
  explicit-length calls keep their original `(text, len, &value)` shape.

### Status values

| Status | Meaning |
|---|---|
| `SN_OK` | Success; output was written. |
| `SN_ERR_EMPTY` | Empty input. |
| `SN_ERR_SYNTAX` | Invalid character, sign, or digit for the selected base. |
| `SN_ERR_RANGE` | Syntactically valid number does not fit the target type. |
| `SN_ERR_ARG` | Invalid pointer/length combination, null `_z` input, null output, or base outside 2..36. |

### Configuration and feature detection

Define configuration macros before including the header:

- `STRICTNUM_NO_STDINT`: derive fixed-width types from `<limits.h>` rather
  than including `<stdint.h>`.
- `STRICTNUM_USE_LONG_LONG`: with `STRICTNUM_NO_STDINT`, enable the 64-bit
  API through a verified `long long` implementation extension.
- `STRICTNUM_NO_64`: omit the 64-bit API.

The header defines these feature macros after inclusion:

- `SN_HAS_STDINT`: `1` if `<stdint.h>` was included, otherwise `0`.
- `SN_HAS_STDINT_32`: `1` if exact 8-, 16- and 32-bit `<stdint.h>` types
  supply the public types, otherwise `0`.
- `SN_HAS_STDINT_64`: `1` if `<stdint.h>` provides exact 64-bit types,
  otherwise `0`. This reports type availability, not API enablement:
  `STRICTNUM_NO_64` can disable the 64-bit parser API even when this is `1`.
- `SN_HAVE_64`: defined only when the `sn_i64`/`sn_u64` API is available.

The 8-, 16- and 32-bit exact-width types must be available. The 64-bit API is
provided only when an exact 64-bit type is available through `<stdint.h>`, a
verified 64-bit `long`, or the explicitly requested `long long` fallback.
Internal implementation macros are undefined before the header finishes
processing; applications should use only the documented configuration and
feature macros. The header does not reserve or reject unrelated consumer macros such as
`SN_API`; all definitions retain internal linkage to preserve safe
multi-translation-unit header-only use.

### Public function inventory

For each available type below, the library provides decimal and explicit-
base functions in both explicit-length and NUL-terminated forms:

| Type | Decimal / length | Base / length | Decimal / NUL | Base / NUL |
|---|---|---|---|---|
| `sn_i8` | `sn_parse_i8` | `sn_parse_i8_base` | `sn_parse_i8_z` | `sn_parse_i8_base_z` |
| `sn_i16` | `sn_parse_i16` | `sn_parse_i16_base` | `sn_parse_i16_z` | `sn_parse_i16_base_z` |
| `sn_i32` | `sn_parse_i32` | `sn_parse_i32_base` | `sn_parse_i32_z` | `sn_parse_i32_base_z` |
| `sn_i64` | `sn_parse_i64` | `sn_parse_i64_base` | `sn_parse_i64_z` | `sn_parse_i64_base_z` |
| `sn_u8` | `sn_parse_u8` | `sn_parse_u8_base` | `sn_parse_u8_z` | `sn_parse_u8_base_z` |
| `sn_u16` | `sn_parse_u16` | `sn_parse_u16_base` | `sn_parse_u16_z` | `sn_parse_u16_base_z` |
| `sn_u32` | `sn_parse_u32` | `sn_parse_u32_base` | `sn_parse_u32_z` | `sn_parse_u32_base_z` |
| `sn_u64` | `sn_parse_u64` | `sn_parse_u64_base` | `sn_parse_u64_z` | `sn_parse_u64_base_z` |

The `i64` and `u64` rows are available only when `SN_HAVE_64` is defined.
`sn_status_string` converts a status value to a static diagnostic string.

### Displaying parsed values

For `sn_u8`, `sn_u16`, and `sn_u32`, promote to `unsigned long` and use
`%lu`; for `sn_i8`, `sn_i16`, and `sn_i32`, promote to `long` and use `%ld`.
These conversions are safe because `long` can represent every 32-bit signed
value and `unsigned long` can represent every 32-bit unsigned value on an
implementation that supplies the library's 32-bit types.

For 64-bit values, do not guess a `printf` length modifier. When
`SN_HAS_STDINT_64 == 1` and `SN_HAVE_64` is defined, include `<inttypes.h>`
and use `PRId64` / `PRIu64` with `int64_t` / `uint64_t`. For fallback
configurations, `sn_i64` / `sn_u64` may be `long` or `long long`; select the
matching conversion specifier only after checking the selected typedef. The
library deliberately does not expose a universal `printf` format macro.

## Testing and verification

The repository contains reproducible test programs and runners. To run the
checks available on the current machine:

```sh
sh tests/run_all.sh
sh tests/check_docs.sh
```

The runner performs:

- the C99 differential/property suite;
- C89 fallback tests with and without the `long long` extension;
- C11 and C17 smoke tests, plus C23 or the clearly labelled C23 draft mode
  when supported by the compiler;
- the full portable test suite in C++98, C++11, C++14, C++17, C++20 and C++23
  modes when supported (draft spellings are labelled as such);
- compilation of documentation examples;
- compilation and runtime checks of all shipped examples, including embedded
  NUL, CRLF, empty-file and overlong-line input;
- C and C++ multi-translation-unit link tests and header macro-leak checks;
- Clang reserved-identifier diagnostics when `clang++` is installed;
- CMake install/export and downstream `find_package` integration when CMake
  is installed;
- Clang AddressSanitizer/UndefinedBehaviorSanitizer and semantic libFuzzer
  checks when the installed Clang supports them;
- native x86 32-bit execution when the local compiler and runtime support it.

Local developer machines may skip unavailable optional facilities. CI has
separate mandatory Linux 32-bit and libFuzzer jobs; those jobs fail rather
than silently passing when the required toolchain is unavailable. See
[`TESTING.md`](TESTING.md) for commands and CI coverage.


**Verification status:** the GitHub Actions workflow completed successfully
for commit `d0fd865a0b8b6ae2b91c48b7b74a682fb38769f1` on 13 November 2026.

The successful run verified the following configured targets:

- Linux x86-64 and ARM64;
- macOS Intel and Apple Silicon;
- Windows MSVC x64 and x86;
- Linux x86 32-bit execution (ILP32);
- Linux Clang AddressSanitizer/UndefinedBehaviorSanitizer and mandatory
  semantic libFuzzer testing.

These results establish verification for the listed configurations and
that specific commit. They do not establish compatibility with every
compiler, operating system, architecture or C implementation.

Big-endian, EBCDIC and 8-/16-bit microcontroller targets are not currently
verified. Local developer machines may skip optional facilities when the
required toolchain or runtime is unavailable. See [`TESTING.md`](TESTING.md)
for the test commands and CI coverage.


## Examples and tutorial

- [`examples/port.c`](examples/port.c): strict TCP port parsing.
- [`examples/kv.c`](examples/kv.c): bounded key/value input with explicit
  byte-count handling.
- [`examples/max_lines.c`](examples/max_lines.c): bounded line-oriented input,
  including embedded-NUL, overlong-line and stream-error handling.
- [`TUTORIAL.md`](TUTORIAL.md): API use, bases, explicit-length slices and
  integration guidance.

## Contributing

For a bug report, include a minimal failing input, expected result, actual
result, compiler identity/version, target architecture and exact build
command. For code changes, run `sh tests/run_all.sh` and
`sh tests/check_docs.sh`. New parsing behaviour requires regression tests.

## Non-goals

Floating-point conversion, locale-aware parsing, whitespace trimming,
automatic base detection and prefix inference are outside this library's
scope.

## License

MIT. See [`LICENSE`](LICENSE).

## Example input-reader contracts

The examples intentionally demonstrate different record policies:

- `examples/kv.c` reads bounded `key=value` records. It skips blank lines and
  comments, accepts LF and CRLF, rejects embedded NUL and overlong records,
  and accepts a final record without a trailing LF. A bare CR is retained as
  data and therefore rejected when it occurs in a parsed value.
- `examples/max_lines.c` requires at least one non-empty integer record. It
  accepts LF and CRLF, rejects blank records, embedded NUL, bare CR and
  overlong lines, drains an overlong physical line, and accepts a final line
  without a trailing LF. It publishes the maximum only if the entire input is
  valid.

These are example-program policies, not additional behaviours of the parser.

## Choosing strictnum over standard conversion functions

`strtol` and `strtoul` are useful general-purpose conversions, but callers
must check `errno`, the end pointer, and target-type bounds. They also accept
leading whitespace and a leading `+`, and their behaviour is locale-sensitive
for some bases. `strictnum` instead provides fixed-width targets, whole-slice
validation, explicit error statuses, and an unchanged output on failure.

In C++17 and later, `std::from_chars` is also a strong low-level alternative:
it is locale-independent, non-allocating, and reports the parsed end pointer
and range errors. Prefer it when you are already using C++17+, need its
standard-library integration, or want partial parsing. Prefer `strictnum`
when you need the same fixed-width API in C89/C99-era code and C++98, or want
strict whole-input validation with identical semantics across those languages.

## Reentrancy and concurrency

The implementation has no mutable global state, allocation, locale changes,
or `errno` side effects. Calls are reentrant and can be made concurrently
provided each call receives valid input memory that is not being modified
concurrently and each call writes to its own output object.

## Repository hosting name

The source and CMake project are named `strictnum`. See [REPOSITORY_RENAME.md](REPOSITORY_RENAME.md) for the separate hosted-repository rename and remote-URL update steps.
