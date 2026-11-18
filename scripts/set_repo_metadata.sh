#!/bin/sh
# Set the GitHub repository description, homepage topics and features.
# Needs the GitHub CLI, logged in as a user with admin rights on the repo:
#   gh auth login
#   sh scripts/set_repo_metadata.sh Bombay-Boyz/strictnum
set -eu
repo=${1:?usage: set_repo_metadata.sh OWNER/REPO}
gh repo edit "$repo" \
    --description "Strict, whole-input integer parsing for C89+ and C++98+: fixed-width types, one status code, header-only. A safer atoi/strtol." \
    --add-topic c \
    --add-topic header-only \
    --add-topic integer-parsing \
    --add-topic strtol-alternative \
    --add-topic c89 \
    --add-topic cpp \
    --add-topic single-header \
    --add-topic parsing
echo "updated $repo"
