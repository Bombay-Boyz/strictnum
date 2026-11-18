#!/bin/sh
# Print the CHANGELOG.md section for one version.
#   sh scripts/release_notes.sh 1.0.0
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
version=${1:?usage: release_notes.sh VERSION}
notes=$(awk -v h="## $version" '
    $0 == h { on = 1; next }
    /^## / { on = 0 }
    on { print }
' "$ROOT/CHANGELOG.md")
if [ -z "$(printf '%s' "$notes" | tr -d '[:space:]')" ]; then
    echo "release_notes: no CHANGELOG.md section for version $version" >&2
    exit 1
fi
printf '%s\n' "$notes"
