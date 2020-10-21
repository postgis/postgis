/*
 * Runtime support for verifying flatbuffers.
 *
 * Depends mutually on generated verifier functions for table types that
 * call into this library.
 */
#include <string.h>

#include "flatcc/flatcc_rtconfig.h"
#include "flatcc/flatcc_flatbuffers.h"
#include "flatcc/flatcc_verifier.h"
#include "flatcc/flatcc_identifier.h"

/* Customization for testing. */
#if FLATCC_DEBUG_VERIFY
#define FLATCC_VERIFIER_ASSERT_ON_ERROR 1
#include <stdio.h>
#define FLATCC_VERIFIER_ASSERT(cond, reason)                                \
    if (!(cond)) { fprintf(stderr, "verifier assert: %s\n",                 \
        flatcc_verify_error_string(reason)); FLATCC_ASSERT(0); return reason; }
#endif

#if FLATCC_TRACE_VERIFY
#include <stdio.h>
#define trace_verify(s, p) \
    fprintf(stderr, "trace verify: %s: 0x%02x\n", (s), (unsigned)(size_t)(p));
#else
#define trace_verify(s, p) ((void)0)
#endif

/* The runtime library does not use the global config file. */

/* This is a guideline, not an exact measure. */
#ifndef FLATCC_VERIFIER_MAX_LEVELS
#define FLATCC_VERIFIER_MAX_LEVELS 100
#endif

#ifndef FLATCC_VERIFIER_ASSERT_ON_ERROR
#define FLATCC_VERIFIER_ASSERT_ON_ERROR 0
#endif

/*
 * Generally a check should tell if a buffer is valid or not such
 * that runtime can take appropriate actions rather than crash,
 * also in debug, but assertions are helpful in debugging a problem.
 *
 * This must be compiled into the debug runtime library to take effect.
 */
#ifndef FLATCC_VERIFIER_ASSERT_ON_ERROR
#define FLATCC_VERIFIER_ASSERT_ON_ERROR 1
#endif

/* May be redefined for logging purposes. */
#ifndef FLATCC_VERIFIER_ASSERT
#define FLATCC_VERIFIER_ASSERT(cond, reason) FLATCC_ASSERT(cond)
#endif

#if FLATCC_VERIFIER_ASSERT_ON_ERROR
#define flatcc_verify(cond, reason) if (!(cond)) { FLATCC_VERIFIER_ASSERT(cond, reason); return reason; }
#else
#define flatcc_verify(cond, reason) if (!(cond)) { return reason; }
#endif


#define uoffset_t flatbuffers_uoffset_t
#define soffset_t flatbuffers_soffset_t
#define voffset_t flatbuffers_voffset_t
#define utype_t flatbuffers_utype_t
#define thash_t flatbuffers_thash_t

#define uoffset_size sizeof(uoffset_t)
#define soffset_size sizeof(soffset_t)
#define voffset_size sizeof(voffset_t)
#define utype_size sizeof(utype_t)
#define thash_size sizeof(thash_t)
#define offset_size uoffset_size

const char *flatcc_verify_error_string(int err)
{
    switch (err) {
#define XX(no, str)                                                         \
    case flatcc_verify_error_##no:                                          \
        return str;
        FLATCC_VERIFY_ERROR_MAP(XX)
#undef XX
    default:
        return "unknown";
    }
}

/* `cond` may have side effects. */
#define verify(cond, reason) do { int c = (cond); flatcc_verify(c, reason); } while(0)

/*
 * Identify checks related to runtime conditions (buffer size and
 * alignment) as seperate from those related to buffer content.
 */
#define verify_runtime(cond, reason) verify(cond, reason)

#define check_result(x) if (x) { return (x); }

#define check_field(td, id, required, base) do {                            \
    int ret = get_offset_field(td, id, required, &base);                    \
    if (ret || !base) { return ret; }} while (0)

static inline uoffset_t read_uoffset(const void *p, uoffset_t base)
{
    return __flatbuffers_uoffset_read_from_pe((uint8_t *)p + base);
}

static inline thash_t read_thash_identifier(const char *identifier)
{
    return flatbuffers_type_hash_from_string(identifier);
}

static inline thash_t read_thash(const void *p, uoffset_t base)
{
    return __flatbuffers_thash_read_from_pe((uint8_t *)p + base);
}

