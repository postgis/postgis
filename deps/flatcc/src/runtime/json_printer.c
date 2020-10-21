/*
 * Runtime support for printing flatbuffers to JSON.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "flatcc/flatcc_rtconfig.h"
#include "flatcc/flatcc_assert.h"

/*
 * Grisu significantly improves printing speed of floating point values
 * and also the overall printing speed when floating point values are
 * present in non-trivial amounts. (Also applies to parsing).
 */
#if FLATCC_USE_GRISU3 && !defined(PORTABLE_USE_GRISU3)
#define PORTABLE_USE_GRISU3 1
#endif

#include "flatcc/flatcc_flatbuffers.h"
#include "flatcc/flatcc_json_printer.h"
#include "flatcc/flatcc_identifier.h"

#include "flatcc/portable/pprintint.h"
#include "flatcc/portable/pprintfp.h"
#include "flatcc/portable/pbase64.h"


#define RAISE_ERROR(err) flatcc_json_printer_set_error(ctx, flatcc_json_printer_error_##err)

const char *flatcc_json_printer_error_string(int err)
{
    switch (err) {
#define XX(no, str)                                                         \
    case flatcc_json_printer_error_##no:                                    \
        return str;
        FLATCC_JSON_PRINT_ERROR_MAP(XX)
#undef XX
    default:
        return "unknown";
    }
}

#define flatcc_json_printer_utype_enum_f flatcc_json_printer_union_type_f
#define flatbuffers_utype_read_from_pe __flatbuffers_utype_read_from_pe

#define uoffset_t flatbuffers_uoffset_t
#define soffset_t flatbuffers_soffset_t
#define voffset_t flatbuffers_voffset_t
#define utype_t flatbuffers_utype_t

#define uoffset_size sizeof(uoffset_t)
#define soffset_size sizeof(soffset_t)
#define voffset_size sizeof(voffset_t)
#define utype_size sizeof(utype_t)

#define offset_size uoffset_size

#if FLATBUFFERS_UTYPE_MAX == UINT8_MAX
#define print_utype print_uint8
#else
#ifdef FLATBUFFERS_UTYPE_MIN
#define print_utype print_int64
#else
#define print_utype print_uint64
#endif
#endif

static inline const void *read_uoffset_ptr(const void *p)
{
    return (uint8_t *)p + __flatbuffers_uoffset_read_from_pe(p);
}

static inline voffset_t read_voffset(const void *p, uoffset_t base)
{
    return __flatbuffers_voffset_read_from_pe((uint8_t *)p + base);
}

static inline const void *get_field_ptr(flatcc_json_printer_table_descriptor_t *td, int id)
{
    uoffset_t vo = (uoffset_t)(id + 2) * (uoffset_t)sizeof(voffset_t);

    if (vo >= (uoffset_t)td->vsize) {
        return 0;
    }
    vo = read_voffset(td->vtable, vo);
    if (vo == 0) {
        return 0;
    }
    return (uint8_t *)td->table + vo;
}

#define print_char(c) *ctx->p++ = (c)

#define print_null() do {                                                   \
    print_char('n');                                                        \
    print_char('u');                                                        \
    print_char('l');                                                        \
    print_char('l');                                                        \
} while (0)

#define print_start(c) do {                                                 \
    ++ctx->level;                                                           \
    *ctx->p++ = c;                                                          \
} while (0)

#define print_end(c) do {                                                   \
    if (ctx->indent) {                                                      \
        *ctx->p++ = '\n';                                                   \
        --ctx->level;                                                       \
        print_indent(ctx);                                                  \
    }                                                                       \
    *ctx->p++ = c;                                                          \
} while (0)

#define print_space() do {                                                  \
    *ctx->p = ' ';                                                          \
    ctx->p += !!ctx->indent;                                                \
} while (0)

#define print_nl() do {                                                     \
    if (ctx->indent) {                                                      \
        *ctx->p++ = '\n';                                                   \
        print_indent(ctx);                                                  \
    } else {                                                                \
        flatcc_json_printer_flush_partial(ctx);                             \
    }                                                                       \
} while (0)

/* Call at the end so print_end does not have to check for level. */
#define print_last_nl() do {                                                \
    if (ctx->indent && ctx->level == 0) {                                   \
        *ctx->p++ = '\n';                                                   \
    }                                                                       \
    ctx->flush(ctx, 1);                                                     \
} while (0)

int flatcc_json_printer_fmt_float(char *buf, float n)
{
#if FLATCC_JSON_PRINT_HEX_FLOAT
    return print_hex_float(buf, n);
#else
    return print_float(n, buf);
#endif
}

int flatcc_json_printer_fmt_double(char *buf, double n)
{
#if FLATCC_JSON_PRINT_HEX_FLOAT
    return print_hex_double(buf, n);
#else
    return print_double(n, buf);
#endif
}

int flatcc_json_printer_fmt_bool(char *buf, int n)
{
    if (n) {
        memcpy(buf, "true", 4);
        return 4;
    }
    memcpy(buf, "false", 5);
    return 5;
}

static void print_ex(flatcc_json_printer_t *ctx, const char *s, size_t n)
{
    size_t k;

    if (ctx->p >= ctx->pflush) {
        ctx->flush(ctx, 0);
    }
    k = (size_t)(ctx->pflush - ctx->p);
    while (n > k) {
        memcpy(ctx->p, s, k);
        ctx->p += k;
        s += k;
        n -= k;
        ctx->flush(ctx, 0);
        k = (size_t)(ctx->pflush - ctx->p);
    }
    memcpy(ctx->p, s, n);
    ctx->p += n;
}

static inline void print(flatcc_json_printer_t *ctx, const char *s, size_t n)
{
    if (ctx->p + n >= ctx->pflush) {
        print_ex(ctx, s, n);
    } else {
        memcpy(ctx->p, s, n);
        ctx->p += n;
    }
}

static void print_escape(flatcc_json_printer_t *ctx, unsigned char c)
{
    unsigned char x;

    print_char('\\');
    switch (c) {
    case '"': print_char('\"'); break;
    case '\\': print_char('\\'); break;
    case '\t' : print_char('t'); break;
    case '\f' : print_char('f'); break;
    case '\r' : print_char('r'); break;
    case '\n' : print_char('n'); break;
    case '\b' : print_char('b'); break;
    default:
        print_char('u');
        print_char('0');
        print_char('0');
        x = c >> 4;
        x += x < 10 ? '0' : 'a' - 10;
        print_char((char)x);
        x = c & 15;
        x += x < 10 ? '0' : 'a' - 10;
        print_char((char)x);
        break;
    }
}

