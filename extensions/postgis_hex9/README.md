# postgis_hex9

A PostGIS extension for the **Hex9 (H9) Discrete Global Grid System** — an
equal-area hexagonal global grid with self-contained UUID cell identifiers and a
9-fold (3×3) hierarchy from layer 0 (12 base cells) down to layer 29
(~95 nm cell diameter).

Each cell ID is a standard `uuid`, so it indexes, joins, and `GROUP BY`s with
zero custom types. An authalic (equal-area) warp keeps cell areas uniform to
~0.014 % across the globe.

---

## Install

```sql
CREATE EXTENSION postgis;          -- required dependency
CREATE EXTENSION postgis_hex9;
```

Build from source (standalone / development):

```sh
cd extensions/postgis_hex9
make && sudo make install
make installcheck        # needs a PostgreSQL with PostGIS available
```

> The module resolves liblwgeom symbols at load time and works in any backend
> regardless of whether a PostGIS function was called first — no `shared_preload_libraries`
> or `LOAD` needed.

---

## Concepts

| Term | Meaning |
|------|---------|
| **layer** | Resolution level `0`–`29`. Each step is a 3×3 (9×) subdivision. Layer 29 ≈ 95 nm. |
| **full UUID** | From `h9_encode()`. Carries the exact point context — losslessly reversible via `h9_decode()`. |
| **bin UUID** | From `h9_bin(uuid, layer)`. All points in the same layer-`L` cell collapse to one UUID — the cell key for aggregation. (Lossy below `L`.) |
| **cell** | The hexagon for one UUID at a layer — `h9_cell()` (one) or `h9_grid()` (many over an area). |

---

## Quick start

```sql
-- 1. Encode a point to its layer-29 cell UUID, and round-trip back.
SELECT h9_encode('SRID=4326;POINT(-3.19 55.95)'::geometry);   -- → uuid
SELECT ST_AsText(h9_decode(h9_encode('SRID=4326;POINT(-3.19 55.95)'::geometry)));

-- 2. Bin points into layer-8 cells and count (spatial GROUP BY).
SELECT h9_bin(h9_encode(geom), 8) AS cell, count(*)
FROM   my_points
GROUP  BY cell;

-- 3. Draw the hexagon for a cell.
SELECT h9_cell(h9_bin(h9_encode(geom), 8), 8) FROM my_points;

-- 4. Tile an area with a layer-8 grid (UUID + polygon + centroid per cell).
SELECT hex9, ST_AsText(geom), ST_AsText(centroid)
FROM   h9_grid(ST_MakeEnvelope(-0.2, 51.4, 0.0, 51.6, 4326), 8);
```

A bin UUID makes a clean **functional index** or **generated column** (all
functions are `IMMUTABLE STRICT`):

```sql
ALTER TABLE my_points
  ADD COLUMN cell8 uuid GENERATED ALWAYS AS (h9_bin(h9_encode(geom), 8)) STORED;
CREATE INDEX ON my_points (cell8);
```

---

## Function reference

| Function | Returns | Purpose |
|----------|---------|---------|
| `h9_encode(geometry)` | `uuid` | Encode a POINT (any SRID transformable to 4326) to its layer-29 cell UUID. |
| `h9_decode(uuid)` | `geometry` | Decode a UUID back to a POINT (SRID 4326). Round-trips `h9_encode` to ~95 nm. |
| `h9_bin(uuid, layer)` | `uuid` | Cell-key UUID at `layer` 0–29. Equal for all points sharing that cell — use for `GROUP BY` / indexes. |
| `h9_cell(uuid, layer, densify DEFAULT 0)` | `geometry` | Hexagon polygon (SRID 4326) for a UUID at `layer` 1–29. |
| `h9_grid(bounds, layer, densify DEFAULT 0)` | `TABLE(hex9 uuid, geom geometry, centroid geometry)` | Every cell at `layer` 1–29 whose centre lies in `bounds`. |
| `h9_label(uuid, layer)` | `text` | Compact body-nibble label, e.g. `478232778`. |
| `h9_label_key(uuid, layer)` | `text` | `h9_label` plus the `.key_tail` nibble, e.g. `478232778.9`. |
| `h9_version()` | `text` | Extension version + build stamp. |

`h9_diag(geometry) → text` also exists but is an internal frame-parity
diagnostic, not part of the stable API.

### `densify` (rendering large cells)

`h9_cell` and `h9_grid` take an optional `densify` (default `0`) that subdivides
each of the 6 hex edges into `3^densify` segments, so edges follow the H9 lattice
instead of straight `(lon, lat)` chords. Ring size is `6·3^densify + 1`
(`0`→7, `1`→19, `2`→55, `3`→163 points). Useful at coarse layers (≤ 3); at
layer ≥ 5 the corner-only form is normally fine. Constraints: `densify ≥ 0`,
`layer + densify ≤ 29`, `densify ≤ 9`.

```sql
-- A layer-2 grid over Europe with smooth (densified) hex edges:
SELECT hex9, geom
FROM   h9_grid(ST_MakeEnvelope(-10, 35, 30, 60, 4326), 2, 3);
```

---

## Caveats

- **Bin UUIDs are lossy below their layer.** `h9_cell` on a bin UUID may collapse
  distinct cells to the same polygon — for per-cell rendering over an area prefer
  `h9_grid`, which keeps non-lossy state internally.
- **`h9_grid` is capped** by the `hex9.grid_max_cells` GUC (default 708 588). For a
  large area, use a coarser layer/smaller bounds or raise the cap:
  `SET hex9.grid_max_cells = 5000000;`
- **Antimeridian / poles:** cell polygons spanning the antimeridian are returned
  unsplit; split with `ST_Split`/your own logic if your renderer needs it.

## Settings (GUCs)

| GUC | Default | Effect |
|-----|---------|--------|
| `hex9.grid_max_cells` | `708588` | Max cells `h9_grid()` will emit before erroring. |
| `hex9.use_warp` | `on` | Apply the authalic (equal-area) warp. `off` = raw octahedral projection. |
| `hex9.encoder` | `nn` | Encoder path: `nn` (legacy) or `containment` (grid-canonical at cell boundaries). |
