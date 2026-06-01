/*
 * h9_addressing.h — UUID nibble encode/decode for the postgis_hex9 extension.
 *
 * Part of the Hex9 (H9) Project
 * Copyright ©2025, Ben Griffin
 * Licensed under the Apache License, Version 2.0
 *
 * Depends on h9_math.h (must be included first — uses H9Tables H9, H9BOct,
 * h9_braw_from_boct, H9_UP_CIDS/DN_CIDS, H9_UP_MODE/DN_MODE).
 *
 * Public interface
 * ────────────────
 *   void   h9_boct_to_uuid(H9BOct b, uint8_t uuid[16])
 *   H9BOct h9_uuid_to_boct(const uint8_t uuid[16])
 *   void   h9_bin_uuid(const uint8_t in[16], int layer, uint8_t out[16])
 *
 * UUID layout (32 nibbles, big-endian)
 * ─────────────────────────────────────
 *   nibbles  0..29 : body  L0..L29 (hierarchical hex digits; valid range 0..11
 *                    at L0, 0..8 at L1+; 0xF is never a valid body nibble)
 *   nibble      30 : h_term — terminal RID (0..11); 0xF = OOB sentinel in bins
 *   nibble      31 : key_tail = (p_mo << 3) | (p_c2 << 1) | r_mo
 *                    p_mo  = rids[29] & 1  (mode of parent, seeds backward pass)
 *                    p_c2  = c2 from H9_REG_HEX at the L29→L30 forward step
 *                    r_mo  = oct_mode (0=DN, 1=UP); invariant within any cell
 *
 * Bin UUID (from h9_bin_uuid)
 * ───────────────────────────
 *   nibbles  0..L  : same body as the full UUID
 *   nibbles L+1..30: 0x0F sentinel (OOB — valid digits never reach 0xF)
 *   nibble      31 : bin key_tail = (c2_L << 1) | r_mo
 *                    c2_L = backward-pass c2 context at layer L (matches Python tail_pack_key)
 *
 * Octant ID convention
 * ─────────────────────
 *   H9BOct.oct_i stores the Python oid (canonical):
 *     oct_i = ((eZ<0)?4:0)|((eY<0)?2:0)|((eX<0)?1:0)  — 1-bit means negative
 *     bit0=eX<0, bit1=eY<0, bit2=eZ<0
 *   Extract axis signs: su=(oct_i&1)?-1:+1, sv=(oct_i&2)?-1:+1,
 *                       sw=(oct_i&4)?-1:+1
 *   All L0 LUTs are indexed by oct_i directly (no ^7 conversion needed).
 */

#pragma once
//#include <iostream>
//#include <iomanip>

#include <string.h>    /* memcpy, memmove, memset */
#include "h9_math.h"   /* H9BOct, H9Tables H9, h9_braw_from_boct, H9_UP/DN_* */

/* ── Sentinel ────────────────────────────────────────────────────────────── */

static const uint8_t H9A_OOB = 0xFF;

/* ── CID / RID tables ────────────────────────────────────────────────────── */

/*
 * UP / DN child CIDs matching H9Tables.UP_X/Y and DN_X/Y (index 0..8).
 * Shared between h9_math.h (commented) and this file for offset lookup.
 */
static const uint8_t H9_UP_CIDS[9] = {
    0x16, 0x25, 0x26, 0x2a, 0x34, 0x35, 0x39, 0x3a, 0x3e
};
static const uint8_t H9_DN_CIDS[9] = {
    0x21, 0x25, 0x26, 0x2a, 0x2b, 0x35, 0x39, 0x3a, 0x49
};

/*
 * rid2cell[rid] → CID  (rid 0..15; rid 12..15 map to 0x5F = invalid sentinel)
 */
static const uint8_t H9_RID2CELL[16] = {
    0x49, 0x16, 0x2b, 0x34, 0x21, 0x3e, 0x26, 0x39,
    0x35, 0x2a, 0x3a, 0x25, 0x5f, 0x5f, 0x5f, 0x5f
};

/*
 * cell2rid[CID] → rid  (indexed 0..95; 0xFF = invalid)
 */
static const uint8_t H9_CELL2RID[96] = {
    /* 0x00 */ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    /* 0x10 */ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,   1,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    /* 0x20 */ 0xFF,   4,0xFF,0xFF,0xFF,  11,   6,0xFF,0xFF,0xFF,   9,   2,0xFF,0xFF,0xFF,0xFF,
    /* 0x30 */ 0xFF,0xFF,0xFF,0xFF,   3,   8,0xFF,0xFF,0xFF,   7,  10,0xFF,0xFF,0xFF,   5,0xFF,
    /* 0x40 */ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,   0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    /* 0x50 */ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  15
};

/* ── L0 hex LUTs ─────────────────────────────────────────────────────────── */

/*
 * Octant mode by Python oid (0..7).  oid_mo[py_oid] == py_oid's r_mo.
 * (Not directly used in encode; oct_mode from H9BOct is the source of truth.)
 */
static const uint8_t H9_OID_MO[8] = { 0, 1, 1, 0, 1, 0, 0, 1 };

/*
 * l0hex_by_id[py_oid][c2] → L0 hex digit (0..11)
 * Used in encode: body[0] = l0hex_by_id[oct_i ^ 7][c2_l0]
 */
static const uint8_t H9_L0HEX_BY_ID[8][3] = {
    { 0,  4,  5},   /* py_oid=0 */
    { 1,  5,  7},   /* py_oid=1 */
    { 2,  6,  4},   /* py_oid=2 */
    { 3,  7,  6},   /* py_oid=3 */
    { 0,  8, 10},   /* py_oid=4 */
    { 1,  9,  8},   /* py_oid=5 */
    { 2, 10, 11},   /* py_oid=6 */
    { 3, 11,  9}    /* py_oid=7 */
};

/*
 * l0hex_back[hex_digit][r_mo] → {py_oid, c2}
 * Used in decode: py_oid = l0hex_back[nibbles[0]][r_mo][0], oct_i = py_oid ^ 7
 */
