#ifndef PBASE64_H
#define PBASE64_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

/* Guarded to allow inclusion of pstdint.h first, if stdint.h is not supported. */
#ifndef UINT8_MAX
#include <stdint.h>
#endif

#define BASE64_EOK    0
/* 0 or mure full blocks decoded, remaining content may be parsed with fresh buffer. */
#define BASE64_EMORE  1
/* The `src_len` argument is required when encoding. */
#define BASE64_EARGS  2
/* Unsupported mode, or modifier not supported by mode when encoding. */
#define BASE64_EMODE  3
/* Decoding ends at invalid tail length - either by source length or by non-alphabet symbol. */
#define BASE64_ETAIL  4
/* Decoding ends at valid tail length but last byte has non-zero bits where it shouldn't have. */
#define BASE64_EDIRTY 5

static inline const char *base64_strerror(int err);

/* All codecs are URL safe. Only Crockford allow for non-canocical decoding. */
enum {
    /* Most common base64 codec, but not url friendly. */
    base64_mode_rfc4648 = 0,

    /*  URL safe version, '+' -> '-', '/' -> '_'. */
    base64_mode_url = 1,

    /*
     * Skip ' ', '\r', and '\n' - we do not allow tab because common
     * uses of base64 such as PEM do not allow tab.
     */
    base64_dec_modifier_skipspace = 32,

    /* Padding is excluded by default. Not allowed for zbase64. */
    base64_enc_modifier_padding = 128,

    /* For internal use or to decide codec of mode. */
    base64_modifier_mask = 32 + 64 + 128,
};

/* Encoded size with or without padding. */
static inline size_t base64_encoded_size(size_t len, int mode);

/*
 * Decoded size assuming no padding.
 * If `len` does include padding, the actual size may be less
 * when decoding, but never more.
 */
static inline size_t base64_decoded_size(size_t len);

/*
 * `dst` must hold ceil(len * 4 / 3) bytes.
 * `src_len` points to length of source and is updated with length of
 * parse on both success and failure. If `dst_len` is not null
 * it is used to store resulting output lengt withh length of decoded
 * output on both success and failure.
 * If `hyphen` is non-zero a hyphen is encoded every `hyphen` output bytes.
 * `mode` selects encoding alphabet defaulting to Crockfords base64.
 * Returns 0 on success.
 *
 * A terminal space can be added with `dst[dst_len++] = ' '` after the
 * encode call. All non-alphabet can be used as terminators except the
 * padding character '='. The following characters will work as
 * terminator for all modes: { '\0', '\n', ' ', '\t' }. A terminator is
 * optional when the source length is given to the decoder. Note that
 * crockford also reserves a few extra characters for checksum but the
 * checksum must be separate from the main buffer and is not supported
 * by this library.
 */
static inline int base64_encode(uint8_t *dst, const uint8_t *src, size_t *dst_len, size_t *src_len, int mode);

/*
 * Decodes according to mode while ignoring encoding modifiers.
 * `src_len` and `dst_len` are optional pointers. If `src_len` is set it
 * must contain the length of the input, otherwise the input must be
 * terminated with a non-alphabet character or valid padding (a single
 * padding character is accepted) - if the src_len output is needed but
 * not the input due to guaranteed termination, then set it to
 * (size_t)-1. `dst_len` must contain length of output buffer if present
 * and parse will fail with BASE64_EMORE after decoding a block multiple
 * if dst_len is exhausted - the parse can thus be resumed after
 * draining destination. `src_len` and `dst_len` are updated with parsed
 * and decoded length, when present, on both success and failure.
 * Returns 0 on success. Invalid characters are not considered errors -
 * they simply terminate the parse, however, if the termination is not
 * at a block multiple or a valid partial block length then BASE64_ETAIL
 * without output holding the last full block, if any. BASE64_ETAIL is also
 * returned if the a valid length holds non-zero unused tail bits.
 */
static inline int base64_decode(uint8_t *dst, const uint8_t *src, size_t *dst_len, size_t *src_len, int mode);

