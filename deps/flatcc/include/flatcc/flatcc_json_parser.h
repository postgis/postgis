#ifndef FLATCC_JSON_PARSE_H
#define FLATCC_JSON_PARSE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * JSON RFC:
 * http://www.ietf.org/rfc/rfc4627.txt?number=4627
 *
 * With several flatbuffers specific extensions.
 */

#include <stdlib.h>
#include <string.h>

#include "flatcc/flatcc_rtconfig.h"
#include "flatcc/flatcc_builder.h"
#include "flatcc/flatcc_unaligned.h"

#define PDIAGNOSTIC_IGNORE_UNUSED
#include "flatcc/portable/pdiagnostic_push.h"

enum flatcc_json_parser_flags {
    flatcc_json_parser_f_skip_unknown = 1,
    flatcc_json_parser_f_force_add = 2,
    flatcc_json_parser_f_with_size = 4,
    flatcc_json_parser_f_skip_array_overflow = 8,
    flatcc_json_parser_f_reject_array_underflow = 16
};

#define FLATCC_JSON_PARSE_ERROR_MAP(XX)                                     \
    XX(ok,                      "ok")                                       \
    XX(eof,                     "eof")                                      \
    XX(deep_nesting,            "deep nesting")                             \
    XX(trailing_comma,          "trailing comma")                           \
    XX(expected_colon,          "expected colon")                           \
    XX(unexpected_character,    "unexpected character")                     \
    XX(invalid_numeric,         "invalid numeric")                          \
    XX(overflow,                "overflow")                                 \
    XX(underflow,               "underflow")                                \
    XX(unbalanced_array,        "unbalanced array")                         \
    XX(unbalanced_object,       "unbalanced object")                        \
    XX(precision_loss,          "precision loss")                           \
    XX(float_unexpected,        "float unexpected")                         \
    XX(unknown_symbol,          "unknown symbol")                           \
    XX(unquoted_symbolic_list,  "unquoted list of symbols")                 \
    XX(unknown_union,           "unknown union type")                       \
    XX(expected_string,         "expected string")                          \
    XX(invalid_character,       "invalid character")                        \
    XX(invalid_escape,          "invalid escape")                           \
    XX(invalid_type,            "invalid type")                             \
    XX(unterminated_string,     "unterminated string")                      \
    XX(expected_object,         "expected object")                          \
    XX(expected_array,          "expected array")                           \
    XX(expected_scalar,         "expected literal or symbolic scalar")      \
    XX(expected_union_type,     "expected union type")                      \
    XX(union_none_present,      "union present with type NONE")             \
    XX(union_none_not_null,     "union of type NONE is not null")           \
    XX(union_incomplete,        "table has incomplete union")               \
    XX(duplicate,               "table has duplicate field")                \
    XX(required,                "required field missing")                   \
    XX(union_vector_length,     "union vector length mismatch")             \
    XX(base64,                  "invalid base64 content")                   \
    XX(base64url,               "invalid base64url content")                \
    XX(array_underflow,         "fixed length array underflow")               \
    XX(array_overflow,          "fixed length array overflow")                \
    XX(runtime,                 "runtime error")                            \
    XX(not_supported,           "not supported")

enum flatcc_json_parser_error_no {
#define XX(no, str) flatcc_json_parser_error_##no,
    FLATCC_JSON_PARSE_ERROR_MAP(XX)
#undef XX
};

const char *flatcc_json_parser_error_string(int err);

#define flatcc_json_parser_ok flatcc_json_parser_error_ok
#define flatcc_json_parser_eof flatcc_json_parser_error_eof

/*
 * The struct may be zero initialized in which case the line count will
 * start at line zero, or the line may be set to 1 initially. The ctx
 * is only used for error reporting and tracking non-standard unquoted
 * ctx.
 *
 * `ctx` may for example hold a flatcc_builder_t pointer.
 */
typedef struct flatcc_json_parser_ctx flatcc_json_parser_t;
struct flatcc_json_parser_ctx {
    flatcc_builder_t *ctx;
    const char *line_start;
    int flags;
#if FLATCC_JSON_PARSE_ALLOW_UNQUOTED
    int unquoted;
#endif