static const uint8_t H9_L0HEX_BACK[12][2][2] = {
    /* hex  0 */ {{ 0, 0}, { 4, 0}},
    /* hex  1 */ {{ 5, 0}, { 1, 0}},
    /* hex  2 */ {{ 6, 0}, { 2, 0}},
    /* hex  3 */ {{ 3, 0}, { 7, 0}},
    /* hex  4 */ {{ 0, 1}, { 2, 2}},
    /* hex  5 */ {{ 0, 2}, { 1, 1}},
    /* hex  6 */ {{ 3, 2}, { 2, 1}},
    /* hex  7 */ {{ 3, 1}, { 1, 2}},
    /* hex  8 */ {{ 5, 2}, { 4, 1}},
    /* hex  9 */ {{ 5, 1}, { 7, 2}},
    /* hex 10 */ {{ 6, 1}, { 4, 2}},
    /* hex 11 */ {{ 6, 2}, { 7, 1}}
};

/*
 * mcc2[r_mo][CID] → c2 value (0..2) for the L0 child cell.
 * 0xFF = invalid (CID not valid for that root mode).
 * Used in encode: c2_l0 = H9_MCC2[oct_mode][cids[1]]
 */
static const uint8_t H9_MCC2[2][96] = {
    {   /* r_mo = 0 (DOWN proto) */
        /* 0x00 */ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        /* 0x10 */ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        /* 0x20 */ 0xFF,   2,0xFF,0xFF,0xFF,   2,   0,0xFF,0xFF,0xFF,   0,   0,0xFF,0xFF,0xFF,0xFF,
        /* 0x30 */ 0xFF,0xFF,0xFF,0xFF,0xFF,   2,0xFF,0xFF,0xFF,   1,   1,0xFF,0xFF,0xFF,0xFF,0xFF,
        /* 0x40 */ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,   1,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        /* 0x50 */ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
    },
    {   /* r_mo = 1 (UP proto) */
        /* 0x00 */ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        /* 0x10 */ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,   2,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        /* 0x20 */ 0xFF,0xFF,0xFF,0xFF,0xFF,   1,   2,0xFF,0xFF,0xFF,   2,0xFF,0xFF,0xFF,0xFF,0xFF,
        /* 0x30 */ 0xFF,0xFF,0xFF,0xFF,   1,   1,0xFF,0xFF,0xFF,   0,   0,0xFF,0xFF,0xFF,   0,0xFF,
        /* 0x40 */ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        /* 0x50 */ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
    }
};

/* ── reg_hex: region→hex LUT ─────────────────────────────────────────────── */

/*
 * H9_REG_HEX[gp_mo][parent_rid][child_rid][0..1] = {hex_digit, c2}
 * gp_mo = grandparent mode = (rids[ri-2] & 1)
 * 0xFF = invalid combination.
 *
 * Dimensions: [2][16][16][2]  (p_mo, parent_rid, child_rid, {h,c2})
 */
#define _I {0xFF,0xFF}
static const uint8_t H9_REG_HEX[2][16][16][2] = {
  { /* gp_mo = 0 */
    { /* p=0  */
      {2,1},_I,{4,0},_I,{8,2},_I,{4,0},{2,1},{8,2},{4,0},{2,1},{8,2},_I,_I,_I,_I
    },
    { /* p=1  */ _I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I },
    { /* p=2  */
      {8,1},_I,{2,0},_I,{4,2},_I,{2,0},{8,1},{4,2},{2,0},{8,1},{4,2},_I,_I,_I,_I
    },
    { /* p=3  */ _I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I },
    { /* p=4  */
      {4,1},_I,{8,0},_I,{2,2},_I,{8,0},{4,1},{2,2},{8,0},{4,1},{2,2},_I,_I,_I,_I
    },
    { /* p=5  */ _I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I },
    { /* p=6  */
      {0,1},_I,{3,0},_I,{6,2},_I,{3,0},{0,1},{6,2},{3,0},{0,1},{6,2},_I,_I,_I,_I
    },
    { /* p=7  */
      _I,{0,2},_I,{6,1},_I,{4,0},{0,2},{4,0},{6,1},{0,2},{4,0},{6,1},_I,_I,_I,_I
    },
    { /* p=8  */
      {6,1},_I,{0,0},_I,{3,2},_I,{0,0},{6,1},{3,2},{0,0},{6,1},{3,2},_I,_I,_I,_I
    },
    { /* p=9  */
      _I,{4,2},_I,{0,1},_I,{6,0},{4,2},{6,0},{0,1},{4,2},{6,0},{0,1},_I,_I,_I,_I
    },
    { /* p=10 */
      {3,1},_I,{6,0},_I,{0,2},_I,{6,0},{3,1},{0,2},{6,0},{3,1},{0,2},_I,_I,_I,_I
    },
    { /* p=11 */
      _I,{6,2},_I,{4,1},_I,{0,0},{6,2},{0,0},{4,1},{6,2},{0,0},{4,1},_I,_I,_I,_I
    },
    { /* p=12 */ _I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I },
    { /* p=13 */ _I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I },
    { /* p=14 */ _I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I },
    { /* p=15 */ _I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I }
  },
  { /* gp_mo = 1 */
    { /* p=0  */ _I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I },
    { /* p=1  */
      _I,{2,2},_I,{8,1},_I,{5,0},{2,2},{5,0},{8,1},{2,2},{5,0},{8,1},_I,_I,_I,_I
    },
    { /* p=2  */ _I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I },
    { /* p=3  */
      _I,{5,2},_I,{2,1},_I,{8,0},{5,2},{8,0},{2,1},{5,2},{8,0},{2,1},_I,_I,_I,_I
    },
    { /* p=4  */ _I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I },
    { /* p=5  */
      _I,{8,2},_I,{5,1},_I,{2,0},{8,2},{2,0},{5,1},{8,2},{2,0},{5,1},_I,_I,_I,_I
    },
    { /* p=6  */
      {1,1},_I,{5,0},_I,{7,2},_I,{5,0},{1,1},{7,2},{5,0},{1,1},{7,2},_I,_I,_I,_I
    },
    { /* p=7  */
      _I,{1,2},_I,{7,1},_I,{3,0},{1,2},{3,0},{7,1},{1,2},{3,0},{7,1},_I,_I,_I,_I
    },
    { /* p=8  */
      {7,1},_I,{1,0},_I,{5,2},_I,{1,0},{7,1},{5,2},{1,0},{7,1},{5,2},_I,_I,_I,_I
    },
    { /* p=9  */
      _I,{3,2},_I,{1,1},_I,{7,0},{3,2},{7,0},{1,1},{3,2},{7,0},{1,1},_I,_I,_I,_I
    },
    { /* p=10 */
      {5,1},_I,{7,0},_I,{1,2},_I,{7,0},{5,1},{1,2},{7,0},{5,1},{1,2},_I,_I,_I,_I
    },
    { /* p=11 */
      _I,{7,2},_I,{3,1},_I,{1,0},{7,2},{1,0},{3,1},{7,2},{1,0},{3,1},_I,_I,_I,_I
    },
    { /* p=12 */ _I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I },
    { /* p=13 */ _I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I },
    { /* p=14 */ _I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I },
    { /* p=15 */ _I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I,_I }
  }
};
#undef _I

