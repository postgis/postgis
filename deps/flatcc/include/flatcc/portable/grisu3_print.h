/*
 * Copyright (c) 2016 Mikkel F. Jørgensen, dvide.com
 * Copyright author of MathGeoLib (https://github.com/juj)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. http://www.apache.org/licenses/LICENSE-2.0
 */

/*
 * Extracted from MathGeoLib.
 *
 * mikkelfj:
 * - Fixed final output when printing single digit negative exponent to
 * have leading zero (important for JSON).
 * - Changed formatting to prefer 0.012 over 1.2-e-2.
 *
 * Large portions of the original grisu3.c file has been moved to
 * grisu3_math.h, the rest is placed here.
 *
 * See also comments in grisu3_math.h.
 *
 * MatGeoLib grisu3.c comment:
 *
 *     This file is part of an implementation of the "grisu3" double to string
 *     conversion algorithm described in the research paper
 *
 *     "Printing Floating-Point Numbers Quickly And Accurately with Integers"
 *     by Florian Loitsch, available at
 *     http://www.cs.tufts.edu/~nr/cs257/archive/florian-loitsch/printf.pdf
 */

#ifndef GRISU3_PRINT_H
#define GRISU3_PRINT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h> /* sprintf, only needed for fallback printing */
#include <assert.h> /* assert */

#include "grisu3_math.h"

/*
 * The lightweight "portable" C library recognizes grisu3 support if
 * included first.
 */
#define grisu3_print_double_is_defined 1

/*
 * Not sure we have an exact definition, but we get up to 23
 * emperically. There is some math ensuring it does not go awol though,
 * like 18 digits + exponent or so.
 * This max should be safe size buffer for printing, including zero term.
 */
#define GRISU3_PRINT_MAX 30

static int grisu3_round_weed(char *buffer, int len, uint64_t wp_W, uint64_t delta, uint64_t rest, uint64_t ten_kappa, uint64_t ulp)
{
    uint64_t wp_Wup = wp_W - ulp;
    uint64_t wp_Wdown = wp_W + ulp;
    while(rest < wp_Wup && delta - rest >= ten_kappa
        && (rest + ten_kappa < wp_Wup || wp_Wup - rest >= rest + ten_kappa - wp_Wup))
    {
        --buffer[len-1];
        rest += ten_kappa;
    }
    if (rest < wp_Wdown && delta - rest >= ten_kappa
        && (rest + ten_kappa < wp_Wdown || wp_Wdown - rest > rest + ten_kappa - wp_Wdown))
        return 0;

    return 2*ulp <= rest && rest <= delta - 4*ulp;
}

static int grisu3_digit_gen(grisu3_diy_fp_t low, grisu3_diy_fp_t w, grisu3_diy_fp_t high, char *buffer, int *length, int *kappa)
{
    uint64_t unit = 1;
    grisu3_diy_fp_t too_low = { low.f - unit, low.e };
    grisu3_diy_fp_t too_high = { high.f + unit, high.e };
    grisu3_diy_fp_t unsafe_interval =  grisu3_diy_fp_minus(too_high, too_low);
    grisu3_diy_fp_t one = { 1ULL << -w.e, w.e };
    uint32_t p1 = (uint32_t)(too_high.f >> -one.e);
    uint64_t p2 = too_high.f & (one.f - 1);
    uint32_t div;
    *kappa = grisu3_largest_pow10(p1, GRISU3_DIY_FP_FRACT_SIZE + one.e, &div);
    *length = 0;

    while(*kappa > 0)
    {
        uint64_t rest;
        char digit = (char)(p1 / div);
        buffer[*length] = '0' + digit;
        ++*length;
        p1 %= div;
        --*kappa;
        rest = ((uint64_t)p1 << -one.e) + p2;
        if (rest < unsafe_interval.f) return grisu3_round_weed(buffer, *length, grisu3_diy_fp_minus(too_high, w).f, unsafe_interval.f, rest, (uint64_t)div << -one.e, unit);
        div /= 10;
    }

    for(;;)
    {
        char digit;
        p2 *= 10;
        unit *= 10;
        unsafe_interval.f *= 10;
        /* Integer division by one. */
        digit = (char)(p2 >> -one.e);
        buffer[*length] = '0' + digit;
        ++*length;
        p2 &= one.f - 1; /* Modulo by one. */
        --*kappa;
        if (p2 < unsafe_interval.f) return grisu3_round_weed(buffer, *length, grisu3_diy_fp_minus(too_high, w).f * unit, unsafe_interval.f, p2, one.f, unit);
    }
}

