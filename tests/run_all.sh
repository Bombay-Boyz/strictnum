#!/bin/sh
# Local verification driver. CI requires 32-bit and sanitizer checks; fuzzing has a dedicated job.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD=$(mktemp -d "${TMPDIR:-/tmp}/strictnum-tests.XXXXXX")
cleanup() { rm -rf "$BUILD"; }
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
CC=${CC:-cc}
CXX=${CXX:-c++}
FUZZ_RUNS=${FUZZ_RUNS:-100000}

run() { echo "+ $*"; "$@"; }
echo "== Project: strictnum / strictnum.h =="
echo "== Host: $(uname -s 2>/dev/null || echo unknown) / $(uname -m 2>/dev/null || echo unknown) =="

echo '== Strict C99 differential/property suite =='
run "$CC" -std=c99 -O2 -Wall -Wextra -Wpedantic -Werror -I"$ROOT" "$ROOT/tests/test_strictnum.c" -o "$BUILD/test_strictnum"
run "$BUILD/test_strictnum"

cat > "$BUILD/analyzer_probe.c" <<'C'
int main(void) { return 0; }
C
if "$CC" -std=c99 -fanalyzer -Werror "$BUILD/analyzer_probe.c" -o "$BUILD/analyzer_probe" >/dev/null 2>&1; then
    echo '== GCC -fanalyzer =='
    run "$CC" -std=c99 -O1 -Wall -Wextra -Wpedantic -Werror -fanalyzer -I"$ROOT" "$ROOT/tests/test_strictnum.c" -o "$BUILD/test_gcc_analyzer"
    run "$BUILD/test_gcc_analyzer"
else
    echo 'SKIP: compiler does not provide GCC -fanalyzer'
fi

echo '== C99 differential suite with optional 64-bit API disabled =='
run "$CC" -std=c99 -O2 -Wall -Wextra -Wpedantic -Werror -DSTRICTNUM_NO_64 -I"$ROOT" "$ROOT/tests/test_strictnum.c" -o "$BUILD/test_c99_no64"
run "$BUILD/test_c99_no64"
run "$CC" -std=c99 -O2 -Wall -Wextra -Wpedantic -Werror -DSTRICTNUM_NO_STDINT -DSTRICTNUM_NO_64 -I"$ROOT" "$ROOT/tests/test_strictnum.c" -o "$BUILD/test_c99_no_stdint_no64"
run "$BUILD/test_c99_no_stdint_no64"

echo '== C89 fallback, 32-bit APIs only =='
run "$CC" -std=c89 -pedantic-errors -Wall -Wextra -Werror -DSTRICTNUM_NO_STDINT -DSTRICTNUM_NO_64 -I"$ROOT" "$ROOT/tests/test_portable.c" -o "$BUILD/test_c89_no64"
run "$BUILD/test_c89_no64"
echo '== Forced non-ASCII letter-table fallback =='
run "$CC" -std=c99 -O2 -Wall -Wextra -Wpedantic -Werror -DSTRICTNUM_TEST_FORCE_NONASCII_DIGITS -I"$ROOT" "$ROOT/tests/test_portable.c" -o "$BUILD/test_digit_fallback"
run "$BUILD/test_digit_fallback"

echo '== C89 compiler-extension path with long long =='
run "$CC" -std=c89 -Wall -Wextra -Wpedantic -Werror -Wno-long-long -DSTRICTNUM_NO_STDINT -DSTRICTNUM_USE_LONG_LONG -I"$ROOT" "$ROOT/tests/test_portable.c" -o "$BUILD/test_c89_ll"
run "$BUILD/test_c89_ll"

echo '== C89 native-long 64-bit fallback (when long is exactly 64-bit) =='
cat > "$BUILD/probe_long64.c" <<'C'
#include "strictnum.h"
#ifndef SN_HAVE_64
#error native long 64-bit fallback unavailable
#endif
int main(void) { return 0; }
C
if "$CC" -std=c89 -pedantic-errors -DSTRICTNUM_NO_STDINT -I"$ROOT" "$BUILD/probe_long64.c" -o "$BUILD/probe_long64" >/dev/null 2>&1; then
    run "$CC" -std=c89 -pedantic-errors -Wall -Wextra -Werror -DSTRICTNUM_NO_STDINT -I"$ROOT" "$ROOT/tests/test_portable.c" -o "$BUILD/test_c89_long64"
    run "$BUILD/test_c89_long64"