/* ── hex_reg: hex→region inverse LUT ────────────────────────────────────── */

/*
 * H9_HEX_REG[hex_digit][c_mo][c2] = {rid, c_mo_new, c2_new}
 * Used in backward decode pass.
 * hex_digit: 0..8 (L1+ range); c_mo: 0..1; c2: 0..2
 */
static const uint8_t H9_HEX_REG[9][2][3][3] = {
    { /* h=0 */
        {{ 8, 0, 2}, { 6, 0, 0}, {10, 0, 1}},   /* c_mo=0: c2=0,1,2 */
        {{11, 0, 2}, { 9, 0, 0}, { 7, 0, 1}}    /* c_mo=1 */
    },
    { /* h=1 */
        {{ 8, 1, 1}, { 6, 1, 2}, {10, 1, 0}},
        {{11, 1, 1}, { 9, 1, 2}, { 7, 1, 0}}
    },
    { /* h=2 */
        {{ 2, 0, 0}, { 0, 0, 1}, { 4, 0, 2}},
        {{ 5, 1, 0}, { 3, 1, 1}, { 1, 1, 2}}
    },
    { /* h=3 */
        {{ 6, 0, 0}, {10, 0, 1}, { 8, 0, 2}},
        {{ 7, 1, 0}, {11, 1, 1}, { 9, 1, 2}}
    },
    { /* h=4 */
        {{ 0, 0, 1}, { 4, 0, 2}, { 2, 0, 0}},
        {{ 7, 0, 1}, {11, 0, 2}, { 9, 0, 0}}
    },
    { /* h=5 */
        {{ 6, 1, 2}, {10, 1, 0}, { 8, 1, 1}},
        {{ 1, 1, 2}, { 5, 1, 0}, { 3, 1, 1}}
    },
    { /* h=6 */
        {{10, 0, 1}, { 8, 0, 2}, { 6, 0, 0}},
        {{ 9, 0, 0}, { 7, 0, 1}, {11, 0, 2}}
    },
    { /* h=7 */
        {{10, 1, 0}, { 8, 1, 1}, { 6, 1, 2}},
        {{ 9, 1, 2}, { 7, 1, 0}, {11, 1, 1}}
    },
    { /* h=8 */
        {{ 4, 0, 2}, { 2, 0, 0}, { 0, 0, 1}},
        {{ 3, 1, 1}, { 1, 1, 2}, { 5, 1, 0}}
    }
};

/* ── h9_boct_from_braw ───────────────────────────────────────────────────── */
/*                                                                             */
/*   Inverse of h9_braw_from_boct: construct H9BOct from (fx, fy, oct_i).   */
/*   oct_mode is determined by oct_i via H9_OID_MO[oct_i ^ 7].              */

static H9BOct h9_boct_from_braw(double fx, double fy, int oct_i) {
    H9BOct b;
    b.oct_i    = oct_i;
    b.oct_mode = (int)H9_OID_MO[oct_i];   /* oct_i IS Python oid — no ^7 */

    /* Python oid: bit0=eX<0, bit1=eY<0, bit2=eZ<0; 1 means negative.
     * u maps to eX, v to eY, w to eZ (via h9_c_oct_to_c_ell). */
    const double su = (oct_i & 1) ? -1.0 : 1.0;  /* sign(eX) */
    const double sv = (oct_i & 2) ? -1.0 : 1.0;  /* sign(eY) */
    const double sw = (oct_i & 4) ? -1.0 : 1.0;  /* sign(eZ) */

    double au, av, aw;
    if (b.oct_mode == 1) {          /* Up */
        au = fy * H9.inv_H + (1.0/3.0);
        const double rest = 1.0 - au;
        aw = rest * 0.5 + fx * H9.inv_W;
        av = rest - aw;
    } else {                        /* Down */
        au = (1.0/3.0) - fy * H9.inv_H;
        const double rest = 1.0 - au;
        av = rest * 0.5 + fx * H9.inv_W;
        aw = rest - av;
    }
    b.u = su * au;
    b.v = sv * av;
    b.w = sw * aw;
    return b;
}

/* Returns true if (fx, fy) lies within the triangular octant face for oct_i. */
static bool h9_braw_in_face(double fx, double fy, int oct_i) {
    const int oct_mode = (int)H9_OID_MO[oct_i];  /* oct_i IS Python oid */
    double au, av, aw;
    if (oct_mode == 1) {
        au = fy * H9.inv_H + (1.0/3.0);
        const double rest = 1.0 - au;
        aw = rest * 0.5 + fx * H9.inv_W;
        av = rest - aw;
    } else {
        au = (1.0/3.0) - fy * H9.inv_H;
        const double rest = 1.0 - au;
        av = rest * 0.5 + fx * H9.inv_W;
        aw = rest - av;
    }
    return au >= -1e-9 && av >= -1e-9 && aw >= -1e-9;
}

