-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-- 
-- $Id$
--
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.refractions.net
-- Copyright 2001-2003 Refractions Research Inc.
-- Copyright 2009 Open Source Geospatial Foundation <http://osgeo.org>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--  
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- NOTE: This file lists items in reverse section order so that
--       dependency issues are not encountered during removal.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include "sqldefines.h"

BEGIN;

---------------------------------------------------------------
-- USER CONTRIUBUTED
---------------------------------------------------------------

DROP FUNCTION ST_MinimumBoundingCircle(geometry);
DROP FUNCTION ST_MinimumBoundingCircle(inputgeom geometry, segs_per_quarter integer);
DROP FUNCTION ST_ShortestLine(geometry, geometry);
DROP FUNCTION ST_LongestLine(geometry, geometry);
DROP FUNCTION ST_DFullyWithin(geometry, geometry, float8);
DROP FUNCTION _ST_DumpPoints(geometry, integer[]);

---------------------------------------------------------------
-- SQL-MM
---------------------------------------------------------------

DROP FUNCTION ST_LineToCurve(geometry);
DROP FUNCTION ST_HasArc(geometry);
DROP FUNCTION ST_CurveToLine(geometry);
DROP FUNCTION ST_CurveToLine(geometry, integer);

------------------------------------------------------------------------
-- OGC defined
------------------------------------------------------------------------

#include "uninstall_sqlmm.sql.in.c"
#include "uninstall_long_xact.sql.in.c"
#include "uninstall_geography.sql.in.c"

