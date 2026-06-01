/* h9_ct.h — Clough-Tocher state built at runtime from a WarpMesh.
 *
 * Port of the per-vertex gradient estimator and (later) the Alfeld-CT
 * Bernstein coefficient construction from
 *   - scipy/interpolate/_interpnd.pyx :: _estimate_gradients_2d_global
 *     (Nielson '83, Renka & Cline '84 — global gradient minimisation)
 *   - hex9_cli/export_ct_mesh.py :: ct_coeffs
 *
 * The gradient algorithm in one line:
 *
 *   minimise  Z = sum_edges ∫_E (W'')² ds
 *
 * iteratively, Gauss-Seidel over vertices. Per-edge contribution to the
 * 2x2 local system at vertex V_1 (with V_2 its neighbour, e = V_2 - V_1,
 * L = |e|, L3 = L³) is:
 *
 *   Q += 4 (e eᵀ) / L³                           (Hessian)
 *   s += (6 (f1 - f2) - 2 df2) e / L³            (residual)
 *   df2 = -e · grad[V_2]                         (other vertex grad proj)
 *
 * Solving Q·g_V = -s gives the new gradient at V_1.
 *
 * Header-only; depends on h9_warp_mesh.h.
 */
#ifndef H9_CT_H
#define H9_CT_H

#include "h9_warp_mesh.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

/* Optional phase timing for build_ct_state, compiled in only under
 * -DH9_CT_BENCH (see warp_bench.cpp). Zero cost in normal builds. */
#ifdef H9_CT_BENCH
#include <chrono>
#include <cstdio>
#define H9_CT_BENCH_T(var) auto var = std::chrono::steady_clock::now()
#define H9_CT_BENCH_MS(label, a, b) \
    std::fprintf(stderr, "  [ct] %-18s %8.1f ms\n", label, \
        std::chrono::duration<double, std::milli>((b) - (a)).count())
#else
#define H9_CT_BENCH_T(var) ((void)0)
#define H9_CT_BENCH_MS(label, a, b) ((void)0)
#endif