    int line, pos;
    int error;
    const char *start;
    const char *end;
    const char *error_loc;
    /* Set at end of successful parse. */
    const char *end_loc;
};

static inline int flatcc_json_parser_get_error(flatcc_json_parser_t *ctx)
{
    return ctx->error;
}

static inline void flatcc_json_parser_init(flatcc_json_parser_t *ctx, flatcc_builder_t *B, const char *buf, const char *end, int flags)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->ctx = B;
    ctx->line_start = buf;
    ctx->line = 1;
    ctx->flags = flags;
    /* These are not needed for parsing, but may be helpful in reporting etc. */
    ctx->start = buf;
    ctx->end = end;
    ctx->error_loc = buf;
}

const char *flatcc_json_parser_set_error(flatcc_json_parser_t *ctx, const char *loc, const char *end, int reason);

/*
 * Wide space is not necessarily beneficial in the typical space, but it
 * also isn't expensive so it may be added when there are applications
 * that can benefit.
 */
const char *flatcc_json_parser_space_ext(flatcc_json_parser_t *ctx, const char *buf, const char *end);

static inline const char *flatcc_json_parser_space(flatcc_json_parser_t *ctx, const char *buf, const char *end)
{
    if (end - buf > 1) {
        if (buf[0] > 0x20) {
            return buf;
        }
        if (buf[0] == 0x20 && buf[1] > 0x20) {
            return buf + 1;
        }
    }
    return flatcc_json_parser_space_ext(ctx, buf, end);
}


static inline const char *flatcc_json_parser_string_start(flatcc_json_parser_t *ctx, const char *buf, const char *end)
{
    if (buf == end || *buf != '\"') {
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_expected_string);
    }
    return ++buf;
}

static inline const char *flatcc_json_parser_string_end(flatcc_json_parser_t *ctx, const char *buf, const char *end)
{
    if (buf == end || *buf != '\"') {
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_unterminated_string);
    }
    return ++buf;
}

/*
 * Parse a string as a fixed length char array as `s` with length `n`.
 * and raise errors according to overflow/underflow runtime flags. Zero
 * and truncate as needed. A trailing zero is not inserted if the input
 * is at least the same length as the char array.
 * 
 * Runtime flags: `skip_array_overflow`, `pad_array_underflow`.
 */
const char *flatcc_json_parser_char_array(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, char *s, size_t n);

/*
 * Creates a string. Returns *ref == 0 on unrecoverable error or
 * sets *ref to a valid new string reference.
 */
const char *flatcc_json_parser_build_string(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, flatcc_builder_ref_t *ref);

typedef char flatcc_json_parser_escape_buffer_t[5];
/*
 * If the buffer does not hold a valid escape sequence, an error is
 * returned with code[0] = 0/
 *
 * Otherwise code[0] the length (1-4) of the remaining
 * characters in the code, transcoded from the escape sequence
 * where a length of 4 only happens with escapaped surrogate pairs.
 *
 * The JSON extension `\xXX` is supported and may produced invalid UTF-8
 * characters such as 0xff. The standard JSON escape `\uXXXX` is not
 * checked for invalid code points and may produce invalid UTF-8.
 *
 * Regular characters are expected to valid UTF-8 but they are not checked
 * and may therefore produce invalid UTF-8.
 *
 * Control characters within a string are rejected except in the
 * standard JSON escpaped form for `\n \r \t \b \f`.
 *
 * Additional escape codes as per standard JSON: `\\ \/ \"`.
 */
const char *flatcc_json_parser_string_escape(flatcc_json_parser_t *ctx, const char *buf, const char *end, flatcc_json_parser_escape_buffer_t code);

/*
 * Parses the longest unescaped run of string content followed by either
 * an escape encoding, string termination, or error.
 */
const char *flatcc_json_parser_string_part(flatcc_json_parser_t *ctx, const char *buf, const char *end);

