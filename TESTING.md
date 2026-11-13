# Testing and verification

Run the local test driver from any working directory:

```sh
sh tests/run_all.sh
```

The script uses `CC` and `CXX` when set, otherwise `cc` and `c++`. It creates
its build products in a temporary directory and removes them on exit.

## Local checks

The driver runs the C99 differential/property suite with and without the
optional 64-bit API, C89 fallback suites (including native 64-bit `long` when
available), C11/C17 and available C23 modes, the full portable suite in supported C++
standards, documentation compilation, example runtime tests, and available
Clang sanitizer/fuzzer checks. It also attempts native x86 32-bit execution.

Optional local facilities may be skipped when the compiler/runtime is absent.
To require them locally:

```sh
REQUIRE_FUZZER=1 sh tests/run_all.sh
REQUIRE_32BIT=1 sh tests/run_all.sh
REQUIRE_SANITIZERS=1 sh tests/run_all.sh
```

The runner probes toolchain/runtime availability with a minimal program first.
Once a capability is established, compilation or execution failures in the
actual project tests are failures, not skips. CI requires sanitizer execution
on the native Unix matrix.

A local skip is not evidence that a platform passed. The corresponding CI job
must run before claiming that target is verified.

## Documentation and example checks

```sh
sh tests/check_docs.sh
sh tests/test_examples.sh
```

The example tests compile `examples/port.c`, `examples/kv.c`, and
`examples/max_lines.c` (including the C89 no-64 fallback), then exercise
normal input, port-zero/range rejection, embedded NUL rejection, LF/CRLF and
bare-CR handling, empty input, and overlong physical-line draining.

## Semantic fuzzing

`tests/fuzz_strictnum.c` is a libFuzzer entry point. It compares the parser's
status and result against an independent shape validator plus `strtoll` /
`strtoull`, checks that output sentinels remain unchanged on failure, and
compares explicit-length and NUL-terminated calls when no embedded NUL exists.
The input is capped at 256 bytes and each input is checked in one or two
selected bases to keep the campaign bounded.

For a longer local campaign, use a temporary corpus directory so that
libFuzzer does not add generated inputs to the checked-in regression corpus:

```sh
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
mkdir -p "$tmp/corpus"
cp -R tests/fuzz_corpus/. "$tmp/corpus/"
clang -std=c99 -O1 -g -fsanitize=fuzzer,address,undefined \
  tests/fuzz_strictnum.c -o "$tmp/fuzz_strictnum"
"$tmp/fuzz_strictnum" -max_len=256 -max_total_time=3600 \
  -rss_limit_mb=512 -timeout=5 "$tmp/corpus"
```

Keep minimized crashing or mismatching inputs as regression corpus entries
and convert important discoveries into deterministic tests.


## CI coverage

`.github/workflows/ci.yml` defines the following CI jobs:

- Linux x86-64 and ARM64;
- macOS Intel and Apple Silicon;
- Linux x86 32-bit execution (ILP32);
- Windows MSVC x64 and x86;
- a mandatory Linux Clang/libFuzzer campaign.

**Verified CI result:** all eight jobs completed successfully on commit
`d0fd865a0b8b6ae2b91c48b7b74a682fb38769f1` on 13 November 2026.

The Windows job uses MSVC's supported C language mode; it does not claim
strict C89 compiler conformance.

The Unix native jobs require sanitizer execution. The Linux 32-bit job
requires actual 32-bit compilation and execution. The dedicated fuzzing
job requires the Clang libFuzzer runtime and executes a mandatory
semantic fuzzing campaign.

These results verify the listed configurations on the recorded commit.
They do not establish compatibility with every compiler, operating system,
architecture or C implementation.

Big-endian, EBCDIC and 8-/16-bit microcontroller targets are not currently
verified.


## Header and integration checks

The local runner also verifies that private implementation macros do not leak,
that the header can be included by multiple C and C++ translation units without
linker collisions, and (with Clang) that the header declares no
implementation-reserved identifiers. When CMake is installed, it installs the
package into a temporary prefix and builds a separate consumer using
`find_package(strictnum CONFIG REQUIRED)`.

The semantic fuzzer starts from the checked-in seeds in `tests/fuzz_corpus/`.
Fuzzing is still probabilistic: retain every minimized crash or semantic
mismatch as a deterministic regression test before changing the implementation.


## CTest and CMake integration

For a top-level checkout, CMake enables the `strictnum.core`,
`strictnum.portable`, and `strictnum.namespace` tests by default:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

When strictnum is added with `add_subdirectory()`, installation and its
standalone CTest suite default to OFF. A parent project can opt in explicitly
with `STRICTNUM_INSTALL=ON` or `STRICTNUM_BUILD_TESTS=ON`.

The shell runner remains the authoritative cross-standard and portability
suite. `tests/check_docs.sh` compiles each Markdown C/C++ snippet; complete
examples are separately linked and executed by `tests/test_examples.sh`.
CTest is an integration point, not a replacement for that matrix.
