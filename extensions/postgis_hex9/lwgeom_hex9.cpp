/*
 * lwgeom_hex9.cpp — PostgreSQL/PostGIS C++ extension for Hex9 DGGS.
 *
 * Part of the Hex9 (H9) Project
 * Copyright ©2025, Ben Griffin
 * Licensed under the Apache License, Version 2.0
 *
 * SQL functions (all use h9_ prefix to match existing Python reference):
 *
 *	 h9_encode(geometry)			→ uuid		 encode point to self-contained UUID
 *	 h9_decode(uuid)				→ geometry	 POINT(lon lat) SRID 4326
 *	 h9_bin(uuid, integer)		→ uuid		 IMMUTABLE — bin key at layer L (0..29)
 *	 h9_cell(uuid, integer)		 → geometry	 cell polygon SRID 4326 (layer 1..29)
 *	 h9_label(uuid, integer)		→ text		 human label e.g. '478232778'
 *	 h9_label_key(uuid, integer)	→ text		 label with key_tail e.g. '478232778.9'
 *	 h9_grid(integer, geometry)	 → TABLE(hex9 uuid, geom geometry)
 *
 * Build notes
 * ───────────
 *	 Compile as C++ (not C): this file uses struct aggregate init and
 *	 local struct definitions that require C++11 or later.
 *
 *	 Required headers from this directory:
 *	 h9_warp_data.h	 — copy from h9_proj/proj-9.8.0/src/projections/
 *	 h9_math.h		— self-contained math layer (no PROJ, no PG)
 *	 h9_addressing.h	— UUID nibble encode/decode (Ben's structural piece)
 *
 *	 The PG_FUNCTION_INFO_V1 macros and Datum prototypes must sit in
 *	 extern "C" blocks so the PostgreSQL loader can find them by name.
 */

extern "C" {
#include "postgres.h"
#include "fmgr.h"
#include "utils/uuid.h"		 /* pg_uuid_t, UUID_LEN */
#include "utils/builtins.h"	 /* CStringGetTextDatum etc. */
#include "utils/guc.h"		 /* DefineCustomIntVariable */
#include "catalog/pg_type.h"	/* UUIDOID, INT2OID */
#include "funcapi.h"			/* ReturnSetInfo, SRF_* macros */
#include "access/htup_details.h"

/* PostGIS */
#include "liblwgeom.h"
#include "lwgeom_pg.h"

PG_MODULE_MAGIC;

} /* extern "C" */

/* Runtime liblwgeom resolution — see header. Must follow liblwgeom.h (types)
 * and precede the redirect macros further down. */
#include "h9_lwgeom_shim.h"

#undef  H9_WARP_ENABLE
#define H9_WARP_ENABLE 1  /* warp ON; runtime toggle via `SET hex9.use_warp = …` */
#include "h9_math.h"
#include "h9_addressing.h"	 /* h9_boct_to_uuid, h9_uuid_to_boct, h9_bin_uuid */
#include "h9_uv_lattice.h"   /* integer UV supercell constants for create_clipped */
#include "h9_grid.h"         /* h9grid::enumerate, H9GridCell, uuid_from_cxcy */
#include "h9_warp_runtime.h" /* h9::h9_warp_init_embedded() for _PG_init */
#include <vector>

/* ── _PG_init: build the runtime WarpState once per backend ─────────────
 *
 * Runs at LOAD time (before any function call). Builds the CT state from
 * the embedded sidecar blob; ~13 s. If H9_WARP_ENABLE is 0 (current
 * default for binning/grid), the WarpState is still built so that any
 * SQL function compiled with H9_WARP_ENABLE=1 has a ready state without
 * paying the build cost on first query. */
/* Forward declaration — definition lives where it's first used (h9_grid SRF). */
static int h9_grid_max_cells;

/* h9_encode encoder choice. Default 0 = NN (h9_boct_to_uuid in
 * h9_addressing.h); 1 = containment (uuid_from_cxcy_full in h9_grid.h,
 * matches h9_grid's cell-key encoder at boundaries). Set via the
 * `hex9.encoder` GUC registered in _PG_init. */
static int h9_encoder_mode = 0;
static const struct config_enum_entry h9_encoder_options[] = {
    {"nn",          0, false},
    {"containment", 1, false},
    {NULL,          0, false}
};

/* Resolve liblwgeom entry points (see h9_lwgeom_shim.h). Defined here, ahead of
 * the redirect macros, so the real symbol names reach dlsym. */
extern "C" void h9_lwgeom_resolve(void) {
	/* load_file runs PostGIS's _PG_init (wires liblwgeom's palloc/pfree
	 * handlers) and loads it RTLD_GLOBAL; no-op if already resident. */
	load_file("$libdir/postgis-3", false);

	#define H9_RESOLVE(fn)                                                     \
		do {                                                                   \
			h9lw_##fn = (h9lw_fp_##fn) dlsym(RTLD_DEFAULT, #fn);               \
			if (!h9lw_##fn)                                                    \
				ereport(ERROR,                                                 \
					(errcode(ERRCODE_UNDEFINED_FUNCTION),                      \
					 errmsg("postgis_hex9: cannot resolve liblwgeom symbol "   \
					        "\"%s\"", #fn),                                    \
					 errhint("A compatible PostGIS (3.x) must be installed."))); \
		} while (0)

	H9_RESOLVE(getPoint4d_p);
	H9_RESOLVE(gserialized_from_lwgeom);
	H9_RESOLVE(lwgeom_calculate_gbox);
	H9_RESOLVE(lwgeom_free);
	H9_RESOLVE(lwgeom_from_gserialized);
	H9_RESOLVE(lwpoint_free);
	H9_RESOLVE(lwpoint_make2d);
	H9_RESOLVE(lwpoly_construct);
	H9_RESOLVE(lwpoly_contains_point);
	H9_RESOLVE(lwpoly_free);
	H9_RESOLVE(ptarray_construct);
	H9_RESOLVE(ptarray_set_point4d);

	#undef H9_RESOLVE
}

extern "C" PGDLLEXPORT void _PG_init(void);
extern "C" PGDLLEXPORT void _PG_init(void) {
	/* Resolve liblwgeom first — every geometry-producing function depends on
	 * it, and this also ensures PostGIS is loaded for the rest of init. */
	h9_lwgeom_resolve();

#if H9_WARP_ENABLE
	/* Warp init takes ~13 s and is only useful for addressing paths
	 * (encode/decode). The grid pipeline is currently no-warp by
	 * construction (see h9_grid.h:10), so skipping init when the warp is
	 * disabled saves the cost on every backend start. */
	std::string err;
	if (!h9::h9_warp_init_embedded(&err)) {
		ereport(WARNING,
			(errmsg("postgis_hex9: warp init failed: %s "
			        "(falling back to identity warp)", err.c_str())));
	}
#endif

	/* hex9.grid_max_cells — operator-tunable safety cap for h9_grid.
	 * Default 708,588 = 12 × 9⁵ = global L5 cell count. Bump it (e.g.
	 *   SET hex9.grid_max_cells = 5000000;
	 * ) when validating coarser global meshes or running area-uniformity
	 * tests at deeper layers. */
	DefineCustomIntVariable(
		"hex9.grid_max_cells",
		"Maximum estimated cells h9_grid() will produce before erroring.",
		"Estimated from bbox area / mean cell area at the requested layer. "
		"Raise via SET if your bbox legitimately exceeds the default.",
		&h9_grid_max_cells,
		708588,        /* boot */
		1000,          /* min */
		100000000,     /* max — 100M, plenty of headroom */
		PGC_USERSET,
		0,
		NULL, NULL, NULL);

	/* hex9.encoder — A/B switch between the two encoder paths in h9_encode.
	 *
	 *   SET hex9.encoder = 'nn';           -- legacy 1-best NN descent (default)
	 *   SET hex9.encoder = 'containment';  -- containment classifier path
	 *                                         matching h9_grid; fixes the
	 *                                         0.13% L13 boundary divergence
	 *                                         (Phase 1 — uses simple NN
	 *                                         fallback for the few points
	 *                                         that don't classify cleanly;
	 *                                         Phase 2 will port _recover).
	 */
	DefineCustomEnumVariable(
		"hex9.encoder",
		"Encoder used by h9_encode: 'nn' (legacy) or 'containment' (grid-canonical).",
		"NN matches h9_boct_to_uuid; containment matches uuid_from_cxcy. "
		"They diverge at cell boundaries (~0.13% of L13 cells).",
		&h9_encoder_mode,
		0,                  /* boot — keep legacy default until Phase 3 lands */
		h9_encoder_options,
		PGC_USERSET,
		0,
		NULL, NULL, NULL);

#if H9_WARP_ENABLE
	/* hex9.use_warp — runtime toggle for the authalic warp.
	 *
	 *   SET hex9.use_warp = off;   -- bypass warp, fall through to identity
	 *   SET hex9.use_warp = on;    -- re-enable (default)
	 *
	 * Useful for A/B comparison ("does this query differ with/without
	 * warp?") without rebuilding. Affects both addressing (encode/decode)
	 * and grid (cell-vertex projection) since both go through the same
	 * h9_warp_fwd shim. */
	DefineCustomBoolVariable(
		"hex9.use_warp",
		"Apply the authalic warp when projecting cells / encoding points.",
		"When off, h9_warp_fwd/inv are identity. The WarpState is still "
		"built at backend start (~13 s); only the apply is bypassed.",
		&h9::g_warp_use,
		true,           /* boot */
		PGC_USERSET,
		0,
		NULL, NULL, NULL);
#endif
}
/* lwpoly_contains_point is in liblwgeom_internal.h (not the public header) but
 * is exported from postgis-3.dylib; forward-declare with C linkage. */
extern "C" int lwpoly_contains_point(const LWPOLY *poly, const POINT2D *pt);

/* Redirect the natural liblwgeom names onto the runtime-resolved pointers
 * (h9_lwgeom_shim.h). Defined here — after every liblwgeom prototype and after
 * h9_lwgeom_resolve()'s body — so neither the library headers nor the resolver
 * are rewritten. Every call site below this point binds through the pointer,
 * leaving our module with no undefined liblwgeom symbols. */
#define getPoint4d_p            (*h9lw_getPoint4d_p)
#define gserialized_from_lwgeom (*h9lw_gserialized_from_lwgeom)
#define lwgeom_calculate_gbox   (*h9lw_lwgeom_calculate_gbox)
#define lwgeom_free             (*h9lw_lwgeom_free)
#define lwgeom_from_gserialized (*h9lw_lwgeom_from_gserialized)
#define lwpoint_free            (*h9lw_lwpoint_free)
#define lwpoint_make2d          (*h9lw_lwpoint_make2d)
#define lwpoly_construct        (*h9lw_lwpoly_construct)
#define lwpoly_contains_point   (*h9lw_lwpoly_contains_point)
#define lwpoly_free             (*h9lw_lwpoly_free)
#define ptarray_construct       (*h9lw_ptarray_construct)
#define ptarray_set_point4d     (*h9lw_ptarray_set_point4d)

/* ── h9_version() → text ─────────────────────────────────────────────────── */

extern "C" {
PG_FUNCTION_INFO_V1(h9_version);
/* H9_GIT_REV is injected by the Makefile via -DH9_GIT_REV=… so every
 * build is uniquely identifiable. The fallback "dev" applies when
 * compiled outside the Makefile (e.g. Xcode-only CLI builds). */
#ifndef H9_GIT_REV
#define H9_GIT_REV "dev"
#endif
Datum h9_version(PG_FUNCTION_ARGS) {
	const char *ver = "postgis_hex9 1.0.0+" H9_GIT_REV " built " __DATE__ " " __TIME__;
	PG_RETURN_TEXT_P(cstring_to_text(ver));
}
} /* extern "C" */

/* ── Internal helpers ────────────────────────────────────────────────────── */

/*
 * Extract lon/lat degrees from a PostGIS geometry.
 * Returns false (and sets *err) if the geometry is not a POINT.
 */
static bool geom_to_lonlat(GSERIALIZED *g, double *lon, double *lat,
							const char **err) {
	LWGEOM	*lwg = lwgeom_from_gserialized(g);
	if (lwg->type != POINTTYPE) {
		*err = "h9 functions require a POINT geometry";
		lwgeom_free(lwg);
		return false;
	}
	LWPOINT *pt	= lwgeom_as_lwpoint(lwg);
	POINT4D	p;
	getPoint4d_p(pt->point, 0, &p);
	*lon = p.x;
	*lat = p.y;
	lwgeom_free(lwg);
	return true;
}

/*
 * Build a 2D POINT geometry (SRID 4326) from lon/lat degrees.
 */
static GSERIALIZED *lonlat_to_geom(double lon, double lat) {
	LWPOINT	*pt	= lwpoint_make2d(4326, lon, lat);
	GSERIALIZED *g	= gserialized_from_lwgeom((LWGEOM *)pt, NULL);
	lwpoint_free(pt);
	return g;
}

/*
 * Copy a raw 16-byte UUID value into a palloc'd pg_uuid_t.
 */
static pg_uuid_t *make_pg_uuid(const uint8_t bytes[UUID_LEN]) {
	pg_uuid_t *u = (pg_uuid_t *) palloc(sizeof(pg_uuid_t));
	memcpy(u->data, bytes, UUID_LEN);
	return u;
}

/* ── h9_diag(geometry) → text ───────────────────────────────────────────
 *
 * Diagnostic helper: returns BRAW (pre-warp) and BARY (post-warp) (cx, cy)
 * that C++ would feed into the encoder for a given geographic point.
 * Lets us bisect frame mismatches vs Python's b_oct.encode pipeline
 * without rebuilding. Drop once encoder parity is confirmed. */

extern "C" {
PG_FUNCTION_INFO_V1(h9_diag);
Datum h9_diag(PG_FUNCTION_ARGS) {
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	GSERIALIZED *g = PG_GETARG_GSERIALIZED_P(0);
	double lon, lat;
	const char *err = NULL;
	if (!geom_to_lonlat(g, &lon, &lat, &err))
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("%s", err)));

	double cx, cy;
	int    oid;
	h9grid::lonlatdeg_to_cxcy_oid(lon, lat, &cx, &cy, &oid);
	const int oct_mode = (int)H9_OID_MO[oid];
	double bx = cx, by = cy;
#if H9_WARP_ENABLE
	/* Match the encoder pipeline: descent runs on warp_inv(BRAW). */
	h9_warp_inv(cx, cy, oct_mode, &bx, &by);
#endif

	char buf[256];
	int n = snprintf(buf, sizeof(buf),
		"BRAW=(%.17g, %.17g) descent=(%.17g, %.17g) oid=%d mode=%d",
		cx, cy, bx, by, oid, oct_mode);
	PG_RETURN_TEXT_P(cstring_to_text_with_len(buf, n));
}
} /* extern "C" */