static inline const char *flatcc_json_parser_symbol_start(flatcc_json_parser_t *ctx, const char *buf, const char *end)
{
    if (buf == end) {
        return buf;
    }
    if (*buf == '\"') {
        ++buf;
#if FLATCC_JSON_PARSE_ALLOW_UNQUOTED
        ctx->unquoted = 0;
#endif
    } else {
#if FLATCC_JSON_PARSE_ALLOW_UNQUOTED
        if (*buf == '.') {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_unexpected_character);
        }
        ctx->unquoted = 1;
#else
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_unexpected_character);
#endif
    }
    return buf;
}

static inline uint64_t flatcc_json_parser_symbol_part_ext(const char *buf, const char *end)
{
    uint64_t w = 0;
    size_t n = (size_t)(end - buf);

    if (n > 8) {
        n = 8;
    }
    /* This can bloat inlining for a rarely executed case. */
#if 1
    /* Fall through comments needed to silence gcc 7 warnings. */
    switch (n) {
    case 8: w |= ((uint64_t)buf[7]) << (0 * 8);
        /* Fall through */
    case 7: w |= ((uint64_t)buf[6]) << (1 * 8);
        /* Fall through */
    case 6: w |= ((uint64_t)buf[5]) << (2 * 8);
        /* Fall through */
    case 5: w |= ((uint64_t)buf[4]) << (3 * 8);
        /* Fall through */
    case 4: w |= ((uint64_t)buf[3]) << (4 * 8);
        /* Fall through */
    case 3: w |= ((uint64_t)buf[2]) << (5 * 8);
        /* Fall through */
    case 2: w |= ((uint64_t)buf[1]) << (6 * 8);
        /* Fall through */
    case 1: w |= ((uint64_t)buf[0]) << (7 * 8);
        /* Fall through */
    case 0:
        break;
    }
#else
    /* But this is hardly much of an improvement. */
    {
        size_t i;
        for (i = 0; i < n; ++i) {
            w <<= 8;
            if (i < n) {
                w = buf[i];
            }
        }
    }
#endif
    return w;
}

/*
 * Read out string as a big endian word. This allows for trie lookup,
 * also when trailing characters are beyond keyword. This assumes the
 * external words tested against are valid and therefore there need be
 * no checks here. If a match is not made, the symbol_end function will
 * consume and check any unmatched content - from _before_ this function
 * was called - i.e. the returned buffer is tentative for use only if we
 * accept the part returned here.
 *
 * Used for both symbols and symbolic constants.
 */
static inline uint64_t flatcc_json_parser_symbol_part(const char *buf, const char *end)
{
    size_t n = (size_t)(end - buf);

#if FLATCC_ALLOW_UNALIGNED_ACCESS
    if (n >= 8) {
        return be64toh(*(uint64_t *)buf);
    }
#endif
    return flatcc_json_parser_symbol_part_ext(buf, end);
}

/* Don't allow space in dot notation neither inside nor outside strings. */
static inline const char *flatcc_json_parser_match_scope(flatcc_json_parser_t *ctx, const char *buf, const char *end, int pos)
{
    const char *mark = buf;

    (void)ctx;

    if (end - buf <= pos) {
        return mark;
    }
    if (buf[pos] != '.') {
        return mark;
    }
    return buf + pos + 1;
}

const char *flatcc_json_parser_match_constant(flatcc_json_parser_t *ctx, const char *buf, const char *end, int pos, int *more);

/* We allow '.' in unquoted symbols, but not at the start or end. */
static inline const char *flatcc_json_parser_symbol_end(flatcc_json_parser_t *ctx, const char *buf, const char *end)
{
    char c, clast = 0;


#if FLATCC_JSON_PARSE_ALLOW_UNQUOTED
    if (ctx->unquoted) {
        while (buf != end && *buf > 0x20) {
            clast = c = *buf;
            if (c == '_' || c == '.' || (c & 0x80) || (c >= '0' && c <= '9')) {
                ++buf;
                continue;
            }
            /* Lower case. */
            c |= 0x20;
            if (c >= 'a' && c <= 'z') {
                ++buf;
                continue;
            }
            break;
        }
        if (clast == '.') {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_unexpected_character);
        }
    } else {
#else
    {
#endif
        while (buf != end && *buf != '\"') {
            if (*buf == '\\') {
                if (end - buf < 2) {
                    break;
                }
                ++buf;
            }
            ++buf;
        }
        if (buf == end || *buf != '\"') {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_unterminated_string);
        }
        ++buf;
    }
    return buf;
}

