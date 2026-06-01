/* h9_warp_mesh.h — build the padded mesh that CT operates on.
 *
 * Port of the ghost-row padding step in AuthalicWarp.__init__
 * (hhg9/domains/octahedral_barycentric.py:180-206) together with the
 * Delaunay topology that scipy would have built around `padded_src`.
 *
 * Findings recorded 2026-05-27 (see memory `warp-distribution-plan`,
 * `warp-padding-strict-mask`):
 *
 *   Under the strict band mask (excluding y == Y_EQ to avoid redundant
 *   duplicates), the topology of `padded_src` decomposes into:
 *     - interior      : tri_mesh(L=5, mode=0) tris, used verbatim
 *     - ghost strip   : for each tri_mesh tri whose verts all lie in
 *                       (band ∪ equator), map band verts to their ghost
 *                       twin (Y-mirror across Y_EQ); keep equator verts
 *                       as-is (shared seam between halves)
 *   This reproduces scipy.spatial.Delaunay(padded_src) bit-for-bit:
 *   572,882 / 572,882 tris match, zero cocircular ambiguity.
 *
 * The deltas are mirrored alongside the vertices: (dx, dy) at a band
 * point becomes (dx, -dy) at its ghost twin. dy at exact equator is 0
 * by trainer symmetry — but those points are excluded by the strict
 * mask anyway.
 *
 * Header-only; depends on h9_lattice_gen.h.
 */
#ifndef H9_WARP_MESH_H
#define H9_WARP_MESH_H

#include "h9_lattice_gen.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace h9 {

/* Per-triangle neighbour indices in the same convention as scipy.Delaunay:
 *   neighbors[i][k] = the triangle sharing the edge OPPOSITE vertex k
 *                    (i.e. the edge between simplex[(k+1)%3] and
 *                    simplex[(k+2)%3]). -1 = boundary. */
using TriNeighbors = std::array<std::int32_t, 3>;

struct WarpMesh {
    /* Padded vertex set: [interior, ghost]. interior is tri_mesh(L, mode)
     * verts in tri_mesh order; ghost is the Y-mirror of band points, in
     * the order they appear in interior (i.e. np.where(band_mask) order). */
    std::vector<std::array<double, 2>> padded_src;

    /* Per-padded-vertex displacement. interior: caller-supplied deltas
     * (one per tri_mesh vertex). ghost: (dx, -dy) of the band-vertex twin. */
    std::vector<std::array<double, 2>> padded_diff;

    /* Full mesh topology: [interior tri_mesh tris, ghost-strip tris]. */
    std::vector<LatticeTri>   tris;
    std::vector<TriNeighbors> neighbors;

    /* Partition sizes — useful for diagnostics and for stitching back to
     * caller-side per-vertex data that's indexed by tri_mesh vertex. */
    std::size_t n_interior_verts = 0;   /* = tri_mesh(L, mode) vertex count */
    std::size_t n_interior_tris  = 0;   /* = tri_mesh(L, mode) tri count    */

    /* Indices of band verts (in `padded_src`/interior space). Useful for
     * caller-side debugging / cross-checking against Python. */
    std::vector<std::uint32_t> band_idx;
};

/* Constants matching octahedral_barycentric.py. Define as inline functions
 * to avoid TU-level `static` warnings and stay header-only. */
inline double y_equator() { return std::sqrt(6.0) / 6.0; }
inline double eq_band_width() { return 0.05; }     /* strict: 0 < Y_EQ-y < this */

namespace detail {

/* Build neighbours from a tri list. Edge hash: sorted (lo, hi) packed into
 * one uint64. Each interior edge has exactly two incident tris; each
 * boundary edge has one. Cost: O(N) with one pass + map lookup. */
inline void build_neighbors(const std::vector<LatticeTri>& tris,
                            std::vector<TriNeighbors>&     out)
{
    struct EdgeOwner { std::uint32_t tri; std::uint8_t k; };
    std::unordered_map<std::uint64_t, EdgeOwner> first;
    out.assign(tris.size(), {-1, -1, -1});
    auto edge_key = [](std::uint32_t a, std::uint32_t b) {
        if (a > b) std::swap(a, b);
        return (static_cast<std::uint64_t>(a) << 32) | b;
    };
    for (std::uint32_t ti = 0; ti < tris.size(); ++ti) {
        const auto& t = tris[ti];
        const std::uint32_t v[3] = {t.a, t.b, t.c};
        /* Edge k is opposite vertex k → connects v[(k+1)%3] and v[(k+2)%3]. */
        for (std::uint8_t k = 0; k < 3; ++k) {
            const std::uint32_t va = v[(k + 1) % 3];
            const std::uint32_t vb = v[(k + 2) % 3];
            const std::uint64_t key = edge_key(va, vb);
            auto it = first.find(key);
            if (it == first.end()) {
                first.emplace(key, EdgeOwner{ti, k});
            } else {
                const auto& other = it->second;
                out[ti][k]              = static_cast<std::int32_t>(other.tri);
                out[other.tri][other.k] = static_cast<std::int32_t>(ti);
                first.erase(it);
            }
        }
    }
}

} /* namespace detail */

