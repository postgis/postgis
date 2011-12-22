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
DROP FUNCTION ST_BdPolyFromText(text, integer);
DROP FUNCTION ST_GeomCollFromWKB(bytea);
DROP FUNCTION ST_GeomCollFromWKB(bytea, int);
DROP FUNCTION ST_MultiPolyFromWKB(bytea);
DROP FUNCTION ST_MultiPolyFromWKB(bytea, int);
DROP FUNCTION ST_MPolyFromWKB(bytea);
DROP FUNCTION ST_MPolyFromWKB(bytea, int);
DROP FUNCTION ST_MLineFromWKB(bytea);
DROP FUNCTION ST_MLineFromWKB(bytea, int);
DROP FUNCTION ST_MultiLineFromWKB(bytea);
DROP FUNCTION ST_MultiPointFromWKB(bytea);
DROP FUNCTION ST_MultiPointFromWKB(bytea, int);
DROP FUNCTION ST_MPointFromWKB(bytea);
DROP FUNCTION ST_MPointFromWKB(bytea, int);
DROP FUNCTION ST_PolygonFromWKB(bytea);
DROP FUNCTION ST_PolygonFromWKB(bytea, int);
DROP FUNCTION ST_PolyFromWKB(bytea);
DROP FUNCTION ST_PolyFromWKB(bytea, int);
DROP FUNCTION ST_LinestringFromWKB(bytea);
DROP FUNCTION ST_LinestringFromWKB(bytea, int);
DROP FUNCTION ST_LineFromWKB(bytea);
DROP FUNCTION ST_LineFromWKB(bytea, int);
DROP FUNCTION ST_PointFromWKB(bytea);
DROP FUNCTION ST_PointFromWKB(bytea, int);
DROP FUNCTION ST_GeomFromWKB(bytea, int);
DROP FUNCTION ST_GeomFromWKB(bytea);
DROP FUNCTION ST_GeomCollFromText(text);
DROP FUNCTION ST_GeomCollFromText(text, int4);
DROP FUNCTION ST_MultiPolygonFromText(text);
DROP FUNCTION ST_MultiPolygonFromText(text, int4);
DROP FUNCTION ST_MPolyFromText(text);
DROP FUNCTION ST_MPolyFromText(text, int4);
DROP FUNCTION ST_MultiPointFromText(text);
DROP FUNCTION ST_MPointFromText(text);
DROP FUNCTION ST_MPointFromText(text, int4);
DROP FUNCTION ST_MultiLineStringFromText(text, int4);
DROP FUNCTION ST_MultiLineStringFromText(text);
DROP FUNCTION ST_MLineFromText(text);
DROP FUNCTION ST_MLineFromText(text, int4);
DROP FUNCTION ST_PolygonFromText(text);
DROP FUNCTION ST_PolygonFromText(text, int4);
DROP FUNCTION ST_PolyFromText(text, int4);
DROP FUNCTION ST_PolyFromText(text);
DROP FUNCTION ST_LineFromText(text, int4);
DROP FUNCTION ST_LineFromText(text);
DROP FUNCTION ST_PointFromText(text, int4);
DROP FUNCTION ST_PointFromText(text);
DROP FUNCTION ST_GeomFromText(text, int4);
DROP FUNCTION ST_GeomFromText(text);
DROP FUNCTION ST_GeometryFromText(text, int4);
DROP FUNCTION ST_GeometryFromText(text);
DROP FUNCTION ST_AsText(geometry);
DROP FUNCTION ST_AsBinary(geometry,text);
DROP FUNCTION ST_AsBinary(geometry);
DROP FUNCTION ST_SetSRID(geometry,int4);
DROP FUNCTION ST_SRID(geometry);
DROP FUNCTION ST_IsEmpty(geometry);
DROP FUNCTION ST_IsClosed(geometry);
DROP FUNCTION ST_EndPoint(geometry);
DROP FUNCTION ST_StartPoint(geometry);
DROP FUNCTION ST_M(geometry);
DROP FUNCTION ST_Z(geometry);
DROP FUNCTION ST_Y(geometry);
DROP FUNCTION ST_X(geometry);
DROP FUNCTION ST_PointN(geometry,integer);
DROP FUNCTION ST_GeometryType(geometry);
DROP FUNCTION ST_InteriorRingN(geometry,integer);
DROP FUNCTION ST_NumInteriorRing(geometry);
DROP FUNCTION ST_NumInteriorRings(geometry);
DROP FUNCTION ST_ExteriorRing(geometry);
DROP FUNCTION ST_Dimension(geometry);
DROP FUNCTION ST_GeometryN(geometry,integer);
DROP FUNCTION ST_NumGeometries(geometry);
DROP FUNCTION ST_NumPoints(geometry);

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
DROP FUNCTION ST_AsKML(geometry, int4);