/* ── h9_encode(geometry) → uuid ─────────────────────────────────────────── */

extern "C" {
PG_FUNCTION_INFO_V1(h9_encode);
Datum h9_encode(PG_FUNCTION_ARGS) {
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();

	GSERIALIZED *g = PG_GETARG_GSERIALIZED_P(0);
	double lon, lat;
	const char *err = NULL;
	if (!geom_to_lonlat(g, &lon, &lat, &err))
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("%s", err)));

	uint8_t uuid_bytes[UUID_LEN];
	if (h9_encoder_mode == 1) {
		/* Containment encoder — uses the same descent as h9_grid's
		 * uuid_from_cxcy. Agrees with h9_grid at cell boundaries. */
		double cx, cy;
		int    oid;
		h9grid::lonlatdeg_to_cxcy_oid(lon, lat, &cx, &cy, &oid);
		h9grid::uuid_from_cxcy_full(cx, cy, oid, uuid_bytes);
	} else {
		/* Legacy NN encoder — h9_boct_to_uuid. */
		H9BOct b = h9_lonlatdeg_to_boct(lon, lat);
		h9_boct_to_uuid(b, uuid_bytes);
	}
	PG_RETURN_UUID_P(make_pg_uuid(uuid_bytes));
}
} /* extern "C" */

/* ── h9_decode(uuid) → geometry ─────────────────────────────────────────── */
/*																			 */
/*	 Decode a UUID back to a POINT geometry (SRID 4326).						*/
/*	 The UUID is self-contained: no companion byte is needed.				 */

extern "C" {
PG_FUNCTION_INFO_V1(h9_decode);
Datum h9_decode(PG_FUNCTION_ARGS) {
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();

	pg_uuid_t *u = PG_GETARG_UUID_P(0);
	H9BOct b = h9_uuid_to_boct(u->data);

	double lon, lat;
	h9_boct_to_lonlatdeg(b, &lon, &lat);

	GSERIALIZED *g = lonlat_to_geom(lon, lat);
	PG_RETURN_POINTER(g);
}
} /* extern "C" */

/* ── h9_bin(uuid, integer) → uuid ───────────────────────────────────────── */
/*																			 */
/*	 Returns the bin-key UUID at the given layer (0..30).					 */
/*	 All points in the same cell at layer L produce the same bin UUID.		*/
/*	 The adr byte is NOT needed.												*/
/*	 Declared IMMUTABLE and STRICT in SQL — safe for GENERATED STORED cols	*/
/*	 and functional indexes.													 */

extern "C" {
PG_FUNCTION_INFO_V1(h9_bin);
Datum h9_bin(PG_FUNCTION_ARGS) {
	if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) PG_RETURN_NULL();

	pg_uuid_t *u	 = PG_GETARG_UUID_P(0);
	int32		layer = PG_GETARG_INT32(1);

	if (layer < 0 || layer > 29)
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("h9_bin: layer must be 0..29, got %d", layer)));

	uint8_t out_bytes[UUID_LEN];
	h9_bin_uuid(u->data, layer, out_bytes);

	PG_RETURN_UUID_P(make_pg_uuid(out_bytes));
}
} /* extern "C" */

/* ── Cell geometry helpers ───────────────────────────────────────────────── */

/* Approximate inradius in metres for an H9 cell at layer L.
 * Derived from earth surface area: A = 5.1006e14 m², hex formula r=√(A/(2√3)). */
static double h9_inradius_m(int layer) {
	const double cell_area = 5.1006e14 / (12.0 * pow(9.0, (double)layer));
	return sqrt(cell_area / (2.0 * 1.7320508));
}

/* Common preamble shared by h9_make_cell_geom and h9_cell_centre_lonlat.
 * Decodes the bin UUID into (fx, fy) cell centre in metric-bary space plus
 * the octant identity fields. */
static void h9_cell_unpack(const uint8_t uuid[16], int layer,
							double *fx_out, double *fy_out,
							int *oct_i_out, int *oct_mode_out) {
	uint8_t nibbles[32];
	h9a_unpack(uuid, nibbles);

	const uint8_t key_tail = nibbles[31];
	const uint8_t r_mo = key_tail & 1u;
	uint8_t c2   = (key_tail >> 1) & 3u;
	/* Python bin UUID nibble[31] = (c2<<1)|r_mo — no c_mo bit (always 0 in stored data).
	 * Detect bin UUIDs by nibbles[30]==0xF and try both c_mo values, picking the one
	 * whose backward chain terminates at the expected L0 c2. */
	uint8_t rids[32] = {};
	rids[0] = r_mo;
	const bool is_bin = (nibbles[30] == 0x0Fu);
	if (is_bin) {
		const uint8_t expected_c2 = H9_L0HEX_BACK[nibbles[0]][r_mo][1];
		for (int try_cmo = 0; try_cmo < 2; try_cmo++) {
			uint8_t tc_mo = (uint8_t)try_cmo, tc2 = c2;
			uint8_t tmp_rids[32] = {};
			tmp_rids[0] = r_mo;
			for (int l = layer; l >= 1; --l) {
				const uint8_t *e = H9_HEX_REG[nibbles[l]][tc_mo][tc2];
				tmp_rids[l] = e[0]; tc_mo = e[1]; tc2 = e[2];
			}
			if (tc2 == expected_c2 || try_cmo == 1) {
				memcpy(rids, tmp_rids, (size_t)(layer + 2));
				break;
			}
		}
	} else {
		uint8_t c_mo = (key_tail >> 3) & 1u;
		for (int l = layer; l >= 1; --l) {
			const uint8_t *entry = H9_HEX_REG[nibbles[l]][c_mo][c2];
			rids[l] = entry[0]; c_mo = entry[1]; c2 = entry[2];
		}
	}

	const uint8_t py_oid = H9_L0HEX_BACK[nibbles[0]][r_mo][0];
	*oct_i_out    = (int)py_oid;   /* Python oid — no ^7 */
	*oct_mode_out = (int)r_mo;

	uint8_t cids[32] = {};
	for (int i = 0; i <= layer; ++i)
		cids[i] = H9_RID2CELL[rids[i]];

	double fx = 0.0, fy = 0.0;
	for (int l = layer - 1; l >= 0; --l) {
		double ox, oy;
		h9a_cid_offset(cids[l + 1], &ox, &oy);
		fx = fx / 3.0 + ox; fy = fy / 3.0 + oy;
	}
	*fx_out = fx; *fy_out = fy;
}

