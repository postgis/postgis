/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Mikkel F. Jørgensen, dvide.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *
 * Fast printing of (u)int8/16/32/64_t, (u)int, (u)long.
 *
 * Functions take for the
 *
 *   int print_<type>(type value, char *buf);
 *
 * and returns number of characters printed, excluding trailing '\0'
 * which is also printed. Prints at most 21 characters including zero-
 * termination.
 *
 * The function `print_bool` is a bit different - it simply prints "true\0" for
 * non-zero integers, and "false\0" otherwise.
 *
 * The general algorithm is in-place formatting using binary search log10
 * followed by duff device loop unrolling div / 100 stages.
 *
 * The simpler post copy algorithm also provided for fmt_(u)int uses a
 * temp buffer and loops over div/100 and post copy to target buffer.
 *
 *
 * Benchmarks on core-i7, 2.2GHz, 64-bit clang/OS-X -O2:
 *
 * print_int64: avg 15ns for values between INT64_MIN + (10^7/2 .. 10^7/2)
 * print_int64: avg 11ns for values between 10^9 + (0..10,000,000).
 * print_int32: avg 7ns for values cast from INT64_MIN + (10^7/2 .. 10^7/2)
 * print_int32: avg 7ns for values between 10^9 + (0..10,000,000).
 * print_int64: avg 13ns for values between 10^16 + (0..10,000,000).
 * print_int64: avg 5ns for values between 0 and 10,000,000.
 * print_int32: avg 5ns for values between 0 and 10,000,000.
 * print_int16: avg 10ns for values cast from 0 and 10,000,000.
 * print_int8:  avg 4ns for values cast from 0 and 10,000,000.
 *
 * Post copy algorithm:
 * print_int: avg 12ns for values between INT64_MIN + (10^7/2 .. 10^7/2)
 * print_int: avg 14ns for values between 10^9 + (0..10,000,000).
 * print_long: avg 29ns for values between INT64_MIN + (10^7/2 .. 10^7/2)
 *
 * The post copy algorithm is nearly half as fast as the in-place
 * algorithm, but can also be faster occasionally - possibly because the
 * optimizer being able to skip the copy step.
 */

#ifndef PPRINTINT_H
#define PPRINTINT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UINT8_MAX
#include <stdint.h>
#endif

#include "pattributes.h" /* fallthrough */

#define PDIAGNOSTIC_IGNORE_UNUSED_FUNCTION
#include "pdiagnostic_push.h"

static int print_bool(int n, char *p);

static int print_uint8(uint8_t n, char *p);
static int print_uint16(uint16_t n, char *p);
static int print_uint32(uint32_t n, char *p);
static int print_uint64(uint64_t n, char *p);
static int print_int8(int8_t n, char *p);
static int print_int16(int16_t n, char *p);
static int print_int32(int32_t n, char *p);
static int print_int64(int64_t n, char *p);

/*
 * Uses slightly slower, but more compact alogrithm
 * that is not hardcoded to implementation size.
 * Other types may be defined using macros below.
 */
static int print_ulong(unsigned long n, char *p);
static int print_uint(unsigned int n, char *p);
static int print_int(int n, char *p);
static int print_long(long n, char *p);


#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#define __print_unaligned_copy_16(p, q) (*(uint16_t*)(p) = *(uint16_t*)(q))
#else
#define __print_unaligned_copy_16(p, q)                                     \
    ((((uint8_t*)(p))[0] = ((uint8_t*)(q))[0]),                             \
     (((uint8_t*)(p))[1] = ((uint8_t*)(q))[1]))
#endif

static const char __print_digit_pairs[] =
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";

#define __print_stage()                                                     \
        p -= 2;                                                             \
        dp = __print_digit_pairs + (n % 100) * 2;                           \
        n /= 100;                                                           \
        __print_unaligned_copy_16(p, dp);

#define __print_long_stage()                                                \
        __print_stage()                                                     \
        __print_stage()

#define __print_short_stage()                                               \
        *--p = (n % 10) + '0';                                              \
        n /= 10;

static int print_bool(int n, char *buf)
{
    if (n) {
        memcpy(buf, "true\0", 5);
        return 4;
    } else {
        memcpy(buf, "false\0", 6);
        return 5;
    }
}

