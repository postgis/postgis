#include "flatcc/flatcc_rtconfig.h"
#include "flatcc/flatcc_json_parser.h"
#include "flatcc/flatcc_assert.h"

#define uoffset_t flatbuffers_uoffset_t
#define soffset_t flatbuffers_soffset_t
#define voffset_t flatbuffers_voffset_t
#define utype_t flatbuffers_utype_t

#define uoffset_size sizeof(uoffset_t)
#define soffset_size sizeof(soffset_t)
#define voffset_size sizeof(voffset_t)
#define utype_size sizeof(utype_t)

#define offset_size uoffset_size
#if FLATCC_USE_GRISU3 && !defined(PORTABLE_USE_GRISU3)
#define PORTABLE_USE_GRISU3 1
#endif
#include "flatcc/portable/pparsefp.h"
#include "flatcc/portable/pbase64.h"

#if FLATCC_USE_SSE4_2
#ifdef __SSE4_2__
#define USE_SSE4_2
#endif
#endif

#ifdef USE_SSE4_2
#include <nmmintrin.h>
#define cmpistri(end, haystack, needle, flags)                              \
        if (end - haystack >= 16) do {                                      \
        int i;                                                              \
        __m128i a = _mm_loadu_si128((const __m128i *)(needle));             \
        do {                                                                \
            __m128i b = _mm_loadu_si128((const __m128i *)(haystack));       \
            i = _mm_cmpistri(a, b, flags);                                  \
            haystack += i;                                                  \
        } while (i == 16 && end - haystack >= 16);                          \
        } while(0)
#endif

const char *flatcc_json_parser_error_string(int err)
{
    switch (err) {
#define XX(no, str)                                                         \
    case flatcc_json_parser_error_##no:                                     \
        return str;
        FLATCC_JSON_PARSE_ERROR_MAP(XX)
#undef XX
    default:
        return "unknown";
    }
}

const char *flatcc_json_parser_set_error(flatcc_json_parser_t *ctx, const char *loc, const char *end, int err)
{
    if (!ctx->error) {
        ctx->error = err;
        ctx->pos = (int)(loc - ctx->line_start + 1);
        ctx->error_loc = loc;
    }
    return end;
}

const char *flatcc_json_parser_string_part(flatcc_json_parser_t *ctx, const char *buf, const char *end)
{
/*
 * Disabled because it doesn't catch all control characters, but is
 * useful for performance testing.
 */
#if 0
//#ifdef USE_SSE4_2
    cmpistri(end, buf, "\"\\\0\r\n\t\v\f", _SIDD_POSITIVE_POLARITY);
#else
    /*
     * Testing for signed char >= 0x20 would also capture UTF-8
     * encodings that we could verify, and also invalid encodings like
     * 0xff, but we do not wan't to enforce strict UTF-8.
     */
    while (buf != end && *buf != '\"' && ((unsigned char)*buf) >= 0x20 && *buf != '\\') {
        ++buf;
    }
#endif
    if (buf == end) {
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_unterminated_string);
    }
    if (*buf == '"') {
        return buf;
    }
    if (*buf < 0x20) {
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_invalid_character);
    }
    return buf;
}

const char *flatcc_json_parser_space_ext(flatcc_json_parser_t *ctx, const char *buf, const char *end)
{
again:
#ifdef USE_SSE4_2
    /*
     * We can include line break, but then error reporting suffers and
     * it really makes no big difference.
     */
    //cmpistri(end, buf, "\x20\t\v\f\r\n", _SIDD_NEGATIVE_POLARITY);
    cmpistri(end, buf, "\x20\t\v\f", _SIDD_NEGATIVE_POLARITY);
#else
#if FLATCC_ALLOW_UNALIGNED_ACCESS
    while (end - buf >= 16) {
        if (*buf > 0x20) {
            return buf;
        }
#if FLATCC_JSON_PARSE_WIDE_SPACE
        if (((uint64_t *)buf)[0] != 0x2020202020202020) {
descend:
            if (((uint32_t *)buf)[0] == 0x20202020) {
                buf += 4;
            }
#endif
            if (((uint16_t *)buf)[0] == 0x2020) {
                buf += 2;
            }
            if (*buf == 0x20) {
                ++buf;
            }
            if (*buf > 0x20) {
                return buf;
            }
            break;
#if FLATCC_JSON_PARSE_WIDE_SPACE
        }
        if (((uint64_t *)buf)[1] != 0x2020202020202020) {
            buf += 8;
            goto descend;
        }
        buf += 16;
#endif
    }
#endif
#endif
    while (buf != end && *buf == 0x20) {
        ++buf;
    }
    while (buf != end && *buf <= 0x20) {
        /* Fall through comments needed to silence gcc 7 warnings. */
        switch (*buf) {
        case 0x0d: buf += (end - buf > 1 && buf[1] == 0x0a);
            /* Fall through */
            /* Consume following LF or treating CR as LF. */
        case 0x0a: ++ctx->line; ctx->line_start = ++buf; continue;
        case 0x09: ++buf; continue;
        case 0x20: goto again; /* Don't consume here, sync with power of 2 spaces. */
        default: return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_unexpected_character);
        }
    }
    return buf;
}

