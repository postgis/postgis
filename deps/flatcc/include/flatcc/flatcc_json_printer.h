#ifndef FLATCC_JSON_PRINTER_H
#define FLATCC_JSON_PRINTER_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Definitions for default implementation, do not assume these are
 * always valid.
 */
#define FLATCC_JSON_PRINT_FLUSH_SIZE (1024 * 16)
#define FLATCC_JSON_PRINT_RESERVE 64
#define FLATCC_JSON_PRINT_BUFFER_SIZE (FLATCC_JSON_PRINT_FLUSH_SIZE + FLATCC_JSON_PRINT_RESERVE)

#ifndef FLATCC_JSON_PRINTER_ALLOC
#define FLATCC_JSON_PRINTER_ALLOC(n) FLATCC_ALLOC(n)
#endif

#ifndef FLATCC_JSON_PRINTER_FREE
#define FLATCC_JSON_PRINTER_FREE(p) FLATCC_FREE(p)
#endif

#ifndef FLATCC_JSON_PRINTER_REALLOC
#define FLATCC_JSON_PRINTER_REALLOC(p, n) FLATCC_REALLOC(p, n)
#endif

/* Initial size that grows exponentially. */
#define FLATCC_JSON_PRINT_DYN_BUFFER_SIZE 4096


#include <stdlib.h>
#include <string.h>

#include "flatcc/flatcc_rtconfig.h"
#include "flatcc/flatcc_flatbuffers.h"

/* -DFLATCC_PORTABLE may help if inttypes.h is missing. */
#ifndef PRId64
#include <inttypes.h>
#endif

#define FLATCC_JSON_PRINT_ERROR_MAP(XX)                                     \
    XX(ok,                      "ok")                                       \
    /*                                                                      \
     * When the flatbuffer is null, has too small a header, or has          \
     * mismatching identifier when a match was requested.                   \
     */                                                                     \
    XX(bad_input,               "bad input")                                \
    XX(deep_recursion,          "deep recursion")                           \
    /*                                                                      \
     * When the output was larger than the available fixed length buffer,     \
     * or dynamic allocation could not grow the buffer sufficiently.        \
     */                                                                     \
    XX(overflow,                "overflow")

enum flatcc_json_printer_error_no {
#define XX(no, str) flatcc_json_printer_error_##no,
    FLATCC_JSON_PRINT_ERROR_MAP(XX)
#undef XX
};

#define flatcc_json_printer_ok flatcc_json_printer_error_ok

typedef struct flatcc_json_printer_ctx flatcc_json_printer_t;

typedef void flatcc_json_printer_flush_f(flatcc_json_printer_t *ctx, int all);

struct flatcc_json_printer_ctx {
    char *buf;
    size_t size;
    size_t flush_size;
    size_t total;
    const char *pflush;
    char *p;
    uint8_t own_buffer;
    uint8_t indent;
    uint8_t unquote;
    uint8_t noenum;
    uint8_t skip_default;
    uint8_t force_default;
    int level;
    int error;

    void *fp;
    flatcc_json_printer_flush_f *flush;
};

static inline void flatcc_json_printer_set_error(flatcc_json_printer_t *ctx, int err)
{
    if (!ctx->error) {
        ctx->error = err;
    }
}

const char *flatcc_json_printer_error_string(int err);

static inline int flatcc_json_printer_get_error(flatcc_json_printer_t *ctx)
{
    return ctx->error;
}

/*
 * Call to reuse context between parses without without
 * returning buffer. If a file pointer is being used,
 * it will remain open.
 *
 * Reset does not affect the formatting settings indentation, and
 * operational flags, but does zero the indentation level.
 */
static inline void flatcc_json_printer_reset(flatcc_json_printer_t *ctx)
{
    ctx->p = ctx->buf;
    ctx->level = 0;
    ctx->total = 0;
    ctx->error = 0;
}