/* ── Internal helpers ───────────────────��─────────────────────────────���──── */

/*
 * Metric-barycentric offset (ox, oy) for a child CID.
 * CIDs shared between UP and DN tables have identical offsets, so the UP
 * table is checked first and is sufficient for all valid CIDs.
 */
static void h9a_cid_offset(uint8_t cid, double *ox, double *oy) {
    for (int j = 0; j < 9; ++j)
        if (H9_UP_CIDS[j] == cid) { *ox = H9.UP_X[j]; *oy = H9.UP_Y[j]; return; }
    for (int j = 0; j < 9; ++j)
        if (H9_DN_CIDS[j] == cid) { *ox = H9.DN_X[j]; *oy = H9.DN_Y[j]; return; }
    *ox = 0.0; *oy = 0.0;
}

/* Pack 32 nibbles (each 0..15) into 16 big-endian bytes. */
static void h9a_pack(const uint8_t nibbles[32], uint8_t uuid[16]) {
    for (int k = 0; k < 16; ++k)
        uuid[k] = (uint8_t)((nibbles[2*k] << 4) | nibbles[2*k+1]);
}

/* Unpack 16 big-endian bytes into 32 nibbles. */
static void h9a_unpack(const uint8_t uuid[16], uint8_t nibbles[32]) {
    for (int k = 0; k < 16; ++k) {
        nibbles[2*k]   = uuid[k] >> 4;
        nibbles[2*k+1] = uuid[k] & 0x0F;
    }
}

/* ── h9_boct_to_uuid ─────────────────────────────────────────────────────── */
/*                                                                             */
/*   Encodes an H9BOct (from h9_lonlatdeg_to_boct) to a self-contained UUID. */
/*   No companion byte is needed; all decode context lives in nibbles[30..31].*/
/*                                                                             */
/*   Algorithm:                                                                */
/*     1. Extract (fx, fy) in unwarped metric-barycentric space.              */
/*     2. 1-best Euclidean descent through 30 levels → CID chain.            */
/*     3. CID chain → RID chain via H9_CELL2RID.                             */
/*     4. RID chain → 30 body nibbles via L0hex + H9_REG_HEX.               */
/*     5. Store h_term=rids[30] in nibbles[30]; compose key_tail.            */

static void to_uuid(double fx, double fy, const int oct_i, const int oct_mode, uint8_t uuid[16]) {
//	const int oct_i    = b.oct_i;
//	const int oct_mode = b.oct_mode;
//	const int py_oid   = oct_i;   /* oct_i already is Python oid — no ^7 */

	/* Step 1 — (fx, fy) in BRAW frame (oriented + mode-1 y-flip), matching Python */
//	double raw_x, raw_y;
//	h9_braw_from_boct(b, &raw_x, &raw_y);
//	double fx, fy;
//	h9_orient_fwd(raw_x, raw_y, oct_i, &fx, &fy);
//	if (oct_mode == 1) fy = -fy;  /* braw_y convention: ry0 = -ry for mode=1 */

	/* Step 2 — 1-best descent: 36 levels → CID chain (cids[0]=proto, cids[1..36]=children)
	 * Descend 6 extra levels for look-ahead; nibble encoding still uses only 1..30. */
	uint8_t cids[37];
	cids[0] = H9_RID2CELL[oct_mode];

	int cur_mode = oct_mode;
	double cx = fx, cy = fy;

	for (int l = 0; l < 36; ++l) {
		const double  *offx     = (cur_mode == 1) ? H9.UP_X    : H9.DN_X;
		const double  *offy     = (cur_mode == 1) ? H9.UP_Y    : H9.DN_Y;
		const uint8_t *cid_tab  = (cur_mode == 1) ? H9_UP_CIDS : H9_DN_CIDS;
		const int     *mode_tab = (cur_mode == 1) ? H9_UP_MODE : H9_DN_MODE;

		int    best_j  = 0;
		double best_d2 = 1e300;
		for (int j = 0; j < 9; ++j) {
			const double dx = cx - offx[j];
			const double dy = cy - offy[j];
			const double d2 = dx*dx + dy*dy;
			if (d2 < best_d2) { best_d2 = d2; best_j = j; }
		}
		cids[l+1]  = cid_tab[best_j];
		cx         = (cx - offx[best_j]) * 3.0;
		cy         = (cy - offy[best_j]) * 3.0;
		cur_mode   = mode_tab[best_j];
	}

	/* Step 3 — CID → RID */
	uint8_t rids[37];
	for (int i = 0; i <= 36; ++i)
		rids[i] = H9_CELL2RID[cids[i]];

	/* Step 4 — compute 30 body nibbles */
	uint8_t nibbles[32];

	/* L0 body nibble */
	uint8_t c2_l0 = H9_MCC2[oct_mode][cids[1]];
	nibbles[0] = H9_L0HEX_BY_ID[oct_i][c2_l0];

	/* L1..L29 body nibbles via H9_REG_HEX */
	uint8_t last_c2 = 0;
	for (int ri = 2; ri <= 30; ++ri) {
		const uint8_t gp_mo  = rids[ri-2] & 1u;
		const uint8_t p_rid  = rids[ri-1];
		const uint8_t c_rid  = rids[ri];
		const uint8_t *entry = H9_REG_HEX[gp_mo][p_rid][c_rid];
		nibbles[ri-1] = entry[0];
		last_c2       = entry[1];
	}

	/* Step 5 — terminal nibble and key_tail */
	/* nibbles[30] = h_term: the terminal RID, stored raw (0..11, never 0xF) */
	nibbles[30] = rids[30];
	/* key_tail seeds the backward pass starting at L29:
	 *   p_mo = rids[29] & 1  (c_mo for the H9_HEX_REG lookup at l=29)
	 *   p_c2 = last_c2       (c2  for the H9_HEX_REG lookup at l=29)
	 *   r_mo = oct_mode      (invariant; encodes octant UP/DN proto) */
	const uint8_t p_mo = rids[29] & 1u;
	const uint8_t r_mo = (uint8_t)oct_mode;
	nibbles[31] = (uint8_t)((p_mo << 3) | (last_c2 << 1) | r_mo);
	h9a_pack(nibbles, uuid);
}