/*
 * Even though we know the the string length, we need to scan for escape
 * characters. There may be embedded zeroes. Because FlatBuffer strings
 * are always zero terminated, we assume and optimize for this.
 *
 * We enforce \u00xx for control characters, but not for invalid
 * characters like 0xff - this makes it possible to handle some other
 * codepages transparently while formally not valid.  (Formally JSON
 * also supports UTF-16/32 little/big endian but flatbuffers only
 * support UTF-8 and we expect this in JSON input/output too).
 */
static void print_string(flatcc_json_printer_t *ctx, const char *s, size_t n)
{
    const char *p = s;
    /* Unsigned is important. */
    unsigned char c;
    size_t k;

    print_char('\"');
    for (;;) {
        c = (unsigned char)*p;
        while (c >= 0x20 && c != '\"' && c != '\\') {
            c = (unsigned char)*++p;
        }
        k = (size_t)(p - s);
        /* Even if k == 0, print ensures buffer flush. */
        print(ctx, s, k);
        n -= k;
        if (n == 0) break;
        s += k;
        print_escape(ctx, c);
        ++p;
        --n;
        ++s;
    }
    print_char('\"');
}

/*
 * Similar to print_string, but null termination is not guaranteed, and
 * trailing nulls are stripped.
 */
static void print_char_array(flatcc_json_printer_t *ctx, const char *s, size_t n)
{
    const char *p = s;
    /* Unsigned is important. */
    unsigned char c = 0;
    size_t k;

    while (n > 0 && s[n - 1] == '\0') --n;

    print_char('\"');
    for (;;) {
        while (n) {
            c = (unsigned char)*p;
            if (c < 0x20 || c == '\"' || c == '\\') break;
            ++p;
            --n;
        }
        k = (size_t)(p - s);
        /* Even if k == 0, print ensures buffer flush. */
        print(ctx, s, k);
        if (n == 0) break;
        s += k;
        print_escape(ctx, c);
        ++p;
        --n;
        ++s;
    }
    print_char('\"');
}

static void print_uint8_vector_base64_object(flatcc_json_printer_t *ctx, const void *p, int mode)
{
    const int unpadded_mode = mode & ~base64_enc_modifier_padding;
    size_t k, n, len;
    const uint8_t *data;
    size_t data_len, src_len;

    data_len = (size_t)__flatbuffers_uoffset_read_from_pe(p);
    data = (const uint8_t *)p + uoffset_size;

    print_char('\"');

    len = base64_encoded_size(data_len, mode);
    if (ctx->p + len >= ctx->pflush) {
        ctx->flush(ctx, 0);
    }
    while (ctx->p + len > ctx->pflush) {
        /* Multiples of 4 output chars consumes exactly 3 bytes before final padding. */
        k = (size_t)(ctx->pflush - ctx->p) & ~(size_t)3;
        n = k * 3 / 4;
        FLATCC_ASSERT(n > 0);
        src_len = k * 3 / 4;
        base64_encode((uint8_t *)ctx->p, data, 0, &src_len, unpadded_mode);
        ctx->p += k;
        data += n;
        data_len -= n;
        ctx->flush(ctx, 0);
        len = base64_encoded_size(data_len, mode);
    }
    base64_encode((uint8_t *)ctx->p, data, 0, &data_len, mode);
    ctx->p += len;
    print_char('\"');
}

static void print_indent_ex(flatcc_json_printer_t *ctx, size_t n)
{
    size_t k;

    if (ctx->p >= ctx->pflush) {
        ctx->flush(ctx, 0);
    }
    k = (size_t)(ctx->pflush - ctx->p);
    while (n > k) {
        memset(ctx->p, ' ', k);
        ctx->p += k;
        n -= k;
        ctx->flush(ctx, 0);
        k = (size_t)(ctx->pflush - ctx->p);
    }
    memset(ctx->p, ' ', n);
    ctx->p += n;
}

static inline void print_indent(flatcc_json_printer_t *ctx)
{
    size_t n = (size_t)(ctx->level * ctx->indent);

    if (ctx->p + n > ctx->pflush) {
        print_indent_ex(ctx, n);
    } else {
        memset(ctx->p, ' ', n);
        ctx->p += n;
    }
}

/*
 * Helpers for external use - does not do autmatic pretty printing, but
 * does escape strings.
 */
void flatcc_json_printer_string(flatcc_json_printer_t *ctx, const char *s, size_t n)
{
    print_string(ctx, s, n);
}

void flatcc_json_printer_write(flatcc_json_printer_t *ctx, const char *s, size_t n)
{
    print(ctx, s, n);
}

void flatcc_json_printer_nl(flatcc_json_printer_t *ctx)
{
    print_char('\n');
    flatcc_json_printer_flush_partial(ctx);
}

void flatcc_json_printer_char(flatcc_json_printer_t *ctx, char c)
{
    print_char(c);
}

void flatcc_json_printer_indent(flatcc_json_printer_t *ctx)
{
    /*
     * This is only needed when indent is 0 but helps external users
     * to avoid flushing when indenting.
     */
    print_indent(ctx);
}

void flatcc_json_printer_add_level(flatcc_json_printer_t *ctx, int n)
{
    ctx->level += n;
}

int flatcc_json_printer_get_level(flatcc_json_printer_t *ctx)
{
    return ctx->level;
}

static inline void print_symbol(flatcc_json_printer_t *ctx, const char *name, size_t len)
{
    *ctx->p = '\"';
    ctx->p += !ctx->unquote;
    if (ctx->p + len < ctx->pflush) {
        memcpy(ctx->p, name, len);
        ctx->p += len;
    } else {
        print(ctx, name, len);
    }
    *ctx->p = '\"';
    ctx->p += !ctx->unquote;
}

static inline void print_name(flatcc_json_printer_t *ctx, const char *name, size_t len)
{
    print_nl();
    print_symbol(ctx, name, len);
    print_char(':');
    print_space();
}