static inline voffset_t read_voffset(const void *p, uoffset_t base)
{
    return __flatbuffers_voffset_read_from_pe((uint8_t *)p + base);
}

static inline int check_header(uoffset_t end, uoffset_t base, uoffset_t offset)
{
    uoffset_t k = base + offset;

    if (uoffset_size <= voffset_size && k + offset_size < k) {
        return 0;
    }

    /* The `k > base` rather than `k >= base` is to avoid null offsets. */
    return k > base && k + offset_size <= end && !(k & (offset_size - 1));
}

static inline int check_aligned_header(uoffset_t end, uoffset_t base, uoffset_t offset, uint16_t align)
{
    uoffset_t k = base + offset;

    if (uoffset_size <= voffset_size && k + offset_size < k) {
        return 0;
    }
    /* Alignment refers to element 0 and header must also be aligned. */
    align = align < uoffset_size ? uoffset_size : align;

    /* Note to self: the builder can also use the mask OR trick to propagate `min_align`. */
    return k > base && k + offset_size <= end && !((k + offset_size) & ((offset_size - 1) | (align - 1u)));
}

static inline int verify_struct(uoffset_t end, uoffset_t base, uoffset_t offset, uoffset_t size, uint16_t align)
{
    /* Structs can have zero size so `end` is a valid value. */
    if (offset == 0 || base + offset > end) {
        return flatcc_verify_error_offset_out_of_range;
    }
    base += offset;
    verify(base + size >= base, flatcc_verify_error_struct_size_overflow);
    verify(base + size <= end, flatcc_verify_error_struct_out_of_range);
    verify (!(base & (align - 1u)), flatcc_verify_error_struct_unaligned);
    return flatcc_verify_ok;
}

static inline voffset_t read_vt_entry(flatcc_table_verifier_descriptor_t *td, voffset_t id)
{
    voffset_t vo = (id + 2u) * sizeof(voffset_t);

    /* Assumes tsize has been verified for alignment. */
    if (vo >= td->vsize) {
        return 0;
    }
    return read_voffset(td->vtable, vo);
}

static inline const void *get_field_ptr(flatcc_table_verifier_descriptor_t *td, voffset_t id)
{
    voffset_t vte = read_vt_entry(td, id);
    return vte ? (const uint8_t *)td->buf + td->table + vte : 0;
}

static int verify_field(flatcc_table_verifier_descriptor_t *td,
        voffset_t id, int required, uoffset_t size, uint16_t align)
{
    uoffset_t k, k2;
    voffset_t vte;
    uoffset_t base = (uoffset_t)(size_t)td->buf;


    /*
     * Otherwise range check assumptions break, and normal access code likely also.
     * We don't require voffset_size < uoffset_size, but some checks are faster if true.
     */
    FLATCC_ASSERT(uoffset_size >= voffset_size);
    FLATCC_ASSERT(soffset_size == uoffset_size);

    vte = read_vt_entry(td, id);
    if (!vte) {
        verify(!required, flatcc_verify_error_required_field_missing);
        return flatcc_verify_ok;
    }
    trace_verify("table buffer", td->buf);
    trace_verify("table", td->table);
    trace_verify("id", id);
    trace_verify("vte", vte);

    /*
     * Note that we don't add td.table to k and we test against table
     * size not table end or buffer end. Otherwise it would not be safe
     * to optimized out the k <= k2 check for normal uoffset and voffset
     * configurations.
     */
    k = vte;
    k2 = k + size;
    verify(k2 <= td->tsize, flatcc_verify_error_table_field_out_of_range);
    /* This normally optimizes to nop. */
    verify(uoffset_size > voffset_size || k <= k2, flatcc_verify_error_table_field_size_overflow);
    trace_verify("table + vte", vte + td->table);
    k += td->table + base;
    trace_verify("entry: buf + table + vte", k);
    trace_verify("align", align);
    trace_verify("align masked entry", k & (align - 1u));
    verify(!(k & (align - 1u)), flatcc_verify_error_table_field_not_aligned);
    /* We assume the table size has already been verified. */
    return flatcc_verify_ok;
}

