/* Shared test helpers: counters and the CHECK macro. Include after <stdio.h>. */
#ifndef STRICTNUM_TEST_SUPPORT_H
#define STRICTNUM_TEST_SUPPORT_H

static unsigned long g_checks, g_fails;

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++g_checks;                                                          \
        if (!(cond)) {                                                       \
            ++g_fails;                                                       \
            if (g_fails <= 25)                                               \
                fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                    \
    } while (0)

#endif