-----------------------------------------------------------------------
-- GML OUTPUT
-----------------------------------------------------------------------

DROP FUNCTION ST_AsGML(int4, geometry, int4, int4);
DROP FUNCTION ST_AsGML(geometry, int4, int4);
DROP FUNCTION ST_AsGML(int4, geometry, int4);
DROP FUNCTION ST_AsGML(int4, geometry);
DROP FUNCTION ST_AsGML(geometry);
DROP FUNCTION ST_AsGML(geometry, int4);
DROP FUNCTION _ST_AsGML(int4, geometry, int4, int4, text);
DROP FUNCTION ST_GeomFromGML(text);
DROP FUNCTION ST_GMLToSQL(text);
DROP FUNCTION ST_GeomFromKML(text);

-----------------------------------------------------------------------
-- SVG OUTPUT
-----------------------------------------------------------------------

DROP FUNCTION ST_AsSVG(geometry);
DROP FUNCTION ST_AsSVG(geometry,int4);
DROP FUNCTION ST_AsSVG(geometry,int4,int4);

--------------------------------------------------------------------------------

DROP FUNCTION ST_Equals(geometry,geometry);
DROP FUNCTION ST_IsSimple(geometry);
DROP FUNCTION ST_PointOnSurface(geometry);
DROP FUNCTION ST_IsRing(geometry);
DROP FUNCTION ST_Centroid(geometry);
DROP FUNCTION ST_IsValid(geometry);
DROP FUNCTION ST_Overlaps(geometry,geometry);
DROP FUNCTION _ST_Overlaps(geometry,geometry);
DROP FUNCTION ST_ContainsProperly(geometry,geometry);
DROP FUNCTION _ST_ContainsProperly(geometry,geometry);
DROP FUNCTION ST_Covers(geometry,geometry);
DROP FUNCTION _ST_Covers(geometry,geometry);
DROP FUNCTION ST_CoveredBy(geometry,geometry);
DROP FUNCTION _ST_CoveredBy(geometry,geometry);

DROP FUNCTION ST_Contains(geometry,geometry);
DROP FUNCTION _ST_Contains(geometry,geometry);
DROP FUNCTION ST_Within(geometry,geometry);
DROP FUNCTION _ST_Within(geometry,geometry);
DROP FUNCTION ST_Crosses(geometry,geometry);
DROP FUNCTION _ST_Crosses(geometry,geometry);
DROP FUNCTION ST_Intersects(geometry,geometry);
DROP FUNCTION _ST_Intersects(geometry,geometry);
DROP FUNCTION ST_DWithin(geometry, geometry, float8);
DROP FUNCTION _ST_DWithin(geometry,geometry,float8);
DROP FUNCTION ST_Touches(geometry,geometry);
DROP FUNCTION _ST_Touches(geometry,geometry);
DROP FUNCTION ST_Disjoint(geometry,geometry);
DROP FUNCTION ST_Relate(geometry,geometry,text);
DROP FUNCTION ST_relate(geometry,geometry);
DROP FUNCTION st_relatematch(text, text);

--------------------------------------------------------------------------------
-- Aggregates and their supporting functions
--------------------------------------------------------------------------------

DROP AGGREGATE ST_MakeLine(geometry);
DROP AGGREGATE ST_Polygonize(geometry);
DROP AGGREGATE ST_Collect(geometry);
DROP AGGREGATE ST_Union(geometry);

DROP FUNCTION ST_Union (geometry[]);

DROP AGGREGATE ST_Accum(geometry);

DROP FUNCTION pgis_geometry_makeline_finalfn(pgis_abs);
DROP FUNCTION pgis_geometry_polygonize_finalfn(pgis_abs);
DROP FUNCTION pgis_geometry_collect_finalfn(pgis_abs);
DROP FUNCTION pgis_geometry_union_finalfn(pgis_abs);
DROP FUNCTION pgis_geometry_accum_finalfn(pgis_abs);
DROP FUNCTION pgis_geometry_accum_transfn(pgis_abs, geometry);
-- This drops pgis_abs_in, pgis_abs_out and the type in an atomic fashion
DROP TYPE pgis_abs CASCADE;

DROP AGGREGATE ST_MemUnion(geometry);
DROP FUNCTION ST_collect(geometry[]);
DROP AGGREGATE ST_memcollect(geometry);
DROP FUNCTION ST_collect(geometry, geometry);