static int print_uint8(uint8_t n, char *p)
{
    const char *dp;

    if (n >= 100) {
        p += 3;
        *p = '\0';
        __print_stage();
        p[-1] = (char)n + '0';
        return 3;
    }
    if (n >= 10) {
        p += 2;
        *p = '\0';
        __print_stage();
        return 2;
    }
    p[1] = '\0';
    p[0] = (char)n + '0';
    return 1;
}

static int print_uint16(uint16_t n, char *p)
{
    int k = 0;
    const char *dp;

    if (n >= 1000) {
        if(n >= 10000) {
            k = 5;
        } else {
            k = 4;
        }
    } else {
        if(n >= 100) {
            k = 3;
        } else if(n >= 10) {
            k = 2;
        } else {
            k = 1;
        }
    }
    p += k;
    *p = '\0';
    if (k & 1) {
        switch (k) {
        case 5:
            __print_stage();
	    pattribute(fallthrough);
        case 3:
            __print_stage();
	    pattribute(fallthrough);
        case 1:
            p[-1] = (char)n + '0';
        }
    } else {
        switch (k) {
        case 4:
            __print_stage();
	    pattribute(fallthrough);
        case 2:
            __print_stage();
        }
    }
    return k;
}

static int print_uint32(uint32_t n, char *p)
{
    int k = 0;
    const char *dp;

    if(n >= 10000UL) {
        if(n >= 10000000UL) {
            if(n >= 1000000000UL) {
                k = 10;
            } else if(n >= 100000000UL) {
                k = 9;
            } else {
               k = 8;
            }
        } else {
            if(n >= 1000000UL) {
                k = 7;
            } else if(n >= 100000UL) {
                k = 6;
            } else {
                k = 5;
            }
        }
    } else {
        if(n >= 100UL) {
            if(n >= 1000UL) {
                k = 4;
            } else {
                k = 3;
            }
        } else {
            if(n >= 10UL) {
                k = 2;
            } else {
                k = 1UL;
            }
        }
    }
    p += k;
    *p = '\0';
    if (k & 1) {
        switch (k) {
        case 9:
            __print_stage();
	    pattribute(fallthrough);
        case 7:
            __print_stage();
	    pattribute(fallthrough);
        case 5:
            __print_stage();
	    pattribute(fallthrough);
        case 3:
            __print_stage();
	    pattribute(fallthrough);
        case 1:
            p[-1] = (char)n + '0';
        }
    } else {
        switch (k) {
        case 10:
            __print_stage();
	    pattribute(fallthrough);
        case 8:
            __print_stage();
	    pattribute(fallthrough);
        case 6:
            __print_stage();
	    pattribute(fallthrough);
        case 4:
            __print_stage();
	    pattribute(fallthrough);
        case 2:
            __print_stage();
        }
    }
    return k;
}

static int print_uint64(uint64_t n, char *p)
{
    int k = 0;
    const char *dp;
    const uint64_t x = 1000000000ULL;

    if (n < x) {
        return print_uint32((uint32_t)n, p);
    }
    if(n >= 10000ULL * x) {
        if(n >= 10000000ULL * x) {
            if(n >= 1000000000ULL * x) {
                if (n >= 10000000000ULL * x) {
                    k = 11 + 9;
                } else {
                    k = 10 + 9;
                }
            } else if(n >= 100000000ULL * x) {
                k = 9 + 9;
            } else {
               k = 8 + 9;
            }
        } else {
            if(n >= 1000000ULL * x) {
                k = 7 + 9;
            } else if(n >= 100000ULL * x) {
                k = 6 + 9;
            } else {
                k = 5 + 9;
            }
        }
    } else {
        if(n >= 100ULL * x) {
            if(n >= 1000ULL * x) {
                k = 4 + 9;
            } else {
                k = 3 + 9;
            }
        } else {
            if(n >= 10ULL * x) {
                k = 2 + 9;
            } else {
                k = 1 + 9;
            }
        }
    }
    p += k;
    *p = '\0';
    if (k & 1) {
        switch (k) {
        case 19:
            __print_stage();
	    pattribute(fallthrough);
        case 17:
            __print_stage();
	    pattribute(fallthrough);
        case 15:
            __print_stage();
	    pattribute(fallthrough);
        case 13:
            __print_stage();
	    pattribute(fallthrough);
        case 11:
            __print_stage()
            __print_short_stage();
        }
    } else {
        switch (k) {
        case 20:
            __print_stage();
	    pattribute(fallthrough);
        case 18:
            __print_stage();
	    pattribute(fallthrough);
        case 16:
            __print_stage();
	    pattribute(fallthrough);
        case 14:
            __print_stage();
	    pattribute(fallthrough);
        case 12:
            __print_stage();
	    pattribute(fallthrough);
        case 10:
            __print_stage();
        }
    }
    __print_long_stage()
    __print_long_stage()
    return k;
}

