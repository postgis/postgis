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

/* 2016-02-02: Updated by mikkelfj
 *
 * Extracted from MatGeoLib grisu3.c, Apache 2.0 license, and extended.
 *
 * This file is usually include via grisu3_print.h or grisu3_parse.h.
 *
 * The original MatGeoLib dtoa_grisu3 implementation is largely
 * unchanged except for the uint64 to double cast. The remaining changes
 * are file structure, name changes, and new additions for parsing:
 *
 * - Split into header files only:
 *   grisu3_math.h, grisu3_print.h, (added grisu3_parse.h)
 *
 * - names prefixed with grisu3_, grisu3_diy_fp_, GRISU3_.
 * - added static to all functions.
 * - disabled clang unused function warnings.
 * - guarded <stdint.h> to allow for alternative impl.
 * - added extra numeric constants needed for parsing.
 * - added dec_pow, cast_double_from_diy_fp.
 * - changed some function names for consistency.
 * - moved printing specific grisu3 functions to grisu3_print.h.
 * - changed double to uint64 cast to avoid aliasing.
 * - added new grisu3_parse.h for parsing doubles.
 * - grisu3_print_double (dtoa_grisu3) format .1 as 0.1 needed for valid JSON output
 *   and grisu3_parse_double wouldn't consume it.
 * - grsu3_print_double changed formatting to prefer 0.012 over 1.2e-2.
 *
 * These changes make it possible to include the files as headers only
 * in other software libraries without risking name conflicts, and to
 * extend the implementation with a port of Googles Double Conversion
 * strtod functionality for parsing doubles.
 *
 * Extracted from: rev. 915501a / Dec 22, 2015
 * <https://github.com/juj/MathGeoLib/blob/master/src/Math/grisu3.c>
 * MathGeoLib License: http://www.apache.org/licenses/LICENSE-2.0.html
 */

#ifndef GRISU3_MATH_H
#define GRISU3_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Guarded to allow inclusion of pstdint.h first, if stdint.h is not supported. */
#ifndef UINT8_MAX
#include <stdint.h> /* uint64_t etc. */
#endif

#ifdef GRISU3_NO_ASSERT
#undef GRISU3_ASSERT
#define GRISU3_ASSERT(x) ((void)0)
#endif

#ifndef GRISU3_ASSERT
#include <assert.h> /* assert */
#define GRISU3_ASSERT(x) assert(x)
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4204) /* nonstandard extension used : non-constant aggregate initializer */
#endif

#define GRISU3_D64_SIGN             0x8000000000000000ULL
#define GRISU3_D64_EXP_MASK         0x7FF0000000000000ULL
#define GRISU3_D64_FRACT_MASK       0x000FFFFFFFFFFFFFULL
#define GRISU3_D64_IMPLICIT_ONE     0x0010000000000000ULL
#define GRISU3_D64_EXP_POS          52
#define GRISU3_D64_EXP_BIAS         1075
#define GRISU3_D64_DENORM_EXP       (-GRISU3_D64_EXP_BIAS + 1)
#define GRISU3_DIY_FP_FRACT_SIZE    64
#define GRISU3_D_1_LOG2_10          0.30102999566398114 /* 1 / lg(10) */
#define GRISU3_MIN_TARGET_EXP       -60
#define GRISU3_MASK32               0xFFFFFFFFULL
#define GRISU3_MIN_CACHED_EXP       -348
#define GRISU3_MAX_CACHED_EXP       340
#define GRISU3_CACHED_EXP_STEP      8
#define GRISU3_D64_MAX_DEC_EXP      309
#define GRISU3_D64_MIN_DEC_EXP      -324
#define GRISU3_D64_INF              GRISU3_D64_EXP_MASK

#define GRISU3_MIN(x,y) ((x) <= (y) ? (x) : (y))
#define GRISU3_MAX(x,y) ((x) >= (y) ? (x) : (y))


typedef struct grisu3_diy_fp
{
    uint64_t f;
    int e;
} grisu3_diy_fp_t;

