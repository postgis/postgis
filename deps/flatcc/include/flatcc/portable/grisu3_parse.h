/*
 * Copyright (c) 2016 Mikkel F. Jørgensen, dvide.com
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
 * Port of parts of Google Double Conversion strtod functionality
 * but with fallback to strtod instead of a bignum implementation.
 *
 * Based on grisu3 math from MathGeoLib.
 *
 * See also grisu3_math.h comments.
 */

#ifndef GRISU3_PARSE_H
#define GRISU3_PARSE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UINT8_MAX
#include <stdint.h>
#endif

#include <stdlib.h>
#include <limits.h>

#include "grisu3_math.h"


/*
 * The maximum number characters a valid number may contain.  The parse
 * fails if the input length is longer but the character after max len
 * was part of the number.
 *
 * The length should not be set too high because it protects against
 * overflow in the exponent part derived from the input length.
 */
#define GRISU3_NUM_MAX_LEN 1000

/*
 * The lightweight "portable" C library recognizes grisu3 support if
 * included first.
 */
#define grisu3_parse_double_is_defined 1

/*
 * Disable to compare performance and to test diy_fp algorithm in
 * broader range.
 */
#define GRISU3_PARSE_FAST_CASE

/* May result in a one off error, otherwise when uncertain, fall back to strtod. */
//#define GRISU3_PARSE_ALLOW_ERROR


/*
 * The dec output exponent jumps in 8, so the result is offset at most
 * by 7 when the input is within range.
 */
static int grisu3_diy_fp_cached_dec_pow(int d_exp, grisu3_diy_fp_t *p)
{
    const int cached_offset = -GRISU3_MIN_CACHED_EXP;
    const int d_exp_dist = GRISU3_CACHED_EXP_STEP;
    int i, a_exp;

    GRISU3_ASSERT(GRISU3_MIN_CACHED_EXP <= d_exp);
    GRISU3_ASSERT(d_exp <  GRISU3_MAX_CACHED_EXP + d_exp_dist);

    i = (d_exp + cached_offset) / d_exp_dist;
    a_exp = grisu3_diy_fp_pow_cache[i].d_exp;
    p->f = grisu3_diy_fp_pow_cache[i].fract;
    p->e = grisu3_diy_fp_pow_cache[i].b_exp;

    GRISU3_ASSERT(a_exp <= d_exp);
    GRISU3_ASSERT(d_exp < a_exp + d_exp_dist);

    return a_exp;
}

/*
 * Ported from google double conversion strtod using
 * MathGeoLibs diy_fp functions for grisu3 in C.
 *
 * ulp_half_error is set if needed to trunacted non-zero trialing
 * characters.
 *
 * The actual value we need to encode is:
 *
 * (sign ? -1 : 1) * fraction * 2 ^ (exponent - fraction_exp)
 * where exponent is the base 10 exponent assuming the decimal point is
 * after the first digit. fraction_exp is the base 10 magnitude of the
 * fraction or number of significant digits - 1.
 *
 * If the exponent is between 0 and 22 and the fraction is encoded in
 * the lower 53 bits (the largest bit is implicit in a double, but not
 * in this fraction), then the value can be trivially converted to
 * double without loss of precision. If the fraction was in fact
 * multiplied by trailing zeroes that we didn't convert to exponent,
 * we there are larger values the 53 bits that can also be encoded
 * trivially - but then it is better to handle this during parsing
 * if it is worthwhile. We do not optimize for this here, because it
 * can be done in a simple check before calling, and because it might
 * not be worthwile to do at all since it cery likely will fail for
 * numbers printed to be convertible back to double without loss.
 *
 * Returns 0 if conversion was not exact. In that case the vale is
 * either one smaller than the correct one, or the correct one.
 *
 * Exponents must be range protected before calling otherwise cached
 * powers will blow up.
 *
 * Google Double Conversion seems to prefer the following notion:
 *
 * x >= 10^309 => +Inf
 * x <= 10^-324 => 0,
 *
 * max double: HUGE_VAL = 1.7976931348623157 * 10^308
 * min double: 4.9406564584124654 * 10^-324
 *
 * Values just below or above min/max representable number
 * may round towards large/small non-Inf/non-neg values.
 *
 * but `strtod` seems to return +/-HUGE_VAL on overflow?
 */