static int print_int8(int8_t n, char *p)
{
    int sign;

    if ((sign = n < 0)) {
        *p++ = '-';
        n = -n;
    }
    return print_uint8((uint8_t)n, p) + sign;
}

static int print_int16(int16_t n, char *p)
{
    int sign;

    if ((sign = n < 0)) {
        *p++ = '-';
        n = -n;
    }
    return print_uint16((uint16_t)n, p) + sign;
}

static int print_int32(int32_t n, char *p)
{
    int sign;

    if ((sign = n < 0)) {
        *p++ = '-';
        n = -n;
    }
    return print_uint32((uint32_t)n, p) + sign;
}

static int print_int64(int64_t n, char *p)
{
    int sign;

    if ((sign = n < 0)) {
        *p++ = '-';
        n = -n;
    }
    return print_uint64((uint64_t)n, p) + sign;
}

#define __define_print_int_simple(NAME, UNAME, T, UT)                       \
static int UNAME(UT n, char *buf)                                           \
{                                                                           \
    char tmp[20];                                                           \
    char* p = tmp + 20;                                                     \
    char* q = p;                                                            \
    unsigned int k, m;                                                      \
                                                                            \
    while (n >= 100) {                                                      \
        p -= 2;                                                             \
        m = (unsigned int)(n % 100) * 2;                                    \
        n /= 100;                                                           \
        __print_unaligned_copy_16(p, __print_digit_pairs + m);              \
    }                                                                       \
    p -= 2;                                                                 \
    m = (unsigned int)n * 2;                                                \
    __print_unaligned_copy_16(p, __print_digit_pairs + m);                  \
    if (n < 10) {                                                           \
        ++p;                                                                \
    }                                                                       \
    k = (unsigned int)(q - p);                                              \
    while (p != q) {                                                        \
        *buf++ = *p++;                                                      \
    }                                                                       \
    *buf = '\0';                                                            \
    return (int)k;                                                          \
}                                                                           \
                                                                            \
static int NAME(T n, char *buf)                                             \
{                                                                           \
    int sign = n < 0;                                                       \
                                                                            \
    if (sign) {                                                             \
        *buf++ = '-';                                                       \
        n = -n;                                                             \
    }                                                                       \
    return UNAME((UT)n, buf) + sign;                                        \
}

__define_print_int_simple(print_int, print_uint, int, unsigned int)
__define_print_int_simple(print_long, print_ulong, long, unsigned long)

#ifdef PPRINTINT_BENCH
int main() {
    int64_t count = 10000000; /* 10^7 */
#if 0
    int64_t base = 0;
    int64_t base = 10000000000000000; /* 10^16 */
    int64_t base = 1000000000; /* 10^9 */
#endif
    int64_t base = INT64_MIN - count/2;
    char buf[100];
    int i, k = 0, n = 0;
    for (i = 0; i < count; i++) {
        k = print_int64(i + base, buf);
        n += buf[0] + buf[k - 1];
    }
    return n;
}
/* Call with time on executable, multiply time in seconds by 100 to get time unit in ns/number. */
#endif /* PPRINTINT_BENCH */

#ifdef PPRINTINT_TEST

#include <stdio.h>
#include <string.h>

