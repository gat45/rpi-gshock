#pragma once
#include <stdio.h>
#include <string.h>

/* Minimal assertion framework: no dependency, no build system. */
extern unsigned g_checks;
extern unsigned g_failures;

#define CK(cond)                                                    \
    do {                                                            \
        g_checks++;                                                 \
        if(!(cond)) {                                               \
            g_failures++;                                           \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);   \
        }                                                           \
    } while(0)

#define CK_INT(actual, expected)                                                \
    do {                                                                        \
        long _a = (long)(actual), _e = (long)(expected);                        \
        g_checks++;                                                             \
        if(_a != _e) {                                                          \
            g_failures++;                                                       \
            printf("FAIL %s:%d  %s == %ld, expected %ld\n",                     \
                   __FILE__, __LINE__, #actual, _a, _e);                        \
        }                                                                       \
    } while(0)

#define CK_STR(actual, expected)                                                \
    do {                                                                        \
        const char* _a = (actual);                                              \
        const char* _e = (expected);                                            \
        g_checks++;                                                             \
        if(!_a || strcmp(_a, _e) != 0) {                                        \
            g_failures++;                                                       \
            printf("FAIL %s:%d  %s == \"%s\", expected \"%s\"\n",               \
                   __FILE__, __LINE__, #actual, _a ? _a : "(null)", _e);        \
        }                                                                       \
    } while(0)

#define CK_BYTES(actual, expected, len)                                         \
    do {                                                                        \
        g_checks++;                                                             \
        if(memcmp((actual), (expected), (len)) != 0) {                          \
            g_failures++;                                                       \
            printf("FAIL %s:%d  %s: %zu bytes differ\n",                        \
                   __FILE__, __LINE__, #actual, (size_t)(len));                 \
        }                                                                       \
    } while(0)

void test_drsp(void);
void test_decoders(void);
void test_export(void);
void test_live(void);
void test_regress(void);