static int grisu3_diy_fp_encode_double(uint64_t fraction, int exponent, int fraction_exp, int ulp_half_error, double *result)
{
    /*
     * Error is measures in fractions of integers, so we scale up to get
     * some resolution to represent error expressions.
     */
    const int log2_error_one = 3;
    const int error_one = 1 << log2_error_one;
    const int denorm_exp = GRISU3_D64_DENORM_EXP;
    const uint64_t hidden_bit = GRISU3_D64_IMPLICIT_ONE;
    const int diy_size = GRISU3_DIY_FP_FRACT_SIZE;
    const int max_digits = 19;

    int error = ulp_half_error ? error_one / 2 : 0;
    int d_exp = (exponent - fraction_exp);
    int a_exp;
    int o_exp;
    grisu3_diy_fp_t v = { fraction, 0 };
    grisu3_diy_fp_t cp;
    grisu3_diy_fp_t rounded;
    int mag;
    int prec;
    int prec_bits;
    int half_way;

    /* When fractions in a double aren't stored with implicit msb fraction bit. */

    /* Shift fraction to msb. */
    v = grisu3_diy_fp_normalize(v);
    /* The half point error moves up while the exponent moves down. */
    error <<= -v.e;

    a_exp = grisu3_diy_fp_cached_dec_pow(d_exp, &cp);

    /* Interpolate between cached powers at distance 8. */
    if (a_exp != d_exp) {
        int adj_exp = d_exp - a_exp - 1;
        static grisu3_diy_fp_t cp_10_lut[] = {
            { 0xa000000000000000ULL, -60 },
            { 0xc800000000000000ULL, -57 },
            { 0xfa00000000000000ULL, -54 },
            { 0x9c40000000000000ULL, -50 },
            { 0xc350000000000000ULL, -47 },
            { 0xf424000000000000ULL, -44 },
            { 0x9896800000000000ULL, -40 },
        };
        GRISU3_ASSERT(adj_exp >= 0 && adj_exp < 7);
        v = grisu3_diy_fp_multiply(v, cp_10_lut[adj_exp]);

        /* 20 decimal digits won't always fit in 64 bit.
         * (`fraction_exp` is one less than significant decimal
         * digits in fraction, e.g. 1 * 10e0).
         * If we cannot fit, introduce 1/2 ulp error
         * (says double conversion reference impl.) */
        if (1 + fraction_exp + adj_exp > max_digits) {
            error += error_one / 2;
        }
    }

    v = grisu3_diy_fp_multiply(v, cp);
    /*
     * Google double conversion claims that:
     *
     *   The error introduced by a multiplication of a*b equals
     *     error_a + error_b + error_a*error_b/2^64 + 0.5
     *   Substituting a with 'input' and b with 'cached_power' we have
     *     error_b = 0.5  (all cached powers have an error of less than 0.5 ulp),
     *     error_ab = 0 or 1 / error_oner > error_a*error_b/ 2^64
     *
     * which in our encoding becomes:
     * error_a = error_one/2
     * error_ab = 1 / error_one (rounds up to 1 if error != 0, or 0 * otherwise)
     * fixed_error = error_one/2
     *
     * error += error_a + fixed_error + (error ? 1 : 0)
     *
     * (this isn't entirely clear, but that is as close as we get).
     */
    error += error_one + (error ? 1 : 0);

    o_exp = v.e;
    v = grisu3_diy_fp_normalize(v);
    /* Again, if we shift the significant bits, the error moves along. */
    error <<= o_exp - v.e;

    /*
     * The value `v` is bounded by 2^mag which is 64 + v.e. because we
     * just normalized it by shifting towards msb.
     */
    mag = diy_size + v.e;

    /* The effective magnitude of the IEEE double representation. */
    mag = mag >= diy_size + denorm_exp ? diy_size : mag <= denorm_exp ? 0 : mag - denorm_exp;
    prec = diy_size - mag;
    if (prec + log2_error_one >= diy_size) {
        int e_scale = prec + log2_error_one - diy_size - 1;
        v.f >>= e_scale;
        v.e += e_scale;
        error = (error >> e_scale) + 1 + error_one;
        prec -= e_scale;
    }
    rounded.f = v.f >> prec;
    rounded.e = v.e + prec;
    prec_bits = (int)(v.f & ((uint64_t)1 << (prec - 1))) * error_one;
    half_way = (int)((uint64_t)1 << (prec - 1)) * error_one;
    if (prec >= half_way + error) {
        rounded.f++;
        /* Prevent overflow. */
        if (rounded.f & (hidden_bit << 1)) {
            rounded.f >>= 1;
            rounded.e += 1;
        }
    }
    *result = grisu3_cast_double_from_diy_fp(rounded);
    return half_way - error >= prec_bits || prec_bits >= half_way + error;
}