static inline const char *base64_strerror(int err)
{
    switch (err) {
    case BASE64_EOK: return "ok";
    case BASE64_EARGS: return "invalid argument";
    case BASE64_EMODE: return "invalid mode";
    case BASE64_EMORE: return "destination full";
    case BASE64_ETAIL: return "invalid tail length";
    case BASE64_EDIRTY: return "invalid tail content";
    default: return "unknown error";
    }
}

static inline size_t base64_encoded_size(size_t len, int mode)
{
    size_t k = len % 3;
    size_t n = (len * 4 / 3 + 3) & ~(size_t)3;
    int pad = mode & base64_enc_modifier_padding;

    if (!pad) {
        switch (k) {
        case 2:
            n -= 1;
            break;
        case 1:
            n -= 2;
            break;
        default:
            break;
        }
    }
    return n;
}

static inline size_t base64_decoded_size(size_t len)
{
    size_t k = len % 4;
    size_t n = len / 4 * 3;

    switch (k) {
    case 3:
        return n + 2;
    case 2:
        return n + 1;
    case 1: /* Not valid without padding. */
    case 0:
    default:
        return n;
    }
}

static inline int base64_encode(uint8_t *dst, const uint8_t *src, size_t *dst_len, size_t *src_len, int mode)
{
    const uint8_t *rfc4648_alphabet            = (const uint8_t *)
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const uint8_t *url_alphabet                = (const uint8_t *)
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

    const uint8_t *T;
    uint8_t *dst_base = dst;
    int pad = mode & base64_enc_modifier_padding;
    size_t len = 0;
    int ret = BASE64_EMODE;

    if (!src_len) {
        ret = BASE64_EARGS;
        goto done;
    }
    len = *src_len;
    mode = mode & ~base64_modifier_mask;
    switch (mode) {
    case base64_mode_rfc4648:
        T = rfc4648_alphabet;
        break;
    case base64_mode_url:
        T = url_alphabet;
        break;
    default:
        /* Invalid mode. */
        goto done;
    }

    ret = BASE64_EOK;

    /* Encodes 4 destination bytes from 3 source bytes. */
    while (len >= 3) {
        dst[0] = T[((src[0] >> 2))];
        dst[1] = T[((src[0] << 4) & 0x30) | (src[1] >> 4)];
        dst[2] = T[((src[1] << 2) & 0x3c) | (src[2] >> 6)];
        dst[3] = T[((src[2] & 0x3f))];
        len -= 3;
        dst += 4;
        src += 3;
    }
    /* Encodes 8 destination bytes from 1 to 4 source bytes, if any. */
    switch(len) {
    case 2:
        dst[0] = T[((src[0] >> 2))];
        dst[1] = T[((src[0] << 4) & 0x30) | (src[1] >> 4)];
        dst[2] = T[((src[1] << 2) & 0x3c)];
        dst += 3;
        if (pad) {
            *dst++ = '=';
        }
        break;
    case 1:
        dst[0] = T[((src[0] >> 2))];
        dst[1] = T[((src[0] << 4) & 0x30)];
        dst += 2;
        if (pad) {
            *dst++ = '=';
            *dst++ = '=';
        }
        break;
    default:
        pad = 0;
        break;
    }
    len = 0;
done:
    if (dst_len) {
        *dst_len = (size_t)(dst - dst_base);
    }
    if (src_len) {
        *src_len -= len;
    }
    return ret;
}