DROP FUNCTION ST_BdMPolyFromText(text, integer);
DROP FUNCTION BdMPolyFromText(text, integer);
DROP FUNCTION ST_BdPolyFromText(text, integer);
DROP FUNCTION BdPolyFromText(text, integer);
DROP FUNCTION ST_GeomCollFromWKB(bytea);
DROP FUNCTION GeomCollFromWKB(bytea);
DROP FUNCTION ST_GeomCollFromWKB(bytea, int);
DROP FUNCTION GeomCollFromWKB(bytea, int);
DROP FUNCTION ST_MultiPolyFromWKB(bytea);
DROP FUNCTION MultiPolyFromWKB(bytea);
DROP FUNCTION ST_MultiPolyFromWKB(bytea, int);
DROP FUNCTION MultiPolyFromWKB(bytea, int);
DROP FUNCTION ST_MPolyFromWKB(bytea);
DROP FUNCTION MPolyFromWKB(bytea);
DROP FUNCTION ST_MPolyFromWKB(bytea, int);
DROP FUNCTION MPolyFromWKB(bytea, int);
DROP FUNCTION ST_MLineFromWKB(bytea);
DROP FUNCTION MLineFromWKB(bytea);
DROP FUNCTION ST_MLineFromWKB(bytea, int);
DROP FUNCTION MLineFromWKB(bytea, int);
DROP FUNCTION ST_MultiLineFromWKB(bytea);
DROP FUNCTION MultiLineFromWKB(bytea);
DROP FUNCTION MultiLineFromWKB(bytea, int);
DROP FUNCTION ST_MultiPointFromWKB(bytea);
DROP FUNCTION MultiPointFromWKB(bytea);
DROP FUNCTION ST_MultiPointFromWKB(bytea, int);
DROP FUNCTION MultiPointFromWKB(bytea, int);
DROP FUNCTION ST_MPointFromWKB(bytea);
DROP FUNCTION MPointFromWKB(bytea);
DROP FUNCTION ST_MPointFromWKB(bytea, int);
DROP FUNCTION MPointFromWKB(bytea, int);
DROP FUNCTION ST_PolygonFromWKB(bytea);
DROP FUNCTION PolygonFromWKB(bytea);
DROP FUNCTION ST_PolygonFromWKB(bytea, int);
DROP FUNCTION PolygonFromWKB(bytea, int);
DROP FUNCTION ST_PolyFromWKB(bytea);
DROP FUNCTION PolyFromWKB(bytea);
DROP FUNCTION ST_PolyFromWKB(bytea, int);
DROP FUNCTION PolyFromWKB(bytea, int);
DROP FUNCTION ST_LinestringFromWKB(bytea);
DROP FUNCTION LinestringFromWKB(bytea);
DROP FUNCTION ST_LinestringFromWKB(bytea, int);
DROP FUNCTION LinestringFromWKB(bytea, int);
DROP FUNCTION ST_LineFromWKB(bytea);
DROP FUNCTION LineFromWKB(bytea);
DROP FUNCTION ST_LineFromWKB(bytea, int);
DROP FUNCTION LineFromWKB(bytea, int);
DROP FUNCTION ST_PointFromWKB(bytea);
DROP FUNCTION PointFromWKB(bytea);
DROP FUNCTION ST_PointFromWKB(bytea, int);
DROP FUNCTION PointFromWKB(bytea, int);
DROP FUNCTION ST_GeomFromWKB(bytea, int);
DROP FUNCTION GeomFromWKB(bytea, int);
DROP FUNCTION ST_GeomFromWKB(bytea);
DROP FUNCTION GeomFromWKB(bytea);
DROP FUNCTION ST_GeomCollFromText(text);
DROP FUNCTION GeomCollFromText(text);
DROP FUNCTION ST_GeomCollFromText(text, int4);
DROP FUNCTION GeomCollFromText(text, int4);
DROP FUNCTION ST_MultiPolygonFromText(text);
DROP FUNCTION MultiPolygonFromText(text);
DROP FUNCTION ST_MultiPolygonFromText(text, int4);
DROP FUNCTION MultiPolygonFromText(text, int4);
DROP FUNCTION ST_MPolyFromText(text);
DROP FUNCTION MPolyFromText(text);
DROP FUNCTION ST_MPolyFromText(text, int4);
DROP FUNCTION MPolyFromText(text, int4);
DROP FUNCTION ST_MultiPointFromText(text);
DROP FUNCTION MultiPointFromText(text);
DROP FUNCTION MultiPointFromText(text, int4);
DROP FUNCTION ST_MPointFromText(text);
DROP FUNCTION MPointFromText(text);
DROP FUNCTION ST_MPointFromText(text, int4);
DROP FUNCTION MPointFromText(text, int4);
DROP FUNCTION ST_MultiLineStringFromText(text, int4);
DROP FUNCTION MultiLineStringFromText(text, int4);
DROP FUNCTION ST_MultiLineStringFromText(text);
DROP FUNCTION MultiLineStringFromText(text);
DROP FUNCTION ST_MLineFromText(text);
DROP FUNCTION MLineFromText(text);
DROP FUNCTION ST_MLineFromText(text, int4);
DROP FUNCTION MLineFromText(text, int4);
DROP FUNCTION ST_PolygonFromText(text);
DROP FUNCTION PolygonFromText(text);
DROP FUNCTION ST_PolygonFromText(text, int4);
DROP FUNCTION PolygonFromText(text, int4);
DROP FUNCTION ST_PolyFromText(text, int4);
DROP FUNCTION PolyFromText(text, int4);
DROP FUNCTION ST_PolyFromText(text);
DROP FUNCTION PolyFromText(text);
DROP FUNCTION LineStringFromText(text, int4);
DROP FUNCTION LineStringFromText(text);
DROP FUNCTION ST_LineFromText(text, int4);
DROP FUNCTION LineFromText(text, int4);
DROP FUNCTION ST_LineFromText(text);
DROP FUNCTION LineFromText(text);
DROP FUNCTION ST_PointFromText(text, int4);
DROP FUNCTION PointFromText(text, int4);
DROP FUNCTION ST_PointFromText(text);
DROP FUNCTION PointFromText(text);
DROP FUNCTION ST_GeomFromText(text, int4);
DROP FUNCTION GeomFromText(text, int4);
DROP FUNCTION ST_GeomFromText(text);
DROP FUNCTION GeomFromText(text);
DROP FUNCTION ST_GeometryFromText(text, int4);
DROP FUNCTION GeometryFromText(text, int4);
DROP FUNCTION ST_GeometryFromText(text);
DROP FUNCTION GeometryFromText(text);
DROP FUNCTION ST_AsText(geometry);
DROP FUNCTION AsText(geometry);
DROP FUNCTION ST_AsBinary(geometry,text);
DROP FUNCTION AsBinary(geometry,text);
DROP FUNCTION ST_AsBinary(geometry);
DROP FUNCTION AsBinary(geometry);
DROP FUNCTION ST_SetSRID(geometry,int4);
DROP FUNCTION SetSRID(geometry,int4);
DROP FUNCTION ST_SRID(geometry);
DROP FUNCTION SRID(geometry);
DROP FUNCTION ST_IsEmpty(geometry);
DROP FUNCTION IsEmpty(geometry);
DROP FUNCTION ST_IsClosed(geometry);
DROP FUNCTION IsClosed(geometry);
DROP FUNCTION ST_EndPoint(geometry);
DROP FUNCTION EndPoint(geometry);
DROP FUNCTION ST_StartPoint(geometry);
DROP FUNCTION StartPoint(geometry);
DROP FUNCTION ST_M(geometry);
DROP FUNCTION M(geometry);
DROP FUNCTION ST_Z(geometry);
DROP FUNCTION Z(geometry);
DROP FUNCTION ST_Y(geometry);
DROP FUNCTION Y(geometry);
DROP FUNCTION ST_X(geometry);
DROP FUNCTION X(geometry);
DROP FUNCTION ST_PointN(geometry,integer);
DROP FUNCTION PointN(geometry,integer);
DROP FUNCTION ST_GeometryType(geometry);
DROP FUNCTION GeometryType(geometry);
DROP FUNCTION ST_InteriorRingN(geometry,integer);
DROP FUNCTION InteriorRingN(geometry,integer);
DROP FUNCTION ST_NumInteriorRing(geometry);
DROP FUNCTION NumInteriorRing(geometry);
DROP FUNCTION ST_NumInteriorRings(geometry);
DROP FUNCTION NumInteriorRings(geometry);
DROP FUNCTION ST_ExteriorRing(geometry);
DROP FUNCTION ExteriorRing(geometry);
DROP FUNCTION ST_Dimension(geometry);
DROP FUNCTION Dimension(geometry);
DROP FUNCTION ST_GeometryN(geometry,integer);
DROP FUNCTION GeometryN(geometry,integer);
DROP FUNCTION ST_NumGeometries(geometry);
DROP FUNCTION NumGeometries(geometry);
DROP FUNCTION ST_NumPoints(geometry);
DROP FUNCTION NumPoints(geometry);

------------------------------------------------------------------------
-- GeoHash (geohash.org)
------------------------------------------------------------------------

DROP FUNCTION ST_GeoHash(geometry);
DROP FUNCTION ST_GeoHash(geometry, int4);

-----------------------------------------------------------------------
-- GEOJSON OUTPUT
-----------------------------------------------------------------------