static int decode_hex4(const char *buf, uint32_t *result)
{
    uint32_t u, x;
    char c;

    u = 0;
    c = buf[0];
    if (c >= '0' && c <= '9') {
        x = (uint32_t)(c - '0');
        u = x << 12;
    } else {
        /* Lower case. */
        c |= 0x20;
        if (c >= 'a' && c <= 'f') {
            x = (uint32_t)(c - 'a' + 10);
            u |= x << 12;
        } else {
            return -1;
        }
    }
    c = buf[1];
    if (c >= '0' && c <= '9') {
        x = (uint32_t)(c - '0');
        u |= x << 8;
    } else {
        /* Lower case. */
        c |= 0x20;
        if (c >= 'a' && c <= 'f') {
            x = (uint32_t)(c - 'a' + 10);
            u |= x << 8;
        } else {
            return -1;
        }
    }
    c = buf[2];
    if (c >= '0' && c <= '9') {
        x = (uint32_t)(c - '0');
        u |= x << 4;
    } else {
        /* Lower case. */
        c |= 0x20;
        if (c >= 'a' && c <= 'f') {
            x = (uint32_t)(c - 'a' + 10);
            u |= x << 4;
        } else {
            return -1;
        }
    }
    c = buf[3];
    if (c >= '0' && c <= '9') {
        x = (uint32_t)(c - '0');
        u |= x;
    } else {
        /* Lower case. */
        c |= 0x20;
        if (c >= 'a' && c <= 'f') {
            x = (uint32_t)(c - 'a' + 10);
            u |= x;
        } else {
            return -1;
        }
    }
    *result = u;
    return 0;
}

static int decode_unicode_char(uint32_t u, char *code)
{
    if (u <= 0x7f) {
        code[0] = 1;
        code[1] = (char)u;
    } else if (u <= 0x7ff) {
        code[0] = 2;
        code[1] = (char)(0xc0 | (u >> 6));
        code[2] = (char)(0x80 | (u & 0x3f));
    } else if (u <= 0xffff) {
        code[0] = 3;
        code[1] = (char)(0xe0 | (u >> 12));
        code[2] = (char)(0x80 | ((u >> 6) & 0x3f));
        code[3] = (char)(0x80 | (u & 0x3f));
    } else if (u <= 0x10ffff) {
        code[0] = 4;
        code[1] = (char)(0xf0 | (u >> 18));
        code[2] = (char)(0x80 | ((u >> 12) & 0x3f));
        code[3] = (char)(0x80 | ((u >> 6) & 0x3f));
        code[4] = (char)(0x80 | (u & 0x3f));
    } else {
        code[0] = 0;
        return -1;
    }
    return 0;
}

static inline uint32_t combine_utf16_surrogate_pair(uint32_t high, uint32_t low)
{
    return (high - 0xd800) * 0x400 + (low - 0xdc00) + 0x10000;
}

static inline int decode_utf16_surrogate_pair(uint32_t high, uint32_t low, char *code)
{
    return decode_unicode_char(combine_utf16_surrogate_pair(high, low), code);
}


/* 
 * UTF-8 code points can have up to 4 bytes but JSON can only
 * encode up to 3 bytes via the \uXXXX syntax.
 * To handle the range U+10000..U+10FFFF two UTF-16 surrogate
 * pairs must be used. If this is not detected, the pairs
 * survive in the output which is not valid but often tolerated.
 * Emojis generally require such a pair, unless encoded
 * unescaped in UTF-8.
 *
 * If a high surrogate pair is detected and a low surrogate pair
 * follows, the combined sequence is decoded as a 4 byte
 * UTF-8 sequence. Unpaired surrogate halves are decoded as is
 * despite being an invalid UTF-8 value.
 */

const char *flatcc_json_parser_string_escape(flatcc_json_parser_t *ctx, const char *buf, const char *end, flatcc_json_parser_escape_buffer_t code)
{
    char c, v;
    uint32_t u, u2;

    if (end - buf < 2 || buf[0] != '\\') {
        code[0] = 0;
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_invalid_escape);
    }
    switch (buf[1]) {
    case 'x':
        v = 0;
        code[0] = 1;
        if (end - buf < 4) {
            code[0] = 0;
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_invalid_escape);
        }
        c = buf[2];
        if (c >= '0' && c <= '9') {
            v |= (c - '0') << 4;
        } else {
            /* Lower case. */
            c |= 0x20;
            if (c >= 'a' && c <= 'f') {
                v |= (c - 'a' + 10) << 4;
            } else {
                code[0] = 0;
                return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_invalid_escape);
            }
        }
        c = buf[3];
        if (c >= '0' && c <= '9') {
            v |= c - '0';
        } else {
            /* Lower case. */
            c |= 0x20;
            if (c >= 'a' && c <= 'f') {
                v |= c - 'a' + 10;
            } else {
                code[0] = 0;
                return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_invalid_escape);
            }
        }
        code[1] = v;
        return buf + 4;
    case 'u':
        if (end - buf < 6) {
            code[0] = 0;
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_invalid_escape);
        }
        if (decode_hex4(buf + 2, &u)) {
            code[0] = 0;
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_invalid_escape);
        };
        /* If a high UTF-16 surrogate half pair was detected */
        if (u >= 0xd800 && u <= 0xdbff && 
                /* and there is space for a matching low half pair */
                end - buf >= 12 &&
                /* and there is a second escape following immediately */
                buf[6] == '\\' && buf[7] == 'u' &&
                /* and it is valid hex */
                decode_hex4(buf + 8, &u2) == 0 &&
                /* and it is a low UTF-16 surrogate pair */
                u2 >= 0xdc00 && u2 <= 0xdfff) {
            /* then decode the pair into a single 4 byte utf-8 sequence. */
            if (decode_utf16_surrogate_pair(u, u2, code)) {
                code[0] = 0;
                return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_invalid_escape);
            }
            return buf + 12;
            /*  
             *  Otherwise decode unmatched surrogate pairs as is any
             *  other UTF-8. Some systems might depend on these surviving.
             *  Leave ignored errors for the next parse step.
             */
        }
        decode_unicode_char(u, code);
        return buf + 6;
    case 't':
        code[0] = 1;
        code[1] = '\t';
        return buf + 2;
    case 'n':
        code[0] = 1;
        code[1] = '\n';
        return buf + 2;
    case 'r':
        code[0] = 1;
        code[1] = '\r';
        return buf + 2;
    case 'b':
        code[0] = 1;
        code[1] = '\b';
        return buf + 2;
    case 'f':
        code[0] = 1;
        code[1] = '\f';
        return buf + 2;
    case '\"':
        code[0] = 1;
        code[1] = '\"';
        return buf + 2;
    case '\\':
        code[0] = 1;
        code[1] = '\\';
        return buf + 2;
    case '/':
        code[0] = 1;
        code[1] = '/';
        return buf + 2;
    default:
        code[0] = 0;
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_invalid_escape);
    }
}

