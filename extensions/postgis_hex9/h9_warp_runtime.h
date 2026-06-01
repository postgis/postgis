/* h9_warp_runtime.h — process-global WarpState + thin h9_warp_fwd/inv
 * delegators that match the existing legacy API.
 *
 * Lifecycle:
 *   - The state is a single h9::WarpState owned by one TU (see
 *     h9_warp_runtime.cpp). Declared `extern` here so any header that
 *     wants to use it via the thin h9_warp_fwd/inv wrappers can.
 *   - Caller responsibility: call h9_warp_init_embedded() exactly once,
 *     early, before any h9_warp_fwd/inv invocation. For the PostGIS
 *     extension that means _PG_init(); for the CLI it means main().
 *   - h9_warp_init_embedded() reads the .h9warp blob baked into
 *     h9_warp_embedded.h and runs the full WarpState build (~0.9 s).
 *     (Was ~13 s until the 729² grid fill switched from a per-cell
 *     bin-spiral to a multi-source BFS flood — see h9_ct.h build_ct_state.)
 *
 * The h9_warp_fwd / h9_warp_inv shims preserve the legacy call-site
 * signature so h9_addressing.h, main.cpp, lwgeom_hex9.cpp need no
 * source edits. They are only compiled when H9_WARP_ENABLE is set;
 * otherwise the warp is a no-op identity.
 */
#ifndef H9_WARP_RUNTIME_H
#define H9_WARP_RUNTIME_H

#include "h9_warp.h"

#include <string>

namespace h9 {

/* Global, lazily-but-explicitly initialised. Owned by the TU that
 * defines it in h9_warp_runtime.cpp. */
extern WarpState g_warp_state;
extern bool      g_warp_state_ready;

/* Runtime apply toggle. true (default) ⇒ h9_warp_fwd/inv apply the
 * authalic correction when state is ready; false ⇒ identity. Exposed
 * in PostGIS as the `hex9.use_warp` GUC; CLI / standalone builds get
 * the default value and need not touch it. Decoupled from state-ready
 * so the GUC can pre-disable warp without requiring the 13 s init to
 * be skipped (or vice-versa). */
extern bool      g_warp_use;

/* Build g_warp_state from the embedded sidecar blob baked into
 * h9_warp_embedded.h. Idempotent (no-op after first success). Returns
 * true on success; on failure `err` is filled and g_warp_state_ready
 * stays false. */
bool h9_warp_init_embedded(std::string* err = nullptr,
                           int    grad_maxiter = 2000,
                           double grad_tol     = 1e-12);

/* Optional alternate init from a runtime sidecar file (e.g. for tests
 * or operator-supplied warpfiles). */
bool h9_warp_init_from_path(const std::string& path,
                            std::string* err = nullptr,
                            int    grad_maxiter = 2000,
                            double grad_tol     = 1e-12);

} /* namespace h9 */

/* ── Legacy-API delegators, header-only -─────────────────────────────
 * Replace the static-data versions formerly in h9_math.h:281-413. The
 * shims pass through to identity when (a) the state isn't built (CLI
 * forgot to call h9_warp_init_embedded), or (b) the operator has
 * disabled warp at runtime via `SET hex9.use_warp = off`. */
#if H9_WARP_ENABLE
static inline void h9_warp_fwd(double rx, double ry, int oct_mode,
                               double *wx, double *wy)
{
    if (!h9::g_warp_use || !h9::g_warp_state_ready) { *wx = rx; *wy = ry; return; }
    h9::warp_do(h9::g_warp_state, rx, ry, oct_mode, wx, wy);
}

/* Hint overload retained for API compatibility — hints are unused now
 * that the inverse seed is identity-based. */
static inline void h9_warp_fwd(double rx, double ry, int oct_mode,
                               double *wx, double *wy,
                               double /*hint_x*/, double /*hint_y*/)
{
    h9_warp_fwd(rx, ry, oct_mode, wx, wy);
}

static inline void h9_warp_inv(double wx, double wy, int oct_mode,
                               double *rx, double *ry)
{
    if (!h9::g_warp_use || !h9::g_warp_state_ready) { *rx = wx; *ry = wy; return; }
    h9::warp_undo(h9::g_warp_state, wx, wy, oct_mode, rx, ry);
}
#endif /* H9_WARP_ENABLE */

#endif /* H9_WARP_RUNTIME_H */
