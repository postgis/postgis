-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-- 
-- $Id$
--
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.refractions.net
-- Copyright 2001-2003 Refractions Research Inc.
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--  
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-- This file defines a subset of SQL/MM functions (that is, only those
-- currently defined by ESRI's ArcSDE). Since these functions already exist
-- in PostGIS (for the most part), these functions simply expose the current
-- functions. Although mostly complying with SQL/MM standards, these prototypes
-- follow ESRI's ArcSDE SQL Specifications and not SQL/MM standards where 
-- disparities exist.
--
-- Specification Disparity Notes:
--   * ST_OrderingEquals(geometry, geometry) is implemented as per
--     ESRI's ArcSDE SQL specifications, not SQL/MM specifications.
--     (http://edndoc.esri.com/arcsde/9.1/sql_api/sqlapi3.htm#ST_OrderingEquals)
--   * Geometry constructors default to an SRID of -1, not 0 as per SQL/MM specs.
--   * Boolean return type methods (ie. ST_IsValid, ST_IsEmpty, ...)
--      * SQL/MM           : RETURNS 1 if TRUE, 0 if (FALSE, NULL)
--      * ESRI in Informix : RETURNS 1 if (TRUE, NULL), 0 if FALSE
--      * ESRI in DB2      : RETURNS 1 if TRUE, 0 if FALSE, NULL if NULL 
--      * PostGIS          : RETURNS 1 if TRUE, 0 if FALSE, NULL if NULL 
--
-- TODO: Implement ESRI's Shape constructors
--   * SE_AsShape(geometry)
--   * SE_ShapeToSQL
--   * SE_GeomFromShape
--   * SE_PointFromShape
--   * SE_LineFromShape
--   * SE_PolyFromShape
--   * SE_MPointFromShape
--   * SE_MLineFromShape
--   * SE_MPolyFromShape
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include "sqldefines.h"

-------------------------------------------------------------------------------
-- SQL/MM (ArcSDE subset) - SQL Functions for constructing an ST_Geometry
--     value given its WTK representation
-- (http://edndoc.esri.com/arcsde/9.1/general_topics/storing_geo_in_rdbms.html)
-------------------------------------------------------------------------------

-- PostGIS equivalent function: ST_GeometryFromText(text)
-- Note: Defaults to an SRID=-1, not 0 as per SQL/MM specs.
CREATE OR REPLACE FUNCTION ST_WKTToSQL(text)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_from_text'
	LANGUAGE 'C' IMMUTABLE STRICT; 

-- ST_GeomFromText(text, int4) - already defined
-- ST_PointFromText(text, int4) - already defined
-- ST_LineFromText(text, int4) - already defined
-- ST_PolyFromText(text, int4) - already defined
-- ST_MPointFromText(text, int4) - already defined
-- ST_MLineFromText(text, int4) - already defined
-- ST_MPolyFromText(text, int4) - already defined

-------------------------------------------------------------------------------
-- SQL/MM (ArcSDE subset) - SQL Functions for constructing an ST_Geometry
--     value given its WKB representation
-------------------------------------------------------------------------------

-- PostGIS equivalent function: GeomFromWKB(bytea))
-- Note: Defaults to an SRID=-1, not 0 as per SQL/MM specs.

CREATE OR REPLACE FUNCTION ST_WKBToSQL(bytea)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_from_WKB'
	LANGUAGE 'C' IMMUTABLE STRICT; 

-- ST_GeomFromWKB(bytea, int) - already defined
-- ST_PointFromWKB(bytea, int) - already defined
-- ST_LineFromWKB(bytea, int) - already defined
-- ST_PolyFromWKB(bytea, int) - already defined
-- ST_MPointFromWKB(bytea, int) - already defined
-- ST_MLineFromWKB(bytea, int) - already defined
-- ST_MPolyFromWKB(bytea, int) - already defined

-------------------------------------------------------------------------------
-- SQL/MM (ArcSDE subset) - SQL Functions for constructing an ST_Geometry
--     value given an ESRI Shape representation
-------------------------------------------------------------------------------

-- TODO: SE_ShapeToSQL
-- TODO: SE_GeomFromShape
-- TODO: SE_PointFromShape
-- TODO: SE_LineFromShape
-- TODO: SE_PolyFromShape
-- TODO: SE_MPointFromShape
-- TODO: SE_MLineFromShape
-- TODO: SE_MPolyFromShape

-------------------------------------------------------------------------------
-- SQL/MM (ArcSDE subset) - SQL Functions for obtaining the WKT representation
--     of an ST_Geometry
-------------------------------------------------------------------------------

-- ST_AsText(geometry) - already defined

-------------------------------------------------------------------------------
-- SQL/MM (ArcSDE subset) - SQL Functions for obtaining the WKB representation
--     of an ST_Geometry
-------------------------------------------------------------------------------

-- ST_AsBinary(geometry) - already defined

-------------------------------------------------------------------------------
-- SQL/MM (ArcSDE subset) - SQL Functions for obtaining the ESRI Shape 
-- representation of an ST_Geometry
-------------------------------------------------------------------------------

-- TODO: SE_AsShape(geometry)
--CREATE OR REPLACE FUNCTION SE_AsShape(geometry)
--    RETURNS bytea
--    AS 'MODULE_PATHNAME','LWGEOM_AsShape'
--    LANGUAGE 'C' IMMUTABLE STRICT; 

-------------------------------------------------------------------------------
-- SQL/MM (ArcSDE subset) - SQL Functions on type ST_Geometry
-------------------------------------------------------------------------------

-- PostGIS equivalent function: ndims(geometry)
CREATE OR REPLACE FUNCTION ST_CoordDim(geometry)
	RETURNS smallint
	AS 'MODULE_PATHNAME', 'LWGEOM_ndims'
	LANGUAGE 'C' IMMUTABLE STRICT; 

-- ST_Dimension(geometry) - already defined.
-- ST_GeometryType(geometry) - already defined.
-- ST_SRID(geometry) - already defined.
-- ST_IsEmpty(geometry) - already defined.
-- ST_IsSimple(geometry) - already defined.
-- ST_IsValid(geometry) - already defined.
-- ST_Boundary(geometry) - already defined.
-- ST_Envelope(geometry) - already defined.
-- ST_Transform(geometry) - already defined.
-- ST_AsText(geometry) - already defined.
-- ST_AsBinary(geometry) - already defined.
-- SE_AsShape(geometry) - already defined.
-- ST_X(geometry) - already defined.
-- ST_Y(geometry) - already defined.

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION _ST_OrderingEquals(geometry, geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'LWGEOM_same'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.3.0
CREATE OR REPLACE FUNCTION ST_OrderingEquals(geometry, geometry)
	RETURNS boolean
	AS $$ 
	SELECT $1 ~= $2 AND _ST_OrderingEquals($1, $2)
	$$	
	LANGUAGE 'SQL' IMMUTABLE STRICT; 

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION SE_Is3D(geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_hasz'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION SE_IsMeasured(geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_hasm'
	LANGUAGE 'C' IMMUTABLE STRICT;

-------------------------------------------------------------------------------
-- SQL/MM (ArcSDE subset) - SQL Functions on type ST_Point
-------------------------------------------------------------------------------

-- PostGIS equivalent function: makePoint(float8,float8)
CREATE OR REPLACE FUNCTION ST_Point(float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint'
	LANGUAGE 'C' IMMUTABLE STRICT; 

-- PostGIS equivalent function: Z(geometry)
CREATE OR REPLACE FUNCTION SE_Z(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_z_point'
	LANGUAGE 'C' IMMUTABLE STRICT; 

-- PostGIS equivalent function: M(geometry)
CREATE OR REPLACE FUNCTION SE_M(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_m_point'
	LANGUAGE 'C' IMMUTABLE STRICT; 

-------------------------------------------------------------------------------
-- SQL/MM (ArcSDE subset) - SQL Functions on type ST_Curve
-------------------------------------------------------------------------------

-- ST_StartPoint(geometry) - already defined.
-- ST_EndPoint(geometry) - already defined.
-- ST_IsClosed(geometry) - already defined.
-- ST_IsRing(geometry) - already defined.
-- ST_Length(geometry) - already defined.

-------------------------------------------------------------------------------
-- SQL/MM (ArcSDE subset) - SQL Functions on type ST_LineString
-------------------------------------------------------------------------------

-- ST_NumPoints(geometry) - already defined.
-- ST_PointN(geometry) - already defined.

-------------------------------------------------------------------------------
-- SQL/MM (ArcSDE subset) - SQL Functions on type ST_Surface
-------------------------------------------------------------------------------

-- ST_Centroid(geometry) - already defined.
-- ST_PointOnSurface(geometry) - already defined.
-- ST_Area(geometry) - already defined.
-- ST_Perimeter(geometry) - already defined.

-------------------------------------------------------------------------------
-- SQL/MM (ArcSDE subset) - SQL Functions on type ST_Polygon
-------------------------------------------------------------------------------

-- PostGIS equivalent function: MakePolygon(geometry)
CREATE OR REPLACE FUNCTION ST_Polygon(geometry, int)
	RETURNS geometry
	AS $$ 
	SELECT setSRID(makepolygon($1), $2)
	$$	
	LANGUAGE 'SQL' IMMUTABLE STRICT; 

-- ST_ExteriorRing(geometry) - already defined.
-- ST_NumInteriorRing(geometry) - already defined.
-- ST_InteriorRingN(geometry, integer) - already defined.

-------------------------------------------------------------------------------
-- SQL/MM (ArcSDE subset) - SQL Functions on type ST_GeomCollection
-------------------------------------------------------------------------------

-- ST_NumGeometries(geometry) - already defined.
-- ST_GeometryN(geometry, integer) - already defined.

-------------------------------------------------------------------------------
-- SQL/MM (ArcSDE subset) - SQL Functions on type ST_MultiCurve
-------------------------------------------------------------------------------

-- ST_IsClosed(geometry) - already defined.
-- ST_Length(geometry) - already defined.

-------------------------------------------------------------------------------
-- SQL/MM (ArcSDE subset) - SQL Functions on type ST_MultiSurface
-------------------------------------------------------------------------------

-- ST_Centroid(geometry) - already defined.
-- ST_PointOnSurface(geometry) - already defined.
-- ST_Area(geometry) - already defined.
-- ST_Perimeter(geometry) - already defined.

-------------------------------------------------------------------------------
-- SQL/MM (ArcSDE subset) - SQL Functions that test spatial relationships
-------------------------------------------------------------------------------

-- ST_Equals(geometry, geometry) - already defined.
-- ST_Disjoint(geometry, geometry) - already defined.
-- ST_Touches(geometry, geometry) - already defined.
-- ST_Within(geometry, geometry) - already defined.
-- ST_Overlaps(geometry, geometry) - already defined.
-- ST_Crosses(geometry, geometry) - already defined.
-- ST_Intersects(geometry, geometry) - already defined.
-- ST_Contains(geometry, geometry) - already defined.
-- ST_Relate(geometry, geometry, text) - already defined.

-- PostGIS equivalent function: none
CREATE OR REPLACE FUNCTION SE_EnvelopesIntersect(geometry,geometry)
	RETURNS boolean
	AS $$ 
	SELECT $1 && $2
	$$	
	LANGUAGE 'SQL' IMMUTABLE STRICT; 

-------------------------------------------------------------------------------
-- SQL/MM (ArcSDE subset) - SQL Functions for distance relationships
-------------------------------------------------------------------------------

-- ST_Distance(geometry, geometry) - already defined.

-------------------------------------------------------------------------------
-- SQL/MM (ArcSDE subset) - SQL Functions that implement spatial operators
-------------------------------------------------------------------------------

-- ST_Intersection(geometry, geometry) - already defined.
-- ST_Difference(geometry, geometry) - already defined.
-- ST_Union(geometry, geometry) - already defined.
-- ST_SymDifference(geometry, geometry) - already defined.
-- ST_Buffer(geometry, float8) - already defined.
-- ST_ConvexHull(geometry) already defined.

-- PostGIS equivalent function: locate_between_measures(geometry, float8, float8)
CREATE OR REPLACE FUNCTION SE_LocateBetween(geometry, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_locate_between_m'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- PostGIS equivalent function: locate_along_measure(geometry, float8)
CREATE OR REPLACE FUNCTION SE_LocateAlong(geometry, float8)
	RETURNS geometry
	AS $$ SELECT SE_LocateBetween($1, $2, $2) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;



-------------------------------------------------------------------------------
-- END
-------------------------------------------------------------------------------