static inline const char *flatcc_json_parser_constant_start(flatcc_json_parser_t *ctx, const char *buf, const char *end)
{
    buf = flatcc_json_parser_symbol_start(ctx, buf, end);
#if FLATCC_JSON_PARSE_ALLOW_UNQUOTED
    if (!ctx->unquoted) {
#else
    {
#endif
        buf = flatcc_json_parser_space(ctx, buf, end);
    }
    return buf;
}

static inline const char *flatcc_json_parser_object_start(flatcc_json_parser_t *ctx, const char *buf, const char *end, int *more)
{
    if (buf == end || *buf != '{') {
        *more = 0;
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_expected_object);
    }
    buf = flatcc_json_parser_space(ctx, buf + 1, end);
    if (buf != end && *buf == '}') {
        *more = 0;
        return flatcc_json_parser_space(ctx, buf + 1, end);
    }
    *more = 1;
    return buf;
}

static inline const char *flatcc_json_parser_object_end(flatcc_json_parser_t *ctx, const char *buf,
        const char *end, int *more)
{
    buf = flatcc_json_parser_space(ctx, buf, end);
    if (buf == end) {
        *more = 0;
        return buf;
    }
    if (*buf != ',') {
        *more = 0;
        if (*buf != '}') {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_unbalanced_object);
        } else {
            return flatcc_json_parser_space(ctx, buf + 1, end);
        }
    }
    buf = flatcc_json_parser_space(ctx, buf + 1, end);
    if (buf == end) {
        *more = 0;
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_unbalanced_object);
    }
#if FLATCC_JSON_PARSE_ALLOW_TRAILING_COMMA
    if (*buf == '}') {
        *more = 0;
        return flatcc_json_parser_space(ctx, buf + 1, end);
    }
#endif
    *more = 1;
    return buf;
}

static inline const char *flatcc_json_parser_array_start(flatcc_json_parser_t *ctx, const char *buf, const char *end, int *more)
{
    if (buf == end || *buf != '[') {
        *more = 0;
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_expected_array);
    }
    buf = flatcc_json_parser_space(ctx, buf + 1, end);
    if (buf != end && *buf == ']') {
        *more = 0;
        return flatcc_json_parser_space(ctx, buf + 1, end);
    }
    *more = 1;
    return buf;
}

static inline const char *flatcc_json_parser_array_end(flatcc_json_parser_t *ctx, const char *buf,
        const char *end, int *more)
{
    buf = flatcc_json_parser_space(ctx, buf, end);
    if (buf == end) {
        *more = 0;
        return buf;
    }
    if (*buf != ',') {
        *more = 0;
        if (*buf != ']') {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_unbalanced_array);
        } else {
            return flatcc_json_parser_space(ctx, buf + 1, end);
        }
    }
    buf = flatcc_json_parser_space(ctx, buf + 1, end);
    if (buf == end) {
        *more = 0;
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_unbalanced_array);
    }
#if FLATCC_JSON_PARSE_ALLOW_TRAILING_COMMA
    if (*buf == ']') {
        *more = 0;
        return flatcc_json_parser_space(ctx, buf + 1, end);
    }
#endif
    *more = 1;
    return buf;
}

/*
 * Detects if a symbol terminates at a given `pos` relative to the
 * buffer pointer, or return fast.
 *
 * Failure to match is not an error but a recommendation to try
 * alternative longer suffixes - only if such do not exist will
 * there be an error. If a match was not eventually found,
 * the `flatcc_json_parser_unmatched_symbol` should be called to consume
 * the symbol and generate error messages.
 *
 * If a match was detected, ':' and surrounding space is consumed,
 * or an error is generated.
 */