/* Convert BARY-frame (fx, fy) + octant to lon/lat degrees.
 * (fx,fy) from h9_cell_unpack are in the oriented BARY frame — must undo
 * braw_y flip and h9_orient_fwd before applying the b_raw→(u,v,w) formula. */
static void h9_bary_to_lonlat(double fx, double fy, int oct_i, int oct_mode,
								 double *lon, double *lat) {
	/* Step 1: inverse warp (no-op when H9_WARP_ENABLE=0). */
	double bx = fx, by = fy;
#if H9_WARP_ENABLE
	h9_warp_inv(fx, fy, oct_mode, &bx, &by);
#endif
	/* Step 2: undo braw_y flip. */
	const double ry = (oct_mode == 1) ? -by : by;
	/* Step 3: undo orient_fwd rotation → unoriented b_raw (raw_x, raw_y). */
	double raw_x, raw_y;
	h9_orient_inv(bx, ry, oct_i, &raw_x, &raw_y);
	/* Step 4: b_raw → unsigned bary (u,v,w) using raw_y as the height axis. */
	const double inv_H  = H9.inv_H;
	const double inv_W  = H9.inv_W;
	const double inv_2H = H9.inv_2H;
	double au, av, aw;
	if (oct_mode == 1) {
		au = raw_y * inv_H  + (1.0 / 3.0);
		av = (1.0 / 3.0) - raw_y * inv_2H - raw_x * inv_W;
		aw = (1.0 / 3.0) - raw_y * inv_2H + raw_x * inv_W;
	} else {
		au = (1.0 / 3.0) - raw_y * inv_H;
		av = (1.0 / 3.0) + raw_y * inv_2H + raw_x * inv_W;
		aw = (1.0 / 3.0) + raw_y * inv_2H - raw_x * inv_W;
	}
	if (au < 0.0) au = 0.0; if (av < 0.0) av = 0.0; if (aw < 0.0) aw = 0.0;
	const double s = au + av + aw;
	if (s > 1e-30) { au /= s; av /= s; aw /= s; }
	/* Step 5: apply octant signs and convert to lon/lat. */
	const double su = (oct_i & 1) ? -1.0 : 1.0;
	const double sv = (oct_i & 2) ? -1.0 : 1.0;
	const double sw = (oct_i & 4) ? -1.0 : 1.0;
	H9BOct b; b.u = su*au; b.v = sv*av; b.w = sw*aw;
	b.oct_i = oct_i; b.oct_mode = oct_mode;
	h9_boct_to_lonlatdeg(b, lon, lat);
}

/* Geographic centre of a bin UUID cell — used by h9_grid for polygon clipping. */
static void h9_cell_centre_lonlat(const uint8_t uuid[16], int layer,
									 double *lon, double *lat) {
	double fx, fy; int oct_i, oct_mode;
	h9_cell_unpack(uuid, layer, &fx, &fy, &oct_i, &oct_mode);
	h9_bary_to_lonlat(fx, fy, oct_i, oct_mode, lon, lat);
}

/* Recursively densify an edge segment in barycentric (fx, fy) space.
 * At each recursion level, the segment [a..b] is split into 3 equal sub-
 * segments using the 1/3 and 2/3 lattice points — matches HexMesh._get_verts
 * (Python hhg9/h9/grid.py:547). After `depth` levels, the edge contains
 * 3^depth segments and emits 3^depth vertices (start side, end exclusive).
 *
 * NOTE on cross-mode edges: Python's _get_verts handles edges whose end-
 * points sit on different mode faces by negating the mo-0 endpoint's v
 * before interpolating, so all interior vertices live on the mo-1 face.
 * The C++ path projects every interpolated (fx, fy) through the parent
 * hex's (oct_i, oct_mode) frame — same approximation as the corner-only
 * path. For interior hexes (layer ≥ 3) this is exact; for hexes straddling
 * an octant or mode boundary it inherits the same imprecision as the
 * existing corner output (see note on h9_cell below). True cross-mode
 * support would need per-corner (oid, mode) detection and per-segment
 * frame flipping — additional work, not done here. */
static void densify_edge_bary(double fx_a, double fy_a,
                              double fx_b, double fy_b,
                              int depth,
                              double *fx_out, double *fy_out, int *n_out)
{
	if (depth <= 0) {
		fx_out[*n_out] = fx_a;
		fy_out[*n_out] = fy_a;
		(*n_out)++;
		return;
	}
	const double fx_m1 = fx_a +       (fx_b - fx_a) / 3.0;
	const double fy_m1 = fy_a +       (fy_b - fy_a) / 3.0;
	const double fx_m2 = fx_a + 2.0 * (fx_b - fx_a) / 3.0;
	const double fy_m2 = fy_a + 2.0 * (fy_b - fy_a) / 3.0;
	densify_edge_bary(fx_a,  fy_a,  fx_m1, fy_m1, depth - 1, fx_out, fy_out, n_out);
	densify_edge_bary(fx_m1, fy_m1, fx_m2, fy_m2, depth - 1, fx_out, fy_out, n_out);
	densify_edge_bary(fx_m2, fy_m2, fx_b,  fy_b,  depth - 1, fx_out, fy_out, n_out);
}

/* Normalise a closed lon/lat ring across the antimeridian / octant seam and
 * assemble it into a GSERIALIZED POLYGON (SRID 4326). Shifts each vertex's lon
 * by ±360° relative to its predecessor so no consecutive pair differs by more
 * than 180°, then re-centres the ring. Same stopgap as the legacy corner path:
 * the proper split (ST_Split etc.) is projection-dependent and left to the
 * caller. Mutates lons[] in place; does not free the caller's arrays. */
static GSERIALIZED *h9_polygon_from_lonlat(double *lons, double *lats, int n_ring) {
	/* Pairwise-normalise only the unique vertices (exclude the duplicate close
	 * at n_ring-1): a pole-winding ring legitimately accumulates ±360° around
	 * the circuit, so shifting the closing vertex relative to its predecessor
	 * would leave it != vertex 0 and break ring closure. */
	for (int i = 1; i < n_ring - 1; ++i) {
		while (lons[i] - lons[i - 1] >  180.0) lons[i] -= 360.0;
		while (lons[i] - lons[i - 1] < -180.0) lons[i] += 360.0;
	}
	double mean_lon = 0.0;
	for (int i = 0; i < n_ring - 1; ++i) mean_lon += lons[i];  /* exclude duplicate close */
	mean_lon /= (double)(n_ring - 1);
	if      (mean_lon >  180.0) for (int i=0; i<n_ring; ++i) lons[i] -= 360.0;
	else if (mean_lon < -180.0) for (int i=0; i<n_ring; ++i) lons[i] += 360.0;
	/* Close the ring exactly on the normalised vertex 0 (matches
	 * h9_make_geom_from_verts), so the LinearRing is always closed. */
	lons[n_ring - 1] = lons[0];
	lats[n_ring - 1] = lats[0];

	POINTARRAY **rings = (POINTARRAY **) palloc(sizeof(POINTARRAY *));
	POINTARRAY  *pa    = ptarray_construct(false, false, (uint32_t)n_ring);
	rings[0] = pa;
	for (int i = 0; i < n_ring; ++i) {
		POINT4D pt = { lons[i], lats[i], 0.0, 0.0 };
		ptarray_set_point4d(pa, (uint32_t)i, &pt);
	}

	LWPOLY      *poly = lwpoly_construct(4326, NULL, 1, rings);
	GSERIALIZED *g    = gserialized_from_lwgeom((LWGEOM *)poly, NULL);
	lwpoly_free(poly);
	return g;
}

/* Common polygon-assembly body. Takes 6 pre-computed corner (fx, fy) values
 * in the cell's b_oct frame and a single (oct_i, oct_mode) projection frame.
 * Densifies each edge structurally (3^densify segments per edge) and emits
 * the closed POLYGON as a GSERIALIZED. */
static GSERIALIZED *h9_polygon_from_corners(
		const double fxv[6], const double fyv[6],
		int oct_i, int oct_mode, int densify) {

	/* Ring size: 6 edges × 3^densify segments + 1 closing vertex. */
	int v_per_edge = 1;
	for (int i = 0; i < densify; ++i) v_per_edge *= 3;
	const int n_ring = 6 * v_per_edge + 1;

	double *fxs  = (double *) palloc((size_t)n_ring * sizeof(double));
	double *fys  = (double *) palloc((size_t)n_ring * sizeof(double));
	double *lons = (double *) palloc((size_t)n_ring * sizeof(double));
	double *lats = (double *) palloc((size_t)n_ring * sizeof(double));

	/* Walk the 6 edges, densifying each in (fx, fy) space. */
	int n = 0;
	for (int e = 0; e < 6; ++e) {
		const int e_next = (e + 1) % 6;
		densify_edge_bary(fxv[e], fyv[e], fxv[e_next], fyv[e_next],
		                  densify, fxs, fys, &n);
	}
	/* Close the ring: copy first vertex to last position. */
	fxs[n_ring - 1] = fxs[0];
	fys[n_ring - 1] = fys[0];

	/* Project every vertex via the parent hex's (oct_i, oct_mode). */
	for (int i = 0; i < n_ring; ++i) {
		h9_bary_to_lonlat(fxs[i], fys[i], oct_i, oct_mode, &lons[i], &lats[i]);
	}

	GSERIALIZED *g = h9_polygon_from_lonlat(lons, lats, n_ring);

	pfree(fxs);
	pfree(fys);
	pfree(lons);
	pfree(lats);
	return g;
}

/* UUID-driven entry: decode the bin UUID to (fx, fy, oct_i, oct_mode) and
 * compute corners using the symmetric vo[6][2] table around the cell centre.
 * NOTE: bin UUIDs are lossy by design — different bin UUIDs in the same
 * equivalence class decode to the same centre. Use _from_iauv when you have
 * the BFS-stored (oid, c2, ia, ib) — that path is non-lossy. */