#define __flatcc_define_json_printer_scalar(TN, T)                          \
void flatcc_json_printer_ ## TN(                                            \
        flatcc_json_printer_t *ctx, T v)                                    \
{                                                                           \
    ctx->p += print_ ## TN(v, ctx->p);                                      \
}

__flatcc_define_json_printer_scalar(uint8, uint8_t)
__flatcc_define_json_printer_scalar(uint16, uint16_t)
__flatcc_define_json_printer_scalar(uint32, uint32_t)
__flatcc_define_json_printer_scalar(uint64, uint64_t)
__flatcc_define_json_printer_scalar(int8, int8_t)
__flatcc_define_json_printer_scalar(int16, int16_t)
__flatcc_define_json_printer_scalar(int32, int32_t)
__flatcc_define_json_printer_scalar(int64, int64_t)
__flatcc_define_json_printer_scalar(float, float)
__flatcc_define_json_printer_scalar(double, double)

void flatcc_json_printer_enum(flatcc_json_printer_t *ctx, const char *symbol, size_t len)
{
    print_symbol(ctx, symbol, len);
}

void flatcc_json_printer_delimit_enum_flags(flatcc_json_printer_t *ctx, int multiple)
{
#if FLATCC_JSON_PRINT_ALWAYS_QUOTE_MULTIPLE_FLAGS
    int quote = !ctx->unquote || multiple;
#else
    int quote = !ctx->unquote;
#endif
    *ctx->p = '"';
    ctx->p += quote;
}

void flatcc_json_printer_enum_flag(flatcc_json_printer_t *ctx, int count, const char *symbol, size_t len)
{
    *ctx->p = ' ';
    ctx->p += count > 0;
    print(ctx, symbol, len);
}

static inline void print_string_object(flatcc_json_printer_t *ctx, const void *p)
{
    size_t len;
    const char *s;

    len = (size_t)__flatbuffers_uoffset_read_from_pe(p);
    s = (const char *)p + uoffset_size;
    print_string(ctx, s, len);
}

#define __define_print_scalar_struct_field(TN, T)                           \
void flatcc_json_printer_ ## TN ## _struct_field(flatcc_json_printer_t *ctx,\
        int index, const void *p, size_t offset,                            \
        const char *name, size_t len)                                       \
{                                                                           \
    T x = flatbuffers_ ## TN ## _read_from_pe((uint8_t *)p + offset);       \
                                                                            \
    if (index) {                                                            \
        print_char(',');                                                    \
    }                                                                       \
    print_name(ctx, name, len);                                             \
    ctx->p += print_ ## TN (x, ctx->p);                                     \
}

void flatcc_json_printer_char_array_struct_field(
        flatcc_json_printer_t *ctx,
        int index, const void *p, size_t offset,
        const char *name, size_t len, size_t count)
{
    p = (void *)((size_t)p + offset);
    if (index) {
        print_char(',');
    }
    print_name(ctx, name, len);
    print_char_array(ctx, p, count);
}

#define __define_print_scalar_array_struct_field(TN, T)                     \
void flatcc_json_printer_ ## TN ## _array_struct_field(                     \
        flatcc_json_printer_t *ctx,                                         \
        int index, const void *p, size_t offset,                            \
        const char *name, size_t len, size_t count)                         \
{                                                                           \
    p = (void *)((size_t)p + offset);                                       \
    if (index) {                                                            \
        print_char(',');                                                    \
    }                                                                       \
    print_name(ctx, name, len);                                             \
    print_start('[');                                                       \
    if (count) {                                                            \
        print_nl();                                                         \
        ctx->p += print_ ## TN (                                            \
                flatbuffers_ ## TN ## _read_from_pe(p),                     \
                ctx->p);                                                    \
        p = (void *)((size_t)p + sizeof(T));                                \
        --count;                                                            \
    }                                                                       \
    while (count--) {                                                       \
        print_char(',');                                                    \
        print_nl();                                                         \
        ctx->p += print_ ## TN (                                            \
                flatbuffers_ ## TN ## _read_from_pe(p),                     \
                ctx->p);                                                    \
        p = (void *)((size_t)p + sizeof(T));                                \
    }                                                                       \
    print_end(']');                                                         \
}

#define __define_print_enum_array_struct_field(TN, T)                       \
void flatcc_json_printer_ ## TN ## _enum_array_struct_field(                \
        flatcc_json_printer_t *ctx,                                         \
        int index, const void *p, size_t offset,                            \
        const char *name, size_t len, size_t count,                         \
        flatcc_json_printer_ ## TN ##_enum_f *pf)                           \
{                                                                           \
    T x;                                                                    \
                                                                            \
    p = (void *)((size_t)p + offset);                                       \
    if (index) {                                                            \
        print_char(',');                                                    \
    }                                                                       \
    print_name(ctx, name, len);                                             \
    print_start('[');                                                       \
    if (count) {                                                            \
        print_nl();                                                         \
        x = flatbuffers_ ## TN ## _read_from_pe(p);                         \
        if (ctx->noenum) {                                                  \
            ctx->p += print_ ## TN (x, ctx->p);                             \
        } else {                                                            \
            pf(ctx, x);                                                     \
        }                                                                   \
        p = (void *)((size_t)p + sizeof(T));                                \
        --count;                                                            \
    }                                                                       \
    while (count--) {                                                       \
        print_char(',');                                                    \
        print_nl();                                                         \
        x = flatbuffers_ ## TN ## _read_from_pe(p);                         \
        if (ctx->noenum) {                                                  \
            ctx->p += print_ ## TN (x, ctx->p);                             \
        } else {                                                            \
            pf(ctx, x);                                                     \
        }                                                                   \
        p = (void *)((size_t)p + sizeof(T));                                \
    }                                                                       \
    print_end(']');                                                         \
}

#define __define_print_enum_struct_field(TN, T)                             \
void flatcc_json_printer_ ## TN ## _enum_struct_field(                      \
        flatcc_json_printer_t *ctx,                                         \
        int index, const void *p, size_t offset,                            \
        const char *name, size_t len,                                       \
        flatcc_json_printer_ ## TN ##_enum_f *pf)                           \
{                                                                           \
    T x = flatbuffers_ ## TN ## _read_from_pe((uint8_t *)p + offset);       \
                                                                            \
    if (index) {                                                            \
        print_char(',');                                                    \
    }                                                                       \
    print_name(ctx, name, len);                                             \
    if (ctx->noenum) {                                                      \
        ctx->p += print_ ## TN (x, ctx->p);                                 \
    } else {                                                                \
        pf(ctx, x);                                                         \
    }                                                                       \
}

