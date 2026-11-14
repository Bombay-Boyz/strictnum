# Security policy

## Supported versions

Only the latest released version receives security fixes.

## Reporting a vulnerability

Please do not open a public issue for a suspected vulnerability. Use GitHub's
private vulnerability reporting ("Security" tab, "Report a vulnerability") on
this repository. Include the input, build configuration and compiler that
reproduce the problem. Expect an acknowledgement within a few days.

## Scope

In scope: incorrect status or value, out-of-bounds read or write, undefined
behaviour, or a violated documented contract in `strictnum.h`.
Out of scope: a caller passing a pointer that does not refer to `len` readable
bytes, which is a documented precondition.