static GSERIALIZED *h9_make_cell_geom_dense(const uint8_t uuid[16],
                                            int layer, int densify) {
	double fx, fy; int oct_i, oct_mode;
	h9_cell_unpack(uuid, layer, &fx, &fy, &oct_i, &oct_mode);
	const double du = (H9.W / 6.0) * pow(1.0 / 3.0, (double)layer);
	const double dv = (H9.H / 3.0) * pow(1.0 / 3.0, (double)layer);
	static const int8_t vo[6][2] = {
		{ 2, 0}, { 1, 1}, {-1, 1}, {-2, 0}, {-1,-1}, { 1,-1}
	};
	double fxv[6], fyv[6];
	for (int v = 0; v < 6; ++v) {
		fxv[v] = fx + vo[v][0] * du;
		fyv[v] = fy + vo[v][1] * dv;
	}
	return h9_polygon_from_corners(fxv, fyv, oct_i, oct_mode, densify);
}

/* BFS-driven entry with per-vertex frame switching — the full-fidelity densify
 * used by the h9_grid SRF. Unlike _from_iauv, this honours the BFS's
 * ext/v4/v5 octant-seam reflection: each cell carries per-vertex (pu, pv, poid)
 * so cross-octant edges and half-hex (ext) cells densify correctly instead of
 * projecting their reflected vertices through the cell's own frame.
 *
 * Mirrors Python HexMesh._get_verts (hhg9/h9/grid.py:547-631): each edge is
 * interpolated in a single frame. For an interior edge (poid[e]==poid[e_next])
 * the cell's own frame is used. For a cross-mode edge (the ext v4/v5 case, one
 * mode-0 endpoint and one mode-1 endpoint) the mode-0 endpoint's v is negated
 * to bring it into the mode-1 frame, and every intermediate is projected
 * through the mode-1 (neighbour) oid. The 1/3-recursive subdivision of
 * _get_verts is affine, so we interpolate the endpoint (cx, cy) linearly —
 * the evenly spaced samples land on the same fine-layer lattice points.
 * Projection uses cxcy_to_lonlat (strict=false), matching the BFS that
 * populated H9GridCell.vlon/vlat, so densify endpoints are bit-identical to
 * the densify=0 ring. */
static GSERIALIZED *h9_make_cell_geom_dense_from_cell(
		const H9GridCell &c, int layer, int densify) {
	int v_per_edge = 1;
	for (int i = 0; i < densify; ++i) v_per_edge *= 3;
	const int    n_ring = 6 * v_per_edge + 1;
	const double div_f  = pow(3.0, (double)layer);

	double *lons = (double *) palloc((size_t)n_ring * sizeof(double));
	double *lats = (double *) palloc((size_t)n_ring * sizeof(double));

	int n = 0;
	for (int e = 0; e < 6; ++e) {
		const int e_next = (e + 1) % 6;
		const int oa = c.poid[e],  ob = c.poid[e_next];
		const int mo_a = (int)H9_OID_MO[oa], mo_b = (int)H9_OID_MO[ob];
		int64_t ua = c.pu[e], va = c.pv[e];
		int64_t ub = c.pu[e_next], vb = c.pv[e_next];
		int frame_oid;
		if (oa == ob || mo_a == mo_b) {
			frame_oid = oa;                       /* interior / same-mode edge */
		} else if (mo_a == 0) {
			frame_oid = ob; va = -va;             /* a→mode-1: reflect a's v */
		} else {
			frame_oid = oa; vb = -vb;             /* b→mode-1: reflect b's v */
		}
		const double cxa = (double)ua * H9_UV_U1 / div_f;
		const double cya = (double)va * H9_UV_V3 / div_f;
		const double cxb = (double)ub * H9_UV_U1 / div_f;
		const double cyb = (double)vb * H9_UV_V3 / div_f;
		/* v_per_edge points from start (inclusive) to end (exclusive). */
		for (int s = 0; s < v_per_edge; ++s) {
			const double t  = (double)s / (double)v_per_edge;
			const double cx = cxa + (cxb - cxa) * t;
			const double cy = cya + (cyb - cya) * t;
			h9grid::cxcy_to_lonlat(cx, cy, frame_oid, &lons[n], &lats[n], nullptr, false);
			n++;
		}
	}
	/* Close the ring. */
	lons[n_ring - 1] = lons[0];
	lats[n_ring - 1] = lats[0];

	GSERIALIZED *g = h9_polygon_from_lonlat(lons, lats, n_ring);
	pfree(lons);
	pfree(lats);
	return g;
}

/* Build the hexagonal cell polygon (7-point ring, SRID 4326) from a bin UUID.
 * Called by h9_cell (PG function). Thin wrapper over the densify-aware
 * variant for back-compat. */
static GSERIALIZED *h9_make_cell_geom(const uint8_t uuid[16], int layer) {
	return h9_make_cell_geom_dense(uuid, layer, 0);
}

/* ── h9_cell(uuid, integer) → geometry ──────────────────────────────────── */
/*																			 */
/*	 Returns the hexagonal cell polygon (SRID 4326) for the bin UUID at the	*/
/*	 given layer (1..29).	Pass a bin UUID from h9_bin(uuid, layer).		 */
/*																			 */
/*	 Note: vertices of cells near octant boundaries at coarse layers (< 3)	 */
/*	 may project imprecisely; use layer ≥ 3 for reliable polygons.			 */

/* ── h9_cell(uuid, layer, densify) → geometry ─────────────────────────────── */
/*                                                                             */
/*   Returns the hexagonal cell polygon (SRID 4326) for the bin UUID at the    */
/*   given layer (1..29). The third arg `densify` is an optional non-negative  */
/*   offset (default 0 via SQL): each of the 6 hex edges is subdivided into    */
/*   3^densify segments using the recursive 1/3-step rule from                 */
/*   HexMesh._get_verts. Output ring size: 6·3^densify + 1 points.             */
/*                                                                             */
/*       densify = 0 → 7 points (corners only — same as old h9_cell).          */
/*       densify = 1 → 19 points; densify = 2 → 55; densify = 3 → 163.         */
/*                                                                             */
/*   Use case for densify > 0: rendering large hexes (layer ≤ 3) where         */
/*   straight-line edges in (lon, lat) visibly diverge from the underlying     */
/*   great-circle / lattice geometry. For layer ≥ 5 it's rarely needed.        */
/*                                                                             */
/*   Caveat: vertices near octant boundaries inherit the same approximation    */
/*   as the corner-only output (parent-frame projection — see comment on       */
/*   h9_make_cell_geom_dense for the cross-mode limitation).                   */

extern "C" {
PG_FUNCTION_INFO_V1(h9_cell);
Datum h9_cell(PG_FUNCTION_ARGS) {
	if (PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2)) PG_RETURN_NULL();
	pg_uuid_t *u       = PG_GETARG_UUID_P(0);
	int32      layer   = PG_GETARG_INT32(1);
	int32      densify = PG_GETARG_INT32(2);
	if (layer < 1 || layer > 29)
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("h9_cell: layer must be 1..29, got %d", layer)));
	if (densify < 0)
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("h9_cell: densify must be >= 0, got %d", densify)));
	if (layer + densify > 29)
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("h9_cell: layer + densify must be <= 29 "
						       "(layer=%d, densify=%d -> %d)",
						       layer, densify, layer + densify)));
	/* Hard cap to keep ring sizes sane (6·3^densify+1 = 118099 at densify=9). */
	if (densify > 9)
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("h9_cell: densify must be <= 9, got %d", densify)));
	PG_RETURN_POINTER(h9_make_cell_geom_dense(u->data, layer, densify));
}
} /* extern "C" */

/* ── h9_label helpers ────────────────────────────────────────────────────── */

/*
 * Write body nibbles[0..layer] as a hex string into buf[].
 * Digits 0..9 → '0'..'9', 10 → 'a', 11 → 'b'.
 * Returns the number of characters written (= layer + 1).
 */
static int h9_write_label(const uint8_t nibbles[32], int layer, char *buf) {
	static const char H9D[] = "0123456789ab";
	for (int i = 0; i <= layer; ++i)
		buf[i] = H9D[nibbles[i] & 0x0Fu];
	return layer + 1;
}

/* ── h9_label(uuid, integer) → text ─────────────────────────────────────── */
/*																			 */
/*	 Returns the body nibbles at the given layer as a plain text label.		 */
/*	 Example: h9_label(h9_bin(h9_encode(pt), 8), 8) → '478232778'			 */

extern "C" {
PG_FUNCTION_INFO_V1(h9_label);
Datum h9_label(PG_FUNCTION_ARGS) {
	if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) PG_RETURN_NULL();

	pg_uuid_t *u	 = PG_GETARG_UUID_P(0);
	int32		layer = PG_GETARG_INT32(1);

	if (layer < 0 || layer > 29)
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("h9_label: layer must be 0..29, got %d", layer)));

	uint8_t nibbles[32];
	h9a_unpack(u->data, nibbles);

	char buf[32];
	int	len = h9_write_label(nibbles, layer, buf);
	PG_RETURN_TEXT_P(cstring_to_text_with_len(buf, len));
}
} /* extern "C" */

/* ── h9_label_key(uuid, integer) → text ─────────────────────────────────── */
/*																			 */
/*	 Returns the label with the key_tail nibble appended as '.X'.			 */
/*	 Example: h9_label_key(h9_encode(pt), 8) → '478232778.9'				*/