DROP FUNCTION ST_AsGeoJson(int4, geometry, int4, int4);
DROP FUNCTION ST_AsGeoJson(geometry, int4, int4);
DROP FUNCTION ST_AsGeoJson(int4, geometry, int4);
DROP FUNCTION ST_AsGeoJson(int4, geometry);
DROP FUNCTION ST_AsGeoJson(geometry);
DROP FUNCTION ST_AsGeoJson(geometry, int4);
DROP FUNCTION _ST_AsGeoJson(int4, geometry, int4, int4);

-----------------------------------------------------------------------
-- KML OUTPUT
-----------------------------------------------------------------------

DROP FUNCTION ST_AsKML(int4, geometry, int4);
DROP FUNCTION ST_AsKML(int4, geometry);
DROP FUNCTION ST_AsKML(geometry);
DROP FUNCTION AsKML(int4, geometry, int4);
DROP FUNCTION AsKML(geometry);
DROP FUNCTION ST_AsKML(geometry, int4);
DROP FUNCTION AsKML(geometry, int4);
DROP FUNCTION _ST_AsKML(int4, geometry, int4);

-----------------------------------------------------------------------
-- GML OUTPUT
-----------------------------------------------------------------------

DROP FUNCTION ST_AsGML(int4, geometry, int4, int4);
DROP FUNCTION ST_AsGML(geometry, int4, int4);
DROP FUNCTION ST_AsGML(int4, geometry, int4);
DROP FUNCTION ST_AsGML(int4, geometry);
DROP FUNCTION ST_AsGML(geometry);
DROP FUNCTION AsGML(geometry);
DROP FUNCTION ST_AsGML(geometry, int4);
DROP FUNCTION AsGML(geometry, int4);
DROP FUNCTION _ST_AsGML(int4, geometry, int4, int4);
DROP FUNCTION ST_GeomFromGML(text);
DROP FUNCTION ST_GMLToSQL(text);
DROP FUNCTION ST_GeomFromKML(text);

-----------------------------------------------------------------------
-- SVG OUTPUT
-----------------------------------------------------------------------

DROP FUNCTION ST_AsSVG(geometry);
DROP FUNCTION AsSVG(geometry);
DROP FUNCTION ST_AsSVG(geometry,int4);
DROP FUNCTION AsSVG(geometry,int4);
DROP FUNCTION ST_AsSVG(geometry,int4,int4);
DROP FUNCTION AsSVG(geometry,int4,int4);

--------------------------------------------------------------------------------

DROP FUNCTION ST_Equals(geometry,geometry);
DROP FUNCTION Equals(geometry,geometry);
DROP FUNCTION ST_IsSimple(geometry);
DROP FUNCTION IsSimple(geometry);
DROP FUNCTION ST_PointOnSurface(geometry);
DROP FUNCTION PointOnSurface(geometry);
DROP FUNCTION ST_IsRing(geometry);
DROP FUNCTION IsRing(geometry);
DROP FUNCTION ST_Centroid(geometry);
DROP FUNCTION Centroid(geometry);
DROP FUNCTION ST_IsValid(geometry);
DROP FUNCTION IsValid(geometry);
DROP FUNCTION ST_Overlaps(geometry,geometry);
DROP FUNCTION _ST_Overlaps(geometry,geometry);
DROP FUNCTION overlaps(geometry,geometry);
DROP FUNCTION ST_ContainsProperly(geometry,geometry);
DROP FUNCTION _ST_ContainsProperly(geometry,geometry);
DROP FUNCTION ST_Covers(geometry,geometry);
DROP FUNCTION _ST_Covers(geometry,geometry);
DROP FUNCTION ST_CoveredBy(geometry,geometry);
DROP FUNCTION _ST_CoveredBy(geometry,geometry);

DROP FUNCTION ST_Contains(geometry,geometry);
DROP FUNCTION _ST_Contains(geometry,geometry);
DROP FUNCTION Contains(geometry,geometry);
DROP FUNCTION ST_Within(geometry,geometry);
DROP FUNCTION _ST_Within(geometry,geometry);
DROP FUNCTION within(geometry,geometry);
DROP FUNCTION ST_Crosses(geometry,geometry);
DROP FUNCTION _ST_Crosses(geometry,geometry);
DROP FUNCTION crosses(geometry,geometry);
DROP FUNCTION ST_Intersects(geometry,geometry);
DROP FUNCTION _ST_Intersects(geometry,geometry);
DROP FUNCTION intersects(geometry,geometry);
DROP FUNCTION ST_DWithin(geometry, geometry, float8);
DROP FUNCTION _ST_DWithin(geometry,geometry,float8);
DROP FUNCTION ST_Touches(geometry,geometry);
DROP FUNCTION _ST_Touches(geometry,geometry);
DROP FUNCTION touches(geometry,geometry);
DROP FUNCTION ST_Disjoint(geometry,geometry);
DROP FUNCTION disjoint(geometry,geometry);
DROP FUNCTION ST_Relate(geometry,geometry,text);
DROP FUNCTION relate(geometry,geometry,text);
DROP FUNCTION ST_relate(geometry,geometry);
DROP FUNCTION relate(geometry,geometry);

--------------------------------------------------------------------------------
-- Aggregates and their supporting functions
--------------------------------------------------------------------------------

DROP AGGREGATE ST_MakeLine(geometry);
DROP AGGREGATE makeline(geometry);
DROP AGGREGATE ST_Polygonize(geometry);
DROP AGGREGATE Polygonize(geometry);
DROP AGGREGATE ST_Collect(geometry);
DROP AGGREGATE collect(geometry);
DROP AGGREGATE ST_Union(geometry);