static inline const char *flatcc_json_parser_match_symbol(flatcc_json_parser_t *ctx, const char *buf,
        const char *end, int pos)
{
    const char *mark = buf;

    if (end - buf <= pos) {
        return mark;
    }
#if FLATCC_JSON_PARSE_ALLOW_UNQUOTED
    if (ctx->unquoted) {
        if (buf[pos] > 0x20 && buf[pos] != ':') {
            return mark;
        }
        buf += pos;
        ctx->unquoted = 0;
    } else {
#else
    {
#endif
        if (buf[pos] != '\"') {
            return mark;
        }
        buf += pos + 1;
    }
    buf = flatcc_json_parser_space(ctx, buf, end);
    if (buf != end && *buf == ':') {
        ++buf;
        return flatcc_json_parser_space(ctx, buf, end);
    }
    return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_expected_colon);
}

static inline const char *flatcc_json_parser_match_type_suffix(flatcc_json_parser_t *ctx, const char *buf, const char *end, int pos)
{
    if (end - buf <= pos + 5) {
        return buf;
    }
    if (memcmp(buf + pos, "_type", 5)) {
        return buf;
    }
    return flatcc_json_parser_match_symbol(ctx, buf, end, pos + 5);
}

const char *flatcc_json_parser_unmatched_symbol(flatcc_json_parser_t *ctx, const char *buf, const char *end);

static inline const char *flatcc_json_parser_coerce_uint64(
        flatcc_json_parser_t *ctx, const char *buf,
        const char *end, int value_sign, uint64_t value, uint64_t *v)
{
    if (value_sign) {
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_underflow);
    }
    *v = value;
    return buf;
}

static inline const char *flatcc_json_parser_coerce_bool(flatcc_json_parser_t *ctx, const char *buf,
        const char *end, int value_sign, uint64_t value, uint8_t *v)
{
    if (value_sign) {
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_underflow);
    }
    *v = (uint8_t)!!value;
    return buf;
}

#define __flatcc_json_parser_define_coerce_unsigned(type, basetype, uctype) \
static inline const char *flatcc_json_parser_coerce_ ## type(               \
        flatcc_json_parser_t *ctx, const char *buf,                         \
        const char *end, int value_sign, uint64_t value, basetype *v)       \
{                                                                           \
    if (value_sign) {                                                       \
        return flatcc_json_parser_set_error(ctx, buf, end,                  \
                flatcc_json_parser_error_underflow);                        \
    }                                                                       \
    if (value > uctype ## _MAX) {                                           \
        return flatcc_json_parser_set_error(ctx, buf, end,                  \
                flatcc_json_parser_error_overflow);                         \
    }                                                                       \
    *v = (basetype)value;                                                   \
    return buf;                                                             \
}

__flatcc_json_parser_define_coerce_unsigned(uint32, uint32_t, UINT32)
__flatcc_json_parser_define_coerce_unsigned(uint16, uint16_t, UINT16)
__flatcc_json_parser_define_coerce_unsigned(uint8, uint8_t, UINT8)