static int get_offset_field(flatcc_table_verifier_descriptor_t *td, voffset_t id, int required, uoffset_t *out)
{
    uoffset_t k, k2;
    voffset_t vte;

    vte = read_vt_entry(td, id);
    if (!vte) {
        *out = 0;
        if (required) {
            return flatcc_verify_error_required_field_missing;
        }
        /* Missing, but not invalid. */
        return flatcc_verify_ok;
    }
    /*
     * Note that we don't add td.table to k and we test against table
     * size not table end or buffer end. Otherwise it would not be safe
     * to optimized out the k <= k2 check for normal uoffset and voffset
     * configurations.
     */
    k = vte;
    k2 = k + offset_size;
    verify(k2 <= td->tsize, flatcc_verify_error_table_field_out_of_range);
    /* This normally optimizes to nop. */
    verify(uoffset_size > voffset_size || k <= k2, flatcc_verify_error_table_field_size_overflow);
    k += td->table;
    verify(!(k & (offset_size - 1u)), flatcc_verify_error_table_field_not_aligned);
    /* We assume the table size has already been verified. */
    *out = k;
    return flatcc_verify_ok;
}

static inline int verify_string(const void *buf, uoffset_t end, uoffset_t base, uoffset_t offset)
{
    uoffset_t n;

    verify(check_header(end, base, offset), flatcc_verify_error_string_header_out_of_range_or_unaligned);
    base += offset;
    n = read_uoffset(buf, base);
    base += offset_size;
    verify(end - base >= n + 1, flatcc_verify_error_string_out_of_range);
    verify(((uint8_t *)buf + base)[n] == 0, flatcc_verify_error_string_not_zero_terminated);
    return flatcc_verify_ok;
}

/*
 * Keep interface somwewhat similar ot flatcc_builder_start_vector.
 * `max_count` is a precomputed division to manage overflow check on vector length.
 */
static inline int verify_vector(const void *buf, uoffset_t end, uoffset_t base, uoffset_t offset, uoffset_t elem_size, uint16_t align, uoffset_t max_count)
{
    uoffset_t n;

    verify(check_aligned_header(end, base, offset, align), flatcc_verify_error_vector_header_out_of_range_or_unaligned);
    base += offset;
    n = read_uoffset(buf, base);
    base += offset_size;
    /* `n * elem_size` can overflow uncontrollably otherwise. */
    verify(n <= max_count, flatcc_verify_error_vector_count_exceeds_representable_vector_size);
    verify(end - base >= n * elem_size, flatcc_verify_error_vector_out_of_range);
    return flatcc_verify_ok;
}

static inline int verify_string_vector(const void *buf, uoffset_t end, uoffset_t base, uoffset_t offset)
{
    uoffset_t i, n;

    check_result(verify_vector(buf, end, base, offset, offset_size, offset_size, FLATBUFFERS_COUNT_MAX(offset_size)));
    base += offset;
    n = read_uoffset(buf, base);
    base += offset_size;
    for (i = 0; i < n; ++i, base += offset_size) {
        check_result(verify_string(buf, end, base, read_uoffset(buf, base)));
    }
    return flatcc_verify_ok;
}

static inline int verify_table(const void *buf, uoffset_t end, uoffset_t base, uoffset_t offset,
        int ttl, flatcc_table_verifier_f tvf)
{
    uoffset_t vbase, vend;
    flatcc_table_verifier_descriptor_t td;

    verify((td.ttl = ttl - 1), flatcc_verify_error_max_nesting_level_reached);
    verify(check_header(end, base, offset), flatcc_verify_error_table_header_out_of_range_or_unaligned);
    td.table = base + offset;
    /* Read vtable offset - it is signed, but we want it unsigned, assuming 2's complement works. */
    vbase = td.table - read_uoffset(buf, td.table);
    verify((soffset_t)vbase >= 0 && !(vbase & (voffset_size - 1)), flatcc_verify_error_vtable_offset_out_of_range_or_unaligned);
    verify(vbase + voffset_size <= end, flatcc_verify_error_vtable_header_out_of_range);
    /* Read vtable size. */
    td.vsize = read_voffset(buf, vbase);
    vend = vbase + td.vsize;
    verify(vend <= end && !(td.vsize & (voffset_size - 1)), flatcc_verify_error_vtable_size_out_of_range_or_unaligned);
    /* Optimizes away overflow check if uoffset_t is large enough. */
    verify(uoffset_size > voffset_size || vend >= vbase, flatcc_verify_error_vtable_size_overflow);

    verify(td.vsize >= 2 * voffset_size, flatcc_verify_error_vtable_header_too_small);
    /* Read table size. */
    td.tsize = read_voffset(buf, vbase + voffset_size);
    verify(end - td.table >= td.tsize, flatcc_verify_error_table_size_out_of_range);
    td.vtable = (uint8_t *)buf + vbase;
    td.buf = buf;
    td.end = end;
    return tvf(&td);
}