/* NOTE: for grid-canonical UUIDs that match HexMesh.create_clipped, prefer
 * h9grid::uuid_from_cxcy in h9_grid.h.  This 1-best encoder uses nearest-
 * neighbour descent and diverges from Python's containment classifier at
 * cell boundaries; kept for non-grid callers. */
static void h9_boct_to_uuid(H9BOct b, uint8_t uuid[16]) {
    const int oct_i    = b.oct_i;
    const int oct_mode = b.oct_mode;
    const int py_oid   = oct_i;   /* oct_i already is Python oid — no ^7 */

    /* Step 1 — (fx, fy) in BARY (b_oct) frame, matching Python h9_enc which
     * descends in b_oct space (not b_raw). Pipeline: braw_from_boct →
     * orient_fwd → braw_y flip → warp_fwd (NR) → BARY. */
    double raw_x, raw_y;
    h9_braw_from_boct(b, &raw_x, &raw_y);
    double bx, by;
    h9_orient_fwd(raw_x, raw_y, oct_i, &bx, &by);
    if (oct_mode == 1) by = -by;  /* braw_y convention */
    double fx = bx, fy = by;
#if H9_WARP_ENABLE
    /* Descent runs in the warp.undo(BRAW) frame — matches Python
     * b_oct.encode (verified via h9_diag in containment path). */
    h9_warp_inv(bx, by, oct_mode, &fx, &fy);
#endif
    h9_clamp_bary(&fx, &fy, oct_mode);

    /* Step 2 — 1-best descent: 36 levels → CID chain (cids[0]=proto, cids[1..36]=children)
     * Descend 6 extra levels for look-ahead; nibble encoding still uses only 1..30. */
    uint8_t cids[37];
    cids[0] = H9_RID2CELL[oct_mode];

    int cur_mode = oct_mode;
    double cx = fx, cy = fy;

    for (int l = 0; l < 36; ++l) {
        const double  *offx     = (cur_mode == 1) ? H9.UP_X    : H9.DN_X;
        const double  *offy     = (cur_mode == 1) ? H9.UP_Y    : H9.DN_Y;
        const uint8_t *cid_tab  = (cur_mode == 1) ? H9_UP_CIDS : H9_DN_CIDS;
        const int     *mode_tab = (cur_mode == 1) ? H9_UP_MODE : H9_DN_MODE;

        int    best_j  = 0;
        double best_d2 = 1e300;
        for (int j = 0; j < 9; ++j) {
            const double dx = cx - offx[j];
            const double dy = cy - offy[j];
            const double d2 = dx*dx + dy*dy;
            if (d2 < best_d2) { best_d2 = d2; best_j = j; }
        }
        cids[l+1]  = cid_tab[best_j];
        cx         = (cx - offx[best_j]) * 3.0;
        cy         = (cy - offy[best_j]) * 3.0;
        cur_mode   = mode_tab[best_j];
    }

    /* Step 3 — CID → RID */
    uint8_t rids[37];
	for (int i = 0; i <= 36; ++i) {
		rids[i] = H9_CELL2RID[cids[i]];
//		std::cout << std::hex << std::uppercase << (int)rids[idx];
	}
//	std::cout << std::endl;

	
    /* Step 4 — compute 30 body nibbles */
    uint8_t nibbles[32];

    /* L0 body nibble */
    uint8_t c2_l0 = H9_MCC2[oct_mode][cids[1]];
    nibbles[0] = H9_L0HEX_BY_ID[py_oid][c2_l0];

    /* L1..L29 body nibbles via H9_REG_HEX */
    uint8_t last_c2 = 0;
    for (int ri = 2; ri <= 30; ++ri) {
        const uint8_t gp_mo  = rids[ri-2] & 1u;
        const uint8_t p_rid  = rids[ri-1];
        const uint8_t c_rid  = rids[ri];
        const uint8_t *entry = H9_REG_HEX[gp_mo][p_rid][c_rid];
        nibbles[ri-1] = entry[0];
        last_c2       = entry[1];
    }

    /* Step 5 — terminal nibble and key_tail */
    /* nibbles[30] = h_term: the terminal RID, stored raw (0..11, never 0xF) */
    nibbles[30] = rids[30];
    /* key_tail seeds the backward pass starting at L29:
     *   p_mo = rids[29] & 1  (c_mo for the H9_HEX_REG lookup at l=29)
     *   p_c2 = last_c2       (c2  for the H9_HEX_REG lookup at l=29)
     *   r_mo = oct_mode      (invariant; encodes octant UP/DN proto) */
    const uint8_t p_mo = rids[29] & 1u;
    const uint8_t r_mo = (uint8_t)oct_mode;
    nibbles[31] = (uint8_t)((p_mo << 3) | (last_c2 << 1) | r_mo);
	
    h9a_pack(nibbles, uuid);
}

/* Run 1-best for `depth` levels from (cx, cy, mode); return final ||(cx,cy)||².
 * Used by h9_boct_to_uuid_beam to score candidate paths by their final residual
 * rather than accumulated step-wise d2, which is biased toward greedy choices. */