/*
 * A custom init function can be implemented with a custom flush
 * function can be custom implemented. A few have been provided:
 * init with external fixed length buffer, and init with dynamically
 * growing buffer.
 *
 * Because there are a lot of small print functions, it is essentially
 * always faster to print to local buffer than moving to io directly
 * such as using fprintf or fwrite. The flush callback is used to
 * move data when enough has been collected.
 *
 * `fp` should be of type `FILE *` but we do not enforce it here
 * because it allows the header to be independent of <stdio.h>
 * when not required. If `fp` is null, it defaults to stdout.
 *
 * Returns -1 on alloc error (no cleanup needed), or 0 on success.
 * Eventually the clear method must be called to return memory.
 *
 * The file pointer may be stdout or a custom file. The file pointer
 * is not affected by reset or clear and should be closed manually.
 *
 * `set_flags` and related may be called subsequently to modify
 * behavior.
 */
int flatcc_json_printer_init(flatcc_json_printer_t *ctx, void *fp);

/*
 * Prints to external buffer and sets overflow error if buffer is too
 * small. Earlier content is then overwritten. A custom version of this
 * function could flush the content to elsewhere before allowing the
 * buffer content to be overwritten. The `buffers_size` must be large
 * enough to hold `FLATCC_JSON_PRINT_RESERVED_SIZE` which is small but
 * large enough value to hold entire numbers and the like.
 *
 * It is not strictly necessary to call clear because the buffer is
 * external, but still good form and case the context type is changed
 * later.
 *
 * Returns -1 on buffer size error (no cleanup needed), or 0 on success.
 *
 * `set_flags` and related may be called subsequently to modify
 * behavior.
 */
int flatcc_json_printer_init_buffer(flatcc_json_printer_t *ctx, char *buffer, size_t buffer_size);

/*
 * Returns the current buffer pointer and also the content size in
 * `buffer_size` if it is null. The operation is not very useful for
 * file oriented printers (created with `init`) and will then only
 * return the unflushed buffer content. For fixed length buffers
 * (`init_buffer`), only the last content is available if the buffer
 * overflowed. Works well with (`init_buffer`) when the dynamic buffer
 * is be reused, otherwise `finalize_dynamic_buffer` could be more
 * appropriate.
 *
 * The returned buffer is zero terminated.
 *
 * The returned pointer is only valid until next operation and should
 * not deallocated manually.
 */
void *flatcc_json_printer_get_buffer(flatcc_json_printer_t *ctx, size_t *buffer_size);

/*
 * Set to non-zero if names and enum symbols can be unquoted thus
 * diverging from standard JSON while remaining compatible with `flatc`
 * JSON flavor.
 */
static inline void flatcc_json_printer_set_unquoted(flatcc_json_printer_t *ctx, int x)
{
    ctx->unquote = !!x;
}

/*
 * Set to non-zero if enums should always be printed as numbers.
 * Otherwise enums are printed as a symbol for member values, and as
 * numbers for other values.
 *
 * NOTE: this setting will not affect code generated with enum mapping
 * disabled - statically disabling enum mapping is signficantly faster
 * for enums, less so for for union types.
 */
static inline void flatcc_json_printer_set_noenum(flatcc_json_printer_t *ctx, int x)
{
    ctx->noenum = !!x;
}

/*
 * Override priting an existing scalar field if it equals the default value.
 * Note that this setting is not mutually exclusive to `set_force_default`.
 */
static inline void flatcc_json_printer_set_skip_default(flatcc_json_printer_t *ctx, int x)
{
    ctx->skip_default = !!x;
}

/*
 * Override skipping absent scalar fields and print the default value.
 * Note that this setting is not mutually exclusive to `set_skip_default`.
 */
static inline void flatcc_json_printer_set_force_default(flatcc_json_printer_t *ctx, int x)
{
    ctx->force_default = !!x;
}


/*
 * Set pretty-print indentation in number of spaces. 0 (default) is
 * compact with no spaces or linebreaks (default), anything above
 * triggers pretty print.
 */
static inline void flatcc_json_printer_set_indent(flatcc_json_printer_t *ctx, uint8_t x)
{
    ctx->indent = x;
}

/*
 * Override the default compact valid JSON format with a
 * pretty printed non-strict version. Enums are translated
 * to names, which is also the default.
 */