#define __define_print_scalar_field(TN, T)                                  \
void flatcc_json_printer_ ## TN ## _field(flatcc_json_printer_t *ctx,       \
        flatcc_json_printer_table_descriptor_t *td,                         \
        int id, const char *name, size_t len, T v)                          \
{                                                                           \
    T x;                                                                    \
    const void *p = get_field_ptr(td, id);                                  \
                                                                            \
    if (p) {                                                                \
        x = flatbuffers_ ## TN ## _read_from_pe(p);                         \
        if (x == v && ctx->skip_default) {                                  \
            return;                                                         \
        }                                                                   \
    } else {                                                                \
        if (!ctx->force_default) {                                          \
            return;                                                         \
        }                                                                   \
        x = v;                                                              \
    }                                                                       \
    if (td->count++) {                                                      \
        print_char(',');                                                    \
    }                                                                       \
    print_name(ctx, name, len);                                             \
    ctx->p += print_ ## TN (x, ctx->p);                                     \
}

#define __define_print_scalar_optional_field(TN, T)                         \
void flatcc_json_printer_ ## TN ## _optional_field(                         \
        flatcc_json_printer_t *ctx,                                         \
        flatcc_json_printer_table_descriptor_t *td,                         \
        int id, const char *name, size_t len)                               \
{                                                                           \
    T x;                                                                    \
    const void *p = get_field_ptr(td, id);                                  \
                                                                            \
    if (!p) return;                                                         \
    x = flatbuffers_ ## TN ## _read_from_pe(p);                             \
    if (td->count++) {                                                      \
        print_char(',');                                                    \
    }                                                                       \
    print_name(ctx, name, len);                                             \
    ctx->p += print_ ## TN (x, ctx->p);                                     \
}


#define __define_print_enum_field(TN, T)                                    \
void flatcc_json_printer_ ## TN ## _enum_field(flatcc_json_printer_t *ctx,  \
        flatcc_json_printer_table_descriptor_t *td,                         \
        int id, const char *name, size_t len, T v,                          \
        flatcc_json_printer_ ## TN ##_enum_f *pf)                           \
{                                                                           \
    T x;                                                                    \
    const void *p = get_field_ptr(td, id);                                  \
                                                                            \
    if (p) {                                                                \
        x = flatbuffers_ ## TN ## _read_from_pe(p);                         \
        if (x == v && ctx->skip_default) {                                  \
            return;                                                         \
        }                                                                   \
    } else {                                                                \
        if (!ctx->force_default) {                                          \
            return;                                                         \
        }                                                                   \
        x = v;                                                              \
    }                                                                       \
    if (td->count++) {                                                      \
        print_char(',');                                                    \
    }                                                                       \
    print_name(ctx, name, len);                                             \
    if (ctx->noenum) {                                                      \
        ctx->p += print_ ## TN (x, ctx->p);                                 \
    } else {                                                                \
        pf(ctx, x);                                                         \
    }                                                                       \
}

#define __define_print_enum_optional_field(TN, T)                           \
void flatcc_json_printer_ ## TN ## _enum_optional_field(                    \
        flatcc_json_printer_t *ctx,                                         \
        flatcc_json_printer_table_descriptor_t *td,                         \
        int id, const char *name, size_t len,                               \
        flatcc_json_printer_ ## TN ##_enum_f *pf)                           \
{                                                                           \
    T x;                                                                    \
    const void *p = get_field_ptr(td, id);                                  \
                                                                            \
    if (!p) return;                                                         \
    x = flatbuffers_ ## TN ## _read_from_pe(p);                             \
    if (td->count++) {                                                      \
        print_char(',');                                                    \
    }                                                                       \
    print_name(ctx, name, len);                                             \
    if (ctx->noenum) {                                                      \
        ctx->p += print_ ## TN (x, ctx->p);                                 \
    } else {                                                                \
        pf(ctx, x);                                                         \
    }                                                                       \
}

static inline void print_table_object(flatcc_json_printer_t *ctx,
        const void *p, int ttl, flatcc_json_printer_table_f pf)
{
    flatcc_json_printer_table_descriptor_t td;

    if (!--ttl) {
        flatcc_json_printer_set_error(ctx, flatcc_json_printer_error_deep_recursion);
        return;
    }
    print_start('{');
    td.count = 0;
    td.ttl = ttl;
    td.table = p;
    td.vtable = (uint8_t *)p - __flatbuffers_soffset_read_from_pe(p);
    td.vsize = __flatbuffers_voffset_read_from_pe(td.vtable);
    pf(ctx, &td);
    print_end('}');
}

void flatcc_json_printer_string_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len)
{
    const void *p = get_field_ptr(td, id);

    if (p) {
        if (td->count++) {
            print_char(',');
        }
        print_name(ctx, name, len);
        print_string_object(ctx, read_uoffset_ptr(p));
    }
}

void flatcc_json_printer_uint8_vector_base64_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len, int urlsafe)
{
    const void *p = get_field_ptr(td, id);
    int mode;

    mode = urlsafe ? base64_mode_url : base64_mode_rfc4648;
    mode |= base64_enc_modifier_padding;

    if (p) {
        if (td->count++) {
            print_char(',');
        }
        print_name(ctx, name, len);
        print_uint8_vector_base64_object(ctx, read_uoffset_ptr(p), mode);
    }
}

#define __define_print_scalar_vector_field(TN, T)                           \
void flatcc_json_printer_ ## TN ## _vector_field(                           \
        flatcc_json_printer_t *ctx,                                         \
        flatcc_json_printer_table_descriptor_t *td,                         \
        int id, const char *name, size_t len)                               \
{                                                                           \
    const void *p = get_field_ptr(td, id);                                  \
    uoffset_t count;                                                        \
                                                                            \
    if (p) {                                                                \
        if (td->count++) {                                                  \
            print_char(',');                                                \
        }                                                                   \
        p = read_uoffset_ptr(p);                                            \
        count = __flatbuffers_uoffset_read_from_pe(p);                      \
        p = (void *)((size_t)p + uoffset_size);                             \
        print_name(ctx, name, len);                                         \
        print_start('[');                                                   \
        if (count) {                                                        \
            print_nl();                                                     \
            ctx->p += print_ ## TN (                                        \
                    flatbuffers_ ## TN ## _read_from_pe(p),                 \
                    ctx->p);                                                \
            p = (void *)((size_t)p + sizeof(T));                            \
            --count;                                                        \
        }                                                                   \
        while (count--) {                                                   \
            print_char(',');                                                \
            print_nl();                                                     \
            ctx->p += print_ ## TN (                                        \
                    flatbuffers_ ## TN ## _read_from_pe(p),                 \
                    ctx->p);                                                \
            p = (void *)((size_t)p + sizeof(T));                            \
        }                                                                   \
        print_end(']');                                                     \
    }                                                                       \
}