static double h9_1best_residual(double cx, double cy, int mode, int depth) {
    for (int l = 0; l < depth; ++l) {
        const double *offx = (mode == 1) ? H9.UP_X : H9.DN_X;
        const double *offy = (mode == 1) ? H9.UP_Y : H9.DN_Y;
        const int    *mt   = (mode == 1) ? H9_UP_MODE : H9_DN_MODE;
        int bj = 0; double bd = 1e300;
        for (int j = 0; j < 9; ++j) {
            const double dx = cx - offx[j], dy = cy - offy[j];
            const double d2 = dx*dx + dy*dy;
            if (d2 < bd) { bd = d2; bj = j; }
        }
        cx = (cx - offx[bj]) * 3.0;
        cy = (cy - offy[bj]) * 3.0;
        mode = mt[bj];
    }
    return cx*cx + cy*cy;
}

/* Width-w beam search for the first beam_depth levels, then 1-best for the
 * remainder.  Ranks candidates by FINAL residual (||(cx,cy)||² after running
 * 1-best to beam_depth from each candidate), not accumulated step-wise d2.
 * This correctly handles failures where the greedy 1-best picks the wrong child
 * at any intermediate level: the correct path has final residual ≈ 0 while
 * wrong paths diverge and accumulate non-zero final residual. */
/* NOTE: for grid-canonical UUIDs that match HexMesh.create_clipped, prefer
 * h9grid::uuid_from_cxcy in h9_grid.h.  This beam-search encoder uses k-best
 * nearest-neighbour descent in post-orient frame and diverges from Python at
 * cell boundaries; kept for non-grid callers (point-to-UUID encoding etc.). */
static void h9_boct_to_uuid_beam(H9BOct b, int beam_depth, int beam_w, uint8_t uuid[16]) {
    const int oct_i    = b.oct_i;
    const int oct_mode = b.oct_mode;
    const int py_oid   = oct_i;   /* oct_i already is Python oid — no ^7 */

    double raw_x, raw_y;
    h9_braw_from_boct(b, &raw_x, &raw_y);
    double bx, by;
    h9_orient_fwd(raw_x, raw_y, oct_i, &bx, &by);
    if (oct_mode == 1) by = -by;  /* braw_y convention */
    double fx = bx, fy = by;
#if H9_WARP_ENABLE
    /* Descent runs in the warp.undo(BRAW) frame — see h9_boct_to_uuid. */
    h9_warp_inv(bx, by, oct_mode, &fx, &fy);
#endif
    h9_clamp_bary(&fx, &fy, oct_mode);

    struct BCand { double cx, cy, score; int mode; uint8_t cids[37]; };
    BCand beam[9], nxt[81];   /* max beam_w=9, next has beam_w*9 slots */
    beam[0].cx = fx; beam[0].cy = fy; beam[0].score = 0.0;
    beam[0].mode = oct_mode;
    memset(beam[0].cids, 0, 37);
    beam[0].cids[0] = H9_RID2CELL[oct_mode];
    int bn = 1;

    for (int l = 0; l < beam_depth; ++l) {
        int nn = 0;
        for (int bi = 0; bi < bn; ++bi) {
            const BCand &c = beam[bi];
            const double  *offx     = (c.mode == 1) ? H9.UP_X    : H9.DN_X;
            const double  *offy     = (c.mode == 1) ? H9.UP_Y    : H9.DN_Y;
            const uint8_t *cid_tab  = (c.mode == 1) ? H9_UP_CIDS : H9_DN_CIDS;
            const int     *mode_tab = (c.mode == 1) ? H9_UP_MODE : H9_DN_MODE;
            for (int j = 0; j < 9; ++j) {
                const double dx = c.cx - offx[j], dy = c.cy - offy[j];
                BCand &nc = nxt[nn++];
                nc.cx = dx * 3.0; nc.cy = dy * 3.0;
                nc.mode  = mode_tab[j];
                /* Score = final residual after running 1-best for remaining levels.
                 * The correct path has residual ≈ 0; wrong paths diverge. */
                /* Look-ahead 36 levels from current position for accurate scoring. */
                nc.score = h9_1best_residual(nc.cx, nc.cy, nc.mode, 36 - l - 1);
                memcpy(nc.cids, c.cids, (size_t)(l + 1));
                nc.cids[l + 1] = cid_tab[j];
            }
        }
        /* Keep best beam_w by score (ascending) using insertion sort */
        bn = 0;
        for (int ni = 0; ni < nn; ++ni) {
            int p = 0;
            while (p < bn && beam[p].score < nxt[ni].score) ++p;
            if (p >= beam_w) continue;
            if (bn < beam_w) ++bn;
            memmove(&beam[p+1], &beam[p], sizeof(BCand) * (size_t)(bn - p - 1));
            beam[p] = nxt[ni];
        }
    }

    /* 1-best continuation from best candidate for remaining levels (up to 36 total) */
    uint8_t cids[37];
    memcpy(cids, beam[0].cids, (size_t)(beam_depth + 1));
    double cx = beam[0].cx, cy = beam[0].cy;
    int cur_mode = beam[0].mode;
    for (int l = beam_depth; l < 36; ++l) {
        const double  *offx     = (cur_mode == 1) ? H9.UP_X    : H9.DN_X;
        const double  *offy     = (cur_mode == 1) ? H9.UP_Y    : H9.DN_Y;
        const uint8_t *cid_tab  = (cur_mode == 1) ? H9_UP_CIDS : H9_DN_CIDS;
        const int     *mode_tab = (cur_mode == 1) ? H9_UP_MODE : H9_DN_MODE;
        int best_j = 0; double best_d2 = 1e300;
        for (int j = 0; j < 9; ++j) {
            const double dx = cx - offx[j], dy = cy - offy[j];
            const double d2 = dx*dx + dy*dy;
            if (d2 < best_d2) { best_d2 = d2; best_j = j; }
        }
        cids[l + 1] = cid_tab[best_j];
        cx = (cx - offx[best_j]) * 3.0;
        cy = (cy - offy[best_j]) * 3.0;
        cur_mode = mode_tab[best_j];
    }

    /* Pack: CIDs → RIDs → nibbles → UUID (same as h9_boct_to_uuid steps 3–5) */
    uint8_t rids[37];
    for (int i = 0; i <= 36; ++i) rids[i] = H9_CELL2RID[cids[i]];

    uint8_t nibbles[32];
    nibbles[0] = H9_L0HEX_BY_ID[py_oid][H9_MCC2[oct_mode][cids[1]]];
    uint8_t last_c2 = 0;
    for (int ri = 2; ri <= 30; ++ri) {
        const uint8_t *e = H9_REG_HEX[rids[ri-2] & 1u][rids[ri-1]][rids[ri]];
        nibbles[ri-1] = e[0]; last_c2 = e[1];
    }
    nibbles[30] = rids[30];
    nibbles[31] = (uint8_t)(((rids[29] & 1u) << 3) | (last_c2 << 1) | (uint8_t)oct_mode);
    h9a_pack(nibbles, uuid);
}