static inline void flatcc_json_printer_set_nonstrict(flatcc_json_printer_t *ctx)
{
    flatcc_json_printer_set_indent(ctx, 2);
    flatcc_json_printer_set_unquoted(ctx, 1);
    flatcc_json_printer_set_noenum(ctx, 0);
}

enum flatcc_json_printer_flags {
    flatcc_json_printer_f_unquote = 1,
    flatcc_json_printer_f_noenum = 2,
    flatcc_json_printer_f_skip_default = 4,
    flatcc_json_printer_f_force_default = 8,
    flatcc_json_printer_f_pretty = 16,
    flatcc_json_printer_f_nonstrict = 32,
};

/*
 * May be called instead of setting operational modes individually.
 * Formatting is strict quoted json witout pretty printing by default.
 *
 * flags are:
 *
 *   `unquote`,
 *   `noenum`,
 *   `skip_default`,
 *   `force_default`,
 *   `pretty`,
 *   `nonstrict`
 *
 * `pretty` flag sets indentation to 2.
 * `nonstrict` implies: `noenum`, `unquote`, `pretty`.
 */
static inline void flatcc_json_printer_set_flags(flatcc_json_printer_t *ctx, int flags)
{
    ctx->unquote = !!(flags & flatcc_json_printer_f_unquote);
    ctx->noenum = !!(flags & flatcc_json_printer_f_noenum);
    ctx->skip_default = !!(flags & flatcc_json_printer_f_skip_default);
    ctx->force_default = !!(flags & flatcc_json_printer_f_force_default);
    if (flags & flatcc_json_printer_f_pretty) {
        flatcc_json_printer_set_indent(ctx, 2);
    }
    if (flags & flatcc_json_printer_f_nonstrict) {
        flatcc_json_printer_set_nonstrict(ctx);
    }
}


/*
 * Detects if the conctext type uses dynamically allocated memory
 * using malloc and realloc and frees any such memory.
 *
 * Not all context types needs to be cleared.
 */
void flatcc_json_printer_clear(flatcc_json_printer_t *ctx);

/*
 * Ensures that there ia always buffer capacity for priting the next
 * primitive with delimiters.
 *
 * Only flushes complete flush units and is inexpensive to call.
 * The content buffer has an extra reserve which ensures basic
 * data types and delimiters can always be printed after a partial
 * flush. At the end, a `flush` is required to flush the
 * remaining incomplete buffer data.
 *
 * Numbers do not call partial flush but will always fit into the reserve
 * capacity after a partial flush, also surrounded by delimiters.
 *
 * Variable length operations generally submit a partial flush so it is
 * safe to print a number after a name without flushing, but vectors of
 * numbers must (and do) issue a partial flush between elements. This is
 * handled automatically but must be considered if using the primitives
 * for special purposes. Because repeated partial flushes are very cheap
 * this is only a concern for high performance applications.
 *
 * When identiation is enabled, partial flush is also automatically
 * issued .
 */
static inline void flatcc_json_printer_flush_partial(flatcc_json_printer_t *ctx)
{
    if (ctx->p >= ctx->pflush) {
        ctx->flush(ctx, 0);
    }
}

/* Returns the total printed size but flushed and in buffer. */
static inline size_t flatcc_json_printer_total(flatcc_json_printer_t *ctx)
{
    return ctx->total + (size_t)(ctx->p - ctx->buf);
}

/*
 * Flush the remaining data not flushed by partial flush. It is valid to
 * call at any point if it is acceptable to have unaligned flush units,
 * but this is not desireable if, for example, compression or encryption
 * is added to the flush pipeline.
 *
 * Not called automatically at the end of printing a flatbuffer object
 * in case more data needs to be appended without submitting incomplete
 * flush units prematurely - for example adding a newline at the end.
 *
 * The flush behavior depeends on the underlying `ctx` object, for
 * example dynamic buffers have no distinction between partial and full
 * flushes - here it is merely ensured that the buffer always has a
 * reserve capacity left.
 *
 * Returns the total printed size.
 */