else
    echo 'SKIP: target does not provide a verified 64-bit long type'
fi

echo '== C11, C17 and C23 header smoke tests =='
cat > "$BUILD/probe.c" <<'C'
int main(void) { return 0; }
C
cat > "$BUILD/smoke.c" <<'C'
#include "strictnum.h"
int main(void) { sn_i32 v = 0; return sn_parse_i32_z("42", &v) == SN_OK && v == 42 ? 0 : 1; }
C
for standard in c11 c17; do
    run "$CC" "-std=$standard" -Wall -Wextra -Wpedantic -Werror -I"$ROOT" "$BUILD/smoke.c" -o "$BUILD/$standard"
    run "$BUILD/$standard"
done
if "$CC" -std=c23 "$BUILD/probe.c" -o "$BUILD/probe_c23" >/dev/null 2>&1; then
    echo '  TEST: finalized C23 (-std=c23)'
    run "$CC" -std=c23 -Wall -Wextra -Wpedantic -Werror -I"$ROOT" "$BUILD/smoke.c" -o "$BUILD/c23"
    run "$BUILD/c23"
elif "$CC" -std=c2x "$BUILD/probe.c" -o "$BUILD/probe_c2x" >/dev/null 2>&1; then
    echo '  TEST: C23 working draft (-std=c2x); this is not a finalized C23 mode'
    run "$CC" -std=c2x -Wall -Wextra -Wpedantic -Werror -I"$ROOT" "$BUILD/smoke.c" -o "$BUILD/c2x"
    run "$BUILD/c2x"
else
    echo 'SKIP: neither -std=c23 nor -std=c2x is supported by this compiler'
fi

echo '== C++98/11/14/17/20/23 full portable suite =='
cat > "$BUILD/smoke.cpp" <<'CPP'
#include "strictnum.h"
int main() { sn_i32 v = 0; return sn_parse_i32_z("42", &v) == SN_OK && v == 42 ? 0 : 1; }
CPP
for standard in c++98 c++11 c++14 c++17; do
    run "$CXX" "-std=$standard" -O2 -Wall -Wextra -Werror -I"$ROOT" -x c++ "$ROOT/tests/test_portable.c" -o "$BUILD/portable_$standard"
    run "$BUILD/portable_$standard"
done
echo '== C++ reserved-identifier diagnostics =='
if command -v clang++ >/dev/null 2>&1; then
    run clang++ -std=c++98 -Wall -Wextra -Wreserved-identifier -Werror -I"$ROOT" -x c++ "$ROOT/tests/test_header_namespace.c" -o "$BUILD/cxx_reserved"
    run "$BUILD/cxx_reserved"
else
    echo 'SKIP: clang++ unavailable; reserved-identifier diagnostic not run'
fi

echo '== Header namespace and multi-translation-unit integration =='
run "$CC" -std=c89 -pedantic-errors -Wall -Wextra -Werror -I"$ROOT" "$ROOT/tests/test_header_namespace.c" -o "$BUILD/header_namespace_c"
run "$BUILD/header_namespace_c"
run "$CC" -std=c99 -Wall -Wextra -Werror -DSTRICTNUM_NO_64 -I"$ROOT" "$ROOT/tests/test_header_namespace.c" -o "$BUILD/header_namespace_no64"
run "$BUILD/header_namespace_no64"
echo '== C++ public-header strict cast warnings =='
run "$CXX" -std=c++98 -Wall -Wextra -Wold-style-cast -Werror -I"$ROOT" -x c++ "$ROOT/tests/test_header_namespace.c" -o "$BUILD/cxx_old_cast"
run "$BUILD/cxx_old_cast"
if command -v clang >/dev/null 2>&1; then
    echo '== Clang unused-attribute regression =='
    run clang -std=c99 -Wall -Wextra -Wused-but-marked-unused -Werror -I"$ROOT" "$ROOT/tests/test_header_namespace.c" -o "$BUILD/clang_unused_attribute"
    run "$BUILD/clang_unused_attribute"