/* Only applies to unquoted constants during generic parsring, otherwise it is skipped as a string. */
const char *flatcc_json_parser_skip_constant(flatcc_json_parser_t *ctx, const char *buf, const char *end)
{
    char c;
    const char *k;

    while (buf != end) {
        c = *buf;
        if ((c & 0x80) || (c == '_') || (c >= '0' && c <= '9') || c == '.') {
            ++buf;
            continue;
        }
        /* Upper case. */
        c |= 0x20;
        if (c >= 'a' && c <= 'z') {
            ++buf;
            continue;
        }
        buf = flatcc_json_parser_space(ctx, (k = buf), end);
        if (buf == k) {
            return buf;
        }
    }
    return buf;
}

const char *flatcc_json_parser_match_constant(flatcc_json_parser_t *ctx, const char *buf, const char *end, int pos, int *more)
{
    const char *mark = buf, *k = buf + pos;

    if (end - buf <= pos) {
        *more = 0;
        return buf;
    }
#if FLATCC_JSON_PARSE_ALLOW_UNQUOTED
    if (ctx->unquoted) {
        buf = flatcc_json_parser_space(ctx, k, end);
        if (buf == end) {
            /*
             * We cannot make a decision on more.
             * Just return end and let parser handle sync point in
             * case it is able to resume parse later on.
             * For the same reason we do not lower ctx->unquoted.
             */
            *more = 0;
            return buf;
        }
        if (buf != k) {
            char c = *buf;
            /*
             * Space was seen - and thus we have a valid match.
             * If the next char is an identifier start symbol
             * we raise the more flag to support syntax like:
             *
             *     `flags: Hungry Sleepy Awake, ...`
             */
            if (c == '_' || (c & 0x80)) {
                *more = 1;
                return buf;
            }
            c |= 0x20;
            if (c >= 'a' && c <= 'z') {
                *more = 1;
                return buf;
            }
        }
        /*
         * Space was not seen, so the match is only valid if followed
         * by a JSON separator symbol, and there cannot be more values
         * following so `more` is lowered.
         */
        *more = 0;
        if (*buf == ',' || *buf == '}' || *buf == ']') {
            return buf;
        }
        return mark;
    }
#endif
    buf = k;
    if (*buf == 0x20) {
        ++buf;
        while (buf != end && *buf == 0x20) {
            ++buf;
        }
        if (buf == end) {
            *more = 0;
            return buf;
        }
        /* We accept untrimmed space like "  Green  Blue  ". */
        if (*buf != '\"') {
            *more = 1;
            return buf;
        }
    }
    switch (*buf) {
    case '\\':
        *more = 0;
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_invalid_escape);
    case '\"':
        buf = flatcc_json_parser_space(ctx, buf + 1, 0);
        *more = 0;
        return buf;
    }
    *more = 0;
    return mark;
}

const char *flatcc_json_parser_unmatched_symbol(flatcc_json_parser_t *ctx, const char *buf, const char *end)
{
    if (ctx->flags & flatcc_json_parser_f_skip_unknown) {
        buf = flatcc_json_parser_symbol_end(ctx, buf, end);
        buf = flatcc_json_parser_space(ctx, buf, end);
        if (buf != end && *buf == ':') {
            ++buf;
            buf = flatcc_json_parser_space(ctx, buf, end);
        } else {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_expected_colon);
        }
        return flatcc_json_parser_generic_json(ctx, buf, end);
    } else {
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_unknown_symbol);
    }
}

