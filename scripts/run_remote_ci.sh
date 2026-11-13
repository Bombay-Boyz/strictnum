#!/bin/sh
# Push the committed branch and wait for its GitHub Actions cross-platform matrix.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
command -v git >/dev/null 2>&1 || { echo "git is required" >&2; exit 1; }
command -v gh >/dev/null 2>&1 || { echo "GitHub CLI (gh) is required" >&2; exit 1; }
gh auth status >/dev/null
git_root=$(git rev-parse --show-toplevel)
[ "$git_root" = "$ROOT" ] || { echo "The strictnum directory must be the Git repository root." >&2; exit 1; }
branch=$(git branch --show-current)
[ -n "$branch" ] || { echo "Detached HEAD: check out a branch first" >&2; exit 1; }
if [ -n "$(git status --porcelain)" ]; then
    echo "Commit all changes (including new files) before triggering remote CI." >&2
    exit 1
fi
commit=$(git rev-parse HEAD)
git push -u origin "$branch"
echo "Waiting for GitHub Actions push workflow for commit: $commit"
run_id=
i=0
while [ "$i" -lt 60 ]; do
    run_id=$(gh run list --workflow ci.yml --branch "$branch" --event push --limit 20 \
        --json databaseId,headSha \
        --jq ".[] | select(.headSha == \"$commit\") | .databaseId" 2>/dev/null | head -n 1 || true)
    [ -n "$run_id" ] && break
    i=$((i + 1))
    sleep 2
done
[ -n "$run_id" ] || { echo "No CI run appeared for commit $commit. Check that ci.yml is on the branch/default branch." >&2; exit 1; }
echo "Watching workflow run $run_id for commit $commit"
gh run watch "$run_id" --exit-status