static inline int verify_table_vector(const void *buf, uoffset_t end, uoffset_t base, uoffset_t offset, int ttl, flatcc_table_verifier_f tvf)
{
    uoffset_t i, n;

    verify(ttl-- > 0, flatcc_verify_error_max_nesting_level_reached);
    check_result(verify_vector(buf, end, base, offset, offset_size, offset_size, FLATBUFFERS_COUNT_MAX(offset_size)));
    base += offset;
    n = read_uoffset(buf, base);
    base += offset_size;
    for (i = 0; i < n; ++i, base += offset_size) {
        check_result(verify_table(buf, end, base, read_uoffset(buf, base), ttl, tvf));
    }
    return flatcc_verify_ok;
}

static inline int verify_union_vector(const void *buf, uoffset_t end, uoffset_t base, uoffset_t offset,
        uoffset_t count, const utype_t *types, int ttl, flatcc_union_verifier_f uvf)
{
    uoffset_t i, n, elem;
    flatcc_union_verifier_descriptor_t ud;

    verify(ttl-- > 0, flatcc_verify_error_max_nesting_level_reached);
    check_result(verify_vector(buf, end, base, offset, offset_size, offset_size, FLATBUFFERS_COUNT_MAX(offset_size)));
    base += offset;
    n = read_uoffset(buf, base);
    verify(n == count, flatcc_verify_error_union_vector_length_mismatch);
    base += offset_size;

    ud.buf = buf;
    ud.end = end;
    ud.ttl = ttl;

    for (i = 0; i < n; ++i, base += offset_size) {
        /* Table vectors can never be null, but unions can when the type is NONE. */
        elem = read_uoffset(buf, base);
        if (elem == 0) {
            verify(types[i] == 0, flatcc_verify_error_union_element_absent_without_type_NONE);
        } else {
            verify(types[i] != 0, flatcc_verify_error_union_element_present_with_type_NONE);
            ud.type = types[i];
            ud.base = base;
            ud.offset = elem;
            check_result(uvf(&ud));
        }
    }
    return flatcc_verify_ok;
}

int flatcc_verify_field(flatcc_table_verifier_descriptor_t *td,
        voffset_t id, size_t size, uint16_t align)
{
    check_result(verify_field(td, id, 0, (uoffset_t)size, align));
    return flatcc_verify_ok;
}

int flatcc_verify_string_field(flatcc_table_verifier_descriptor_t *td,
        voffset_t id, int required)
{
    uoffset_t base;

    check_field(td, id, required, base);
    return verify_string(td->buf, td->end, base, read_uoffset(td->buf, base));
}

int flatcc_verify_vector_field(flatcc_table_verifier_descriptor_t *td,
        voffset_t id, int required, size_t elem_size, uint16_t align, size_t max_count)
{
    uoffset_t base;

    check_field(td, id, required, base);
    return verify_vector(td->buf, td->end, base, read_uoffset(td->buf, base),
        (uoffset_t)elem_size, align, (uoffset_t)max_count);
}

int flatcc_verify_string_vector_field(flatcc_table_verifier_descriptor_t *td,
    voffset_t id, int required)
{
    uoffset_t base;

    check_field(td, id, required, base);
    return verify_string_vector(td->buf, td->end, base, read_uoffset(td->buf, base));
}

int flatcc_verify_table_field(flatcc_table_verifier_descriptor_t *td,
    voffset_t id, int required, flatcc_table_verifier_f tvf)
{
    uoffset_t base;

    check_field(td, id, required, base);
    return verify_table(td->buf, td->end, base, read_uoffset(td->buf, base), td->ttl, tvf);
}

int flatcc_verify_table_vector_field(flatcc_table_verifier_descriptor_t *td,
        voffset_t id, int required, flatcc_table_verifier_f tvf)
{
    uoffset_t base;

    check_field(td, id, required, base);
    return verify_table_vector(td->buf, td->end, base, read_uoffset(td->buf, base), td->ttl, tvf);
}

