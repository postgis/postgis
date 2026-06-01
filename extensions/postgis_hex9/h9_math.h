/*
 * h9_math.h — Self-contained Hex9 math for the postgis_hex9 extension.
 *
 * Part of the Hex9 (H9) Project
 * Copyright ©2025, Ben Griffin
 * Licensed under the Apache License, Version 2.0
 *
 * Ported from h9_boct.cpp (PROJ plugin) and h9_proj/test_h9_standalone.cpp.
 * No external dependencies — pure C++ math, no PROJ, no PostgreSQL.
 *
 * Requires h9_warp_data.h in the same directory (copy from h9_proj/).
 *
 * Public interface
 * ────────────────
 *   H9BOct                   — signed normalised barycentric b_oct point
 *   h9_lonlat_to_boct()      — (lon,lat) radians → H9BOct  [beam search]
 *   h9_boct_to_lonlat()      — H9BOct → (lon,lat) radians  [AK inverse]
 *   h9_lonlatdeg_to_boct()   — degree-input convenience
 *   h9_boct_to_lonlatdeg()   — degree-output convenience
 *   h9_braw_from_boct()      — extract unoriented (fx,fy) b_raw from H9BOct
 *   h9_orient_fwd()          — CCW orient rotation per octant
 *   h9_orient_inv()          — inverse (CW) orient rotation
 *   h9_warp_fwd()            — authalic forward warp (for addressing layer)
 *   h9_warp_inv()            — authalic inverse warp  (Newton-Raphson)
 *
 * Addressing layer interface (implement in h9_addressing.h)
 * ──────────────────────────────────────────────────────────
 *   void   h9_boct_to_uuid(H9BOct, uint8_t uuid[16]);
 *   H9BOct h9_uuid_to_boct(const uint8_t uuid[16]);
 *   void   h9_bin_uuid(const uint8_t in[16], int layer, uint8_t out[16]);
 *
 * UUID layout (32 nibbles = 128 bits) — see h9_addressing.h for full spec.
 *   nibbles  0..29 : body L0..L29  (hierarchy path; valid 0..8 at L1+)
 *   nibble      30 : h_term — terminal RID (0..11)
 *   nibble      31 : key_tail = (p_mo<<3)|(p_c2<<1)|r_mo
 */

#pragma once
#include <cstdint>
#include <limits>
#include <math.h>
#include <stdint.h>

/* Set to 1 to enable authalic warp (requires the heavy CT mesh headers).
 * Set to 0 during binning / hexgrid work until warp retraining is complete. */
#ifndef H9_WARP_ENABLE
#define H9_WARP_ENABLE 1
#endif

/* Runtime warp:
 *   - load + Bell-Sibson gradient + Alfeld-CT coefficients are built at
 *     extension load from the .h9warp sidecar (embedded or on disk).
 *   - h9_warp_fwd / h9_warp_inv shims live in h9_warp_runtime.h.
 *   - The three former static-data headers (h9_wgs84_warp_data.h /
 *     h9_wgs84_warp_grads.h / h9_wgs84_ct_mesh.h) are gone — see
 *     [[warp-distribution-plan]] for the 620 MB → 2 MB reduction. */

/* ── WGS84 ellipsoid ─────────────────────────────────────────────────────── */
static const double H9_WGS84_A  = 6378137.0;
static const double H9_WGS84_B  = 6356752.3142451793;
static const double H9_WGS84_F  = 0.0033528106647474805;
static const double H9_WGS84_A2 = H9_WGS84_A * H9_WGS84_A;
static const double H9_WGS84_B2 = H9_WGS84_B * H9_WGS84_B;
static const double H9_WGS84_E2 = (2.0 * H9_WGS84_F) - (H9_WGS84_F * H9_WGS84_F);
static const double H9_NORM_B2  = (H9_WGS84_B / H9_WGS84_A) * (H9_WGS84_B / H9_WGS84_A);

