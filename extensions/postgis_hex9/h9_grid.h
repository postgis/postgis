/* h9_grid.h — header-only standalone H9 grid enumerator.
 *
 * Ports Python HexMesh.create_clipped: integer-UV supercell BFS with lat/lon
 * bbox pruning, then per-supercell c2 hex emission. No sampling, no Postgres.
 *
 * Output is sorted by UUID and deduplicated. Each H9GridCell carries the bin
 * UUID, the originating (oid, ia, ib, c2), the geographic centroid, and the
 * 6 vertex lon/lats — enough to draw and compare against Python.
 *
 * H9_WARP_ENABLE is treated as 0 (binning/grid path), matching Python's
 * b_oct.no_warp() convention in the grid pipeline.
 */
#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <algorithm>

#include "h9_math.h"
#include "h9_addressing.h"
#include "h9_uv_lattice.h"

struct H9GridCell {
    uint8_t uuid[16];
    int     oid;           /* Python oid of the producing supercell */
    int     c2;
    int64_t ia, ib;        /* mode-0 supercell origin at layer L (integer UV) */
    bool    ext;           /* half-hex: v4/v5 reflected to neighbour octant */
    int     poid[6];       /* per-vertex oid (== oid except reflected v4/v5) */
    int64_t pu[6], pv[6];  /* per-vertex integer-UV in each vertex's own frame */
    double  cen_lon, cen_lat;
    double  vlon[6], vlat[6];
};