DROP FUNCTION ST_Union (geometry[]);
DROP FUNCTION ST_unite_garray (geometry[]);
DROP FUNCTION unite_garray (geometry[]);

DROP AGGREGATE ST_Accum(geometry);
DROP AGGREGATE accum(geometry);

DROP FUNCTION pgis_geometry_makeline_finalfn(pgis_abs);
DROP FUNCTION pgis_geometry_polygonize_finalfn(pgis_abs);
DROP FUNCTION pgis_geometry_collect_finalfn(pgis_abs);
DROP FUNCTION pgis_geometry_union_finalfn(pgis_abs);
DROP FUNCTION pgis_geometry_accum_finalfn(pgis_abs);
DROP FUNCTION pgis_geometry_accum_transfn(pgis_abs, geometry);
-- This drops pgis_abs_in, pgis_abs_out and the type in an atomic fashion
DROP TYPE pgis_abs CASCADE;

DROP AGGREGATE ST_MemUnion(geometry);
DROP AGGREGATE MemGeomUnion(geometry);
DROP FUNCTION ST_collect(geometry[]);
DROP AGGREGATE ST_memcollect(geometry);
DROP AGGREGATE memcollect(geometry);
DROP FUNCTION ST_collect(geometry, geometry);
DROP FUNCTION collect(geometry, geometry);


---------------------------------------------------------------
-- GEOS
---------------------------------------------------------------

DROP FUNCTION ST_Union(geometry,geometry);
DROP FUNCTION GeomUnion(geometry,geometry);
DROP FUNCTION ST_symmetricdifference(geometry,geometry);
DROP FUNCTION symmetricdifference(geometry,geometry);
DROP FUNCTION ST_SymDifference(geometry,geometry);
DROP FUNCTION symdifference(geometry,geometry);
DROP FUNCTION ST_Boundary(geometry);
DROP FUNCTION boundary(geometry);
DROP FUNCTION ST_Difference(geometry,geometry);
DROP FUNCTION difference(geometry,geometry);
DROP FUNCTION ST_IsValidReason(geometry);
DROP FUNCTION ST_SimplifyPreserveTopology(geometry, float8);
DROP FUNCTION ST_LocateBetweenElevations(geometry, float8, float8);
DROP FUNCTION ST_LineCrossingDirection(geometry, geometry);
DROP FUNCTION _ST_LineCrossingDirection(geometry, geometry);
DROP FUNCTION ST_ConvexHull(geometry);
DROP FUNCTION convexhull(geometry);
DROP FUNCTION ST_buffer(geometry,float8,integer);
DROP FUNCTION buffer(geometry,float8,integer);
DROP FUNCTION ST_Buffer(geometry,float8);
DROP FUNCTION buffer(geometry,float8);
DROP FUNCTION ST_buffer(geometry,float8,text);
DROP FUNCTION _ST_buffer(geometry,float8,cstring);
DROP FUNCTION ST_Intersection(geometry,geometry);
DROP FUNCTION intersection(geometry,geometry);
#if POSTGIS_GEOS_VERSION >= 32
DROP FUNCTION ST_HausdorffDistance(geometry, geometry);
DROP FUNCTION ST_HausdorffDistance(geometry, geometry, float8);
#endif


---------------------------------------------------------------
-- LRS
---------------------------------------------------------------

DROP FUNCTION ST_locate_along_measure(geometry, float8);
DROP FUNCTION locate_along_measure(geometry, float8);
DROP FUNCTION ST_locate_between_measures(geometry, float8, float8);
DROP FUNCTION locate_between_measures(geometry, float8, float8);
DROP FUNCTION ST_line_locate_point(geometry, geometry);
DROP FUNCTION line_locate_point(geometry, geometry);
DROP FUNCTION ST_line_substring(geometry, float8, float8);
DROP FUNCTION line_substring(geometry, float8, float8);
DROP FUNCTION ST_line_interpolate_point(geometry, float8);
DROP FUNCTION line_interpolate_point(geometry, float8);
DROP FUNCTION ST_AddMeasure(geometry, float8, float8);

---------------------------------------------------------------
-- Algorithms
---------------------------------------------------------------

DROP FUNCTION ST_Segmentize(geometry, float8);
DROP FUNCTION Segmentize(geometry, float8);
DROP FUNCTION ST_SnapToGrid(geometry, geometry, float8, float8, float8, float8);
DROP FUNCTION SnapToGrid(geometry, geometry, float8, float8, float8, float8);
DROP FUNCTION ST_SnapToGrid(geometry, float8);
DROP FUNCTION SnapToGrid(geometry, float8);
DROP FUNCTION ST_SnapToGrid(geometry, float8, float8);
DROP FUNCTION SnapToGrid(geometry, float8, float8);
DROP FUNCTION ST_SnapToGrid(geometry, float8, float8, float8, float8);
DROP FUNCTION SnapToGrid(geometry, float8, float8, float8, float8);
DROP FUNCTION ST_Simplify(geometry, float8);
DROP FUNCTION Simplify(geometry, float8);


---------------------------------------------------------------
-- CASTS
---------------------------------------------------------------

DROP CAST (box3d_extent AS geometry);
DROP CAST (box3d_extent AS box2d);
DROP CAST (box3d_extent AS box3d);