static inline int base64_decode(uint8_t *dst, const uint8_t *src, size_t *dst_len, size_t *src_len, int mode)
{
    static const uint8_t cinvalid = 64;
    static const uint8_t cignore = 65;
    static const uint8_t cpadding = 66;

    /*
     * 0..63: 6-bit encoded value.
     * 64: flags non-alphabet symbols.
     * 65: codes for ignored symbols.
     * 66: codes for pad symbol '='.
     * All codecs consider padding an optional terminator and if present
     * consumes as many pad bytes as possible up to block termination,
     * but does not fail if a block is not full.
     *
     * We do not currently have any ignored characters but we might
     * add spaces as per MIME spec, but  assuming spaces only happen
     * at block boundaries this is probalby better handled by repeated
     * parsing.
     */
    static const uint8_t base64rfc4648_decode[256] = {
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 66, 64, 64,
        64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
        64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
    };

    static const uint8_t base64url_decode[256] = {
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 66, 64, 64,
        64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 63,
        64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
    };

    static const uint8_t base64rfc4648_decode_skipspace[256] = {
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 65, 64, 64, 65, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        65, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 66, 64, 64,
        64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
        64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
    };

    static const uint8_t base64url_decode_skipspace[256] = {
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 65, 64, 64, 65, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        65, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 66, 64, 64,
        64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 63,
        64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
    };

    int ret = BASE64_EOK;
    size_t i, k;
    uint8_t hold[4];
    uint8_t *dst_base = dst;
    size_t limit = (size_t)-1;
    size_t len = (size_t)-1, mark;
    const uint8_t *T = base64rfc4648_decode;
    int skipspace = mode & base64_dec_modifier_skipspace;

    if (src_len) {
        len = *src_len;
    }
    mark = len;
    mode = mode & ~base64_modifier_mask;
    switch (mode) {
    case base64_mode_rfc4648:
        T = skipspace ? base64rfc4648_decode_skipspace : base64rfc4648_decode;
        break;
    case base64_mode_url:
        T = skipspace ?  base64url_decode_skipspace : base64url_decode;
        break;
    default:
        ret = BASE64_EMODE;
        goto done;
    }

    if (dst_len && *dst_len > 0) {
        limit = *dst_len;
    }
    while(limit > 0) {
        for (i = 0; i < 4; ++i) {
            if (len == i) {
                k = i;
                len -= i;
                goto tail;
            }
            if ((hold[i] = T[src[i]]) >= cinvalid) {
                if (hold[i] == cignore) {
                    ++src;
                    --len;
                    --i;
                    continue;
                }
                k = i;
                /* Strip padding and ignore hyphen in padding, if present. */
                if (hold[i] == cpadding) {
                    ++i;
                    while (i < len && i < 8) {
                        if (T[src[i]] != cpadding && T[src[i]] != cignore) {
                            break;
                        }
                        ++i;
                    }
                }
                len -= i;
                goto tail;
            }
        }
        if (limit < 3) {
            goto more;
        }
        dst[0] = (uint8_t)((hold[0] << 2) | (hold[1] >> 4));
        dst[1] = (uint8_t)((hold[1] << 4) | (hold[2] >> 2));
        dst[2] = (uint8_t)((hold[2] << 6) | (hold[3]));
        dst += 3;
        src += 4;
        limit -= 3;
        len -= 4;
        mark = len;
    }
done:
    if (dst_len) {
        *dst_len = (size_t)(dst - dst_base);
    }
    if (src_len) {
        *src_len -= mark;
    }
    return ret;

tail:
    switch (k) {
    case 0:
        break;
    case 2:
        if ((hold[1] << 4) & 0xff) {
            goto dirty;
        }
        if (limit < 1) {
            goto more;
        }
        dst[0] = (uint8_t)((hold[0] << 2) | (hold[1] >> 4));
        dst += 1;
        break;
    case 3:
        if ((hold[2] << 6) & 0xff) {
            goto dirty;
        }
        if (limit < 2) {
            goto more;
        }
        dst[0] = (uint8_t)((hold[0] << 2) | (hold[1] >> 4));
        dst[1] = (uint8_t)((hold[1] << 4) | (hold[2] >> 2));
        dst += 2;
        break;
    default:
        ret = BASE64_ETAIL;
        goto done;
    }
    mark = len;
    goto done;
dirty:
    ret = BASE64_EDIRTY;
    goto done;
more:
    ret = BASE64_EMORE;
    goto done;
}

#ifdef __cplusplus
}
#endif

#endif /* PBASE64_H */