/* ── H9_Constants ────────────────────────────────────────────────────────── */
static const double H9_R3 = sqrt(3.0);
static const double H9_W  = sqrt(2.0);
static const double H9_H  = H9_W * H9_R3 * 0.5;
static const double H9_1H_3 = H9_H / 3.0;     // Ḣ Vertical spacing between stacked child cells
static const double H9_2H_3 = H9_1H_3 * 2.0;  // alias of ΛC; dH+dW = H9_H
static const double H9_TR  = H9_W / 2.0;   // Triangle Limit Right.
static const double H9_TL  = -H9_TR;       // Triangle Limit Left.
static const double H9_C1  = H9_2H_3;      // ΛC Mode 1 Ceiling
static const double H9_F1  = -H9_1H_3;     // ΛF Mode 1 Floor
static const double H9_C0  = H9_1H_3;      // VC Mode 0 Ceiling
static const double H9_F0  = -H9_2H_3;     // VF Mode 0 Floor
static const double H9_LU  = H9_W/6;       // U Lattice Unit
static const double H9_LV  = H9_H/9;       // V Lattice Unit

/* ── AK formula constants ────────────────────────────────────────────────── */

static const double H9_ALPHA    = 3.227806237143884260376580;
static const double H9_EPS      = 1e-200;
static const double H9_VERT_EPS = 1e-15;


/* ── Orient rotation per octant (CCW, units of π/3) ─────────────────────── */
/*   Indexed by C++ oct_i = H9BOct.oct_i ^ 7                                  */
/*   C++ oct_i = ((eX≥0)?4:0)|((eY≥0)?2:0)|((eZ≥0)?1:0)                    */
/*   SWP NWP SEP NEP SWA NWA SEA NEA                                          */

static const int H9_ORIENT_TH[8] = {5, 2, 2, 5, 2, 5, 5, 2};

/* ── Child net_mode tables per parent mode ───────────────────────────────── */

static const int H9_UP_MODE[9] = {1, 1, 0, 1, 1, 0, 1, 0, 1};
static const int H9_DN_MODE[9] = {0, 1, 0, 1, 0, 0, 1, 0, 0};

/* ── H9 lattice: child cell offsets in b_raw metric barycentric space ─────── */
/*                                                                             */
/*   All values derived from sqrt(2)/sqrt(3) to preserve ULP consistency       */
/*   with Python's h9_constants() chain.                                       */
/*                                                                             */
/*   W = sqrt(2),  r3 = sqrt(3)                                                */
/*   H = W * r3 / 2    (supercell height)                                      */
/*   U = W / 6         (H9K.U — horizontal lattice step)                       */
/*   V = H / 9         (H9K.V — vertical lattice step)                         */
/*                                                                             */
/*   Up   (mode=1) URIs: [0x16,0x25,0x26,0x2a,0x34,0x35,0x39,0x3a,0x3e]        */
/*   Down (mode=0) URIs: [0x21,0x25,0x26,0x2a,0x2b,0x35,0x39,0x3a,0x49]        */

struct H9Tables {
    double H, W, inv_H, inv_W, inv_2H;
    double UP_X[9], UP_Y[9];
    double DN_X[9], DN_Y[9];

    H9Tables() {
        W = sqrt(2.0);
        H = W * sqrt(3.0) * 0.5;
        const double U  = W / 6.0;
        const double V  = H / 9.0;
        const double U2 = 2.0*U, V2 = 2.0*V, V4 = 4.0*V;
        inv_H  = 1.0 / H;
        inv_W  = 1.0 / W;
        inv_2H = 0.5 / H;
        /* up children */
        const double ux[] = {  0, -U,   0, +U, -U2, -U,   0, +U,  U2};
        const double uy[] = { V4, +V, +V2, +V, -V2, -V, -V2, -V, -V2};
        /* down children */
        const double dx[] = {-U2, -U,   0, +U,  U2, -U,   0, +U,   0};
        const double dy[] = { V2, +V, +V2, +V, +V2, -V, -V2, -V, -V4};
        for (int i = 0; i < 9; ++i) {
            UP_X[i] = ux[i]; UP_Y[i] = uy[i];
            DN_X[i] = dx[i]; DN_Y[i] = dy[i];
        }
    }
};