static const char *__flatcc_json_parser_number(flatcc_json_parser_t *ctx, const char *buf, const char *end)
{
    if (buf == end) {
        return buf;
    }
    if (*buf == '-') {
        ++buf;
        if (buf == end) {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_invalid_numeric);
        }
    }
    if (*buf == '0') {
        ++buf;
    } else {
        if (*buf < '1' || *buf > '9') {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_invalid_numeric);
        }
        ++buf;
        while (buf != end && *buf >= '0' && *buf <= '9') {
            ++buf;
        }
    }
    if (buf != end) {
        if (*buf == '.') {
            ++buf;
            if (*buf < '0' || *buf > '9') {
                return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_invalid_numeric);
            }
            ++buf;
            while (buf != end && *buf >= '0' && *buf <= '9') {
                ++buf;
            }
        }
    }
    if (buf != end && (*buf == 'e' || *buf == 'E')) {
        ++buf;
        if (buf == end) {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_invalid_numeric);
        }
        if (*buf == '+' || *buf == '-') {
            ++buf;
        }
        if (buf == end || *buf < '0' || *buf > '9') {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_invalid_numeric);
        }
        ++buf;
        while (buf != end && *buf >= '0' && *buf <= '9') {
            ++buf;
        }
    }

    /*
     * For strtod termination we must ensure the tail is not valid
     * including non-json exponent types. The simplest approach is
     * to accept anything that could be valid json successor
     * characters and reject end of buffer since we expect a closing
     * '}'.
     *
     * The ',' is actually not safe if strtod uses a non-POSIX locale.
     */
    if (buf != end) {
        switch (*buf) {
        case ',':
        case ':':
        case ']':
        case '}':
        case ' ':
        case '\r':
        case '\t':
        case '\n':
        case '\v':
            return buf;
        }
    }
    return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_invalid_numeric);
}

const char *flatcc_json_parser_double(flatcc_json_parser_t *ctx, const char *buf, const char *end, double *v)
{
    const char *next, *k;

    *v = 0.0;
    if (buf == end) {
        return buf;
    }
    k = buf;
    if (*buf == '-') ++k;
    if (end - k > 1 && (k[0] == '.' || (k[0] == '0' && k[1] == '0'))) {
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_invalid_numeric);
    }
    next = parse_double(buf, (size_t)(end - buf), v);
    if (next == 0 || next == buf) {
        if (parse_double_isinf(*v)) {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_overflow);
        }
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_invalid_numeric);
    }
    return next;
}

const char *flatcc_json_parser_float(flatcc_json_parser_t *ctx, const char *buf, const char *end, float *v)
{
    const char *next, *k;

    *v = 0.0;
    if (buf == end) {
        return buf;
    }
    k = buf;
    if (*buf == '-') ++k;
    if (end - k > 1 && (k[0] == '.' || (k[0] == '0' && k[1] == '0'))) {
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_invalid_numeric);
    }
    next = parse_float(buf, (size_t)(end - buf), v);
    if (next == 0 || next == buf) {
        if (parse_float_isinf(*v)) {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_overflow);
        }
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_invalid_numeric);
    }
    return next;
}

const char *flatcc_json_parser_generic_json(flatcc_json_parser_t *ctx, const char *buf, const char *end)
{
    char stack[FLATCC_JSON_PARSE_GENERIC_MAX_NEST];
    char *sp, *spend;
    const char *k;
    flatcc_json_parser_escape_buffer_t code;
    int more = 0;

    sp = stack;
    spend = sp + FLATCC_JSON_PARSE_GENERIC_MAX_NEST;

again:
    if (buf == end) {
        return buf;
    }
    if (sp != stack && sp[-1] == '}') {
        /* Inside an object, about to read field name. */
        buf = flatcc_json_parser_symbol_start(ctx, buf, end);
        buf = flatcc_json_parser_symbol_end(ctx, buf, end);
        buf = flatcc_json_parser_space(ctx, buf, end);
        if (buf == end) {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_unbalanced_object);
        }
        if (*buf != ':') {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_expected_colon);
        }
        buf = flatcc_json_parser_space(ctx, buf + 1, end);
    }
    switch (*buf) {
    case '\"':
        buf = flatcc_json_parser_string_start(ctx, buf, end);
        while (buf != end && *buf != '\"') {
            buf = flatcc_json_parser_string_part(ctx, buf, end);
            if (buf != end && *buf == '\"') {
                break;
            }
            buf = flatcc_json_parser_string_escape(ctx, buf, end, code);
        }
        buf = flatcc_json_parser_string_end(ctx, buf, end);
        break;
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        buf = __flatcc_json_parser_number(ctx, buf, end);
        break;
#if !FLATCC_JSON_PARSE_ALLOW_UNQUOTED
    case 't': case 'f':
        {
            uint8_t v;
            buf = flatcc_json_parser_bool(ctx, (k = buf), end, &v);
            if (k == buf) {
                return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_unexpected_character);
            }
        }
        break;
    case 'n':
        buf = flatcc_json_parser_null((k = buf), end);
        if (k == buf) {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_unexpected_character);
        }
        break;
#endif
    case '[':
        if (sp == spend) {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_deep_nesting);
        }
        *sp++ = ']';
        buf = flatcc_json_parser_space(ctx, buf + 1, end);
        if (buf != end && *buf == ']') {
            break;
        }
        goto again;
    case '{':
        if (sp == spend) {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_deep_nesting);
        }
        *sp++ = '}';
        buf = flatcc_json_parser_space(ctx, buf + 1, end);
        if (buf != end && *buf == '}') {
            break;
        }
        goto again;

    default:
