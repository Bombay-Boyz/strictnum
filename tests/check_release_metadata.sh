#!/bin/sh
# Check version and licence metadata that must remain consistent at release time.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

header_major=$(sed -n 's/^#define STRICTNUM_VERSION_MAJOR[[:space:]]*//p' "$ROOT/strictnum.h")
header_minor=$(sed -n 's/^#define STRICTNUM_VERSION_MINOR[[:space:]]*//p' "$ROOT/strictnum.h")
header_patch=$(sed -n 's/^#define STRICTNUM_VERSION_PATCH[[:space:]]*//p' "$ROOT/strictnum.h")
header_comment_version=$(sed -n 's/^ \* Version \([0-9.]*\)$/\1/p' "$ROOT/strictnum.h")
header_version="$header_major.$header_minor.$header_patch"

if [ "$header_version" != "$header_comment_version" ]; then
    echo "release metadata: version mismatch (header macros=$header_version, header comment=$header_comment_version)" >&2
    exit 1
fi
# CMake takes its version from the header; make sure nobody re-hard-codes it.
if ! grep -q 'STRICTNUM_VERSION_(MAJOR|MINOR|PATCH)' "$ROOT/CMakeLists.txt" ||
   grep -Eq '^project\(strictnum VERSION [0-9]' "$ROOT/CMakeLists.txt"; then
    echo 'release metadata: CMakeLists.txt must read its version from strictnum.h' >&2
    exit 1
fi

license_notice=$(sed -n 's/^\(Copyright (c) .*$\)/\1/p' "$ROOT/LICENSE" | head -n 1)
header_notice=$(sed -n 's/^ \* \(Copyright (c) .*$\)/\1/p' "$ROOT/strictnum.h" | head -n 1)
if [ -z "$license_notice" ] || [ "$license_notice" != "$header_notice" ]; then
    echo 'release metadata: LICENSE and strictnum.h copyright notices differ' >&2
    exit 1
fi

placeholder_hits=$(find "$ROOT" -type f \
    ! -path "$ROOT/.git/*" \
    ! -path "$ROOT/tests/fuzz_corpus/*" \
    ! -name check_release_metadata.sh \
    -exec grep -nIE 'YOUR NAME|Your Name|example\.invalid|@COPYRIGHT_HOLDER@|TODO|FIXME' {} + 2>/dev/null || true)
if [ -n "$placeholder_hits" ]; then
    printf '%s\n' "$placeholder_hits" >&2
    echo 'release metadata: placeholder or unfinished marker found' >&2
    exit 1
fi

echo "release metadata: version $header_version and copyright notices consistent"
