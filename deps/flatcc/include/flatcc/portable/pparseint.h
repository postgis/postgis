#ifndef PPARSEINT_H
#define PPARSEINT_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Type specific integer parsers:
 *
 *     const char *
 *     parse_<type-name>(const char *buf, size_t len, <type> *value, int *status);
 *
 *     parse_uint64, parse_int64
 *     parse_uint32, parse_int32
 *     parse_uint16, parse_int16
 *     parse_uint8, parse_int8
 *     parse_ushort,  parse_short
 *     parse_uint, parse_int
 *     parse_ulong, parse_long
 *
 * Leading space must be stripped in advance. Status argument can be
 * null.
 *
 * Returns pointer to end of match and a non-negative status code
 * on succcess (0 for unsigned, 1 for signed):
 *
 *     PARSE_INTEGER_UNSIGNED
 *     PARSE_INTEGER_SIGNED
 *
 * Returns null with a negative status code and unmodified value on
 * invalid integer formats:
 *
 *     PARSE_INTEGER_OVERFLOW
 *     PARSE_INTEGER_UNDERFLOW
 *     PARSE_INTEGER_INVALID
 *
 * Returns input buffer with negative status code and unmodified value
 * if first character does not start an integer (not a sign or a digit).
 *
 *     PARSE_INTEGER_UNMATCHED
 *     PARSE_INTEGER_END
 *
 * The signed parsers only works with two's complement architectures.
 *
 * Note: the corresponding parse_float and parse_double parsers do not
 * have a status argument because +/-Inf and NaN are conventionally used
 * for this.
 */

#include "limits.h"
#ifndef UINT8_MAX
#include <stdint.h>
#endif

#define PARSE_INTEGER_UNSIGNED       0
#define PARSE_INTEGER_SIGNED         1
#define PARSE_INTEGER_OVERFLOW      -1
#define PARSE_INTEGER_UNDERFLOW     -2
#define PARSE_INTEGER_INVALID       -3
#define PARSE_INTEGER_UNMATCHED     -4
#define PARSE_INTEGER_END           -5

/*
 * Generic integer parser that holds 64-bit unsigned values and stores
 * sign separately. Leading space is not valid.
 *
 * Note: this function differs from the type specific parsers like
 * parse_int64 by not negating the value when there is a sign. It
 * differs from parse_uint64 by being able to return a negative
 * UINT64_MAX successfully.
 *
 * This parser is used by all type specific integer parsers.
 *
 * Status argument can be null.
 */
static const char *parse_integer(const char *buf, size_t len, uint64_t *value, int *status)
{
    uint64_t x0, x = 0;
    const char *k, *end = buf + len;
    int sign, status_;

    if (!status) {
        status = &status_;
    }
    if (buf == end) {
        *status = PARSE_INTEGER_END;
        return buf;
    }
    k = buf;
    sign = *buf == '-';
    buf += sign;
    while (buf != end && *buf >= '0' && *buf <= '9') {
        x0 = x;
        x = x * 10 + (uint64_t)(*buf - '0');
        if (x0 > x) {
            *status = sign ? PARSE_INTEGER_UNDERFLOW : PARSE_INTEGER_OVERFLOW;
            return 0;
        }
        ++buf;
    }
    if (buf == k) {
        /* No number was matched, but it isn't an invalid number either. */
        *status = PARSE_INTEGER_UNMATCHED;
        return buf;
    }
    if (buf == k + sign) {
        *status = PARSE_INTEGER_INVALID;
        return 0;
    }
    if (buf != end)
    switch (*buf) {
    case 'e': case 'E': case '.': case 'p': case 'P':
        *status = PARSE_INTEGER_INVALID;
        return 0;
    }
    *value = x;
    *status = sign;
    return buf;
}

/*
 * Parse hex values like 0xff, -0xff, 0XdeAdBeaf42, cannot be trailed by '.', 'p', or 'P'.
 * Overflows if string is more than 16 valid hex digits. Otherwise similar to parse_integer.
 */