extern "C" {
PG_FUNCTION_INFO_V1(h9_label_key);
Datum h9_label_key(PG_FUNCTION_ARGS) {
	if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) PG_RETURN_NULL();

	pg_uuid_t *u	 = PG_GETARG_UUID_P(0);
	int32		layer = PG_GETARG_INT32(1);

	if (layer < 0 || layer > 29)
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("h9_label_key: layer must be 0..29, got %d", layer)));

	uint8_t nibbles[32];
	h9a_unpack(u->data, nibbles);

	/* Compute bin key_tail at `layer`.	nibbles[31] holds the key_tail for the
	 * deepest level (L29 for full UUIDs, L for bin UUIDs), so we must run the
	 * same backward pass that h9_bin_uuid uses to derive the context at layer L. */
	uint8_t r_mo = nibbles[31] & 1u;
	uint8_t c_mo = (nibbles[31] >> 3) & 1u;
	uint8_t c2	 = (nibbles[31] >> 1) & 3u;
	if (nibbles[30] != 0x0Fu) {
		/* Full UUID: backward pass L29→L+1 using the valid body nibbles. */
		for (int l = 29; l >= layer + 1; --l) {
			const uint8_t *e = H9_HEX_REG[nibbles[l]][c_mo][c2];
			c_mo = e[1]; c2 = e[2];
		}
	}
	/* Bin UUID: nibbles[31] already holds the bin key_tail for its layer. */
	const uint8_t bin_key_tail = (uint8_t)((c_mo << 3) | (c2 << 1) | r_mo);

	char buf[35];	 /* 30 body + '.' + 1 key nibble + NUL */
	int	len = h9_write_label(nibbles, layer, buf);
	buf[len++] = '.';
	static const char H9D[] = "0123456789abcdef";
	buf[len++] = H9D[bin_key_tail & 0x0Fu];
	PG_RETURN_TEXT_P(cstring_to_text_with_len(buf, len));
}
} /* extern "C" */

/* Build a closed hex polygon (SRID 4326) directly from per-vertex lon/lat
 * data carried by H9GridCell.  Avoids the lossy decode that
 * h9_make_cell_geom(uuid, layer) does — vertices here are the *exact* ones
 * the BFS pool produced, so adjacent cells share edges by construction. */
static GSERIALIZED *h9_make_geom_from_verts(const H9GridCell &c) {
	POINTARRAY **rings = (POINTARRAY **) palloc(sizeof(POINTARRAY *));
	POINTARRAY  *pa    = ptarray_construct(false, false, 7);
	rings[0] = pa;

	/* Normalise longitudes to avoid antimeridian-wrap self-intersection.
	 * Same stopgap logic as h9_make_cell_geom — pairwise shift each
	 * vertex's lon by ±360° so no consecutive pair differs by more than
	 * 180°, then re-centre the ring so its mean longitude lies in
	 * [-180,+180] when possible. See the TODO C in h9_make_cell_geom
	 * for the long-term proper antimeridian-split fix. */
	double lons[6], lats[6];
	for (int v = 0; v < 6; ++v) { lons[v] = c.vlon[v]; lats[v] = c.vlat[v]; }
	for (int v = 1; v < 6; ++v) {
		while (lons[v] - lons[v - 1] >  180.0) lons[v] -= 360.0;
		while (lons[v] - lons[v - 1] < -180.0) lons[v] += 360.0;
	}
	double mean_lon = 0.0;
	for (int v = 0; v < 6; ++v) mean_lon += lons[v];
	mean_lon /= 6.0;
	if      (mean_lon >  180.0) for (int v=0; v<6; ++v) lons[v] -= 360.0;
	else if (mean_lon < -180.0) for (int v=0; v<6; ++v) lons[v] += 360.0;
	for (int v = 0; v < 6; ++v) {
		POINT4D pt = { lons[v], lats[v], 0.0, 0.0 };
		ptarray_set_point4d(pa, v, &pt);
	}
	POINT4D first; getPoint4d_p(pa, 0, &first);
	ptarray_set_point4d(pa, 6, &first);
	LWPOLY      *poly = lwpoly_construct(4326, NULL, 1, rings);
	GSERIALIZED *g    = gserialized_from_lwgeom((LWGEOM *)poly, NULL);
	lwpoly_free(poly);
	return g;
}

/* ── h9_grid(layer, bounds) → TABLE(hex9 uuid, geom geometry) ───────────── */
/*																			 */
/*	 Enumerates every H9 cell at layer L whose centre lies within bounds.	 */
/*																			 */
/*	 Algorithm (HexMesh integer lattice):										*/
/*	 1. Convert bbox corners → H9BOct (beam search, ~4–8 calls total).		*/
/*	 2. Per octant oct_i: convert (fx,fy) range → integer (ia,ib) range.	*/
/*		ia_step = U/3^L,	ib_step = V3/3^L	(V3 = H/3 = 3V).				*/
/*		Valid hex centres satisfy: parity(ia)==parity(ib) && ib%3 != 0.	*/
/*		This is the same integer lattice used by HexMesh.create() in the	*/
/*		Python library — exact enumeration, no sampling, no duplication.	*/
/*	 3. For each valid (ia,ib): reconstruct H9BOct cheaply (no beam search) */
/*		via h9_boct_from_braw, encode to bin UUID, collect.				 */
/*	 4. Sort + deduplicate (handles seam cells shared by adjacent octants). */
/*	 5. Per call: centre-point test via lwpoly_contains_point; yield tuple. */
/*																			 */
/*	 Safety cap: hex9.grid_max_cells GUC.  Default 708 588 = 12 × 9⁵        */
/*	 (exactly one global L5 mesh — generous for ordinary local queries,	   */
/*	 lets a coarse global pass without override).							 */
/*	 (Forward declaration + GUC registration in _PG_init above; this is	   */
/*	 the tentative-definition init value.)								   */

static int h9_uuid16_cmp(const void *a, const void *b) {
	return memcmp(a, b, 16);
}

/* Point-in-polygon test using liblwgeom (no GEOS symbols needed).
 * Handles POLYGON and MULTIPOLYGON; passes through for other types. */
static bool h9_lwgeom_contains_pt(const LWGEOM *geom, const POINT2D *pt) {
	switch (geom->type) {
	case POLYGONTYPE:
		/* LW_INSIDE=1, LW_BOUNDARY=0, LW_OUTSIDE=-1.
		 * '>= LW_FALSE(=0)' passes INSIDE and BOUNDARY, rejects OUTSIDE. */
		return lwpoly_contains_point((const LWPOLY *)geom, pt) >= LW_FALSE;
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE: {
		const LWCOLLECTION *col = (const LWCOLLECTION *)geom;
		for (uint32_t i = 0; i < col->ngeoms; i++)
			if (h9_lwgeom_contains_pt(col->geoms[i], pt)) return true;
		return false;
	}
	default:
		return true;	 /* non-polygon bounds: let through, bbox already filtered */
	}
}

struct H9GridState {
	int          layer;
	int          densify;        /* per-row polygon densify offset (0 = legacy) */
	int          count;
	int          idx;
	H9GridCell  *cells;          /* palloc'd array */
	LWGEOM      *bounds_lwg;
	GBOX         bounds_box;
};

/* Legacy ClipNode struct kept only for h9_grid_prx below.  The active
 * h9_grid SRF now delegates enumeration to h9grid::enumerate in h9_grid.h. */
struct ClipNode { int mode; int64_t ia, ib, scale; };