/* ── h9_uuid_to_boct ─────────────────────────────────────────────────────── */
/*                                                                             */
/*   Decodes a UUID back to an H9BOct. No companion byte needed.              */
/*   h_term is read directly from nibbles[30]; key_tail seeds the backward    */
/*   pass starting at L29.                                                    */
/*                                                                             */
/*   Algorithm:                                                                */
/*     1. Extract nibbles; recover tail fields from nibbles[30..31].          */
/*     2. Backward pass L29→L1: recover RID chain using H9_HEX_REG.         */
/*     3. CID chain from RID chain; accumulate (fx, fy) over L0..L29.       */
/*     4. (fx, fy) → H9BOct via inverse barycentric formula.                 */

static H9BOct h9_uuid_to_boct(const uint8_t uuid[16]) {
    uint8_t nibbles[32];
    h9a_unpack(uuid, nibbles);

    const uint8_t key_tail = nibbles[31];
    const uint8_t p_mo = (key_tail >> 3) & 1u;
    const uint8_t p_c2 = (key_tail >> 1) & 3u;
    const uint8_t r_mo = key_tail & 1u;

    /* Detect bin UUID (nibbles[30]==0xF) and find effective top level.
     * Full UUID: top_l=30, h_term in nibbles[30], backward pass L29→L1.
     * Bin  UUID: top_l=L,  sentinels in nibbles[L+1..30], backward pass LL→L1. */
    int top_l;
    if (nibbles[30] == 0x0Fu) {
        top_l = 0;
        for (int l = 1; l <= 29; ++l) {
            if (nibbles[l] == 0x0Fu) break;
            top_l = l;
        }
    } else {
        top_l = 30;
    }

    uint8_t rids[31] = {};
    rids[0] = r_mo;
    if (nibbles[30] != 0x0Fu)
        rids[30] = nibbles[30];  /* h_term for full UUID */

    uint8_t c_mo = p_mo;
    uint8_t c2   = p_c2;
    /* Full UUID: backward pass L29→L1 (rids[30] already set from h_term).
     * Bin  UUID: backward pass LL→L1 (key_tail holds context at layer L). */
    const int back_start = (nibbles[30] != 0x0Fu) ? top_l - 1 : top_l;
    for (int l = back_start; l >= 1; --l) {
        const uint8_t *entry = H9_HEX_REG[nibbles[l]][c_mo][c2];
        rids[l] = entry[0];
        c_mo    = entry[1];
        c2      = entry[2];
    }

    const uint8_t py_oid = H9_L0HEX_BACK[nibbles[0]][r_mo][0];
    const int     oct_i  = (int)py_oid;  /* py_oid from LUT IS the Python oid */
    const int  oct_mode  = (int)r_mo;

    uint8_t cids[31] = {};
    for (int i = 0; i <= top_l; ++i)
        cids[i] = H9_RID2CELL[rids[i]];

    /* Reconstruct (fx, fy) over the valid levels only.
     * Geometric series: fx = ox(cids[1]) + ox(cids[2])/3 + … + ox(cids[top_l])/3^(top_l-1) */
    double fx = 0.0, fy = 0.0;
    for (int l = top_l - 1; l >= 0; --l) {
        double ox, oy;
        h9a_cid_offset(cids[l + 1], &ox, &oy);
        fx = fx / 3.0 + ox;
        fy = fy / 3.0 + oy;
    }

    /* (fx, fy) is in the descent frame (warp.undo(BRAW)); apply warp_fwd
     * to bring it back to BRAW for orient_inv → b_raw. Symmetric inverse
     * of the warp_inv applied at encode time (h9_boct_to_uuid). */
    double bx_braw = fx, by_braw = fy;
#if H9_WARP_ENABLE
    h9_warp_fwd(fx, fy, oct_mode, &bx_braw, &by_braw);
#endif
    const double ry = (oct_mode == 1) ? -by_braw : by_braw;  /* undo braw_y flip */
    double raw_x, raw_y;
    h9_orient_inv(bx_braw, ry, oct_i, &raw_x, &raw_y);

    /* (raw_x, raw_y) → normalised unsigned barycentric (u, v, w) */
    const double inv_H  = H9.inv_H;
    const double inv_W  = H9.inv_W;
    const double inv_2H = H9.inv_2H;
    double u, v, w;
    if (oct_mode == 1) {
        u = raw_y * inv_H  + (1.0/3.0);
        v = (1.0/3.0) - raw_y * inv_2H - raw_x * inv_W;
        w = (1.0/3.0) - raw_y * inv_2H + raw_x * inv_W;
    } else {
        u = (1.0/3.0) - raw_y * inv_H;
        v = (1.0/3.0) + raw_y * inv_2H + raw_x * inv_W;
        w = (1.0/3.0) + raw_y * inv_2H - raw_x * inv_W;
    }
    if (u < 0.0) u = 0.0;
    if (v < 0.0) v = 0.0;
    if (w < 0.0) w = 0.0;
    const double s = u + v + w;
    if (s > 1e-30) { u /= s; v /= s; w /= s; }

    /* Apply octant signs. Python oid: bit0=eX<0, bit1=eY<0, bit2=eZ<0.
     * u maps to eX, v to eY, w to eZ. */
    const double su = (oct_i & 1) ? -1.0 : 1.0;  /* sign(eX) */
    const double sv = (oct_i & 2) ? -1.0 : 1.0;  /* sign(eY) */
    const double sw = (oct_i & 4) ? -1.0 : 1.0;  /* sign(eZ) */

    H9BOct b;
    b.u        = su * u;
    b.v        = sv * v;
    b.w        = sw * w;
    b.oct_i    = oct_i;
    b.oct_mode = oct_mode;
    return b;
}