typedef struct grisu3_diy_fp_power
{
    uint64_t fract;
    int16_t b_exp, d_exp;
} grisu3_diy_fp_power_t;

typedef union {
    uint64_t u64;
    double d64;
} grisu3_cast_double_t;

static uint64_t grisu3_cast_uint64_from_double(double d)
{
    grisu3_cast_double_t cd;
    cd.d64 = d;
    return cd.u64;
}

static double grisu3_cast_double_from_uint64(uint64_t u)
{
    grisu3_cast_double_t cd;
    cd.u64 = u;
    return cd.d64;
}

#define grisu3_double_infinity grisu3_cast_double_from_uint64(GRISU3_D64_INF)
#define grisu3_double_nan grisu3_cast_double_from_uint64(GRISU3_D64_INF + 1)

static const grisu3_diy_fp_power_t grisu3_diy_fp_pow_cache[] =
{
    { 0xfa8fd5a0081c0288ULL, -1220, -348 },
    { 0xbaaee17fa23ebf76ULL, -1193, -340 },
    { 0x8b16fb203055ac76ULL, -1166, -332 },
    { 0xcf42894a5dce35eaULL, -1140, -324 },
    { 0x9a6bb0aa55653b2dULL, -1113, -316 },
    { 0xe61acf033d1a45dfULL, -1087, -308 },
    { 0xab70fe17c79ac6caULL, -1060, -300 },
    { 0xff77b1fcbebcdc4fULL, -1034, -292 },
    { 0xbe5691ef416bd60cULL, -1007, -284 },
    { 0x8dd01fad907ffc3cULL,  -980, -276 },
    { 0xd3515c2831559a83ULL,  -954, -268 },
    { 0x9d71ac8fada6c9b5ULL,  -927, -260 },
    { 0xea9c227723ee8bcbULL,  -901, -252 },
    { 0xaecc49914078536dULL,  -874, -244 },
    { 0x823c12795db6ce57ULL,  -847, -236 },
    { 0xc21094364dfb5637ULL,  -821, -228 },
    { 0x9096ea6f3848984fULL,  -794, -220 },
    { 0xd77485cb25823ac7ULL,  -768, -212 },
    { 0xa086cfcd97bf97f4ULL,  -741, -204 },
    { 0xef340a98172aace5ULL,  -715, -196 },
    { 0xb23867fb2a35b28eULL,  -688, -188 },
    { 0x84c8d4dfd2c63f3bULL,  -661, -180 },
    { 0xc5dd44271ad3cdbaULL,  -635, -172 },
    { 0x936b9fcebb25c996ULL,  -608, -164 },
    { 0xdbac6c247d62a584ULL,  -582, -156 },
    { 0xa3ab66580d5fdaf6ULL,  -555, -148 },
    { 0xf3e2f893dec3f126ULL,  -529, -140 },
    { 0xb5b5ada8aaff80b8ULL,  -502, -132 },
    { 0x87625f056c7c4a8bULL,  -475, -124 },
    { 0xc9bcff6034c13053ULL,  -449, -116 },
    { 0x964e858c91ba2655ULL,  -422, -108 },
    { 0xdff9772470297ebdULL,  -396, -100 },
    { 0xa6dfbd9fb8e5b88fULL,  -369,  -92 },
    { 0xf8a95fcf88747d94ULL,  -343,  -84 },
    { 0xb94470938fa89bcfULL,  -316,  -76 },
    { 0x8a08f0f8bf0f156bULL,  -289,  -68 },
    { 0xcdb02555653131b6ULL,  -263,  -60 },
    { 0x993fe2c6d07b7facULL,  -236,  -52 },
    { 0xe45c10c42a2b3b06ULL,  -210,  -44 },
    { 0xaa242499697392d3ULL,  -183,  -36 },
    { 0xfd87b5f28300ca0eULL,  -157,  -28 },
    { 0xbce5086492111aebULL,  -130,  -20 },
    { 0x8cbccc096f5088ccULL,  -103,  -12 },
    { 0xd1b71758e219652cULL,   -77,   -4 },
    { 0x9c40000000000000ULL,   -50,    4 },
    { 0xe8d4a51000000000ULL,   -24,   12 },
    { 0xad78ebc5ac620000ULL,     3,   20 },
    { 0x813f3978f8940984ULL,    30,   28 },
    { 0xc097ce7bc90715b3ULL,    56,   36 },
    { 0x8f7e32ce7bea5c70ULL,    83,   44 },
    { 0xd5d238a4abe98068ULL,   109,   52 },
    { 0x9f4f2726179a2245ULL,   136,   60 },
    { 0xed63a231d4c4fb27ULL,   162,   68 },
    { 0xb0de65388cc8ada8ULL,   189,   76 },
    { 0x83c7088e1aab65dbULL,   216,   84 },
    { 0xc45d1df942711d9aULL,   242,   92 },
    { 0x924d692ca61be758ULL,   269,  100 },
    { 0xda01ee641a708deaULL,   295,  108 },
    { 0xa26da3999aef774aULL,   322,  116 },
    { 0xf209787bb47d6b85ULL,   348,  124 },
    { 0xb454e4a179dd1877ULL,   375,  132 },
    { 0x865b86925b9bc5c2ULL,   402,  140 },
    { 0xc83553c5c8965d3dULL,   428,  148 },
    { 0x952ab45cfa97a0b3ULL,   455,  156 },
    { 0xde469fbd99a05fe3ULL,   481,  164 },
    { 0xa59bc234db398c25ULL,   508,  172 },
    { 0xf6c69a72a3989f5cULL,   534,  180 },
    { 0xb7dcbf5354e9beceULL,   561,  188 },
    { 0x88fcf317f22241e2ULL,   588,  196 },
    { 0xcc20ce9bd35c78a5ULL,   614,  204 },
    { 0x98165af37b2153dfULL,   641,  212 },
    { 0xe2a0b5dc971f303aULL,   667,  220 },
    { 0xa8d9d1535ce3b396ULL,   694,  228 },
    { 0xfb9b7cd9a4a7443cULL,   720,  236 },
    { 0xbb764c4ca7a44410ULL,   747,  244 },
    { 0x8bab8eefb6409c1aULL,   774,  252 },
    { 0xd01fef10a657842cULL,   800,  260 },
    { 0x9b10a4e5e9913129ULL,   827,  268 },
    { 0xe7109bfba19c0c9dULL,   853,  276 },
    { 0xac2820d9623bf429ULL,   880,  284 },
    { 0x80444b5e7aa7cf85ULL,   907,  292 },
    { 0xbf21e44003acdd2dULL,   933,  300 },
    { 0x8e679c2f5e44ff8fULL,   960,  308 },
    { 0xd433179d9c8cb841ULL,   986,  316 },
    { 0x9e19db92b4e31ba9ULL,  1013,  324 },
    { 0xeb96bf6ebadf77d9ULL,  1039,  332 },
    { 0xaf87023b9bf0ee6bULL,  1066,  340 }
};