---------------------------------------------------------------
-- GEOS
---------------------------------------------------------------

DROP FUNCTION ST_Union(geometry,geometry);
DROP FUNCTION ST_symmetricdifference(geometry,geometry);
DROP FUNCTION ST_SymDifference(geometry,geometry);
DROP FUNCTION ST_Boundary(geometry);
DROP FUNCTION ST_Difference(geometry,geometry);
DROP FUNCTION ST_IsValidReason(geometry);
DROP FUNCTION ST_SimplifyPreserveTopology(geometry, float8);
DROP FUNCTION ST_LocateBetweenElevations(geometry, float8, float8);
DROP FUNCTION ST_LineCrossingDirection(geometry, geometry);
DROP FUNCTION _ST_LineCrossingDirection(geometry, geometry);
DROP FUNCTION ST_ConvexHull(geometry);
DROP FUNCTION ST_buffer(geometry,float8,integer);
DROP FUNCTION ST_Buffer(geometry,float8);
DROP FUNCTION ST_buffer(geometry,float8,text);
DROP FUNCTION _ST_buffer(geometry,float8,cstring);
DROP FUNCTION ST_Intersection(geometry,geometry);
#if POSTGIS_GEOS_VERSION >= 32
DROP FUNCTION ST_HausdorffDistance(geometry, geometry);
DROP FUNCTION ST_HausdorffDistance(geometry, geometry, float8);
#endif


---------------------------------------------------------------
-- LRS
---------------------------------------------------------------

DROP FUNCTION ST_locate_along_measure(geometry, float8);
DROP FUNCTION ST_locate_between_measures(geometry, float8, float8);
DROP FUNCTION ST_line_locate_point(geometry, geometry);
DROP FUNCTION ST_line_substring(geometry, float8, float8);
DROP FUNCTION ST_line_interpolate_point(geometry, float8);
DROP FUNCTION ST_AddMeasure(geometry, float8, float8);

---------------------------------------------------------------
-- Algorithms
---------------------------------------------------------------

DROP FUNCTION ST_Segmentize(geometry, float8);
DROP FUNCTION ST_SnapToGrid(geometry, geometry, float8, float8, float8, float8);
DROP FUNCTION ST_SnapToGrid(geometry, float8);
DROP FUNCTION ST_SnapToGrid(geometry, float8, float8);
DROP FUNCTION ST_SnapToGrid(geometry, float8, float8, float8, float8);
DROP FUNCTION ST_Simplify(geometry, float8);


---------------------------------------------------------------
-- CASTS
---------------------------------------------------------------

DROP CAST (geometry AS bytea);
DROP CAST (bytea AS geometry);
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
DROP FUNCTION AddGeometryColumn(varchar,varchar,integer,varchar,integer,boolean);
DROP FUNCTION AddGeometryColumn(varchar,varchar,varchar,integer,varchar,integer,boolean);
DROP FUNCTION AddGeometryColumn(varchar,varchar,varchar,varchar,integer,varchar,integer,boolean);
DROP FUNCTION populate_geometry_columns(oid,boolean);
DROP FUNCTION populate_geometry_columns(boolean);

DROP VIEW geometry_columns;


-------------------------------------------------------------------
-- SPATIAL_REF_SYS
-------------------------------------------------------------------

-- Currently leave this for upgrade purposes
DROP TABLE spatial_ref_sys;


--
-- Aggregate functions
--

DROP AGGREGATE ST_3DExtent(geometry);
DROP AGGREGATE ST_Extent(geometry);
DROP FUNCTION ST_find_extent(text,text);
DROP FUNCTION ST_find_extent(text,text,text);
DROP FUNCTION ST_estimated_extent(text,text);
DROP FUNCTION ST_estimated_extent(text,text,text);
DROP FUNCTION ST_Combine_BBox(box3d,geometry);
DROP FUNCTION ST_Combine_BBox(box2d,geometry);


------------------------------------------------------------------------
-- CONSTRUCTORS
------------------------------------------------------------------------