static inline size_t flatcc_json_printer_flush(flatcc_json_printer_t *ctx)
{
    ctx->flush(ctx, 1);
    return flatcc_json_printer_total(ctx);
}

/*
 * Helper functions to print anything into the json buffer.
 * Strings are escaped.
 *
 * When pretty printing (indent > 0), level 0 has special significance -
 * so if wrapping printed json in a manually printed container json
 * object, these functions can help manage this.
 */

/* Escaped and quoted string. */
void flatcc_json_printer_string(flatcc_json_printer_t *ctx, const char *s, size_t n);
/* Unescaped and unquoted string. */
void flatcc_json_printer_write(flatcc_json_printer_t *ctx, const char *s, size_t n);
/* Print a newline and issues a partial flush. */
void flatcc_json_printer_nl(flatcc_json_printer_t *ctx);
/* Like numbers, a partial flush is not issued. */
void flatcc_json_printer_char(flatcc_json_printer_t *ctx, char c);
/* Indents and issues a partial flush. */
void flatcc_json_printer_indent(flatcc_json_printer_t *ctx);
/* Adjust identation level, usually +/-1. */
void flatcc_json_printer_add_level(flatcc_json_printer_t *ctx, int n);
/* Returns current identation level (0 is top level). */
int flatcc_json_printer_get_level(flatcc_json_printer_t *ctx);

/*
 * If called explicitly be aware that repeated calls to numeric
 * printers may cause buffer overflow without flush in-between.
 */
void flatcc_json_printer_uint8(flatcc_json_printer_t *ctx, uint8_t v);
void flatcc_json_printer_uint16(flatcc_json_printer_t *ctx, uint16_t v);
void flatcc_json_printer_uint32(flatcc_json_printer_t *ctx, uint32_t v);
void flatcc_json_printer_uint64(flatcc_json_printer_t *ctx, uint64_t v);
void flatcc_json_printer_int8(flatcc_json_printer_t *ctx, int8_t v);
void flatcc_json_printer_int16(flatcc_json_printer_t *ctx, int16_t v);
void flatcc_json_printer_int32(flatcc_json_printer_t *ctx, int32_t v);
void flatcc_json_printer_int64(flatcc_json_printer_t *ctx, int64_t v);
void flatcc_json_printer_bool(flatcc_json_printer_t *ctx, int v);
void flatcc_json_printer_float(flatcc_json_printer_t *ctx, float v);
void flatcc_json_printer_double(flatcc_json_printer_t *ctx, double v);

void flatcc_json_printer_enum(flatcc_json_printer_t *ctx,
        const char *symbol, size_t len);

/*
 * Convenience function to add a trailing newline, flush the buffer,
 * test for error and reset the context for reuse.
 *
 * Returns total size printed or < 0 on error.
 *
 * This function makes most sense for file oriented output.
 * See also `finalize_dynamic_buffer`.
 */
static inline int flatcc_json_printer_finalize(flatcc_json_printer_t *ctx)
{
    int ret;
    flatcc_json_printer_nl(ctx);
    ret = (int)flatcc_json_printer_flush(ctx);
    if (ctx->error) {
        ret = -1;
    }
    flatcc_json_printer_reset(ctx);
    return ret;
}

/*
 * Allocates a small buffer and grows it dynamically.
 * Buffer survives past reset. To reduce size between uses, call clear
 * followed by init call. To reuse buffer just call reset between uses.
 * If `buffer_size` is 0 a sensible default is being used. The size is
 * automatically rounded up to reserved size if too small.
 *
 * Returns -1 on alloc error (no cleanup needed), or 0 on success.
 * Eventually the clear method must be called to return memory.
 *
 * `set_flags` and related may be called subsequently to modify
 * behavior.
 */
int flatcc_json_printer_init_dynamic_buffer(flatcc_json_printer_t *ctx, size_t buffer_size);

/*
 * Similar to calling `finalize` but returns the buffer and does NOT
 * reset, but rather clears printer object and the returned buffer must
 * be deallocated with `free`.
 *
 * The returned buffer is zero terminated.
 *
 * NOTE: it is entirely optional to use this method. For repeated used
 * of dynamic buffers, `newline` (or not) followed by `get_buffer`
 * and `reset` will be an alternative.
 *
 * Stores the printed buffer size in `buffer_size` if it is not null.
 *
 * See also `get_dynamic_buffer`.
 */
