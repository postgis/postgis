/* h9_warp.h — public WarpState API for forward and inverse authalic warp.
 *
 * Port of hhg9/domains/octahedral_barycentric.py :: AuthalicWarp.do /
 * AuthalicWarp.undo (lines 316-475). Composed of:
 *
 *   load .h9warp        : h9_warp_io.h
 *   ghost padding       : h9_warp_mesh.h
 *   vertex gradients    : h9_ct.h :: estimate_gradients_2d_global
 *   Alfeld-CT coeffs    : h9_ct.h :: ct_coeffs_for_tri
 *   729² grid index     : h9_ct.h :: build_ct_state
 *
 * `WarpState` holds all of the above plus the runtime knobs (Newton
 * iteration count, edge-band tolerance). Build once at extension load.
 */
#ifndef H9_WARP_H
#define H9_WARP_H

#include "h9_ct.h"
#include "h9_warp_io.h"
#include "h9_warp_mesh.h"

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace h9 {

struct WarpState {
    H9WarpData      data;
    WarpMesh        mesh;
    VertexNeighbors vn;
    CTState         ct;
    int             newton_iter   = 25;
    double          edge_tol      = 1e-7;     /* lateral-edge band */
    bool            edge_bypass   = true;     /* LATERAL_EDGE_BYPASS */
    double          newton_h      = 1e-7;     /* finite-diff step for J */
    double          newton_safe   = 1e-12;    /* min |det| for Cramer */

    /* Cached constants. Mode-0 lateral edges: y = ±√3·x − √6/3. */
    double R3   = std::sqrt(3.0);
    double W_eq = std::sqrt(6.0) / 3.0;
};

/* Build a fully-loaded WarpState from a .h9warp file. */
inline bool build_warp_state(const std::string& path,
                             WarpState&         out,
                             std::string*       err = nullptr,
                             int                grad_maxiter = 2000,
                             double             grad_tol     = 1e-12)
{
    if (!load_h9warp(path, out.data, err)) return false;
    out.mesh = build_warp_mesh(out.data.deltas, out.data.header.level, out.data.header.mode);
    out.vn   = build_vertex_neighbors(out.mesh.padded_src.size(), out.mesh.tris);

    const std::size_t n = out.mesh.padded_src.size();
    std::vector<double> fdx(n), fdy(n);
    for (std::size_t i = 0; i < n; ++i) {
        fdx[i] = out.mesh.padded_diff[i][0];
        fdy[i] = out.mesh.padded_diff[i][1];
    }
    std::vector<std::array<double, 2>> gdx, gdy;
    estimate_gradients_2d_global(out.mesh.padded_src, out.vn, fdx.data(), gdx,
                                 grad_maxiter, grad_tol);
    estimate_gradients_2d_global(out.mesh.padded_src, out.vn, fdy.data(), gdy,
                                 grad_maxiter, grad_tol);
    out.ct = build_ct_state(out.mesh, fdx.data(), fdy.data(), gdx, gdy);
    return true;
}

/* Forward warp: b_raw → b_oct in mode-`mo`.
 *
 * mo=0 keeps y as-is; mo=1 mirrors y on entry and exit so the warp
 * mesh (built for mode-0) can be reused for the antipodal octant. */
inline void warp_do(const WarpState& ws,
                    double rx, double ry, int mo,
                    double* wx, double* wy)
{
    const double sign = (mo == 0) ? 1.0 : -1.0;
    const double cx = rx;
    const double cy = ry * sign;

    /* Lateral-edge bypass: matches AuthalicWarp.do() lines 322-329.
     * Edges in mode-0 frame: y − R3·x + Ẇ = 0 (right) and
     *                        y + R3·x + Ẇ = 0 (left). */
    const bool on_right = std::fabs(cy - ws.R3 * cx + ws.W_eq) < ws.edge_tol;
    const bool on_left  = std::fabs(cy + ws.R3 * cx + ws.W_eq) < ws.edge_tol;
    if (ws.edge_bypass && (on_right || on_left)) {
        *wx = cx;
        *wy = cy * sign;
        return;
    }

    double b[3];
    const std::int32_t ti = ct_find_tri(ws.ct, cx, cy, b);
    double dx = 0.0, dy = 0.0;
    if (ti >= 0) {
        dx = ct_eval_with(ws.ct.coeffs_dx[ti], b);
        dy = ct_eval_with(ws.ct.coeffs_dy[ti], b);
    }
    *wx = cx + dx;
    *wy = (cy + dy) * sign;
}

/* Inverse warp: b_oct (in mode-`mo`) → b_raw.
 *
 * Newton in 2D with finite-difference Jacobian (2×2 Cramer solve) —
 * matches AuthalicWarp.undo() lines 408-445. Seeded with identity
 * (curr = target); the warp's max magnitude (~1e-2 b_oct) is well
 * inside Newton's quadratic basin. Edge points are identity-bypassed. */
inline void warp_undo(const WarpState& ws,
                      double wx_in, double wy_in, int mo,
                      double* rx, double* ry)
{
    const double sign = (mo == 0) ? 1.0 : -1.0;
    const double tx = wx_in;
    const double ty = wy_in * sign;

    const bool on_right = std::fabs(ty - ws.R3 * tx + ws.W_eq) < ws.edge_tol;
    const bool on_left  = std::fabs(ty + ws.R3 * tx + ws.W_eq) < ws.edge_tol;
    const bool on_edge  = (on_right || on_left);

    if (ws.edge_bypass && on_edge) {
        *rx = tx;
        *ry = ty * sign;
        return;
    }

    /* Identity seed. */
    double cx = tx, cy = ty;
    const double h = ws.newton_h;
    for (int it = 0; it < ws.newton_iter; ++it) {
        double b[3];
        const std::int32_t ti = ct_find_tri(ws.ct, cx, cy, b);
        const double dx = (ti < 0) ? 0.0 : ct_eval_with(ws.ct.coeffs_dx[ti], b);
        const double dy = (ti < 0) ? 0.0 : ct_eval_with(ws.ct.coeffs_dy[ti], b);
        const double ex = cx + dx - tx;
        const double ey = cy + dy - ty;

        /* Finite-diff Jacobian of (curr + warp(curr)) wrt curr. */
        double bp[3];
        const std::int32_t tx_ti = ct_find_tri(ws.ct, cx + h, cy, bp);
        const double dx_px = (tx_ti < 0) ? 0.0 : ct_eval_with(ws.ct.coeffs_dx[tx_ti], bp);
        const double dy_px = (tx_ti < 0) ? 0.0 : ct_eval_with(ws.ct.coeffs_dy[tx_ti], bp);
        const std::int32_t ty_ti = ct_find_tri(ws.ct, cx, cy + h, bp);
        const double dx_py = (ty_ti < 0) ? 0.0 : ct_eval_with(ws.ct.coeffs_dx[ty_ti], bp);
        const double dy_py = (ty_ti < 0) ? 0.0 : ct_eval_with(ws.ct.coeffs_dy[ty_ti], bp);

        const double a =  1.0 + (dx_px - dx) / h;   /* 1 + ∂dx/∂x */
        const double bj =        (dx_py - dx) / h;  /* ∂dx/∂y     */
        const double c  =        (dy_px - dy) / h;  /* ∂dy/∂x     */
        const double d  = 1.0 + (dy_py - dy) / h;   /* 1 + ∂dy/∂y */
        const double det = a * d - bj * c;

        double delta_x, delta_y;
        if (std::fabs(det) > ws.newton_safe) {
            delta_x = -(d * ex - bj * ey) / det;
            delta_y = -(a * ey - c  * ex) / det;
        } else {
            /* Singular Jacobian — fall back to simple Picard step. */
            delta_x = -ex;
            delta_y = -ey;
        }
        cx += delta_x;
        cy += delta_y;
    }

    *rx = cx;
    *ry = cy * sign;
}

} /* namespace h9 */

#endif /* H9_WARP_H */