namespace h9grid {

/* ── Per-oid b_oct → c_oct matrices ────────────────────────────────────────
 *
 * Mirrors Python's OctahedralBaryRaw matrix construction (Python:
 * hhg9/domains/octahedral_bary_raw.py, lines 70-98).  Each row of `M[oid]`
 * is one of the three octant-face basis vectors, pre-scaled by 1/sqrt(2),
 * 1/sqrt(6), 1/sqrt(3) respectively.
 *
 * Computation:
 *   c_oct = (rotated_xyz) @ M[oid]
 *   where rotated_xyz = (rot_z(+th) * (cx, cy), 1/sqrt(3)),
 *         th          = H9_OID_TH[oid] * pi/3.
 *
 * Generated from a one-shot probe of `b_raw.projs[face].matrix` in Python —
 * see py_global_mesh.py for the dump that produced this table.  Each
 * matrix is orthogonal with det=1; det check & opposite-face check live
 * in OctahedralBaryRaw._validate_matrices. */
static const double H9_BRAW_M[8][3][3] = {
    /* oid=0 (NEA) th_pi3=2 mode=0 */
    {{ -0.70710678118654746, +0.0,                  +0.70710678118654746 },
     { +0.40824829046386307, -0.81649658092772615, +0.40824829046386307 },
     { +0.57735026918962584, +0.57735026918962584, +0.57735026918962584 }},
    /* oid=1 (NEP) th_pi3=5 mode=1 */
    {{ +0.0,                 -0.70710678118654746, +0.70710678118654746 },
     { +0.81649658092772615, +0.40824829046386307, +0.40824829046386307 },
     { -0.57735026918962584, +0.57735026918962584, +0.57735026918962584 }},
    /* oid=2 (NWA) th_pi3=5 mode=1 */
    {{ +0.0,                 +0.70710678118654746, +0.70710678118654746 },
     { -0.81649658092772615, -0.40824829046386307, +0.40824829046386307 },
     { +0.57735026918962584, -0.57735026918962584, +0.57735026918962584 }},
    /* oid=3 (NWP) th_pi3=2 mode=0 */
    {{ +0.70710678118654746, +0.0,                  +0.70710678118654746 },
     { -0.40824829046386307, +0.81649658092772615, +0.40824829046386307 },
     { -0.57735026918962584, -0.57735026918962584, +0.57735026918962584 }},
    /* oid=4 (SEA) th_pi3=5 mode=1 */
    {{ +0.0,                 -0.70710678118654746, -0.70710678118654746 },
     { -0.81649658092772615, +0.40824829046386307, -0.40824829046386307 },
     { +0.57735026918962584, +0.57735026918962584, -0.57735026918962584 }},
    /* oid=5 (SEP) th_pi3=2 mode=0 */
    {{ +0.70710678118654746, +0.0,                  -0.70710678118654746 },
     { -0.40824829046386307, -0.81649658092772615, -0.40824829046386307 },
     { -0.57735026918962584, +0.57735026918962584, -0.57735026918962584 }},
    /* oid=6 (SWA) th_pi3=2 mode=0 */
    {{ -0.70710678118654746, +0.0,                  -0.70710678118654746 },
     { +0.40824829046386307, +0.81649658092772615, -0.40824829046386307 },
     { +0.57735026918962584, -0.57735026918962584, -0.57735026918962584 }},
    /* oid=7 (SWP) th_pi3=5 mode=1 */
    {{ +0.0,                 +0.70710678118654746, -0.70710678118654746 },
     { +0.81649658092772615, -0.40824829046386307, -0.40824829046386307 },
     { -0.57735026918962584, -0.57735026918962584, -0.57735026918962584 }},
};

/* Per-oid Z-rotation theta in units of pi/3.  Mirrors H9O.oid_tht.
 * Note: this is INDEXED BY OID (no XOR-7 transform) — distinct from
 * the H9_ORIENT_TH table in h9_math.h which uses `oid ^ 7` indexing.
 * The Python projection rotates (cx, cy) CCW by +th BEFORE the matrix
 * multiplication; the matrix then accounts for the mode-0 / mode-1
 * pole convention (mode-0 pole at down-apex, mode-1 pole at up-apex). */
static const int H9_BRAW_TH_PI3[8] = { 2, 5, 5, 2, 5, 2, 2, 5 };

/* b_oct (cx, cy) in `oid`'s frame → geographic (lon, lat).
 *
 * Mirrors Python's reg.project([b_oct, b_raw, c_oct, c_ell, g_gcd]) chain
 * exactly (in the b_oct.no_warp() configuration the grid pipeline uses):
 *
 *   b_oct = b_raw       (no warp)
 *   b_raw → c_oct       (Python OctantBraw.backward)
 *      1. xyz = (cx, cy, 1/sqrt(3))                — lift 2D into 3D
 *      2. rotate xyz CCW around Z by th            — orient.T
 *      3. c_oct = xyz @ matrix                     — per-oid 3x3 mult
 *   c_oct → c_ell       — h9_c_oct_to_c_ell (authalic projection)
 *   c_ell → g_gcd       — h9_c_ell_to_lonlat (spherical to deg)
 *
 * DO NOT replace the matrix path with a bary-formula in the rotated
 * frame: this was tried (briefly) and is invisibly broken — see
 * py_global_mesh.py + diff_global_mesh.py.  The trap is that (0, 0) is
 * a fixed point of the per-oid Z-rotation, so single-point sanity tests
 * pass while the bug shows up everywhere else.  Python uses the matrix
 * form directly; mirror it directly.
 *
 * If `strict`, the projection is rejected (returns false) when the point
 * lies outside the octant triangle (any face barycentric < -OOT_EPS).
 * The face barycentric is recovered from c_oct as |X|, |Y|, |Z| (each
 * octant face triangle has corners at the cube vertices (±1, 0, 0),
 * (0, ±1, 0), (0, 0, ±1)).
 *
 * If `!strict`, the strict-out-of-triangle check is skipped.  We always
 * compute lon/lat regardless. */
static inline bool cxcy_to_lonlat(double cx, double cy, int oid,
                                  double *lon, double *lat,
                                  H9BOct *bc_out = nullptr,
                                  bool strict = true) {
    constexpr double OOT_EPS = 1e-9;     /* out-of-triangle tolerance */
    const int oct_mode = (int)H9_OID_MO[oid];

#if H9_WARP_ENABLE
    /* Apply the authalic warp: lattice (cx, cy) is the regular b_raw
     * coordinate; warp_fwd maps it to the b_oct frame that the rest of
     * this function projects to lon/lat. When the runtime GUC
     * `hex9.use_warp` is off (or the WarpState wasn't built) the shim
     * is identity, so the grid behaves as before.
     *
     * Why here: the entire grid path funnels through this function for
     * every cell vertex AND centre, so injecting once gives consistent
     * warped output across h9_grid (cell polygons) and h9_cell (single
     * cell lookup) without scattering calls. */
    {
        double bx, by;
        h9_warp_fwd(cx, cy, oct_mode, &bx, &by);
        cx = bx; cy = by;
    }
#endif

    /* Step 1+2: lift to 3D and rotate by +th around Z.
     * Python equivalent: xyz @ orient.T, where orient = R_z(+th). */
    const double th = H9_BRAW_TH_PI3[oid] * (M_PI / 3.0);
    const double ct = std::cos(th), st = std::sin(th);
    const double rx = cx * ct - cy * st;
    const double ry = cx * st + cy * ct;
    const double rz = 1.0 / std::sqrt(3.0);                   /* z_off */

    /* Step 3: multiply by per-oid matrix to get c_oct (X, Y, Z).
     * Each component is one axis of the octant face triangle:
     *   X = rx * M[0][0] + ry * M[1][0] + rz * M[2][0]   ... etc. */
    const double (*M)[3] = H9_BRAW_M[oid];
    const double X = rx * M[0][0] + ry * M[1][0] + rz * M[2][0];
    const double Y = rx * M[0][1] + ry * M[1][1] + rz * M[2][1];
    const double Z = rx * M[0][2] + ry * M[1][2] + rz * M[2][2];

    /* Strict out-of-triangle check: the face triangle for oid sits on the
     * plane X+Y+Z=±1 in c_oct space (sign matches the oid's eZ-direction).
     * Inside the triangle, each face-barycentric value is non-negative;
     * outside, at least one is negative.  The signed bc components (= X,Y,Z)
     * relate to the unsigned face-bary via the per-axis sign masks. */
    if (strict) {
        const double su = (oid & 1) ? -1.0 : 1.0;
        const double sv = (oid & 2) ? -1.0 : 1.0;
        const double sw = (oid & 4) ? -1.0 : 1.0;
        const double au = su * X;
        const double av = sv * Y;
        const double aw = sw * Z;
        if (au < -OOT_EPS || av < -OOT_EPS || aw < -OOT_EPS) return false;
    }

    /* Step 4+5: c_oct → c_ell (authalic) → lon/lat (spherical degrees).
     * Both calls live in h9_math.h.  h9_boct_to_lonlatdeg is the same path
     * run_csv_test exercises in round-trip — but note that run_csv_test
     * does NOT cover this enclosing function (it uses h9_lonlatdeg_to_boct
     * directly), so a green csv test tells you nothing about cxcy_to_lonlat. */
    H9BOct bc;
    bc.u = X; bc.v = Y; bc.w = Z;
    bc.oct_i    = oid;
    bc.oct_mode = oct_mode;
    h9_boct_to_lonlatdeg(bc, lon, lat);
    if (bc_out) *bc_out = bc;
    return !std::isnan(*lon) && !std::isnan(*lat);
}

/* Forward decl — classify_band is defined further down (with uuid_from_cxcy). */
static inline uint8_t classify_band(double xdot, double y, int p_mo);

/* Locate the cell at `layer` whose region contains the point (cx, cy, oid).
 *
 * Returns the cell identity (c2, ia_leaf, ib_leaf, ext) which can then be
 * passed to uuid_from_iauv to get the canonical grid-matching UUID for the
 * point.  Built for hexbin: ensures the binner produces UUIDs that are
 * guaranteed to appear in the grid output for the same bbox+layer (by
 * construction, since both go through uuid_from_iauv).
 *
 * Strategy: walk the floating-point descent (same code as uuid_from_cxcy)
 * but track the running INTEGER (ia_leaf, ib_leaf) — sum of chosen
 * H9_MODE_OFS · 3^(layer-1-k) at each level — instead of accumulating
 * float offsets.  After `layer` steps, (ia_leaf, ib_leaf) IS the leaf
 * supercell's origin in L_layer integer-UV.  Determine c2 by which of the
 * 3 c2 hex centroids (at integer-UV offsets (1,1)/(1,-1)/(-2,0) inside the
 * supercell) is nearest to the point's leaf-relative residual.  Compute
 * ext via the same outside_oct test the BFS emit uses.
 *
 * Boundary caveat: the float descent here inherits cxcy's boundary fuzz —
 * at the ~0.13% of points whose descent path is ambiguous, the binner
 * may assign the point to an adjacent cell.  The assigned cell IS in the
 * grid (so SQL aggregations don't lose rows), but a tiny fraction of
 * points are mis-bucketed by one cell at the boundary.  Same behaviour
 * Python's hex_layer exhibits.  A full fix would use exact integer
 * descent on the point's (cx, cy) — deferred. */
static inline void cell_from_cxcy(double cx, double cy, int oid, int layer,
                                  int *c2_out, int64_t *ia_out, int64_t *ib_out,
                                  bool *ext_out) {
    int     p_mo = (int)H9_OID_MO[oid];
    int64_t ia_leaf = 0, ib_leaf = 0;
    int64_t scale_int = 1;
    for (int i = 0; i < layer; i++) scale_int *= 3;        /* = 3^layer */

    double cx_run = cx, cy_run = cy;
    const double R3 = std::sqrt(3.0);

    /* `layer` descent steps — identify the leaf SUPERCELL (not the
     * c2-distinguishing leaf-child).  Mirrors the first layer steps of
     * uuid_from_cxcy exactly, including the bad-cid NN fallback. */
    for (int k = 0; k < layer; k++) {
        const double  xdot  = cx_run * R3;
        const uint8_t cid   = classify_band(xdot, cy_run, p_mo);
        const double *offx  = (p_mo == 1) ? H9.UP_X    : H9.DN_X;
        const double *offy  = (p_mo == 1) ? H9.UP_Y    : H9.DN_Y;
        const int (*ofs_i)[2] = (p_mo == 1) ? H9_MODE1_OFS         : H9_MODE0_OFS;
        const int *cm         = (p_mo == 1) ? H9_MODE1_CHILD_MODE  : H9_MODE0_CHILD_MODE;
        const uint8_t *ctab   = (p_mo == 1) ? H9_UP_CIDS            : H9_DN_CIDS;

        int j = -1;
        for (int kk = 0; kk < 9; kk++) if (ctab[kk] == cid) { j = kk; break; }
        if (j < 0) {
            double best = 1e300; int best_j = 0;
            for (int kk = 0; kk < 9; kk++) {
                const double dx = cx_run - offx[kk], dy = cy_run - offy[kk];
                const double d2 = dx*dx + dy*dy;
                if (d2 < best) { best = d2; best_j = kk; }
            }
            j = best_j;
        }

        scale_int /= 3;                                    /* L_(k+1) child scale */
        ia_leaf += (int64_t)ofs_i[j][0] * scale_int;
        ib_leaf += (int64_t)ofs_i[j][1] * scale_int;
        cx_run = (cx_run - offx[j]) * 3.0;
        cy_run = (cy_run - offy[j]) * 3.0;
        p_mo = cm[j];
    }

    /* After `layer` rescalings, (cx_run, cy_run) is the point's position in
     * the leaf's rescaled frame.  The 3 c2 hex centroids in this frame are
     * at (U1, V3), (U1, -V3), (-2·U1, 0) — i.e. integer offsets (1,1),
     * (1,-1), (-2,0) × (U1, V3).  Pick c2 by nearest centroid (Voronoi
     * tessellation of the supercell by the 3 c2 hex centres). */
    static const double c2_cx[3] = {  1.0 * H9_UV_U1,  1.0 * H9_UV_U1, -2.0 * H9_UV_U1 };
    static const double c2_cy[3] = {  1.0 * H9_UV_V3, -1.0 * H9_UV_V3,  0.0 };
    int    best_c2 = 0;
    double best_d2 = 1e300;
    for (int c2 = 0; c2 < 3; c2++) {
        const double du = cx_run - c2_cx[c2];
        const double dv = cy_run - c2_cy[c2];
        const double d2 = du*du + dv*dv;
        if (d2 < best_d2) { best_d2 = d2; best_c2 = c2; }
    }
    *c2_out = best_c2;
    *ia_out = ia_leaf;
    *ib_out = ib_leaf;

    /* ext: same gated outside_oct test the BFS emit uses.  Mode-0 octants
     * own the seam reflection; mode-1 octants leave v4/v5 in place and
     * never set ext.  Mirrors Python _assemble_clipped (grid.py:1042-1055). */
    const int64_t s_int = (int64_t)std::llround(std::pow(3.0, (double)layer));
    auto outside_oct = [s_int](int64_t u, int64_t v) -> bool {
        return (v > s_int) || (u - v > 2*s_int) || (u + v < -2*s_int);
    };
    const int64_t u4 = (int64_t)H9_HI[0][best_c2][4][0] + ia_leaf;
    const int64_t v4 = (int64_t)H9_HI[0][best_c2][4][1] + ib_leaf;
    const int64_t u5 = (int64_t)H9_HI[0][best_c2][5][0] + ia_leaf;
    const int64_t v5 = (int64_t)H9_HI[0][best_c2][5][1] + ib_leaf;
    *ext_out = (H9_OID_MO[oid] == 0)
               && outside_oct(u4, v4) && outside_oct(u5, v5);
}

/* Inverse of cxcy_to_lonlat: take a lon/lat point and recover (cx, cy, oid)
 * in the post-orient b_oct frame.  Composition: lonlat → bc (via the
 * existing h9_lonlatdeg_to_boct) → derive oid from bc signs → invert the
 * per-oid matrix to recover the rotated (rx, ry, rz) → rotate by -th to
 * get (cx, cy).
 *
 * Used by the `hexbin` subcommand to feed arbitrary geographic points
 * into uuid_from_cxcy (the grid-canonical encoder that matches Python's
 * hex_layer).  The beam encoder h9_boct_to_uuid_beam does NOT match
 * Python — see the encoder header above for the difference. */
static inline void lonlatdeg_to_cxcy_oid(double lon_deg, double lat_deg,
                                         double *cx, double *cy, int *oid_out) {
    const H9BOct bc = h9_lonlatdeg_to_boct(lon_deg, lat_deg);

    /* oid is determined by sign of each axis — same bit packing as h9_math.h:
     *   bit0 = (u < 0), bit1 = (v < 0), bit2 = (w < 0). */
    const int oid = ((bc.u < 0.0) ? 1 : 0)
                  | ((bc.v < 0.0) ? 2 : 0)
                  | ((bc.w < 0.0) ? 4 : 0);
    *oid_out = oid;

    /* Invert the per-oid matrix.  H9_BRAW_M is orthogonal (Python validates
     * this in OctahedralBaryRaw._validate_matrices), so M^-1 = M^T.  The
     * forward transform was: xyz @ M = (X, Y, Z), where xyz = (rx, ry, 1/sqrt(3))
     * and (X, Y, Z) are the signed bc components.  Therefore:
     *   xyz = (X, Y, Z) @ M^T,
     * which expands to:  rx = sum_k bc[k] * M[0][k], etc. */
    const double (*M)[3] = H9_BRAW_M[oid];
    const double rx = bc.u * M[0][0] + bc.v * M[0][1] + bc.w * M[0][2];
    const double ry = bc.u * M[1][0] + bc.v * M[1][1] + bc.w * M[1][2];
    /* rz = bc.u * M[2][0] + bc.v * M[2][1] + bc.w * M[2][2] — should be ≈ 1/sqrt(3) */

    /* Undo the orient rotation: forward applied R_z(+th), so inverse is R_z(-th). */
    const double th = -H9_BRAW_TH_PI3[oid] * (M_PI / 3.0);
    const double ct = std::cos(th), st = std::sin(th);
    *cx = rx * ct - ry * st;
    *cy = rx * st + ry * ct;
}

/* Integer-UV (pu, pv) at layer L → geographic (lon, lat). */
static inline bool uv_to_lonlat(int64_t pu, int64_t pv, int oid, double div_f,
                                double *lon, double *lat, H9BOct *bc_out = nullptr) {
    const double cx = (double)pu * H9_UV_U1 / div_f;
    const double cy = (double)pv * H9_UV_V3 / div_f;
    return cxcy_to_lonlat(cx, cy, oid, lon, lat, bc_out);
}

/* ── Containment-based UUID encoder (Python xy_regions port) ─────────────── */
/*
 *  The default C++ encoder (h9_boct_to_uuid_beam) does k-best nearest-neighbour
 *  descent in a post-orient frame.  Python's HexMesh.create_clipped encodes
 *  via hex_layer → hex_digits → xy_regions, which does CONTAINMENT
 *  partitioning using mode-aware band tests.  They agree for points well
 *  inside cells but diverge at cell boundaries, which used to be the source
 *  of same-UUID-different-position drift.
 *
 *  This function ports Python's xy_regions (region.py:574) + reg_hex_digits
 *  (addressing.py:654) into one routine specialised for bin-UUIDs at a
 *  given layer.  It expects (cx, cy) in the same b_oct frame Python's
 *  classify_band consumes — the POST-orient frame.  Our enumerate path
 *  produces (cx, cy) in that frame already (H9_HI integer-UV × (U1, V3))
 *  so no rotation is needed at the entry of this function — see the matched
 *  comment in cxcy_to_lonlat above.
 *
 *  See also: h9_boct_to_uuid_beam in h9_addressing.h (kept for posterity /
 *  non-grid uses — use h9grid::uuid_from_cxcy for grid-canonical UUIDs
 *  that match Python).
 */

/* Classify (ẋ, y) into one of 96 cell ids using mode-dependent band tests.
 * ẋ = x * sqrt(3).  Mirrors classifier.py::classify_mode_cell exactly.
 *
 * NOTE: Python casts to np.longdouble before the band tests — but on Apple
 * Silicon both `long double` (C) and `np.longdouble` (numpy) are 64-bit
 * doubles, so the cast is a no-op there.  The real difference between
 * Python and us at the descent boundary is _recover (region.py:447) — Python
 * ULP-nudges the point when classify returns a cid outside the legal child
 * set, where we just nearest-neighbour-fall-back.  See uuid_from_cxcy below.
 */
static inline uint8_t classify_band(double xdot, double y, int p_mo) {
    const double h_dot = H9.H / 3.0;     /* Ḣ = h/3 */
    const double w_dot = 2.0 * h_dot;    /* Ẇ = 2Ḣ;  also = ΛC = -VF */
    const double ymx   = y - xdot;
    const double ypx   = y + xdot;
    int h_id, p_id, n_id;
    if (p_mo == 0) {
        /* mode 0 (down) — strict on outer bands, inclusive on inner */
        h_id = (y > w_dot) ? 0 : (y >= h_dot) ? 1 : (y >= 0.0) ? 2 :
               (y > -h_dot) ? 3 : (y > -w_dot) ? 4 : 5;
        p_id = (ymx >= w_dot) ? 0 : (ymx >= 0.0) ? 1 : (ymx >= -w_dot) ? 2 : 3;
        n_id = (ypx < -w_dot) ? 0 : (ypx <= 0.0) ? 1 : (ypx < w_dot) ? 2 : 3;
    } else {
        /* mode 1 (up) — strict/inclusive swapped on the central tier */
        h_id = (y > w_dot) ? 0 : (y > h_dot) ? 1 : (y >= 0.0) ? 2 :
               (y > -h_dot) ? 3 : (y > -w_dot) ? 4 : 5;
        p_id = (ymx > w_dot) ? 0 : (ymx > 0.0) ? 1 : (ymx > -w_dot) ? 2 : 3;
        n_id = (ypx <= -w_dot) ? 0 : (ypx <= 0.0) ? 1 : (ypx <= w_dot) ? 2 : 3;
    }
    return (uint8_t)((h_id << 4) | (p_id << 2) | n_id);
}

/* ── Triangle-membership predicate (port of classifier.py::in_up/in_down) ─
 *
 * Tests whether (ẋ, y) is inside the supercell for the given parent mode,
 * with the same tolerance Python uses (256·eps·max(1,|ẋ|,|y|)).  Used by
 * the descent loops to detect "out-of-scope" cases — points that drifted
 * past a parent's triangle edge during ×3 rescale and whose classify_band
 * result is geometrically unreliable.  Matches Python's `bad` mask which
 * triggers _recover on either an out-of-scope point or an out-of-mode cid.
 */
static inline bool h9_in_scope(double xdot, double y, int p_mo) {
    const double h_dot = H9.H / 3.0;            /* Ḣ */
    const double w_dot = 2.0 * h_dot;           /* Ẇ = ΛC = -VF */
    const double scale = std::max(1.0, std::max(std::fabs(xdot), std::fabs(y)));
    const double tol   = 256.0 * std::numeric_limits<double>::epsilon() * scale;
    if (p_mo == 1) {
        /* in_up: y ≥ ΛF, y − ẋ ≤ +Ẇ, y + ẋ ≤ +Ẇ.  ΛF = −Ḣ. */
        const double lam_f = -h_dot;
        return (y    >= lam_f - tol)
            && ((y - xdot) <= w_dot + tol)
            && ((y + xdot) <= w_dot + tol);
    } else {
        /* in_down: y ≤ VC, y − ẋ ≥ −Ẇ, y + ẋ ≥ −Ẇ.  VC = +Ḣ. */
        const double v_c = h_dot;
        return (y    <= v_c   + tol)
            && ((y - xdot) >= -w_dot - tol)
            && ((y + xdot) >= -w_dot - tol);
    }
}

/* ── ULP-nudge recovery (port of region.py::_recover, strategies 0+1) ─────
 *
 * When classify_band returns a cid not in the current parent's child set,
 * Python nudges (ẋ, y) one ULP toward the nearest lattice line and
 * re-classifies, rather than snapping to the nearest centroid.  Two distinct
 * points landing off-grid at the same level get nudged to different sides
 * and stay distinct; the NN fallback collapses them.
 *
 * Phase 2a: strategies 0 (nudge-y) and 1 (nudge-ẋ).  soft/hard_clamp
 * (strategies 2..6) not yet ported — caller still falls through to NN if
 * both ULP nudges miss.
 */
struct H9RecoverStats {
    uint64_t nudge_y;
    uint64_t nudge_xdot;
    uint64_t soft_05;      /* soft_clamp at 0.5 ULP succeeded */
    uint64_t soft_10;      /* soft_clamp at 1.0 ULP succeeded */
    uint64_t failed;       /* recovery entered (j<0) but no strategy fixed */
    uint64_t preamble;     /* top-level !in_scope inputs that got nudged   */
};
/* Function-static (not `inline` variable) so any TU compiled pre-C++17 still
 * sees a single shared counter via the inline function's local-static rule. */
inline H9RecoverStats& h9_recover_stats() {
    static H9RecoverStats s = {0, 0, 0, 0, 0, 0};
    return s;
}

static inline double h9_ulp_nudge(double z, double unit) {
    if (unit == 0.0) return z;
    const double target = std::rint(z / unit) * unit;
    return std::nextafter(z, target);
}

/* Local ULP size at z — matches np.spacing(z). */
static inline double h9_spacing(double z) {
    return std::nextafter(z, std::numeric_limits<double>::infinity()) - z;
}

/* True if x is within k ULPs of target — matches Python near_ulps. */
static inline bool h9_near_ulps(double x, double target, double k) {
    const double ulp = std::max(h9_spacing(target),
                                std::numeric_limits<double>::epsilon());
    return std::fabs(x - target) <= k * ulp;
}

/* Port of region.py::soft_clamp (single-point).  Snaps a point to the
 * triangle boundary IFF it is near the boundary (within tol_ulps) AND
 * actually out-of-bounds.  Returns true if (xdot, y) was modified. */
static inline bool h9_soft_clamp(double *p_xdot, double *p_y,
                                 int mode, double tol_ulps) {
    const double h_dot = H9.H / 3.0;
    const double w_dot = 2.0 * h_dot;           /* Ẇ */
    double xdot = *p_xdot, y = *p_y;

    if (mode == 1) {
        const double lam_c = +w_dot;            /* ΛC */
        const double lam_f = -h_dot;            /* ΛF */
        const bool near_apex = h9_near_ulps(y, lam_c, tol_ulps);
        const bool near_base = h9_near_ulps(y, lam_f, tol_ulps);
        if (near_apex && y > lam_c) {
            *p_y = lam_c; *p_xdot = 0.0; return true;
        }
        if (near_base && y < lam_f) {
            const double maxx = lam_c - lam_f;  /* full base width */
            *p_y = lam_f;
            *p_xdot = std::max(-maxx, std::min(maxx, xdot));
            return true;
        }
        if (!near_apex && !near_base) {
            const double maxax = lam_c - y;     /* |ẋ| limit on slant edges */
            if (xdot >  maxax && h9_near_ulps(xdot,  maxax, tol_ulps)) { *p_xdot =  maxax; return true; }
            if (xdot < -maxax && h9_near_ulps(xdot, -maxax, tol_ulps)) { *p_xdot = -maxax; return true; }
        }
    } else {
        const double v_f = -w_dot;              /* VF */
        const double v_c = +h_dot;              /* VC */
        const bool near_apex = h9_near_ulps(y, v_f, tol_ulps);
        const bool near_base = h9_near_ulps(y, v_c, tol_ulps);
        if (near_apex && y < v_f) {
            *p_y = v_f; *p_xdot = 0.0; return true;
        }
        if (near_base && y > v_c) {
            const double maxx = v_c - v_f;
            *p_y = v_c;
            *p_xdot = std::max(-maxx, std::min(maxx, xdot));
            return true;
        }
        if (!near_apex && !near_base) {
            const double maxax = y - v_f;
            if (xdot >  maxax && h9_near_ulps(xdot,  maxax, tol_ulps)) { *p_xdot =  maxax; return true; }
            if (xdot < -maxax && h9_near_ulps(xdot, -maxax, tol_ulps)) { *p_xdot = -maxax; return true; }
        }
    }
    return false;
}

/* Try strategies 0 then 1.  On success returns j ∈ [0,9) and mutates
 * *p_cx / *p_cy to the nudged values; on failure returns -1 (caller falls
 * back to nearest-neighbour). */
static inline int h9_recover_cell(double *p_cx, double *p_cy, int p_mo,
                                  const uint8_t *ctab) {
    const double R3       = std::sqrt(3.0);
    const double LAT_V    = H9_UV_V3 / 3.0;     /* h9k.lattice.V */
    const double LAT_UDOT = H9_UV_U1 * R3;      /* h9k.lattice.Ü = U·√3 */
    const double cx0 = *p_cx, cy0 = *p_cy;

    /* Strategy 0: nudge cy one ULP toward nearest k·V. */
    {
        const double cy_new = h9_ulp_nudge(cy0, LAT_V);
        if (cy_new != cy0) {
            const uint8_t cid = classify_band(cx0 * R3, cy_new, p_mo);
            for (int k = 0; k < 9; ++k) {
                if (ctab[k] == cid) {
                    *p_cy = cy_new;
                    ++h9_recover_stats().nudge_y;
                    return k;
                }
            }
        }
    }

    /* Strategy 1: nudge ẋ = cx·√3 one ULP toward nearest m·Ü; map back via /√3
     * (the /√3 round-trip costs ½ ULP, far below the boundary fuzz we're
     * recovering from; nudging in ẋ-space keeps step-size parity with Python). */
    {
        const double xdot0  = cx0 * R3;
        const double xdot_n = h9_ulp_nudge(xdot0, LAT_UDOT);
        if (xdot_n != xdot0) {
            const double cx_new = xdot_n / R3;
            const uint8_t cid   = classify_band(xdot_n, cy0, p_mo);
            for (int k = 0; k < 9; ++k) {
                if (ctab[k] == cid) {
                    *p_cx = cx_new;
                    ++h9_recover_stats().nudge_xdot;
                    return k;
                }
            }
        }
    }

    /* Strategies 2 & 3: soft_clamp at 0.5 then 1.0 ULP — snap to triangle
     * boundary if both near AND out-of-bounds.  Operates in ẋ-space. */
    {
        const double tols[2]    = {0.5, 1.0};
        uint64_t *counters[2]   = {&h9_recover_stats().soft_05,
                                   &h9_recover_stats().soft_10};
        for (int s = 0; s < 2; ++s) {
            double xdot = cx0 * R3;
            double y    = cy0;
            if (!h9_soft_clamp(&xdot, &y, p_mo, tols[s])) continue;
            const double cx_new = xdot / R3;
            const uint8_t cid   = classify_band(xdot, y, p_mo);
            for (int k = 0; k < 9; ++k) {
                if (ctab[k] == cid) {
                    *p_cx = cx_new;
                    *p_cy = y;
                    ++(*counters[s]);
                    return k;
                }
            }
        }
    }

    ++h9_recover_stats().failed;
    return -1;
}

/* Pre-amble nudge — Python region.py:678.  If the top-level (ẋ, y) is out
 * of scope for its supercell (geometric drift past a parent edge), apply
 * a single ULP-nudge to both axes before descent.  This is the cheaper
 * pre-conditioning Python does before entering xy_regions' descent loop;
 * the full recovery cascade only fires per-level when classify returns a
 * bad cid (handled in h9_recover_cell). */
static inline void h9_preamble_nudge(double *p_cx, double *p_cy, int oct_mode) {
    const double R3       = std::sqrt(3.0);
    const double LAT_V    = H9_UV_V3 / 3.0;
    const double LAT_UDOT = H9_UV_U1 * R3;
    if (h9_in_scope((*p_cx) * R3, *p_cy, oct_mode)) return;
    *p_cy = h9_ulp_nudge(*p_cy, LAT_V);
    const double xdot_new = h9_ulp_nudge((*p_cx) * R3, LAT_UDOT);
    *p_cx = xdot_new / R3;
    ++h9_recover_stats().preamble;
}

/* Encode (cx, cy) in oid's b_oct frame into a bin UUID at `layer`.
 * Output `out[16]` is the packed UUID.  Matches Python create_clipped's
 * _hex_layer → hex_digits → xy_regions pipeline exactly (no warp, no orient). */
static inline void uuid_from_cxcy(double cen_cx, double cen_cy,
                                  int oid, int layer, uint8_t out[16]) {
    const int    oct_mode = (int)H9_OID_MO[oid];
    const double R3       = std::sqrt(3.0);

    /* No orient transform here — (cen_cx, cen_cy) is already in the
     * post-orient frame xy_regions/classify_band expects.  See header
     * comment above for the diagnostic that established this. */
    double cx = cen_cx;
    double cy = cen_cy;
#if H9_WARP_ENABLE
    /* Python's b_oct.encode descent runs on warp.undo(BRAW), not warp.do.
     * The lattice is at regular positions in BRAW; for the FORWARD
     * direction (lattice → lon/lat) cxcy_to_lonlat applies h9_warp_fwd
     * to project area-uniformly. For the INVERSE direction (lon/lat →
     * cell descent), apply h9_warp_inv so the input lands in the same
     * frame Python's encode pipeline descends. Verified: w.undo(BRAW,
     * mo=0) matches ex0062's BARY used for descent; w.do(BRAW, mo=0)
     * does not. The `hex9.use_warp = off` GUC makes h9_warp_inv
     * identity, so this is a no-op when warp is disabled. */
    {
        double bx, by;
        h9_warp_inv(cx, cy, oct_mode, &bx, &by);
        cx = bx; cy = by;
    }
#endif
    h9_preamble_nudge(&cx, &cy, oct_mode);

    /* L+1 containment descents → cids[1..layer+1]; cids[0] = proto. */
    uint8_t cids[40] = {};
    cids[0] = H9_RID2CELL[oct_mode];

    int p_mo = oct_mode;

    for (int l = 0; l <= layer; l++) {
        const double  xdot  = cx * R3;
        const uint8_t cid   = classify_band(xdot, cy, p_mo);
        const double *offx  = (p_mo == 1) ? H9.UP_X    : H9.DN_X;
        const double *offy  = (p_mo == 1) ? H9.UP_Y    : H9.DN_Y;
        const uint8_t *ctab = (p_mo == 1) ? H9_UP_CIDS : H9_DN_CIDS;
        const int    *mtab  = (p_mo == 1) ? H9_UP_MODE : H9_DN_MODE;

        /* Locate j ∈ 0..8 with ctab[j] == cid.  Trigger matches Python's
         * per-level `bad = ~in_mode[p_mo, cid]` (= cid not in ctab).  On
         * recovery failure, fall back to nearest-neighbour. */
        int j = -1;
        for (int k = 0; k < 9; k++) if (ctab[k] == cid) { j = k; break; }
        if (j < 0) {
            j = h9_recover_cell(&cx, &cy, p_mo, ctab);
            if (j < 0) {
                double best = 1e300; int best_j = 0;
                for (int k = 0; k < 9; k++) {
                    const double dx = cx - offx[k], dy = cy - offy[k];
                    const double d2 = dx*dx + dy*dy;
                    if (d2 < best) { best = d2; best_j = k; }
                }
                j = best_j;
            }
        }
        cids[l+1] = ctab[j];
        cx        = (cx - offx[j]) * 3.0;
        cy        = (cy - offy[j]) * 3.0;
        p_mo      = mtab[j];
    }

    /* CID chain → RID chain (only the first layer+2 entries matter). */
    uint8_t rids[40] = {};
    for (int i = 0; i <= layer + 1; i++) rids[i] = H9_CELL2RID[cids[i]];

    /* Body nibbles 0..layer via L0HEX_BY_ID + REG_HEX. */
    uint8_t nibbles[32];
    const uint8_t c2_l0 = H9_MCC2[oct_mode][cids[1]];
    nibbles[0] = H9_L0HEX_BY_ID[oid][c2_l0];

    uint8_t last_c2 = c2_l0;
    for (int ri = 2; ri <= layer + 1; ri++) {
        const uint8_t gp_mo = rids[ri-2] & 1u;
        const uint8_t p_rid = rids[ri-1];
        const uint8_t c_rid = rids[ri];
        const uint8_t *e    = H9_REG_HEX[gp_mo][p_rid][c_rid];
        nibbles[ri-1] = e[0];
        last_c2       = e[1];
    }

    /* Bin sentinels at layer+1..30, key tail at 31.
     * Tail layout matches Python tail_pack_key high nibble:
     *   bit 3 = 0 (c_mo always 0 for bin UUIDs)
     *   bits 2..1 = c2
     *   bit 0 = r_mo */
    for (int i = layer + 1; i < 31; i++) nibbles[i] = 0x0F;
    nibbles[31] = (uint8_t)((last_c2 << 1) | (uint8_t)oct_mode);

    h9a_pack(nibbles, out);
}

/* Full-depth (30 body nibbles + tail) containment-encoder for h9_encode.
 *
 * Parallels h9_boct_to_uuid in h9_addressing.h but uses the containment
 * classifier (classify_band → lookup in parent's child set) instead of
 * nearest-neighbour descent.  Matches what uuid_from_cxcy does for grid
 * cells — so an h9_encode result and the corresponding h9_grid cell's
 * `hex9` column will agree byte-for-byte, eliminating the boundary
 * divergence between the two paths at the cost of inheriting the same
 * ULP-level fallback behaviour.
 *
 * Current fallback when classify_band returns a cid not in parent's set:
 * nearest-neighbour (the same simple fallback uuid_from_cxcy uses).
 * Phase 2 will replace that with a port of Python region.py::_recover
 * (~150 LOC, removes the residual L13+ collision tail).  Phase 1 here
 * delivers the encoder-dispatch infrastructure so the rest can be A/B'd. */
static inline void uuid_from_cxcy_full(double cen_cx, double cen_cy,
                                        int oid, uint8_t out[16])
{
    const int    oct_mode = (int)H9_OID_MO[oid];
    const double R3       = std::sqrt(3.0);

    double cx = cen_cx;
    double cy = cen_cy;
#if H9_WARP_ENABLE
    /* See uuid_from_cxcy above for rationale — descent runs on
     * warp_inv(BRAW) to match Python b_oct.encode. */
    {
        double bx, by;
        h9_warp_inv(cx, cy, oct_mode, &bx, &by);
        cx = bx; cy = by;
    }
#endif
    h9_preamble_nudge(&cx, &cy, oct_mode);

    uint8_t cids[37] = {};
    cids[0] = H9_RID2CELL[oct_mode];

    int p_mo = oct_mode;
    for (int l = 0; l < 36; ++l) {
        const double  xdot = cx * R3;
        const uint8_t cid  = classify_band(xdot, cy, p_mo);
        const double  *offx = (p_mo == 1) ? H9.UP_X    : H9.DN_X;
        const double  *offy = (p_mo == 1) ? H9.UP_Y    : H9.DN_Y;
        const uint8_t *ctab = (p_mo == 1) ? H9_UP_CIDS : H9_DN_CIDS;
        const int     *mtab = (p_mo == 1) ? H9_UP_MODE : H9_DN_MODE;

        /* Same trigger as uuid_from_cxcy above. */
        int j = -1;
        for (int k = 0; k < 9; k++) if (ctab[k] == cid) { j = k; break; }
        if (j < 0) {
            j = h9_recover_cell(&cx, &cy, p_mo, ctab);
            if (j < 0) {
                double best = 1e300; int best_j = 0;
                for (int k = 0; k < 9; k++) {
                    const double dx = cx - offx[k], dy = cy - offy[k];
                    const double d2 = dx*dx + dy*dy;
                    if (d2 < best) { best = d2; best_j = k; }
                }
                j = best_j;
            }
        }
        cids[l+1] = ctab[j];
        cx        = (cx - offx[j]) * 3.0;
        cy        = (cy - offy[j]) * 3.0;
        p_mo      = mtab[j];
    }

    /* Steps 3-5: CID → RID → body nibbles → tail (same as h9_boct_to_uuid). */
    uint8_t rids[37];
    for (int i = 0; i <= 36; ++i) rids[i] = H9_CELL2RID[cids[i]];

    uint8_t nibbles[32];
    const uint8_t c2_l0 = H9_MCC2[oct_mode][cids[1]];
    nibbles[0] = H9_L0HEX_BY_ID[oid][c2_l0];

    uint8_t last_c2 = 0;
    for (int ri = 2; ri <= 30; ++ri) {
        const uint8_t gp_mo  = rids[ri-2] & 1u;
        const uint8_t p_rid  = rids[ri-1];
        const uint8_t c_rid  = rids[ri];
        const uint8_t *entry = H9_REG_HEX[gp_mo][p_rid][c_rid];
        nibbles[ri-1] = entry[0];
        last_c2       = entry[1];
    }
    nibbles[30] = rids[30];
    const uint8_t p_mo_final = rids[29] & 1u;
    nibbles[31] = (uint8_t)((p_mo_final << 3) | (last_c2 << 1) | (uint8_t)oct_mode);
    h9a_pack(nibbles, out);
}

struct ClipNode { int mode; int64_t ia, ib, scale; };

/* ── Integer descent UUID encoder (WP1) ───────────────────────────────────
 *
 * uuid_from_iauv: build a hex's UUID directly from its lattice identity
 * (oid, c2, ia_org, ib_org, layer, ext) using EXACT integer arithmetic
 * for the descent — no floating point, no boundary fuzz, no _recover.
 *
 * Why: uuid_from_cxcy's per-level (cx, cy) = (cx - offx[j]) * 3.0 rescaling
 * accumulates double-precision drift; at L13+ ~0.13% of cells fall to the
 * nearest-neighbour fallback at an intermediate level, two distinct cells
 * collapse to the same nibble path, and dedup silently drops one (= gap).
 *
 * How: classify_band's thresholds reduce to pure integer comparisons.
 * Using H9 lattice relations:
 *   U1 = sqrt(2)/6,  V3 = sqrt(6)/3,  H = sqrt(6)/2,
 *   h_dot = H/3 = V3,                w_dot = 2·V3,
 *   V3 = 2·U1·sqrt(3)        (key identity — lets xdot drop out).
 *
 * For an integer centroid (ia_c, ib_c) and rescale-level scale = 3^(layer-k):
 *   y       = ib_c · V3 / scale
 *   xdot    = ia_c · U1·sqrt(3) / scale = ia_c · V3 / (2·scale)
 *   y - xdot ∝ (2·ib_c - ia_c)
 *   y + xdot ∝ (2·ib_c + ia_c)
 * So every "y ⋚ h_dot/w_dot" test reduces to an integer comparison against
 * a multiple of `scale`.  The classifier becomes pure-integer.
 *
 * The descent target is the hex's CENTROID, not its (ia_org, ib_org) — the
 * cxcy descent uses the centroid, and we must match that to produce the
 * same UUID for the same cell.  For non-ext cells centroid = supercell
 * origin + H9_HI[mode][c2][6] (template vertex 6, integer offset).  For ext
 * cells centroid = mode-matching mean of v0..v3 — 1/4-integer fractions.
 * We scale everything by 4 to keep the arithmetic exact (centroid_4x,
 * scale_4x = 4·scale, threshold_4x = 4·scale, etc.).
 */

/* Integer classify_band.  Derivation uses the H9 identity U1·√3 = V3, which
 * collapses cxcy's float comparisons:
 *
 *   y      = ib_c · V3 / scale           → integer test: ib_c ⋚ k·scale
 *   xdot   = ia_c · U1·√3 / scale = ia_c · V3 / scale
 *   ymx = y - xdot = (ib_c - ia_c) · V3 / scale  → integer test on (ib_c - ia_c)
 *   ypx = y + xdot = (ib_c + ia_c) · V3 / scale  → integer test on (ib_c + ia_c)
 *
 * Thresholds: h_dot = V3, w_dot = 2·V3.  In integer units relative to scale:
 *   y ⋚ h_dot  → ib_t ⋚ s1
 *   y ⋚ w_dot  → ib_t ⋚ s2  (s2 = 2·s1)
 *   ymx ⋚ w_dot → ymx_t ⋚ s2
 *   ypx ⋚ w_dot → ypx_t ⋚ s2
 *
 * `scale` is the L_l (parent) scale in caller's integer units. */
static inline uint8_t classify_band_int(int64_t cent_u, int64_t cent_v,
                                        int64_t scale, int p_mo) {
    const int64_t ib_t  = cent_v;
    const int64_t ymx_t = cent_v - cent_u;
    const int64_t ypx_t = cent_v + cent_u;
    const int64_t s1 = scale;          /* h_dot threshold */
    const int64_t s2 = 2*scale;        /* w_dot threshold */

    int h_id, p_id, n_id;
    if (p_mo == 0) {
        /* mode 0 (down) — strict on outer bands, inclusive on inner */
        h_id = (ib_t > s2) ? 0 : (ib_t >= s1) ? 1 : (ib_t >= 0) ? 2 :
               (ib_t > -s1) ? 3 : (ib_t > -s2) ? 4 : 5;
        p_id = (ymx_t >= s2) ? 0 : (ymx_t >= 0) ? 1 : (ymx_t >= -s2) ? 2 : 3;
        n_id = (ypx_t < -s2) ? 0 : (ypx_t <= 0) ? 1 : (ypx_t < s2) ? 2 : 3;
    } else {
        /* mode 1 (up) — strict/inclusive swapped on the central tier */
        h_id = (ib_t > s2) ? 0 : (ib_t > s1) ? 1 : (ib_t >= 0) ? 2 :
               (ib_t > -s1) ? 3 : (ib_t > -s2) ? 4 : 5;
        p_id = (ymx_t > s2) ? 0 : (ymx_t > 0) ? 1 : (ymx_t > -s2) ? 2 : 3;
        n_id = (ypx_t <= -s2) ? 0 : (ypx_t <= 0) ? 1 : (ypx_t <= s2) ? 2 : 3;
    }
    return (uint8_t)((h_id << 4) | (p_id << 2) | n_id);
}

/* cids[layer+1] for a hex within a leaf supercell of mode_leaf.
 *
 * The hex's mode-matching centroid offset within its leaf supercell is a
 * fixed constant per (mode_leaf, c2, ext) — same value for all (ia, ib).
 * We classify that offset using the float classifier (input is a known
 * small set of constants; no per-cell drift, no rescaling accumulation),
 * then look up the cid.  The 9 valid child cids of the leaf form the
 * candidate set; on the rare case where classify returns a non-member,
 * NN fallback in absolute (du² + 3·dv²) picks the nearest. */
static inline uint8_t cids_leaf_for_c2(int mode_leaf, int c2, bool ext) {
    /* Mode-matching centroid offset within leaf supercell, in integer-UV.
     * Non-ext: mean of 6 vertices. Ext: mean of 4 (v0..v3 mode-matching).
     * Use H9_HI[0][c2] templates regardless of mode (matches emit_hexes_for_oid).
     * Stored ×4 to absorb the 1/4 fractions from ext's 4-vertex mean. */
    int sum_u = 0, sum_v = 0, n = ext ? 4 : 6;
    for (int v = 0; v < n; v++) {
        sum_u += H9_HI[0][c2][v][0];
        sum_v += H9_HI[0][c2][v][1];
    }
    /* (cx, cy) = (sum_u/n × U1, sum_v/n × V3) absolute — the centroid in
     * the leaf supercell's frame at scale 1.  classify_band's thresholds
     * are at fractions of (U1, V3) so we can compare in float without
     * accumulated descent drift. */
    const double cx = (double)sum_u * H9_UV_U1 / (double)n;
    const double cy = (double)sum_v * H9_UV_V3 / (double)n;
    const double xdot = cx * std::sqrt(3.0);
    const uint8_t cid = classify_band(xdot, cy, mode_leaf);
    const uint8_t *ctab = (mode_leaf == 1) ? H9_UP_CIDS : H9_DN_CIDS;
    for (int k = 0; k < 9; k++) if (ctab[k] == cid) return ctab[k];
    /* NN fallback in absolute (cx, cy) — same as cxcy. */
    const double *offx = (mode_leaf == 1) ? H9.UP_X : H9.DN_X;
    const double *offy = (mode_leaf == 1) ? H9.UP_Y : H9.DN_Y;
    double best_d = 1e300; int best_k = 0;
    for (int k = 0; k < 9; k++) {
        const double dx = cx - offx[k], dy = cy - offy[k];
        const double d2 = dx*dx + dy*dy;
        if (d2 < best_d) { best_d = d2; best_k = k; }
    }
    return ctab[best_k];
}

/* Integer descent on the cell ORIGIN (ia_org, ib_org) — unique per cell at
 * every layer by construction.  Fills cids[1..layer] (the supercell hierarchy
 * path).  cids[layer+1] is the c2-distinguishing nibble — derived separately
 * by cids_leaf_for_c2 and set by uuid_from_iauv after this call.
 *
 * Why origin descent and not centroid descent:
 *   The hex centroid is the supercell origin plus a c2-specific offset
 *   (template vertex 6).  cxcy and Python's hex_layer descend on the
 *   centroid, which works for most cells but breaks at L13+ for ~0.13% of
 *   cells whose centroids land on band intersections that classify to a
 *   "bad" cid not in the legal child set — the NN fallback then picks
 *   the same child for two physically distinct cells, collapsing their
 *   onward paths and producing UUID collisions (gaps in the grid).
 *   Verified at L13 pair A=(2056794, -1131670), B=(2056740, -1131652)
 *   under cxcy AND under integer-on-centroid (both collide).
 *   The supercell origin (ia, ib) is exact, unique per cell, and integer-
 *   aligned to the lattice — cannot collide.  Trade-off: at cells where
 *   centroid descent and origin descent disagree on which supercell
 *   contains the cell, our UUID differs from Python's.  For hexbinning,
 *   what matters is that the encoder and the grid use the SAME convention
 *   (both call this function) so a binned point lands in the same UUID
 *   the grid lists.  c2 is independent of (ia, ib), so cells differing
 *   only in c2 share cids[1..layer] and differ only at cids[layer+1] —
 *   which is the correct hierarchical structure. */
static inline void cids_from_iauv(int oid, int c2, int64_t ia_org, int64_t ib_org,
                                  int layer, bool ext, uint8_t cids[40]) {
    const int oct_mode = (int)H9_OID_MO[oid];
    cids[0] = H9_RID2CELL[oct_mode];

    /* scale_l at level l = 3^(layer-l) — the L_l supercell extent in
     * L_layer integer units.  Starts at 3^layer for l=0 (root). */
    int64_t scale = 1;
    for (int i = 0; i < layer; i++) scale *= 3;

    int     p_mo = oct_mode;
    int64_t orig_u = 0, orig_v = 0;

    /* `layer` descent steps fill cids[1..layer].  No descent for layer=0
     * (the L0 cell IS the proto's child hex — distinguished by c2 only). */
    for (int k = 0; k < layer; k++) {
        const int (*ofs)[2] = (p_mo == 1) ? H9_MODE1_OFS         : H9_MODE0_OFS;
        const int *cm       = (p_mo == 1) ? H9_MODE1_CHILD_MODE  : H9_MODE0_CHILD_MODE;
        const uint8_t *ctab = (p_mo == 1) ? H9_UP_CIDS            : H9_DN_CIDS;

        const int64_t resu = ia_org - orig_u;
        const int64_t resv = ib_org - orig_v;
        const uint8_t cid  = classify_band_int(resu, resv, scale, p_mo);

        /* Membership-or-NN.  When classify returns a cid in the legal child
         * set, the choice is exact.  NN fallback uses du² + 3·dv² (= |Δ|² in
         * absolute b_oct space, since V3 = √3·U1).  Distinct (ia, ib) cells
         * yield distinct residuals → distinct NN winners → distinct paths. */
        const int64_t child_scale = scale / 3;
        int j = -1;
        for (int kk = 0; kk < 9; kk++) if (ctab[kk] == cid) { j = kk; break; }
        if (j < 0) {
            int64_t best_d = INT64_MAX;
            int     best_j = 0;
            for (int kk = 0; kk < 9; kk++) {
                const int64_t du = resu - (int64_t)ofs[kk][0] * child_scale;
                const int64_t dv = resv - (int64_t)ofs[kk][1] * child_scale;
                const int64_t d2 = du*du + 3 * dv*dv;
                if (d2 < best_d) { best_d = d2; best_j = kk; }
            }
            j = best_j;
        }
        cids[k+1] = ctab[j];
        orig_u += (int64_t)ofs[j][0] * child_scale;
        orig_v += (int64_t)ofs[j][1] * child_scale;
        p_mo = cm[j];
        scale = child_scale;
    }

    /* cids[layer+1]: c2-distinguishing nibble at the leaf.  Same value for
     * all (ia, ib) sharing the same (mode_leaf, c2, ext). */
    cids[layer + 1] = cids_leaf_for_c2(p_mo, c2, ext);
}

/* Build a hex's UUID from its lattice identity using exact integer descent. */
static inline void uuid_from_iauv(int oid, int c2, int64_t ia_org, int64_t ib_org,
                                  int layer, bool ext, uint8_t out[16]) {
    const int oct_mode = (int)H9_OID_MO[oid];

    uint8_t cids[40] = {};
    cids_from_iauv(oid, c2, ia_org, ib_org, layer, ext, cids);

    /* Nibble assembly — identical to uuid_from_cxcy. */
    uint8_t rids[40] = {};
    for (int i = 0; i <= layer + 1; i++) rids[i] = H9_CELL2RID[cids[i]];

    uint8_t nibbles[32];
    const uint8_t c2_l0 = H9_MCC2[oct_mode][cids[1]];
    nibbles[0] = H9_L0HEX_BY_ID[oid][c2_l0];

    uint8_t last_c2 = c2_l0;
    for (int ri = 2; ri <= layer + 1; ri++) {
        const uint8_t gp_mo = rids[ri-2] & 1u;
        const uint8_t p_rid = rids[ri-1];
        const uint8_t c_rid = rids[ri];
        const uint8_t *e    = H9_REG_HEX[gp_mo][p_rid][c_rid];
        nibbles[ri-1] = e[0];
        last_c2       = e[1];
    }

    for (int i = layer + 1; i < 31; i++) nibbles[i] = 0x0F;
    nibbles[31] = (uint8_t)((last_c2 << 1) | (uint8_t)oct_mode);

    h9a_pack(nibbles, out);
}

/* Conservative footprint of a supercell node: bbox of the 18 projected outer
 * vertices (3 c2 hexes × 6 corners).  Matches Python _nodes_overlap
 * (grid.py:965), which slices template[:, :6, :] — the centroid template
 * (index 6) is NOT included.  Including it makes the test more permissive:
 * for supercells whose origin sits past an octant seam, the 18 outer-vertex
 * projections may all fail strict in-octant projection (NaN/false), while
 * the centroid template at a small offset like (1,1)/(1,-1)/(-2,0) may
 * happen to pass strict and produce a misleading lat/lon inside the bbox,
 * keeping a supercell whose footprint actually belongs to the neighbour
 * octant's enumeration.  See grid_face_vertex_oid_bug.md. */
static inline bool node_overlaps_bbox(const ClipNode &nd, int oid, double div_f,
                                      double lon_min, double lat_min,
                                      double lon_max, double lat_max,
                                      double pad) {
    double vlon_min = 1e30, vlon_max = -1e30;
    double vlat_min = 1e30, vlat_max = -1e30;
    bool   has_pt = false;
    for (int c2 = 0; c2 < H9_N_C2; c2++) {
        for (int v = 0; v < 6; v++) {
            const int64_t pu = (int64_t)H9_HI[nd.mode][c2][v][0] * nd.scale + nd.ia;
            const int64_t pv = (int64_t)H9_HI[nd.mode][c2][v][1] * nd.scale + nd.ib;
            double vlon, vlat;
            if (!uv_to_lonlat(pu, pv, oid, div_f, &vlon, &vlat)) continue;
            has_pt = true;
            if (vlon < vlon_min) vlon_min = vlon;
            if (vlon > vlon_max) vlon_max = vlon;
            if (vlat < vlat_min) vlat_min = vlat;
            if (vlat > vlat_max) vlat_max = vlat;
        }
    }
    /* No valid vertex projection means the entire supercell lies outside
     * this octant's region — prune it.  (Matches Python: a node whose 18
     * vertices all NaN under reg.project produces nanmin/max == NaN, and
     * the bbox-overlap test then evaluates False.) */
    if (!has_pt) return false;
    return (vlon_min <= lon_max + pad && vlon_max >= lon_min - pad &&
            vlat_min <= lat_max + pad && vlat_max >= lat_min - pad);
}

/* Emit the 3 c2 hexes for each mode-0 leaf supercell in `q_cur` under `oid`.
 * Pulled out of `enumerate()` so the global L0/L1 path (no BFS, no bbox
 * filter) can share the exact same emit logic — see `enumerate_global()`.
 *
 * If `apply_bbox_filter` is true, cells whose centroid lon/lat falls outside
 * [lon_min,lon_max] × [lat_min,lat_max] are skipped (used by `enumerate()`).
 * For the unbounded global path, pass `apply_bbox_filter = false`.
 */
static inline void emit_hexes_for_oid(int oid, int layer, double div_f,
                                      const std::vector<ClipNode> &q_cur,
                                      bool apply_bbox_filter,
                                      double lon_min, double lat_min,
                                      double lon_max, double lat_max,
                                      std::vector<H9GridCell> &out)
{
    const int64_t s_int     = (int64_t)std::llround(div_f);   /* 3^L */
    const int     face_mode = (int)H9_OID_MO[oid];

    for (size_t qi = 0; qi < q_cur.size(); qi++) {
        if (q_cur[qi].mode != 0) continue;
        const int64_t ia_org = q_cur[qi].ia;
        const int64_t ib_org = q_cur[qi].ib;
        for (int c2 = 0; c2 < H9_N_C2; c2++) {
            /* Outer-ring integer-UV in the *original* octant frame. */
            int64_t uvo[6][2];
            for (int v = 0; v < 6; v++) {
                uvo[v][0] = (int64_t)H9_HI[0][c2][v][0] + ia_org;
                uvo[v][1] = (int64_t)H9_HI[0][c2][v][1] + ib_org;
            }

            /* Seam test on v4 and v5 (both must be outside the octant).
             * Mode-0 octants own the seam reflection; mode-1 octants leave
             * their v4/v5 in place and share vertices via the pooled dedup.
             * Mirrors Python _assemble_clipped (grid.py:1042-1055). */
            auto outside_oct = [s_int](int64_t u, int64_t v) -> bool {
                return (v > s_int) || (u - v > 2 * s_int) || (u + v < -2 * s_int);
            };
            const bool ext = (H9_OID_MO[oid] == 0) &&
                             outside_oct(uvo[4][0], uvo[4][1]) &&
                             outside_oct(uvo[5][0], uvo[5][1]);

            /* Per-vertex (pu, pv, project_oid). v4/v5 reflect to neighbour
             * octant when `ext`; otherwise they stay in the original. */
            int64_t pu[6], pv[6];
            int     poid[6];
            for (int v = 0; v < 6; v++) {
                pu[v]   = uvo[v][0];
                pv[v]   = uvo[v][1];
                poid[v] = oid;
            }
            int nb_oid = oid;
            if (ext) {
                nb_oid = H9_OID_NB[oid][c2];
                /* v4 ← reflect(v2);  v5 ← reflect(v1)  across v = 0. */
                pu[4] = uvo[2][0];  pv[4] = -uvo[2][1];  poid[4] = nb_oid;
                pu[5] = uvo[1][0];  pv[5] = -uvo[1][1];  poid[5] = nb_oid;
            }

            /* Per-vertex (cx, cy) in each vertex's own b_oct frame. */
            double cxv[6], cyv[6];
            for (int v = 0; v < 6; v++) {
                cxv[v] = (double)pu[v] * H9_UV_U1 / div_f;
                cyv[v] = (double)pv[v] * H9_UV_V3 / div_f;
            }

            /* Mode-matching centroid: only vertices whose oid_mo matches
             * face_mode contribute (their (cx,cy) live in a compatible
             * frame; reflected v4/v5 are excluded since the neighbour
             * octant always has the opposite oid_mo). */
            double sum_cx = 0.0, sum_cy = 0.0;
            int    n_match = 0;
            for (int v = 0; v < 6; v++) {
                if ((int)H9_OID_MO[poid[v]] != face_mode) continue;
                sum_cx += cxv[v]; sum_cy += cyv[v]; n_match++;
            }
            if (n_match == 0) continue;   /* shouldn't happen */
            const double cen_cx = sum_cx / (double)n_match;
            const double cen_cy = sum_cy / (double)n_match;

            /* Centroid lon/lat (face_oid frame) for bbox + UUID. */
            double clon, clat;
            if (!cxcy_to_lonlat(cen_cx, cen_cy, oid, &clon, &clat, nullptr)) continue;

            if (apply_bbox_filter) {
                if (clon < lon_min || clon > lon_max ||
                    clat < lat_min || clat > lat_max) continue;
            }

            /* Project each vertex from its own oid frame.
             *
             * Mirrors Python _assemble_clipped: trust the reflection
             * arithmetic outright.  An earlier "closer-to-centroid"
             * robustness check used naive Δlon²+Δlat² distance which
             * fails near the antimeridian and was systematically
             * picking the unreflected (geographically wrong) vertex
             * for L0/L1 cells (verified by primitive test against
             * Python reg.project — same (cx, cy, oid) inputs agree
             * to floating-point precision). */
            double hlon[6], hlat[6];
            bool ok = true;
            for (int v = 0; v < 6; v++) {
                if (!cxcy_to_lonlat(cxv[v], cyv[v], poid[v], &hlon[v], &hlat[v], nullptr, false)) {
                    ok = false; break;
                }
            }
            if (!ok) continue;

            /* Encode via integer-descent on cell origin.  Exact and unique
             * per (oid, c2, ia, ib, ext) — no L13+ collisions.  See the
             * cids_from_iauv header for why we use origin descent and not
             * centroid descent (which inherits cxcy's collision bug). */
            uint8_t bin_uuid[16];
            uuid_from_iauv(oid, c2, ia_org, ib_org, layer, ext, bin_uuid);
            (void)cen_cx; (void)cen_cy;   /* still computed for centroid filter */

            H9GridCell cell;
            std::memcpy(cell.uuid, bin_uuid, 16);
            cell.oid     = oid;
            cell.c2      = c2;
            cell.ia      = ia_org;
            cell.ib      = ib_org;
            cell.ext     = ext;
            cell.cen_lon = clon;
            cell.cen_lat = clat;
            for (int v = 0; v < 6; v++) {
                cell.poid[v] = poid[v];
                cell.pu[v]   = pu[v];
                cell.pv[v]   = pv[v];
                cell.vlon[v] = hlon[v];
                cell.vlat[v] = hlat[v];
            }
            out.push_back(cell);
        }
    }
}

/* Final pass: sort by UUID and drop duplicates (keeps first occurrence).
 * Shared by all enumeration paths.
 *
 * NOTE: a non-zero dedup-drop count means uuid_from_cxcy assigned the same
 * UUID to two physically distinct cells — a containment-classifier bug,
 * not a benign duplicate.  We log it to stderr and dump the first few
 * colliding pairs so the failure mode is visible without needing a
 * separate diagnostic build. */
static inline void sort_and_dedup_by_uuid(std::vector<H9GridCell> &out) {
    std::sort(out.begin(), out.end(),
              [](const H9GridCell &a, const H9GridCell &b) -> bool {
                  return std::memcmp(a.uuid, b.uuid, 16) < 0;
              });
    const size_t before = out.size();
    size_t dropped = 0;
    size_t shown = 0;
    /* First scan only counts and prints collisions; we use a separate
     * write index w to compact in-place afterwards. */
    for (size_t r = 1; r < out.size(); r++) {
        if (std::memcmp(out[r-1].uuid, out[r].uuid, 16) == 0) {
            dropped++;
            if (shown < 5) {
                char ub[37] = {};
                snprintf(ub, 37,
                    "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                    out[r].uuid[0],  out[r].uuid[1],  out[r].uuid[2],  out[r].uuid[3],
                    out[r].uuid[4],  out[r].uuid[5],  out[r].uuid[6],  out[r].uuid[7],
                    out[r].uuid[8],  out[r].uuid[9],  out[r].uuid[10], out[r].uuid[11],
                    out[r].uuid[12], out[r].uuid[13], out[r].uuid[14], out[r].uuid[15]);
                std::fprintf(stderr,
                    "  collision %s: A oid=%d c2=%d (ia=%lld ib=%lld) cen=(%.6f,%.6f)"
                    " | B oid=%d c2=%d (ia=%lld ib=%lld) cen=(%.6f,%.6f)\n",
                    ub,
                    out[r-1].oid, out[r-1].c2, (long long)out[r-1].ia, (long long)out[r-1].ib,
                    out[r-1].cen_lon, out[r-1].cen_lat,
                    out[r].oid,   out[r].c2,   (long long)out[r].ia,   (long long)out[r].ib,
                    out[r].cen_lon,   out[r].cen_lat);
                shown++;
            }
        }
    }
    /* In-place compaction */
    size_t w = 0;
    for (size_t r = 0; r < out.size(); r++) {
        if (w == 0 || std::memcmp(out[w-1].uuid, out[r].uuid, 16) != 0) {
            if (w != r) out[w] = out[r];
            w++;
        }
    }
    out.resize(w);
    if (dropped) {
        std::fprintf(stderr, "sort_and_dedup_by_uuid: dropped %zu duplicate UUIDs "
                             "(emitted %zu, kept %zu)\n", dropped, before, w);
    }
}

/* Enumerate H9 cells at `layer` whose CENTROID lies inside the geographic bbox.
 * Output is sorted by uuid and unique. */
static inline void enumerate(int layer,
                             double lon_min, double lat_min,
                             double lon_max, double lat_max,
                             std::vector<H9GridCell> &out) {
    out.clear();
    if (layer < 1 || layer > 29) return;

    const double  div_f  = std::pow(3.0, (double)layer);
    const int     levels = layer - 1;
    int64_t       scale0 = 1;
    for (int i = 0; i < levels; i++) scale0 *= 3;
    /* Pad for node_overlaps_bbox: lets cells whose projected footprint sits
     * a hair outside the bbox still survive pruning (handles numerical fuzz
     * at the bbox boundary).  ORIGINAL: fixed 0.002° — works for typical
     * polygon-sized bboxes but is catastrophic for micro-bboxes — at L≥16
     * with a 3e-7° bbox the pad is 7 orders of magnitude larger than the
     * bbox, every supercell within ±200m "overlaps", and the BFS explodes
     * 9× per level (observed: depth 9→14 grows 10→240640).  FIX: scale pad
     * to 10% of the smaller bbox dimension, with a tiny floor to handle
     * point-bboxes. */
    const double  bbox_span = std::min(lon_max - lon_min, lat_max - lat_min);
    const double  pad       = std::max(1e-12, 0.1 * bbox_span);

    std::vector<ClipNode> q_cur, q_nxt;
    q_cur.reserve(4096);
    q_nxt.reserve(4096);

    for (int oid = 0; oid < 8; oid++) {
        const int oid_mode      = (int)H9_OID_MO[oid];
        const int (*ofs_init)[2] = (oid_mode == 0) ? H9_MODE0_OFS         : H9_MODE1_OFS;
        const int *cm_init       = (oid_mode == 0) ? H9_MODE0_CHILD_MODE  : H9_MODE1_CHILD_MODE;

        q_cur.clear();
        for (int k = 0; k < H9_N_CHILDREN; k++) {
            q_cur.push_back({ cm_init[k],
                              (int64_t)ofs_init[k][0] * scale0,
                              (int64_t)ofs_init[k][1] * scale0,
                              scale0 });
        }

        for (int depth = 0; depth <= levels; depth++) {
            /* prune by bbox footprint */
            size_t w = 0;
            for (size_t qi = 0; qi < q_cur.size(); qi++) {
                if (node_overlaps_bbox(q_cur[qi], oid, div_f,
                                       lon_min, lat_min, lon_max, lat_max, pad))
                    q_cur[w++] = q_cur[qi];
            }
            q_cur.resize(w);

            if (depth == levels || q_cur.empty()) break;

            /* expand into 9 children */
            q_nxt.clear();
            for (size_t qi = 0; qi < q_cur.size(); qi++) {
                const ClipNode  &nd = q_cur[qi];
                const int64_t    ks = nd.scale / 3;
                const int (*ofs_nd)[2] = (nd.mode == 0) ? H9_MODE0_OFS        : H9_MODE1_OFS;
                const int *cm_nd       = (nd.mode == 0) ? H9_MODE0_CHILD_MODE : H9_MODE1_CHILD_MODE;
                for (int k = 0; k < H9_N_CHILDREN; k++) {
                    q_nxt.push_back({ cm_nd[k],
                                      nd.ia + (int64_t)ofs_nd[k][0] * ks,
                                      nd.ib + (int64_t)ofs_nd[k][1] * ks,
                                      ks });
                }
            }
            q_cur.swap(q_nxt);
        }

        emit_hexes_for_oid(oid, layer, div_f, q_cur, /*apply_bbox_filter=*/true,
                           lon_min, lat_min, lon_max, lat_max, out);
    } /* oid loop */

    sort_and_dedup_by_uuid(out);
}

/* Enumerate the full *unbounded* global H9 mesh at `layer`.
 *
 * L=0  → 12 cells   (3 per mode-0 root supercell × 4 mode-0 oids)
 * L=1  → 108 cells
 * L=N  → 12·9^N cells
 *
 * No bbox pruning, no centroid filter — every hex on the globe is emitted.
 * Used by the L0/L1 foundation tests against Python HexMesh.create(). */
static inline void enumerate_global(int layer, std::vector<H9GridCell> &out) {
    out.clear();
    if (layer < 0 || layer > 29) return;

    const double div_f = std::pow(3.0, (double)layer);

    /* L=0: special case — the supercell IS the root.  Skip the BFS entirely
     * and emit hexes for each mode-0 octant's root supercell at (ia=0, ib=0,
     * scale=1).  Mode-1 octants share their hexes with neighbouring mode-0
     * octants (via the v4/v5 reflection inside emit_hexes_for_oid). */
    if (layer == 0) {
        std::vector<ClipNode> root = { { 0, 0, 0, 1 } };
        for (int oid = 0; oid < 8; oid++) {
            if ((int)H9_OID_MO[oid] != 0) continue;
            emit_hexes_for_oid(oid, layer, div_f, root, /*apply_bbox_filter=*/false,
                               0, 0, 0, 0, out);
        }
        sort_and_dedup_by_uuid(out);
        return;
    }

    /* L≥1: build the full BFS queue (no pruning), then emit. */
    const int     levels = layer - 1;
    int64_t       scale0 = 1;
    for (int i = 0; i < levels; i++) scale0 *= 3;

    std::vector<ClipNode> q_cur, q_nxt;
    q_cur.reserve(4096);
    q_nxt.reserve(4096);

    for (int oid = 0; oid < 8; oid++) {
        const int oid_mode       = (int)H9_OID_MO[oid];
        const int (*ofs_init)[2] = (oid_mode == 0) ? H9_MODE0_OFS         : H9_MODE1_OFS;
        const int *cm_init       = (oid_mode == 0) ? H9_MODE0_CHILD_MODE  : H9_MODE1_CHILD_MODE;

        q_cur.clear();
        for (int k = 0; k < H9_N_CHILDREN; k++) {
            q_cur.push_back({ cm_init[k],
                              (int64_t)ofs_init[k][0] * scale0,
                              (int64_t)ofs_init[k][1] * scale0,
                              scale0 });
        }

        for (int depth = 0; depth < levels; depth++) {
            q_nxt.clear();
            for (size_t qi = 0; qi < q_cur.size(); qi++) {
                const ClipNode  &nd = q_cur[qi];
                const int64_t    ks = nd.scale / 3;
                const int (*ofs_nd)[2] = (nd.mode == 0) ? H9_MODE0_OFS        : H9_MODE1_OFS;
                const int *cm_nd       = (nd.mode == 0) ? H9_MODE0_CHILD_MODE : H9_MODE1_CHILD_MODE;
                for (int k = 0; k < H9_N_CHILDREN; k++) {
                    q_nxt.push_back({ cm_nd[k],
                                      nd.ia + (int64_t)ofs_nd[k][0] * ks,
                                      nd.ib + (int64_t)ofs_nd[k][1] * ks,
                                      ks });
                }
            }
            q_cur.swap(q_nxt);
        }

        emit_hexes_for_oid(oid, layer, div_f, q_cur, /*apply_bbox_filter=*/false,
                           0, 0, 0, 0, out);
    } /* oid loop */

    sort_and_dedup_by_uuid(out);
}

} /* namespace h9grid */