fi
run "$CC" -std=c99 -Wall -Wextra -Werror -I"$ROOT" "$ROOT/tests/multi_tu/first.c" "$ROOT/tests/multi_tu/second.c" "$ROOT/tests/multi_tu/main.c" -o "$BUILD/multi_tu_c"
run "$BUILD/multi_tu_c"
run "$CXX" -std=c++98 -Wall -Wextra -Werror -I"$ROOT" -x c++ "$ROOT/tests/multi_tu/first.c" "$ROOT/tests/multi_tu/second.c" "$ROOT/tests/multi_tu/main.c" -o "$BUILD/multi_tu_cpp"
run "$BUILD/multi_tu_cpp"

echo '== C++98 fallback configurations =='
run "$CXX" -std=c++98 -pedantic-errors -Wall -Wextra -Werror -DSTRICTNUM_NO_STDINT -DSTRICTNUM_NO_64 -I"$ROOT" -x c++ "$ROOT/tests/test_portable.c" -o "$BUILD/portable_cpp98_no64"
run "$BUILD/portable_cpp98_no64"
run "$CXX" -std=c++98 -Wall -Wextra -Wpedantic -Werror -Wno-long-long -DSTRICTNUM_NO_STDINT -DSTRICTNUM_USE_LONG_LONG -I"$ROOT" -x c++ "$ROOT/tests/test_portable.c" -o "$BUILD/portable_cpp98_ll"
run "$BUILD/portable_cpp98_ll"
echo '== C++98 native-long 64-bit fallback (when long is exactly 64-bit) =='
cat > "$BUILD/probe_cpp98_long64.cpp" <<'CPP'
#include "strictnum.h"
#ifndef SN_HAVE_64
#error native long 64-bit fallback unavailable
#endif
int main() { return 0; }
CPP
if "$CXX" -std=c++98 -pedantic-errors -DSTRICTNUM_NO_STDINT -I"$ROOT" "$BUILD/probe_cpp98_long64.cpp" -o "$BUILD/probe_cpp98_long64" >/dev/null 2>&1; then
    run "$BUILD/probe_cpp98_long64"
    run "$CXX" -std=c++98 -pedantic-errors -Wall -Wextra -Werror -DSTRICTNUM_NO_STDINT -I"$ROOT" -x c++ "$ROOT/tests/test_portable.c" -o "$BUILD/portable_cpp98_long64"
    run "$BUILD/portable_cpp98_long64"
else
    echo 'SKIP: target does not provide a verified 64-bit long type in C++98 mode'
fi
for pair in 'c++20 c++2a' 'c++23 c++2b'; do
    set -- $pair
    final=$1
    draft=$2
    printf 'int main() { return 0; }\n' > "$BUILD/probe.cpp"
    if "$CXX" "-std=$final" "$BUILD/probe.cpp" -o "$BUILD/probe_$final" >/dev/null 2>&1; then
        echo "  TEST: full portable suite ($final)"
        run "$CXX" "-std=$final" -O2 -Wall -Wextra -Werror -I"$ROOT" -x c++ "$ROOT/tests/test_portable.c" -o "$BUILD/portable_$final"
        run "$BUILD/portable_$final"
    elif "$CXX" "-std=$draft" "$BUILD/probe.cpp" -o "$BUILD/probe_$draft" >/dev/null 2>&1; then
        echo "  TEST: full portable suite ($draft working draft; final mode unavailable)"
        run "$CXX" "-std=$draft" -O2 -Wall -Wextra -Werror -I"$ROOT" -x c++ "$ROOT/tests/test_portable.c" -o "$BUILD/portable_$draft"
        run "$BUILD/portable_$draft"
    elif [ "$final" = c++20 ]; then
        echo 'SKIP: C++20 and its draft mode unavailable locally; CI must verify on a current compiler'
    else
        echo 'SKIP: C++23 and its draft mode unavailable locally; CI must verify on a current compiler'
    fi
done

echo '== Documentation examples =='
run sh "$ROOT/tests/check_docs.sh"
echo '== Release metadata consistency =='
run sh "$ROOT/tests/check_release_metadata.sh"

echo '== Example compile and runtime tests =='
run sh "$ROOT/tests/test_examples.sh"