#define __define_print_enum_vector_field(TN, T)                             \
void flatcc_json_printer_ ## TN ## _enum_vector_field(                      \
        flatcc_json_printer_t *ctx,                                         \
        flatcc_json_printer_table_descriptor_t *td,                         \
        int id, const char *name, size_t len,                               \
        flatcc_json_printer_ ## TN ##_enum_f *pf)                           \
{                                                                           \
    const void *p;                                                          \
    uoffset_t count;                                                        \
                                                                            \
    if (ctx->noenum) {                                                      \
        flatcc_json_printer_ ## TN ## _vector_field(ctx, td, id, name, len);\
        return;                                                             \
    }                                                                       \
    p = get_field_ptr(td, id);                                              \
    if (p) {                                                                \
        if (td->count++) {                                                  \
            print_char(',');                                                \
        }                                                                   \
        p = read_uoffset_ptr(p);                                            \
        count = __flatbuffers_uoffset_read_from_pe(p);                      \
        p = (void *)((size_t)p + uoffset_size);                             \
        print_name(ctx, name, len);                                         \
        print_start('[');                                                   \
        if (count) {                                                        \
            print_nl();                                                     \
            pf(ctx, flatbuffers_ ## TN ## _read_from_pe(p));                \
            p = (void *)((size_t)p + sizeof(T));                            \
            --count;                                                        \
        }                                                                   \
        while (count--) {                                                   \
            print_char(',');                                                \
            print_nl();                                                     \
            pf(ctx, flatbuffers_ ## TN ## _read_from_pe(p));                \
            p = (void *)((size_t)p + sizeof(T));                            \
        }                                                                   \
        print_end(']');                                                     \
    }                                                                       \
}

__define_print_scalar_field(uint8, uint8_t)
__define_print_scalar_field(uint16, uint16_t)
__define_print_scalar_field(uint32, uint32_t)
__define_print_scalar_field(uint64, uint64_t)
__define_print_scalar_field(int8, int8_t)
__define_print_scalar_field(int16, int16_t)
__define_print_scalar_field(int32, int32_t)
__define_print_scalar_field(int64, int64_t)
__define_print_scalar_field(bool, flatbuffers_bool_t)
__define_print_scalar_field(float, float)
__define_print_scalar_field(double, double)

__define_print_enum_field(uint8, uint8_t)
__define_print_enum_field(uint16, uint16_t)
__define_print_enum_field(uint32, uint32_t)
__define_print_enum_field(uint64, uint64_t)
__define_print_enum_field(int8, int8_t)
__define_print_enum_field(int16, int16_t)
__define_print_enum_field(int32, int32_t)
__define_print_enum_field(int64, int64_t)
__define_print_enum_field(bool, flatbuffers_bool_t)

__define_print_scalar_optional_field(uint8, uint8_t)
__define_print_scalar_optional_field(uint16, uint16_t)
__define_print_scalar_optional_field(uint32, uint32_t)
__define_print_scalar_optional_field(uint64, uint64_t)
__define_print_scalar_optional_field(int8, int8_t)
__define_print_scalar_optional_field(int16, int16_t)
__define_print_scalar_optional_field(int32, int32_t)
__define_print_scalar_optional_field(int64, int64_t)
__define_print_scalar_optional_field(bool, flatbuffers_bool_t)
__define_print_scalar_optional_field(float, float)
__define_print_scalar_optional_field(double, double)

__define_print_enum_optional_field(uint8, uint8_t)
__define_print_enum_optional_field(uint16, uint16_t)
__define_print_enum_optional_field(uint32, uint32_t)
__define_print_enum_optional_field(uint64, uint64_t)
__define_print_enum_optional_field(int8, int8_t)
__define_print_enum_optional_field(int16, int16_t)
__define_print_enum_optional_field(int32, int32_t)
__define_print_enum_optional_field(int64, int64_t)
__define_print_enum_optional_field(bool, flatbuffers_bool_t)

__define_print_scalar_struct_field(uint8, uint8_t)
__define_print_scalar_struct_field(uint16, uint16_t)
__define_print_scalar_struct_field(uint32, uint32_t)
__define_print_scalar_struct_field(uint64, uint64_t)
__define_print_scalar_struct_field(int8, int8_t)
__define_print_scalar_struct_field(int16, int16_t)
__define_print_scalar_struct_field(int32, int32_t)
__define_print_scalar_struct_field(int64, int64_t)
__define_print_scalar_struct_field(bool, flatbuffers_bool_t)
__define_print_scalar_struct_field(float, float)
__define_print_scalar_struct_field(double, double)

__define_print_scalar_array_struct_field(uint8, uint8_t)
__define_print_scalar_array_struct_field(uint16, uint16_t)
__define_print_scalar_array_struct_field(uint32, uint32_t)
__define_print_scalar_array_struct_field(uint64, uint64_t)
__define_print_scalar_array_struct_field(int8, int8_t)
__define_print_scalar_array_struct_field(int16, int16_t)
__define_print_scalar_array_struct_field(int32, int32_t)
__define_print_scalar_array_struct_field(int64, int64_t)
__define_print_scalar_array_struct_field(bool, flatbuffers_bool_t)
__define_print_scalar_array_struct_field(float, float)
__define_print_scalar_array_struct_field(double, double)

__define_print_enum_array_struct_field(uint8, uint8_t)
__define_print_enum_array_struct_field(uint16, uint16_t)
__define_print_enum_array_struct_field(uint32, uint32_t)
__define_print_enum_array_struct_field(uint64, uint64_t)
__define_print_enum_array_struct_field(int8, int8_t)
__define_print_enum_array_struct_field(int16, int16_t)
__define_print_enum_array_struct_field(int32, int32_t)
__define_print_enum_array_struct_field(int64, int64_t)
__define_print_enum_array_struct_field(bool, flatbuffers_bool_t)

__define_print_enum_struct_field(uint8, uint8_t)
__define_print_enum_struct_field(uint16, uint16_t)
__define_print_enum_struct_field(uint32, uint32_t)
__define_print_enum_struct_field(uint64, uint64_t)
__define_print_enum_struct_field(int8, int8_t)
__define_print_enum_struct_field(int16, int16_t)
__define_print_enum_struct_field(int32, int32_t)
__define_print_enum_struct_field(int64, int64_t)
__define_print_enum_struct_field(bool, flatbuffers_bool_t)

__define_print_scalar_vector_field(utype, flatbuffers_utype_t)
__define_print_scalar_vector_field(uint8, uint8_t)
__define_print_scalar_vector_field(uint16, uint16_t)
__define_print_scalar_vector_field(uint32, uint32_t)
__define_print_scalar_vector_field(uint64, uint64_t)
__define_print_scalar_vector_field(int8, int8_t)
__define_print_scalar_vector_field(int16, int16_t)
__define_print_scalar_vector_field(int32, int32_t)
__define_print_scalar_vector_field(int64, int64_t)
__define_print_scalar_vector_field(bool, flatbuffers_bool_t)
__define_print_scalar_vector_field(float, float)
__define_print_scalar_vector_field(double, double)

__define_print_enum_vector_field(utype, flatbuffers_utype_t)
__define_print_enum_vector_field(uint8, uint8_t)
__define_print_enum_vector_field(uint16, uint16_t)
__define_print_enum_vector_field(uint32, uint32_t)
__define_print_enum_vector_field(uint64, uint64_t)
__define_print_enum_vector_field(int8, int8_t)
__define_print_enum_vector_field(int16, int16_t)
__define_print_enum_vector_field(int32, int32_t)
__define_print_enum_vector_field(int64, int64_t)
__define_print_enum_vector_field(bool, flatbuffers_bool_t)

void flatcc_json_printer_struct_vector_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len,
        size_t size,
        flatcc_json_printer_struct_f pf)
{
    const uint8_t *p = get_field_ptr(td, id);
    uoffset_t count;

    if (p) {
        if (td->count++) {
            print_char(',');
        }
        p = read_uoffset_ptr(p);
        count = __flatbuffers_uoffset_read_from_pe(p);
        p += uoffset_size;
        print_name(ctx, name, len);
        print_start('[');
        if (count) {
            print_nl();
            print_start('{');
            pf(ctx, p);
            print_end('}');
            --count;
        }
        while (count--) {
            p += size;
            print_char(',');
            print_nl();
            print_start('{');
            pf(ctx, p);
            print_end('}');
        }
        print_end(']');
    }
}

void flatcc_json_printer_string_vector_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len)
{
    const uoffset_t *p = get_field_ptr(td, id);
    uoffset_t count;

    if (p) {
        if (td->count++) {
            print_char(',');
        }
        p = read_uoffset_ptr(p);
        count = __flatbuffers_uoffset_read_from_pe(p);
        ++p;
        print_name(ctx, name, len);
        print_start('[');
        if (count) {
            print_nl();
            print_string_object(ctx, read_uoffset_ptr(p));
            --count;
        }
        while (count--) {
            ++p;
            print_char(',');
            print_nl();
            print_string_object(ctx, read_uoffset_ptr(p));
        }
        print_end(']');
    }
}

