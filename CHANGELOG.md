# Changelog

This project follows a concise, release-oriented changelog. Unreleased changes
are listed under `Unreleased`; versioned sections describe published releases.

## Unreleased

- Replaced the overloaded `SN_NTS` length sentinel with explicit `_z` and
  `_base_z` NUL-terminated entry points; explicit-length functions now always
  parse exactly the supplied byte count.
- Standardised repository and CMake project identity on `strictnum`.
- Removed consumer-macro rejection, corrected C++ cast diagnostics, and
  centralised public argument validation.
- Added CTest integration, subproject-safe CMake install defaults, stricter
  CI runtime checks, and expanded argument/fallback regression coverage.
- Corrected documentation examples and release metadata checks.

## 1.0.0

- Initial release of the strict fixed-width integer parsing header.
- Added checked decimal and explicit-base parsing with transactional outputs.
- Added C89/C99 and C++98-and-later compatibility paths where supported.
- Added header-only CMake installation and package export.
- Added bounded input examples, multi-translation-unit checks and semantic fuzzing.
- Added portability, integration, testing and API documentation.
