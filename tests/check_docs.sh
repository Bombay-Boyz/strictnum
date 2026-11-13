#!/bin/sh
# Extracts every ```c and ```cpp fenced block and compiles each as a
# translation unit. Complete shipped examples are linked and run separately
# by tests/test_examples.sh; snippets are not assumed to be standalone apps.
#
#   sh tests/check_docs.sh            (run from the repository root)
set -u
root=$(cd "$(dirname "$0")/.." && pwd)
tmp=$(mktemp -d "${TMPDIR:-/tmp}/strictnum-docs.XXXXXX")
cleanup() { rm -rf "$tmp"; }
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

CC=${CC:-cc}
CXX=${CXX:-c++}
fail=0
count=0

for doc in "$root/README.md" "$root/TUTORIAL.md"; do
    [ -f "$doc" ] || continue
    base=$(basename "$doc" .md)
    # Split into one file per fenced block: <base>_<n>.c or .cpp
    awk -v dir="$tmp" -v base="$base" '
        /^```c$/   { inb = 1; ext = "c";   n++; out = sprintf("%s/%s_%03d.%s", dir, base, n, ext); next }
        /^```cpp$/ { inb = 1; ext = "cpp"; n++; out = sprintf("%s/%s_%03d.%s", dir, base, n, ext); next }
        /^```/     { inb = 0; next }
        inb        { print > out }
    ' "$doc"
done

for f in "$tmp"/*.c; do
    [ -f "$f" ] || continue
    count=$((count + 1))
    if ! $CC -std=c99 -pedantic -Wall -Wextra -Wno-unused-function -I"$root" -c "$f" -o "$f.o" 2>"$f.err"; then
        echo "FAIL: $(basename "$f")"; cat "$f.err"; fail=1
    elif [ -s "$f.err" ]; then
        echo "WARN: $(basename "$f")"; cat "$f.err"; fail=1
    fi
    # Also prove the same snippet is valid C89 (the header's stated floor).
    if ! $CC -std=c89 -pedantic-errors -Wall -Wextra -Wno-unused-function -I"$root" -c "$f" -o "$f.o89" 2>"$f.c89.err"; then
        echo "FAIL (C89 compatibility): $(basename "$f")"
        cat "$f.c89.err"
        fail=1
    elif [ -s "$f.c89.err" ]; then
        echo "WARN (C89 compatibility): $(basename "$f")"
        cat "$f.c89.err"
        fail=1
    fi
done
for f in "$tmp"/*.cpp; do
    [ -f "$f" ] || continue
    count=$((count + 1))
    if ! $CXX -std=c++98 -Wall -Wextra -Wno-unused-function -I"$root" -c "$f" -o "$f.cxx98.o" 2>"$f.cxx98.err"; then
        echo "FAIL (C++98): $(basename "$f")"; cat "$f.cxx98.err"; fail=1
    elif [ -s "$f.cxx98.err" ]; then
        echo "WARN (C++98): $(basename "$f")"; cat "$f.cxx98.err"; fail=1
    fi
    if ! $CXX -std=c++17 -Wall -Wextra -Wno-unused-function -I"$root" -c "$f" -o "$f.cxx17.o" 2>"$f.cxx17.err"; then
        echo "FAIL (C++17): $(basename "$f")"; cat "$f.cxx17.err"; fail=1
    elif [ -s "$f.cxx17.err" ]; then
        echo "WARN (C++17): $(basename "$f")"; cat "$f.cxx17.err"; fail=1
    fi
done

if [ "$fail" -eq 0 ]; then
    echo "docs: $count code blocks compiled cleanly"
else
    echo "docs: problems found"
fi
exit $fail
