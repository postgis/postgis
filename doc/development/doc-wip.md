# Documentation WIP

This draft PR is still a migration workspace. The notes below track review
comments that need more content work or a maintainer decision before the branch
should leave draft state.

## Structure Questions

* `PRRC_kwDOAEM_Wc7POEs0`, `PRRC_kwDOAEM_Wc7POIRS`: the development environment
  material is now split under `environment/ubuntu/` and `environment/docker/`.
  Check whether the first-level directory should eventually be role-oriented,
  as suggested in the PR discussion by strk.
* `PRRC_kwDOAEM_Wc7POmvr`: `workflow/`, root `CONTRIBUTING.md`, and maintainer
  workflow still overlap. Decide whether contributor workflow should be one
  page with role-specific anchors.
* `PRRC_kwDOAEM_Wc7PQ_kd`, `PRRC_kwDOAEM_Wc7PRB5P`: the current directory tree
  uses many `README.md` files. Decide whether to flatten this into files such as
  `building.md`, `testing.md`, and `release/versioning.md` before the PR leaves
  draft.
* `PRRC_kwDOAEM_Wc7POSzg`, `PRRC_kwDOAEM_Wc7POUBR`,
  `PRRC_kwDOAEM_Wc7POVRO`: governance still needs a final shape. Current notes
  keep RFC-1/RFC-5 context, but the intended end state may be a governance page
  that declares bankruptcy on RFC numbering while preserving historical text.
* `PRRC_kwDOAEM_Wc7POljg`: the wiki migration map is intentionally temporary.
  When every useful page has a maintained home or a recorded retirement reason,
  flatten the map into the relevant topic pages or remove it.

## Developer Deep Dives

* `PRRC_kwDOAEM_Wc7POpi3`, `PRRC_kwDOAEM_Wc7POqCA`: `3DDistancecalc` needs a
  current-code audit and image recovery before it can become a maintained
  2D-vs-3D distance chapter.
* `PRRC_kwDOAEM_Wc7POyT-`, `PRRC_kwDOAEM_Wc7PPc2J`: affine and raster real
  parameter notes look useful, but need image recovery and Markdown conversion.
* `PRRC_kwDOAEM_Wc7PO25O`, `PRRC_kwDOAEM_Wc7PPfAw`: empty-geometry and spatial
  collection material should probably become advanced geometry deep dives after
  checking current behavior and examples.
* `PRRC_kwDOAEM_Wc7PPpX5`, `PRRC_kwDOAEM_Wc7PPqUS`: distance-calculation pages
  need image recovery and reconciliation with current distance code.
* `PRRC_kwDOAEM_Wc7PPrPX`: `SomeSplitting` should be compressed to only ideas
  not already implemented.
* `PRRC_kwDOAEM_Wc7PPr_S`: tolerance notes need comparison with current GEOS,
  MVT, and PostGIS tolerance behavior before conversion.

## Testing, Tools, and CI

* `PRRC_kwDOAEM_Wc7POtFt`, `PRRC_kwDOAEM_Wc7POtRS`: CI notes need a standards
  decision. If a maintained CI inventory stays in repo docs, update the
  associated scripts to check or regenerate it.

## Windows and Packaging

* `PRRC_kwDOAEM_Wc7PPIhX`, `PRRC_kwDOAEM_Wc7PPgkg`: Windows/MinGW setup needs a
  current development-environment section and deduplication against newer
  Windows notes.
* `PRRC_kwDOAEM_Wc7PPjxk`: old MinGW/MSYS 1.4/1.5 material needs a uniqueness
  audit before deciding whether to retire it.
* `PRRC_kwDOAEM_Wc7PQEne`: old Ubuntu 11.10 package notes need an EOL audit and
  should probably be retired unless a still-useful packaging pattern remains.

## User Manual Candidates

* `PRRC_kwDOAEM_Wc7PP3Lh`: check whether the user wiki FAQ is already covered by
  the manual or website FAQ.
* `PRRC_kwDOAEM_Wc7PP4bz`: batch shapefile loading on Windows needs comparison
  with the current loader manual.
* `PRRC_kwDOAEM_Wc7PP6Zj`: buffer-unbuffer coastline simplification may be worth
  a user-manual example after checking the referenced source.
* `PRRC_kwDOAEM_Wc7PP8cp`: invalid-geometry table scanning is a user tip; decide
  whether it belongs near validity functions rather than in the archive corpus.
* `PRRC_kwDOAEM_Wc7PQAPn`: line interpolation with offset needs comparison with
  current linear-referencing examples.
* `PRRC_kwDOAEM_Wc7PQBv7`: spike removal is not clearly equivalent to
  `ST_SimplifyVW`; compare against simplify and validity functions before
  retiring or rewriting.
* `PRRC_kwDOAEM_Wc7PQCiO`: hex-grid notes should point to built-in grid
  functions and mention that H3 support now lives in the `h3-pg` umbrella
  project.

## Documentation Backlog

The following ideas were extracted from `GoogleSeasonDocs2019` before leaving
that historical planning page on Trac:

* Review manual prose for consistent style, spelling, and function parameter
  names.
* Decide whether the core manual or workshop material needs workbook-style
  paths for geometry/geography, topology, and raster workflows. Route workshop
  changes to <https://github.com/postgis/postgis-workshops>.
* Improve SQL example markup by separating SQL program listings from query
  output where practical, so formatting and validation can treat them
  differently.
* Decide whether DocBook XML source formatting should have a stronger
  line-width convention for cleaner diffs.

## Project History and Planning

* `PRRC_kwDOAEM_Wc7POwKG`, `PRRC_kwDOAEM_Wc7PO3dX`: decide how historical
  development discussions and meeting/event notes should be preserved if Trac is
  eventually retired.
* `PRRC_kwDOAEM_Wc7PPsd_`, `PRRC_kwDOAEM_Wc7PPtGb`: keep the FOSS4G sprint note
  on Trac or in history, but extract any not-yet-implemented ideas into a
  maintained list.
* `PRRC_kwDOAEM_Wc7PPvW1`: `PostGIS3` should be pruned to ideas that were not
  completed.
* `PRRC_kwDOAEM_Wc7PPw2H`, `PRRC_kwDOAEM_Wc7PPyUi`: the obsolete versions matrix
  needs either a clearer format or a generator/range-based compatibility view.

## Raster Deep Dives

* `PRRC_kwDOAEM_Wc7PPzvo`: check whether `WKTRaster_Documentation01` was merged
  into the raster manual before deciding whether to keep any source material.
* `PRRC_kwDOAEM_Wc7PP04s`: the GDAL driver specification looks like useful
  raster deep-dive material, but it may belong in a future geometry/topology/
  raster deep-dive split rather than this broad migration commit.