DROP CAST (geometry AS bytea);
DROP CAST (bytea AS geometry);
DROP CAST (chip AS geometry);
DROP CAST (geometry AS text);
DROP CAST (text AS geometry);
DROP CAST (box3d AS geometry);
DROP CAST (box3d AS box);
DROP CAST (box2d AS geometry);
DROP CAST (box2d AS box3d);
DROP CAST (box3d AS box2d);
DROP CAST (geometry AS box);
DROP CAST (geometry AS box3d);
DROP CAST (geometry AS box2d);

DROP FUNCTION ST_bytea(geometry);
DROP FUNCTION bytea(geometry);
DROP FUNCTION ST_geometry(bytea);
DROP FUNCTION geometry(bytea);
DROP FUNCTION ST_geometry(chip);
DROP FUNCTION geometry(chip);
DROP FUNCTION ST_geometry(text);
DROP FUNCTION geometry(text);
DROP FUNCTION ST_geometry(box3d);
DROP FUNCTION geometry(box3d);
DROP FUNCTION ST_geometry(box2d);
DROP FUNCTION geometry(box2d);
DROP FUNCTION box3dtobox(box3d);
DROP FUNCTION ST_text(geometry);
DROP FUNCTION text(geometry);
DROP FUNCTION ST_box(box3d);
DROP FUNCTION box(box3d);
DROP FUNCTION ST_box3d(box2d);
DROP FUNCTION box3d(box2d);
DROP FUNCTION ST_box2d(box3d);
DROP FUNCTION box2d(box3d);
DROP FUNCTION ST_box(geometry);
DROP FUNCTION box(geometry);
DROP FUNCTION ST_box3d(geometry);
DROP FUNCTION box3d(geometry);
DROP FUNCTION ST_box2d(geometry);
DROP FUNCTION box2d(geometry);


-----------------------------------------------------------------------
-- POSTGIS_VERSION()
-----------------------------------------------------------------------

DROP FUNCTION postgis_full_version();
DROP FUNCTION postgis_lib_version();
DROP FUNCTION postgis_version();
DROP FUNCTION postgis_lib_build_date();
DROP FUNCTION postgis_scripts_build_date();
DROP FUNCTION postgis_geos_version();
DROP FUNCTION postgis_libxml_version();
DROP FUNCTION postgis_uses_stats();
DROP FUNCTION postgis_scripts_released();
DROP FUNCTION postgis_scripts_installed();
DROP FUNCTION postgis_proj_version();

---------------------------------------------------------------
-- PROJ support
---------------------------------------------------------------

DROP FUNCTION ST_Transform(geometry,integer);
DROP FUNCTION transform(geometry,integer);
DROP FUNCTION get_proj4_from_srid(integer);


-------------------------------------------------------------------
-- GEOMETRY_COLUMNS
-------------------------------------------------------------------

DROP FUNCTION find_srid(varchar,varchar,varchar);
DROP FUNCTION UpdateGeometrySRID(varchar,varchar,integer);
DROP FUNCTION UpdateGeometrySRID(varchar,varchar,varchar,integer);
DROP FUNCTION UpdateGeometrySRID(varchar,varchar,varchar,varchar,integer);
DROP FUNCTION DropGeometryTable(varchar);
DROP FUNCTION DropGeometryTable(varchar,varchar);
DROP FUNCTION DropGeometryTable(varchar, varchar,varchar);
DROP FUNCTION DropGeometryColumn(varchar,varchar);
DROP FUNCTION DropGeometryColumn(varchar,varchar,varchar);
DROP FUNCTION DropGeometryColumn(varchar, varchar,varchar,varchar);
DROP FUNCTION AddGeometryColumn(varchar,varchar,integer,varchar,integer);
DROP FUNCTION AddGeometryColumn(varchar,varchar,varchar,integer,varchar,integer);
DROP FUNCTION AddGeometryColumn(varchar,varchar,varchar,varchar,integer,varchar,integer);
DROP FUNCTION probe_geometry_columns();
DROP FUNCTION populate_geometry_columns(oid);
DROP FUNCTION populate_geometry_columns();
DROP FUNCTION fix_geometry_columns();
DROP FUNCTION rename_geometry_table_constraints();

DROP TABLE geometry_columns;


-------------------------------------------------------------------
-- SPATIAL_REF_SYS
-------------------------------------------------------------------

-- Currently leave this for upgrade purposes
DROP TABLE spatial_ref_sys;


--
-- Aggregate functions
--

DROP FUNCTION ST_find_extent(text,text);
DROP FUNCTION find_extent(text,text);
DROP FUNCTION ST_find_extent(text,text,text);
DROP FUNCTION find_extent(text,text,text);
DROP FUNCTION ST_estimated_extent(text,text);
DROP FUNCTION estimated_extent(text,text);
DROP FUNCTION ST_estimated_extent(text,text,text);
DROP FUNCTION estimated_extent(text,text,text);
DROP AGGREGATE ST_Extent3d(geometry);
DROP AGGREGATE Extent3d(geometry);
DROP FUNCTION ST_Combine_BBox(box3d,geometry);
DROP FUNCTION combine_bbox(box3d,geometry);
DROP AGGREGATE ST_Extent(geometry);
DROP AGGREGATE Extent(geometry);
DROP FUNCTION ST_Combine_BBox(box3d_extent,geometry);
DROP FUNCTION combine_bbox(box3d_extent,geometry);
DROP FUNCTION ST_Combine_BBox(box2d,geometry);
DROP FUNCTION combine_bbox(box2d,geometry);