/* ── h9_bin_uuid ─────────────────────────────────────────────────────────── */
/*                                                                             */
/*   Returns the bin-key UUID for layer L (0..29).                           */
/*   All points sharing nibbles[0..layer] (same L-level cell) produce the    */
/*   same output. Nibbles[layer+1..30] are set to 0x0F (OOB sentinel; valid  */
/*   body digits are 0..11 at L0, 0..8 at L1+; h_term is 0..11). Nibble[31] */
/*   stores the backward-pass context (c_mo_L, c2_L, r_mo) for h9_cell.     */
/*                                                                             */
/*   Canonicalization: body[layer] = nibbles[layer] is computed by the       */
/*   forward encoder as H9_REG_HEX[rids[layer-1]&1][rids[layer]][rids[layer+1]],*/
/*   so it depends on the level L+1 child. To make the bin UUID truly        */
/*   independent of sub-cell choices, this function:                          */
/*     1. Backward-decodes rids[0..layer] from the input UUID.                */
/*     2. Picks canonical rids[layer+1..30] via repeated child-0 descent.    */
/*     3. Forward-re-encodes nibbles[1..29] with these canonical RIDs.       */
/*     4. Applies sentinel fill at nibbles[layer+1..30] and writes the       */
/*        canonical key_tail.                                                 */
/*   Result: every sibling encoding of the same L-layer cell collapses to    */
/*   one bin UUID, and h9_encode(pt)→h9_bin matches h9_grid's emitted UUIDs. */
/*                                                                             */
/*   Declared IMMUTABLE in SQL — safe for GENERATED STORED columns and       */
/*   functional indexes.                                                      */

static void h9_bin_uuid(const uint8_t in[16], int layer, uint8_t out[16]) {
    uint8_t nibbles[32];
    h9a_unpack(in, nibbles);
    const uint8_t r_mo = nibbles[31] & 1u;

    /* Step 1a — backward pass from L29 down to layer+1.
     * After the loop (c_mo, c2) = backward-pass context at level layer —
     * used both to seed step 1b and as the key_tail context for this bin.
     * The actual rids[layer+1] is saved on the last iteration (or from
     * nibbles[30] directly when layer==29). */
    uint8_t c_mo = (nibbles[31] >> 3) & 1u;
    uint8_t c2   = (nibbles[31] >> 1) & 3u;
    uint8_t rids[31] = {};
    rids[0] = r_mo;

    for (int l = 29; l >= layer + 1; --l) {
        const uint8_t *e = H9_HEX_REG[nibbles[l]][c_mo][c2];
        if (l == layer + 1) rids[l] = e[0];   /* actual child at layer+1 */
        c_mo = e[1]; c2 = e[2];
    }
    /* Special case: layer==29 — rids[30] is the h_term stored directly. */
    if (layer == 29) rids[30] = nibbles[30];

    /* Save backward-pass context at layer for the key_tail. */
    const uint8_t kt_c_mo = c_mo, kt_c2 = c2;

    /* Step 1b — backward pass layer→1 to recover rids[1..layer]. */
    {
        uint8_t bc_mo = c_mo, bc2 = c2;
        for (int l = layer; l >= 1; --l) {
            const uint8_t *e = H9_HEX_REG[nibbles[l]][bc_mo][bc2];
            rids[l] = e[0]; bc_mo = e[1]; bc2 = e[2];
        }
    }

    /* Step 2 — canonical child-0 descent for rids[layer+2..30].
     * rids[layer+1] is the actual child decoded in step 1a, preserving the
     * lookahead nibble[layer] = REG_HEX[...][rids[layer]][rids[layer+1]]. */
    if (layer + 1 < 30) {
        int cur_mode = (int)(rids[layer + 1] & 1u);
        for (int l = layer + 1; l < 30; ++l) {
            const uint8_t *cidtab  = (cur_mode == 1) ? H9_UP_CIDS : H9_DN_CIDS;
            const int     *modetab = (cur_mode == 1) ? H9_UP_MODE : H9_DN_MODE;
            rids[l + 1] = H9_CELL2RID[cidtab[0]];
            cur_mode    = modetab[0];
        }
    }

    /* Step 3 — forward re-encode nibbles[1..29].
     * nibbles[1..layer-1] reproduce the input body (round-trip identity).
     * nibbles[layer] is recomputed with actual rids[layer+1] — matching
     * Python's TailStyle.key which uses the actual descent child.
     * nibbles[layer+1..29] use canonical descendants (overwritten in step 4). */
    uint8_t last_c2 = 0;
    for (int ri = 2; ri <= 30; ++ri) {
        const uint8_t *e = H9_REG_HEX[rids[ri-2]&1u][rids[ri-1]][rids[ri]];
        nibbles[ri-1] = e[0]; last_c2 = e[1];
    }

    /* Step 4 — sentinel fill and key_tail.
     * Python tail_pack_key stores (c2_L << 1) | r_mo — no c_mo bit.
     * Match that format: nibble[31] = (kt_c2 << 1) | r_mo. */
    for (int i = layer + 1; i <= 30; ++i)
        nibbles[i] = 0x0Fu;
    nibbles[31] = (uint8_t)((kt_c2 << 1) | r_mo);

    h9a_pack(nibbles, out);
}