if command -v cmake >/dev/null 2>&1; then
    echo '== CMake install/export consumer test =='
    run cmake -S "$ROOT" -B "$BUILD/cmake-build" -DCMAKE_INSTALL_PREFIX="$BUILD/cmake-install"
    run cmake --build "$BUILD/cmake-build" --parallel
    run ctest --test-dir "$BUILD/cmake-build" --output-on-failure
    run cmake --install "$BUILD/cmake-build"
    pc_file=$(find "$BUILD/cmake-install" -name strictnum.pc -print -quit)
    if [ -z "$pc_file" ]; then
        echo 'FAIL: strictnum.pc was not installed' >&2
        exit 1
    fi
    if [ "$(find "$BUILD/cmake-install" -name strictnum.h | wc -l | tr -d ' ')" != 1 ]; then
        echo 'FAIL: strictnum.h must be installed exactly once' >&2
        exit 1
    fi
    if command -v pkg-config >/dev/null 2>&1; then
        run env PKG_CONFIG_PATH="$(dirname "$pc_file")" pkg-config --exists strictnum
        pc_cflags=$(PKG_CONFIG_PATH="$(dirname "$pc_file")" pkg-config --cflags strictnum)
        # shellcheck disable=SC2086
        run "$CC" -std=c99 -Wall -Wextra -Werror $pc_cflags -c "$ROOT/tests/cmake_consumer/main.c" -o "$BUILD/pc_consumer.o"
    fi
    version_file=$(find "$BUILD/cmake-install" -name strictnumConfigVersion.cmake -print -quit)
    if [ -z "$version_file" ] || grep -F 'CMAKE_SIZEOF_VOID_P' "$version_file" >/dev/null; then
        echo 'ERROR: installed header-only package is not architecture-independent' >&2
        exit 1
    fi
    run cmake -S "$ROOT/tests/cmake_consumer" -B "$BUILD/cmake-consumer" -DCMAKE_PREFIX_PATH="$BUILD/cmake-install"
    run cmake --build "$BUILD/cmake-consumer"
    run "$BUILD/cmake-consumer/strictnum_consumer"
    run cmake -S "$ROOT/tests/cmake_version_reject" -B "$BUILD/cmake-version-reject" -DCMAKE_PREFIX_PATH="$BUILD/cmake-install"
    run cmake -S "$ROOT/tests/cmake_subproject" -B "$BUILD/cmake-subproject"
    if ! grep -E '^STRICTNUM_INSTALL:BOOL=OFF$' "$BUILD/cmake-subproject/CMakeCache.txt" >/dev/null; then
        echo 'ERROR: strictnum installation is enabled by default as a subproject' >&2
        exit 1
    fi
    if ! grep -E '^STRICTNUM_BUILD_TESTS:BOOL=OFF$' "$BUILD/cmake-subproject/CMakeCache.txt" >/dev/null; then
        echo 'ERROR: strictnum tests are enabled by default as a subproject' >&2
        exit 1
    fi
    run cmake --build "$BUILD/cmake-subproject" --parallel
    run "$BUILD/cmake-subproject/subproject_consumer"
else
    echo 'SKIP: CMake is not installed; package integration smoke test unavailable'
fi

if command -v clang >/dev/null 2>&1; then
    cat > "$BUILD/sanitizer_probe.c" <<'C'