/* Avoid dependence on lib math to get (int)ceil(v) */
static int grisu3_iceil(double v)
{
    int k = (int)v;
    if (v < 0) return k;
    return v - k == 0 ? k : k + 1;
}

static int grisu3_diy_fp_cached_pow(int exp, grisu3_diy_fp_t *p)
{
    int k = grisu3_iceil((exp+GRISU3_DIY_FP_FRACT_SIZE-1) * GRISU3_D_1_LOG2_10);
    int i = (k-GRISU3_MIN_CACHED_EXP-1) / GRISU3_CACHED_EXP_STEP + 1;
    p->f = grisu3_diy_fp_pow_cache[i].fract;
    p->e = grisu3_diy_fp_pow_cache[i].b_exp;
    return grisu3_diy_fp_pow_cache[i].d_exp;
}

static grisu3_diy_fp_t grisu3_diy_fp_minus(grisu3_diy_fp_t x, grisu3_diy_fp_t y)
{
    grisu3_diy_fp_t d; d.f = x.f - y.f; d.e = x.e;
    GRISU3_ASSERT(x.e == y.e && x.f >= y.f);
    return d;
}

static grisu3_diy_fp_t grisu3_diy_fp_multiply(grisu3_diy_fp_t x, grisu3_diy_fp_t y)
{
    uint64_t a, b, c, d, ac, bc, ad, bd, tmp;
    grisu3_diy_fp_t r;
    a = x.f >> 32; b = x.f & GRISU3_MASK32;
    c = y.f >> 32; d = y.f & GRISU3_MASK32;
    ac = a*c; bc = b*c;
    ad = a*d; bd = b*d;
    tmp = (bd >> 32) + (ad & GRISU3_MASK32) + (bc & GRISU3_MASK32);
    tmp += 1U << 31; /* round */
    r.f = ac + (ad >> 32) + (bc >> 32) + (tmp >> 32);
    r.e = x.e + y.e + 64;
    return r;
}

