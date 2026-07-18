---
title: "Raster storage and WKB"
date: 2026-06-26
weight: 70
geekdocHidden: false
---

PostGIS Raster has three related representations that are easy to confuse:

* The PostgreSQL `raster` type is a variable-length value declared in
  `raster/rt_pg/rtpostgis.sql.in`. Its SQL input and output functions accept
  and emit HexWKB.
* The in-memory core object is `struct rt_raster_t`, with one or more
  `rt_band_t` entries owned by the raster.
* The on-disk PostgreSQL value is the serialized form built by
  `rt_raster_serialize()` and read by `rt_raster_deserialize()`.

The old Trac `WKTRaster/RFC/RFC1_V0SerialFormat`,
`WKTRaster/RFC/RFC2_V0WKBFormat`, `WKTRaster/RFC/RFC3_V1SerialFormat`, and
`WKTRaster/Specification*` pages describe early versions of those formats.
They are useful as implementation history, but current work should start from
the source files named below.

## Serialized PostgreSQL Value

`struct rt_raster_serialized_t` in `raster/rt_core/librtcore.h` is the fixed
header copied into the PostgreSQL varlena value. It stores the PostgreSQL size,
format version, band count, affine georeferencing coefficients, SRID, width,
and height.

After the header, `rt_raster_serialize()` writes each band as:

* one byte combining pixel type and band flags such as out-db, has-nodata, and
  is-nodata;
* padding to the pixel-type width used by the serialized form;
* the nodata value encoded in the band's pixel type;
* either the in-db pixel data or an out-db band number plus null-terminated
  path;
* trailing padding to the next eight-byte boundary.

`rt_raster_deserialize()` reads the same layout. It can also read only the
header, which is used by code paths that need dimensions or georeferencing
without touching all band data.

## WKB And HexWKB

`raster_in` and `raster_out`, implemented by `RASTER_in` and `RASTER_out` in
`raster/rt_pg/rtpg_inout.c`, use HexWKB for the SQL type's textual input and
output. `ST_AsWKB`, `ST_AsBinary`, `ST_AsHexWKB`, `ST_RastFromWKB`, and
`ST_RastFromHexWKB` are implemented through `raster/rt_pg/rtpg_wkb.c`.

The WKB reader and writer live in `raster/rt_core/rt_wkb.c`. WKB starts with an
endianness byte and version number, then writes the raster header fields from
band count through dimensions. Bands then carry the same pixel type, nodata,
out-db, and pixel-data concepts as the serialized value, but WKB does not use
the PostgreSQL varlena size field or the serialized-form padding.

The current WKB version accepted by `rt_raster_from_wkb()` is version 0. Any
change to these fields is a storage and wire-format compatibility change, so it
must be reviewed with upgrade behavior, regression fixtures, and external
clients in mind.

## Out-Db Bands

Out-db bands are bands whose pixels are read from a filesystem path rather than
stored inline in the PostgreSQL value. In the core band struct, those bands use
`rt_extband_t` with a zero-based source band number and path. SQL-visible
helpers such as `ST_BandPath`, `ST_BandFileSize`, `ST_BandFileTimestamp`, and
`ST_SetBandPath` live in `raster/rt_pg/rtpg_band_properties.c`.

Because an out-db path is data inside the raster value, code that serializes,
copies, tiles, or converts rasters must preserve whether each band is in-db or
out-db. Server-side access is also governed by the raster GDAL configuration
described in [PostGIS Raster and the GDAL driver](raster-gdal-driver.md).

## Raster Catalogs

`raster_columns` and `raster_overviews` are SQL views built from PostgreSQL
catalogs and PostGIS raster constraints. The durable metadata contract for
clients is the current SQL in `raster/rt_pg/rtpostgis.sql.in` plus the user
manual sections for raster catalogs, constraints, and overviews.

The old specification pages listed beta-era loader flags, prototype Python
scripts, and planned catalog tables. Current loader behavior belongs to
`raster/loader/raster2pgsql.c`, and current user-facing behavior belongs to the
raster manual.
