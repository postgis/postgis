---
title: "PostGIS Raster And The GDAL Driver"
date: 2026-06-26
weight: 80
geekdocHidden: false
---

The PostGIS Raster SQL type, loader, catalog views, and server-side GDAL use live
in this repository. The `PostGISRaster` GDAL driver itself lives in GDAL, so
developer documentation here should explain the boundary and the PostGIS-side
contracts rather than republish old GDAL driver working notes as current code
documentation.

The old Trac `WKTRaster/GDALDriverSpecificationWorking`,
`WKTRaster/ThirdPartySupport`, and `WKTRasterGSoC` pages are useful mostly as
provenance for that boundary. Their current lessons are:

* GDAL opens PostGIS raster data through the external `PostGISRaster` driver,
  not through code maintained in this repository.
* PostGIS must expose enough metadata through raster catalogs, constraints, and
  SQL functions for external clients to discover raster tables, overviews,
  georeferencing, band metadata, alignment, and out-db storage.
* The driver-facing table shape still has two important read modes: one raster
  per row for tables containing independent rasters, and one raster per table
  for tiled coverages.
* Tiled coverage performance depends on loader and database choices: tiling,
  overviews, spatial indexes, and registered constraints.
* Alignment and regular blocking are data properties that should be validated
  by PostGIS constraints/functions, not guessed from stale loader assumptions.

## Current Sources Of Truth

PostGIS developers should check these repository surfaces when changing raster
behavior that could affect GDAL clients:

* `raster/loader/raster2pgsql.c` for loader options such as tiling,
  overviews, spatial indexes, constraints, and out-db loading.
* `raster/rt_pg/rtpostgis.sql.in` for the SQL declarations behind
  `raster_columns`, `raster_overviews`, `AddRasterConstraints`,
  `AddOverviewConstraints`, `ST_SameAlignment`, `ST_NotSameAlignmentReason`,
  `ST_FromGDALRaster`, `ST_AsGDALRaster`, and `ST_GDALDrivers`.
* `raster/rt_core/rt_util.c` for server-side GDAL registration, driver
  filtering through `postgis.gdal_enabled_drivers`, VSICURL handling, raster
  data type conversion, and GDAL open calls used by PostGIS itself.
* `doc/using_raster_dataman.xml` for maintained user documentation on loading
  rasters, out-db/cloud rasters, raster catalogs, and constraints.
* `doc/reference_raster.xml` for the SQL reference entries that client authors
  depend on.
* The GDAL documentation for the external `PostGISRaster` driver when checking
  connection-string parameters, GDAL tool behavior, and driver limitations:
  <https://gdal.org/en/stable/drivers/raster/postgisraster.html>.

The current GDAL driver documentation lists the `PG:` connection string with
`schema`, `table`, `column`, `where`, `mode`, and `outdb_resolution` fields. It
documents `mode=1` as one raster per row and `mode=2` as one raster per table,
and recommends loading tiled rasters with `raster2pgsql -t`, overviews with
`-l`, a GiST index with `-I`, and registered constraints with `-C`.

## PostGIS-Side Contracts

When a PostGIS raster change touches storage, loader output, catalog metadata,
or raster accessors, check whether external GDAL clients can still answer these
questions:

* Which schema, table, and raster column should be opened?
* Is the table an overview of another raster table?
* Are the raster tiles aligned, and is there registered metadata proving that?
* What extent, scale, skew, SRID, band count, pixel type, nodata value, and
  out-db path metadata will clients see?
* Does the raster table support the tiled-coverage assumptions needed by
  `mode=2`?
* Are out-db rasters safe to resolve server-side, client-side, or only through
  the explicit GDAL driver option?

The durable PostGIS-side answer should be a function, catalog entry,
constraint, or manual page in this repository. Driver-specific connection
behavior belongs upstream in GDAL.

## Retired Trac Working-Spec Details

Do not carry forward the 2012 working page, the incomplete third-party support
outline, or the old GSoC write-support idea as maintained instructions for
current contributors. They reference old GDAL 1.9/1.10 status, obsolete
beta-era arrangement labels, tentative write support, hard-coded `rid`
discussion, and implementation details of the external driver that are no
longer owned here. Keep only the boundary checklist above unless a fresh design
ticket needs a deeper GDAL/PostGIS integration proposal.