#define __flatcc_json_parser_define_coerce_signed(type, basetype, uctype)   \
static inline const char *flatcc_json_parser_coerce_ ## type(               \
        flatcc_json_parser_t *ctx, const char *buf,                         \
        const char *end, int value_sign, uint64_t value, basetype *v)       \
{                                                                           \
    if (value_sign) {                                                       \
        if (value > (uint64_t)(uctype ## _MAX) + 1) {                       \
            return flatcc_json_parser_set_error(ctx, buf, end,              \
                    flatcc_json_parser_error_underflow);                    \
        }                                                                   \
        *v = (basetype)-(int64_t)value;                                     \
    } else {                                                                \
        if (value > uctype ## _MAX) {                                       \
            return flatcc_json_parser_set_error(ctx, buf, end,              \
                    flatcc_json_parser_error_overflow);                     \
        }                                                                   \
        *v = (basetype)value;                                               \
    }                                                                       \
    return buf;                                                             \
}

__flatcc_json_parser_define_coerce_signed(int64, int64_t, INT64)
__flatcc_json_parser_define_coerce_signed(int32, int32_t, INT32)
__flatcc_json_parser_define_coerce_signed(int16, int16_t, INT16)
__flatcc_json_parser_define_coerce_signed(int8, int8_t, INT8)

static inline const char *flatcc_json_parser_coerce_float(
        flatcc_json_parser_t *ctx, const char *buf,
        const char *end, int value_sign, uint64_t value, float *v)
{
    (void)ctx;
    (void)end;

    *v = value_sign ? -(float)value : (float)value;
    return buf;
}

static inline const char *flatcc_json_parser_coerce_double(
        flatcc_json_parser_t *ctx, const char *buf,
        const char *end, int value_sign, uint64_t value, double *v)
{
    (void)ctx;
    (void)end;

    *v = value_sign ? -(double)value : (double)value;
    return buf;
}

const char *flatcc_json_parser_double(flatcc_json_parser_t *ctx, const char *buf, const char *end, double *v);

const char *flatcc_json_parser_float(flatcc_json_parser_t *ctx, const char *buf, const char *end, float *v);

/*
 * If the buffer does not contain a valid start character for a numeric
 * value, the function will return the the input buffer without failure.
 * This makes is possible to try a symbolic parse.
 */
const char *flatcc_json_parser_integer(flatcc_json_parser_t *ctx, const char *buf, const char *end,
        int *value_sign, uint64_t *value);

/* Returns unchanged buffer without error if `null` is not matched. */
static inline const char *flatcc_json_parser_null(const char *buf, const char *end)
{
    if (end - buf >= 4 && memcmp(buf, "null", 4) == 0) {
        return buf + 4;
    }
    return buf;
}

static inline const char *flatcc_json_parser_none(flatcc_json_parser_t *ctx,
        const char *buf, const char *end)
{
    if (end - buf >= 4 && memcmp(buf, "null", 4) == 0) {
        return buf + 4;
    }
    return flatcc_json_parser_set_error(ctx, buf, end,
            flatcc_json_parser_error_union_none_not_null);
}

/*
 * `parsers` is a null terminated array of parsers with at least one
 * valid parser. A numeric literal parser may also be included.
 */
#define __flatcc_json_parser_define_integral_parser(type, basetype)         \
static inline const char *flatcc_json_parser_ ## type(                      \
        flatcc_json_parser_t *ctx,                                          \
        const char *buf, const char *end, basetype *v)                      \
{                                                                           \
    uint64_t value = 0;                                                     \
    int value_sign = 0;                                                     \
    const char *mark = buf;                                                 \
                                                                            \
    *v = 0;                                                                 \
    if (buf == end) {                                                       \
        return buf;                                                         \
    }                                                                       \
    buf = flatcc_json_parser_integer(ctx, buf, end, &value_sign, &value);   \
    if (buf != mark) {                                                      \
        return flatcc_json_parser_coerce_ ## type(ctx,                      \
                buf, end, value_sign, value, v);                            \
    }                                                                       \
    return buf;                                                             \
}

__flatcc_json_parser_define_integral_parser(uint64, uint64_t)
__flatcc_json_parser_define_integral_parser(uint32, uint32_t)
__flatcc_json_parser_define_integral_parser(uint16, uint16_t)
__flatcc_json_parser_define_integral_parser(uint8, uint8_t)
__flatcc_json_parser_define_integral_parser(int64, int64_t)
__flatcc_json_parser_define_integral_parser(int32, int32_t)
__flatcc_json_parser_define_integral_parser(int16, int16_t)
__flatcc_json_parser_define_integral_parser(int8, int8_t)

static inline const char *flatcc_json_parser_bool(flatcc_json_parser_t *ctx, const char *buf, const char *end, uint8_t *v)
{
    const char *k;
    uint8_t tmp;

    k = buf;
    if (end - buf >= 4 && memcmp(buf, "true", 4) == 0) {
        *v = 1;
        return k + 4;
    } else if (end - buf >= 5 && memcmp(buf, "false", 5) == 0) {
        *v = 0;
        return k + 5;
    }
    buf = flatcc_json_parser_uint8(ctx, buf, end, &tmp);
    *v = !!tmp;
    return buf;
}