------------------------------------------------------------------------
-- CONSTRUCTORS
------------------------------------------------------------------------

DROP FUNCTION ST_DumpRings(geometry);
DROP FUNCTION DumpRings(geometry);
DROP FUNCTION ST_Dump(geometry);
DROP FUNCTION Dump(geometry);
DROP FUNCTION ST_LineMerge(geometry);
DROP FUNCTION LineMerge(geometry);
DROP FUNCTION ST_Polygonize (geometry[]);
DROP FUNCTION ST_Polygonize_GArray (geometry[]);
DROP FUNCTION Polygonize_GArray (geometry[]);
DROP FUNCTION ST_BuildArea(geometry);
DROP FUNCTION BuildArea(geometry);
DROP FUNCTION ST_MakePolygon(geometry);
DROP FUNCTION MakePolygon(geometry);
DROP FUNCTION ST_MakePolygon(geometry, geometry[]);
DROP FUNCTION MakePolygon(geometry, geometry[]);
DROP FUNCTION ST_SetPoint(geometry, integer, geometry);
DROP FUNCTION SetPoint(geometry, integer, geometry);
DROP FUNCTION ST_RemovePoint(geometry, integer);
DROP FUNCTION RemovePoint(geometry, integer);
DROP FUNCTION ST_AddPoint(geometry, geometry, integer);
DROP FUNCTION AddPoint(geometry, geometry, integer);
DROP FUNCTION ST_AddPoint(geometry, geometry);
DROP FUNCTION AddPoint(geometry, geometry);
DROP FUNCTION ST_MakeLine(geometry, geometry);
DROP FUNCTION MakeLine(geometry, geometry);
DROP FUNCTION ST_LineFromMultiPoint(geometry);
DROP FUNCTION LineFromMultiPoint(geometry);
DROP FUNCTION ST_MakeLine (geometry[]);
DROP FUNCTION ST_MakeLine_GArray (geometry[]);
DROP FUNCTION makeline_garray (geometry[]);
DROP FUNCTION ST_MakeBox3d(geometry, geometry);
DROP FUNCTION MakeBox3d(geometry, geometry);
DROP FUNCTION ST_MakeBox2d(geometry, geometry);
DROP FUNCTION MakeBox2d(geometry, geometry);
DROP FUNCTION ST_MakePointM(float8, float8, float8);
DROP FUNCTION MakePointM(float8, float8, float8);
DROP FUNCTION ST_MakePoint(float8, float8, float8, float8);
DROP FUNCTION MakePoint(float8, float8, float8, float8);
DROP FUNCTION ST_MakePoint(float8, float8, float8);
DROP FUNCTION MakePoint(float8, float8, float8);
DROP FUNCTION ST_MakePoint(float8, float8);
DROP FUNCTION MakePoint(float8, float8);


------------------------------------------------------------------------
-- MISC
------------------------------------------------------------------------

DROP FUNCTION ST_GeomFromEWKT(text);
DROP FUNCTION GeomFromEWKT(text);
DROP FUNCTION ST_GeomFromEWKB(bytea);
DROP FUNCTION GeomFromEWKB(bytea);
DROP FUNCTION ST_AsEWKB(geometry,text);
DROP FUNCTION AsEWKB(geometry,text);
DROP FUNCTION ST_AsHEXEWKB(geometry, text);
DROP FUNCTION AsHEXEWKB(geometry, text);
DROP FUNCTION ST_AsHEXEWKB(geometry);
DROP FUNCTION AsHEXEWKB(geometry);
DROP FUNCTION ST_AsEWKB(geometry);
DROP FUNCTION AsEWKB(geometry);
DROP FUNCTION ST_AsEWKT(geometry);
DROP FUNCTION AsEWKT(geometry);
DROP FUNCTION ST_NDims(geometry);
DROP FUNCTION ndims(geometry);
DROP FUNCTION hasBBOX(geometry);
DROP FUNCTION postgis_cache_bbox();
DROP FUNCTION ST_zmflag(geometry);
DROP FUNCTION zmflag(geometry);
DROP FUNCTION noop(geometry);
DROP FUNCTION ST_ForceRHR(geometry);
DROP FUNCTION ForceRHR(geometry);
DROP FUNCTION ST_Reverse(geometry);
DROP FUNCTION reverse(geometry);
DROP FUNCTION ST_Envelope(geometry);
DROP FUNCTION envelope(geometry);
DROP FUNCTION ST_expand(geometry,float8);
DROP FUNCTION expand(geometry,float8);
DROP FUNCTION ST_expand(box2d,float8);
DROP FUNCTION expand(box2d,float8);
DROP FUNCTION ST_Expand(box3d,float8);
DROP FUNCTION expand(box3d,float8);
DROP FUNCTION ST_multi(geometry);
DROP FUNCTION multi(geometry);
DROP FUNCTION ST_force_collection(geometry);
DROP FUNCTION force_collection(geometry);
DROP FUNCTION ST_force_4d(geometry);
DROP FUNCTION force_4d(geometry);
DROP FUNCTION ST_force_3dm(geometry);
DROP FUNCTION force_3dm(geometry);
DROP FUNCTION ST_force_3d(geometry);
DROP FUNCTION force_3d(geometry);
DROP FUNCTION ST_force_3dz(geometry);
DROP FUNCTION force_3dz(geometry);
DROP FUNCTION ST_force_2d(geometry);
DROP FUNCTION force_2d(geometry);

------------------------------------------------------------------------
-- Misures
------------------------------------------------------------------------