void flatcc_json_printer_table_vector_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len,
        flatcc_json_printer_table_f pf)
{
    const uoffset_t *p = get_field_ptr(td, id);
    uoffset_t count;

    if (p) {
        if (td->count++) {
            print_char(',');
        }
        p = read_uoffset_ptr(p);
        count = __flatbuffers_uoffset_read_from_pe(p);
        ++p;
        print_name(ctx, name, len);
        print_start('[');
        if (count) {
            print_table_object(ctx, read_uoffset_ptr(p), td->ttl, pf);
            --count;
        }
        while (count--) {
            ++p;
            print_char(',');
            print_table_object(ctx, read_uoffset_ptr(p), td->ttl, pf);
        }
        print_end(']');
    }
}

void flatcc_json_printer_union_vector_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len,
        flatcc_json_printer_union_type_f ptf,
        flatcc_json_printer_union_f pf)
{
    const uoffset_t *pt = get_field_ptr(td, id - 1);
    const uoffset_t *p = get_field_ptr(td, id);
    utype_t *types, type;
    uoffset_t count;
    char type_name[FLATCC_JSON_PRINT_NAME_LEN_MAX + 5];
    flatcc_json_printer_union_descriptor_t ud;

    ud.ttl = td->ttl;
    if (len > FLATCC_JSON_PRINT_NAME_LEN_MAX) {
        RAISE_ERROR(bad_input);
        FLATCC_ASSERT(0 && "identifier too long");
        return;
    }
    memcpy(type_name, name, len);
    memcpy(type_name + len, "_type", 5);
    if (p && pt) {
        flatcc_json_printer_utype_enum_vector_field(ctx, td, id - 1,
                type_name, len + 5, ptf);
        if (td->count++) {
            print_char(',');
        }
        p = read_uoffset_ptr(p);
        pt = read_uoffset_ptr(pt);
        count = __flatbuffers_uoffset_read_from_pe(p);
        ++p;
        ++pt;
        types = (utype_t *)pt;
        print_name(ctx, name, len);
        print_start('[');

        if (count) {
            type = __flatbuffers_utype_read_from_pe(types);
            if (type != 0) {
                ud.type = type;
                ud.member = p;
                pf(ctx, &ud);
            } else {
                print_null();
            }
            --count;
        }
        while (count--) {
            ++p;
            ++types;
            type = __flatbuffers_utype_read_from_pe(types);
            print_char(',');
            if (type != 0) {
                ud.type = type;
                ud.member = p;
                pf(ctx, &ud);
            } else {
                print_null();
            }
        }
        print_end(']');
    }
}