extern "C" {
PG_FUNCTION_INFO_V1(h9_grid);
Datum h9_grid(PG_FUNCTION_ARGS) {
	FuncCallContext *funcctx;
	H9GridState     *state;

	if (SRF_IS_FIRSTCALL()) {
		MemoryContext oldctx;
		funcctx = SRF_FIRSTCALL_INIT();
		oldctx  = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* Argument order: (bounds geometry, layer integer, densify integer).
		 * densify always present at the C level — SQL CREATE FUNCTION uses
		 * DEFAULT 0 so callers can write either h9_grid(g, L) or
		 * h9_grid(g, L, k). */
		GSERIALIZED *bounds_raw = PG_GETARG_GSERIALIZED_P(0);
		int32        layer      = PG_GETARG_INT32(1);
		int32        densify    = PG_GETARG_INT32(2);

		if (layer < 1 || layer > 29)
			ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							errmsg("h9_grid: layer must be 1..29, got %d", layer)));
		if (densify < 0)
			ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							errmsg("h9_grid: densify must be >= 0, got %d", densify)));
		if (layer + densify > 29)
			ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							errmsg("h9_grid: layer + densify must be <= 29 "
							       "(layer=%d, densify=%d -> %d)",
							       layer, densify, layer + densify)));
		if (densify > 9)
			ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							errmsg("h9_grid: densify must be <= 9, got %d", densify)));

		GSERIALIZED *bounds = (GSERIALIZED *) palloc(VARSIZE(bounds_raw));
		memcpy(bounds, bounds_raw, VARSIZE(bounds_raw));

		/* Get the bbox from the LWGEOM's actual float8 coords, NOT the
		 * cached float32 header bbox.  The cached bbox has ULP ~ |coord|·2^-23,
		 * which at lat 51.5° is ~6 µdeg — way larger than micro-bbox queries.
		 * E.g., 2e-9° span at L24 rounds OUTWARD to ~6 µdeg, inflating the
		 * area estimate by ~3000× and triggering false overflow rejections
		 * (saw "estimated 169048 cells at layer 24" for an actual-111-cell bbox). */
		LWGEOM *bounds_lwg = lwgeom_from_gserialized(bounds);
		GBOX   gbox;
		if (lwgeom_calculate_gbox(bounds_lwg, &gbox) != LW_SUCCESS)
			ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							errmsg("h9_grid: could not determine bounds extent")));

		/* Safety cap: spherical-zone area for the bbox over mean H9 cell
		 * area at this layer, plus a perimeter term for partial-coverage
		 * cells at the bbox edge.
		 *
		 * The previous flat-rectangle approximation (Δlat·Δlon·M_PER_DEG²
		 * with a cos(mid_lat) longitude correction) over-counted by ~58%
		 * for near-global bboxes — at -180..180 × -89..89, cos(0)=1 gives
		 * 7.94e14 m² vs the true sphere area 5.10e14 m².  The zone formula
		 *
		 *   A = R²·(sinφ₂ − sinφ₁)·(λ₂ − λ₁)
		 *
		 * is exact for spherical zones and reduces to the flat formula
		 * for tiny bboxes (sin Δφ ≈ Δφ near the equator).
		 *
		 * Verified against actual CLI counts (no change at small bboxes):
		 *     L23 micro-bbox (2e-9° × 3e-9°): predicts ≈22, actual 10
		 *     L24 same bbox:                  predicts ≈124, actual 111
		 *     L13 Greenwich (0.018° × 0.011°): predicts ≈115k, actual 109k
		 *     L5  global (-180..180,-89..89):  predicts ≈712k, actual 708,588
		 *                                      (was 1,119,576 under flat). */
		const double EARTH_R_M     = 6378137.0;
		const double EARTH_AREA_M2 = 5.10072e14;
		const double D2R           = M_PI / 180.0;
		const double sin_lat_hi    = sin(gbox.ymax * D2R);
		const double sin_lat_lo    = sin(gbox.ymin * D2R);
		const double dlon_rad      = (gbox.xmax - gbox.xmin) * D2R;
		const double bbox_area_m2  = EARTH_R_M * EARTH_R_M
		                             * fabs(sin_lat_hi - sin_lat_lo)
		                             * fabs(dlon_rad);
		const double cell_area_m2  = EARTH_AREA_M2 / (12.0 * pow(9.0, (double)layer));
		const long   est_interior  = (long)(bbox_area_m2 / cell_area_m2);
		const long   est_perimeter = (long)ceil(4.0 * sqrt(bbox_area_m2) / sqrt(cell_area_m2));
		const long   est_cells     = est_interior + est_perimeter + 1;
		if (est_cells > h9_grid_max_cells)
			ereport(ERROR,
				(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
				 errmsg("h9_grid: estimated %ld cells at layer %d exceeds "
						"limit %d; use a coarser layer or smaller bounds",
						est_cells, layer, h9_grid_max_cells)));

		const double lon_min = gbox.xmin, lon_max = gbox.xmax;
		const double lat_min = gbox.ymin, lat_max = gbox.ymax;

		/* Hand off to the standalone enumerator (h9_grid.h).  Integer-UV
		 * supercell BFS with strict octant-region pruning + v4/v5 seam
		 * reflection — same algorithm Python HexMesh.create_clipped uses.
		 * UUIDs are derived via the containment-based xy_regions encoder. */
		std::vector<H9GridCell> tmp;
		h9grid::enumerate(layer, lon_min, lat_min, lon_max, lat_max, tmp);

		/* Polygon-containment filter on the centroid (matches Python's
		 * matplotlib Path test in create_clipped).  Skipped when bounds is
		 * not a polygon — the bbox prune inside enumerate() already applied. */
		size_t kept = 0;
		for (size_t i = 0; i < tmp.size(); i++) {
			POINT2D pt = { tmp[i].cen_lon, tmp[i].cen_lat };
			if (h9_lwgeom_contains_pt(bounds_lwg, &pt)) {
				if (kept != i) tmp[kept] = tmp[i];
				kept++;
			}
		}
		tmp.resize(kept);

		/* Copy into palloc'd memory so the SRF can iterate after `tmp` goes
		 * out of scope at the end of this FIRSTCALL block. */
		H9GridCell *cells = NULL;
		if (kept > 0) {
			cells = (H9GridCell *) palloc(kept * sizeof(H9GridCell));
			memcpy(cells, tmp.data(), kept * sizeof(H9GridCell));
		}

		/* Legacy in-line enumeration removed; h9grid::enumerate above produced
		 * the cell list.  All the dead code (sampler + per-octant BFS) was
		 * moved into h9_grid.h.  See h9_grid_prx below for the original
		 * BFS port if needed for diagnostics. */
		#if 0  /* ── legacy enumeration (kept for diff against h9_grid_prx) ── */
		const double div_f = pow(3.0, (double)layer);

		const int MAX_Q = 300000;   /* per-octant queue limit */
		ClipNode *q_cur = (ClipNode *) palloc(MAX_Q * sizeof(ClipNode));
		ClipNode *q_nxt = (ClipNode *) palloc(MAX_Q * sizeof(ClipNode));

		const int raw_cap = (est_cells < 50000) ? 250025 : (h9_grid_max_cells + 25);
		uint8_t (*raw)[16] = (uint8_t (*)[16]) palloc(raw_cap * 16);
		int raw_n = 0;

		if (est_cells < 50000) {
			/* Small query: direct 500×500 sampling matches the Python reference's
			 * point-sampling methodology exactly, bypassing the BFS which misses
			 * ~101 cells near face seams for tiny queries (lon≈0° boundary).
			 * No polygon filter here: all sample points are within the bbox by
			 * construction, and h9_lwgeom_contains_pt uses strict interior which
			 * would exclude boundary points (matching Python's inclusive sampling). */
			const int nx = 500, ny = 500;
			for (int iy = 0; iy < ny && raw_n < raw_cap - 1; iy++) {
				double lat = lat_min + (lat_max - lat_min) * iy / (double)(ny - 1);
				for (int ix = 0; ix < nx && raw_n < raw_cap - 1; ix++) {
					double lon = lon_min + (lon_max - lon_min) * ix / (double)(nx - 1);
					H9BOct bc = h9_lonlatdeg_to_boct(lon, lat);
					uint8_t full_uuid[16], bin_uuid[16];
					h9_boct_to_uuid(bc, full_uuid);
					h9_bin_uuid(full_uuid, layer, bin_uuid);
					memcpy(raw[raw_n++], bin_uuid, 16);
				}
			}
		} else {

		for (int oid = 0; oid < 8; oid++) {
			const int oid_mode = (int)H9_OID_MO[oid];  /* Python oid == oct_i */
			const int levels   = layer - 1;             /* 0 for layer=1 */

			/* scale0 = 3^(levels) = 3^(layer-1) */
			int64_t scale0 = 1;
			for (int i = 0; i < levels; i++) scale0 *= 3;

			/* Child offset tables (U already multiplied by 3). */
			const int (*ofs0)[2] = H9_MODE0_OFS;
			const int (*ofs1)[2] = H9_MODE1_OFS;
			const int *cm0 = H9_MODE0_CHILD_MODE;
			const int *cm1 = H9_MODE1_CHILD_MODE;
			const int (*ofs_init)[2] = (oid_mode == 0) ? ofs0 : ofs1;
			const int *cm_init       = (oid_mode == 0) ? cm0  : cm1;

			/* Seed: 9 mode-0 or mode-1 children of the octant root. */
			int q_n = 0;
			for (int k = 0; k < H9_N_CHILDREN; k++) {
				q_cur[q_n++] = { cm_init[k],
				                 (int64_t)ofs_init[k][0] * scale0,
				                 (int64_t)ofs_init[k][1] * scale0,
				                 scale0 };
			}

			/* BFS: at each depth filter by geographic footprint, then expand. */
			const int64_t s_int = (int64_t)llround(div_f);  /* 3^layer */

			auto accum_vert = [&](int64_t pu, int64_t pv_v, int proj_oid,
			                      double &lon_min, double &lon_max,
			                      double &lat_min, double &lat_max,
			                      bool &has_pt) {
				double cx = pu   * H9_UV_U1 / div_f;
				double cy = pv_v * H9_UV_V3 / div_f;
				double fx, fy;
				h9_orient_fwd(cx, cy, proj_oid, &fx, &fy);
				double vlon, vlat;
				h9_bary_to_lonlat(fx, fy, proj_oid, (int)H9_OID_MO[proj_oid], &vlon, &vlat);
				if (isnan(vlon) || isnan(vlat)) return;
				has_pt = true;
				if (vlon < lon_min) lon_min = vlon;
				if (vlon > lon_max) lon_max = vlon;
				if (vlat < lat_min) lat_min = vlat;
				if (vlat > lat_max) lat_max = vlat;
			};

			for (int depth = 0; depth <= levels; depth++) {
				/* Filter: keep nodes whose 18 outer-vertex footprint overlaps bbox.
				 * Vertices 4 and 5 of each c2 hex may cross the octant seam;
				 * mirror _assemble_clipped's reflection logic for those. */
				int new_n = 0;
				for (int qi = 0; qi < q_n; qi++) {
					const ClipNode &nd = q_cur[qi];
					double vlon_min = 1e30, vlon_max = -1e30;
					double vlat_min = 1e30, vlat_max = -1e30;
					bool   has_pt   = false;
					for (int c2 = 0; c2 < H9_N_C2; c2++) {
						int64_t pu[7], pv_c[7];
						for (int v = 0; v < 7; v++) {
							pu[v]   = (int64_t)H9_HI[nd.mode][c2][v][0] * nd.scale + nd.ia;
							pv_c[v] = (int64_t)H9_HI[nd.mode][c2][v][1] * nd.scale + nd.ib;
						}
						/* Vertices 0-5 (outer ring) + vertex 6 (centroid).
						 * Including the centroid helps near face seams where outer
						 * vertices project to wrong geographic positions (clamped)
						 * but the centroid projects correctly near the query. */
						for (int v = 0; v < 7; v++)
							accum_vert(pu[v], pv_c[v], oid,
							           vlon_min, vlon_max, vlat_min, vlat_max, has_pt);
					}
					/* Conservative: if no valid vertices project, keep the node.
					 * Small pad captures boundary supercells whose vertices barely
					 * miss the query box but have leaf hexes inside. */
					const double bfs_pad = 0.002;
					bool overlaps = !has_pt ||
					                (vlon_min <= lon_max + bfs_pad && vlon_max >= lon_min - bfs_pad &&
					                 vlat_min <= lat_max + bfs_pad && vlat_max >= lat_min - bfs_pad);
					if (overlaps) { q_cur[new_n++] = nd; }
				}
				q_n = new_n;

				if (depth == levels || q_n == 0) break;  /* done */

				/* Expand surviving nodes into their 9 children. */
				int nxt_n = 0;
				for (int qi = 0; qi < q_n; qi++) {
					const ClipNode &nd = q_cur[qi];
					const int64_t   ks  = nd.scale / 3;
					const int (*ofs_nd)[2] = (nd.mode == 0) ? ofs0 : ofs1;
					const int *cm_nd       = (nd.mode == 0) ? cm0  : cm1;
					for (int k = 0; k < H9_N_CHILDREN; k++) {
						if (nxt_n >= MAX_Q) { nxt_n = MAX_Q - 1; break; } /* safety */
						q_nxt[nxt_n++] = { cm_nd[k],
						                   nd.ia + (int64_t)ofs_nd[k][0] * ks,
						                   nd.ib + (int64_t)ofs_nd[k][1] * ks,
						                   ks };
					}
				}
				/* Swap buffers. */
				ClipNode *tmp = q_cur; q_cur = q_nxt; q_nxt = tmp;
				q_n = nxt_n;
			}

			/* oct_mode and sign conventions are constant per octant. */
			const int oct_mode = (int)H9_OID_MO[oid];
			const double su_s = (oid & 1) ? -1.0 : 1.0;
			const double sv_s = (oid & 2) ? -1.0 : 1.0;
			const double sw_s = (oid & 4) ? -1.0 : 1.0;

			/* Project integer UV lattice coords → geographic lon/lat via b_oct pipeline.
			 * Optionally returns the H9BOct struct (for UUID generation from centroid). */
			auto uv_to_lonlat = [&](int64_t pu, int64_t pv,
			                        double *lon, double *lat,
			                        H9BOct *bc_out = nullptr) -> bool {
				double cx_ = pu * H9_UV_U1 / div_f;
				double cy_ = pv * H9_UV_V3 / div_f;
				double ox_, oy_;
				h9_orient_fwd(cx_, cy_, oid, &ox_, &oy_);
				double bx_ = ox_, by_ = oy_;
#if H9_WARP_ENABLE
				h9_warp_inv(ox_, oy_, oct_mode, &bx_, &by_);
#endif
				double ry_f_ = (oct_mode == 1) ? -by_ : by_;
				double rx_, ry_;
				h9_orient_inv(bx_, ry_f_, oid, &rx_, &ry_);
				double au_, av_, aw_;
				if (oct_mode == 1) {
					au_ = ry_ * H9.inv_H  + (1.0/3.0);
					av_ = (1.0/3.0) - ry_ * H9.inv_2H - rx_ * H9.inv_W;
					aw_ = (1.0/3.0) - ry_ * H9.inv_2H + rx_ * H9.inv_W;
				} else {
					au_ = (1.0/3.0) - ry_ * H9.inv_H;
					av_ = (1.0/3.0) + ry_ * H9.inv_2H + rx_ * H9.inv_W;
					aw_ = (1.0/3.0) + ry_ * H9.inv_2H - rx_ * H9.inv_W;
				}
				if (au_ < 0.0) au_ = 0.0;
				if (av_ < 0.0) av_ = 0.0;
				if (aw_ < 0.0) aw_ = 0.0;
				double norm_ = au_ + av_ + aw_;
				if (norm_ < 1e-30) return false;
				au_ /= norm_; av_ /= norm_; aw_ /= norm_;
				H9BOct bc_;
				bc_.u = su_s * au_; bc_.v = sv_s * av_; bc_.w = sw_s * aw_;
				bc_.oct_i = oid; bc_.oct_mode = oct_mode;
				h9_boct_to_lonlatdeg(bc_, lon, lat);
				if (bc_out) *bc_out = bc_;
				return !isnan(*lon) && !isnan(*lat);
			};

			/* Assemble hex cells from mode-0 leaf nodes only.
			 * Use SAT (Separating Axis Theorem) for exact convex hex ∩ bbox test:
			 * catches edge-straddle cells missed by centroid/vertex-only checks. */

			/* SAT helper: check if hex (convex, n vertices) intersects axis-aligned bbox. */
			auto hex_intersects_bbox = [&](double *hlon, double *hlat, int n,
			                               double clon, double clat) -> bool {
				/* Step 1: hex bbox overlaps query bbox (rectangle-axis separation test). */
				double hlon_min = hlon[0], hlon_max = hlon[0];
				double hlat_min = hlat[0], hlat_max = hlat[0];
				for (int i = 1; i < n; i++) {
					if (hlon[i] < hlon_min) hlon_min = hlon[i];
					if (hlon[i] > hlon_max) hlon_max = hlon[i];
					if (hlat[i] < hlat_min) hlat_min = hlat[i];
					if (hlat[i] > hlat_max) hlat_max = hlat[i];
				}
				if (hlon_max < lon_min || hlon_min > lon_max) return false;
				if (hlat_max < lat_min || hlat_min > lat_max) return false;

				/* Step 2: hex-edge-normal separation test (SAT for convex shapes).
				 * For each hex edge, check if all 4 bbox corners are strictly
				 * outside (on the opposite side from the centroid). */
				const double bcx[4] = {lon_min, lon_max, lon_max, lon_min};
				const double bcy[4] = {lat_min, lat_min, lat_max, lat_max};
				for (int i = 0; i < n; i++) {
					int j = (i + 1) % n;
					double ex = hlon[j] - hlon[i];
					double ey = hlat[j] - hlat[i];
					/* Cross product sign for centroid determines "inside" direction. */
					double csign = ex * (clat - hlat[i]) - ey * (clon - hlon[i]);
					if (csign == 0.0) continue;
					bool all_outside = true;
					for (int ci = 0; ci < 4 && all_outside; ci++) {
						double ps = ex * (bcy[ci] - hlat[i]) - ey * (bcx[ci] - hlon[i]);
						if ((ps >= 0.0) == (csign > 0.0)) all_outside = false;
					}
					if (all_outside) return false; /* separated by this hex edge */
				}
				return true;
			};

			for (int qi = 0; qi < q_n; qi++) {
				if (q_cur[qi].mode != 0) continue;
				const int nd_mode = q_cur[qi].mode;
				const int64_t ia_org = q_cur[qi].ia;
				const int64_t ib_org = q_cur[qi].ib;
				for (int c2 = 0; c2 < H9_N_C2; c2++) {
					int64_t cen_u = (int64_t)H9_HI[nd_mode][c2][6][0] + ia_org;
					int64_t cen_v = (int64_t)H9_HI[nd_mode][c2][6][1] + ib_org;
					double clon, clat;
					H9BOct bc;
					if (!uv_to_lonlat(cen_u, cen_v, &clon, &clat, &bc)) continue;
					/* Skip cells far from query: prevents anti-meridian octants (e.g.
					 * oid=3 at lon≈±180) from falsely matching via lon wraparound. */
					if (clon < lon_min - 1.0 || clon > lon_max + 1.0 ||
					    clat < lat_min - 1.0 || clat > lat_max + 1.0) continue;

					/* Collect the 6 hex corner vertices in geographic space. */
					double hlon[6], hlat[6];
					int n_verts = 0;
					for (int v = 0; v < 6; v++) {
						int64_t vu = (int64_t)H9_HI[nd_mode][c2][v][0] + ia_org;
						int64_t vv = (int64_t)H9_HI[nd_mode][c2][v][1] + ib_org;
						if (uv_to_lonlat(vu, vv, &hlon[n_verts], &hlat[n_verts]))
							n_verts++;
					}

					bool in_poly = (n_verts >= 3) &&
					               hex_intersects_bbox(hlon, hlat, n_verts, clon, clat);
					/* Also accept if centroid is inside (handles degenerate projections). */
					if (!in_poly) {
						POINT2D pt = {clon, clat};
						in_poly = h9_lwgeom_contains_pt(bounds_lwg, &pt);
					}

					if (!in_poly) continue;
					/* Use canonical re-encoding for correct oid near seam. */
					H9BOct true_bc = h9_lonlatdeg_to_boct(clon, clat);
					uint8_t full_uuid[16], bin_uuid[16];
					h9_boct_to_uuid(true_bc, full_uuid);
					h9_bin_uuid(full_uuid, layer, bin_uuid);
					if (raw_n < raw_cap - 1)
						memcpy(raw[raw_n++], bin_uuid, 16);
				}
			}
		} /* oid loop */

		pfree(q_cur); pfree(q_nxt);
		} /* else: large query BFS */

		/* Sort and deduplicate (handles seam cells touching multiple octants). */
		qsort(raw, raw_n, 16, h9_uuid16_cmp);
		int unique = 0;
		for (int i = 0; i < raw_n; i++)
			if (i == 0 || memcmp(raw[i], raw[i-1], 16) != 0)
				memcpy(raw[unique++], raw[i], 16);
		#endif  /* legacy enumeration */

		state              = (H9GridState *) palloc(sizeof(H9GridState));
		state->layer       = layer;
		state->densify     = densify;
		state->count       = (int)kept;
		state->idx         = 0;
		state->cells       = cells;
		state->bounds_lwg  = bounds_lwg;
		state->bounds_box  = gbox;
		funcctx->user_fctx = state;

		get_call_result_type(fcinfo, NULL, &funcctx->tuple_desc);
		BlessTupleDesc(funcctx->tuple_desc);

		MemoryContextSwitchTo(oldctx);
	}

	funcctx = SRF_PERCALL_SETUP();
	state   = (H9GridState *) funcctx->user_fctx;

	while (state->idx < state->count) {
		const H9GridCell &c = state->cells[state->idx++];

		Datum values[3];
		bool  isnull[3] = {false, false, false};
		values[0] = UUIDPGetDatum(make_pg_uuid(c.uuid));
		/* densify=0 → fast path using pre-computed BFS corner verts;
		 * densify>0 → structural densification from per-vertex BFS state
		 * (pu, pv, poid) — honours ext/octant-seam reflection so cross-octant
		 * edges and half-hex cells densify without gaps. NOT from the bin UUID,
		 * which is lossy. */
		values[1] = PointerGetDatum(
			state->densify > 0
				? h9_make_cell_geom_dense_from_cell(c, state->layer, state->densify)
				: h9_make_geom_from_verts(c));
		/* Cell centroid (POINT, SRID 4326) — already projected by the BFS,
		 * so it's the geometric centre in the cell's own frame, not the
		 * mean of the rendered polygon's vertices. */
		values[2] = PointerGetDatum(lonlat_to_geom(c.cen_lon, c.cen_lat));
		HeapTuple tup = heap_form_tuple(funcctx->tuple_desc, values, isnull);
		SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tup));
	}

	if (state->bounds_lwg) lwgeom_free(state->bounds_lwg);
	SRF_RETURN_DONE(funcctx);
}
} /* extern "C" */