/*
 * The `parsers` argument is a zero terminated array of parser
 * functions with increasingly general scopes.
 *
 * Symbols can be be or'ed together by listing multiple space separated
 * flags in source being parsed, like `{ x : "Red Blue" }`.
 * Intended for flags, but generally available.
 *
 * `aggregate` means there are more symbols to follow.
 *
 * This function does not return input `buf` value if match was
 * unsuccessful. It will either match or error.
 */
typedef const char *flatcc_json_parser_integral_symbol_f(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, int *value_sign, uint64_t *value, int *aggregate);

/*
 * Raise an error if a syntax like `color: Red Green` is seen unless
 * explicitly permitted. `color: "Red Green"` or `"color": "Red Green"
 * or `color: Red` is permitted if unquoted is permitted but not
 * unquoted list. Googles flatc JSON parser does not allow multiple
 * symbolic values unless quoted, so this is the default.
 */
#if !FLATCC_JSON_PARSE_ALLOW_UNQUOTED || FLATCC_JSON_PARSE_ALLOW_UNQUOTED_LIST
#define __flatcc_json_parser_init_check_unquoted_list()
#define __flatcc_json_parser_check_unquoted_list()
#else
#define __flatcc_json_parser_init_check_unquoted_list() int list_count = 0;
#define __flatcc_json_parser_check_unquoted_list()                          \
    if (list_count++ && ctx->unquoted) {                                    \
        return flatcc_json_parser_set_error(ctx, buf, end,                  \
            flatcc_json_parser_error_unquoted_symbolic_list);               \
    }
#endif

#define __flatcc_json_parser_define_symbolic_integral_parser(type, basetype)\
static const char *flatcc_json_parser_symbolic_ ## type(                    \
        flatcc_json_parser_t *ctx,                                          \
        const char *buf, const char *end,                                   \
        flatcc_json_parser_integral_symbol_f *parsers[],                    \
        basetype *v)                                                        \
{                                                                           \
    flatcc_json_parser_integral_symbol_f **p;                               \
    const char *mark;                                                       \
    basetype tmp = 0;                                                       \
    uint64_t value;                                                         \
    int value_sign, aggregate;                                              \
    __flatcc_json_parser_init_check_unquoted_list()                         \
                                                                            \
    *v = 0;                                                                 \
    buf = flatcc_json_parser_constant_start(ctx, buf, end);                 \
    if (buf == end) {                                                       \
        return buf;                                                         \
    }                                                                       \
    do {                                                                    \
        p = parsers;                                                        \
        do {                                                                \
            /* call parser function */                                      \
            buf = (*p)(ctx, (mark = buf), end,                              \
                    &value_sign, &value, &aggregate);                       \
            if (buf == end) {                                               \
                return buf;                                                 \
            }                                                               \
        } while (buf == mark && *++p);                                      \
        if (mark == buf) {                                                  \
            return flatcc_json_parser_set_error(ctx, buf, end,              \
                    flatcc_json_parser_error_expected_scalar);              \
        }                                                                   \
        __flatcc_json_parser_check_unquoted_list()                          \
        if (end == flatcc_json_parser_coerce_ ## type(ctx,                  \
                    buf, end, value_sign, value, &tmp)) {                   \
            return end;                                                     \
        }                                                                   \
        /*                                                                  \
         * `+=`, not `|=` because we also coerce to float and double,       \
         * and because we need to handle signed values. This may give       \
         * unexpected results with duplicate flags.                         \
         */                                                                 \
        *v += tmp;                                                          \
    } while (aggregate);                                                    \
    return buf;                                                             \
}

__flatcc_json_parser_define_symbolic_integral_parser(uint64, uint64_t)
__flatcc_json_parser_define_symbolic_integral_parser(uint32, uint32_t)
__flatcc_json_parser_define_symbolic_integral_parser(uint16, uint16_t)
__flatcc_json_parser_define_symbolic_integral_parser(uint8, uint8_t)
__flatcc_json_parser_define_symbolic_integral_parser(int64, int64_t)
__flatcc_json_parser_define_symbolic_integral_parser(int32, int32_t)
__flatcc_json_parser_define_symbolic_integral_parser(int16, int16_t)
__flatcc_json_parser_define_symbolic_integral_parser(int8, int8_t)

