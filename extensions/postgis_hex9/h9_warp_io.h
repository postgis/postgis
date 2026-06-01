/* h9_warp_io.h — runtime loader for the .h9warp sidecar format.
 *
 * Port of hex9_cli/h9warp_io.py. Reads the binary header, verifies the
 * CRC-32, decodes the per-vertex (dx, dy) payload, and — when the
 * mirror-symmetric half-format is used — unfolds it back to the full
 * tri_mesh vertex set via lex-quantized key match.
 *
 * Sidecar format spec: see export_warp_deltas.py module docstring.
 *
 * Two input paths:
 *   load_h9warp(path, ...)            — read from disk via std::ifstream
 *   load_h9warp(buf, len, ...)        — read from in-memory buffer
 *                                       (e.g. an `incbin`-d blob)
 *
 * Header-only; depends on h9_lattice_gen.h (for tri_mesh).
 */
#ifndef H9_WARP_IO_H
#define H9_WARP_IO_H

#include "h9_lattice_gen.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace h9 {

enum H9WarpDType : std::uint8_t {
    H9W_F64 = 0,
    H9W_F32 = 1,
    H9W_I16 = 2,
    H9W_I32 = 3,
};

static constexpr std::uint8_t H9W_FLAG_MIRRORED = 1u << 0;

struct H9WarpHeader {
    std::uint16_t version;       /* 1 or 2 */
    std::uint8_t  level;         /* 5 for L5 lattice */
    std::uint8_t  mode;          /* 0 = canonical DOWN */
    std::uint32_t count;         /* stored vertex count (half if mirrored) */
    std::uint8_t  dtype_code;
    std::uint8_t  flags;
    double        scale_x;
    double        scale_y;
    double        ellipsoid_a;
    double        ellipsoid_f;
    std::uint32_t crc_stored;
};

struct H9WarpData {
    H9WarpHeader                       header;
    /* deltas[i] = (dx, dy) for tri_mesh vertex i. Always full-mesh
     * (mirror unfolded if the file used the half format). */
    std::vector<std::array<double, 2>> deltas;
};

/* ── CRC-32 (zlib polynomial 0xedb88320) ──────────────────────────────── */
namespace detail {
inline std::uint32_t crc32_zlib(const std::uint8_t* data, std::size_t len) {
    static std::uint32_t table[256] = {};
    static bool init = false;
    if (!init) {
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t c = i;
            for (int k = 0; k < 8; ++k)
                c = (c >> 1) ^ ((c & 1u) ? 0xedb88320u : 0u);
            table[i] = c;
        }
        init = true;
    }
    std::uint32_t c = 0xffffffffu;
    for (std::size_t i = 0; i < len; ++i)
        c = (c >> 8) ^ table[(c ^ data[i]) & 0xffu];
    return c ^ 0xffffffffu;
}

/* Read a little-endian field from `buf` at byte offset `off`. */
template <typename T>
inline T read_le(const std::uint8_t* buf, std::size_t off) {
    T v;
    std::memcpy(&v, buf + off, sizeof(T));
    return v;
}

inline std::int64_t quantize(double x, int k) {
    return static_cast<std::int64_t>(std::nearbyint(std::ldexp(x, k)));
}
} /* namespace detail */

static constexpr std::size_t H9W_HEADER_LEN = 56;

/* Parse a 56-byte header from `buf`. Returns false on bad magic / version. */
inline bool parse_h9warp_header(const std::uint8_t* buf, std::size_t len,
                                H9WarpHeader& h, std::string* err = nullptr)
{
    if (len < H9W_HEADER_LEN) {
        if (err) *err = "h9warp: file shorter than header";
        return false;
    }
    if (buf[0] != 'H' || buf[1] != '9' || buf[2] != 'W' || buf[3] != 'P') {
        if (err) *err = "h9warp: bad magic";
        return false;
    }
    h.version     = detail::read_le<std::uint16_t>(buf, 4);
    h.level       = buf[6];
    h.mode        = buf[7];
    h.count       = detail::read_le<std::uint32_t>(buf, 8);
    h.dtype_code  = buf[12];
    h.flags       = (h.version >= 2) ? buf[13] : 0;
    h.scale_x     = detail::read_le<double>(buf, 16);
    h.scale_y     = detail::read_le<double>(buf, 24);
    h.ellipsoid_a = detail::read_le<double>(buf, 32);
    h.ellipsoid_f = detail::read_le<double>(buf, 40);
    h.crc_stored  = detail::read_le<std::uint32_t>(buf, 48);
    return true;
}

/* Decode the payload into raw (dx, dy) rows — `header.count` entries, in
 * the order they appear in the file (i.e. half-mesh order if mirrored). */
inline bool decode_h9warp_payload(const H9WarpHeader& h,
                                  const std::uint8_t* payload,
                                  std::size_t payload_len,
                                  std::vector<std::array<double, 2>>& rows,
                                  std::string* err = nullptr)
{
    std::size_t per_row = 0;
    switch (h.dtype_code) {
        case H9W_F64: per_row = 16; break;
        case H9W_F32: per_row = 8;  break;
        case H9W_I16: per_row = 4;  break;
        case H9W_I32: per_row = 8;  break;
        default:
            if (err) *err = "h9warp: unknown dtype_code";
            return false;
    }
    if (payload_len != static_cast<std::size_t>(h.count) * per_row) {
        if (err) *err = "h9warp: payload size mismatch";
        return false;
    }
    rows.resize(h.count);
    for (std::uint32_t i = 0; i < h.count; ++i) {
        const std::uint8_t* p = payload + i * per_row;
        switch (h.dtype_code) {
            case H9W_F64:
                rows[i][0] = detail::read_le<double>(p, 0);
                rows[i][1] = detail::read_le<double>(p, 8);
                break;
            case H9W_F32:
                rows[i][0] = detail::read_le<float>(p, 0);
                rows[i][1] = detail::read_le<float>(p, 4);
                break;
            case H9W_I16:
                rows[i][0] = detail::read_le<std::int16_t>(p, 0) * h.scale_x;
                rows[i][1] = detail::read_le<std::int16_t>(p, 2) * h.scale_y;
                break;
            case H9W_I32:
                rows[i][0] = detail::read_le<std::int32_t>(p, 0) * h.scale_x;
                rows[i][1] = detail::read_le<std::int32_t>(p, 4) * h.scale_y;
                break;
        }
    }
    return true;
}

