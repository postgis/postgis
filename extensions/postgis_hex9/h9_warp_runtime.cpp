/* h9_warp_runtime.cpp — single TU that owns the global WarpState and
 * the 2 MB embedded sidecar blob. Including h9_warp_embedded.h here
 * (and only here) keeps the 13 MB ASCII expansion out of other TUs'
 * preprocessor work.
 */
#include "h9_warp_runtime.h"
#include "h9_warp_embedded.h"

namespace h9 {

WarpState g_warp_state;
bool      g_warp_state_ready = false;
bool      g_warp_use         = true;  /* default ON; PostGIS GUC may override */

bool h9_warp_init_embedded(std::string* err,
                           int    grad_maxiter,
                           double grad_tol)
{
    if (g_warp_state_ready) return true;
    H9WarpData data;
    if (!load_h9warp(EMBEDDED_WARP_DATA, EMBEDDED_WARP_SIZE, data, err))
        return false;
    g_warp_state.data = std::move(data);
    g_warp_state.mesh = build_warp_mesh(g_warp_state.data.deltas,
                                        g_warp_state.data.header.level,
                                        g_warp_state.data.header.mode);
    g_warp_state.vn = build_vertex_neighbors(g_warp_state.mesh.padded_src.size(),
                                              g_warp_state.mesh.tris);

    const std::size_t n = g_warp_state.mesh.padded_src.size();
    std::vector<double> fdx(n), fdy(n);
    for (std::size_t i = 0; i < n; ++i) {
        fdx[i] = g_warp_state.mesh.padded_diff[i][0];
        fdy[i] = g_warp_state.mesh.padded_diff[i][1];
    }
    std::vector<std::array<double, 2>> gdx, gdy;
    estimate_gradients_2d_global(g_warp_state.mesh.padded_src, g_warp_state.vn,
                                 fdx.data(), gdx, grad_maxiter, grad_tol);
    estimate_gradients_2d_global(g_warp_state.mesh.padded_src, g_warp_state.vn,
                                 fdy.data(), gdy, grad_maxiter, grad_tol);
    g_warp_state.ct = build_ct_state(g_warp_state.mesh,
                                      fdx.data(), fdy.data(), gdx, gdy);
    g_warp_state_ready = true;
    return true;
}

bool h9_warp_init_from_path(const std::string& path,
                            std::string*       err,
                            int                grad_maxiter,
                            double             grad_tol)
{
    if (g_warp_state_ready) return true;
    if (!build_warp_state(path, g_warp_state, err, grad_maxiter, grad_tol))
        return false;
    g_warp_state_ready = true;
    return true;
}

} /* namespace h9 */