int flatcc_verify_union_table(flatcc_union_verifier_descriptor_t *ud, flatcc_table_verifier_f *tvf)
{
    return verify_table(ud->buf, ud->end, ud->base, ud->offset, ud->ttl, tvf);
}

int flatcc_verify_union_struct(flatcc_union_verifier_descriptor_t *ud, size_t size, uint16_t align)
{
    return verify_struct(ud->end, ud->base, ud->offset, (uoffset_t)size, align);
}

int flatcc_verify_union_string(flatcc_union_verifier_descriptor_t *ud)
{
    return verify_string(ud->buf, ud->end, ud->base, ud->offset);
}

int flatcc_verify_buffer_header(const void *buf, size_t bufsiz, const char *fid)
{
    thash_t id, id2;

    verify_runtime(!(((size_t)buf) & (offset_size - 1)), flatcc_verify_error_runtime_buffer_header_not_aligned);
    /* -8 ensures no scalar or offset field size can overflow. */
    verify_runtime(bufsiz <= FLATBUFFERS_UOFFSET_MAX - 8, flatcc_verify_error_runtime_buffer_size_too_large);
    /*
     * Even if we specify no fid, the user might later. Therefore
     * require space for it. Not all buffer generators will take this
     * into account, so it is possible to fail an otherwise valid buffer
     * - but such buffers aren't safe.
     */
    verify(bufsiz >= offset_size + FLATBUFFERS_IDENTIFIER_SIZE, flatcc_verify_error_buffer_header_too_small);
    if (fid != 0) {
        id2 = read_thash_identifier(fid);
        id = read_thash(buf, offset_size);
        verify(id2 == 0 || id == id2, flatcc_verify_error_identifier_mismatch);
    }
    return flatcc_verify_ok;
}

int flatcc_verify_typed_buffer_header(const void *buf, size_t bufsiz, flatbuffers_thash_t thash)
{
    thash_t id, id2;

    verify_runtime(!(((size_t)buf) & (offset_size - 1)), flatcc_verify_error_runtime_buffer_header_not_aligned);
    /* -8 ensures no scalar or offset field size can overflow. */
    verify_runtime(bufsiz <= FLATBUFFERS_UOFFSET_MAX - 8, flatcc_verify_error_runtime_buffer_size_too_large);
    /*
     * Even if we specify no fid, the user might later. Therefore
     * require space for it. Not all buffer generators will take this
     * into account, so it is possible to fail an otherwise valid buffer
     * - but such buffers aren't safe.
     */
    verify(bufsiz >= offset_size + FLATBUFFERS_IDENTIFIER_SIZE, flatcc_verify_error_buffer_header_too_small);
    if (thash != 0) {
        id2 = thash;
        id = read_thash(buf, offset_size);
        verify(id2 == 0 || id == id2, flatcc_verify_error_identifier_mismatch);
    }
    return flatcc_verify_ok;
}

int flatcc_verify_struct_as_root(const void *buf, size_t bufsiz, const char *fid, size_t size, uint16_t align)
{
    check_result(flatcc_verify_buffer_header(buf, bufsiz, fid));
    return verify_struct((uoffset_t)bufsiz, 0, read_uoffset(buf, 0), (uoffset_t)size, align);
}

int flatcc_verify_struct_as_typed_root(const void *buf, size_t bufsiz, flatbuffers_thash_t thash, size_t size, uint16_t align)
{
    check_result(flatcc_verify_typed_buffer_header(buf, bufsiz, thash));
    return verify_struct((uoffset_t)bufsiz, 0, read_uoffset(buf, 0), (uoffset_t)size, align);
}

int flatcc_verify_table_as_root(const void *buf, size_t bufsiz, const char *fid, flatcc_table_verifier_f *tvf)
{
    check_result(flatcc_verify_buffer_header(buf, (uoffset_t)bufsiz, fid));
    return verify_table(buf, (uoffset_t)bufsiz, 0, read_uoffset(buf, 0), FLATCC_VERIFIER_MAX_LEVELS, tvf);
}

int flatcc_verify_table_as_typed_root(const void *buf, size_t bufsiz, flatbuffers_thash_t thash, flatcc_table_verifier_f *tvf)
{
    check_result(flatcc_verify_typed_buffer_header(buf, (uoffset_t)bufsiz, thash));
    return verify_table(buf, (uoffset_t)bufsiz, 0, read_uoffset(buf, 0), FLATCC_VERIFIER_MAX_LEVELS, tvf);
}