static const H9Tables H9;

/* ── b_oct point ─────────────────────────────────────────────────────────── */
/*                                                                             */
/*   (u, v, w) — signed normalised barycentric coordinates.                   */
/*     u maps to ECEF X, v to Y, w to Z (via h9_c_oct_to_c_ell).             */
/*     Absolute values sum to ≈ 1 (each ≈ 1/3 at the cell centre).            */
/*                                                                             */
/*   oct_i    — Python oid convention (matches Python H9O.oid):               */
/*              ((eZ<0)?4:0)|((eY<0)?2:0)|((eX<0)?1:0)                       */
/*              bit0=eX<0, bit1=eY<0, bit2=eZ<0 (1 means negative)           */
/*              Extract signs: su=(oct_i&1)?-1:+1, sv=(oct_i&2)?-1:+1,       */
/*                             sw=(oct_i&4)?-1:+1                             */
/*   oct_mode — 0=down (even # negative coords), 1=up (odd # negative)       */
/*              Parity of oct_i bits: mode(oct_i) = parity(oct_i) & 1        */
/*                                                                             */
/*   NOTE: H9BOct stores pre-warp, pre-orient b_raw coordinates expressed     */
/*   as signed barycentric. Python's 'boct' is the post-orient/post-warp      */
/*   form. Apply h9_braw_from_boct → h9_orient_fwd → h9_warp_fwd to get      */
/*   the warped (authalic) coordinates used by the addressing layer.          */

struct H9BOct {
    double u, v, w;
    int    oct_i;
    int    oct_mode;
};

/* ── OID/MODE ────────────────────────────────────────────────────────────── */
/*                                                                             */
/*   oid(eX, eY, eZ) returns the Python oid convention used throughout H9:    */
/*     bit2 = (eZ<0), bit1 = (eY<0), bit0 = (eX<0)   — 1 means negative      */
/*   This matches Python H9O.oid and is stored directly in H9BOct.oct_i.      */
/*                                                                             */
/*   mode(oid) returns parity of oid bits:                                    */
/*     0 = even number of negative axes = down (su·sv·sw > 0)                 */
/*     1 = odd  number of negative axes = up   (su·sv·sw < 0)                 */

template <typename T> uint8_t oid(T x, T y, T z) {
	const T tiny = std::numeric_limits<T>::min();
	if (x == T(0.0)) x = tiny;
	if (y == T(0.0)) y = tiny;
	if (z == T(0.0)) z = tiny;
	return static_cast<uint8_t>(
		((z < T(0.0)) << 2) |
		((y < T(0.0)) << 1) |
		 (x < T(0.0))
	);
}

static uint8_t mode(uint8_t oid) {
	return (oid ^ (oid >> 1) ^ (oid >> 2)) & 1;
}

/* ── AK core: normalised barycentric → un-normalised ECEF ─────────────────── */

static void h9_ak_core(double u, double v, double w,
                        double *x, double *y, double *z) {
    const double a  = H9_ALPHA;
    const double e  = H9_EPS;
    const double tu = tan((M_PI * u + e) * 0.5);
    const double tv = tan((M_PI * v + e) * 0.5);
    const double tw = tan((M_PI * w + e) * 0.5);
    const double u2 = tu*tu, v2 = tv*tv, w2 = tw*tw;
    *x = tu * pow(v2 + w2 + a * w2 * v2, 0.25);
    *y = tv * pow(u2 + w2 + a * u2 * w2, 0.25);
    *z = tw * pow(u2 + v2 + a * u2 * v2, 0.25);
}

/* Normalise raw XYZ to unit-scale ellipsoid (a=1, b=B/A). */
static void h9_ak_normalise(double *x, double *y, double *z) {
    const double n = sqrt(*x * *x + *y * *y + (*z * *z) / H9_NORM_B2);
    *x /= n; *y /= n; *z /= n;
}