/*
 * `end` is unchanged if number is handled natively, or it is the result
 * of strtod parsing in case of fallback.
 */
static const char *grisu3_encode_double(const char *buf, const char *end, int sign, uint64_t fraction, int exponent, int fraction_exp, int ulp_half_error, double *result)
{
    const int max_d_exp = GRISU3_D64_MAX_DEC_EXP;
    const int min_d_exp = GRISU3_D64_MIN_DEC_EXP;

    char *v_end;

    /* Both for user experience, and to protect internal power table lookups. */
    if (fraction == 0 || exponent < min_d_exp) {
        *result = 0.0;
        goto done;
    }
    if (exponent - 1 > max_d_exp) {
        *result = grisu3_double_infinity;
        goto done;
    }

    /*
     * `exponent` is the normalized value, fraction_exp is the size of
     * the representation in the `fraction value`, or one less than
     * number of significant digits.
     *
     * If the final value can be kept in 53 bits and we can avoid
     * division, then we can convert to double quite fast.
     *
     * ulf_half_error only happens when fraction is maxed out, so
     * fraction_exp > 22 by definition.
     *
     * fraction_exp >= 0 always.
     *
     * http://www.exploringbinary.com/fast-path-decimal-to-floating-point-conversion/
     */


#ifdef GRISU3_PARSE_FAST_CASE
    if (fraction < (1ULL << 53) && exponent >= 0 && exponent <= 22) {
        double v = (double)fraction;
       /* Multiplying by 1e-k instead of dividing by 1ek results in rounding error. */
        switch (exponent - fraction_exp) {
        case -22: v /= 1e22; break;
        case -21: v /= 1e21; break;
        case -20: v /= 1e20; break;
        case -19: v /= 1e19; break;
        case -18: v /= 1e18; break;
        case -17: v /= 1e17; break;
        case -16: v /= 1e16; break;
        case -15: v /= 1e15; break;
        case -14: v /= 1e14; break;
        case -13: v /= 1e13; break;
        case -12: v /= 1e12; break;
        case -11: v /= 1e11; break;
        case -10: v /= 1e10; break;
        case -9: v /= 1e9; break;
        case -8: v /= 1e8; break;
        case -7: v /= 1e7; break;
        case -6: v /= 1e6; break;
        case -5: v /= 1e5; break;
        case -4: v /= 1e4; break;
        case -3: v /= 1e3; break;
        case -2: v /= 1e2; break;
        case -1: v /= 1e1; break;
        case  0: break;
        case  1: v *= 1e1; break;
        case  2: v *= 1e2; break;
        case  3: v *= 1e3; break;
        case  4: v *= 1e4; break;
        case  5: v *= 1e5; break;
        case  6: v *= 1e6; break;
        case  7: v *= 1e7; break;
        case  8: v *= 1e8; break;
        case  9: v *= 1e9; break;
        case 10: v *= 1e10; break;
        case 11: v *= 1e11; break;
        case 12: v *= 1e12; break;
        case 13: v *= 1e13; break;
        case 14: v *= 1e14; break;
        case 15: v *= 1e15; break;
        case 16: v *= 1e16; break;
        case 17: v *= 1e17; break;
        case 18: v *= 1e18; break;
        case 19: v *= 1e19; break;
        case 20: v *= 1e20; break;
        case 21: v *= 1e21; break;
        case 22: v *= 1e22; break;
        }
        *result = v;
        goto done;
    }
#endif

    if (grisu3_diy_fp_encode_double(fraction, exponent, fraction_exp, ulp_half_error, result)) {
        goto done;
    }
#ifdef GRISU3_PARSE_ALLOW_ERROR
    goto done;
#endif
    *result = strtod(buf, &v_end);
    if (v_end < end) {
        return v_end;
    }
    return end;
done:
    if (sign) {
        *result = -*result;
    }
    return end;
}