/* Main loader — in-memory buffer variant. */
inline bool load_h9warp(const void* data, std::size_t len,
                        H9WarpData& out, std::string* err = nullptr)
{
    const auto* buf = static_cast<const std::uint8_t*>(data);
    if (!parse_h9warp_header(buf, len, out.header, err)) return false;

    const std::uint8_t* payload     = buf + H9W_HEADER_LEN;
    const std::size_t   payload_len = len - H9W_HEADER_LEN;

    const std::uint32_t crc_actual = detail::crc32_zlib(payload, payload_len);
    if (crc_actual != out.header.crc_stored) {
        if (err) *err = "h9warp: CRC mismatch";
        return false;
    }

    std::vector<std::array<double, 2>> stored;
    if (!decode_h9warp_payload(out.header, payload, payload_len, stored, err))
        return false;

    /* Rebuild the tri_mesh vertex set so we can place stored rows into
     * full-mesh order (and unfold the mirror when applicable). */
    const LatticeMesh mesh = tri_mesh(out.header.level, out.header.mode);
    const std::size_t n_full = mesh.verts.size();

    if (!(out.header.flags & H9W_FLAG_MIRRORED)) {
        /* Full format: payload is already in tri_mesh order. */
        if (stored.size() != n_full) {
            if (err) *err = "h9warp: full count != tri_mesh vertex count";
            return false;
        }
        out.deltas = std::move(stored);
        return true;
    }

    /* Mirrored format: payload is the x>=0 subset in tri_mesh order. Use a
     * lex-quantized key (matches h9warp_io.py and _unique_rows_tol) to
     * locate each tri_mesh vertex's stored row (or its negative-x twin). */
    constexpr double tol = 1e-12;
    const int k = static_cast<int>(std::ceil(-std::log2(tol)));

    std::vector<std::int64_t> qx(n_full), qy(n_full);
    for (std::size_t i = 0; i < n_full; ++i) {
        qx[i] = detail::quantize(mesh.verts[i].x, k);
        qy[i] = detail::quantize(mesh.verts[i].y, k);
    }

    /* Map (qx, qy) -> subset row index, but only for vertices with qx >= 0. */
    std::unordered_map<std::uint64_t, std::uint32_t> key_to_subset;
    key_to_subset.reserve(stored.size() * 2);
    auto pack = [](std::int64_t a, std::int64_t b) {
        /* xor-shift mix is overkill; a 32:32 pack is unique enough at L5
         * where |q| <= ~2^39. Stuff sign into high bit of upper half. */
        const auto au = static_cast<std::uint64_t>(a);
        const auto bu = static_cast<std::uint64_t>(b);
        return (au * 0x9E3779B97F4A7C15ull) ^ (bu + 0x9E3779B97F4A7C15ull
                                               + (au << 6) + (au >> 2));
    };

    std::uint32_t subset_i = 0;
    for (std::size_t i = 0; i < n_full; ++i) {
        if (qx[i] >= 0) {
            key_to_subset[pack(qx[i], qy[i])] = subset_i++;
        }
    }
    if (subset_i != out.header.count) {
        if (err) *err = "h9warp: mirrored count != x>=0 vertex count";
        return false;
    }

    out.deltas.assign(n_full, {0.0, 0.0});
    for (std::size_t i = 0; i < n_full; ++i) {
        if (qx[i] >= 0) {
            const std::uint32_t s = key_to_subset.at(pack(qx[i], qy[i]));
            out.deltas[i] = stored[s];
        } else {
            const auto it = key_to_subset.find(pack(-qx[i], qy[i]));
            if (it == key_to_subset.end()) {
                if (err) *err = "h9warp: no mirror counterpart for vertex";
                return false;
            }
            out.deltas[i][0] = -stored[it->second][0];
            out.deltas[i][1] = +stored[it->second][1];
        }
    }
    return true;
}

/* File-path variant — reads whole file into memory and dispatches above. */
inline bool load_h9warp(const std::string& path,
                        H9WarpData& out, std::string* err = nullptr)
{
    std::ifstream fh(path, std::ios::binary);
    if (!fh) {
        if (err) *err = "h9warp: cannot open " + path;
        return false;
    }
    fh.seekg(0, std::ios::end);
    const std::streamoff sz = fh.tellg();
    fh.seekg(0, std::ios::beg);
    if (sz < static_cast<std::streamoff>(H9W_HEADER_LEN)) {
        if (err) *err = "h9warp: file too small: " + path;
        return false;
    }
    std::vector<std::uint8_t> blob(static_cast<std::size_t>(sz));
    fh.read(reinterpret_cast<char*>(blob.data()), sz);
    return load_h9warp(blob.data(), blob.size(), out, err);
}

} /* namespace h9 */

#endif /* H9_WARP_IO_H */
