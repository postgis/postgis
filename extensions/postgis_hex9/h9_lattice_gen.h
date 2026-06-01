/* h9_lattice_gen.h — analytic L5 (or any-level) lattice generator.
 *
 * C++ port of hhg9/h9/polygon.py:
 *   region_grid(levels, mode)   -> recursive subdivision
 *   tri_grid   (levels, mode)   -> per-triangle vertex coordinates
 *   tri_mesh   (levels, mode)   -> deduplicated mesh (verts, tris)
 *
 * Produces the same (verts, tris) arrays as the Python reference,
 * vertex order matching Python's `_unique_rows_tol` (lex-sorted on
 * power-of-two-quantized keys).  Used to rebuild source_pts and the
 * Delaunay-equivalent triangle list at C++ extension load time, from
 * the H9_KIDS / H9_SV constants alone.
 *
 * Memory footprint for L=5: ~13 MB transient (region queue + flat tri
 * vertex buffer), ~12 MB retained (mesh). Build cost: ~150 ms on a
 * modern x86 core.
 *
 * Header-only; depends on h9_lattice_consts.h.
 */
#ifndef H9_LATTICE_GEN_H
#define H9_LATTICE_GEN_H

#include "h9_lattice_consts.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <vector>

namespace h9 {

struct LatticeRegion {
    std::uint8_t mode;
    double origin_x;
    double origin_y;
    double scale;
};

struct LatticeVert {
    double x;
    double y;
};

struct LatticeTri {
    std::uint32_t a;
    std::uint32_t b;
    std::uint32_t c;
};

struct LatticeMesh {
    std::vector<LatticeVert> verts;
    std::vector<LatticeTri>  tris;
};

/* region_grid(levels, mode):  iterative 9-way subdivision.
 *
 * Mirrors `polygon.py:region_grid` exactly — the initial queue is the
 * 9 children of a single root cell of `parent_mode`, each at scale 1/3.
 * Each subsequent depth replaces every entry with its 9 children at
 * scale/3.  After `levels` depths the queue length is 9^(levels+1).
 */
inline void region_grid(int levels, int parent_mode,
                        std::vector<LatticeRegion>& out)
{
    out.clear();
    out.reserve(9);
    const auto& kids0 = H9_KIDS[parent_mode];
    for (int i = 0; i < 9; ++i) {
        out.push_back({kids0[i].mode,
                       kids0[i].off_x,
                       kids0[i].off_y,
                       1.0 / 3.0});
    }
    std::vector<LatticeRegion> next;
    for (int depth = 0; depth < levels; ++depth) {
        next.clear();
        next.reserve(out.size() * 9);
        for (const auto& p : out) {
            const auto& kids = H9_KIDS[p.mode];
            const double new_scale = p.scale / 3.0;
            for (int i = 0; i < 9; ++i) {
                next.push_back({
                    kids[i].mode,
                    p.origin_x + kids[i].off_x * p.scale,
                    p.origin_y + kids[i].off_y * p.scale,
                    new_scale
                });
            }
        }
        out.swap(next);
    }
}

/* tri_grid(levels, mode): flat (T, 3, 2) vertex coordinates.
 * `out_flat` is sized to T*6 = T*(3 vertices)*(2 axes), row-major.
 */
inline void tri_grid(int levels, int parent_mode,
                     std::vector<double>& out_flat)
{
    std::vector<LatticeRegion> regions;
    region_grid(levels, parent_mode, regions);
    out_flat.resize(regions.size() * 6);
    const auto& sv = H9_SV;
    for (std::size_t i = 0; i < regions.size(); ++i) {
        const auto& r = regions[i];
        const double* svm = &sv[r.mode][0][0];
        double* p = &out_flat[6 * i];
        for (int v = 0; v < 3; ++v) {
            p[2*v + 0] = r.origin_x + svm[2*v + 0] * r.scale;
            p[2*v + 1] = r.origin_y + svm[2*v + 1] * r.scale;
        }
    }
}

/* tri_mesh(levels, mode): deduplicated mesh.
 *
 * Quantization matches `polygon.py:_unique_rows_tol`:
 *   tol = (levels >= 6) ? 1e-10 : 1e-12
 *   k   = ceil(-log2(tol))
 *   key = round(ldexp(x, k))
 *
 * Vertices are emitted in lex-sorted (kx, ky) order, with the first
 * occurrence's *original* floats as the representative — same as
 * np.unique(axis=0)'s first-seen-in-sorted-order behaviour.
 */
inline LatticeMesh tri_mesh(int levels, int parent_mode)
{
    std::vector<double> flat;
    tri_grid(levels, parent_mode, flat);
    const std::size_t num_pts = flat.size() / 2;
    const std::size_t num_tris = flat.size() / 6;

    const double tol = (levels >= 6) ? 1e-10 : 1e-12;
    const int k = static_cast<int>(std::ceil(-std::log2(tol)));

    struct Key {
        std::int64_t kx;
        std::int64_t ky;
        std::uint32_t orig;
    };
    std::vector<Key> keys(num_pts);
    for (std::size_t i = 0; i < num_pts; ++i) {
        const double x = flat[2*i + 0];
        const double y = flat[2*i + 1];
        keys[i].kx   = static_cast<std::int64_t>(std::nearbyint(std::ldexp(x, k)));
        keys[i].ky   = static_cast<std::int64_t>(std::nearbyint(std::ldexp(y, k)));
        keys[i].orig = static_cast<std::uint32_t>(i);
    }

    /* Lex-sort by (kx, ky) — must be STABLE so that for duplicate keys the
     * representative we keep is the one with the smallest original index,
     * matching np.unique(axis=0)'s "first occurrence" semantics. Without
     * stable sort the float representative can differ by 1 ULP from Python
     * (same quantized key, slightly different origin+offset*scale rounding). */
    std::vector<std::uint32_t> order(num_pts);
    std::iota(order.begin(), order.end(), 0u);
    std::stable_sort(order.begin(), order.end(),
        [&](std::uint32_t a, std::uint32_t b) {
            const Key& ka = keys[a];
            const Key& kb = keys[b];
            return ka.kx != kb.kx ? ka.kx < kb.kx : ka.ky < kb.ky;
        });

    LatticeMesh mesh;
    mesh.verts.reserve(num_pts);
    std::vector<std::uint32_t> new_idx(num_pts);
    std::int64_t prev_kx = 0;
    std::int64_t prev_ky = 0;
    bool first = true;
    std::uint32_t cur = 0;
    for (std::uint32_t pos : order) {
        const Key& e = keys[pos];
        if (first || e.kx != prev_kx || e.ky != prev_ky) {
            mesh.verts.push_back({flat[2*e.orig + 0], flat[2*e.orig + 1]});
            cur = static_cast<std::uint32_t>(mesh.verts.size()) - 1;
            prev_kx = e.kx;
            prev_ky = e.ky;
            first = false;
        }
        new_idx[e.orig] = cur;
    }

    mesh.tris.reserve(num_tris);
    for (std::size_t t = 0; t < num_tris; ++t) {
        mesh.tris.push_back({
            new_idx[3*t + 0],
            new_idx[3*t + 1],
            new_idx[3*t + 2]
        });
    }
    return mesh;
}

} // namespace h9

#endif /* H9_LATTICE_GEN_H */