/* ── Coordinate conversions ─────────────────────────────────────────────── */

/* Signed normalised barycentric → ECEF (WGS84, metres). */
static void h9_c_oct_to_c_ell(double u, double v, double w,
                               double *X, double *Y, double *Z) {
    const double su = (u >= 0.0) ? 1.0 : -1.0;
    const double sv = (v >= 0.0) ? 1.0 : -1.0;
    const double sw = (w >= 0.0) ? 1.0 : -1.0;
    const double au = fabs(u), av = fabs(v), aw = fabs(w);
    if (av < H9_VERT_EPS && aw < H9_VERT_EPS) {
        *X = H9_WGS84_A * su; *Y = 0.0; *Z = 0.0; return;
    }
    if (au < H9_VERT_EPS && aw < H9_VERT_EPS) {
        *X = 0.0; *Y = H9_WGS84_A * sv; *Z = 0.0; return;
    }
    if (au < H9_VERT_EPS && av < H9_VERT_EPS) {
        *X = 0.0; *Y = 0.0; *Z = H9_WGS84_A * sw; return;
    }
    h9_ak_core(au, av, aw, X, Y, Z);
    h9_ak_normalise(X, Y, Z);
    *X *= H9_WGS84_A * su;
    *Y *= H9_WGS84_A * sv;
    *Z *= H9_WGS84_A * sw;
}

/* ECEF → geodetic lon/lat (radians). Standard surface-Bowring (5 iters).
 * Mirrors Python hhg9/algorithms/wgs84.py::ecef_to_latlon — bit-identical
 * fixed point at h=0 surface points, which is what every caller passes. */
static void h9_c_ell_to_lonlat(double X, double Y, double Z,
                                double *lon, double *lat) {
    const double e2  = 1.0 - H9_WGS84_B2 / H9_WGS84_A2;
    const double p   = sqrt(X*X + Y*Y);
    *lon = atan2(Y, X);
    *lat = atan2(Z, p * (1.0 - e2));
    for (int k = 0; k < 5; ++k) {
        const double sin_lat = sin(*lat);
        const double N       = H9_WGS84_A / sqrt(1.0 - e2 * sin_lat * sin_lat);
        *lat = atan2(Z + e2 * N * sin_lat, p);
    }
}

/* Signed barycentric with explicit octant signs → lon/lat. */
static void h9_coct_to_lonlat(double u, double v, double w,
                               double su, double sv, double sw,
                               double *lon, double *lat) {
    double X, Y, Z;
    h9_c_oct_to_c_ell(su*u, sv*v, sw*w, &X, &Y, &Z);
    h9_c_ell_to_lonlat(X, Y, Z, lon, lat);
}

/* ── Warp: authalic correction ───────────────────────────────────────────── */
/*                                                                             */
/*   Grid stores the b_oct → b_raw displacement (AuthalicWarp.do convention). */
/*   Forward warp b_raw → b_oct requires Newton-Raphson inversion (= Python's */
/*   AuthalicWarp.undo), not direct bilinear application.                      */
/*                                                                             */
/*   Inputs use the braw_y convention: ry0 is positive for mode-1 UP octants. */
/*   The grid is in mode-0 space, so mode-1 y is negated before lookup and    */
/*   the result y is negated back.                                             */
/*                                                                             */
/*   NR iteration (20 steps): cx = rx - dx(cx,cy),  cy = ry - dy(cx,cy)      */
/*     mode-0:  ry = ry0 (no flip);  wy0 = cy        (no flip back)           */
/*     mode-1:  ry = -ry0 (flip);    wy0 = -cy        (flip back)             */

#if H9_WARP_ENABLE
/* h9_warp_fwd / h9_warp_inv live in h9_warp_runtime.h, which is included
 * at the end of this file to keep the legacy call sites unchanged. The
 * pre-port static-data CT walker and edge-blend stubs that used to live
 * here have been replaced by the runtime path.
 *
 * Note: caller MUST have invoked h9::h9_warp_init_embedded() before any
 * warp_fwd/inv call; otherwise the shims fall through to identity. */