#if FLATCC_JSON_PARSE_ALLOW_UNQUOTED
        buf = flatcc_json_parser_skip_constant(ctx, (k = buf), end);
        if (k == buf) {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_unexpected_character);
        }
        break;
#else
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_unexpected_character);
#endif
    }
    while (buf != end && sp != stack) {
        --sp;
        if (*sp == ']') {
            buf = flatcc_json_parser_array_end(ctx, buf, end, &more);
        } else {
            buf = flatcc_json_parser_object_end(ctx, buf, end, &more);
        }
        if (more) {
            ++sp;
            goto again;
        }
    }
    if (buf == end && sp != stack) {
        return flatcc_json_parser_set_error(ctx, buf, end, sp[-1] == ']' ?
                flatcc_json_parser_error_unbalanced_array :
                flatcc_json_parser_error_unbalanced_object);
    }
    /* Any ',', ']', or '}' belongs to parent context. */
    return buf;
}

const char *flatcc_json_parser_integer(flatcc_json_parser_t *ctx, const char *buf, const char *end,
        int *value_sign, uint64_t *value)
{
    uint64_t x0, x = 0;
    const char *k;

    if (buf == end) {
        return buf;
    }
    k = buf;
    *value_sign = *buf == '-';
    buf += *value_sign;
    while (buf != end && *buf >= '0' && *buf <= '9') {
        x0 = x;
        x = x * 10 + (uint64_t)(*buf - '0');
        if (x0 > x) {
            return flatcc_json_parser_set_error(ctx, buf, end, value_sign ?
                    flatcc_json_parser_error_underflow : flatcc_json_parser_error_overflow);
        }
        ++buf;
    }
    if (buf == k) {
        /* Give up, but don't fail the parse just yet, it might be a valid symbol. */
        return buf;
    }
    if (buf != end && (*buf == 'e' || *buf == 'E' || *buf == '.')) {
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_float_unexpected);
    }
    *value = x;
    return buf;
}

/* Array Creation - depends on flatcc builder. */

const char *flatcc_json_parser_build_uint8_vector_base64(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, flatcc_builder_ref_t *ref, int urlsafe)
{
    const char *mark;
    uint8_t *pval;
    size_t max_len;
    size_t decoded_len, src_len;
    int mode;
    int ret;

    mode = urlsafe ? base64_mode_url : base64_mode_rfc4648;
    buf = flatcc_json_parser_string_start(ctx, buf, end);
    buf = flatcc_json_parser_string_part(ctx, (mark = buf), end);
    if (buf == end || *buf != '\"') {
        goto base64_failed;
    }
    max_len = base64_decoded_size((size_t)(buf - mark));
    if (flatcc_builder_start_vector(ctx->ctx, 1, 1, FLATBUFFERS_COUNT_MAX((utype_size)))) {
        goto failed;
    }
    if (!(pval = flatcc_builder_extend_vector(ctx->ctx, max_len))) {
        goto failed;
    }
    src_len = (size_t)(buf - mark);
    decoded_len = max_len;
    if ((ret = base64_decode(pval, (const uint8_t *)mark, &decoded_len, &src_len, mode))) {
        buf = mark + src_len;
        goto base64_failed;
    }
    if (src_len != (size_t)(buf - mark)) {
        buf = mark + src_len;
        goto base64_failed;
    }
    if (decoded_len < max_len) {
        if (flatcc_builder_truncate_vector(ctx->ctx, max_len - decoded_len)) {
            goto failed;
        }
    }
    if (!(*ref = flatcc_builder_end_vector(ctx->ctx))) {
        goto failed;
    }
    return flatcc_json_parser_string_end(ctx, buf, end);

failed:
    *ref = 0;
    return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_runtime);

base64_failed:
    *ref = 0;
    return flatcc_json_parser_set_error(ctx, buf, end,
            urlsafe ? flatcc_json_parser_error_base64url : flatcc_json_parser_error_base64);
}

const char *flatcc_json_parser_char_array(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, char *s, size_t n)
{
    flatcc_json_parser_escape_buffer_t code;
    const char *mark;
    size_t k = 0;

    buf = flatcc_json_parser_string_start(ctx, buf, end);
    if (buf != end)
    while (*buf != '\"') {
        buf = flatcc_json_parser_string_part(ctx, (mark = buf), end);
        if (buf == end) return end;
        k = (size_t)(buf - mark);
        if (k > n) {
            if (!(ctx->flags & flatcc_json_parser_f_skip_array_overflow)) {
                return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_array_overflow);
            } 
            k = n; /* Might truncate UTF-8. */
        }
        memcpy(s, mark, k);
        s += k;
        n -= k;
        if (*buf == '\"') break;
        buf = flatcc_json_parser_string_escape(ctx, buf, end, code);
        if (buf == end) return end;
        k = (size_t)code[0];
        mark = code + 1;
        if (k > n) {
            if (!(ctx->flags & flatcc_json_parser_f_skip_array_overflow)) {
                return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_array_overflow);
            } 
            k = n; /* Might truncate UTF-8. */
        }
        memcpy(s, mark, k);
        s += k;
        n -= k;
    }
    if (n != 0) {
        if (ctx->flags & flatcc_json_parser_f_reject_array_underflow) {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_array_underflow);
        }
        memset(s, 0, n - k);
    }
    return flatcc_json_parser_string_end(ctx, buf, end);
}


/* String Creation - depends on flatcc builder. */