static const char *parse_hex_integer(const char *buf, size_t len, uint64_t *value, int *status)
{
    uint64_t x = 0;
    const char *k, *k2, *end = buf + len;
    int sign, status_;
    unsigned char c;

    if (!status) {
        status = &status_;
    }
    if (buf == end) {
        *status = PARSE_INTEGER_END;
        return buf;
    }
    sign = *buf == '-';
    buf += sign;
    if (end - buf < 2 || buf[0] != '0' || (buf[1] | 0x20) != 'x') {
        *status = PARSE_INTEGER_UNMATCHED;
        return buf - sign;
    }
    buf += 2;
    k = buf;
    k2 = end;
    if (end - buf > 16) {
        k2 = buf + 16;
    }
    while (buf != k2) {
        c = (unsigned char)*buf;
        if (c >= '0' && c <= '9') {
            x = x * 16 + c - '0';
        } else {
            /* Lower case. */
            c |= 0x20;
            if (c >= 'a' && c <= 'f') {
                x = x * 16 + c - 'a' + 10;
            } else {
                break;
            }
        }
        ++buf;
    }
    if (buf == k) {
        if (sign) {
            *status = PARSE_INTEGER_INVALID;
            return 0;
        } else {
            /* No number was matched, but it isn't an invalid number either. */
            *status = PARSE_INTEGER_UNMATCHED;
            return buf;
        }
    }
    if (buf == end) {
        goto done;
    }
    c = (unsigned char)*buf;
    if (buf == k2) {
        if (c >= '0' && c <= '9') {
            *status = sign ? PARSE_INTEGER_UNDERFLOW : PARSE_INTEGER_OVERFLOW;
            return 0;
        }
        c |= 0x20;
        if (c >= 'a' && c <= 'f') {
            *status = sign ? PARSE_INTEGER_UNDERFLOW : PARSE_INTEGER_OVERFLOW;
            return 0;
        }
    }
    switch (c) {
    case '.': case 'p': case 'P':
        *status = PARSE_INTEGER_INVALID;
        return 0;
    }
done:
    *value = x;
    *status = sign;
    return buf;
}


#define __portable_define_parse_unsigned(NAME, TYPE, LIMIT)                 \
static inline const char *parse_ ## NAME                                    \
        (const char *buf, size_t len, TYPE *value, int *status)             \
{                                                                           \
    int status_ = 0;                                                        \
    uint64_t x;                                                             \
                                                                            \
    if (!status) {                                                          \
        status = &status_;                                                  \
    }                                                                       \
    buf = parse_integer(buf, len, &x, status);                              \
    switch (*status) {                                                      \
    case PARSE_INTEGER_UNSIGNED:                                            \
        if (x <= LIMIT) {                                                   \
            *value = (TYPE)x;                                               \
            return buf;                                                     \
        }                                                                   \
        *status = PARSE_INTEGER_OVERFLOW;                                   \
        return 0;                                                           \
    case PARSE_INTEGER_SIGNED:                                              \
        *status = PARSE_INTEGER_UNDERFLOW;                                  \
        return 0;                                                           \
    default:                                                                \
        return buf;                                                         \
    }                                                                       \
}

#define __portable_define_parse_hex_unsigned(NAME, TYPE, LIMIT)             \
static inline const char *parse_hex_ ## NAME                                \
        (const char *buf, size_t len, TYPE *value, int *status)             \
{                                                                           \
    int status_ = 0;                                                        \
    uint64_t x;                                                             \
                                                                            \
    if (!status) {                                                          \
        status = &status_;                                                  \
    }                                                                       \
    buf = parse_hex_integer(buf, len, &x, status);                          \
    switch (*status) {                                                      \
    case PARSE_INTEGER_UNSIGNED:                                            \
        if (x <= LIMIT) {                                                   \
            *value = (TYPE)x;                                               \
            return buf;                                                     \
        }                                                                   \
        *status = PARSE_INTEGER_OVERFLOW;                                   \
        return 0;                                                           \
    case PARSE_INTEGER_SIGNED:                                              \
        *status = PARSE_INTEGER_UNDERFLOW;                                  \
        return 0;                                                           \
    default:                                                                \
        return buf;                                                         \
    }                                                                       \
}

/* This assumes two's complement. */
#define __portable_define_parse_signed(NAME, TYPE, LIMIT)                   \
static inline const char *parse_ ## NAME                                    \
        (const char *buf, size_t len, TYPE *value, int *status)             \
{                                                                           \
    int status_ = 0;                                                        \
    uint64_t x;                                                             \
                                                                            \
    if (!status) {                                                          \
        status = &status_;                                                  \
    }                                                                       \
    buf = parse_integer(buf, len, &x, status);                              \
    switch (*status) {                                                      \
    case PARSE_INTEGER_UNSIGNED:                                            \
        if (x <= LIMIT) {                                                   \
            *value = (TYPE)x;                                               \
            return buf;                                                     \
        }                                                                   \
        *status = PARSE_INTEGER_OVERFLOW;                                   \
        return 0;                                                           \
    case PARSE_INTEGER_SIGNED:                                              \
        if (x <= (uint64_t)(LIMIT) + 1) {                                   \
            *value = (TYPE)-(int64_t)x;                                     \
            return buf;                                                     \
        }                                                                   \
        *status = PARSE_INTEGER_UNDERFLOW;                                  \
        return 0;                                                           \
    default:                                                                \
        return buf;                                                         \
    }                                                                       \
}