DROP FUNCTION ST_DumpRings(geometry);
DROP FUNCTION ST_Dump(geometry);
DROP FUNCTION ST_LineMerge(geometry);
DROP FUNCTION ST_Polygonize (geometry[]);
DROP FUNCTION ST_BuildArea(geometry);
DROP FUNCTION ST_MakePolygon(geometry);
DROP FUNCTION ST_MakePolygon(geometry, geometry[]);
DROP FUNCTION ST_SetPoint(geometry, integer, geometry);
DROP FUNCTION ST_RemovePoint(geometry, integer);
DROP FUNCTION ST_AddPoint(geometry, geometry, integer);
DROP FUNCTION ST_AddPoint(geometry, geometry);
DROP FUNCTION ST_MakeLine(geometry, geometry);
DROP FUNCTION ST_LineFromMultiPoint(geometry);
DROP FUNCTION ST_MakeLine (geometry[]);
DROP FUNCTION ST_3DMakeBox(geometry, geometry);
DROP FUNCTION ST_MakeBox2d(geometry, geometry);
DROP FUNCTION ST_MakePointM(float8, float8, float8);
DROP FUNCTION ST_MakePoint(float8, float8, float8, float8);
DROP FUNCTION ST_MakePoint(float8, float8, float8);
DROP FUNCTION ST_MakePoint(float8, float8);


------------------------------------------------------------------------
-- MISC
------------------------------------------------------------------------

DROP FUNCTION ST_GeomFromEWKT(text);
DROP FUNCTION ST_GeomFromEWKB(bytea);
DROP FUNCTION ST_AsEWKB(geometry,text);
DROP FUNCTION ST_AsHEXEWKB(geometry, text);
DROP FUNCTION ST_AsHEXEWKB(geometry);
DROP FUNCTION ST_AsEWKB(geometry);
DROP FUNCTION ST_AsEWKT(geometry);
DROP FUNCTION ST_NDims(geometry);
DROP FUNCTION ST_zmflag(geometry);
DROP FUNCTION ST_ForceRHR(geometry);
DROP FUNCTION ST_Reverse(geometry);
DROP FUNCTION ST_Envelope(geometry);
DROP FUNCTION ST_expand(geometry,float8);
DROP FUNCTION ST_expand(box2d,float8);
DROP FUNCTION ST_Expand(box3d,float8);
DROP FUNCTION ST_multi(geometry);
DROP FUNCTION ST_force_collection(geometry);
DROP FUNCTION ST_force_4d(geometry);
DROP FUNCTION ST_force_3dm(geometry);
DROP FUNCTION ST_force_3d(geometry);
DROP FUNCTION ST_force_3dz(geometry);
DROP FUNCTION ST_force_2d(geometry);

------------------------------------------------------------------------
-- Misures
------------------------------------------------------------------------

DROP FUNCTION ST_azimuth(geometry,geometry);
DROP FUNCTION ST_point_inside_circle(geometry,float8,float8,float8);
DROP FUNCTION _ST_MaxDistance(geometry,geometry);
DROP FUNCTION ST_maxdistance(geometry,geometry);
DROP FUNCTION ST_Distance(geometry,geometry);
DROP FUNCTION ST_distance_sphere(geometry,geometry);
DROP FUNCTION ST_distance_spheroid(geometry,geometry,spheroid);
DROP FUNCTION ST_Area(geometry);
DROP FUNCTION ST_area2d(geometry);
DROP FUNCTION ST_Perimeter(geometry);
DROP FUNCTION ST_perimeter2d(geometry);
DROP FUNCTION ST_3dperimeter(geometry);
DROP FUNCTION ST_length2d_spheroid(geometry, spheroid);
DROP FUNCTION ST_length_spheroid(geometry, spheroid);
DROP FUNCTION ST_3DLength_spheroid(geometry, spheroid);
DROP FUNCTION ST_Length(geometry);
DROP FUNCTION ST_length2d(geometry);
DROP FUNCTION ST_3DLength(geometry);


------------------------------------------------------------------------
-- DEBUG
------------------------------------------------------------------------

DROP FUNCTION ST_nrings(geometry);
DROP FUNCTION ST_npoints(geometry);
DROP FUNCTION ST_summary(geometry);
DROP FUNCTION ST_mem_size(geometry);


-------------------------------------------
-- other lwgeom functions
-------------------------------------------

DROP FUNCTION postgis_getBBOX(geometry);
DROP FUNCTION getSRID(geometry); 
DROP FUNCTION postgis_cache_bbox();
DROP FUNCTION postgis_constraint_dims(text, text, text);
DROP FUNCTION postgis_constraint_srid(text, text, text);
DROP FUNCTION postgis_constraint_type(text, text, text);
DROP FUNCTION postgis_type_name(character varying, integer, boolean);



-------------------------------------------
-- GIST opclass index binding entries.
-------------------------------------------

DROP OPERATOR FAMILY gist_geometry_ops_2d USING gist CASCADE;

-- gist support functions