DROP FUNCTION ST_azimuth(geometry,geometry);
DROP FUNCTION azimuth(geometry,geometry);
DROP FUNCTION ST_point_inside_circle(geometry,float8,float8,float8);
DROP FUNCTION point_inside_circle(geometry,float8,float8,float8);
DROP FUNCTION _ST_MaxDistance(geometry,geometry);
DROP FUNCTION ST_maxdistance(geometry,geometry);
DROP FUNCTION max_distance(geometry,geometry);
DROP FUNCTION ST_Distance(geometry,geometry);
DROP FUNCTION distance(geometry,geometry);
DROP FUNCTION ST_distance_sphere(geometry,geometry);
DROP FUNCTION distance_sphere(geometry,geometry);
DROP FUNCTION ST_distance_spheroid(geometry,geometry,spheroid);
DROP FUNCTION distance_spheroid(geometry,geometry,spheroid);
DROP FUNCTION ST_Area(geometry);
DROP FUNCTION area(geometry);
DROP FUNCTION ST_area2d(geometry);
DROP FUNCTION area2d(geometry);
DROP FUNCTION ST_Perimeter(geometry);
DROP FUNCTION perimeter(geometry);
DROP FUNCTION ST_perimeter2d(geometry);
DROP FUNCTION perimeter2d(geometry);
DROP FUNCTION ST_perimeter3d(geometry);
DROP FUNCTION perimeter3d(geometry);
DROP FUNCTION ST_length2d_spheroid(geometry, spheroid);
DROP FUNCTION length2d_spheroid(geometry, spheroid);
DROP FUNCTION ST_length_spheroid(geometry, spheroid);
DROP FUNCTION length_spheroid(geometry, spheroid);
DROP FUNCTION ST_length3d_spheroid(geometry, spheroid);
DROP FUNCTION length3d_spheroid(geometry, spheroid);
DROP FUNCTION ST_Length(geometry);
DROP FUNCTION length(geometry);
DROP FUNCTION ST_length2d(geometry);
DROP FUNCTION length2d(geometry);
DROP FUNCTION ST_length3d(geometry);
DROP FUNCTION length3d(geometry);


------------------------------------------------------------------------
-- DEBUG
------------------------------------------------------------------------

DROP FUNCTION ST_nrings(geometry);
DROP FUNCTION nrings(geometry);
DROP FUNCTION ST_npoints(geometry);
DROP FUNCTION npoints(geometry);
DROP FUNCTION ST_summary(geometry);
DROP FUNCTION summary(geometry);
DROP FUNCTION ST_mem_size(geometry);
DROP FUNCTION mem_size(geometry);


-------------------------------------------
-- other lwgeom functions
-------------------------------------------

DROP FUNCTION getBBOX(geometry);
DROP FUNCTION getSRID(geometry);

-------------------------------------------
-- GIST opclass index binding entries.
-------------------------------------------

DROP OPERATOR CLASS gist_geometry_ops USING gist CASCADE;

-- gist support functions
DROP FUNCTION LWGEOM_gist_decompress(internal);
DROP FUNCTION LWGEOM_gist_same(box2d, box2d, internal);
DROP FUNCTION LWGEOM_gist_union(bytea, internal);
DROP FUNCTION LWGEOM_gist_picksplit(internal, internal);
DROP FUNCTION LWGEOM_gist_penalty(internal,internal,internal);
DROP FUNCTION LWGEOM_gist_compress(internal);
DROP FUNCTION LWGEOM_gist_consistent(internal,geometry,int4);

-- GEOMETRY operators

DROP OPERATOR ~ (geometry,geometry);
DROP OPERATOR @ (geometry,geometry);
DROP OPERATOR ~= (geometry,geometry);
DROP OPERATOR |>> (geometry,geometry);
DROP OPERATOR |&> (geometry,geometry);
DROP OPERATOR >> (geometry,geometry);
DROP OPERATOR &> (geometry,geometry);
DROP OPERATOR && (geometry,geometry);
DROP OPERATOR &<| (geometry,geometry);
DROP OPERATOR <<| (geometry,geometry);
DROP OPERATOR &< (geometry,geometry);
DROP OPERATOR << (geometry,geometry);


-------------------------------------------------------------------
-- GiST indexes
-------------------------------------------------------------------

DROP FUNCTION ST_geometry_same(geometry, geometry);
DROP FUNCTION geometry_same(geometry, geometry);
DROP FUNCTION ST_geometry_overlap(geometry, geometry);
DROP FUNCTION geometry_overlap(geometry, geometry);
DROP FUNCTION ST_geometry_contained(geometry, geometry);
DROP FUNCTION geometry_contained(geometry, geometry);
DROP FUNCTION ST_geometry_contain(geometry, geometry);
DROP FUNCTION geometry_contain(geometry, geometry);
DROP FUNCTION ST_geometry_below(geometry, geometry);
DROP FUNCTION geometry_below(geometry, geometry);
DROP FUNCTION ST_geometry_above(geometry, geometry);
DROP FUNCTION geometry_above(geometry, geometry);
DROP FUNCTION ST_geometry_right(geometry, geometry);
DROP FUNCTION geometry_right(geometry, geometry);
DROP FUNCTION ST_geometry_left(geometry, geometry);
DROP FUNCTION geometry_left(geometry, geometry);
DROP FUNCTION ST_geometry_overbelow(geometry, geometry);
DROP FUNCTION geometry_overbelow(geometry, geometry);
DROP FUNCTION ST_geometry_overabove(geometry, geometry);
DROP FUNCTION geometry_overabove(geometry, geometry);
DROP FUNCTION ST_geometry_overright(geometry, geometry);
DROP FUNCTION geometry_overright(geometry, geometry);
DROP FUNCTION ST_geometry_overleft(geometry, geometry);
DROP FUNCTION geometry_overleft(geometry, geometry);
DROP FUNCTION ST_postgis_gist_joinsel(internal, oid, internal, smallint);
DROP FUNCTION postgis_gist_joinsel(internal, oid, internal, smallint);
DROP FUNCTION ST_postgis_gist_sel (internal, oid, internal, int4);
DROP FUNCTION postgis_gist_sel (internal, oid, internal, int4);