const char *flatcc_json_parser_build_string(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, flatcc_builder_ref_t *ref)
{
    flatcc_json_parser_escape_buffer_t code;
    const char *mark;

    buf = flatcc_json_parser_string_start(ctx, buf, end);
    buf = flatcc_json_parser_string_part(ctx, (mark = buf), end);
    if (buf != end && *buf == '\"') {
        *ref = flatcc_builder_create_string(ctx->ctx, mark, (size_t)(buf - mark));
    } else {
        if (flatcc_builder_start_string(ctx->ctx) ||
                0 == flatcc_builder_append_string(ctx->ctx, mark, (size_t)(buf - mark))) goto failed;
        while (buf != end && *buf != '\"') {
            buf = flatcc_json_parser_string_escape(ctx, buf, end, code);
            if (0 == flatcc_builder_append_string(ctx->ctx, code + 1, (size_t)code[0])) goto failed;
            if (end != (buf = flatcc_json_parser_string_part(ctx, (mark = buf), end))) {
                if (0 == flatcc_builder_append_string(ctx->ctx, mark, (size_t)(buf - mark))) goto failed;
            }
        }
        *ref = flatcc_builder_end_string(ctx->ctx);
    }
    return flatcc_json_parser_string_end(ctx, buf, end);

failed:
    *ref = 0;
    return buf;
}

/* UNIONS */

/*
 * Unions are difficult to parse because the type field may appear after
 * the union table and because having two fields opens up for many more
 * possible error scenarios. We must store each union of a table
 * temporarily - this cannot be in the generated table parser function
 * because there could be many unions (about 2^15 with default voffsets)
 * although usually there will be only a few. We can also not store the
 * data encoded in the existing table buffer in builder because we may
 * have to remove it due to schema forwarding and removing it messes up
 * the table layout. We also cannot naively allocate it dynamically for
 * performance reasons. Instead we place the temporary union data in a
 * separate frame from the table buffer, but on a similar stack. This is
 * called the user stack and we manage one frame per table that is known
 * to contain unions.
 *
 * Even the temporary structures in place we still cannot parse a union
 * before we know its type. Due to JSON typically sorting fields
 * alphabetically in various pretty printers, we are likely to receive
 * the type late with (`<union_name>_type` following `<union_name>`.
 * To deal with this we store a backtracking pointer and parses the
 * table generically in a first pass and reparse the table once the type
 * is known. This can happen recursively with nested tables containing
 * unions which is why we need to have a stack frame.
 *
 * If the type field is stored first we just store the type in the
 * custom frame and immediately parses the table with the right type
 * once we see it. The parse will be much faster and we can strongly
 * recommend that flatbuffer serializers do this, but we cannot require
 * it.
 *
 * The actual overhead of dealing with the custom stack frame is fairly
 * cheap once we get past the first custom stack allocation.
 *
 * We cannot update the builder before both the table and table type
 * has been parsed because the the type might have to be ingored due
 * to schema forwarding. Therefore the union type must be cached or
 * reread. This happens trivially be calling the union parser with the
 * type as argument, but it is important to be aware of before
 * refactoring the code.
 *
 * The user frame is created at table start and remains valid until
 * table exit, but we cannot assume the pointers to the frame remain
 * valid. Specifically we cannot use frame pointers after calling
 * the union parser. This means the union type must be cached or reread
 * so it can be added to the table. Because the type is passed to
 * the union parser this caching happens automatically but it is still
 * important to be aware that it is required.
 *
 * The frame reserves temporary information for all unions the table
 * holds, enumerated 0 <= `union_index` < `union_total`
 * where the `union_total` is fixed type specific number.
 *
 * The `type_present` is needed because union types range from 0..255
 * and we need an extra bit do distinguish not present from union type
 * `NONE = 0`.
 */

typedef struct {
    const char *backtrace;
    const char *line_start;
    int line;
    uint8_t type_present;
    uint8_t type;
    /* Union vectors: */
    uoffset_t count;
    size_t h_types;
} __flatcc_json_parser_union_entry_t;

typedef struct {
    size_t union_total;
    size_t union_count;
    __flatcc_json_parser_union_entry_t unions[1];
} __flatcc_json_parser_union_frame_t;

const char *flatcc_json_parser_prepare_unions(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, size_t union_total, size_t *handle)
{
    __flatcc_json_parser_union_frame_t *f;

    if (!(*handle = flatcc_builder_enter_user_frame(ctx->ctx,
                sizeof(__flatcc_json_parser_union_frame_t) + (union_total - 1) *
                sizeof(__flatcc_json_parser_union_entry_t)))) {
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_runtime);
    }
    f = flatcc_builder_get_user_frame_ptr(ctx->ctx, *handle);
    /* Frames have zeroed memory. */
    f->union_total = union_total;
    return buf;
}

const char *flatcc_json_parser_finalize_unions(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, size_t handle)
{
    __flatcc_json_parser_union_frame_t *f = flatcc_builder_get_user_frame_ptr(ctx->ctx, handle);

    if (f->union_count) {
        buf = flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_union_incomplete);
    }
    flatcc_builder_exit_user_frame_at(ctx->ctx, handle);
    return buf;
}