__flatcc_json_parser_define_symbolic_integral_parser(bool, uint8_t)

/* We still parse integral values, but coerce to float or double. */
__flatcc_json_parser_define_symbolic_integral_parser(float, float)
__flatcc_json_parser_define_symbolic_integral_parser(double, double)

/* Parse vector as a base64 or base64url encoded string with no spaces permitted. */
const char *flatcc_json_parser_build_uint8_vector_base64(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, flatcc_builder_ref_t *ref, int urlsafe);

/*
 * This doesn't do anything other than validate and advance past
 * a JSON value which may use unquoted symbols.
 *
 * Upon call it is assumed that leading space has been stripped and that
 * a JSON value is expected (i.e. root, or just after ':' in a
 * container object, or less likely as an array member). Any trailing
 * comma is assumed to belong to the parent context. Returns a parse
 * location stripped from space so container should post call expect
 * ',', '}', or ']', or EOF if the JSON is valid.
 */
const char *flatcc_json_parser_generic_json(flatcc_json_parser_t *ctx, const char *buf, const char *end);

/* Parse a JSON table. */
typedef const char *flatcc_json_parser_table_f(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, flatcc_builder_ref_t *pref);

/* Parses a JSON struct. */
typedef const char *flatcc_json_parser_struct_f(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, flatcc_builder_ref_t *pref);

/* Constructs a table, struct, or string object unless the type is 0 or unknown. */
typedef const char *flatcc_json_parser_union_f(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, uint8_t type, flatcc_builder_ref_t *pref);

typedef int flatcc_json_parser_is_known_type_f(uint8_t type);

/* Called at start by table parsers with at least 1 union. */
const char *flatcc_json_parser_prepare_unions(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, size_t union_total, size_t *handle);

const char *flatcc_json_parser_finalize_unions(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, size_t handle);

const char *flatcc_json_parser_union(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, size_t union_index,
        flatbuffers_voffset_t id, size_t handle,
        flatcc_json_parser_union_f *union_parser);

const char *flatcc_json_parser_union_type(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, size_t union_index,
        flatbuffers_voffset_t id, size_t handle,
        flatcc_json_parser_integral_symbol_f *type_parsers[],
        flatcc_json_parser_union_f *union_parser);

const char *flatcc_json_parser_union_vector(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, size_t union_index,
        flatbuffers_voffset_t id, size_t handle,
        flatcc_json_parser_union_f *union_parser);

const char *flatcc_json_parser_union_type_vector(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, size_t union_index,
        flatbuffers_voffset_t id, size_t handle,
        flatcc_json_parser_integral_symbol_f *type_parsers[],
        flatcc_json_parser_union_f *union_parser,
        flatcc_json_parser_is_known_type_f accept_type);

/*
 * Parses a table as root.
 *
 * Use the flag `flatcc_json_parser_f_with_size` to create a buffer with
 * size prefix.
 *
 * `ctx` may be null or an uninitialized json parser to receive parse results.
 * `builder` must a newly initialized or reset builder object.
 * `buf`, `bufsiz` may be larger than the parsed json if trailing
 * space or zeroes are expected, but they must represent a valid memory buffer.
 * `fid` must be null, or a valid file identifier.
 * `flags` default to 0. See also `flatcc_json_parser_flags`.
 */
int flatcc_json_parser_table_as_root(flatcc_builder_t *B, flatcc_json_parser_t *ctx,
        const char *buf, size_t bufsiz, int flags, const char *fid,
        flatcc_json_parser_table_f *parser);

/*
 * Similar to `flatcc_json_parser_table_as_root` but parses a struct as
 * root.
 */
int flatcc_json_parser_struct_as_root(flatcc_builder_t *B, flatcc_json_parser_t *ctx,
        const char *buf, size_t bufsiz, int flags, const char *fid,
        flatcc_json_parser_struct_f *parser);

#include "flatcc/portable/pdiagnostic_pop.h"

#ifdef __cplusplus
}
#endif

#endif /* FLATCC_JSON_PARSE_H */