int main(void) { return 0; }
C
    if clang -std=c99 -fsanitize=address,undefined "$BUILD/sanitizer_probe.c" -o "$BUILD/sanitizer_probe" >/dev/null 2>&1 && "$BUILD/sanitizer_probe" >/dev/null 2>&1; then
        echo '== Clang C99 + ASan/UBSan =='
        run clang -std=c99 -O1 -g -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -I"$ROOT" "$ROOT/tests/test_strictnum.c" -o "$BUILD/test_clang_san"
        run "$BUILD/test_clang_san"
        # The test PRNG intentionally uses defined unsigned modulo arithmetic.
        # Keep signed overflow, invalid shifts, divide-by-zero and conversion
        # diagnostics enabled while suppressing those two intentional cases.
        run clang -std=c99 -O1 -g -Wall -Wextra -Wpedantic -Werror -fsanitize=undefined,integer,implicit-conversion -fno-sanitize=unsigned-integer-overflow,unsigned-shift-base -fno-sanitize-recover=all -I"$ROOT" "$ROOT/tests/test_strictnum.c" -o "$BUILD/test_clang_integer_san"
        run "$BUILD/test_clang_integer_san"
    elif [ "${REQUIRE_SANITIZERS:-0}" = 1 ]; then
        echo 'ERROR: Clang ASan/UBSan runtime unavailable (REQUIRE_SANITIZERS=1)' >&2
        exit 1
    else
        echo 'SKIP: Clang ASan/UBSan runtime unavailable in this environment'
    fi
    echo '== Clang static analyzer (core tests and examples) =='
    for source in "$ROOT/tests/test_header_namespace.c" "$ROOT/examples/kv.c" "$ROOT/examples/max_lines.c" "$ROOT/examples/port.c"; do
        name=${source##*/}
        name=${name%.c}
        run clang --analyze -std=c99 -Wall -Wextra -Wpedantic -Werror -I"$ROOT" -o "$BUILD/analyzer_$name.plist" "$source"
    done
    if [ "${RUN_FUZZER:-1}" = 0 ]; then
        echo 'SKIP: semantic fuzzing is run in the dedicated CI job (RUN_FUZZER=0)'
    else
    cat > "$BUILD/fuzzer_probe.c" <<'C'
#include <stddef.h>
#include <stdint.h>
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) { (void)data; (void)size; return 0; }
C
    if clang -std=c99 -fsanitize=fuzzer,address,undefined "$BUILD/fuzzer_probe.c" -o "$BUILD/fuzzer_probe" >/dev/null 2>&1; then
        run clang -std=c99 -O1 -g -Wall -Wextra -Wpedantic -Werror -fsanitize=fuzzer,address,undefined -I"$ROOT" "$ROOT/tests/fuzz_strictnum.c" -o "$BUILD/fuzz_strictnum"
        echo "== Semantic libFuzzer campaign ($FUZZ_RUNS executions; max input 256 bytes) =="
        mkdir -p "$BUILD/fuzz_corpus"
        if [ -d "$ROOT/tests/fuzz_corpus" ]; then
            cp -R "$ROOT/tests/fuzz_corpus/." "$BUILD/fuzz_corpus/"
        fi
        run "$BUILD/fuzz_strictnum" -verbosity=0 -max_len=256 -runs="$FUZZ_RUNS" -rss_limit_mb=512 -timeout=5 "$BUILD/fuzz_corpus"
    elif [ "${REQUIRE_FUZZER:-0}" = 1 ]; then
        echo 'ERROR: libFuzzer runtime unavailable (REQUIRE_FUZZER=1)' >&2
        exit 1
    else
        echo 'SKIP: this Clang build does not provide libFuzzer'
    fi
    fi
else
    if [ "${REQUIRE_FUZZER:-0}" = 1 ]; then
        echo 'ERROR: Clang is required for libFuzzer (REQUIRE_FUZZER=1)' >&2
        exit 1
    fi
    if [ "${REQUIRE_SANITIZERS:-0}" = 1 ]; then
        echo 'ERROR: Clang is required for ASan/UBSan (REQUIRE_SANITIZERS=1)' >&2
        exit 1
    fi
    echo 'SKIP: Clang not installed; sanitizer/fuzzer run unavailable'
fi

if "$CC" -m32 -std=c99 "$BUILD/probe.c" -o "$BUILD/probe_32" >/dev/null 2>&1 && "$BUILD/probe_32" >/dev/null 2>&1; then
    echo '== Native x86 32-bit execution =='
    run "$CC" -m32 -std=c99 -O2 -Wall -Wextra -Wpedantic -Werror -I"$ROOT" "$ROOT/tests/test_strictnum.c" -o "$BUILD/test_32"
    run "$BUILD/test_32"
    run "$CC" -m32 -std=c99 -O2 -Wall -Wextra -Wpedantic -Werror -DSTRICTNUM_NO_64 -I"$ROOT" "$ROOT/tests/test_strictnum.c" -o "$BUILD/test_32_no64"
    run "$BUILD/test_32_no64"
    run "$CC" -m32 -std=c99 -O2 -Wall -Wextra -Wpedantic -Werror -DSTRICTNUM_NO_STDINT -DSTRICTNUM_NO_64 -I"$ROOT" "$ROOT/tests/test_strictnum.c" -o "$BUILD/test_32_no_stdint_no64"
    run "$BUILD/test_32_no_stdint_no64"
elif [ "${REQUIRE_32BIT:-0}" = 1 ]; then
    echo 'ERROR: 32-bit compiler/runtime unavailable (REQUIRE_32BIT=1)' >&2
    exit 1
else
    echo 'SKIP: 32-bit toolchain/runtime unavailable; CI must verify this target'
fi

echo '== PASS: all available local checks completed =='