/* Legacy proxy enumerator — never registered with PG (see commented
 * PG_FUNCTION_INFO_V1 below).  Kept for development diffing against the
 * active h9_grid; references the old `state->uuids` field, so wrapped
 * out of the build.  Re-enable by removing the `#if 0` and rewiring it
 * to the H9GridCell-based H9GridState (or replacing with a thin shim
 * that just calls h9grid::enumerate the same way h9_grid does). */
#if 0
extern "C" {
//PG_FUNCTION_INFO_V1(h9_grid);
Datum h9_grid_prx(PG_FUNCTION_ARGS) {
	FuncCallContext *funcctx;
	H9GridState	 *state;

	if (SRF_IS_FIRSTCALL()) {
	MemoryContext oldctx;
	funcctx = SRF_FIRSTCALL_INIT();
	oldctx = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

	int32	layer	 = PG_GETARG_INT32(0);
	GSERIALIZED *bounds_raw = PG_GETARG_GSERIALIZED_P(1);

	if (layer < 1 || layer > 29)
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("h9_grid: layer must be 1..29, got %d", layer)));

	GSERIALIZED *bounds = (GSERIALIZED *) palloc(VARSIZE(bounds_raw));
	memcpy(bounds, bounds_raw, VARSIZE(bounds_raw));

	GBOX gbox;
	if (!gserialized_get_gbox_p(bounds, &gbox))
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("h9_grid: could not determine bounds extent")));

	/* Safety cap (flat degrees² approximation). */
	const double cell_area_deg2 = 64800.0 / (12.0 * pow(9.0, (double)layer));
	const double bbox_area = (gbox.xmax - gbox.xmin) * (gbox.ymax - gbox.ymin);
	const long	est_cells = (long)(bbox_area / cell_area_deg2) + 1;
	if (est_cells > h9_grid_max_cells)
		ereport(ERROR,
		(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
		errmsg("h9_grid: estimated %ld cells at layer %d exceeds "
			"limit %d; use a coarser layer or smaller bounds",
			est_cells, layer, h9_grid_max_cells)));

	/* Integer UV lattice constants at this layer.
	*	rx = ia * U1 / 3^layer	(U1 = W/6)
	*	ry = ib * V3 / 3^layer	(V3 = H/3 = 3V)
	* Valid hex centres: (ia & 1) == (ib & 1) && ib % 3 != 0 */
	const double pow3L = pow(3.0, (double)(layer - 1));
	const double U1	= H9.W / 6.0;	 /* U = sqrt(2)/6 — hex lattice x-step */
	const double V1	= H9.H / 9.0;	 /* V = sqrt(6)/18 — hex lattice y-step */
	const double ia_step = U1 / pow3L;
	const double ib_step = V1 / pow3L;

	/* Bbox sample corners: 4 corners + 4 edge midpoints to handle
	* bboxes that cross octant boundaries. */
