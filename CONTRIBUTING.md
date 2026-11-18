# Contributing

1. Keep `strictnum.h` portable C89/C++98: no declarations after statements,
   no `//` comments, no non-ASCII source.
2. Every behaviour change needs a test in `tests/`.
3. Before opening a pull request run, from the repository root:

       sh tests/run_all.sh

   Optionally `ctest` after configuring with CMake.
4. Version, licence and docs consistency are checked by
   `tests/check_release_metadata.sh` and `tests/check_docs.sh`.