/* This assumes two's complement. */
#define __portable_define_parse_hex_signed(NAME, TYPE, LIMIT)               \
static inline const char *parse_hex_ ## NAME                                \
        (const char *buf, size_t len, TYPE *value, int *status)             \
{                                                                           \
    int status_ = 0;                                                        \
    uint64_t x;                                                             \
                                                                            \
    if (!status) {                                                          \
        status = &status_;                                                  \
    }                                                                       \
    buf = parse_hex_integer(buf, len, &x, status);                          \
    switch (*status) {                                                      \
    case PARSE_INTEGER_UNSIGNED:                                            \
        if (x <= LIMIT) {                                                   \
            *value = (TYPE)x;                                               \
            return buf;                                                     \
        }                                                                   \
        *status = PARSE_INTEGER_OVERFLOW;                                   \
        return 0;                                                           \
    case PARSE_INTEGER_SIGNED:                                              \
        if (x <= (uint64_t)(LIMIT) + 1) {                                   \
            *value = (TYPE)-(int64_t)x;                                     \
            return buf;                                                     \
        }                                                                   \
        *status = PARSE_INTEGER_UNDERFLOW;                                  \
        return 0;                                                           \
    default:                                                                \
        return buf;                                                         \
    }                                                                       \
}

static inline const char *parse_uint64(const char *buf, size_t len, uint64_t *value, int *status)
{
    buf = parse_integer(buf, len, value, status);
    if (*status == PARSE_INTEGER_SIGNED) {
        *status = PARSE_INTEGER_UNDERFLOW;
        return 0;
    }
    return buf;
}

static inline const char *parse_hex_uint64(const char *buf, size_t len, uint64_t *value, int *status)
{
    buf = parse_hex_integer(buf, len, value, status);
    if (*status == PARSE_INTEGER_SIGNED) {
        *status = PARSE_INTEGER_UNDERFLOW;
        return 0;
    }
    return buf;
}

__portable_define_parse_signed(int64, int64_t, INT64_MAX)
__portable_define_parse_signed(int32, int32_t, INT32_MAX)
__portable_define_parse_unsigned(uint16, uint16_t, UINT16_MAX)
__portable_define_parse_signed(int16, int16_t, INT16_MAX)
__portable_define_parse_unsigned(uint8, uint8_t, UINT8_MAX)
__portable_define_parse_signed(int8, int8_t, INT8_MAX)

__portable_define_parse_hex_signed(int64, int64_t, INT64_MAX)
__portable_define_parse_hex_signed(int32, int32_t, INT32_MAX)
__portable_define_parse_hex_unsigned(uint16, uint16_t, UINT16_MAX)
__portable_define_parse_hex_signed(int16, int16_t, INT16_MAX)
__portable_define_parse_hex_unsigned(uint8, uint8_t, UINT8_MAX)
__portable_define_parse_hex_signed(int8, int8_t, INT8_MAX)

__portable_define_parse_unsigned(ushort, unsigned short, USHRT_MAX)
__portable_define_parse_signed(short, short, SHRT_MAX)
__portable_define_parse_unsigned(uint, unsigned int, UINT_MAX)
__portable_define_parse_signed(int, int, INT_MAX)
__portable_define_parse_unsigned(ulong, unsigned long, ULONG_MAX)
__portable_define_parse_signed(long, unsigned long, LONG_MAX)

__portable_define_parse_hex_unsigned(ushort, unsigned short, USHRT_MAX)
__portable_define_parse_hex_signed(short, short, SHRT_MAX)
__portable_define_parse_hex_unsigned(uint, unsigned int, UINT_MAX)
__portable_define_parse_hex_signed(int, int, INT_MAX)
__portable_define_parse_hex_unsigned(ulong, unsigned long, ULONG_MAX)
__portable_define_parse_hex_signed(long, unsigned long, LONG_MAX)

#ifdef __cplusplus
}
#endif

#endif /* PPARSEINT_H */