--
-- Sorting operators for Btree
--

DROP OPERATOR CLASS btree_geometry_ops USING btree;
DROP OPERATOR > (geometry,geometry);
DROP OPERATOR >= (geometry,geometry);
DROP OPERATOR = (geometry,geometry);
DROP OPERATOR <= (geometry,geometry);
DROP OPERATOR < (geometry,geometry);


-------------------------------------------------------------------
-- BTREE indexes
-------------------------------------------------------------------

DROP FUNCTION geometry_cmp(geometry, geometry);
DROP FUNCTION geometry_eq(geometry, geometry);
DROP FUNCTION geometry_ge(geometry, geometry);
DROP FUNCTION geometry_gt(geometry, geometry);
DROP FUNCTION geometry_le(geometry, geometry);
DROP FUNCTION geometry_lt(geometry, geometry);



-----------------------------------------------------------------------
-- BOX2D
-----------------------------------------------------------------------

-- This drops ST_box2d_in, ST_box2d_out, box2d_in, box2d_out and the type in an atomic fashion
DROP TYPE box2d CASCADE;



-------------------------------------------------------------------
--  BOX3D TYPE
-------------------------------------------------------------------


DROP FUNCTION ST_ZMax(box3d);
DROP FUNCTION zmax(box3d);
DROP FUNCTION ST_YMax(box3d);
DROP FUNCTION ymax(box3d);
DROP FUNCTION ST_XMax(box3d);
DROP FUNCTION xmax(box3d);
DROP FUNCTION ST_ZMin(box3d);
DROP FUNCTION zmin(box3d);
DROP FUNCTION ST_YMin(box3d);
DROP FUNCTION ymin(box3d);
DROP FUNCTION ST_XMin(box3d);
DROP FUNCTION xmin(box3d);

-- This drops box3d_extent_in, box3d_extent_out and the type in an atomic fashion
DROP TYPE box3d_extent CASCADE;

-- This drops ST_box3d_in, ST_box3d_out, box3d_in, box3d_out and the type in an atomic fashion
DROP TYPE box3d CASCADE;


-------------------------------------------
-- Affine transforms
-------------------------------------------

DROP FUNCTION ST_shift_longitude(geometry);
DROP FUNCTION shift_longitude(geometry);
DROP FUNCTION ST_transscale(geometry,float8,float8,float8,float8);
DROP FUNCTION transscale(geometry,float8,float8,float8,float8);
DROP FUNCTION ST_Scale(geometry,float8,float8);
DROP FUNCTION Scale(geometry,float8,float8);
DROP FUNCTION ST_Scale(geometry,float8,float8,float8);
DROP FUNCTION Scale(geometry,float8,float8,float8);
DROP FUNCTION ST_Translate(geometry,float8,float8);
DROP FUNCTION Translate(geometry,float8,float8);
DROP FUNCTION ST_Translate(geometry,float8,float8,float8);
DROP FUNCTION Translate(geometry,float8,float8,float8);
DROP FUNCTION ST_RotateY(geometry,float8);
DROP FUNCTION RotateY(geometry,float8);
DROP FUNCTION ST_RotateX(geometry,float8);
DROP FUNCTION RotateX(geometry,float8);
DROP FUNCTION ST_Rotate(geometry,float8);
DROP FUNCTION Rotate(geometry,float8);
DROP FUNCTION ST_RotateZ(geometry,float8);
DROP FUNCTION RotateZ(geometry,float8);
DROP FUNCTION ST_Affine(geometry,float8,float8,float8,float8,float8,float8);
DROP FUNCTION Affine(geometry,float8,float8,float8,float8,float8,float8);
DROP FUNCTION ST_Affine(geometry,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8);
DROP FUNCTION Affine(geometry,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8);


-------------------------------------------------------------------
--  GEOMETRY TYPE (geometry_dump)
-------------------------------------------------------------------
DROP TYPE geometry_dump CASCADE;

-------------------------------------------------------------------
--  GEOMETRY TYPE (lwgeom)
-------------------------------------------------------------------

-- This drops ST_geometry_send, geometry_send, ST_geometry_recv, geometry_recv,
-- ST_geometry_out, geometry_out, ST_geometry_in, geometry_in and the type in an atomic fashion

DROP TYPE geometry CASCADE;

DROP FUNCTION geometry_analyze(internal);
DROP FUNCTION geometry_gist_sel (internal, oid, internal, int4);
DROP FUNCTION geometry_gist_joinsel(internal, oid, internal, smallint);

-------------------------------------------------------------------
--  SPHEROID TYPE
-------------------------------------------------------------------

-- This drops ST_spheroid_out, spheroid_out, ST_spheroid_in, spheroid_in and the type in an atomic fashion
DROP TYPE spheroid CASCADE;


COMMIT;