#endif /* H9_WARP_ENABLE */

/* Project (fx,fy) onto the fundamental domain via UVW barycentric clamping.
 * Converts to barycentric coords, zeroes negatives, renormalises, converts back.
 * Handles corner violations correctly (sequential edge clamps fail at corners). */
static void h9_clamp_bary(double *fx, double *fy, int oct_mode) {
    const double iH  = H9.inv_H, iW = H9.inv_W, i2H = H9.inv_2H;
    double u, v, w;
    if (oct_mode == 0) {
        u = (1.0/3.0) - (*fy) * iH;
        v = (1.0/3.0) + (*fy) * i2H + (*fx) * iW;
        w = (1.0/3.0) + (*fy) * i2H - (*fx) * iW;
    } else {
        u = (1.0/3.0) + (*fy) * iH;
        v = (1.0/3.0) - (*fy) * i2H + (*fx) * iW;
        w = (1.0/3.0) - (*fy) * i2H - (*fx) * iW;
    }
    if (u < 0.0) u = 0.0;
    if (v < 0.0) v = 0.0;
    if (w < 0.0) w = 0.0;
    const double s = u + v + w;
    if (s > 1e-30) { u /= s; v /= s; w /= s; }
    if (oct_mode == 0) {
        *fy = -H9.H * (u - (1.0/3.0));
        *fx =  H9.W * 0.5 * (v - w);
    } else {
        *fy =  H9.H * (u - (1.0/3.0));
        *fx =  H9.W * 0.5 * (v - w);
    }
}

/* ── Orient rotation ─────────────────────────────────────────────────────── */
/*                                                                             */
/*   CCW rotation by ORIENT_TH[oct_i] * π/3 maps b_raw (fx,fy) to the        */
/*   canonical oriented frame (rx,ry) in which the warp grid is defined.      */

static void h9_orient_fwd(double fx, double fy, int oct_i,
                           double *rx, double *ry) {
    const double th = H9_ORIENT_TH[oct_i ^ 7] * (M_PI / 3.0);  /* ^7: Python oid → C++ oct_i */
    const double fc = cos(th), fs = sin(th);
    *rx = fx * fc - fy * fs;
    *ry = fx * fs + fy * fc;
}

/* Inverse (CW = transpose) rotation: oriented (rx,ry) → b_raw (fx,fy). */
static void h9_orient_inv(double rx, double ry, int oct_i,
                           double *fx, double *fy) {
    const double th = H9_ORIENT_TH[oct_i ^ 7] * (M_PI / 3.0);  /* ^7: Python oid → C++ oct_i */
    const double fc = cos(th), fs = sin(th);
    *fx =  rx * fc + ry * fs;
    *fy = -rx * fs + ry * fc;
}

/* ── Recover unoriented b_raw (fx, fy) from H9BOct ─────────────────────── */
/*                                                                             */
/*   Inverts the (u,v,w) ↔ (fx,fy) c_oct formulas for the given oct_mode.    */
/*   Result is in the same metric-barycentric space as H9Tables offsets.      */
/*                                                                             */
/*   Up   (mode=1): fx = W/2·(|w|−|v|),  fy = H·(|u|−1/3)                   */
/*   Down (mode=0): fx = W/2·(|v|−|w|),  fy = H·(1/3−|u|)                   */

static void h9_braw_from_boct(H9BOct b, double *fx, double *fy) {
    const double au = fabs(b.u), av = fabs(b.v), aw = fabs(b.w);
    if (b.oct_mode == 1) {
        *fy =  H9.H * (au - 1.0/3.0);
        *fx =  H9.W * 0.5 * (aw - av);
    } else {
        *fy = -H9.H * (au - 1.0/3.0);
        *fx =  H9.W * 0.5 * (av - aw);
    }
}