static grisu3_diy_fp_t grisu3_diy_fp_normalize(grisu3_diy_fp_t n)
{
    GRISU3_ASSERT(n.f != 0);
    while(!(n.f & 0xFFC0000000000000ULL)) { n.f <<= 10; n.e -= 10; }
    while(!(n.f & GRISU3_D64_SIGN)) { n.f <<= 1; --n.e; }
    return n;
}

static grisu3_diy_fp_t grisu3_cast_diy_fp_from_double(double d)
{
    grisu3_diy_fp_t fp;
    uint64_t u64 = grisu3_cast_uint64_from_double(d);
    if (!(u64 & GRISU3_D64_EXP_MASK)) { fp.f = u64 & GRISU3_D64_FRACT_MASK; fp.e = 1 - GRISU3_D64_EXP_BIAS; }
    else { fp.f = (u64 & GRISU3_D64_FRACT_MASK) + GRISU3_D64_IMPLICIT_ONE; fp.e = (int)((u64 & GRISU3_D64_EXP_MASK) >> GRISU3_D64_EXP_POS) - GRISU3_D64_EXP_BIAS; }
    return fp;
}

static double grisu3_cast_double_from_diy_fp(grisu3_diy_fp_t n)
{
    const uint64_t hidden_bit = GRISU3_D64_IMPLICIT_ONE;
    const uint64_t frac_mask = GRISU3_D64_FRACT_MASK;
    const int denorm_exp = GRISU3_D64_DENORM_EXP;
    const int exp_bias = GRISU3_D64_EXP_BIAS;
    const int exp_pos = GRISU3_D64_EXP_POS;

    grisu3_diy_fp_t v = n;
    uint64_t e_biased;

    while (v.f > hidden_bit + frac_mask) {
        v.f >>= 1;
        ++v.e;
    }
    if (v.e < denorm_exp) {
        return 0.0;
    }
    while (v.e > denorm_exp && (v.f & hidden_bit) == 0) {
        v.f <<= 1;
        --v.e;
    }
    if (v.e == denorm_exp && (v.f & hidden_bit) == 0) {
        e_biased = 0;
    } else {
        e_biased = (uint64_t)(v.e + exp_bias);
    }
    return grisu3_cast_double_from_uint64((v.f & frac_mask) | (e_biased << exp_pos));
}

/* pow10_cache[i] = 10^(i-1) */
static const unsigned int grisu3_pow10_cache[] = { 0, 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };

static int grisu3_largest_pow10(uint32_t n, int n_bits, uint32_t *power)
{
    int guess = ((n_bits + 1) * 1233 >> 12) + 1/*skip first entry*/;
    if (n < grisu3_pow10_cache[guess]) --guess; /* We don't have any guarantees that 2^n_bits <= n. */
    *power = grisu3_pow10_cache[guess];
    return guess;
}

#ifdef __cplusplus
}
#endif

#endif /* GRISU3_MATH_H */
