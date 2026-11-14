#!/bin/sh
# Compile and exercise shipped examples, including hostile input boundaries.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CC=${CC:-cc}
BUILD=$(mktemp -d "${TMPDIR:-/tmp}/strictnum-examples.XXXXXX")
cleanup() { rm -rf "$BUILD"; }
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

"$CC" -std=c99 -O2 -Wall -Wextra -Wpedantic -Werror -I"$ROOT" \
    "$ROOT/examples/kv.c" -o "$BUILD/kv"
"$CC" -std=c99 -O2 -Wall -Wextra -Wpedantic -Werror -I"$ROOT" \
    "$ROOT/examples/port.c" -o "$BUILD/port"
"$CC" -std=c99 -O2 -Wall -Wextra -Wpedantic -Werror -I"$ROOT" \
    "$ROOT/examples/max_lines.c" -o "$BUILD/max_lines"
"$CC" -std=c89 -pedantic-errors -O2 -Wall -Wextra -Werror \
    -DSTRICTNUM_NO_STDINT -DSTRICTNUM_NO_64 -I"$ROOT" \
    "$ROOT/examples/max_lines.c" -o "$BUILD/max_lines_c89"

printf 'workers=8\ntimeout_ms=2500\nmask=ff\noffset=-42\n' | "$BUILD/kv" >"$BUILD/out"
grep -F 'workers    = 8' "$BUILD/out" >/dev/null
grep -F 'timeout_ms = 2500' "$BUILD/out" >/dev/null
grep -F 'mask       = 0xff' "$BUILD/out" >/dev/null
grep -F 'offset     = -42' "$BUILD/out" >/dev/null

# Embedded NUL must reject the whole record; the following line must survive.
if printf 'workers=8\000junk\nworkers=9\n' | "$BUILD/kv" >"$BUILD/nul.out" 2>"$BUILD/nul.err"; then
    echo 'FAIL: kv accepted an embedded NUL record' >&2
    exit 1
fi
grep -F 'embedded NUL byte' "$BUILD/nul.err" >/dev/null
grep -F 'workers    = 9' "$BUILD/nul.out" >/dev/null

# CRLF is accepted, but a bare CR is part of the value and must be rejected.
printf 'workers=8\r\n' | "$BUILD/kv" >"$BUILD/crlf.out"
grep -F 'workers    = 8' "$BUILD/crlf.out" >/dev/null
if printf 'workers=8\r' | "$BUILD/kv" >"$BUILD/bare-cr-eof.out" 2>"$BUILD/bare-cr-eof.err"; then
    echo 'FAIL: kv accepted a bare CR at EOF' >&2
    exit 1
fi
grep -F 'not a valid number' "$BUILD/bare-cr-eof.err" >/dev/null
if printf 'workers=8\r\nworkers=9\rX\n' | "$BUILD/kv" >"$BUILD/bare-cr-line.out" 2>"$BUILD/bare-cr-line.err"; then
    echo 'FAIL: kv accepted a bare CR in a line' >&2
    exit 1
fi
grep -F 'workers    = 8' "$BUILD/bare-cr-line.out" >/dev/null
grep -F 'not a valid number' "$BUILD/bare-cr-line.err" >/dev/null

# An overlong physical line must be drained, not split into fake records.
if {
    i=0
    while [ "$i" -lt 300 ]; do printf x; i=$((i + 1)); done
    printf '\nworkers=7\n'
} | "$BUILD/kv" >"$BUILD/long.out" 2>"$BUILD/long.err"; then
    echo 'FAIL: kv accepted an overlong record' >&2
    exit 1
fi
grep -F 'record exceeds' "$BUILD/long.err" >/dev/null
grep -F 'workers    = 7' "$BUILD/long.out" >/dev/null
"$BUILD/port" 8080 >"$BUILD/port.out"
grep -F 'port = 8080' "$BUILD/port.out" >/dev/null
if "$BUILD/port" 70000 >/dev/null 2>&1; then
    echo 'FAIL: port example accepted an out-of-range port' >&2
    exit 1
fi
if "$BUILD/port" 0 >/dev/null 2>&1; then
    echo 'FAIL: port example accepted port zero' >&2
    exit 1
fi

# Every "$ ./port ARG" transcript in TUTORIAL.md must match real output.
awk '/^\$ \.\/port /{arg=$3; next} arg!="" {print arg "\t" $0; arg=""}' \
    "$ROOT/TUTORIAL.md" >"$BUILD/port.expected"
[ -s "$BUILD/port.expected" ] || { echo 'FAIL: no port transcript in TUTORIAL.md' >&2; exit 1; }
while IFS=$(printf '\t') read -r arg want; do
    got=$("$BUILD/port" "$arg" 2>&1 || true)
    if [ "$got" != "$want" ]; then
        echo "FAIL: TUTORIAL transcript for './port $arg' is stale" >&2
        echo "  doc:    $want" >&2
        echo "  actual: $got" >&2
        exit 1
    fi
done <"$BUILD/port.expected"

# The max-lines example accepts LF/CRLF and a final line without LF.
printf '12\r\n-4\r\n31' | "$BUILD/max_lines" >"$BUILD/max.out"
grep -F 'maximum = 31' "$BUILD/max.out" >/dev/null

# An embedded NUL must not make strlen-like prefix acceptance possible.
if printf '42\000junk\n' | "$BUILD/max_lines" >"$BUILD/max-nul.out" 2>"$BUILD/max-nul.err"; then
    echo 'FAIL: max_lines accepted a record containing an embedded NUL' >&2
    exit 1
fi
[ ! -s "$BUILD/max-nul.out" ]
grep -F 'not a valid number' "$BUILD/max-nul.err" >/dev/null

# An overlong line is rejected as one record, never split into fake lines.
if {
    i=0
    while [ "$i" -lt 160 ]; do printf 7; i=$((i + 1)); done
    printf '\n'
} | "$BUILD/max_lines" >"$BUILD/max-long.out" 2>"$BUILD/max-long.err"; then
    echo 'FAIL: max_lines accepted an overlong line' >&2
    exit 1
fi
[ ! -s "$BUILD/max-long.out" ]
grep -F 'line exceeds' "$BUILD/max-long.err" >/dev/null

if : | "$BUILD/max_lines" >"$BUILD/max-empty.out" 2>"$BUILD/max-empty.err"; then
    echo 'FAIL: max_lines accepted an empty file' >&2
    exit 1
fi

echo 'examples: compile and runtime boundary tests passed'