//	const double lon_min = gbox.xmin, lon_max = gbox.xmax;
//	const double lat_min = gbox.ymin, lat_max = gbox.ymax;
//	const double lon_mid = (lon_min + lon_max) * 0.5;
//	const double lat_mid = (lat_min + lat_max) * 0.5;
//	const double sample_lons[8] = {
//		lon_min, lon_max, lon_min, lon_max,
//		lon_mid, lon_mid, lon_min, lon_max
//	};
//	const double sample_lats[8] = {
//		lat_min, lat_min, lat_max, lat_max,
//		lat_min, lat_max, lat_mid, lat_mid
//	};
//
//	/* Track per-octant (fx,fy) bounds. */
//	struct OctBounds { double fx0, fx1, fy0, fy1; bool used; };
//	OctBounds oct_bounds[8];
//	for (int i = 0; i < 8; i++)
//		oct_bounds[i] = {0,0,0,0, false};
//
//	for (int s = 0; s < 8; s++) {
//		H9BOct b = h9_lonlatdeg_to_boct(sample_lons[s], sample_lats[s]);
//		double fx, fy;
//		h9_braw_from_boct(b, &fx, &fy);
//		OctBounds &ob = oct_bounds[b.oct_i];
//		if (!ob.used) {
//		ob.fx0 = ob.fx1 = fx;
//		ob.fy0 = ob.fy1 = fy;
//		ob.used = true;
//		} else {
//		if (fx < ob.fx0) ob.fx0 = fx;
//		if (fx > ob.fx1) ob.fx1 = fx;
//		if (fy < ob.fy0) ob.fy0 = fy;
//		if (fy > ob.fy1) ob.fy1 = fy;
//		}
//	}
	const double lon_min = gbox.xmin, lon_max = gbox.xmax;
		const double lat_min = gbox.ymin, lat_max = gbox.ymax;

		/* Track per-octant (fx,fy) bounds. */
		struct OctBounds { double fx0, fx1, fy0, fy1; bool used; };
		OctBounds oct_bounds[8];
		for (int i = 0; i < 8; i++)
			oct_bounds[i] = {0,0,0,0, false};

		/* FIX: 5x5 sample grid to capture spherical bowing for large bounds */
		for (int i = 0; i <= 4; i++) {
			for (int j = 0; j <= 4; j++) {
				double slon = lon_min + (lon_max - lon_min) * (i / 4.0);
				double slat = lat_min + (lat_max - lat_min) * (j / 4.0);
				
				H9BOct b = h9_lonlatdeg_to_boct(slon, slat);
				double fx, fy;
				h9_braw_from_boct(b, &fx, &fy);
				
				OctBounds &ob = oct_bounds[b.oct_i];
				if (!ob.used) {
					ob.fx0 = ob.fx1 = fx;
					ob.fy0 = ob.fy1 = fy;
					ob.used = true;
				} else {
					if (fx < ob.fx0) ob.fx0 = fx;
					if (fx > ob.fx1) ob.fx1 = fx;
					if (fy < ob.fy0) ob.fy0 = fy;
					if (fy > ob.fy1) ob.fy1 = fy;
				}
			}
		}
		
	/* Pre-compute integer ranges per octant to determine allocation size.
	* max_raw = sum of all (ia × ib) candidates before face-validity check.
	* This is conservative (not all pass h9_braw_in_face) but always safe. */
	struct OctRange { long ia0, ia1, ib0, ib1; };
	OctRange oct_range[8];
	long max_raw = 0;
	for (int oct_i = 0; oct_i < 8; oct_i++) {
		if (!oct_bounds[oct_i].used) { oct_range[oct_i] = {0,0,0,0}; continue; }
		const OctBounds &ob = oct_bounds[oct_i];
		long ia0 = (long)floor((ob.fx0 - 4.0*ia_step) / ia_step);
		long ia1 = (long)ceil ((ob.fx1 + 4.0*ia_step) / ia_step);
		long ib0 = (long)floor((ob.fy0 - 4.0*ib_step) / ib_step);
		long ib1 = (long)ceil ((ob.fy1 + 4.0*ib_step) / ib_step);
		oct_range[oct_i] = {ia0, ia1, ib0, ib1};
		max_raw += (ia1 - ia0 + 1) * (ib1 - ib0 + 1);
	}
	if (max_raw < 64) max_raw = 64;

	uint8_t (*raw)[16] = (uint8_t (*)[16]) palloc(max_raw * 16);
	int raw_n = 0;

	/* Per-octant lattice enumeration. */
	for (int oct_i = 0; oct_i < 8; oct_i++) {
		if (!oct_bounds[oct_i].used) continue;

		const long ia_lo = oct_range[oct_i].ia0;
		const long ia_hi = oct_range[oct_i].ia1;
		const long ib_lo = oct_range[oct_i].ib0;
		const long ib_hi = oct_range[oct_i].ib1;

		for (long ib = ib_lo; ib <= ib_hi; ib++) {
			
			/* FIX: Delete the mod-3 filter entirely.
			 * The centers of flat-topped hexagons form a perfect triangular
			 * lattice where `ia` and `ib` share parity. */
			if (((ib % 3L) + 3L) % 3L == 0L) continue;
			const long ia_parity = ib & 1L;

			for (long ia = ia_lo; ia <= ia_hi; ia++) {
				if ((ia & 1L) != ia_parity) continue;

				const double fx = ia * ia_step;
				const double fy = ib * ib_step;
				if (!h9_braw_in_face(fx, fy, oct_i)) continue;

				H9BOct b = h9_boct_from_braw(fx, fy, oct_i);
				uint8_t full_uuid[16], bin_uuid[16];
				h9_boct_to_uuid(b, full_uuid);
				h9_bin_uuid(full_uuid, layer, bin_uuid);
				memcpy(raw[raw_n++], bin_uuid, 16);
			}
		}
	}

	/* Sort and deduplicate (handles seam cells touching multiple octants). */
	qsort(raw, raw_n, 16, h9_uuid16_cmp);
	int unique = 0;
	for (int i = 0; i < raw_n; i++)
		if (i == 0 || memcmp(raw[i], raw[i-1], 16) != 0)
		memcpy(raw[unique++], raw[i], 16);

	LWGEOM *bounds_lwg = lwgeom_from_gserialized(bounds);

	state		 = (H9GridState *) palloc(sizeof(H9GridState));
	state->layer	 = layer;
	state->count	 = unique;
	state->idx	= 0;
	state->uuids	 = raw;
	state->bounds_lwg = bounds_lwg;
	funcctx->user_fctx = state;

	get_call_result_type(fcinfo, NULL, &funcctx->tuple_desc);
	BlessTupleDesc(funcctx->tuple_desc);

	MemoryContextSwitchTo(oldctx);
	}

	funcctx = SRF_PERCALL_SETUP();
	state	= (H9GridState *) funcctx->user_fctx;

	while (state->idx < state->count) {
	const uint8_t *bin_uuid = state->uuids[state->idx++];

	double clon, clat;
	h9_cell_centre_lonlat(bin_uuid, state->layer, &clon, &clat);

	POINT2D pt = {clon, clat};
		bool in_bounds = h9_lwgeom_contains_pt(state->bounds_lwg, &pt);
		if (!in_bounds) {
			/* Centre is outside — check the 6 hex vertices to catch edge cells
			 * whose polygon overlaps the query bounds even when the centre does not. */
			double fx, fy; int oct_i, oct_mode;
			h9_cell_unpack(bin_uuid, state->layer, &fx, &fy, &oct_i, &oct_mode);
			const double du = (H9.W / 6.0) * pow(1.0 / 3.0, (double)state->layer);
			const double dv = (H9.H / 3.0) * pow(1.0 / 3.0, (double)state->layer);
			static const int8_t vo[6][2] = {{2,0},{1,1},{-1,1},{-2,0},{-1,-1},{1,-1}};
			for (int v = 0; v < 6 && !in_bounds; v++) {
				double vlon, vlat;
				h9_bary_to_lonlat(fx + vo[v][0]*du, fy + vo[v][1]*dv, oct_i, oct_mode, &vlon, &vlat);
				POINT2D vpt = {vlon, vlat};
				in_bounds = h9_lwgeom_contains_pt(state->bounds_lwg, &vpt);
			}
		}
		if (!in_bounds) continue;
		
	Datum values[2];
	bool isnull[2] = {false, false};
	values[0] = UUIDPGetDatum(make_pg_uuid(bin_uuid));
	values[1] = PointerGetDatum(h9_make_cell_geom(bin_uuid, state->layer));
	HeapTuple tup = heap_form_tuple(funcctx->tuple_desc, values, isnull);
	SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tup));
	}

	lwgeom_free(state->bounds_lwg);
	SRF_RETURN_DONE(funcctx);
}
} /* extern "C" */
#endif /* legacy h9_grid_prx */