/* ── Encode: (lon, lat) radians → H9BOct ────────────────────────────────── */
/*                                                                             */
/*   Beam search (width=6, depth=40) in b_raw metric barycentric space.       */
/*   Matches h9_boct_fwd3d in test_h9_standalone.cpp and h9_boct.cpp          */
/*   (without PROJ type wrappers and without authalic warp — that is applied   */
/*   by the addressing layer in warped b_oct space if required).              */

static void h9_rad_lonlat_to_ecef(double lon_rad, double lat_rad, double *x, double *y, double *z) {
	const double sin_lat = sin(lat_rad);
	const double cos_lat = cos(lat_rad);
	const double n = H9_WGS84_A / sqrt(1.0 - H9_WGS84_E2 * (sin_lat * sin_lat));
	*x = n * cos_lat * cos(lon_rad);
	*y = n * cos_lat * sin(lon_rad);
	*z = n * (1.0 - H9_WGS84_E2) * sin_lat;
}

static void h9_ecef_to_rad_lonlat(double x, double y, double z, double *lon_rad, double *lat_rad) {
	const double p = sqrt(x * x + y * y);
	*lon_rad = atan2(y, x);
	*lat_rad = atan2(z, p * H9_NORM_B2);
}


static H9BOct h9_lonlat_to_boct(double lon_rad, double lat_rad) {
	H9BOct result;
	double eX, eY, eZ;
	h9_rad_lonlat_to_ecef(lon_rad, lat_rad, &eX, &eY, &eZ);

	const double cl = cos(lat_rad);
	result.oct_i = oid(eX, eY, eZ);
	result.oct_mode = mode(result.oct_i);

	/* Axis sign values — used in beam search (h9_coct_to_lonlat) and final
	 * result assignment. Separate from oct_i bit encoding. */
	const double su = (eX >= 0.0) ? 1.0 : -1.0;
	const double sv = (eY >= 0.0) ? 1.0 : -1.0;
	const double sw = (eZ >= 0.0) ? 1.0 : -1.0;

    /* Pole shortcut: b_raw vertex for w=1. */
	const double VT = 1e-9;  // tolerance level.
	if (fabs(fabs(eZ) - H9_WGS84_B) < VT) {
		result.w = eZ > 0 ? 1.0 : -1.0;
		result.u = 0.0; result.v = 0.0;
		return result;
	}

	if (fabs(fabs(eX) - H9_WGS84_A) < VT) {
		result.u = eX > 0 ? 1.0 : -1.0;
		result.v = 0.0; result.w = 0.0;
		return result;
	}

	if (fabs(fabs(eY) - H9_WGS84_A) < VT) {
		result.v = eY > 0 ? 1.0 : -1.0;
		result.u = 0.0; result.w = 0.0;
		return result;
	}

	const int oct_mode = result.oct_mode;
    const double cos_lat  = cl;
    const double inv_H    = H9.inv_H, inv_W = H9.inv_W, inv_2H = H9.inv_2H;

    /* Beam search */
    const int BEAM = 6, DEPTH = 40;
    struct Cand { int mode; double x, y, dist; };
    Cand cur[BEAM], nxt[BEAM * 9];
    int bw = 1;
    cur[0] = {oct_mode, 0.0, 0.0, 1e300};

    double scale = 1.0;
    for (int d = 0; d < DEPTH; ++d) {
        int nn = 0;
        for (int k = 0; k < bw; ++k) {
            const double *off_x     = (cur[k].mode == 1) ? H9.UP_X : H9.DN_X;
            const double *off_y     = (cur[k].mode == 1) ? H9.UP_Y : H9.DN_Y;
            const int    *chld_mode = (cur[k].mode == 1) ? H9_UP_MODE : H9_DN_MODE;
            for (int j = 0; j < 9; ++j) {
                const double cx = cur[k].x + off_x[j] * scale;
                const double cy = cur[k].y + off_y[j] * scale;
                double cu, cv, cw;
                if (oct_mode == 1) {
                    cu = cy * inv_H  + (1.0/3.0);
                    cv = (1.0/3.0) - cy * inv_2H - cx * inv_W;
                    cw = (1.0/3.0) - cy * inv_2H + cx * inv_W;
                } else {
                    cu = (1.0/3.0) - cy * inv_H;
                    cv = (1.0/3.0) + cy * inv_2H + cx * inv_W;
                    cw = (1.0/3.0) + cy * inv_2H - cx * inv_W;
                }
                if (cu < 0.0) cu = 0.0;
                if (cv < 0.0) cv = 0.0;
                if (cw < 0.0) cw = 0.0;
                const double s = cu + cv + cw;
                if (s > 1e-30) { cu /= s; cv /= s; cw /= s; }
                double lon0, lat0;
                h9_coct_to_lonlat(cu, cv, cw, su, sv, sw, &lon0, &lat0);
                double dlat = lat0 - lat_rad;
                double dlon = lon0 - lon_rad;
                if (dlon >  M_PI) dlon -= 2.0*M_PI;
                if (dlon < -M_PI) dlon += 2.0*M_PI;
                const double sl = sin(dlat * 0.5), sd = sin(dlon * 0.5);
                nxt[nn++] = {chld_mode[j], cx, cy,
                             sl*sl + cos_lat * cos(lat0) * sd*sd};
            }
        }
        int bw_new = (nn < BEAM) ? nn : BEAM;
        /* Partial selection sort: put the best bw_new candidates first. */
        for (int i = 0; i < bw_new; ++i) {
            int best = i;
            for (int j = i+1; j < nn; ++j)
                if (nxt[j].dist < nxt[best].dist) best = j;
            Cand tmp = nxt[i]; nxt[i] = nxt[best]; nxt[best] = tmp;
        }
        for (int i = 0; i < bw_new; ++i) cur[i] = nxt[i];
        bw = bw_new;
        if (cur[0].dist < 3e-31) break;
        scale /= 3.0;
    }

    /* NR post-refinement: converge (fx,fy) to machine precision.
     * Skipped when the beam lands near a vertex: ∂lat/∂fy → 0 there, making
     * the Jacobian ill-conditioned and the NR step diverge despite det > 1e-30. */
    double fx = cur[0].x, fy = cur[0].y;
    bool skip_nr = true;
    {
        /* Vertex proximity check: skip NR if any barycentric coord > 0.85. */
        double vt_u, vt_v, vt_w;
        if (oct_mode == 0) {
            vt_u = (1.0/3.0) - fy * inv_H;
            vt_v = (1.0/3.0) + fy * inv_2H + fx * inv_W;
            vt_w = (1.0/3.0) + fy * inv_2H - fx * inv_W;
        } else {
            vt_u = (1.0/3.0) + fy * inv_H;
            vt_v = (1.0/3.0) - fy * inv_2H + fx * inv_W;
            vt_w = (1.0/3.0) - fy * inv_2H - fx * inv_W;
        }
        skip_nr = (vt_u > 0.85 || vt_v > 0.85 || vt_w > 0.85);
    }
    if (!skip_nr)
    {
        /* NR usually exits at iter 0 since beam search already converges
         * below the lon/lat residual threshold; nr_eps only matters in
         * the rare cases where beam plateaus above threshold. */
        const double nr_eps = 1e-7;
        auto to_lonlat = [&](double qx, double qy, double *qlon, double *qlat) {
            double qu, qv, qw;
            if (oct_mode == 1) {
                qu = qy * inv_H  + (1.0/3.0);
                qv = (1.0/3.0) - qy * inv_2H - qx * inv_W;
                qw = (1.0/3.0) - qy * inv_2H + qx * inv_W;
            } else {
                qu = (1.0/3.0) - qy * inv_H;
                qv = (1.0/3.0) + qy * inv_2H + qx * inv_W;
                qw = (1.0/3.0) + qy * inv_2H - qx * inv_W;
            }
            if (qu < 0.0) qu = 0.0;
            if (qv < 0.0) qv = 0.0;
            if (qw < 0.0) qw = 0.0;
            const double qs = qu + qv + qw;
            if (qs > 1e-30) { qu /= qs; qv /= qs; qw /= qs; }
            h9_coct_to_lonlat(qu, qv, qw, su, sv, sw, qlon, qlat);
        };
        for (int nr = 0; nr < 8; ++nr) {
            double lon0, lat0;
            to_lonlat(fx, fy, &lon0, &lat0);
            double dlon = lon_rad - lon0, dlat = lat_rad - lat0;
            if (dlon >  M_PI) dlon -= 2.0 * M_PI;
            if (dlon < -M_PI) dlon += 2.0 * M_PI;
            if (fabs(dlon) < 1e-15 && fabs(dlat) < 1e-15) break;
            double lp, lm, ap, am;
            to_lonlat(fx + nr_eps, fy, &lp, &ap);
            to_lonlat(fx - nr_eps, fy, &lm, &am);
            const double lon_fx = (lp - lm) / (2.0 * nr_eps);
            const double lat_fx = (ap - am) / (2.0 * nr_eps);
            to_lonlat(fx, fy + nr_eps, &lp, &ap);
            to_lonlat(fx, fy - nr_eps, &lm, &am);
            const double lon_fy = (lp - lm) / (2.0 * nr_eps);
            const double lat_fy = (ap - am) / (2.0 * nr_eps);
            const double det = lon_fx * lat_fy - lon_fy * lat_fx;
            if (fabs(det) < 1e-30) break;
            fx += ( lat_fy * dlon - lon_fy * dlat) / det;
            fy += (-lat_fx * dlon + lon_fx * dlat) / det;
        }
    }

    /* Convert final (fx,fy) to normalised barycentric (u,v,w). */
    double u, v, w;
    if (oct_mode == 1) {
        u = fy * inv_H  + (1.0/3.0);
        v = (1.0/3.0) - fy * inv_2H - fx * inv_W;
        w = (1.0/3.0) - fy * inv_2H + fx * inv_W;
    } else {
        u = (1.0/3.0) - fy * inv_H;
        v = (1.0/3.0) + fy * inv_2H + fx * inv_W;
        w = (1.0/3.0) + fy * inv_2H - fx * inv_W;
    }
    if (u < 0.0) u = 0.0;
    if (v < 0.0) v = 0.0;
    if (w < 0.0) w = 0.0;
    const double s = u + v + w;
    if (s > 1e-30) { u /= s; v /= s; w /= s; }

    result.u = su * u;
    result.v = sv * v;
    result.w = sw * w;
    return result;
}