void *flatcc_json_printer_finalize_dynamic_buffer(flatcc_json_printer_t *ctx, size_t *buffer_size);


/*************************************************************
 * The following is normally only used by generated code.
 *************************************************************/

typedef struct flatcc_json_printer_table_descriptor flatcc_json_printer_table_descriptor_t;

struct flatcc_json_printer_table_descriptor {
    const void *table;
    const void *vtable;
    int vsize;
    int ttl;
    int count;
};

typedef struct flatcc_json_printer_union_descriptor flatcc_json_printer_union_descriptor_t;

struct flatcc_json_printer_union_descriptor {
    const void *member;
    int ttl;
    uint8_t type;
};

typedef void flatcc_json_printer_table_f(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td);

typedef void flatcc_json_printer_struct_f(flatcc_json_printer_t *ctx,
        const void *p);

typedef void flatcc_json_printer_union_f(flatcc_json_printer_t *ctx,
        flatcc_json_printer_union_descriptor_t *ud);

/* Generated value to name map callbacks. */
typedef void flatcc_json_printer_union_type_f(flatcc_json_printer_t *ctx, flatbuffers_utype_t type);
typedef void flatcc_json_printer_uint8_enum_f(flatcc_json_printer_t *ctx, uint8_t v);
typedef void flatcc_json_printer_uint16_enum_f(flatcc_json_printer_t *ctx, uint16_t v);
typedef void flatcc_json_printer_uint32_enum_f(flatcc_json_printer_t *ctx, uint32_t v);
typedef void flatcc_json_printer_uint64_enum_f(flatcc_json_printer_t *ctx, uint64_t v);
typedef void flatcc_json_printer_int8_enum_f(flatcc_json_printer_t *ctx, int8_t v);
typedef void flatcc_json_printer_int16_enum_f(flatcc_json_printer_t *ctx, int16_t v);
typedef void flatcc_json_printer_int32_enum_f(flatcc_json_printer_t *ctx, int32_t v);
typedef void flatcc_json_printer_int64_enum_f(flatcc_json_printer_t *ctx, int64_t v);
typedef void flatcc_json_printer_bool_enum_f(flatcc_json_printer_t *ctx, flatbuffers_bool_t v);

#define __define_print_scalar_field_proto(TN, T)                            \
void flatcc_json_printer_ ## TN ## _field(flatcc_json_printer_t *ctx,       \
        flatcc_json_printer_table_descriptor_t *td,                         \
        int id, const char *name, size_t len, T v);

#define __define_print_scalar_optional_field_proto(TN, T)                   \
void flatcc_json_printer_ ## TN ## _optional_field(                         \
        flatcc_json_printer_t *ctx,                                         \
        flatcc_json_printer_table_descriptor_t *td,                         \
        int id, const char *name, size_t len);

#define __define_print_scalar_struct_field_proto(TN, T)                     \
void flatcc_json_printer_ ## TN ## _struct_field(flatcc_json_printer_t *ctx,\
        int index, const void *p, size_t offset,                            \
        const char *name, size_t len);

#define __define_print_scalar_array_struct_field_proto(TN, T)               \
void flatcc_json_printer_ ## TN ## _array_struct_field(                     \
        flatcc_json_printer_t *ctx,                                         \
        int index, const void *p, size_t offset,                            \
        const char *name, size_t len, size_t count);

#define __define_print_enum_array_struct_field_proto(TN, T)                 \
void flatcc_json_printer_ ## TN ## _enum_array_struct_field(                \
        flatcc_json_printer_t *ctx,                                         \
        int index, const void *p, size_t offset,                            \
        const char *name, size_t len, size_t count,                         \
        flatcc_json_printer_ ## TN ##_enum_f *pf);