/*
 * Returns buf if number wasn't matched, or null if number starts ok
 * but contains invalid content.
 */
static const char *grisu3_parse_hex_fp(const char *buf, const char *end, int sign, double *result)
{
    (void)buf;
    (void)end;
    (void)sign;
    *result = 0.0;
    /* Not currently supported. */
    return buf;
}

/*
 * Returns end pointer on success, or null, or buf if start is not a number.
 * Sets result to 0.0 on error.
 * Reads up to len + 1 bytes from buffer where len + 1 must not be a
 * valid part of a number, but all of buf, buf + len need not be a
 * number. Leading whitespace is NOT valid.
 * Very small numbers are truncated to +/-0.0 and numerically very large
 * numbers are returns as +/-infinity.
 *
 * A value must not end or begin with '.' (like JSON), but can have
 * leading zeroes (unlike JSON). A single leading zero followed by
 * an encoding symbol may or may not be interpreted as a non-decimal
 * encoding prefix, e.g. 0x, but a leading zero followed by a digit is
 * NOT interpreted as octal.
 * A single leading negative sign may appear before digits, but positive
 * sign is not allowed and space after the sign is not allowed.
 * At most the first 1000 characters of the input is considered.
 */
static const char *grisu3_parse_double(const char *buf, size_t len, double *result)
{
    const char *mark, *k, *end;
    int sign = 0, esign = 0;
    uint64_t fraction = 0;
    int exponent = 0;
    int ee = 0;
    int fraction_exp = 0;
    int ulp_half_error = 0;

    *result = 0.0;

    end = buf + len + 1;

    /* Failsafe for exponent overflow. */
    if (len > GRISU3_NUM_MAX_LEN) {
        end = buf + GRISU3_NUM_MAX_LEN + 1;
    }

    if (buf == end) {
        return buf;
    }
    mark = buf;
    if (*buf == '-') {
        ++buf;
        sign = 1;
        if (buf == end) {
            return 0;
        }
    }
    if (*buf == '0') {
        ++buf;
        /* | 0x20 is lower case ASCII. */
        if (buf != end && (*buf | 0x20) == 'x') {
            k = grisu3_parse_hex_fp(buf, end, sign, result);
            if (k == buf) {
                return mark;
            }
            return k;
        }
        /* Not worthwhile, except for getting the scale of integer part. */
        while (buf != end && *buf == '0') {
            ++buf;
        }
    } else {
        if (*buf < '1' || *buf > '9') {
            /*
             * If we didn't see a sign, just don't recognize it as
             * number, otherwise make it an error.
             */
            return sign ? 0 : mark;
        }
        fraction = (uint64_t)(*buf++ - '0');
    }
    k = buf;
    /*
     * We do not catch trailing zeroes when there is no decimal point.
     * This misses an opportunity for moving the exponent down into the
     * fast case. But it is unlikely to be worthwhile as it complicates
     * parsing.
     */
    while (buf != end && *buf >= '0' && *buf <= '9') {
        if (fraction >= UINT64_MAX / 10) {
            fraction += *buf >= '5';
            ulp_half_error = 1;
            break;
        }
        fraction = fraction * 10 + (uint64_t)(*buf++ - '0');
    }
    fraction_exp = (int)(buf - k);
    /* Skip surplus digits. Trailing zero does not introduce error. */
    while (buf != end && *buf == '0') {
        ++exponent;
        ++buf;
    }
    if (buf != end && *buf >= '1' && *buf <= '9') {
        ulp_half_error = 1;
        ++exponent;
        ++buf;
        while (buf != end && *buf >= '0' && *buf <= '9') {
            ++exponent;
            ++buf;
        }
    }
    if (buf != end && *buf == '.') {
        ++buf;
        k = buf;
        if (*buf < '0' || *buf > '9') {
            /* We don't accept numbers without leading or trailing digit. */
            return 0;
        }
        while (buf != end && *buf >= '0' && *buf <= '9') {
            if (fraction >= UINT64_MAX / 10) {
                if (!ulp_half_error) {
                    fraction += *buf >= '5';
                    ulp_half_error = 1;
                }
                break;
            }
            fraction = fraction * 10 + (uint64_t)(*buf++ - '0');
            --exponent;
        }
        fraction_exp += (int)(buf - k);
        while (buf != end && *buf == '0') {
            ++exponent;
            ++buf;
        }
        if (buf != end && *buf >= '1' && *buf <= '9') {
            ulp_half_error = 1;
            ++buf;
            while (buf != end && *buf >= '0' && *buf <= '9') {
                ++buf;
            }
        }
    }
    /*
     * Normalized exponent e.g: 1.23434e3 with fraction = 123434,
     * fraction_exp = 5, exponent = 3.
     * So value = fraction * 10^(exponent - fraction_exp)
     */
    exponent += fraction_exp;
    if (buf != end && (*buf | 0x20) == 'e') {
        if (end - buf < 2) {
            return 0;
        }
        ++buf;
        if (*buf == '+') {
            ++buf;
            if (buf == end) {
                return 0;
            }
        } else if (*buf == '-') {
            esign = 1;
            ++buf;
            if (buf == end) {
                return 0;
            }
        }
        if (*buf < '0' || *buf > '9') {
            return 0;
        }
        ee = *buf++ - '0';
        while (buf != end && *buf >= '0' && *buf <= '9') {
            /*
             * This test impacts performance and we do not need an
             * exact value just one large enough to dominate the fraction_exp.
             * Subsequent handling maps large absolute ee to 0 or infinity.
             */
            if (ee <= 0x7fff) {
                ee = ee * 10 + *buf - '0';
            }
            ++buf;
        }
    }
    exponent = exponent + (esign ? -ee : ee);

    /*
     * Exponent is now a base 10 normalized exponent so the absolute value
     * is less the 10^(exponent + 1) for positive exponents. For
     * denormalized doubles (using 11 bit exponent 0 with a fraction
     * shiftet down, extra small numbers can be achieved.
     *
     * https://en.wikipedia.org/wiki/Double-precision_floating-point_format
     *
     * 10^-324 holds the smallest normalized exponent (but not value) and
     * 10^308 holds the largest exponent. Internally our lookup table is
     * only safe to use within a range slightly larger than this.
     * Externally, a slightly larger/smaller value represents NaNs which
     * are technically also possible to store as a number.
     *
     */

    /* This also protects strod fallback parsing. */
    if (buf == end) {
        return 0;
    }
    return grisu3_encode_double(mark, buf, sign, fraction, exponent, fraction_exp, ulp_half_error, result);
}

#ifdef __cplusplus
}
#endif

#endif /* GRISU3_PARSE_H */