namespace h9 {

/* CSR-style vertex adjacency. neighbors[indptr[v] .. indptr[v+1]) lists
 * the neighbours of vertex v (deduplicated, unsorted — order is
 * encounter-order during the tri-scan). */
struct VertexNeighbors {
    std::vector<std::uint32_t> indptr;     /* size = npoints + 1 */
    std::vector<std::uint32_t> indices;
};

inline VertexNeighbors build_vertex_neighbors(std::size_t npoints,
                                              const std::vector<LatticeTri>& tris)
{
    /* Two passes: first count per-vertex degree, then fill. */
    std::vector<std::uint32_t> deg(npoints + 1, 0);

    /* To dedupe, we use a "seen" marker per vertex during the per-vertex
     * fill pass. Allocate `seen` sized to npoints, initialised to the
     * sentinel value max_u32; for each vertex `v`, we mark `seen[other]=v`
     * when we add `other` to v's neighbour list. */
    std::vector<std::uint32_t> seen(npoints, std::numeric_limits<std::uint32_t>::max());

    /* Build per-vertex incident-tri lists (to walk the local 1-ring). */
    std::vector<std::uint32_t> tri_of_vert_indptr(npoints + 1, 0);
    for (const auto& t : tris) {
        tri_of_vert_indptr[t.a + 1]++;
        tri_of_vert_indptr[t.b + 1]++;
        tri_of_vert_indptr[t.c + 1]++;
    }
    for (std::size_t i = 0; i < npoints; ++i)
        tri_of_vert_indptr[i + 1] += tri_of_vert_indptr[i];

    std::vector<std::uint32_t> tri_of_vert(tri_of_vert_indptr[npoints]);
    std::vector<std::uint32_t> cursor = tri_of_vert_indptr;
    for (std::uint32_t ti = 0; ti < tris.size(); ++ti) {
        const auto& t = tris[ti];
        tri_of_vert[cursor[t.a]++] = ti;
        tri_of_vert[cursor[t.b]++] = ti;
        tri_of_vert[cursor[t.c]++] = ti;
    }

    /* Pass 1: count degree per vertex (dedupe within the 1-ring). */
    for (std::uint32_t v = 0; v < npoints; ++v) {
        std::uint32_t d = 0;
        for (std::uint32_t j = tri_of_vert_indptr[v]; j < tri_of_vert_indptr[v + 1]; ++j) {
            const auto& t = tris[tri_of_vert[j]];
            const std::uint32_t verts[3] = {t.a, t.b, t.c};
            for (int k = 0; k < 3; ++k) {
                const std::uint32_t u = verts[k];
                if (u == v || seen[u] == v) continue;
                seen[u] = v;
                ++d;
            }
        }
        deg[v + 1] = d;
        /* Reset markers used for this v so we can reuse for next v. */
        for (std::uint32_t j = tri_of_vert_indptr[v]; j < tri_of_vert_indptr[v + 1]; ++j) {
            const auto& t = tris[tri_of_vert[j]];
            seen[t.a] = std::numeric_limits<std::uint32_t>::max();
            seen[t.b] = std::numeric_limits<std::uint32_t>::max();
            seen[t.c] = std::numeric_limits<std::uint32_t>::max();
        }
    }

    VertexNeighbors out;
    out.indptr.resize(npoints + 1);
    out.indptr[0] = 0;
    for (std::size_t i = 0; i < npoints; ++i)
        out.indptr[i + 1] = out.indptr[i] + deg[i + 1];
    out.indices.resize(out.indptr[npoints]);

    /* Pass 2: fill. Encounter order: tris in tri-list order, verts in
     * (a, b, c) order within each tri. */
    for (std::uint32_t v = 0; v < npoints; ++v) {
        std::uint32_t pos = out.indptr[v];
        for (std::uint32_t j = tri_of_vert_indptr[v]; j < tri_of_vert_indptr[v + 1]; ++j) {
            const auto& t = tris[tri_of_vert[j]];
            const std::uint32_t verts[3] = {t.a, t.b, t.c};
            for (int k = 0; k < 3; ++k) {
                const std::uint32_t u = verts[k];
                if (u == v || seen[u] == v) continue;
                seen[u] = v;
                out.indices[pos++] = u;
            }
        }
        /* Reset markers. */
        for (std::uint32_t j = tri_of_vert_indptr[v]; j < tri_of_vert_indptr[v + 1]; ++j) {
            const auto& t = tris[tri_of_vert[j]];
            seen[t.a] = std::numeric_limits<std::uint32_t>::max();
            seen[t.b] = std::numeric_limits<std::uint32_t>::max();
            seen[t.c] = std::numeric_limits<std::uint32_t>::max();
        }
    }
    return out;
}

/* Estimate per-vertex gradients via Bell-Sibson minimisation
 * (Nielson '83 / Renka-Cline '84). Solves
 *
 *     min  Σ_edges  ∫_E (W'')² ds
 *
 * by Gauss-Seidel over vertices. `data` is the scalar field at each
 * padded vertex; output `grad` is (npoints * 2) row-major (g_x, g_y).
 *
 * Returns the iteration count if converged, 0 if maxiter was reached
 * without convergence (matches scipy's return convention).
 *
 * To match scipy bit-exactly we'd need scipy's qhull-CSR neighbour
 * ordering. Encounter-order convergence is *mathematically* the same
 * fixed point — at tol=1e-12, gradient agreement should be ~1 ULP. */
inline int estimate_gradients_2d_global(const std::vector<std::array<double, 2>>& padded_src,
                                        const VertexNeighbors& vn,
                                        const double* data,
                                        std::vector<std::array<double, 2>>& grad,
                                        int    maxiter = 400,
                                        double tol     = 1e-6)
{
    const std::size_t n = padded_src.size();
    grad.assign(n, {0.0, 0.0});

    for (int iiter = 0; iiter < maxiter; ++iiter) {
        double err = 0.0;
        for (std::uint32_t ip = 0; ip < n; ++ip) {
            double Q[4] = {0.0, 0.0, 0.0, 0.0};
            double s[2] = {0.0, 0.0};
            const double px = padded_src[ip][0];
            const double py = padded_src[ip][1];
            const double f1 = data[ip];

            const std::uint32_t lo = vn.indptr[ip];
            const std::uint32_t hi = vn.indptr[ip + 1];
            for (std::uint32_t j = lo; j < hi; ++j) {
                const std::uint32_t ip2 = vn.indices[j];
                const double ex = padded_src[ip2][0] - px;
                const double ey = padded_src[ip2][1] - py;
                const double L2 = ex * ex + ey * ey;
                const double L  = std::sqrt(L2);
                const double L3 = L * L2;
                const double f2 = data[ip2];
                const double df2 = -ex * grad[ip2][0] - ey * grad[ip2][1];

                const double coef = (6.0 * (f1 - f2) - 2.0 * df2) / L3;
                Q[0] += 4.0 * ex * ex / L3;
                Q[1] += 4.0 * ex * ey / L3;
                Q[3] += 4.0 * ey * ey / L3;
                s[0] += coef * ex;
                s[1] += coef * ey;
            }
            Q[2] = Q[1];   /* symmetric */
            const double det = Q[0] * Q[3] - Q[1] * Q[2];
            if (!(std::fabs(det) > 0.0)) continue;   /* isolated vertex: skip */
            const double rx = ( Q[3] * s[0] - Q[1] * s[1]) / det;
            const double ry = (-Q[2] * s[0] + Q[0] * s[1]) / det;

            const double change_raw = std::max(std::fabs(grad[ip][0] + rx),
                                               std::fabs(grad[ip][1] + ry));
            const double norm = std::max(1.0,
                                         std::max(std::fabs(rx), std::fabs(ry)));
            const double change = change_raw / norm;
            if (change > err) err = change;

            grad[ip][0] = -rx;
            grad[ip][1] = -ry;
        }
        if (err < tol) return iiter + 1;
    }
    return 0;
}

/* ── Alfeld-CT Bernstein coefficient construction ──────────────────────
 *
 * Mirrors hex9_cli/export_ct_mesh.py :: ct_coeffs. Per triangle, builds
 * 19 control points in this order (matching the existing h9_ct_eval()):
 *
 *   c3000, c2100, c2010, c2001, c1200, c1101, c1020, c1011, c1002,
 *   c0300, c0210, c0201, c0120, c0111, c0102,
 *   c0030, c0021, c0012, c0003
 */
using CTCoeffs = std::array<double, 19>;

/* Affine-invariant g-values for triangle ti from neighbour centroids.
 * g[k] = -0.5 at convex-hull boundary (no neighbour opposite vertex k). */
inline void ct_g_values(const WarpMesh& mesh, std::uint32_t ti, double g[3]) {
    const auto& tri = mesh.tris[ti];
    const std::uint32_t s[3] = {tri.a, tri.b, tri.c};
    const auto& P0 = mesh.padded_src[s[0]];
    const auto& P1 = mesh.padded_src[s[1]];
    const auto& P2 = mesh.padded_src[s[2]];
    const double V4x = (P0[0] + P1[0] + P2[0]) / 3.0;
    const double V4y = (P0[1] + P1[1] + P2[1]) / 3.0;

    const auto& nb = mesh.neighbors[ti];
    const std::array<const std::array<double, 2>*, 3> P = {&P0, &P1, &P2};
    for (int k = 0; k < 3; ++k) {
        g[k] = -0.5;
        const std::int32_t n = nb[k];
        if (n < 0) continue;
        const auto& nt = mesh.tris[n];
        const auto& Q0 = mesh.padded_src[nt.a];
        const auto& Q1 = mesh.padded_src[nt.b];
        const auto& Q2 = mesh.padded_src[nt.c];
        const double V4px = (Q0[0] + Q1[0] + Q2[0]) / 3.0;
        const double V4py = (Q0[1] + Q1[1] + Q2[1]) / 3.0;
        const double dx = V4px - V4x;
        const double dy = V4py - V4y;
        const auto& V2 = *P[(k + 1) % 3];
        const auto& V3 = *P[(k + 2) % 3];
        const double ax = V4x - V2[0], ay = V4y - V2[1];
        const double bx = V3[0] - V2[0], by = V3[1] - V2[1];
        const double denom = dx * by - dy * bx;
        if (std::fabs(denom) > 1e-30) {
            g[k] = (dy * ax - dx * ay) / denom;
        }
    }
}

/* 19 Alfeld-CT Bernstein coefficients for one triangle. `val` and `grad`
 * are indexed by *padded* vertex (so val[mesh.tris[ti].a] etc.). */
inline CTCoeffs ct_coeffs_for_tri(const WarpMesh& mesh,
                                   std::uint32_t   ti,
                                   const double*   val,
                                   const std::vector<std::array<double, 2>>& grad)
{
    const auto& tri = mesh.tris[ti];
    const std::uint32_t s[3] = {tri.a, tri.b, tri.c};
    const auto& P0 = mesh.padded_src[s[0]];
    const auto& P1 = mesh.padded_src[s[1]];
    const auto& P2 = mesh.padded_src[s[2]];
    const double v[3] = {val[s[0]], val[s[1]], val[s[2]]};
    const auto& g0 = grad[s[0]];
    const auto& g1 = grad[s[1]];
    const auto& g2 = grad[s[2]];

    /* Edge vectors from each vertex. */
    const double e01x = P1[0] - P0[0], e01y = P1[1] - P0[1];
    const double e02x = P2[0] - P0[0], e02y = P2[1] - P0[1];
    const double e10x = -e01x,         e10y = -e01y;
    const double e12x = P2[0] - P1[0], e12y = P2[1] - P1[1];
    const double e20x = -e02x,         e20y = -e02y;
    const double e21x = -e12x,         e21y = -e12y;

    /* Directional derivatives at each corner. */
    const double d01 = g0[0] * e01x + g0[1] * e01y;
    const double d02 = g0[0] * e02x + g0[1] * e02y;
    const double d10 = g1[0] * e10x + g1[1] * e10y;
    const double d12 = g1[0] * e12x + g1[1] * e12y;
    const double d20 = g2[0] * e20x + g2[1] * e20y;
    const double d21 = g2[0] * e21x + g2[1] * e21y;

    const double c3000 = v[0];
    const double c0300 = v[1];
    const double c0030 = v[2];

    const double c2100 = (d01 + 3.0 * c3000) / 3.0;
    const double c1200 = (d10 + 3.0 * c0300) / 3.0;
    const double c2010 = (d02 + 3.0 * c3000) / 3.0;
    const double c0210 = (d12 + 3.0 * c0300) / 3.0;
    const double c1020 = (d20 + 3.0 * c0030) / 3.0;
    const double c0120 = (d21 + 3.0 * c0030) / 3.0;

    const double c2001 = (c2100 + c2010 + c3000) / 3.0;
    const double c0201 = (c1200 + c0300 + c0210) / 3.0;
    const double c0021 = (c1020 + c0120 + c0030) / 3.0;

    double g[3];
    ct_g_values(mesh, ti, g);

    const double c0111 = (g[0] * (-c0300 + 3.0 * c0210 - 3.0 * c0120 + c0030)
                          + (-c0300 + 2.0 * c0210 - c0120 + c0021 + c0201)) / 2.0;
    const double c1011 = (g[1] * (-c0030 + 3.0 * c1020 - 3.0 * c2010 + c3000)
                          + (-c0030 + 2.0 * c1020 - c2010 + c2001 + c0021)) / 2.0;
    const double c1101 = (g[2] * (-c3000 + 3.0 * c2100 - 3.0 * c1200 + c0300)
                          + (-c3000 + 2.0 * c2100 - c1200 + c2001 + c0201)) / 2.0;

    const double c1002 = (c1101 + c1011 + c2001) / 3.0;
    const double c0102 = (c1101 + c0111 + c0201) / 3.0;
    const double c0012 = (c1011 + c0111 + c0021) / 3.0;

    const double c0003 = (c1002 + c0102 + c0012) / 3.0;

    return { c3000, c2100, c2010, c2001, c1200, c1101, c1020, c1011, c1002,
             c0300, c0210, c0201, c0120, c0111, c0102,
             c0030, c0021, c0012, c0003 };
}

/* ── Grid-acceleration index + find_tri walker ─────────────────────────
 *
 * Build a 729² grid spanning the mode-0 b_oct domain. Each cell stores
 * the index of a Delaunay triangle suitable as a starting point for the
 * point-in-tri walk. Outside-hull cells get the nearest-centroid tri. */
static constexpr int H9_CT_GRID_N = 729;

struct CTState {
    const WarpMesh*       mesh = nullptr;     /* borrowed */
    std::vector<CTCoeffs> coeffs_dx;          /* per padded tri */
    std::vector<CTCoeffs> coeffs_dy;
    std::vector<std::int32_t> grid_tri;       /* GRID_N * GRID_N */
    double xmin, xmax, ymin, ymax;
};

/* Locate the Delaunay triangle containing (x, y) via grid hint + walk.
 * Sets b[0..2] to barycentric coordinates. Returns the triangle index
 * (clamped on hull boundary). */
inline std::int32_t ct_find_tri(const CTState& st, double x, double y, double b[3])
{
    const double inv_dx = (H9_CT_GRID_N - 1) / (st.xmax - st.xmin);
    const double inv_dy = (H9_CT_GRID_N - 1) / (st.ymax - st.ymin);
    int ix = static_cast<int>((x - st.xmin) * inv_dx);
    int iy = static_cast<int>((y - st.ymin) * inv_dy);
    if (ix < 0) ix = 0; else if (ix >= H9_CT_GRID_N) ix = H9_CT_GRID_N - 1;
    if (iy < 0) iy = 0; else if (iy >= H9_CT_GRID_N) iy = H9_CT_GRID_N - 1;
    std::int32_t ti = st.grid_tri[iy * H9_CT_GRID_N + ix];
    if (ti < 0) { b[0] = b[1] = b[2] = 1.0 / 3.0; return ti; }

    for (int iter = 0; iter < 64; ++iter) {
        const auto& tri = st.mesh->tris[ti];
        const auto& nb  = st.mesh->neighbors[ti];
        const auto& P0  = st.mesh->padded_src[tri.a];
        const auto& P1  = st.mesh->padded_src[tri.b];
        const auto& P2  = st.mesh->padded_src[tri.c];
        const double T00 = P0[0] - P2[0], T01 = P1[0] - P2[0];
        const double T10 = P0[1] - P2[1], T11 = P1[1] - P2[1];
        const double det = T00 * T11 - T01 * T10;
        const double dx = x - P2[0], dy = y - P2[1];
        const double b0 = ( T11 * dx - T01 * dy) / det;
        const double b1 = (-T10 * dx + T00 * dy) / det;
        const double b2 = 1.0 - b0 - b1;
        if (b0 >= -1e-10 && b1 >= -1e-10 && b2 >= -1e-10) {
            b[0] = b0; b[1] = b1; b[2] = b2;
            return ti;
        }
        /* Walk toward the most-negative barycentric direction. */
        if (b0 <= b1 && b0 <= b2) { if (nb[0] >= 0) { ti = nb[0]; continue; } }
        else if (b1 <= b2)         { if (nb[1] >= 0) { ti = nb[1]; continue; } }
        else                       { if (nb[2] >= 0) { ti = nb[2]; continue; } }
        /* Hull boundary — clamp and return. */
        b[0] = b0 < 0.0 ? 0.0 : b0;
        b[1] = b1 < 0.0 ? 0.0 : b1;
        b[2] = b2 < 0.0 ? 0.0 : b2;
        const double s = b[0] + b[1] + b[2];
        if (s > 0.0) { b[0] /= s; b[1] /= s; b[2] /= s; }
        return ti;
    }
    b[0] = b[1] = b[2] = 1.0 / 3.0;
    return ti;
}

/* Evaluate the Alfeld-CT cubic given precomputed coefficients. */
inline double ct_eval_with(const CTCoeffs& c, const double b[3]) {
    const double b0 = b[0], b1 = b[1], b2 = b[2];
    const double mn  = std::min(b0, std::min(b1, b2));
    const double b1s = b0 - mn, b2s = b1 - mn, b3s = b2 - mn, b4s = 3.0 * mn;
    return (b1s*b1s*b1s*c[0]  + 3.0*b1s*b1s*b2s*c[1]  + 3.0*b1s*b1s*b3s*c[2]  +
            3.0*b1s*b1s*b4s*c[3]  + 3.0*b1s*b2s*b2s*c[4]  +
            6.0*b1s*b2s*b4s*c[5]  + 3.0*b1s*b3s*b3s*c[6]  + 6.0*b1s*b3s*b4s*c[7] +
            3.0*b1s*b4s*b4s*c[8]  + b2s*b2s*b2s*c[9]  + 3.0*b2s*b2s*b3s*c[10] +
            3.0*b2s*b2s*b4s*c[11] + 3.0*b2s*b3s*b3s*c[12] + 6.0*b2s*b3s*b4s*c[13] +
            3.0*b2s*b4s*b4s*c[14] + b3s*b3s*b3s*c[15] + 3.0*b3s*b3s*b4s*c[16] +
            3.0*b3s*b4s*b4s*c[17] + b4s*b4s*b4s*c[18]);
}

inline double ct_eval(const CTState& st, bool dy, double x, double y) {
    double b[3];
    const std::int32_t ti = ct_find_tri(st, x, y, b);
    if (ti < 0) return 0.0;
    return ct_eval_with(dy ? st.coeffs_dy[ti] : st.coeffs_dx[ti], b);
}

/* Build the full CT state (coefficients + grid index) from the warp
 * mesh. Inputs are the padded scalar fields and their per-vertex
 * gradients. */
inline CTState build_ct_state(const WarpMesh& mesh,
                              const double*   field_dx,
                              const double*   field_dy,
                              const std::vector<std::array<double, 2>>& grad_dx,
                              const std::vector<std::array<double, 2>>& grad_dy)
{
    CTState st;
    st.mesh = &mesh;
    st.coeffs_dx.resize(mesh.tris.size());
    st.coeffs_dy.resize(mesh.tris.size());
    H9_CT_BENCH_T(t_coeff0);
    for (std::uint32_t ti = 0; ti < mesh.tris.size(); ++ti) {
        st.coeffs_dx[ti] = ct_coeffs_for_tri(mesh, ti, field_dx, grad_dx);
        st.coeffs_dy[ti] = ct_coeffs_for_tri(mesh, ti, field_dy, grad_dy);
    }
    H9_CT_BENCH_T(t_coeff1);
    H9_CT_BENCH_MS("coeffs", t_coeff0, t_coeff1);

    /* Grid bounds match export_ct_mesh.py: */
    st.xmin = -std::sqrt(2.0) / 2.0;
    st.xmax =  std::sqrt(2.0) / 2.0;
    st.ymin = -std::sqrt(6.0) / 3.0;
    st.ymax =  std::sqrt(6.0) / 6.0;
    st.grid_tri.assign(static_cast<std::size_t>(H9_CT_GRID_N) * H9_CT_GRID_N, -1);
    H9_CT_BENCH_T(t_grid0);

    /* For each grid cell centre, find_simplex via brute-force walk seeded
     * from any tri containing a near-centroid hint. Cheap approach: scan
     * tris once and assign each cell to the *first* tri whose AABB
     * encloses the cell point. Then a refinement pass uses the walker
     * starting from that hint to land on the exact containing tri. */

    /* Build per-tri AABB. */
    struct AABB { double xlo, ylo, xhi, yhi; };
    std::vector<AABB> bbox(mesh.tris.size());
    for (std::uint32_t ti = 0; ti < mesh.tris.size(); ++ti) {
        const auto& tri = mesh.tris[ti];
        const auto& P0 = mesh.padded_src[tri.a];
        const auto& P1 = mesh.padded_src[tri.b];
        const auto& P2 = mesh.padded_src[tri.c];
        AABB& a = bbox[ti];
        a.xlo = std::min(P0[0], std::min(P1[0], P2[0]));
        a.xhi = std::max(P0[0], std::max(P1[0], P2[0]));
        a.ylo = std::min(P0[1], std::min(P1[1], P2[1]));
        a.yhi = std::max(P0[1], std::max(P1[1], P2[1]));
    }

    /* Bin tris into a coarse spatial grid. */
    constexpr int BIN_N = 256;
    const double bin_inv_dx = BIN_N / (st.xmax - st.xmin);
    const double bin_inv_dy = BIN_N / (st.ymax - st.ymin);
    std::vector<std::vector<std::uint32_t>> bins(BIN_N * BIN_N);
    auto clamp_int = [](int v, int lo, int hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    };
    auto bin_idx = [&](double x, double y) {
        int bx = clamp_int(static_cast<int>((x - st.xmin) * bin_inv_dx), 0, BIN_N - 1);
        int by = clamp_int(static_cast<int>((y - st.ymin) * bin_inv_dy), 0, BIN_N - 1);
        return by * BIN_N + bx;
    };
    for (std::uint32_t ti = 0; ti < mesh.tris.size(); ++ti) {
        const auto& a = bbox[ti];
        int bxlo = clamp_int(static_cast<int>((a.xlo - st.xmin) * bin_inv_dx), 0, BIN_N - 1);
        int bxhi = clamp_int(static_cast<int>((a.xhi - st.xmin) * bin_inv_dx), 0, BIN_N - 1);
        int bylo = clamp_int(static_cast<int>((a.ylo - st.ymin) * bin_inv_dy), 0, BIN_N - 1);
        int byhi = clamp_int(static_cast<int>((a.yhi - st.ymin) * bin_inv_dy), 0, BIN_N - 1);
        for (int by = bylo; by <= byhi; ++by)
            for (int bx = bxlo; bx <= bxhi; ++bx)
                bins[by * BIN_N + bx].push_back(ti);
    }

    /* Per-cell point-in-tri test using the bin. */
    auto point_in_tri = [&](std::uint32_t ti, double x, double y) {
        const auto& tri = mesh.tris[ti];
        const auto& P0 = mesh.padded_src[tri.a];
        const auto& P1 = mesh.padded_src[tri.b];
        const auto& P2 = mesh.padded_src[tri.c];
        const double T00 = P0[0] - P2[0], T01 = P1[0] - P2[0];
        const double T10 = P0[1] - P2[1], T11 = P1[1] - P2[1];
        const double det = T00 * T11 - T01 * T10;
        const double dx = x - P2[0], dy = y - P2[1];
        const double b0 = ( T11 * dx - T01 * dy) / det;
        const double b1 = (-T10 * dx + T00 * dy) / det;
        const double b2 = 1.0 - b0 - b1;
        return (b0 >= -1e-10) && (b1 >= -1e-10) && (b2 >= -1e-10);
    };

    /* Pass 1: assign each cell its containing triangle via bin + point-in-tri.
     * Fills every inside-hull cell exactly; cells outside the triangular octant
     * domain (the corners of this square grid) stay -1 for now. */
    const int GN = H9_CT_GRID_N;
    for (int iy = 0; iy < GN; ++iy) {
        const double y = st.ymin + iy * (st.ymax - st.ymin) / (GN - 1);
        for (int ix = 0; ix < GN; ++ix) {
            const double x = st.xmin + ix * (st.xmax - st.xmin) / (GN - 1);
            const auto& candidates = bins[bin_idx(x, y)];
            for (std::uint32_t ti : candidates) {
                if (point_in_tri(ti, x, y)) {
                    st.grid_tri[iy * GN + ix] = static_cast<std::int32_t>(ti);
                    break;
                }
            }
        }
    }

    /* Pass 2: flood the remaining outside-hull cells from their filled
     * neighbours via a multi-source BFS. grid_tri is only a *seed* for
     * find_triangle's walk, so a spatially-near triangle is all an outside cell
     * needs — the walk corrects to the true one for any in-domain query. This
     * is O(grid) total and replaces a per-cell bin-spiral that cost ~12 s for
     * the ~50% of grid cells lying outside the octant triangle. */
    {
        std::vector<std::int32_t> queue;
        queue.reserve(static_cast<std::size_t>(GN) * GN);
        for (int i = 0; i < GN * GN; ++i)
            if (st.grid_tri[i] >= 0) queue.push_back(i);
        const int dnx[4] = { 1, -1, 0, 0 };
        const int dny[4] = { 0, 0, 1, -1 };
        for (std::size_t head = 0; head < queue.size(); ++head) {
            const int idx = queue[head];
            const int cx = idx % GN, cy = idx / GN;
            const std::int32_t ti = st.grid_tri[idx];
            for (int k = 0; k < 4; ++k) {
                const int nx = cx + dnx[k], ny = cy + dny[k];
                if (nx < 0 || nx >= GN || ny < 0 || ny >= GN) continue;
                const int nidx = ny * GN + nx;
                if (st.grid_tri[nidx] < 0) {
                    st.grid_tri[nidx] = ti;
                    queue.push_back(nidx);
                }
            }
        }
    }
    H9_CT_BENCH_T(t_grid1);
    H9_CT_BENCH_MS("grid fill", t_grid0, t_grid1);
#ifdef H9_CT_BENCH
    {
        std::size_t n_unfilled = 0;
        for (int i = 0; i < GN * GN; ++i) if (st.grid_tri[i] < 0) ++n_unfilled;
        std::fprintf(stderr, "  [ct] cells still -1 after BFS: %zu / %d\n",
                     n_unfilled, GN * GN);
    }
#endif
    return st;
}

} /* namespace h9 */

#endif /* H9_CT_H */