int flatcc_verify_struct_as_nested_root(flatcc_table_verifier_descriptor_t *td,
        voffset_t id, int required, const char *fid, size_t size, uint16_t align)
{
    const uoffset_t *buf;
    uoffset_t bufsiz;

    check_result(flatcc_verify_vector_field(td, id, required, align, 1, FLATBUFFERS_COUNT_MAX(1)));
    if (0 == (buf = get_field_ptr(td, id))) {
        return flatcc_verify_ok;
    }
    buf = (const uoffset_t *)((size_t)buf + read_uoffset(buf, 0));
    bufsiz = read_uoffset(buf, 0);
    ++buf;
    return flatcc_verify_struct_as_root(buf, bufsiz, fid, size, align);
}

int flatcc_verify_table_as_nested_root(flatcc_table_verifier_descriptor_t *td,
        voffset_t id, int required, const char *fid,
        uint16_t align, flatcc_table_verifier_f tvf)
{
    const uoffset_t *buf;
    uoffset_t bufsiz;

    check_result(flatcc_verify_vector_field(td, id, required, align, 1, FLATBUFFERS_COUNT_MAX(1)));
    if (0 == (buf = get_field_ptr(td, id))) {
        return flatcc_verify_ok;
    }
    buf = (const uoffset_t *)((size_t)buf + read_uoffset(buf, 0));
    bufsiz = read_uoffset(buf, 0);
    ++buf;
    /*
     * Don't verify nested buffers identifier - information is difficult to get and
     * might not be what is desired anyway. User can do it later.
     */
    check_result(flatcc_verify_buffer_header(buf, bufsiz, fid));
    return verify_table(buf, bufsiz, 0, read_uoffset(buf, 0), td->ttl, tvf);
}

int flatcc_verify_union_field(flatcc_table_verifier_descriptor_t *td,
        voffset_t id, int required, flatcc_union_verifier_f uvf)
{
    voffset_t vte_type, vte_table;
    const uint8_t *type;
    uoffset_t base;
    flatcc_union_verifier_descriptor_t ud;

    if (0 == (vte_type = read_vt_entry(td, id - 1))) {
        vte_table = read_vt_entry(td, id);
        verify(vte_table == 0, flatcc_verify_error_union_cannot_have_a_table_without_a_type);
        verify(!required, flatcc_verify_error_type_field_absent_from_required_union_field);
        return flatcc_verify_ok;
    }
    /* No need to check required here. */
    check_result(verify_field(td, id - 1, 0, 1, 1));
    /* Only now is it safe to read the type. */
    vte_table = read_vt_entry(td, id);
    type = (const uint8_t *)td->buf + td->table + vte_type;
    verify(*type || vte_table == 0, flatcc_verify_error_union_type_NONE_cannot_have_a_value);

    if (*type == 0) {
        return flatcc_verify_ok;
    }
    check_field(td, id, required, base);
    ud.buf = td->buf;
    ud.end = td->end;
    ud.ttl = td->ttl;
    ud.base = base;
    ud.offset = read_uoffset(td->buf, base);
    ud.type = *type;
    return uvf(&ud);
}

int flatcc_verify_union_vector_field(flatcc_table_verifier_descriptor_t *td,
    flatbuffers_voffset_t id, int required, flatcc_union_verifier_f uvf)
{
    voffset_t vte_type, vte_table;
    const uoffset_t *buf;
    const utype_t *types;
    uoffset_t count, base;

    if (0 == (vte_type = read_vt_entry(td, id - 1))) {
        if (0 == (vte_table = read_vt_entry(td, id))) {
            verify(!required, flatcc_verify_error_type_field_absent_from_required_union_vector_field);
        }
    }
    check_result(flatcc_verify_vector_field(td, id - 1, required,
                utype_size, utype_size, FLATBUFFERS_COUNT_MAX(utype_size)));
    if (0 == (buf = get_field_ptr(td, id - 1))) {
        return flatcc_verify_ok;
    }
    buf = (const uoffset_t *)((size_t)buf + read_uoffset(buf, 0));
    count = read_uoffset(buf, 0);
    ++buf;
    types = (utype_t *)buf;

    check_field(td, id, required, base);
    return verify_union_vector(td->buf, td->end, base, read_uoffset(td->buf, base),
            count, types, td->ttl, uvf);
}