/* ── Decode: H9BOct → (lon, lat) radians ────────────────────────────────── */
/*                                                                             */
/*   AK analytical inverse — no beam search. The signed (u,v,w) carry enough  */
/*   information for the full inverse: h9_c_oct_to_c_ell handles the signs    */
/*   internally, so no oct_i or oct_mode is needed here.                      */

static void h9_boct_to_lonlat(H9BOct b, double *lon_rad, double *lat_rad) {
    double X, Y, Z;
    h9_c_oct_to_c_ell(b.u, b.v, b.w, &X, &Y, &Z);
    h9_c_ell_to_lonlat(X, Y, Z, lon_rad, lat_rad);
}

/* ── Degree convenience wrappers ─────────────────────────────────────────── */

static H9BOct h9_lonlatdeg_to_boct(double lon_deg, double lat_deg) {
    return h9_lonlat_to_boct(lon_deg * (M_PI / 180.0), lat_deg * (M_PI / 180.0));
}

static void h9_boct_to_lonlatdeg(H9BOct b, double *lon_deg, double *lat_deg) {
    double lon_r, lat_r;
    h9_boct_to_lonlat(b, &lon_r, &lat_r);
    *lon_deg = lon_r * (180.0 / M_PI);
    *lat_deg = lat_r * (180.0 / M_PI);
}


/* Pull in the runtime warp shims (h9_warp_fwd / h9_warp_inv). The header
 * is guarded; including from h9_math.h means every legacy call site
 * keeps its current signature without source edits. */
#include "h9_warp_runtime.h"