const char *flatcc_json_parser_union(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, size_t union_index,
        flatbuffers_voffset_t id, size_t handle, flatcc_json_parser_union_f *union_parser)
{
    __flatcc_json_parser_union_frame_t *f = flatcc_builder_get_user_frame_ptr(ctx->ctx, handle);
    __flatcc_json_parser_union_entry_t *e = &f->unions[union_index];
    flatcc_builder_union_ref_t uref;

    if (e->backtrace) {
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_duplicate);
    }
    if (!e->type_present) {
        /* If we supported table: null, we should not count it, but we don't. */
        ++f->union_count;
        e->line = ctx->line;
        e->line_start = ctx->line_start;
        buf = flatcc_json_parser_generic_json(ctx, (e->backtrace = buf), end);
    } else {
        uref.type = e->type;
        if (e->type == 0) {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_union_none_present);
        }
        --f->union_count;
        buf = union_parser(ctx, buf, end, e->type, &uref.value);
        if (buf != end) {
            if (flatcc_builder_table_add_union(ctx->ctx, id, uref)) {
                return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_duplicate);
            }
        }
    }
    return buf;
}

const char *flatcc_json_parser_union_type(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, size_t union_index, flatbuffers_voffset_t id,
        size_t handle,
        flatcc_json_parser_integral_symbol_f *type_parsers[],
        flatcc_json_parser_union_f *union_parser)
{
    __flatcc_json_parser_union_frame_t *f = flatcc_builder_get_user_frame_ptr(ctx->ctx, handle);
    __flatcc_json_parser_union_entry_t *e = f->unions + union_index;

    flatcc_builder_union_ref_t uref;
    const char *mark;
    int line;
    const char *line_start;

    if (e->type_present) {
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_duplicate);
    }
    e->type_present = 1;
    buf = flatcc_json_parser_uint8(ctx, (mark = buf), end, &e->type);
    if (mark == buf) {
        buf = flatcc_json_parser_symbolic_uint8(ctx, buf, end, type_parsers, &e->type);
    }
    /* Only count the union if the type is not NONE. */
    if (e->backtrace == 0) {
        f->union_count += e->type != 0;
        return buf;
    }
    FLATCC_ASSERT(f->union_count);
    --f->union_count;
    /*
     * IMPORTANT: we cannot access any value in the frame or entry
     * pointer after calling union parse because it might cause the
     * stack to reallocate. We should read the frame pointer again if
     * needed - we don't but remember it if refactoring code.
     *
     * IMPORTANT 2: Do not assign buf here. We are backtracking.
     */
    line = ctx->line;
    line_start = ctx->line_start;
    ctx->line = e->line;
    ctx->line_start = e->line_start;
    uref.type = e->type;
    if (end == union_parser(ctx, e->backtrace, end, e->type, &uref.value)) {
        return end;
    }
    if (flatcc_builder_table_add_union(ctx->ctx, id, uref)) {
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_duplicate);
    }
    ctx->line = line;
    ctx->line_start = line_start;
    return buf;
}

static const char *_parse_union_vector(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, size_t h_types, uoffset_t count,
        flatbuffers_voffset_t id, flatcc_json_parser_union_f *union_parser)
{
    flatcc_builder_ref_t ref = 0, *pref;
    utype_t *types;
    int more;
    size_t i;

    if (flatcc_builder_start_offset_vector(ctx->ctx)) goto failed;
    buf = flatcc_json_parser_array_start(ctx, buf, end, &more);
    i = 0;
    while (more) {
        if (i == count) {
            return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_union_vector_length);
        }
        /* Frame must be restored between calls to table parser. */
        types = flatcc_builder_get_user_frame_ptr(ctx->ctx, h_types);
        buf = union_parser(ctx, buf, end, types[i], &ref);
        if (buf == end) {
            return buf;
        }
        if (!(pref = flatcc_builder_extend_offset_vector(ctx->ctx, 1))) goto failed;
        *pref = ref;
        buf = flatcc_json_parser_array_end(ctx, buf, end, &more);
        ++i;
    }
    if (i != count) {
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_union_vector_length);
    }
    /* Frame must be restored between calls to table parser. */
    types = flatcc_builder_get_user_frame_ptr(ctx->ctx, h_types);
    if (!(ref = flatcc_builder_end_offset_vector_for_unions(ctx->ctx, types))) goto failed;
    if (!(pref = flatcc_builder_table_add_offset(ctx->ctx, id))) goto failed;
    *pref = ref;
    return buf;
failed:
    return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_runtime);
}

const char *flatcc_json_parser_union_vector(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, size_t union_index,
        flatbuffers_voffset_t id, size_t handle, flatcc_json_parser_union_f *union_parser)
{
    __flatcc_json_parser_union_frame_t *f = flatcc_builder_get_user_frame_ptr(ctx->ctx, handle);
    __flatcc_json_parser_union_entry_t *e = f->unions + union_index;

    if (e->backtrace) {
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_duplicate);
    }
    if (!e->type_present) {
        ++f->union_count;
        e->line = ctx->line;
        e->line_start = ctx->line_start;
        buf = flatcc_json_parser_generic_json(ctx, (e->backtrace = buf), end);
    } else {
        --f->union_count;
        buf = _parse_union_vector(ctx, buf, end, e->h_types, e->count, id, union_parser);
    }
    return buf;
}