DROP FUNCTION geometry_gist_decompress_2d(internal);
DROP FUNCTION geometry_gist_same_2d(geometry, geometry, internal);
DROP FUNCTION geometry_gist_union_2d(bytea, internal);
DROP FUNCTION geometry_gist_picksplit_2d(internal, internal);
DROP FUNCTION geometry_gist_penalty_2d(internal,internal,internal);
DROP FUNCTION geometry_gist_compress_2d(internal);
DROP FUNCTION geometry_gist_consistent_2d(internal,geometry,int4);

DROP OPERATOR FAMILY gist_geometry_ops_nd USING gist CASCADE;

DROP FUNCTION geometry_gist_compress_nd(internal);
DROP FUNCTION geometry_gist_decompress_nd(internal);
DROP FUNCTION geometry_gist_penalty_nd(internal, internal, internal);
DROP FUNCTION geometry_gist_picksplit_nd(internal, internal);
DROP FUNCTION geometry_gist_union_nd(bytea, internal);

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

DROP FUNCTION geometry_same(geometry, geometry);
DROP FUNCTION geometry_overlaps(geometry, geometry);
DROP FUNCTION geometry_within(geometry, geometry);
DROP FUNCTION geometry_contains(geometry, geometry);
DROP FUNCTION geometry_below(geometry, geometry);
DROP FUNCTION geometry_above(geometry, geometry);
DROP FUNCTION geometry_right(geometry, geometry);
DROP FUNCTION geometry_left(geometry, geometry);
DROP FUNCTION geometry_overbelow(geometry, geometry);
DROP FUNCTION geometry_overabove(geometry, geometry);
DROP FUNCTION geometry_overright(geometry, geometry);
DROP FUNCTION geometry_overleft(geometry, geometry);
DROP FUNCTION geometry_gist_joinsel_2d(internal, oid, internal, smallint);
DROP FUNCTION geometry_gist_sel_2d (internal, oid, internal, int4);


--
-- Sorting operators for Btree
--

DROP OPERATOR CLASS btree_geometry_ops USING btree;
DROP OPERATOR FAMILY btree_geometry_ops USING btree;
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

-----------------------------------------------------------------------
-- BOX2Df
-----------------------------------------------------------------------

-- This drops box2df_in, box2df_out and the type in an atomic fashion
DROP TYPE box2df CASCADE;


-------------------------------------------------------------------
--  BOX3D TYPE
-------------------------------------------------------------------


DROP FUNCTION ST_ZMax(box3d);
DROP FUNCTION ST_YMax(box3d);
DROP FUNCTION ST_XMax(box3d);
DROP FUNCTION ST_ZMin(box3d);
DROP FUNCTION ST_YMin(box3d);
DROP FUNCTION ST_XMin(box3d);

-- This drops ST_box3d_in, ST_box3d_out, box3d_in, box3d_out and the type in an atomic fashion
DROP TYPE box3d CASCADE;


-------------------------------------------
-- Affine transforms
-------------------------------------------

DROP FUNCTION ST_shift_longitude(geometry);
DROP FUNCTION ST_transscale(geometry,float8,float8,float8,float8);
DROP FUNCTION ST_Scale(geometry,float8,float8);
DROP FUNCTION ST_Scale(geometry,float8,float8,float8);
DROP FUNCTION ST_Translate(geometry,float8,float8);
DROP FUNCTION ST_Translate(geometry,float8,float8,float8);
DROP FUNCTION ST_RotateY(geometry,float8);
DROP FUNCTION ST_RotateX(geometry,float8);
DROP FUNCTION ST_Rotate(geometry,float8);
DROP FUNCTION ST_RotateZ(geometry,float8);
DROP FUNCTION ST_Affine(geometry,float8,float8,float8,float8,float8,float8);
DROP FUNCTION ST_Affine(geometry,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8);

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

-- typmod stuff 

DROP FUNCTION geometry_analyze(internal);
DROP FUNCTION geometry_typmod_in(cstring[]);
DROP FUNCTION geometry_typmod_out(integer);

DROP FUNCTION postgis_typmod_dims(integer);
DROP FUNCTION postgis_typmod_srid(integer);
DROP FUNCTION postgis_typmod_type(integer);

-------------------------------------------------------------------
--  SPHEROID TYPE
-------------------------------------------------------------------

-- This drops ST_spheroid_out, spheroid_out, ST_spheroid_in, spheroid_in and the type in an atomic fashion
DROP TYPE spheroid CASCADE;

-------------------------------------------------------------------
--  valid_detail
-------------------------------------------------------------------

DROP TYPE valid_detail CASCADE;

COMMIT;
