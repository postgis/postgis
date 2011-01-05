#include "sqldefines.h"
--- start functions that in theory should never have been used or internal like stuff deprecated

-- these were superceded by PostGIS_AddBBOX , PostGIS_DropBBOX, PostGIS_HasBBOX in 1.5 --
CREATE OR REPLACE FUNCTION addbbox(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_addBBOX'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
CREATE OR REPLACE FUNCTION dropbbox(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_dropBBOX'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION hasbbox(geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_hasBBOX'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION noop(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_noop'
	LANGUAGE 'C' VOLATILE STRICT;
	
--- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_box2d(geometry)
	RETURNS box2d
	AS 'MODULE_PATHNAME','geometry2box2d'
	LANGUAGE 'C' IMMUTABLE STRICT;

--- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_box3d(geometry)
	RETURNS box3d
	AS 'MODULE_PATHNAME','LWGEOM_to_BOX3D'
	LANGUAGE 'C' IMMUTABLE STRICT;

--- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_box(geometry)
	RETURNS box
	AS 'MODULE_PATHNAME','LWGEOM_to_BOX'
	LANGUAGE 'C' IMMUTABLE STRICT;

--- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_box2d(box3d)
	RETURNS box2d
	AS 'MODULE_PATHNAME','BOX3D_to_BOX2DFLOAT4'
	LANGUAGE 'C' IMMUTABLE STRICT;

--- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_box3d(box2d)
	RETURNS box3d
	AS 'MODULE_PATHNAME','BOX2DFLOAT4_to_BOX3D'
	LANGUAGE 'C' IMMUTABLE STRICT;

--- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_box(box3d)
	RETURNS box
	AS 'MODULE_PATHNAME','BOX3D_to_BOX'
	LANGUAGE 'C' IMMUTABLE STRICT;

--- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_text(geometry)
	RETURNS text
	AS 'MODULE_PATHNAME','LWGEOM_to_text'
	LANGUAGE 'C' IMMUTABLE STRICT;

--- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry(box2d)
	RETURNS geometry
	AS 'MODULE_PATHNAME','BOX2DFLOAT4_to_LWGEOM'
	LANGUAGE 'C' IMMUTABLE STRICT;

--- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry(box3d)
	RETURNS geometry
	AS 'MODULE_PATHNAME','BOX3D_to_LWGEOM'
	LANGUAGE 'C' IMMUTABLE STRICT;

--- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry(text)
	RETURNS geometry
	AS 'MODULE_PATHNAME','parse_WKT_lwgeom'
	LANGUAGE 'C' IMMUTABLE STRICT;

--- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry(chip)
	RETURNS geometry
	AS 'MODULE_PATHNAME','CHIP_to_LWGEOM'
	LANGUAGE 'C' IMMUTABLE STRICT;

--- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry(bytea)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_from_bytea'
	LANGUAGE 'C' IMMUTABLE STRICT;

--- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_bytea(geometry)
	RETURNS bytea
	AS 'MODULE_PATHNAME','LWGEOM_to_bytea'
	LANGUAGE 'C' IMMUTABLE STRICT;

--- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_box3d_extent(box3d_extent)
	RETURNS box3d
	AS 'MODULE_PATHNAME', 'BOX3D_extent_to_BOX3D'
	LANGUAGE 'C' IMMUTABLE STRICT;

--- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_box2d(box3d_extent)
	RETURNS box2d
	AS 'MODULE_PATHNAME', 'BOX3D_to_BOX2DFLOAT4'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_box3d_in(cstring)
	RETURNS box3d
	AS 'MODULE_PATHNAME', 'BOX3D_in'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_box3d_out(box3d)
	RETURNS cstring
	AS 'MODULE_PATHNAME', 'BOX3D_out'
	LANGUAGE 'C' IMMUTABLE STRICT;

--- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry(box3d_extent)
	RETURNS geometry
	AS 'MODULE_PATHNAME','BOX3D_to_LWGEOM'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_analyze(internal)
	RETURNS bool
#ifdef GSERIALIZED_ON
	AS 'MODULE_PATHNAME', 'geometry_analyze'
#else
	AS 'MODULE_PATHNAME', 'LWGEOM_analyze'
#endif
	LANGUAGE 'C' VOLATILE STRICT;
	
-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_in(cstring)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_in'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_out(geometry)
	RETURNS cstring
	AS 'MODULE_PATHNAME','LWGEOM_out'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_recv(internal)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_recv'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_send(geometry)
	RETURNS bytea
	AS 'MODULE_PATHNAME','LWGEOM_send'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_spheroid_in(cstring)
	RETURNS spheroid
	AS 'MODULE_PATHNAME','ellipsoid_in'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_spheroid_out(spheroid)
	RETURNS cstring
	AS 'MODULE_PATHNAME','ellipsoid_out'
	LANGUAGE 'C' IMMUTABLE STRICT;
	


--- end functions that in theory should never have been used


-- begin old ogc (and non-ST) names that have been replaced with new SQL-MM and SQL ST_ Like names --
-- AFFINE Functions --
-- Availability: 1.1.2
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Affine(geometry,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_affine'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Availability: 1.1.2
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Affine(geometry,float8,float8,float8,float8,float8,float8)
	RETURNS geometry
	AS 'SELECT affine($1,  $2, $3, 0,  $4, $5, 0,  0, 0, 1,  $6, $7, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.1.2
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION RotateZ(geometry,float8)
	RETURNS geometry
	AS 'SELECT affine($1,  cos($2), -sin($2), 0,  sin($2), cos($2), 0,  0, 0, 1,  0, 0, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.1.2
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Rotate(geometry,float8)
	RETURNS geometry
	AS 'SELECT rotateZ($1, $2)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.1.2
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION RotateX(geometry,float8)
	RETURNS geometry
	AS 'SELECT affine($1, 1, 0, 0, 0, cos($2), -sin($2), 0, sin($2), cos($2), 0, 0, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.1.2
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION RotateY(geometry,float8)
	RETURNS geometry
	AS 'SELECT affine($1,  cos($2), 0, sin($2),  0, 1, 0,  -sin($2), 0, cos($2), 0,  0, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Translate(geometry,float8,float8,float8)
	RETURNS geometry
	AS 'SELECT affine($1, 1, 0, 0, 0, 1, 0, 0, 0, 1, $2, $3, $4)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Translate(geometry,float8,float8)
	RETURNS geometry
	AS 'SELECT translate($1, $2, $3, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.1.0
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Scale(geometry,float8,float8,float8)
	RETURNS geometry
	AS 'SELECT affine($1,  $2, 0, 0,  0, $3, 0,  0, 0, $4,  0, 0, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.1.0
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Scale(geometry,float8,float8)
	RETURNS geometry
	AS 'SELECT scale($1, $2, $3, 1)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.1.0
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION transscale(geometry,float8,float8,float8,float8)
	RETURNS geometry
	AS 'SELECT affine($1,  $4, 0, 0,  0, $5, 0,
		0, 0, 1,  $2 * $4, $3 * $5, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- END Affine functions
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AddPoint(geometry, geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_addpoint'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AddPoint(geometry, geometry, integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_addpoint'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION area(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_area_polygon'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- this is an alias for 'area(geometry)'
-- there is nothing such an 'area3d'...
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION area2d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_area_polygon'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsEWKB(geometry)
	RETURNS BYTEA
	AS 'MODULE_PATHNAME','WKBFromLWGEOM'
	LANGUAGE 'C' IMMUTABLE STRICT;
	

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsEWKB(geometry,text)
	RETURNS bytea
	AS 'MODULE_PATHNAME','WKBFromLWGEOM'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsEWKT(geometry)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asEWKT'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsGML(geometry, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML(2, $1, $2, 0, null)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;
	
-- AsGML(geom) / precision=15 version=2
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsGML(geometry)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML(2, $1, 15, 0, null)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;
	
-- AsKML(geom, precision) / version=2
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsKML(geometry, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsKML(2, transform($1,4326), $2, null)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;
	
-- AsKML(geom) / precision=15 version=2
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsKML(geometry)
	RETURNS TEXT
	AS 'SELECT _ST_AsKML(2, transform($1,4326), 15, null)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;
	
-- AsKML(version, geom, precision)
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsKML(int4, geometry, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsKML($1, transform($2,4326), $3, null)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsHEXEWKB(geometry)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asHEXEWKB'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsHEXEWKB(geometry, text)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asHEXEWKB'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsSVG(geometry,int4,int4)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asSVG'
	LANGUAGE 'C' IMMUTABLE STRICT;	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsSVG(geometry,int4)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asSVG'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsSVG(geometry)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asSVG'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION azimuth(geometry,geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_azimuth'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION BdPolyFromText(text, integer)
RETURNS geometry
AS $$
DECLARE
	geomtext alias for $1;
	srid alias for $2;
	mline geometry;
	geom geometry;
BEGIN
	mline := MultiLineStringFromText(geomtext, srid);

	IF mline IS NULL
	THEN
		RAISE EXCEPTION 'Input is not a MultiLinestring';
	END IF;

	geom := BuildArea(mline);

	IF GeometryType(geom) != 'POLYGON'
	THEN
		RAISE EXCEPTION 'Input returns more then a single polygon, try using BdMPolyFromText instead';
	END IF;

	RETURN geom;
END;
$$
LANGUAGE 'plpgsql' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION BdMPolyFromText(text, integer)
RETURNS geometry
AS $$
DECLARE
	geomtext alias for $1;
	srid alias for $2;
	mline geometry;
	geom geometry;
BEGIN
	mline := MultiLineStringFromText(geomtext, srid);

	IF mline IS NULL
	THEN
		RAISE EXCEPTION 'Input is not a MultiLinestring';
	END IF;

	geom := ST_Multi(BuildArea(mline));

	RETURN geom;
END;
$$
LANGUAGE 'plpgsql' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION buffer(geometry,float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME','buffer'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION buffer(geometry,float8,integer)
	RETURNS geometry
	AS 'SELECT ST_Buffer($1, $2, $3)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION crosses(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION distance(geometry,geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_mindistance2d'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;
		
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION difference(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','difference'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Dimension(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_dimension'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION disjoint(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION distance_sphere(geometry,geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_distance_sphere'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION distance_spheroid(geometry,geometry,spheroid)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_distance_ellipsoid'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION DumpRings(geometry)
	RETURNS SETOF geometry_dump
	AS 'MODULE_PATHNAME', 'LWGEOM_dump_rings'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION envelope(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_envelope'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION expand(box2d,float8)
	RETURNS box2d
	AS 'MODULE_PATHNAME', 'BOX2DFLOAT4_expand'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION expand(box3d,float8)
	RETURNS box3d
	AS 'MODULE_PATHNAME', 'BOX3D_expand'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION expand(geometry,float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_expand'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION ExteriorRing(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_exteriorring_polygon'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION force_2d(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_2d'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- an alias for force_3dz
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION force_3d(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_3dz'
	LANGUAGE 'C' IMMUTABLE STRICT;
	

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION force_3dm(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_3dm'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION force_3dz(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_3dz'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION force_4d(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_4d'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION force_collection(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_collection'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION ForceRHR(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_clockwise_poly'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION GeomCollFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE
	WHEN geometrytype(GeomFromWKB($1, $2)) = ''GEOMETRYCOLLECTION''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

	-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION GeomCollFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE
	WHEN geometrytype(GeomFromWKB($1)) = ''GEOMETRYCOLLECTION''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION GeometryN(geometry,integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_geometryn_collection'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Availability: 1.5.0  -- replaced with postgis_getbbox
CREATE OR REPLACE FUNCTION getbbox(geometry)
	RETURNS box2d
	AS 'MODULE_PATHNAME','LWGEOM_to_BOX2DFLOAT4'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION length3d_spheroid(geometry, spheroid)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_length_ellipsoid_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MPolyFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''MULTIPOLYGON''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;
		
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION multi(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_multi'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MultiPolyFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''MULTIPOLYGON''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MultiPolyFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''MULTIPOLYGON''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION InteriorRingN(geometry,integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_interiorringn_polygon'
	LANGUAGE 'C' IMMUTABLE STRICT;

	-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION IsClosed(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'LWGEOM_isclosed'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION IsEmpty(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'LWGEOM_isempty'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION IsValid(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'isvalid'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;
-- this is a fake (for back-compatibility)
-- uses 3d if 3d is available, 2d otherwise
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION length3d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_length_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION length2d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_length2d_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION length(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_length_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION LineFromMultiPoint(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_line_from_mpoint'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION M(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_m_point'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MakeBox3d(geometry, geometry)
	RETURNS box3d
	AS 'MODULE_PATHNAME', 'BOX3D_construct'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION makeline_garray (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makeline_garray'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE AGGREGATE makeline (
	BASETYPE = geometry,
	SFUNC = pgis_geometry_accum_transfn,
	STYPE = pgis_abs,
	FINALFUNC = pgis_geometry_makeline_finalfn
	);
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MakeLine(geometry, geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makeline'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MakePoint(float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MakePoint(float8, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint'
	LANGUAGE 'C' IMMUTABLE STRICT;

	-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MakePoint(float8, float8, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MakePointM(float8, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint3dm'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MPolyFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1, $2)) = ''MULTIPOLYGON''
	THEN GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MPolyFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1)) = ''MULTIPOLYGON''
	THEN GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MultiPointFromText(text)
	RETURNS geometry
	AS 'SELECT MPointFromText($1)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MultiPointFromText(text)
	RETURNS geometry
	AS 'SELECT MPointFromText($1)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MultiPointFromText(text, int4)
	RETURNS geometry
	AS 'SELECT MPointFromText($1, $2)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MultiPolygonFromText(text, int4)
	RETURNS geometry
	AS 'SELECT MPolyFromText($1, $2)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MultiPolygonFromText(text)
	RETURNS geometry
	AS 'SELECT MPolyFromText($1)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION ndims(geometry)
	RETURNS smallint
	AS 'MODULE_PATHNAME', 'LWGEOM_ndims'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION NumInteriorRing(geometry)
	RETURNS integer
	AS 'MODULE_PATHNAME','LWGEOM_numinteriorrings_polygon'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION NumInteriorRings(geometry)
	RETURNS integer
	AS 'MODULE_PATHNAME','LWGEOM_numinteriorrings_polygon'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION npoints(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_npoints'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION nrings(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_nrings'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION NumGeometries(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_numgeometries_collection'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION NumPoints(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_numpoints_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION overlaps(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- this is a fake (for back-compatibility)
-- uses 3d if 3d is available, 2d otherwise
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION perimeter3d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_perimeter_poly'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION perimeter2d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_perimeter2d_poly'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION point_inside_circle(geometry,float8,float8,float8)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_inside_circle_point'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION PointN(geometry,integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_pointn_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;
	

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION RemovePoint(geometry, integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_removepoint'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION reverse(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_reverse'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Segmentize(geometry, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_segmentize2d'
	LANGUAGE 'C' IMMUTABLE STRICT;	
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION SetPoint(geometry, integer, geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_setpoint_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Availability: 1.1.0
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION shift_longitude(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_longitude_shift'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Simplify(geometry, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_simplify2d'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION symdifference(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','symdifference'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION summary(geometry)
	RETURNS text
	AS 'MODULE_PATHNAME', 'LWGEOM_summary'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION within(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION X(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_x_point'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Y(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_y_point'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Z(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_z_point'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- end old ogc names that have been replaced with new SQL-MM names --

--- Start Aggregates and supporting functions --
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION collect(geometry, geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_collect'
	LANGUAGE 'C' IMMUTABLE;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION combine_bbox(box2d,geometry)
	RETURNS box2d
	AS 'MODULE_PATHNAME', 'BOX2DFLOAT4_combine'
	LANGUAGE 'C' IMMUTABLE;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION combine_bbox(box3d,geometry)
	RETURNS box3d
	AS 'MODULE_PATHNAME', 'BOX3D_combine'
	LANGUAGE 'C' IMMUTABLE;

-- Deprecation in 1.4.0
CREATE OR REPLACE FUNCTION ST_unite_garray (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME','pgis_union_geometry_array'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION unite_garray (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'pgis_union_geometry_array'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE AGGREGATE Extent3d(
	sfunc = combine_bbox,
	basetype = geometry,
	stype = box3d
	);
	
-- Deprecation in 1.2.3
CREATE AGGREGATE memcollect(
	sfunc = ST_collect,
	basetype = geometry,
	stype = geometry
	);

-- Deprecation in 1.2.3
CREATE AGGREGATE MemGeomUnion (
	basetype = geometry,
	sfunc = geomunion,
	stype = geometry
	);

-- End Aggregates and supporting functions --

-------------------------------------------------------------------
--  CHIP TYPE
-------------------------------------------------------------------

CREATE OR REPLACE FUNCTION chip_in(cstring)
	RETURNS chip
	AS 'MODULE_PATHNAME','CHIP_in'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION chip_out(chip)
	RETURNS cstring
	AS 'MODULE_PATHNAME','CHIP_out'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION ST_chip_in(cstring)
	RETURNS chip
	AS 'MODULE_PATHNAME','CHIP_in'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION ST_chip_out(chip)
	RETURNS cstring
	AS 'MODULE_PATHNAME','CHIP_out'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE TYPE chip (
	alignment = double,
	internallength = variable,
	input = chip_in,
	output = chip_out,
	storage = extended
);
--- End CHIP TYPE --
-------------------------------------------
--- Begin CHIP functions
-------------------------------------------

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION srid(chip)
	RETURNS int4
	AS 'MODULE_PATHNAME','CHIP_getSRID'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_srid(chip)
	RETURNS int4
	AS 'MODULE_PATHNAME','CHIP_getSRID'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION height(chip)
	RETURNS int4
	AS 'MODULE_PATHNAME','CHIP_getHeight'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_height(chip)
	RETURNS int4
	AS 'MODULE_PATHNAME','CHIP_getHeight'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION factor(chip)
	RETURNS FLOAT4
	AS 'MODULE_PATHNAME','CHIP_getFactor'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_factor(chip)
	RETURNS FLOAT4
	AS 'MODULE_PATHNAME','CHIP_getFactor'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION width(chip)
	RETURNS int4
	AS 'MODULE_PATHNAME','CHIP_getWidth'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_width(chip)
	RETURNS int4
	AS 'MODULE_PATHNAME','CHIP_getWidth'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION datatype(chip)
	RETURNS int4
	AS 'MODULE_PATHNAME','CHIP_getDatatype'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_datatype(chip)
	RETURNS int4
	AS 'MODULE_PATHNAME','CHIP_getDatatype'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION compression(chip)
	RETURNS int4
	AS 'MODULE_PATHNAME','CHIP_getCompression'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_compression(chip)
	RETURNS int4
	AS 'MODULE_PATHNAME','CHIP_getCompression'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION setSRID(chip,int4)
	RETURNS chip
	AS 'MODULE_PATHNAME','CHIP_setSRID'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION setFactor(chip,float4)
	RETURNS chip
	AS 'MODULE_PATHNAME','CHIP_setFactor'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_setFactor(chip,float4)
	RETURNS chip
	AS 'MODULE_PATHNAME','CHIP_setFactor'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
CREATE OR REPLACE FUNCTION geometry(chip)
	RETURNS geometry
	AS 'MODULE_PATHNAME','CHIP_to_LWGEOM'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
CREATE CAST (chip AS geometry) WITH FUNCTION geometry(chip) AS IMPLICIT;
-- END CHIP --

