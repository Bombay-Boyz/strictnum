
# Changelog

This project follows a concise, release-oriented changelog. Unreleased changes
are listed under `Unreleased`; versioned sections describe released versions.

## Unreleased

- Fixed the `TUTORIAL.md` port example so its code and transcript match
  `examples/port.c`; `tests/test_examples.sh` now checks the transcript.
- Added `SECURITY.md`, `CONTRIBUTING.md`, `.editorconfig` and Dependabot
  configuration for GitHub Actions.

## 1.0.0

- Initial release of the strict fixed-width integer parsing header.
- Added checked decimal and explicit-base parsing with transactional outputs.
- Replaced the overloaded `SN_NTS` length sentinel with explicit `_z` and
  `_base_z` NUL-terminated entry points; explicit-length functions always
  parse exactly the supplied byte count.
- Standardised repository and CMake project identity on `strictnum`.
- Removed consumer-macro rejection, corrected C++ cast diagnostics, and
  centralised public argument validation.
- Added C89/C99 and C++98-and-later compatibility paths where supported.
- Added CTest integration, subproject-safe CMake install defaults, and
  header-only CMake installation and package export.
- Added bounded input examples, multi-translation-unit checks, expanded
  argument and fallback regression coverage, and semantic fuzzing.
- Added portability, integration, testing, API documentation and release
  metadata checks.