/* Build the padded mesh from per-tri_mesh-vertex deltas.
 *
 * `deltas` must have length == tri_mesh(level, mode).verts.size()
 * (266,815 for L5, mode=0). They are interpreted in tri_mesh vertex order,
 * matching what h9::load_h9warp() returns. */
inline WarpMesh build_warp_mesh(const std::vector<std::array<double, 2>>& deltas,
                                int level = 5,
                                int mode  = 0)
{
    const LatticeMesh interior = tri_mesh(level, mode);
    const std::size_t n_interior = interior.verts.size();

    WarpMesh m;
    m.n_interior_verts = n_interior;
    m.n_interior_tris  = interior.tris.size();

    /* Strict band mask: 0 < (Y_EQ - y) < eq_band_width. */
    const double Y_EQ = y_equator();
    const double bw   = eq_band_width();
    m.band_idx.reserve(n_interior / 12);   /* L5 yields ~21k; rough upper. */
    for (std::uint32_t i = 0; i < n_interior; ++i) {
        const double dy = Y_EQ - interior.verts[i].y;
        if (dy > 0.0 && dy < bw) m.band_idx.push_back(i);
    }
    const std::size_t n_ghost = m.band_idx.size();

    /* Padded vertex array. */
    m.padded_src.resize(n_interior + n_ghost);
    for (std::size_t i = 0; i < n_interior; ++i) {
        m.padded_src[i] = { interior.verts[i].x, interior.verts[i].y };
    }
    for (std::size_t k = 0; k < n_ghost; ++k) {
        const auto& v = interior.verts[m.band_idx[k]];
        m.padded_src[n_interior + k] = { v.x, 2.0 * Y_EQ - v.y };
    }

    /* Padded displacement array.
     * Interior: caller-supplied deltas (mirror to deltas argument order).
     * Ghost: (dx, -dy) of the band-vertex twin. */
    m.padded_diff.resize(n_interior + n_ghost);
    for (std::size_t i = 0; i < n_interior; ++i) {
        m.padded_diff[i] = deltas[i];
    }
    for (std::size_t k = 0; k < n_ghost; ++k) {
        const auto& d = deltas[m.band_idx[k]];
        m.padded_diff[n_interior + k] = { d[0], -d[1] };
    }

    /* Index map: interior vertex i (must be in band) -> ghost padded index. */
    std::vector<std::int32_t> src_to_ghost(n_interior, -1);
    for (std::size_t k = 0; k < n_ghost; ++k) {
        src_to_ghost[m.band_idx[k]] = static_cast<std::int32_t>(n_interior + k);
    }

    /* Boolean masks over interior verts: in-band, on-equator. */
    std::vector<std::uint8_t> in_band(n_interior, 0);
    for (auto i : m.band_idx) in_band[i] = 1;
    std::vector<std::uint8_t> on_eq(n_interior, 0);
    for (std::uint32_t i = 0; i < n_interior; ++i) {
        const double y = interior.verts[i].y;
        on_eq[i] = (std::fabs(y - Y_EQ) < 1e-14) ? 1u : 0u;
    }

    /* Build full tri list: interior verbatim, then analytic strip. */
    m.tris.reserve(interior.tris.size() + n_ghost * 2);    /* upper-ish */
    for (const auto& t : interior.tris) m.tris.push_back(t);

    /* For each interior tri that has at least one band vert, try to mirror
     * it to the ghost strip. Mirror succeeds iff every vert is either
     *   - in-band  → maps to its ghost twin
     *   - on-eq    → kept as-is (shared seam)
     * If any vert is neither in-band nor on-eq, the tri reaches further
     * south than the band and is NOT mirrored. */
    for (const auto& t : interior.tris) {
        const std::uint32_t verts[3] = {t.a, t.b, t.c};
        if (!(in_band[verts[0]] | in_band[verts[1]] | in_band[verts[2]]))
            continue;                              /* no band vert → not a strip tri */

        std::uint32_t out_v[3];
        bool ok = true;
        for (int k = 0; k < 3; ++k) {
            const std::uint32_t v = verts[k];
            if (in_band[v]) {
                out_v[k] = static_cast<std::uint32_t>(src_to_ghost[v]);
            } else if (on_eq[v]) {
                out_v[k] = v;                      /* shared equator vertex */
            } else {
                ok = false;
                break;
            }
        }
        if (ok) m.tris.push_back({out_v[0], out_v[1], out_v[2]});
    }

    detail::build_neighbors(m.tris, m.neighbors);
    return m;
}

} /* namespace h9 */

#endif /* H9_WARP_MESH_H */