#define __define_print_enum_struct_field_proto(TN, T)                       \
void flatcc_json_printer_ ## TN ## _enum_struct_field(                      \
        flatcc_json_printer_t *ctx,                                         \
        int index, const void *p, size_t offset,                            \
        const char *name, size_t len,                                       \
        flatcc_json_printer_ ## TN ##_enum_f *pf);

#define __define_print_enum_field_proto(TN, T)                              \
void flatcc_json_printer_ ## TN ## _enum_field(flatcc_json_printer_t *ctx,  \
        flatcc_json_printer_table_descriptor_t *td,                         \
        int id, const char *name, size_t len, T v,                          \
        flatcc_json_printer_ ## TN ##_enum_f *pf);

#define __define_print_enum_optional_field_proto(TN, T)                     \
void flatcc_json_printer_ ## TN ## _enum_optional_field(                    \
        flatcc_json_printer_t *ctx,                                         \
        flatcc_json_printer_table_descriptor_t *td,                         \
        int id, const char *name, size_t len,                               \
        flatcc_json_printer_ ## TN ##_enum_f *pf);

#define __define_print_scalar_vector_field_proto(TN, T)                     \
void flatcc_json_printer_ ## TN ## _vector_field(flatcc_json_printer_t *ctx,\
        flatcc_json_printer_table_descriptor_t *td,                         \
        int id, const char *name, size_t len);

#define __define_print_enum_vector_field_proto(TN, T)                       \
void flatcc_json_printer_ ## TN ## _enum_vector_field(                      \
        flatcc_json_printer_t *ctx,                                         \
        flatcc_json_printer_table_descriptor_t *td,                         \
        int id, const char *name, size_t len,                               \
        flatcc_json_printer_ ## TN ##_enum_f *pf);

__define_print_scalar_field_proto(uint8, uint8_t)
__define_print_scalar_field_proto(uint16, uint16_t)
__define_print_scalar_field_proto(uint32, uint32_t)
__define_print_scalar_field_proto(uint64, uint64_t)
__define_print_scalar_field_proto(int8, int8_t)
__define_print_scalar_field_proto(int16, int16_t)
__define_print_scalar_field_proto(int32, int32_t)
__define_print_scalar_field_proto(int64, int64_t)
__define_print_scalar_field_proto(bool, flatbuffers_bool_t)
__define_print_scalar_field_proto(float, float)
__define_print_scalar_field_proto(double, double)

__define_print_enum_field_proto(uint8, uint8_t)
__define_print_enum_field_proto(uint16, uint16_t)
__define_print_enum_field_proto(uint32, uint32_t)
__define_print_enum_field_proto(uint64, uint64_t)
__define_print_enum_field_proto(int8, int8_t)
__define_print_enum_field_proto(int16, int16_t)
__define_print_enum_field_proto(int32, int32_t)
__define_print_enum_field_proto(int64, int64_t)
__define_print_enum_field_proto(bool, flatbuffers_bool_t)

__define_print_scalar_optional_field_proto(uint8, uint8_t)
__define_print_scalar_optional_field_proto(uint16, uint16_t)
__define_print_scalar_optional_field_proto(uint32, uint32_t)
__define_print_scalar_optional_field_proto(uint64, uint64_t)
__define_print_scalar_optional_field_proto(int8, int8_t)
__define_print_scalar_optional_field_proto(int16, int16_t)
__define_print_scalar_optional_field_proto(int32, int32_t)
__define_print_scalar_optional_field_proto(int64, int64_t)
__define_print_scalar_optional_field_proto(bool, flatbuffers_bool_t)
__define_print_scalar_optional_field_proto(float, float)
__define_print_scalar_optional_field_proto(double, double)

__define_print_enum_optional_field_proto(uint8, uint8_t)
__define_print_enum_optional_field_proto(uint16, uint16_t)
__define_print_enum_optional_field_proto(uint32, uint32_t)
__define_print_enum_optional_field_proto(uint64, uint64_t)
__define_print_enum_optional_field_proto(int8, int8_t)
__define_print_enum_optional_field_proto(int16, int16_t)
__define_print_enum_optional_field_proto(int32, int32_t)
__define_print_enum_optional_field_proto(int64, int64_t)
__define_print_enum_optional_field_proto(bool, flatbuffers_bool_t)