void flatcc_json_printer_table_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len,
        flatcc_json_printer_table_f pf)
{
    const void *p = get_field_ptr(td, id);

    if (p) {
        if (td->count++) {
            print_char(',');
        }
        print_name(ctx, name, len);
        print_table_object(ctx, read_uoffset_ptr(p), td->ttl, pf);
    }
}

void flatcc_json_printer_union_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len,
        flatcc_json_printer_union_type_f ptf,
        flatcc_json_printer_union_f pf)
{
    const void *pt = get_field_ptr(td, id - 1);
    const void *p = get_field_ptr(td, id);
    utype_t type;
    flatcc_json_printer_union_descriptor_t ud;

    if (!p || !pt) {
        return;
    }
    type = __flatbuffers_utype_read_from_pe(pt);
    if (td->count++) {
        print_char(',');
    }
    print_nl();
    *ctx->p = '\"';
    ctx->p += !ctx->unquote;
    if (ctx->p + len < ctx->pflush) {
        memcpy(ctx->p, name, len);
        ctx->p += len;
    } else {
        print(ctx, name, len);
    }
    print(ctx, "_type", 5);
    *ctx->p = '\"';
    ctx->p += !ctx->unquote;
    print_char(':');
    print_space();
    if (ctx->noenum) {
        ctx->p += print_utype(type, ctx->p);
    } else {
        ptf(ctx, type);
    }
    if (type != 0) {
        print_char(',');
        print_name(ctx, name, len);
        ud.ttl = td->ttl;
        ud.type = type;
        ud.member = p;
        pf(ctx, &ud);
    }
}

void flatcc_json_printer_union_table(flatcc_json_printer_t *ctx,
        flatcc_json_printer_union_descriptor_t *ud,
        flatcc_json_printer_table_f pf)
{
    print_table_object(ctx, read_uoffset_ptr(ud->member), ud->ttl, pf);
}

void flatcc_json_printer_union_struct(flatcc_json_printer_t *ctx,
        flatcc_json_printer_union_descriptor_t *ud,
        flatcc_json_printer_struct_f pf)
{
    print_start('{');
    pf(ctx, read_uoffset_ptr(ud->member));
    print_end('}');
}

void flatcc_json_printer_union_string(flatcc_json_printer_t *ctx,
        flatcc_json_printer_union_descriptor_t *ud)
{
    print_string_object(ctx, read_uoffset_ptr(ud->member));
}

void flatcc_json_printer_embedded_struct_field(flatcc_json_printer_t *ctx,
        int index, const void *p, size_t offset,
        const char *name, size_t len,
        flatcc_json_printer_struct_f pf)
{
    if (index) {
        print_char(',');
    }
    print_name(ctx, name, len);
    print_start('{');
    pf(ctx, (uint8_t *)p + offset);
    print_end('}');
}

void flatcc_json_printer_embedded_struct_array_field(flatcc_json_printer_t *ctx,
        int index, const void *p, size_t offset,
        const char *name, size_t len,
        size_t size, size_t count,
        flatcc_json_printer_struct_f pf)
{
    size_t i;
    if (index) {
        print_char(',');
    }
    print_name(ctx, name, len);
    print_start('[');
    for (i = 0; i < count; ++i) {
        if (i > 0) {
            print_char(',');
        }
        print_start('{');                                                   \
        pf(ctx, (uint8_t *)p + offset + i * size);
        print_end('}');
    }
    print_end(']');
}

void flatcc_json_printer_struct_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len,
        flatcc_json_printer_struct_f *pf)
{
    const void *p = get_field_ptr(td, id);

    if (p) {
        if (td->count++) {
            print_char(',');
        }
        print_name(ctx, name, len);
        print_start('{');
        pf(ctx, p);
        print_end('}');
    }
}

/*
 * Make sure the buffer identifier is valid before assuming the rest of
 * the buffer is sane.
 * NOTE: this won't work with type hashes because these can contain
 * nulls in the fid string. In this case use null as fid to disable
 * check.
 */
static int accept_header(flatcc_json_printer_t * ctx,
        const void *buf, size_t bufsiz, const char *fid)
{
    flatbuffers_thash_t id, id2 = 0;

    if (buf == 0 || bufsiz < offset_size + FLATBUFFERS_IDENTIFIER_SIZE) {
        RAISE_ERROR(bad_input);
        FLATCC_ASSERT(0 && "buffer header too small");
        return 0;
    }
    if (fid != 0) {
        id2 = flatbuffers_type_hash_from_string(fid);
        id = __flatbuffers_thash_read_from_pe((uint8_t *)buf + offset_size);
        if (!(id2 == 0 || id == id2)) {
            RAISE_ERROR(bad_input);
            FLATCC_ASSERT(0 && "identifier mismatch");
            return 0;
        }
    }
    return 1;
}

int flatcc_json_printer_struct_as_root(flatcc_json_printer_t *ctx,
        const void *buf, size_t bufsiz, const char *fid,
        flatcc_json_printer_struct_f *pf)
{
    if (!accept_header(ctx, buf, bufsiz, fid)) {
        return -1;
    }
    print_start('{');
    pf(ctx, read_uoffset_ptr(buf));
    print_end('}');
    print_last_nl();
    return flatcc_json_printer_get_error(ctx) ? -1 : (int)ctx->total + (int)(ctx->p - ctx->buf);
}

int flatcc_json_printer_table_as_root(flatcc_json_printer_t *ctx,
        const void *buf, size_t bufsiz, const char *fid, flatcc_json_printer_table_f *pf)
{
    if (!accept_header(ctx, buf, bufsiz, fid)) {
        return -1;
    }
    print_table_object(ctx, read_uoffset_ptr(buf), FLATCC_JSON_PRINT_MAX_LEVELS, pf);
    print_last_nl();
    return flatcc_json_printer_get_error(ctx) ? -1 : (int)ctx->total + (int)(ctx->p - ctx->buf);
}

void flatcc_json_printer_struct_as_nested_root(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len,
        const char *fid,
        flatcc_json_printer_struct_f *pf)
{
    const uoffset_t *buf;
    uoffset_t bufsiz;

    if (0 == (buf = get_field_ptr(td, id))) {
        return;
    }
    buf = (const uoffset_t *)((size_t)buf + __flatbuffers_uoffset_read_from_pe(buf));
    bufsiz = __flatbuffers_uoffset_read_from_pe(buf);
    if (!accept_header(ctx, buf, bufsiz, fid)) {
        return;
    }
    if (td->count++) {
        print_char(',');
    }
    print_name(ctx, name, len);
    print_start('{');
    pf(ctx, read_uoffset_ptr(buf));
    print_end('}');
}