static int grisu3(double v, char *buffer, int *length, int *d_exp)
{
    int mk, kappa, success;
    grisu3_diy_fp_t dfp = grisu3_cast_diy_fp_from_double(v);
    grisu3_diy_fp_t w = grisu3_diy_fp_normalize(dfp);

    /* normalize boundaries */
    grisu3_diy_fp_t t = { (dfp.f << 1) + 1, dfp.e - 1 };
    grisu3_diy_fp_t b_plus = grisu3_diy_fp_normalize(t);
    grisu3_diy_fp_t b_minus;
    grisu3_diy_fp_t c_mk; /* Cached power of ten: 10^-k */
    uint64_t u64 = grisu3_cast_uint64_from_double(v);
    assert(v > 0 && v <= 1.7976931348623157e308); /* Grisu only handles strictly positive finite numbers. */
    if (!(u64 & GRISU3_D64_FRACT_MASK) && (u64 & GRISU3_D64_EXP_MASK) != 0) { b_minus.f = (dfp.f << 2) - 1; b_minus.e =  dfp.e - 2;} /* lower boundary is closer? */
    else { b_minus.f = (dfp.f << 1) - 1; b_minus.e = dfp.e - 1; }
    b_minus.f = b_minus.f << (b_minus.e - b_plus.e);
    b_minus.e = b_plus.e;

    mk = grisu3_diy_fp_cached_pow(GRISU3_MIN_TARGET_EXP - GRISU3_DIY_FP_FRACT_SIZE - w.e, &c_mk);

    w = grisu3_diy_fp_multiply(w, c_mk);
    b_minus = grisu3_diy_fp_multiply(b_minus, c_mk);
    b_plus  = grisu3_diy_fp_multiply(b_plus,  c_mk);

    success = grisu3_digit_gen(b_minus, w, b_plus, buffer, length, &kappa);
    *d_exp = kappa - mk;
    return success;
}

static int grisu3_i_to_str(int val, char *str)
{
    int len, i;
    char *s;
    char *begin = str;
    if (val < 0) { *str++ = '-'; val = -val; }
    s = str;

    for(;;)
    {
        int ni = val / 10;
        int digit = val - ni*10;
        *s++ = (char)('0' + digit);
        if (ni == 0)
            break;
        val = ni;
    }
    *s = '\0';
    len = (int)(s - str);
    for(i = 0; i < len/2; ++i)
    {
        char ch = str[i];
        str[i] = str[len-1-i];
        str[len-1-i] = ch;
    }

    return (int)(s - begin);
}

static int grisu3_print_nan(uint64_t v, char *dst)
{
    static char hexdigits[16] = "0123456789ABCDEF";
    int i = 0;

    dst[0] = 'N';
    dst[1] = 'a';
    dst[2] = 'N';
    dst[3] = '(';
    dst[20] = ')';
    dst[21] = '\0';
    dst += 4;
    for (i = 15; i >= 0; --i) {
        dst[i] = hexdigits[v & 0x0F];
        v >>= 4;
    }
    return 21;
}

static int grisu3_print_double(double v, char *dst)
{
    int d_exp, len, success, decimals, i;
    uint64_t u64 = grisu3_cast_uint64_from_double(v);
    char *s2 = dst;
    assert(dst);

    /* Prehandle NaNs */
    if ((u64 << 1) > 0xFFE0000000000000ULL) return grisu3_print_nan(u64, dst);
    /* Prehandle negative values. */
    if ((u64 & GRISU3_D64_SIGN) != 0) { *s2++ = '-'; v = -v; u64 ^= GRISU3_D64_SIGN; }
    /* Prehandle zero. */
    if (!u64) { *s2++ = '0'; *s2 = '\0'; return (int)(s2 - dst); }
    /* Prehandle infinity. */
    if (u64 == GRISU3_D64_EXP_MASK) { *s2++ = 'i'; *s2++ = 'n'; *s2++ = 'f'; *s2 = '\0'; return (int)(s2 - dst); }

    success = grisu3(v, s2, &len, &d_exp);
    /* If grisu3 was not able to convert the number to a string, then use old sprintf (suboptimal). */
    if (!success) return sprintf(s2, "%.17g", v) + (int)(s2 - dst);

    /* We now have an integer string of form "151324135" and a base-10 exponent for that number. */
    /* Next, decide the best presentation for that string by whether to use a decimal point, or the scientific exponent notation 'e'. */
    /* We don't pick the absolute shortest representation, but pick a balance between readability and shortness, e.g. */
    /* 1.545056189557677e-308 could be represented in a shorter form */
    /* 1545056189557677e-323 but that would be somewhat unreadable. */
    decimals = GRISU3_MIN(-d_exp, GRISU3_MAX(1, len-1));

    /* mikkelfj:
     * fix zero prefix .1 => 0.1, important for JSON export.
     * prefer unscientific notation at same length:
     * -1.2345e-4 over -1.00012345,
     * -1.0012345 over -1.2345e-3
     */
    if (d_exp < 0 && (len + d_exp) > -3 && len <= -d_exp)
    {
        /* mikkelfj: fix zero prefix .1 => 0.1, and short exponents 1.3e-2 => 0.013. */
        memmove(s2 + 2 - d_exp - len, s2, (size_t)len);
        s2[0] = '0';
        s2[1] = '.';
        for (i = 2; i < 2-d_exp-len; ++i) s2[i] = '0';
        len += i;
    }
    else if (d_exp < 0 && len > 1) /* Add decimal point? */
    {
        for(i = 0; i < decimals; ++i) s2[len-i] = s2[len-i-1];
        s2[len++ - decimals] = '.';
        d_exp += decimals;
        /* Need scientific notation as well? */
        if (d_exp != 0) { s2[len++] = 'e'; len += grisu3_i_to_str(d_exp, s2+len); }
    }
    /* Add scientific notation? */
    else if (d_exp < 0 || d_exp > 2) { s2[len++] = 'e'; len += grisu3_i_to_str(d_exp, s2+len); }
    /* Add zeroes instead of scientific notation? */
    else if (d_exp > 0) { while(d_exp-- > 0) s2[len++] = '0'; }
    s2[len] = '\0'; /* grisu3 doesn't null terminate, so ensure termination. */
    return (int)(s2+len-dst);
}

#ifdef __cplusplus
}
#endif

#endif /* GRISU3_PRINT_H */