__define_print_scalar_struct_field_proto(uint8, uint8_t)
__define_print_scalar_struct_field_proto(uint16, uint16_t)
__define_print_scalar_struct_field_proto(uint32, uint32_t)
__define_print_scalar_struct_field_proto(uint64, uint64_t)
__define_print_scalar_struct_field_proto(int8, int8_t)
__define_print_scalar_struct_field_proto(int16, int16_t)
__define_print_scalar_struct_field_proto(int32, int32_t)
__define_print_scalar_struct_field_proto(int64, int64_t)
__define_print_scalar_struct_field_proto(bool, flatbuffers_bool_t)
__define_print_scalar_struct_field_proto(float, float)
__define_print_scalar_struct_field_proto(double, double)

/*
 * char arrays are special as there are no char fields
 * without arrays and because they are printed as strings.
 */
__define_print_scalar_array_struct_field_proto(char, char)

__define_print_scalar_array_struct_field_proto(uint8, uint8_t)
__define_print_scalar_array_struct_field_proto(uint16, uint16_t)
__define_print_scalar_array_struct_field_proto(uint32, uint32_t)
__define_print_scalar_array_struct_field_proto(uint64, uint64_t)
__define_print_scalar_array_struct_field_proto(int8, int8_t)
__define_print_scalar_array_struct_field_proto(int16, int16_t)
__define_print_scalar_array_struct_field_proto(int32, int32_t)
__define_print_scalar_array_struct_field_proto(int64, int64_t)
__define_print_scalar_array_struct_field_proto(bool, flatbuffers_bool_t)
__define_print_scalar_array_struct_field_proto(float, float)
__define_print_scalar_array_struct_field_proto(double, double)

__define_print_enum_array_struct_field_proto(uint8, uint8_t)
__define_print_enum_array_struct_field_proto(uint16, uint16_t)
__define_print_enum_array_struct_field_proto(uint32, uint32_t)
__define_print_enum_array_struct_field_proto(uint64, uint64_t)
__define_print_enum_array_struct_field_proto(int8, int8_t)
__define_print_enum_array_struct_field_proto(int16, int16_t)
__define_print_enum_array_struct_field_proto(int32, int32_t)
__define_print_enum_array_struct_field_proto(int64, int64_t)
__define_print_enum_array_struct_field_proto(bool, flatbuffers_bool_t)

__define_print_enum_struct_field_proto(uint8, uint8_t)
__define_print_enum_struct_field_proto(uint16, uint16_t)
__define_print_enum_struct_field_proto(uint32, uint32_t)
__define_print_enum_struct_field_proto(uint64, uint64_t)
__define_print_enum_struct_field_proto(int8, int8_t)
__define_print_enum_struct_field_proto(int16, int16_t)
__define_print_enum_struct_field_proto(int32, int32_t)
__define_print_enum_struct_field_proto(int64, int64_t)
__define_print_enum_struct_field_proto(bool, flatbuffers_bool_t)

__define_print_scalar_vector_field_proto(uint8, uint8_t)
__define_print_scalar_vector_field_proto(uint16, uint16_t)
__define_print_scalar_vector_field_proto(uint32, uint32_t)
__define_print_scalar_vector_field_proto(uint64, uint64_t)
__define_print_scalar_vector_field_proto(int8, int8_t)
__define_print_scalar_vector_field_proto(int16, int16_t)
__define_print_scalar_vector_field_proto(int32, int32_t)
__define_print_scalar_vector_field_proto(int64, int64_t)
__define_print_scalar_vector_field_proto(bool, flatbuffers_bool_t)
__define_print_scalar_vector_field_proto(float, float)
__define_print_scalar_vector_field_proto(double, double)

__define_print_enum_vector_field_proto(uint8, uint8_t)
__define_print_enum_vector_field_proto(uint16, uint16_t)
__define_print_enum_vector_field_proto(uint32, uint32_t)
__define_print_enum_vector_field_proto(uint64, uint64_t)
__define_print_enum_vector_field_proto(int8, int8_t)
__define_print_enum_vector_field_proto(int16, int16_t)
__define_print_enum_vector_field_proto(int32, int32_t)
__define_print_enum_vector_field_proto(int64, int64_t)
__define_print_enum_vector_field_proto(bool, flatbuffers_bool_t)