void flatcc_json_printer_table_as_nested_root(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len,
        const char *fid,
        flatcc_json_printer_table_f pf)
{
    const uoffset_t *buf;
    uoffset_t bufsiz;

    if (0 == (buf = get_field_ptr(td, id))) {
        return;
    }
    buf = (const uoffset_t *)((size_t)buf + __flatbuffers_uoffset_read_from_pe(buf));
    bufsiz = __flatbuffers_uoffset_read_from_pe(buf);
    ++buf;
    if (!accept_header(ctx, buf, bufsiz, fid)) {
        return;
    }
    if (td->count++) {
        print_char(',');
    }
    print_name(ctx, name, len);
    print_table_object(ctx, read_uoffset_ptr(buf), td->ttl, pf);
}

static void __flatcc_json_printer_flush(flatcc_json_printer_t *ctx, int all)
{
    if (!all && ctx->p >= ctx->pflush) {
        size_t spill = (size_t)(ctx->p - ctx->pflush);

        fwrite(ctx->buf, ctx->flush_size, 1, ctx->fp);
        memcpy(ctx->buf, ctx->buf + ctx->flush_size, spill);
        ctx->p = ctx->buf + spill;
        ctx->total += ctx->flush_size;
    } else {
        size_t len = (size_t)(ctx->p - ctx->buf);

        fwrite(ctx->buf, len, 1, ctx->fp);
        ctx->p = ctx->buf;
        ctx->total += len;
    }
    *ctx->p = '\0';
}

int flatcc_json_printer_init(flatcc_json_printer_t *ctx, void *fp)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->fp = fp ? fp : stdout;
    ctx->flush = __flatcc_json_printer_flush;
    if (!(ctx->buf = FLATCC_JSON_PRINTER_ALLOC(FLATCC_JSON_PRINT_BUFFER_SIZE))) {
        return -1;
    }
    ctx->own_buffer = 1;
    ctx->size = FLATCC_JSON_PRINT_BUFFER_SIZE;
    ctx->flush_size = FLATCC_JSON_PRINT_FLUSH_SIZE;
    ctx->p = ctx->buf;
    ctx->pflush = ctx->buf + ctx->flush_size;
    /*
     * Make sure we have space for primitive operations such as printing numbers
     * without having to flush.
     */
    FLATCC_ASSERT(ctx->flush_size + FLATCC_JSON_PRINT_RESERVE <= ctx->size);
    return 0;
}

static void __flatcc_json_printer_flush_buffer(flatcc_json_printer_t *ctx, int all)
{
    (void)all;

    if (ctx->p >= ctx->pflush) {
        RAISE_ERROR(overflow);
        ctx->total += (size_t)(ctx->p - ctx->buf);
        ctx->p = ctx->buf;
    }
    *ctx->p = '\0';
}

int flatcc_json_printer_init_buffer(flatcc_json_printer_t *ctx, char *buffer, size_t buffer_size)
{
    FLATCC_ASSERT(buffer_size >= FLATCC_JSON_PRINT_RESERVE);
    if (buffer_size < FLATCC_JSON_PRINT_RESERVE) {
        return -1;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->buf = buffer;
    ctx->size = buffer_size;
    ctx->flush_size = ctx->size - FLATCC_JSON_PRINT_RESERVE;
    ctx->p = ctx->buf;
    ctx->pflush = ctx->buf + ctx->flush_size;
    ctx->flush = __flatcc_json_printer_flush_buffer;
    return 0;
}

static void __flatcc_json_printer_flush_dynamic_buffer(flatcc_json_printer_t *ctx, int all)
{
    size_t len = (size_t)(ctx->p - ctx->buf);
    char *p;

    (void)all;

    *ctx->p = '\0';
    if (ctx->p < ctx->pflush) {
        return;
    }
    p = FLATCC_JSON_PRINTER_REALLOC(ctx->buf, ctx->size * 2);
    if (!p) {
        RAISE_ERROR(overflow);
        ctx->total += len;
        ctx->p = ctx->buf;
    } else {
        ctx->size *= 2;
        ctx->flush_size = ctx->size - FLATCC_JSON_PRINT_RESERVE;
        ctx->buf = p;
        ctx->p = p + len;
        ctx->pflush = p + ctx->flush_size;
    }
    *ctx->p = '\0';
}

int flatcc_json_printer_init_dynamic_buffer(flatcc_json_printer_t *ctx, size_t buffer_size)
{
    if (buffer_size == 0) {
        buffer_size = FLATCC_JSON_PRINT_DYN_BUFFER_SIZE;
    }
    if (buffer_size < FLATCC_JSON_PRINT_RESERVE) {
        buffer_size = FLATCC_JSON_PRINT_RESERVE;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->buf = FLATCC_JSON_PRINTER_ALLOC(buffer_size);
    ctx->own_buffer = 1;
    ctx->size = buffer_size;
    ctx->flush_size = ctx->size - FLATCC_JSON_PRINT_RESERVE;
    ctx->p = ctx->buf;
    ctx->pflush = ctx->buf + ctx->flush_size;
    ctx->flush = __flatcc_json_printer_flush_dynamic_buffer;
    if (!ctx->buf) {
        RAISE_ERROR(overflow);
        return -1;
    }
    return 0;
}

void *flatcc_json_printer_get_buffer(flatcc_json_printer_t *ctx, size_t *buffer_size)
{
    ctx->flush(ctx, 0);
    if (buffer_size) {
        *buffer_size = (size_t)(ctx->p - ctx->buf);
    }
    return ctx->buf;
}

void *flatcc_json_printer_finalize_dynamic_buffer(flatcc_json_printer_t *ctx, size_t *buffer_size)
{
    void *buffer;

    buffer = flatcc_json_printer_get_buffer(ctx, buffer_size);
    memset(ctx, 0, sizeof(*ctx));
    return buffer;
}

void flatcc_json_printer_clear(flatcc_json_printer_t *ctx)
{
    if (ctx->own_buffer && ctx->buf) {
        FLATCC_JSON_PRINTER_FREE(ctx->buf);
    }
    memset(ctx, 0, sizeof(*ctx));
}