const char *flatcc_json_parser_union_type_vector(flatcc_json_parser_t *ctx,
        const char *buf, const char *end, size_t union_index, flatbuffers_voffset_t id,
        size_t handle,
        flatcc_json_parser_integral_symbol_f *type_parsers[],
        flatcc_json_parser_union_f *union_parser,
        flatcc_json_parser_is_known_type_f accept_type)
{
    __flatcc_json_parser_union_frame_t *f = flatcc_builder_get_user_frame_ptr(ctx->ctx, handle);
    __flatcc_json_parser_union_entry_t *e = f->unions + union_index;

    const char *mark;
    int line;
    const char *line_start;
    int more;
    utype_t val;
    void *pval;
    flatcc_builder_ref_t ref, *pref;
    utype_t *types;
    size_t size;
    size_t h_types;
    uoffset_t count;

#if FLATBUFFERS_UTYPE_MAX != UINT8_MAX
#error "Update union vector parser to support current union type definition."
#endif

    if (e->type_present) {
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_duplicate);
    }
    e->type_present = 1;
    if (flatcc_builder_start_vector(ctx->ctx, 1, 1, FLATBUFFERS_COUNT_MAX((utype_size)))) goto failed;
    buf = flatcc_json_parser_array_start(ctx, buf, end, &more);
    while (more) {
        if (!(pval = flatcc_builder_extend_vector(ctx->ctx, 1))) goto failed;
        buf = flatcc_json_parser_uint8(ctx, (mark = buf), end, &val);
        if (mark == buf) {
            buf = flatcc_json_parser_symbolic_uint8(ctx, (mark = buf), end, type_parsers, &val);
            if (buf == mark || buf == end) goto failed;
        }
        /* Parse unknown types as NONE */
        if (!accept_type(val)) {
            if (!(ctx->flags & flatcc_json_parser_f_skip_unknown)) {
                return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_unknown_union);
            }
            val = 0;
        }
        flatbuffers_uint8_write_to_pe(pval, val);
        buf = flatcc_json_parser_array_end(ctx, buf, end, &more);
    }
    count = (uoffset_t)flatcc_builder_vector_count(ctx->ctx);
    e->count = count;
    size = count * utype_size;
    /* Store type vector so it is accessible to the table vector parser.  */
    h_types = flatcc_builder_enter_user_frame(ctx->ctx, size);
    types = flatcc_builder_get_user_frame_ptr(ctx->ctx, h_types);
    memcpy(types, flatcc_builder_vector_edit(ctx->ctx), size);
    if (!((ref = flatcc_builder_end_vector(ctx->ctx)))) goto failed;
    if (!(pref = flatcc_builder_table_add_offset(ctx->ctx, id - 1))) goto failed;
    *pref = ref;

    /* Restore union frame after possible invalidation due to types frame allocation. */
    f = flatcc_builder_get_user_frame_ptr(ctx->ctx, handle);
    e = f->unions + union_index;

    e->h_types = h_types;
    if (e->backtrace == 0) {
        ++f->union_count;
        return buf;
    }
    FLATCC_ASSERT(f->union_count);
    --f->union_count;
    line = ctx->line;
    line_start = ctx->line_start;
    ctx->line = e->line;
    ctx->line_start = e->line_start;
    /* We must not assign buf here because we are backtracking. */
    if (end == _parse_union_vector(ctx, e->backtrace, end, h_types, count, id, union_parser)) return end;
    /*
     * NOTE: We do not need the user frame anymore, but if we did, it
     * would have to be restored from its handle due to the above parse.
     */
    ctx->line = line;
    ctx->line_start = line_start;
    return buf;
failed:
    return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_runtime);
}

int flatcc_json_parser_table_as_root(flatcc_builder_t *B, flatcc_json_parser_t *ctx,
        const char *buf, size_t bufsiz, int flags, const char *fid,
        flatcc_json_parser_table_f *parser)
{
    flatcc_json_parser_t _ctx;
    flatcc_builder_ref_t root;
    int builder_flags = flags & flatcc_json_parser_f_with_size ? flatcc_builder_with_size : 0;

    ctx = ctx ? ctx : &_ctx;
    flatcc_json_parser_init(ctx, B, buf, buf + bufsiz, flags);
    if (flatcc_builder_start_buffer(B, fid, 0, builder_flags)) return -1;
    parser(ctx, buf, buf + bufsiz, &root);
    if (ctx->error) {
        return ctx->error;
    }
    if (!flatcc_builder_end_buffer(B, root)) return -1;
    ctx->end_loc = buf;
    return 0;
}

int flatcc_json_parser_struct_as_root(flatcc_builder_t *B, flatcc_json_parser_t *ctx,
        const char *buf, size_t bufsiz, int flags, const char *fid,
        flatcc_json_parser_table_f *parser)
{
    flatcc_json_parser_t _ctx;
    flatcc_builder_ref_t root;
    int builder_flags = flags & flatcc_json_parser_f_with_size ? flatcc_builder_with_size : 0;

    ctx = ctx ? ctx : &_ctx;
    flatcc_json_parser_init(ctx, B, buf, buf + bufsiz, flags);
    if (flatcc_builder_start_buffer(B, fid, 0, builder_flags)) return -1;
    parser(ctx, buf, buf + bufsiz, &root);
    if (ctx->error) {
        return ctx->error;
    }
    if (!flatcc_builder_end_buffer(B, root)) return -1;
    ctx->end_loc = buf;
    return 0;
}