int main()
{
    char buf[21];
    int failed = 0;
    int k;

    k = print_uint64(UINT64_MAX, buf);
    if (strlen(buf) != k) printf("length error\n");
    if (strcmp("18446744073709551615", buf)) {
        printf("UINT64_MAX didn't print correctly, got:\n'%s'\n", buf);
        ++failed;
    }
    k = print_int64(INT64_MAX, buf);
    if (strlen(buf) != k) printf("length error\n");
    if (strcmp("9223372036854775807", buf)) {
        printf("INT64_MAX didn't print correctly, got:\n'%s'\n", buf);
        ++failed;
    }
    k = print_int64(INT64_MIN, buf);
    if (strlen(buf) != k) printf("length error\n");
    if (strcmp("-9223372036854775808", buf)) {
        printf("INT64_MIN didn't print correctly, got:\n'%s'\n", buf);
        ++failed;
    }
    k = print_uint32(UINT32_MAX, buf);
    if (strlen(buf) != k) printf("length error\n");
    if (strcmp("4294967295", buf)) {
        printf("UINT32_MAX didn't print correctly, got:\n'%s'\n", buf);
        ++failed;
    }
    k = print_int32(INT32_MAX, buf);
    if (strlen(buf) != k) printf("length error\n");
    if (strcmp("2147483647", buf)) {
        printf("INT32_MAX didn't print correctly, got:\n'%s'\n", buf);
        ++failed;
    }
    k = print_int32(INT32_MIN, buf);
    if (strlen(buf) != k) printf("length error\n");
    if (strcmp("-2147483648", buf)) {
        printf("INT32_MIN didn't print correctly, got:\n'%s'\n", buf);
        ++failed;
    }
    k = print_uint16(UINT16_MAX, buf);
    if (strlen(buf) != k) printf("length error\n");
    if (strcmp("65535", buf)) {
        printf("UINT16_MAX didn't print correctly, got:\n'%s'\n", buf);
        ++failed;
    }
    k = print_int16(INT16_MAX, buf);
    if (strlen(buf) != k) printf("length error\n");
    if (strcmp("32767", buf)) {
        printf("INT16_MAX didn't print correctly, got:\n'%s'\n", buf);
        ++failed;
    }
    k = print_int16(INT16_MIN, buf);
    if (strlen(buf) != k) printf("length error\n");
    if (strcmp("-32768", buf)) {
        printf("INT16_MIN didn't print correctly, got:\n'%s'\n", buf);
        ++failed;
    }
    k = print_uint8(UINT8_MAX, buf);
    if (strlen(buf) != k) printf("length error\n");
    if (strcmp("255", buf)) {
        printf("INT8_MAX didn't print correctly, got:\n'%s'\n", buf);
        ++failed;
    }
    k = print_int8(INT8_MAX, buf);
    if (strlen(buf) != k) printf("length error\n");
    if (strcmp("127", buf)) {
        printf("INT8_MAX didn't print correctly, got:\n'%s'\n", buf);
        ++failed;
    }
    k = print_int8(INT8_MIN, buf);
    if (strlen(buf) != k) printf("length error\n");
    if (strcmp("-128", buf)) {
        printf("INT8_MIN didn't print correctly, got:\n'%s'\n", buf);
        ++failed;
    }
    k = print_int(INT32_MAX, buf);
    if (strlen(buf) != k) printf("length error\n");
    if (strcmp("2147483647", buf)) {
        printf("INT32_MAX didn't print correctly with k = print_int, got:\n'%s'\n", buf);
        ++failed;
    }
    k = print_int(INT32_MIN, buf);
    if (strlen(buf) != k) printf("length error\n");
    if (strcmp("-2147483648", buf)) {
        printf("INT32_MIN didn't print correctly k = print_int, got:\n'%s'\n", buf);
        ++failed;
    }
    k = print_long(INT32_MAX, buf);
    if (strlen(buf) != k) printf("length error\n");
    if (strcmp("2147483647", buf)) {
        printf("INT32_MAX didn't print correctly with fmt_long, got:\n'%s'\n", buf);
        ++failed;
    }
    k = print_long(INT32_MIN, buf);
    if (strlen(buf) != k) printf("length error\n");
    if (strcmp("-2147483648", buf)) {
        printf("INT32_MIN didn't print correctly fmt_long, got:\n'%s'\n", buf);
        ++failed;
    }
    k = print_bool(1, buf);
    if (strlen(buf) != k) printf("length error\n");
    if (strcmp("true", buf) {
        printf("1 didn't print 'true' as expected, got:\n'%s'\n", buf);
        ++failed;
    }
    k = print_bool(-1, buf);
    if (strlen(buf) != k) printf("length error\n");
    if (strcmp("true", buf) {
        printf("-1 didn't print 'true' as expected, got:\n'%s'\n", buf);
        ++failed;
    }
    k = print_bool(, buf);
    if (strlen(buf) != k) printf("length error\n");
    if (strcmp("false", buf) {
        printf("0 didn't print 'false' as expected, got:\n'%s'\n", buf);
        ++failed;
    }
    if (failed) {
        printf("FAILED\n");
        return -1;
    }
    printf("SUCCESS\n");
    return 0;
}
#endif /* PPRINTINT_TEST */

#include "pdiagnostic_pop.h"

#ifdef __cplusplus
}
#endif

#endif /* PPRINTINT_H */