void flatcc_json_printer_uint8_vector_base64_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len, int urlsafe);

/*
 * If `fid` is null, the identifier is not checked and is allowed to be
 * entirely absent.
 *
 * The buffer must at least be aligned to uoffset_t on systems that
 * require aligned memory addresses (as always for flatbuffers).
 */
int flatcc_json_printer_table_as_root(flatcc_json_printer_t *ctx,
        const void *buf, size_t bufsiz, const char *fid,
        flatcc_json_printer_table_f *pf);

int flatcc_json_printer_struct_as_root(flatcc_json_printer_t *ctx,
        const void *buf, size_t bufsiz, const char *fid,
        flatcc_json_printer_struct_f *pf);

/*
 * Call before and after enum flags to ensure proper quotation. Enum
 * quotes may be configured runtime, but regardless of this, multiple
 * flags may be forced to be quoted depending on compile time flag since
 * not all parsers may be able to handle unquoted space separated values
 * even if they handle non-strict unquoted json otherwise.
 *
 * Flags should only be called when not empty (0) and when there are no
 * unknown flags in the value. Otherwise print the numeric value. The
 * auto generated code deals with this.
 *
 * This bit twiddling hack may be useful:
 *
 *     `multiple = 0 != (v & (v - 1);`
 */
void flatcc_json_printer_delimit_enum_flags(flatcc_json_printer_t *ctx, int multiple);

/* The index increments from 0 to handle space. It is not the flag bit position. */
void flatcc_json_printer_enum_flag(flatcc_json_printer_t *ctx, int index, const char *symbol, size_t len);

/* A struct inside another struct, as opposed to inside a table or a root. */
void flatcc_json_printer_embedded_struct_field(flatcc_json_printer_t *ctx,
        int index, const void *p, size_t offset,
        const char *name, size_t len,
        flatcc_json_printer_struct_f pf);

void flatcc_json_printer_embedded_struct_array_field(flatcc_json_printer_t *ctx,
        int index, const void *p, size_t offset,
        const char *name, size_t len,
        size_t size, size_t count,
        flatcc_json_printer_struct_f pf);

void flatcc_json_printer_struct_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len,
        flatcc_json_printer_struct_f *pf);

void flatcc_json_printer_string_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len);

void flatcc_json_printer_string_vector_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len);

void flatcc_json_printer_table_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len,
        flatcc_json_printer_table_f pf);

void flatcc_json_printer_struct_vector_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len,
        size_t size,
        flatcc_json_printer_struct_f pf);

void flatcc_json_printer_table_vector_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len,
        flatcc_json_printer_table_f pf);

void flatcc_json_printer_union_vector_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len,
        flatcc_json_printer_union_type_f ptf,
        flatcc_json_printer_union_f pf);

void flatcc_json_printer_struct_as_nested_root(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len,
        const char *fid,
        flatcc_json_printer_struct_f *pf);

void flatcc_json_printer_table_as_nested_root(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len,
        const char *fid,
        flatcc_json_printer_table_f pf);

void flatcc_json_printer_union_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, size_t len,
        flatcc_json_printer_union_type_f ptf,
        flatcc_json_printer_union_f pf);

void flatcc_json_printer_union_table(flatcc_json_printer_t *ctx,
        flatcc_json_printer_union_descriptor_t *ud,
        flatcc_json_printer_table_f pf);

void flatcc_json_printer_union_struct(flatcc_json_printer_t *ctx,
        flatcc_json_printer_union_descriptor_t *ud,
        flatcc_json_printer_struct_f pf);

void flatcc_json_printer_union_string(flatcc_json_printer_t *ctx,
        flatcc_json_printer_union_descriptor_t *ud);

#ifdef __cplusplus
}
#endif

#endif /* FLATCC_JSON_PRINTER_H */
