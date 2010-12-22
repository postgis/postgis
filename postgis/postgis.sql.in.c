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
--
-- WARNING: Any change in this file must be evaluated for compatibility.
--          Changes cleanly handled by postgis_upgrade.sql are fine,
--	    other changes will require a bump in Major version.
--	    Currently only function replaceble by CREATE OR REPLACE
--	    are cleanly handled.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include "sqldefines.h"

SET client_min_messages TO warning;


-- INSTALL VERSION: POSTGIS_LIB_VERSION

BEGIN;

-------------------------------------------------------------------
--  SPHEROID TYPE
-------------------------------------------------------------------

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

CREATE OR REPLACE FUNCTION spheroid_in(cstring)
	RETURNS spheroid
	AS 'MODULE_PATHNAME','ellipsoid_in'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION spheroid_out(spheroid)
	RETURNS cstring
	AS 'MODULE_PATHNAME','ellipsoid_out'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE TYPE spheroid (
	alignment = double,
	internallength = 65,
	input = spheroid_in,
	output = spheroid_out
);

-------------------------------------------------------------------
--  GEOMETRY TYPE (lwgeom)
-------------------------------------------------------------------

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
CREATE OR REPLACE FUNCTION st_geometry_analyze(internal)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_analyze'
	LANGUAGE 'C' VOLATILE STRICT;

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

CREATE OR REPLACE FUNCTION geometry_in(cstring)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_in'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_out(geometry)
	RETURNS cstring
	AS 'MODULE_PATHNAME','LWGEOM_out'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_analyze(internal)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_analyze'
	LANGUAGE 'C' VOLATILE STRICT;

CREATE OR REPLACE FUNCTION geometry_recv(internal)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_recv'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_send(geometry)
	RETURNS bytea
	AS 'MODULE_PATHNAME','LWGEOM_send'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE TYPE geometry (
	internallength = variable,
	input = geometry_in,
	output = geometry_out,
	send = geometry_send,
	receive = geometry_recv,
	delimiter = ':',
	analyze = geometry_analyze,
	storage = main
);

-------------------------------------------
-- Affine transforms
-------------------------------------------
-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Affine(geometry,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_affine'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Affine(geometry,float8,float8,float8,float8,float8,float8)
	RETURNS geometry
	AS 'SELECT ST_Affine($1,  $2, $3, 0,  $4, $5, 0,  0, 0, 1,  $6, $7, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_RotateZ(geometry,float8)
	RETURNS geometry
	AS 'SELECT ST_Affine($1,  cos($2), -sin($2), 0,  sin($2), cos($2), 0,  0, 0, 1,  0, 0, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Rotate(geometry,float8)
	RETURNS geometry
	AS 'SELECT ST_RotateZ($1, $2)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_RotateX(geometry,float8)
	RETURNS geometry
	AS 'SELECT ST_Affine($1, 1, 0, 0, 0, cos($2), -sin($2), 0, sin($2), cos($2), 0, 0, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_RotateY(geometry,float8)
	RETURNS geometry
	AS 'SELECT ST_Affine($1,  cos($2), 0, sin($2),  0, 1, 0,  -sin($2), 0, cos($2), 0,  0, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Translate(geometry,float8,float8,float8)
	RETURNS geometry
	AS 'SELECT ST_Affine($1, 1, 0, 0, 0, 1, 0, 0, 0, 1, $2, $3, $4)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Translate(geometry,float8,float8)
	RETURNS geometry
	AS 'SELECT ST_Translate($1, $2, $3, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Scale(geometry,float8,float8,float8)
	RETURNS geometry
	AS 'SELECT ST_Affine($1,  $2, 0, 0,  0, $3, 0,  0, 0, $4,  0, 0, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Scale(geometry,float8,float8)
	RETURNS geometry
	AS 'SELECT ST_Scale($1, $2, $3, 1)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Transscale(geometry,float8,float8,float8,float8)
	RETURNS geometry
	AS 'SELECT ST_Affine($1,  $4, 0, 0,  0, $5, 0,
		0, 0, 1,  $2 * $4, $3 * $5, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Shift_Longitude(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_longitude_shift'
	LANGUAGE 'C' IMMUTABLE STRICT;

-------------------------------------------------------------------
--  BOX3D TYPE
-------------------------------------------------------------------

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

CREATE OR REPLACE FUNCTION box3d_in(cstring)
	RETURNS box3d
	AS 'MODULE_PATHNAME', 'BOX3D_in'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION box3d_out(box3d)
	RETURNS cstring
	AS 'MODULE_PATHNAME', 'BOX3D_out'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE TYPE box3d (
	alignment = double,
	internallength = 48,
	input = box3d_in,
	output = box3d_out
);

-- Temporary box3d aggregate type to retain full double precision
-- for ST_Extent(). Should be removed when we change the output
-- type of ST_Extent() to return something other than BOX2DFLOAT4.
CREATE OR REPLACE FUNCTION box3d_extent_in(cstring)
	RETURNS box3d_extent
	AS 'MODULE_PATHNAME', 'BOX3D_in'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION box3d_extent_out(box3d_extent)
	RETURNS cstring
	AS 'MODULE_PATHNAME', 'BOX3D_extent_out'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE TYPE box3d_extent (
	alignment = double,
	internallength = 48,
	input = box3d_extent_in,
	output = box3d_extent_out
);

-- Availability: 1.4.0
CREATE OR REPLACE FUNCTION box3d_extent(box3d_extent)
	RETURNS box3d
	AS 'MODULE_PATHNAME', 'BOX3D_extent_to_BOX3D'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry(box3d_extent)
	RETURNS geometry
	AS 'MODULE_PATHNAME','BOX3D_to_LWGEOM'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- End of temporary hack

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION xmin(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_xmin'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_XMin(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_xmin'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION ymin(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_ymin'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_YMin(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_ymin'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION zmin(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_zmin'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_ZMin(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_zmin'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION xmax(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_xmax'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_XMax(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_xmax'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION ymax(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_ymax'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_YMax(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_ymax'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION zmax(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_zmax'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_ZMax(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_zmax'
	LANGUAGE 'C' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- BOX2D
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION box2d_in(cstring)
	RETURNS box2d
	AS 'MODULE_PATHNAME','BOX2DFLOAT4_in'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION box2d_out(box2d)
	RETURNS cstring
	AS 'MODULE_PATHNAME','BOX2DFLOAT4_out'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE TYPE box2d (
#ifdef GSERIALIZED_ON
	internallength = 65,
#else
	internallength = 16,
#endif
	input = box2d_in,
	output = box2d_out,
	storage = plain
);

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION expand(box2d,float8)
	RETURNS box2d
	AS 'MODULE_PATHNAME', 'BOX2DFLOAT4_expand'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_expand(box2d,float8)
	RETURNS box2d
	AS 'MODULE_PATHNAME', 'BOX2DFLOAT4_expand'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION getbbox(geometry)
	RETURNS box2d
	AS 'MODULE_PATHNAME','LWGEOM_to_BOX2DFLOAT4'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION postgis_getbbox(geometry)
	RETURNS box2d
	AS 'MODULE_PATHNAME','LWGEOM_to_BOX2DFLOAT4'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION box2d(box3d_extent)
	RETURNS box2d
	AS 'MODULE_PATHNAME', 'BOX3D_to_BOX2DFLOAT4'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MakeBox2d(geometry, geometry)
	RETURNS box2d
	AS 'MODULE_PATHNAME', 'BOX2DFLOAT4_construct'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MakeBox2d(geometry, geometry)
	RETURNS box2d
	AS 'MODULE_PATHNAME', 'BOX2DFLOAT4_construct'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION combine_bbox(box2d,geometry)
	RETURNS box2d
	AS 'MODULE_PATHNAME', 'BOX2DFLOAT4_combine'
	LANGUAGE 'C' IMMUTABLE;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Combine_BBox(box2d,geometry)
	RETURNS box2d
	AS 'MODULE_PATHNAME', 'BOX2DFLOAT4_combine'
	LANGUAGE 'C' IMMUTABLE;

-----------------------------------------------------------------------
-- ESTIMATED_EXTENT( <schema name>, <table name>, <column name> )
-----------------------------------------------------------------------
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION estimated_extent(text,text,text) RETURNS box2d AS
	'MODULE_PATHNAME', 'LWGEOM_estimated_extent'
	LANGUAGE 'C' IMMUTABLE STRICT SECURITY DEFINER;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_estimated_extent(text,text,text) RETURNS box2d AS
	'MODULE_PATHNAME', 'LWGEOM_estimated_extent'
	LANGUAGE 'C' IMMUTABLE STRICT SECURITY DEFINER;

-----------------------------------------------------------------------
-- ESTIMATED_EXTENT( <table name>, <column name> )
-----------------------------------------------------------------------
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION estimated_extent(text,text) RETURNS box2d AS
	'MODULE_PATHNAME', 'LWGEOM_estimated_extent'
	LANGUAGE 'C' IMMUTABLE STRICT SECURITY DEFINER;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_estimated_extent(text,text) RETURNS box2d AS
	'MODULE_PATHNAME', 'LWGEOM_estimated_extent'
	LANGUAGE 'C' IMMUTABLE STRICT SECURITY DEFINER;

-----------------------------------------------------------------------
-- FIND_EXTENT( <schema name>, <table name>, <column name> )
-----------------------------------------------------------------------
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION find_extent(text,text,text) RETURNS box2d AS
$$
DECLARE
	schemaname alias for $1;
	tablename alias for $2;
	columnname alias for $3;
	myrec RECORD;

BEGIN
	FOR myrec IN EXECUTE 'SELECT extent("' || columnname || '") FROM "' || schemaname || '"."' || tablename || '"' LOOP
		return myrec.extent;
	END LOOP;
END;
$$
LANGUAGE 'plpgsql' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_find_extent(text,text,text) RETURNS box2d AS
$$
DECLARE
	schemaname alias for $1;
	tablename alias for $2;
	columnname alias for $3;
	myrec RECORD;

BEGIN
	FOR myrec IN EXECUTE 'SELECT extent("' || columnname || '") FROM "' || schemaname || '"."' || tablename || '"' LOOP
		return myrec.extent;
	END LOOP;
END;
$$
LANGUAGE 'plpgsql' IMMUTABLE STRICT;


-----------------------------------------------------------------------
-- FIND_EXTENT( <table name>, <column name> )
-----------------------------------------------------------------------
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION find_extent(text,text) RETURNS box2d AS
$$
DECLARE
	tablename alias for $1;
	columnname alias for $2;
	myrec RECORD;

BEGIN
	FOR myrec IN EXECUTE 'SELECT extent("' || columnname || '") FROM "' || tablename || '"' LOOP
		return myrec.extent;
	END LOOP;
END;
$$
LANGUAGE 'plpgsql' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_find_extent(text,text) RETURNS box2d AS
$$
DECLARE
	tablename alias for $1;
	columnname alias for $2;
	myrec RECORD;

BEGIN
	FOR myrec IN EXECUTE 'SELECT extent("' || columnname || '") FROM "' || tablename || '"' LOOP
		return myrec.extent;
	END LOOP;
END;
$$
LANGUAGE 'plpgsql' IMMUTABLE STRICT;

-------------------------------------------------------------------
-- BTREE indexes
-------------------------------------------------------------------

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_lt(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'lwgeom_lt'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_le(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'lwgeom_le'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_gt(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'lwgeom_gt'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_ge(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'lwgeom_ge'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_eq(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'lwgeom_eq'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_cmp(geometry, geometry)
	RETURNS integer
	AS 'MODULE_PATHNAME', 'lwgeom_cmp'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_lt(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'lwgeom_lt'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_le(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'lwgeom_le'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_gt(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'lwgeom_gt'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_ge(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'lwgeom_ge'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_eq(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'lwgeom_eq'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_cmp(geometry, geometry)
	RETURNS integer
	AS 'MODULE_PATHNAME', 'lwgeom_cmp'
	LANGUAGE 'C' IMMUTABLE STRICT;

--
-- Sorting operators for Btree
--

CREATE OPERATOR < (
	LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = geometry_lt,
	COMMUTATOR = '>', NEGATOR = '>=',
	RESTRICT = contsel, JOIN = contjoinsel
);

CREATE OPERATOR <= (
	LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = geometry_le,
	COMMUTATOR = '>=', NEGATOR = '>',
	RESTRICT = contsel, JOIN = contjoinsel
);

CREATE OPERATOR = (
	LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = geometry_eq,
	COMMUTATOR = '=', -- we might implement a faster negator here
	RESTRICT = contsel, JOIN = contjoinsel
);

CREATE OPERATOR >= (
	LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = geometry_ge,
	COMMUTATOR = '<=', NEGATOR = '<',
	RESTRICT = contsel, JOIN = contjoinsel
);
CREATE OPERATOR > (
	LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = geometry_gt,
	COMMUTATOR = '<', NEGATOR = '<=',
	RESTRICT = contsel, JOIN = contjoinsel
);


CREATE OPERATOR CLASS btree_geometry_ops
	DEFAULT FOR TYPE geometry USING btree AS
	OPERATOR	1	< ,
	OPERATOR	2	<= ,
	OPERATOR	3	= ,
	OPERATOR	4	>= ,
	OPERATOR	5	> ,
	FUNCTION	1	geometry_cmp (geometry, geometry);

#ifndef GSERIALIZED_ON
-------------------------------------------------------------------
-- GiST indexes
-------------------------------------------------------------------

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION postgis_gist_sel (internal, oid, internal, int4)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_gist_sel'
	LANGUAGE 'C';

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION postgis_gist_joinsel(internal, oid, internal, smallint)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_gist_joinsel'
	LANGUAGE 'C';

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_postgis_gist_sel (internal, oid, internal, int4)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_gist_sel'
	LANGUAGE 'C';

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_postgis_gist_joinsel(internal, oid, internal, smallint)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_gist_joinsel'
	LANGUAGE 'C';

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_overleft(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_overleft'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_overright(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_overright'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_overabove(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_overabove'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_overbelow(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_overbelow'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_left(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_left'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_right(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_right'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_above(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_above'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_below(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_below'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_contain(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_contain'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_contained(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_contained'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_overlap(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_overlap'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry_same(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_samebox'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION geometry_same(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_samebox'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_gist_sel (internal, oid, internal, int4)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_gist_sel'
	LANGUAGE 'C';

CREATE OR REPLACE FUNCTION geometry_gist_joinsel(internal, oid, internal, smallint)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_gist_joinsel'
	LANGUAGE 'C';

CREATE OR REPLACE FUNCTION geometry_overleft(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_overleft'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_overright(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_overright'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_overabove(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_overabove'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_overbelow(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_overbelow'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_left(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_left'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_right(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_right'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_above(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_above'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_below(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_below'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_contain(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_contain'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_contained(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_contained'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_overlap(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_overlap'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_samebox(geometry, geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_samebox'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OPERATOR << (
	LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = geometry_left,
	COMMUTATOR = '>>',
	RESTRICT = positionsel, JOIN = positionjoinsel
);

CREATE OPERATOR &< (
	LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = geometry_overleft,
	COMMUTATOR = '&>',
	RESTRICT = positionsel, JOIN = positionjoinsel
);

CREATE OPERATOR <<| (
	LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = geometry_below,
	COMMUTATOR = '|>>',
	RESTRICT = positionsel, JOIN = positionjoinsel
);

CREATE OPERATOR &<| (
	LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = geometry_overbelow,
	COMMUTATOR = '|&>',
	RESTRICT = positionsel, JOIN = positionjoinsel
);

CREATE OPERATOR && (
	LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = geometry_overlap,
	COMMUTATOR = '&&',
	RESTRICT = geometry_gist_sel, JOIN = geometry_gist_joinsel
);

CREATE OPERATOR &> (
	LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = geometry_overright,
	COMMUTATOR = '&<',
	RESTRICT = positionsel, JOIN = positionjoinsel
);

CREATE OPERATOR >> (
	LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = geometry_right,
	COMMUTATOR = '<<',
	RESTRICT = positionsel, JOIN = positionjoinsel
);

CREATE OPERATOR |&> (
	LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = geometry_overabove,
	COMMUTATOR = '&<|',
	RESTRICT = positionsel, JOIN = positionjoinsel
);

CREATE OPERATOR |>> (
	LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = geometry_above,
	COMMUTATOR = '<<|',
	RESTRICT = positionsel, JOIN = positionjoinsel
);

CREATE OPERATOR ~= (
	LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = geometry_samebox,
	COMMUTATOR = '~=',
	RESTRICT = eqsel, JOIN = eqjoinsel
);

CREATE OPERATOR @ (
	LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = geometry_contained,
	COMMUTATOR = '~',
	RESTRICT = contsel, JOIN = contjoinsel
);

CREATE OPERATOR ~ (
	LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = geometry_contain,
	COMMUTATOR = '@',
	RESTRICT = contsel, JOIN = contjoinsel
);

-- gist support functions

CREATE OR REPLACE FUNCTION LWGEOM_gist_consistent(internal,geometry,int4)
	RETURNS bool
	AS 'MODULE_PATHNAME' ,'LWGEOM_gist_consistent'
	LANGUAGE 'C';

CREATE OR REPLACE FUNCTION LWGEOM_gist_compress(internal)
	RETURNS internal
	AS 'MODULE_PATHNAME','LWGEOM_gist_compress'
	LANGUAGE 'C';

CREATE OR REPLACE FUNCTION LWGEOM_gist_penalty(internal,internal,internal)
	RETURNS internal
	AS 'MODULE_PATHNAME' ,'LWGEOM_gist_penalty'
	LANGUAGE 'C';

CREATE OR REPLACE FUNCTION LWGEOM_gist_picksplit(internal, internal)
	RETURNS internal
	AS 'MODULE_PATHNAME' ,'LWGEOM_gist_picksplit'
	LANGUAGE 'C';

CREATE OR REPLACE FUNCTION LWGEOM_gist_union(bytea, internal)
	RETURNS internal
	AS 'MODULE_PATHNAME' ,'LWGEOM_gist_union'
	LANGUAGE 'C';

CREATE OR REPLACE FUNCTION LWGEOM_gist_same(box2d, box2d, internal)
	RETURNS internal
	AS 'MODULE_PATHNAME' ,'LWGEOM_gist_same'
	LANGUAGE 'C';

CREATE OR REPLACE FUNCTION LWGEOM_gist_decompress(internal)
	RETURNS internal
	AS 'MODULE_PATHNAME' ,'LWGEOM_gist_decompress'
	LANGUAGE 'C';

-------------------------------------------
-- GIST opclass index binding entries.
-------------------------------------------
--
-- Create opclass index bindings for PG>=73
--

CREATE OPERATOR CLASS gist_geometry_ops
	DEFAULT FOR TYPE geometry USING gist AS
	STORAGE 	box2d,
	OPERATOR        1        << ,
	OPERATOR        2        &<	,
	OPERATOR        3        &&	,
	OPERATOR        4        &>	,
	OPERATOR        5        >>	,
	OPERATOR        6        ~=	,
	OPERATOR        7        ~	,
	OPERATOR        8        @	,
	OPERATOR	9	 &<|	,
	OPERATOR	10	 <<|	,
	OPERATOR	11	 |>>	,
	OPERATOR	12	 |&>	,
	FUNCTION        1        LWGEOM_gist_consistent (internal, geometry, int4),
	FUNCTION        2        LWGEOM_gist_union (bytea, internal),
	FUNCTION        3        LWGEOM_gist_compress (internal),
	FUNCTION        4        LWGEOM_gist_decompress (internal),
	FUNCTION        5        LWGEOM_gist_penalty (internal, internal, internal),
	FUNCTION        6        LWGEOM_gist_picksplit (internal, internal),
	FUNCTION        7        LWGEOM_gist_same (box2d, box2d, internal);

#endif

-------------------------------------------
-- other lwgeom functions
-------------------------------------------
-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION postgis_addbbox(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_addBBOX'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION postgis_dropbbox(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_dropBBOX'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION getsrid(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME','LWGEOM_get_srid'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION postgis_hasbbox(geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_hasBBOX'
	LANGUAGE 'C' IMMUTABLE STRICT;


------------------------------------------------------------------------
-- DEBUG
------------------------------------------------------------------------

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION mem_size(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_mem_size'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_mem_size(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_mem_size'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_summary(geometry)
	RETURNS text
	AS 'MODULE_PATHNAME', 'LWGEOM_summary'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Npoints(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_npoints'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_nrings(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_nrings'
	LANGUAGE 'C' IMMUTABLE STRICT;

------------------------------------------------------------------------
-- Misures
------------------------------------------------------------------------
-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Length3d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_length_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Length2d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_length2d_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: length2d(geometry)
CREATE OR REPLACE FUNCTION ST_Length(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_length2d_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- this is a fake (for back-compatibility)
-- uses 3d if 3d is available, 2d otherwise
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION length3d_spheroid(geometry, spheroid)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_length_ellipsoid_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_length3d_spheroid(geometry, spheroid)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_length_ellipsoid_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION length_spheroid(geometry, spheroid)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_length_ellipsoid_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_length_spheroid(geometry, spheroid)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_length_ellipsoid_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION length2d_spheroid(geometry, spheroid)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_length2d_ellipsoid'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_length2d_spheroid(geometry, spheroid)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_length2d_ellipsoid'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_perimeter3d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_perimeter_poly'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_perimeter2d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_perimeter2d_poly'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION perimeter(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_perimeter_poly'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: perimeter2d(geometry)
CREATE OR REPLACE FUNCTION ST_Perimeter(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_perimeter2d_poly'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- this is an alias for 'area(geometry)'
-- there is nothing such an 'area3d'...
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION area2d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_area_polygon'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
-- Deprecation in 1.3.4
CREATE OR REPLACE FUNCTION ST_area2d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_area_polygon'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION area(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_area_polygon'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: area(geometry)
CREATE OR REPLACE FUNCTION ST_Area(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_area_polygon'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION distance_spheroid(geometry,geometry,spheroid)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_distance_ellipsoid'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_distance_spheroid(geometry,geometry,spheroid)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_distance_ellipsoid'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION distance_sphere(geometry,geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_distance_sphere'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_distance_sphere(geometry,geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_distance_sphere'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Minimum distance. 2d only.
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION distance(geometry,geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_mindistance2d'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- PostGIS equivalent function: distance(geometry,geometry)
CREATE OR REPLACE FUNCTION ST_Distance(geometry,geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_mindistance2d'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION point_inside_circle(geometry,float8,float8,float8)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_inside_circle_point'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_point_inside_circle(geometry,float8,float8,float8)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_inside_circle_point'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION azimuth(geometry,geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_azimuth'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_azimuth(geometry,geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_azimuth'
	LANGUAGE 'C' IMMUTABLE STRICT;

------------------------------------------------------------------------
-- MISC
------------------------------------------------------------------------

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION force_2d(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_2d'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_force_2d(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_2d'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION force_3dz(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_3dz'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_force_3dz(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_3dz'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- an alias for force_3dz
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION force_3d(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_3dz'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_force_3d(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_3dz'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION force_3dm(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_3dm'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_force_3dm(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_3dm'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION force_4d(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_4d'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_force_4d(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_4d'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION force_collection(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_collection'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_force_collection(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_collection'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION ST_CollectionExtract(geometry, integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'ST_CollectionExtract'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION multi(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_multi'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_multi(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_multi'
	LANGUAGE 'C' IMMUTABLE STRICT;


-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION expand(box3d,float8)
	RETURNS box3d
	AS 'MODULE_PATHNAME', 'BOX3D_expand'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Expand(box3d,float8)
	RETURNS box3d
	AS 'MODULE_PATHNAME', 'BOX3D_expand'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION expand(geometry,float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_expand'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_expand(geometry,float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_expand'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION envelope(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_envelope'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: envelope(geometry)
CREATE OR REPLACE FUNCTION ST_Envelope(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_envelope'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION reverse(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_reverse'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Reverse(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_reverse'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION ForceRHR(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_clockwise_poly'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_ForceRHR(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_clockwise_poly'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION noop(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_noop'
	LANGUAGE 'C' VOLATILE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION postgis_noop(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_noop'
	LANGUAGE 'C' VOLATILE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION zmflag(geometry)
	RETURNS smallint
	AS 'MODULE_PATHNAME', 'LWGEOM_zmflag'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION ST_zmflag(geometry)
	RETURNS smallint
	AS 'MODULE_PATHNAME', 'LWGEOM_zmflag'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION ndims(geometry)
	RETURNS smallint
	AS 'MODULE_PATHNAME', 'LWGEOM_ndims'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_NDims(geometry)
	RETURNS smallint
	AS 'MODULE_PATHNAME', 'LWGEOM_ndims'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsEWKT(geometry)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asEWKT'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_AsEWKT(geometry)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asEWKT'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsEWKB(geometry)
	RETURNS BYTEA
	AS 'MODULE_PATHNAME','WKBFromLWGEOM'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_AsEWKB(geometry)
	RETURNS BYTEA
	AS 'MODULE_PATHNAME','WKBFromLWGEOM'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsHEXEWKB(geometry)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asHEXEWKB'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_AsHEXEWKB(geometry)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asHEXEWKB'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsHEXEWKB(geometry, text)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asHEXEWKB'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_AsHEXEWKB(geometry, text)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asHEXEWKB'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsEWKB(geometry,text)
	RETURNS bytea
	AS 'MODULE_PATHNAME','WKBFromLWGEOM'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_AsEWKB(geometry,text)
	RETURNS bytea
	AS 'MODULE_PATHNAME','WKBFromLWGEOM'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 2.0.0
CREATE OR REPLACE FUNCTION ST_AsLatLonText(geometry, text)
	RETURNS text
	AS 'MODULE_PATHNAME','LWGEOM_to_latlon'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 2.0.0
CREATE OR REPLACE FUNCTION ST_AsLatLonText(geometry)
	RETURNS text
	AS $$ SELECT ST_AsLatLonText($1, '') $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION GeomFromEWKB(bytea)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOMFromWKB'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_GeomFromEWKB(bytea)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOMFromWKB'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION GeomFromEWKT(text)
	RETURNS geometry
	AS 'MODULE_PATHNAME','parse_WKT_lwgeom'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_GeomFromEWKT(text)
	RETURNS geometry
	AS 'MODULE_PATHNAME','parse_WKT_lwgeom'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION postgis_cache_bbox()
	RETURNS trigger
	AS 'MODULE_PATHNAME', 'cache_bbox'
	LANGUAGE 'C';

------------------------------------------------------------------------
-- CONSTRUCTORS
------------------------------------------------------------------------

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MakePoint(float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MakePoint(float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MakePoint(float8, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MakePoint(float8, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MakePoint(float8, float8, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MakePoint(float8, float8, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MakePointM(float8, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint3dm'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.3.4
CREATE OR REPLACE FUNCTION ST_MakePointM(float8, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint3dm'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MakeBox3d(geometry, geometry)
	RETURNS box3d
	AS 'MODULE_PATHNAME', 'BOX3D_construct'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MakeBox3d(geometry, geometry)
	RETURNS box3d
	AS 'MODULE_PATHNAME', 'BOX3D_construct'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION makeline_garray (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makeline_garray'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MakeLine_GArray (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makeline_garray'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.4.0
CREATE OR REPLACE FUNCTION ST_MakeLine (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makeline_garray'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION LineFromMultiPoint(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_line_from_mpoint'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_LineFromMultiPoint(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_line_from_mpoint'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MakeLine(geometry, geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makeline'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MakeLine(geometry, geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makeline'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AddPoint(geometry, geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_addpoint'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_AddPoint(geometry, geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_addpoint'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AddPoint(geometry, geometry, integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_addpoint'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_AddPoint(geometry, geometry, integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_addpoint'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION RemovePoint(geometry, integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_removepoint'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_RemovePoint(geometry, integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_removepoint'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION SetPoint(geometry, integer, geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_setpoint_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_SetPoint(geometry, integer, geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_setpoint_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION ST_MakeEnvelope(float8, float8, float8, float8, integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'ST_MakeEnvelope'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MakePolygon(geometry, geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoly'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MakePolygon(geometry, geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoly'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MakePolygon(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoly'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MakePolygon(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoly'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION BuildArea(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_buildarea'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_BuildArea(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_buildarea'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Polygonize_GArray (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'polygonize_garray'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION ST_Polygonize_GArray (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'polygonize_garray'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.4.0
CREATE OR REPLACE FUNCTION ST_Polygonize (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'polygonize_garray'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION LineMerge(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'linemerge'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_LineMerge(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'linemerge'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;


CREATE TYPE geometry_dump AS (path integer[], geom geometry);

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Dump(geometry)
	RETURNS SETOF geometry_dump
	AS 'MODULE_PATHNAME', 'LWGEOM_dump'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Dump(geometry)
	RETURNS SETOF geometry_dump
	AS 'MODULE_PATHNAME', 'LWGEOM_dump'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION DumpRings(geometry)
	RETURNS SETOF geometry_dump
	AS 'MODULE_PATHNAME', 'LWGEOM_dump_rings'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_DumpRings(geometry)
	RETURNS SETOF geometry_dump
	AS 'MODULE_PATHNAME', 'LWGEOM_dump_rings'
	LANGUAGE 'C' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- _ST_DumpPoints()
-----------------------------------------------------------------------
-- A helper function for ST_DumpPoints(geom)
-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION _ST_DumpPoints(the_geom geometry, cur_path integer[]) RETURNS SETOF geometry_dump AS $$
DECLARE
  tmp geometry_dump;
  tmp2 geometry_dump;
  nb_points integer;
  nb_geom integer;
  i integer;
  j integer;
  g geometry;
  
BEGIN
  
  RAISE DEBUG '%,%', cur_path, ST_GeometryType(the_geom);

  -- Special case collections : iterate and return the DumpPoints of the geometries

  IF (ST_IsCollection(the_geom)) THEN
 
    i = 1;
    FOR tmp2 IN SELECT (ST_Dump(the_geom)).* LOOP

      FOR tmp IN SELECT * FROM _ST_DumpPoints(tmp2.geom, cur_path || tmp2.path) LOOP
	    RETURN NEXT tmp;
      END LOOP;
      i = i + 1;
      
    END LOOP;

    RETURN;
  END IF;
  

  -- Special case (POLYGON) : return the points of the rings of a polygon
  IF (ST_GeometryType(the_geom) = 'ST_Polygon') THEN

    FOR tmp IN SELECT * FROM _ST_DumpPoints(ST_ExteriorRing(the_geom), cur_path || ARRAY[1]) LOOP
      RETURN NEXT tmp;
    END LOOP;
    
    j := ST_NumInteriorRings(the_geom);
    FOR i IN 1..j LOOP
        FOR tmp IN SELECT * FROM _ST_DumpPoints(ST_InteriorRingN(the_geom, i), cur_path || ARRAY[i+1]) LOOP
          RETURN NEXT tmp;
        END LOOP;
    END LOOP;
    
    RETURN;
  END IF;

  -- Special case (TRIANGLE) : return the points of the external rings of a TRIANGLE
  IF (ST_GeometryType(the_geom) = 'ST_Triangle') THEN

    FOR tmp IN SELECT * FROM _ST_DumpPoints(ST_ExteriorRing(the_geom), cur_path || ARRAY[1]) LOOP
      RETURN NEXT tmp;
    END LOOP;
    
    RETURN;
  END IF;

    
  -- Special case (POINT) : return the point
  IF (ST_GeometryType(the_geom) = 'ST_Point') THEN

    tmp.path = cur_path || ARRAY[1];
    tmp.geom = the_geom;

    RETURN NEXT tmp;
    RETURN;

  END IF;


  -- Use ST_NumPoints rather than ST_NPoints to have a NULL value if the_geom isn't
  -- a LINESTRING, CIRCULARSTRING.
  SELECT ST_NumPoints(the_geom) INTO nb_points;

  -- This should never happen
  IF (nb_points IS NULL) THEN
    RAISE EXCEPTION 'Unexpected error while dumping geometry %', ST_AsText(the_geom);
  END IF;

  FOR i IN 1..nb_points LOOP
    tmp.path = cur_path || ARRAY[i];
    tmp.geom := ST_PointN(the_geom, i);
    RETURN NEXT tmp;
  END LOOP;
   
END
$$ LANGUAGE plpgsql;

-----------------------------------------------------------------------
-- ST_DumpPoints()
-----------------------------------------------------------------------
-- This function mimicks that of ST_Dump for collections, but this function 
-- that returns a path and all the points that make up a particular geometry.
-- This current implementation in plpgsql does not scale very well at all.
-- and should be ported to C at some point.
-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION ST_DumpPoints(geometry) RETURNS SETOF geometry_dump AS $$
  SELECT * FROM _ST_DumpPoints($1, NULL);
$$ LANGUAGE SQL;


------------------------------------------------------------------------

--
-- Aggregate functions
--


-- Temporary hack function
CREATE OR REPLACE FUNCTION combine_bbox(box3d_extent,geometry)
	RETURNS box3d_extent
	AS 'MODULE_PATHNAME', 'BOX3D_combine'
	LANGUAGE 'C' IMMUTABLE;

-- Temporary hack function
CREATE OR REPLACE FUNCTION ST_Combine_BBox(box3d_extent,geometry)
	RETURNS box3d_extent
	AS 'MODULE_PATHNAME', 'BOX3D_combine'
	LANGUAGE 'C' IMMUTABLE;

-- Deprecation in 1.2.3
CREATE AGGREGATE Extent(
	sfunc = ST_combine_bbox,
	basetype = geometry,
	stype = box3d_extent
	);

-- Availability: 1.2.2
CREATE AGGREGATE ST_Extent(
	sfunc = ST_combine_bbox,
	basetype = geometry,
	stype = box3d_extent
	);

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION combine_bbox(box3d,geometry)
	RETURNS box3d
	AS 'MODULE_PATHNAME', 'BOX3D_combine'
	LANGUAGE 'C' IMMUTABLE;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Combine_BBox(box3d,geometry)
	RETURNS box3d
	AS 'MODULE_PATHNAME', 'BOX3D_combine'
	LANGUAGE 'C' IMMUTABLE;

-- Deprecation in 1.2.3
CREATE AGGREGATE Extent3d(
	sfunc = combine_bbox,
	basetype = geometry,
	stype = box3d
	);

-- Availability: 1.2.2
CREATE AGGREGATE ST_Extent3d(
	sfunc = ST_combine_bbox,
	basetype = geometry,
	stype = box3d
	);



-------------------------------------------------------------------
-- SPATIAL_REF_SYS
-------------------------------------------------------------------
CREATE TABLE spatial_ref_sys (
	 srid integer not null primary key,
	 auth_name varchar(256),
	 auth_srid integer,
	 srtext varchar(2048),
	 proj4text varchar(2048)
);

-------------------------------------------------------------------
-- GEOMETRY_COLUMNS
-------------------------------------------------------------------
CREATE TABLE geometry_columns (
	f_table_catalog varchar(256) not null,
	f_table_schema varchar(256) not null,
	f_table_name varchar(256) not null,
	f_geometry_column varchar(256) not null,
	coord_dimension integer not null,
	srid integer not null,
	type varchar(30) not null,
	CONSTRAINT geometry_columns_pk primary key (
		f_table_catalog,
		f_table_schema,
		f_table_name,
		f_geometry_column )
) WITH OIDS;

-----------------------------------------------------------------------
-- RENAME_GEOMETRY_TABLE_CONSTRAINTS()
-----------------------------------------------------------------------
-- This function has been obsoleted for the difficulty in
-- finding attribute on which the constraint is applied.
-- AddGeometryColumn will name the constraints in a meaningful
-- way, but nobody can rely on it since old postgis versions did
-- not do that.
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION rename_geometry_table_constraints() RETURNS text
AS
$$
SELECT 'rename_geometry_table_constraint() is obsoleted'::text
$$
LANGUAGE 'SQL' IMMUTABLE;

-----------------------------------------------------------------------
-- FIX_GEOMETRY_COLUMNS()
-----------------------------------------------------------------------
-- This function will:
--
--	o try to fix the schema of records with an integer one
--		(for PG>=73)
--
--	o link records to system tables through attrelid and varattnum
--		(for PG<75)
--
--	o delete all records for which no linking was possible
--		(for PG<75)
--
--
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION fix_geometry_columns() RETURNS text
AS
$$
DECLARE
	mislinked record;
	result text;
	linked integer;
	deleted integer;
	foundschema integer;
BEGIN

	-- Since 7.3 schema support has been added.
	-- Previous postgis versions used to put the database name in
	-- the schema column. This needs to be fixed, so we try to
	-- set the correct schema for each geometry_colums record
	-- looking at table, column, type and srid.
	UPDATE geometry_columns SET f_table_schema = n.nspname
		FROM pg_namespace n, pg_class c, pg_attribute a,
			pg_constraint sridcheck, pg_constraint typecheck
			WHERE ( f_table_schema is NULL
		OR f_table_schema = ''
			OR f_table_schema NOT IN (
					SELECT nspname::varchar
					FROM pg_namespace nn, pg_class cc, pg_attribute aa
					WHERE cc.relnamespace = nn.oid
					AND cc.relname = f_table_name::name
					AND aa.attrelid = cc.oid
					AND aa.attname = f_geometry_column::name))
			AND f_table_name::name = c.relname
			AND c.oid = a.attrelid
			AND c.relnamespace = n.oid
			AND f_geometry_column::name = a.attname

			AND sridcheck.conrelid = c.oid
		AND sridcheck.consrc LIKE '(srid(% = %)'
			AND sridcheck.consrc ~ textcat(' = ', srid::text)

			AND typecheck.conrelid = c.oid
		AND typecheck.consrc LIKE
		'((geometrytype(%) = ''%''::text) OR (% IS NULL))'
			AND typecheck.consrc ~ textcat(' = ''', type::text)

			AND NOT EXISTS (
					SELECT oid FROM geometry_columns gc
					WHERE c.relname::varchar = gc.f_table_name
					AND n.nspname::varchar = gc.f_table_schema
					AND a.attname::varchar = gc.f_geometry_column
			);

	GET DIAGNOSTICS foundschema = ROW_COUNT;

	-- no linkage to system table needed
	return 'fixed:'||foundschema::text;

END;
$$
LANGUAGE 'plpgsql' VOLATILE;

-----------------------------------------------------------------------
-- POPULATE_GEOMETRY_COLUMNS()
-----------------------------------------------------------------------
-- Truncates and refills the geometry_columns table from all tables and
-- views in the database that contain geometry columns. This function
-- is a simple wrapper for populate_geometry_columns(oid).  In essence,
-- this function ensures every geometry column in the database has the
-- appropriate spatial contraints (for tables) and exists in the
-- geometry_columns table.
-- Availability: 1.4.0
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION populate_geometry_columns()
	RETURNS text AS
$$
DECLARE
	inserted    integer;
	oldcount    integer;
	probed      integer;
	stale       integer;
	gcs         RECORD;
	gc          RECORD;
	gsrid       integer;
	gndims      integer;
	gtype       text;
	query       text;
	gc_is_valid boolean;

BEGIN
	SELECT count(*) INTO oldcount FROM geometry_columns;
	inserted := 0;

	EXECUTE 'TRUNCATE geometry_columns';

	-- Count the number of geometry columns in all tables and views
	SELECT count(DISTINCT c.oid) INTO probed
	FROM pg_class c,
		 pg_attribute a,
		 pg_type t,
		 pg_namespace n
	WHERE (c.relkind = 'r' OR c.relkind = 'v')
	AND t.typname = 'geometry'
	AND a.attisdropped = false
	AND a.atttypid = t.oid
	AND a.attrelid = c.oid
	AND c.relnamespace = n.oid
	AND n.nspname NOT ILIKE 'pg_temp%';

	-- Iterate through all non-dropped geometry columns
	RAISE DEBUG 'Processing Tables.....';

	FOR gcs IN
	SELECT DISTINCT ON (c.oid) c.oid, n.nspname, c.relname
		FROM pg_class c,
			 pg_attribute a,
			 pg_type t,
			 pg_namespace n
		WHERE c.relkind = 'r'
		AND t.typname = 'geometry'
		AND a.attisdropped = false
		AND a.atttypid = t.oid
		AND a.attrelid = c.oid
		AND c.relnamespace = n.oid
		AND n.nspname NOT ILIKE 'pg_temp%'
	LOOP

	inserted := inserted + populate_geometry_columns(gcs.oid);
	END LOOP;

	-- Add views to geometry columns table
	RAISE DEBUG 'Processing Views.....';
	FOR gcs IN
	SELECT DISTINCT ON (c.oid) c.oid, n.nspname, c.relname
		FROM pg_class c,
			 pg_attribute a,
			 pg_type t,
			 pg_namespace n
		WHERE c.relkind = 'v'
		AND t.typname = 'geometry'
		AND a.attisdropped = false
		AND a.atttypid = t.oid
		AND a.attrelid = c.oid
		AND c.relnamespace = n.oid
	LOOP

	inserted := inserted + populate_geometry_columns(gcs.oid);
	END LOOP;

	IF oldcount > inserted THEN
	stale = oldcount-inserted;
	ELSE
	stale = 0;
	END IF;

	RETURN 'probed:' ||probed|| ' inserted:'||inserted|| ' conflicts:'||probed-inserted|| ' deleted:'||stale;
END

$$
LANGUAGE 'plpgsql' VOLATILE;

-----------------------------------------------------------------------
-- POPULATE_GEOMETRY_COLUMNS(tbl_oid oid)
-----------------------------------------------------------------------
-- DELETEs from and reINSERTs into the geometry_columns table all entries
-- associated with the oid of a particular table or view.
--
-- If the provided oid is for a table, this function tries to determine
-- the srid, dimension, and geometry type of the all geometries
-- in the table, adding contraints as necessary to the table.  If
-- successful, an appropriate row is inserted into the geometry_columns
-- table, otherwise, the exception is caught and an error notice is
-- raised describing the problem. (This is so the wrapper function
-- populate_geometry_columns() can apply spatial constraints to all
-- geometry columns across an entire database at once without erroring
-- out)
--
-- If the provided oid is for a view, as with a table oid, this function
-- tries to determine the srid, dimension, and type of all the geometries
-- in the view, inserting appropriate entries into the geometry_columns
-- table.
-- Availability: 1.4.0
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION populate_geometry_columns(tbl_oid oid)
	RETURNS integer AS
$$
DECLARE
	gcs         RECORD;
	gc          RECORD;
	gsrid       integer;
	gndims      integer;
	gtype       text;
	query       text;
	gc_is_valid boolean;
	inserted    integer;

BEGIN
	inserted := 0;

	-- Iterate through all geometry columns in this table
	FOR gcs IN
	SELECT n.nspname, c.relname, a.attname
		FROM pg_class c,
			 pg_attribute a,
			 pg_type t,
			 pg_namespace n
		WHERE c.relkind = 'r'
		AND t.typname = 'geometry'
		AND a.attisdropped = false
		AND a.atttypid = t.oid
		AND a.attrelid = c.oid
		AND c.relnamespace = n.oid
		AND n.nspname NOT ILIKE 'pg_temp%'
		AND c.oid = tbl_oid
	LOOP

	RAISE DEBUG 'Processing table %.%.%', gcs.nspname, gcs.relname, gcs.attname;

	DELETE FROM geometry_columns
	  WHERE f_table_schema = quote_ident(gcs.nspname)
	  AND f_table_name = quote_ident(gcs.relname)
	  AND f_geometry_column = quote_ident(gcs.attname);

	gc_is_valid := true;

	-- Try to find srid check from system tables (pg_constraint)
	gsrid :=
		(SELECT replace(replace(split_part(s.consrc, ' = ', 2), ')', ''), '(', '')
		 FROM pg_class c, pg_namespace n, pg_attribute a, pg_constraint s
		 WHERE n.nspname = gcs.nspname
		 AND c.relname = gcs.relname
		 AND a.attname = gcs.attname
		 AND a.attrelid = c.oid
		 AND s.connamespace = n.oid
		 AND s.conrelid = c.oid
		 AND a.attnum = ANY (s.conkey)
		 AND s.consrc LIKE '%srid(% = %');
	IF (gsrid IS NULL) THEN
		-- Try to find srid from the geometry itself
		EXECUTE 'SELECT srid(' || quote_ident(gcs.attname) || ')
				 FROM ONLY ' || quote_ident(gcs.nspname) || '.' || quote_ident(gcs.relname) || '
				 WHERE ' || quote_ident(gcs.attname) || ' IS NOT NULL LIMIT 1'
			INTO gc;
		gsrid := gc.srid;

		-- Try to apply srid check to column
		IF (gsrid IS NOT NULL) THEN
			BEGIN
				EXECUTE 'ALTER TABLE ONLY ' || quote_ident(gcs.nspname) || '.' || quote_ident(gcs.relname) || '
						 ADD CONSTRAINT ' || quote_ident('enforce_srid_' || gcs.attname) || '
						 CHECK (srid(' || quote_ident(gcs.attname) || ') = ' || gsrid || ')';
			EXCEPTION
				WHEN check_violation THEN
					RAISE WARNING 'Not inserting ''%'' in ''%.%'' into geometry_columns: could not apply constraint CHECK (srid(%) = %)', quote_ident(gcs.attname), quote_ident(gcs.nspname), quote_ident(gcs.relname), quote_ident(gcs.attname), gsrid;
					gc_is_valid := false;
			END;
		END IF;
	END IF;

	-- Try to find ndims check from system tables (pg_constraint)
	gndims :=
		(SELECT replace(split_part(s.consrc, ' = ', 2), ')', '')
		 FROM pg_class c, pg_namespace n, pg_attribute a, pg_constraint s
		 WHERE n.nspname = gcs.nspname
		 AND c.relname = gcs.relname
		 AND a.attname = gcs.attname
		 AND a.attrelid = c.oid
		 AND s.connamespace = n.oid
		 AND s.conrelid = c.oid
		 AND a.attnum = ANY (s.conkey)
		 AND s.consrc LIKE '%ndims(% = %');
	IF (gndims IS NULL) THEN
		-- Try to find ndims from the geometry itself
		EXECUTE 'SELECT ndims(' || quote_ident(gcs.attname) || ')
				 FROM ONLY ' || quote_ident(gcs.nspname) || '.' || quote_ident(gcs.relname) || '
				 WHERE ' || quote_ident(gcs.attname) || ' IS NOT NULL LIMIT 1'
			INTO gc;
		gndims := gc.ndims;

		-- Try to apply ndims check to column
		IF (gndims IS NOT NULL) THEN
			BEGIN
				EXECUTE 'ALTER TABLE ONLY ' || quote_ident(gcs.nspname) || '.' || quote_ident(gcs.relname) || '
						 ADD CONSTRAINT ' || quote_ident('enforce_dims_' || gcs.attname) || '
						 CHECK (ndims(' || quote_ident(gcs.attname) || ') = '||gndims||')';
			EXCEPTION
				WHEN check_violation THEN
					RAISE WARNING 'Not inserting ''%'' in ''%.%'' into geometry_columns: could not apply constraint CHECK (ndims(%) = %)', quote_ident(gcs.attname), quote_ident(gcs.nspname), quote_ident(gcs.relname), quote_ident(gcs.attname), gndims;
					gc_is_valid := false;
			END;
		END IF;
	END IF;

	-- Try to find geotype check from system tables (pg_constraint)
	gtype :=
		(SELECT replace(split_part(s.consrc, '''', 2), ')', '')
		 FROM pg_class c, pg_namespace n, pg_attribute a, pg_constraint s
		 WHERE n.nspname = gcs.nspname
		 AND c.relname = gcs.relname
		 AND a.attname = gcs.attname
		 AND a.attrelid = c.oid
		 AND s.connamespace = n.oid
		 AND s.conrelid = c.oid
		 AND a.attnum = ANY (s.conkey)
		 AND s.consrc LIKE '%geometrytype(% = %');
	IF (gtype IS NULL) THEN
		-- Try to find geotype from the geometry itself
		EXECUTE 'SELECT geometrytype(' || quote_ident(gcs.attname) || ')
				 FROM ONLY ' || quote_ident(gcs.nspname) || '.' || quote_ident(gcs.relname) || '
				 WHERE ' || quote_ident(gcs.attname) || ' IS NOT NULL LIMIT 1'
			INTO gc;
		gtype := gc.geometrytype;
		--IF (gtype IS NULL) THEN
		--    gtype := 'GEOMETRY';
		--END IF;

		-- Try to apply geometrytype check to column
		IF (gtype IS NOT NULL) THEN
			BEGIN
				EXECUTE 'ALTER TABLE ONLY ' || quote_ident(gcs.nspname) || '.' || quote_ident(gcs.relname) || '
				ADD CONSTRAINT ' || quote_ident('enforce_geotype_' || gcs.attname) || '
				CHECK ((geometrytype(' || quote_ident(gcs.attname) || ') = ' || quote_literal(gtype) || ') OR (' || quote_ident(gcs.attname) || ' IS NULL))';
			EXCEPTION
				WHEN check_violation THEN
					-- No geometry check can be applied. This column contains a number of geometry types.
					RAISE WARNING 'Could not add geometry type check (%) to table column: %.%.%', gtype, quote_ident(gcs.nspname),quote_ident(gcs.relname),quote_ident(gcs.attname);
			END;
		END IF;
	END IF;

	IF (gsrid IS NULL) THEN
		RAISE WARNING 'Not inserting ''%'' in ''%.%'' into geometry_columns: could not determine the srid', quote_ident(gcs.attname), quote_ident(gcs.nspname), quote_ident(gcs.relname);
	ELSIF (gndims IS NULL) THEN
		RAISE WARNING 'Not inserting ''%'' in ''%.%'' into geometry_columns: could not determine the number of dimensions', quote_ident(gcs.attname), quote_ident(gcs.nspname), quote_ident(gcs.relname);
	ELSIF (gtype IS NULL) THEN
		RAISE WARNING 'Not inserting ''%'' in ''%.%'' into geometry_columns: could not determine the geometry type', quote_ident(gcs.attname), quote_ident(gcs.nspname), quote_ident(gcs.relname);
	ELSE
		-- Only insert into geometry_columns if table constraints could be applied.
		IF (gc_is_valid) THEN
			INSERT INTO geometry_columns (f_table_catalog,f_table_schema, f_table_name, f_geometry_column, coord_dimension, srid, type)
			VALUES ('', gcs.nspname, gcs.relname, gcs.attname, gndims, gsrid, gtype);
			inserted := inserted + 1;
		END IF;
	END IF;
	END LOOP;

	-- Add views to geometry columns table
	FOR gcs IN
	SELECT n.nspname, c.relname, a.attname
		FROM pg_class c,
			 pg_attribute a,
			 pg_type t,
			 pg_namespace n
		WHERE c.relkind = 'v'
		AND t.typname = 'geometry'
		AND a.attisdropped = false
		AND a.atttypid = t.oid
		AND a.attrelid = c.oid
		AND c.relnamespace = n.oid
		AND n.nspname NOT ILIKE 'pg_temp%'
		AND c.oid = tbl_oid
	LOOP
		RAISE DEBUG 'Processing view %.%.%', gcs.nspname, gcs.relname, gcs.attname;

		EXECUTE 'SELECT ndims(' || quote_ident(gcs.attname) || ')
				 FROM ' || quote_ident(gcs.nspname) || '.' || quote_ident(gcs.relname) || '
				 WHERE ' || quote_ident(gcs.attname) || ' IS NOT NULL LIMIT 1'
			INTO gc;
		gndims := gc.ndims;

		EXECUTE 'SELECT srid(' || quote_ident(gcs.attname) || ')
				 FROM ' || quote_ident(gcs.nspname) || '.' || quote_ident(gcs.relname) || '
				 WHERE ' || quote_ident(gcs.attname) || ' IS NOT NULL LIMIT 1'
			INTO gc;
		gsrid := gc.srid;

		EXECUTE 'SELECT geometrytype(' || quote_ident(gcs.attname) || ')
				 FROM ' || quote_ident(gcs.nspname) || '.' || quote_ident(gcs.relname) || '
				 WHERE ' || quote_ident(gcs.attname) || ' IS NOT NULL LIMIT 1'
			INTO gc;
		gtype := gc.geometrytype;

		IF (gndims IS NULL) THEN
			RAISE WARNING 'Not inserting ''%'' in ''%.%'' into geometry_columns: could not determine ndims', quote_ident(gcs.attname), quote_ident(gcs.nspname), quote_ident(gcs.relname);
		ELSIF (gsrid IS NULL) THEN
			RAISE WARNING 'Not inserting ''%'' in ''%.%'' into geometry_columns: could not determine srid', quote_ident(gcs.attname), quote_ident(gcs.nspname), quote_ident(gcs.relname);
		ELSIF (gtype IS NULL) THEN
			RAISE WARNING 'Not inserting ''%'' in ''%.%'' into geometry_columns: could not determine gtype', quote_ident(gcs.attname), quote_ident(gcs.nspname), quote_ident(gcs.relname);
		ELSE
			query := 'INSERT INTO geometry_columns (f_table_catalog,f_table_schema, f_table_name, f_geometry_column, coord_dimension, srid, type) ' ||
					 'VALUES ('''', ' || quote_literal(gcs.nspname) || ',' || quote_literal(gcs.relname) || ',' || quote_literal(gcs.attname) || ',' || gndims || ',' || gsrid || ',' || quote_literal(gtype) || ')';
			EXECUTE query;
			inserted := inserted + 1;
		END IF;
	END LOOP;

	RETURN inserted;
END

$$
LANGUAGE 'plpgsql' VOLATILE;


-----------------------------------------------------------------------
-- PROBE_GEOMETRY_COLUMNS()
-----------------------------------------------------------------------
-- Fill the geometry_columns table with values probed from the system
-- catalogues. This is done by simply looking up constraints previously
-- added to a geometry column. If geometry constraints are missing, no
-- attempt is made to add the necessary constraints to the geometry
-- column, nor is it recorded in the geometry_columns table.
-- 3d flag cannot be probed, it defaults to 2
--
-- Note that bogus records already in geometry_columns are not
-- overridden (a check for schema.table.column is performed), so
-- to have a fresh probe backup your geometry_columns, delete from
-- it and probe.
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION probe_geometry_columns() RETURNS text AS
$$
DECLARE
	inserted integer;
	oldcount integer;
	probed integer;
	stale integer;
BEGIN

	SELECT count(*) INTO oldcount FROM geometry_columns;

	SELECT count(*) INTO probed
		FROM pg_class c, pg_attribute a, pg_type t,
			pg_namespace n,
			pg_constraint sridcheck, pg_constraint typecheck

		WHERE t.typname = 'geometry'
		AND a.atttypid = t.oid
		AND a.attrelid = c.oid
		AND c.relnamespace = n.oid
		AND sridcheck.connamespace = n.oid
		AND typecheck.connamespace = n.oid
		AND sridcheck.conrelid = c.oid
		AND sridcheck.consrc LIKE '(srid('||a.attname||') = %)'
		AND typecheck.conrelid = c.oid
		AND typecheck.consrc LIKE
		'((geometrytype('||a.attname||') = ''%''::text) OR (% IS NULL))'
		;

	INSERT INTO geometry_columns SELECT
		''::varchar as f_table_catalogue,
		n.nspname::varchar as f_table_schema,
		c.relname::varchar as f_table_name,
		a.attname::varchar as f_geometry_column,
		2 as coord_dimension,
		trim(both  ' =)' from
			replace(replace(split_part(
				sridcheck.consrc, ' = ', 2), ')', ''), '(', ''))::integer AS srid,
		trim(both ' =)''' from substr(typecheck.consrc,
			strpos(typecheck.consrc, '='),
			strpos(typecheck.consrc, '::')-
			strpos(typecheck.consrc, '=')
			))::varchar as type
		FROM pg_class c, pg_attribute a, pg_type t,
			pg_namespace n,
			pg_constraint sridcheck, pg_constraint typecheck
		WHERE t.typname = 'geometry'
		AND a.atttypid = t.oid
		AND a.attrelid = c.oid
		AND c.relnamespace = n.oid
		AND sridcheck.connamespace = n.oid
		AND typecheck.connamespace = n.oid
		AND sridcheck.conrelid = c.oid
		AND sridcheck.consrc LIKE '(st_srid('||a.attname||') = %)'
		AND typecheck.conrelid = c.oid
		AND typecheck.consrc LIKE
		'((geometrytype('||a.attname||') = ''%''::text) OR (% IS NULL))'

			AND NOT EXISTS (
					SELECT oid FROM geometry_columns gc
					WHERE c.relname::varchar = gc.f_table_name
					AND n.nspname::varchar = gc.f_table_schema
					AND a.attname::varchar = gc.f_geometry_column
			);

	GET DIAGNOSTICS inserted = ROW_COUNT;

	IF oldcount > probed THEN
		stale = oldcount-probed;
	ELSE
		stale = 0;
	END IF;

	RETURN 'probed:'||probed::text||
		' inserted:'||inserted::text||
		' conflicts:'||(probed-inserted)::text||
		' stale:'||stale::text;
END

$$
LANGUAGE 'plpgsql' VOLATILE;

-----------------------------------------------------------------------
-- ADDGEOMETRYCOLUMN
--   <catalogue>, <schema>, <table>, <column>, <srid>, <type>, <dim>
-----------------------------------------------------------------------
--
-- Type can be one of GEOMETRY, GEOMETRYCOLLECTION, POINT, MULTIPOINT, POLYGON,
-- MULTIPOLYGON, LINESTRING, or MULTILINESTRING.
--
-- Geometry types (except GEOMETRY) are checked for consistency using a CHECK constraint.
-- Uses an ALTER TABLE command to add the geometry column to the table.
-- Addes a row to geometry_columns.
-- Addes a constraint on the table that all the geometries MUST have the same
-- SRID. Checks the coord_dimension to make sure its between 0 and 3.
-- Should also check the precision grid (future expansion).
--
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION AddGeometryColumn(varchar,varchar,varchar,varchar,integer,varchar,integer)
	RETURNS text
	AS
$$
DECLARE
	catalog_name alias for $1;
	schema_name alias for $2;
	table_name alias for $3;
	column_name alias for $4;
	new_srid alias for $5;
	new_type alias for $6;
	new_dim alias for $7;
	rec RECORD;
	sr varchar;
	real_schema name;
	sql text;

BEGIN

	-- Verify geometry type
	IF ( NOT ( (new_type = 'GEOMETRY') OR
			   (new_type = 'GEOMETRYCOLLECTION') OR
			   (new_type = 'POINT') OR
			   (new_type = 'MULTIPOINT') OR
			   (new_type = 'POLYGON') OR
			   (new_type = 'MULTIPOLYGON') OR
			   (new_type = 'LINESTRING') OR
			   (new_type = 'MULTILINESTRING') OR
			   (new_type = 'GEOMETRYCOLLECTIONM') OR
			   (new_type = 'POINTM') OR
			   (new_type = 'MULTIPOINTM') OR
			   (new_type = 'POLYGONM') OR
			   (new_type = 'MULTIPOLYGONM') OR
			   (new_type = 'LINESTRINGM') OR
			   (new_type = 'MULTILINESTRINGM') OR
			   (new_type = 'CIRCULARSTRING') OR
			   (new_type = 'CIRCULARSTRINGM') OR
			   (new_type = 'COMPOUNDCURVE') OR
			   (new_type = 'COMPOUNDCURVEM') OR
			   (new_type = 'CURVEPOLYGON') OR
			   (new_type = 'CURVEPOLYGONM') OR
			   (new_type = 'MULTICURVE') OR
			   (new_type = 'MULTICURVEM') OR
			   (new_type = 'MULTISURFACE') OR
			   (new_type = 'MULTISURFACEM') OR
			   (new_type = 'TRIANGLE') OR
			   (new_type = 'TRIANGLEM') OR
			   (new_type = 'POLYHEDRALSURFACE') OR
			   (new_type = 'POLYHEDRALSURFACEM') OR
			   (new_type = 'TIN') OR
			   (new_type = 'TINM')) )
	THEN
		RAISE EXCEPTION 'Invalid type name - valid ones are:
	POINT, MULTIPOINT,
	LINESTRING, MULTILINESTRING,
	POLYGON, MULTIPOLYGON,
	CIRCULARSTRING, COMPOUNDCURVE, MULTICURVE,
	CURVEPOLYGON, MULTISURFACE,
	GEOMETRY, GEOMETRYCOLLECTION,
	POINTM, MULTIPOINTM,
	LINESTRINGM, MULTILINESTRINGM,
	POLYGONM, MULTIPOLYGONM,
	CIRCULARSTRINGM, COMPOUNDCURVEM, MULTICURVEM
	CURVEPOLYGONM, MULTISURFACEM, TRIANGLE, TRIANGLEM,
	POLYHEDRALSURFACE, POLYHEDRALSURFACEM, TIN, TINM
	or GEOMETRYCOLLECTIONM';
		RETURN 'fail';
	END IF;


	-- Verify dimension
	IF ( (new_dim >4) OR (new_dim <0) ) THEN
		RAISE EXCEPTION 'invalid dimension';
		RETURN 'fail';
	END IF;

	IF ( (new_type LIKE '%M') AND (new_dim!=3) ) THEN
		RAISE EXCEPTION 'TypeM needs 3 dimensions';
		RETURN 'fail';
	END IF;


	-- Verify SRID
	IF ( new_srid != -1 ) THEN
		SELECT SRID INTO sr FROM spatial_ref_sys WHERE SRID = new_srid;
		IF NOT FOUND THEN
			RAISE EXCEPTION 'AddGeometryColumns() - invalid SRID';
			RETURN 'fail';
		END IF;
	END IF;


	-- Verify schema
	IF ( schema_name IS NOT NULL AND schema_name != '' ) THEN
		sql := 'SELECT nspname FROM pg_namespace ' ||
			'WHERE text(nspname) = ' || quote_literal(schema_name) ||
			'LIMIT 1';
		RAISE DEBUG '%', sql;
		EXECUTE sql INTO real_schema;

		IF ( real_schema IS NULL ) THEN
			RAISE EXCEPTION 'Schema % is not a valid schemaname', quote_literal(schema_name);
			RETURN 'fail';
		END IF;
	END IF;

	IF ( real_schema IS NULL ) THEN
		RAISE DEBUG 'Detecting schema';
		sql := 'SELECT n.nspname AS schemaname ' ||
			'FROM pg_catalog.pg_class c ' ||
			  'JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace ' ||
			'WHERE c.relkind = ' || quote_literal('r') ||
			' AND n.nspname NOT IN (' || quote_literal('pg_catalog') || ', ' || quote_literal('pg_toast') || ')' ||
			' AND pg_catalog.pg_table_is_visible(c.oid)' ||
			' AND c.relname = ' || quote_literal(table_name);
		RAISE DEBUG '%', sql;
		EXECUTE sql INTO real_schema;

		IF ( real_schema IS NULL ) THEN
			RAISE EXCEPTION 'Table % does not occur in the search_path', quote_literal(table_name);
			RETURN 'fail';
		END IF;
	END IF;


	-- Add geometry column to table
	sql := 'ALTER TABLE ' ||
		quote_ident(real_schema) || '.' || quote_ident(table_name)
		|| ' ADD COLUMN ' || quote_ident(column_name) ||
		' geometry ';
	RAISE DEBUG '%', sql;
	EXECUTE sql;


	-- Delete stale record in geometry_columns (if any)
	sql := 'DELETE FROM geometry_columns WHERE
		f_table_catalog = ' || quote_literal('') ||
		' AND f_table_schema = ' ||
		quote_literal(real_schema) ||
		' AND f_table_name = ' || quote_literal(table_name) ||
		' AND f_geometry_column = ' || quote_literal(column_name);
	RAISE DEBUG '%', sql;
	EXECUTE sql;


	-- Add record in geometry_columns
	sql := 'INSERT INTO geometry_columns (f_table_catalog,f_table_schema,f_table_name,' ||
										  'f_geometry_column,coord_dimension,srid,type)' ||
		' VALUES (' ||
		quote_literal('') || ',' ||
		quote_literal(real_schema) || ',' ||
		quote_literal(table_name) || ',' ||
		quote_literal(column_name) || ',' ||
		new_dim::text || ',' ||
		new_srid::text || ',' ||
		quote_literal(new_type) || ')';
	RAISE DEBUG '%', sql;
	EXECUTE sql;


	-- Add table CHECKs
	sql := 'ALTER TABLE ' ||
		quote_ident(real_schema) || '.' || quote_ident(table_name)
		|| ' ADD CONSTRAINT '
		|| quote_ident('enforce_srid_' || column_name)
		|| ' CHECK (ST_SRID(' || quote_ident(column_name) ||
		') = ' || new_srid::text || ')' ;
	RAISE DEBUG '%', sql;
	EXECUTE sql;

	sql := 'ALTER TABLE ' ||
		quote_ident(real_schema) || '.' || quote_ident(table_name)
		|| ' ADD CONSTRAINT '
		|| quote_ident('enforce_dims_' || column_name)
		|| ' CHECK (ST_NDims(' || quote_ident(column_name) ||
		') = ' || new_dim::text || ')' ;
	RAISE DEBUG '%', sql;
	EXECUTE sql;

	IF ( NOT (new_type = 'GEOMETRY')) THEN
		sql := 'ALTER TABLE ' ||
			quote_ident(real_schema) || '.' || quote_ident(table_name) || ' ADD CONSTRAINT ' ||
			quote_ident('enforce_geotype_' || column_name) ||
			' CHECK (GeometryType(' ||
			quote_ident(column_name) || ')=' ||
			quote_literal(new_type) || ' OR (' ||
			quote_ident(column_name) || ') is null)';
		RAISE DEBUG '%', sql;
		EXECUTE sql;
	END IF;

	RETURN
		real_schema || '.' ||
		table_name || '.' || column_name ||
		' SRID:' || new_srid::text ||
		' TYPE:' || new_type ||
		' DIMS:' || new_dim::text || ' ';
END;
$$
LANGUAGE 'plpgsql' VOLATILE STRICT;

----------------------------------------------------------------------------
-- ADDGEOMETRYCOLUMN ( <schema>, <table>, <column>, <srid>, <type>, <dim> )
----------------------------------------------------------------------------
--
-- This is a wrapper to the real AddGeometryColumn, for use
-- when catalogue is undefined
--
----------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION AddGeometryColumn(varchar,varchar,varchar,integer,varchar,integer) RETURNS text AS $$
DECLARE
	ret  text;
BEGIN
	SELECT AddGeometryColumn('',$1,$2,$3,$4,$5,$6) into ret;
	RETURN ret;
END;
$$
LANGUAGE 'plpgsql' STABLE STRICT;

----------------------------------------------------------------------------
-- ADDGEOMETRYCOLUMN ( <table>, <column>, <srid>, <type>, <dim> )
----------------------------------------------------------------------------
--
-- This is a wrapper to the real AddGeometryColumn, for use
-- when catalogue and schema are undefined
--
----------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION AddGeometryColumn(varchar,varchar,integer,varchar,integer) RETURNS text AS $$
DECLARE
	ret  text;
BEGIN
	SELECT AddGeometryColumn('','',$1,$2,$3,$4,$5) into ret;
	RETURN ret;
END;
$$
LANGUAGE 'plpgsql' VOLATILE STRICT;

-----------------------------------------------------------------------
-- DROPGEOMETRYCOLUMN
--   <catalogue>, <schema>, <table>, <column>
-----------------------------------------------------------------------
--
-- Removes geometry column reference from geometry_columns table.
-- Drops the column with pgsql >= 73.
-- Make some silly enforcements on it for pgsql < 73
--
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION DropGeometryColumn(varchar, varchar,varchar,varchar)
	RETURNS text
	AS
$$
DECLARE
	catalog_name alias for $1;
	schema_name alias for $2;
	table_name alias for $3;
	column_name alias for $4;
	myrec RECORD;
	okay boolean;
	real_schema name;

BEGIN


	-- Find, check or fix schema_name
	IF ( schema_name != '' ) THEN
		okay = 'f';

		FOR myrec IN SELECT nspname FROM pg_namespace WHERE text(nspname) = schema_name LOOP
			okay := 't';
		END LOOP;

		IF ( okay <> 't' ) THEN
			RAISE NOTICE 'Invalid schema name - using current_schema()';
			SELECT current_schema() into real_schema;
		ELSE
			real_schema = schema_name;
		END IF;
	ELSE
		SELECT current_schema() into real_schema;
	END IF;

	-- Find out if the column is in the geometry_columns table
	okay = 'f';
	FOR myrec IN SELECT * from geometry_columns where f_table_schema = text(real_schema) and f_table_name = table_name and f_geometry_column = column_name LOOP
		okay := 't';
	END LOOP;
	IF (okay <> 't') THEN
		RAISE EXCEPTION 'column not found in geometry_columns table';
		RETURN 'f';
	END IF;

	-- Remove ref from geometry_columns table
	EXECUTE 'delete from geometry_columns where f_table_schema = ' ||
		quote_literal(real_schema) || ' and f_table_name = ' ||
		quote_literal(table_name)  || ' and f_geometry_column = ' ||
		quote_literal(column_name);

	-- Remove table column
	EXECUTE 'ALTER TABLE ' || quote_ident(real_schema) || '.' ||
		quote_ident(table_name) || ' DROP COLUMN ' ||
		quote_ident(column_name);

	RETURN real_schema || '.' || table_name || '.' || column_name ||' effectively removed.';

END;
$$
LANGUAGE 'plpgsql' VOLATILE STRICT;

-----------------------------------------------------------------------
-- DROPGEOMETRYCOLUMN
--   <schema>, <table>, <column>
-----------------------------------------------------------------------
--
-- This is a wrapper to the real DropGeometryColumn, for use
-- when catalogue is undefined
--
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION DropGeometryColumn(varchar,varchar,varchar)
	RETURNS text
	AS
$$
DECLARE
	ret text;
BEGIN
	SELECT DropGeometryColumn('',$1,$2,$3) into ret;
	RETURN ret;
END;
$$
LANGUAGE 'plpgsql' VOLATILE STRICT;

-----------------------------------------------------------------------
-- DROPGEOMETRYCOLUMN
--   <table>, <column>
-----------------------------------------------------------------------
--
-- This is a wrapper to the real DropGeometryColumn, for use
-- when catalogue and schema is undefined.
--
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION DropGeometryColumn(varchar,varchar)
	RETURNS text
	AS
$$
DECLARE
	ret text;
BEGIN
	SELECT DropGeometryColumn('','',$1,$2) into ret;
	RETURN ret;
END;
$$
LANGUAGE 'plpgsql' VOLATILE STRICT;

-----------------------------------------------------------------------
-- DROPGEOMETRYTABLE
--   <catalogue>, <schema>, <table>
-----------------------------------------------------------------------
--
-- Drop a table and all its references in geometry_columns
--
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION DropGeometryTable(varchar, varchar,varchar)
	RETURNS text
	AS
$$
DECLARE
	catalog_name alias for $1;
	schema_name alias for $2;
	table_name alias for $3;
	real_schema name;

BEGIN

	IF ( schema_name = '' ) THEN
		SELECT current_schema() into real_schema;
	ELSE
		real_schema = schema_name;
	END IF;

	-- Remove refs from geometry_columns table
	EXECUTE 'DELETE FROM geometry_columns WHERE ' ||
		'f_table_schema = ' || quote_literal(real_schema) ||
		' AND ' ||
		' f_table_name = ' || quote_literal(table_name);

	-- Remove table
	EXECUTE 'DROP TABLE '
		|| quote_ident(real_schema) || '.' ||
		quote_ident(table_name);

	RETURN
		real_schema || '.' ||
		table_name ||' dropped.';

END;
$$
LANGUAGE 'plpgsql' VOLATILE STRICT;

-----------------------------------------------------------------------
-- DROPGEOMETRYTABLE
--   <schema>, <table>
-----------------------------------------------------------------------
--
-- Drop a table and all its references in geometry_columns
--
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION DropGeometryTable(varchar,varchar) RETURNS text AS
$$ SELECT DropGeometryTable('',$1,$2) $$
LANGUAGE 'sql' WITH (isstrict);

-----------------------------------------------------------------------
-- DROPGEOMETRYTABLE
--   <table>
-----------------------------------------------------------------------
--
-- Drop a table and all its references in geometry_columns
-- For PG>=73 use current_schema()
--
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION DropGeometryTable(varchar) RETURNS text AS
$$ SELECT DropGeometryTable('','',$1) $$
LANGUAGE 'sql' VOLATILE STRICT;

-----------------------------------------------------------------------
-- UPDATEGEOMETRYSRID
--   <catalogue>, <schema>, <table>, <column>, <srid>
-----------------------------------------------------------------------
--
-- Change SRID of all features in a spatially-enabled table
--
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION UpdateGeometrySRID(varchar,varchar,varchar,varchar,integer)
	RETURNS text
	AS
$$
DECLARE
	catalog_name alias for $1;
	schema_name alias for $2;
	table_name alias for $3;
	column_name alias for $4;
	new_srid alias for $5;
	myrec RECORD;
	okay boolean;
	cname varchar;
	real_schema name;

BEGIN


	-- Find, check or fix schema_name
	IF ( schema_name != '' ) THEN
		okay = 'f';

		FOR myrec IN SELECT nspname FROM pg_namespace WHERE text(nspname) = schema_name LOOP
			okay := 't';
		END LOOP;

		IF ( okay <> 't' ) THEN
			RAISE EXCEPTION 'Invalid schema name';
		ELSE
			real_schema = schema_name;
		END IF;
	ELSE
		SELECT INTO real_schema current_schema()::text;
	END IF;

	-- Find out if the column is in the geometry_columns table
	okay = 'f';
	FOR myrec IN SELECT * from geometry_columns where f_table_schema = text(real_schema) and f_table_name = table_name and f_geometry_column = column_name LOOP
		okay := 't';
	END LOOP;
	IF (okay <> 't') THEN
		RAISE EXCEPTION 'column not found in geometry_columns table';
		RETURN 'f';
	END IF;

	-- Update ref from geometry_columns table
	EXECUTE 'UPDATE geometry_columns SET SRID = ' || new_srid::text ||
		' where f_table_schema = ' ||
		quote_literal(real_schema) || ' and f_table_name = ' ||
		quote_literal(table_name)  || ' and f_geometry_column = ' ||
		quote_literal(column_name);

	-- Make up constraint name
	cname = 'enforce_srid_'  || column_name;

	-- Drop enforce_srid constraint
	EXECUTE 'ALTER TABLE ' || quote_ident(real_schema) ||
		'.' || quote_ident(table_name) ||
		' DROP constraint ' || quote_ident(cname);

	-- Update geometries SRID
	EXECUTE 'UPDATE ' || quote_ident(real_schema) ||
		'.' || quote_ident(table_name) ||
		' SET ' || quote_ident(column_name) ||
		' = setSRID(' || quote_ident(column_name) ||
		', ' || new_srid::text || ')';

	-- Reset enforce_srid constraint
	EXECUTE 'ALTER TABLE ' || quote_ident(real_schema) ||
		'.' || quote_ident(table_name) ||
		' ADD constraint ' || quote_ident(cname) ||
		' CHECK (srid(' || quote_ident(column_name) ||
		') = ' || new_srid::text || ')';

	RETURN real_schema || '.' || table_name || '.' || column_name ||' SRID changed to ' || new_srid::text;

END;
$$
LANGUAGE 'plpgsql' VOLATILE STRICT;

-----------------------------------------------------------------------
-- UPDATEGEOMETRYSRID
--   <schema>, <table>, <column>, <srid>
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION UpdateGeometrySRID(varchar,varchar,varchar,integer)
	RETURNS text
	AS $$
DECLARE
	ret  text;
BEGIN
	SELECT UpdateGeometrySRID('',$1,$2,$3,$4) into ret;
	RETURN ret;
END;
$$
LANGUAGE 'plpgsql' VOLATILE STRICT;

-----------------------------------------------------------------------
-- UPDATEGEOMETRYSRID
--   <table>, <column>, <srid>
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION UpdateGeometrySRID(varchar,varchar,integer)
	RETURNS text
	AS $$
DECLARE
	ret  text;
BEGIN
	SELECT UpdateGeometrySRID('','',$1,$2,$3) into ret;
	RETURN ret;
END;
$$
LANGUAGE 'plpgsql' VOLATILE STRICT;

-----------------------------------------------------------------------
-- FIND_SRID( <schema>, <table>, <geom col> )
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION find_srid(varchar,varchar,varchar) RETURNS int4 AS
$$
DECLARE
	schem text;
	tabl text;
	sr int4;
BEGIN
	IF $1 IS NULL THEN
	  RAISE EXCEPTION 'find_srid() - schema is NULL!';
	END IF;
	IF $2 IS NULL THEN
	  RAISE EXCEPTION 'find_srid() - table name is NULL!';
	END IF;
	IF $3 IS NULL THEN
	  RAISE EXCEPTION 'find_srid() - column name is NULL!';
	END IF;
	schem = $1;
	tabl = $2;
-- if the table contains a . and the schema is empty
-- split the table into a schema and a table
-- otherwise drop through to default behavior
	IF ( schem = '' and tabl LIKE '%.%' ) THEN
	 schem = substr(tabl,1,strpos(tabl,'.')-1);
	 tabl = substr(tabl,length(schem)+2);
	ELSE
	 schem = schem || '%';
	END IF;

	select SRID into sr from geometry_columns where f_table_schema like schem and f_table_name = tabl and f_geometry_column = $3;
	IF NOT FOUND THEN
	   RAISE EXCEPTION 'find_srid() - couldnt find the corresponding SRID - is the geometry registered in the GEOMETRY_COLUMNS table?  Is there an uppercase/lowercase missmatch?';
	END IF;
	return sr;
END;
$$
LANGUAGE 'plpgsql' IMMUTABLE STRICT;


---------------------------------------------------------------
-- PROJ support
---------------------------------------------------------------

CREATE OR REPLACE FUNCTION get_proj4_from_srid(integer) RETURNS text AS
$$
BEGIN
	RETURN proj4text::text FROM spatial_ref_sys WHERE srid= $1;
END;
$$
LANGUAGE 'plpgsql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION postgis_transform_geometry(geometry,text,text,int)
	RETURNS geometry
	AS 'MODULE_PATHNAME','transform_geom'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION transform(geometry,integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME','transform'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: transform(geometry,integer)
CREATE OR REPLACE FUNCTION ST_Transform(geometry,integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME','transform'
	LANGUAGE 'C' IMMUTABLE STRICT;


-----------------------------------------------------------------------
-- POSTGIS_VERSION()
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION postgis_version() RETURNS text
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE;

CREATE OR REPLACE FUNCTION postgis_proj_version() RETURNS text
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE;

--
-- IMPORTANT:
-- Starting at 1.1.0 this function is used by postgis_proc_upgrade.pl
-- to extract version of postgis being installed.
-- Do not modify this w/out also changing postgis_proc_upgrade.pl
--
CREATE OR REPLACE FUNCTION postgis_scripts_installed() RETURNS text
	AS _POSTGIS_SQL_SELECT_POSTGIS_SCRIPTS_VERSION
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION postgis_lib_version() RETURNS text
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE; -- a new lib will require a new session

-- NOTE: starting at 1.1.0 this is the same of postgis_lib_version()
CREATE OR REPLACE FUNCTION postgis_scripts_released() RETURNS text
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE;

CREATE OR REPLACE FUNCTION postgis_uses_stats() RETURNS bool
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE;

CREATE OR REPLACE FUNCTION postgis_geos_version() RETURNS text
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE;

CREATE OR REPLACE FUNCTION postgis_libxml_version() RETURNS text
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE;

CREATE OR REPLACE FUNCTION postgis_scripts_build_date() RETURNS text
	AS _POSTGIS_SQL_SELECT_POSTGIS_BUILD_DATE
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION postgis_lib_build_date() RETURNS text
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE;



CREATE OR REPLACE FUNCTION postgis_full_version() RETURNS text
AS $$
DECLARE
	libver text;
	projver text;
	geosver text;
	libxmlver text;
	usestats bool;
	dbproc text;
	relproc text;
	fullver text;
BEGIN
	SELECT postgis_lib_version() INTO libver;
	SELECT postgis_proj_version() INTO projver;
	SELECT postgis_geos_version() INTO geosver;
	SELECT postgis_libxml_version() INTO libxmlver;
	SELECT postgis_uses_stats() INTO usestats;
	SELECT postgis_scripts_installed() INTO dbproc;
	SELECT postgis_scripts_released() INTO relproc;

	fullver = 'POSTGIS="' || libver || '"';

	IF  geosver IS NOT NULL THEN
		fullver = fullver || ' GEOS="' || geosver || '"';
	END IF;

	IF  projver IS NOT NULL THEN
		fullver = fullver || ' PROJ="' || projver || '"';
	END IF;

	IF  libxmlver IS NOT NULL THEN
		fullver = fullver || ' LIBXML="' || libxmlver || '"';
	END IF;

	IF usestats THEN
		fullver = fullver || ' USE_STATS';
	END IF;

	-- fullver = fullver || ' DBPROC="' || dbproc || '"';
	-- fullver = fullver || ' RELPROC="' || relproc || '"';

	IF dbproc != relproc THEN
		fullver = fullver || ' (procs from ' || dbproc || ' need upgrade)';
	END IF;

	RETURN fullver;
END
$$
LANGUAGE 'plpgsql' IMMUTABLE;

---------------------------------------------------------------
-- CASTS
---------------------------------------------------------------
		
CREATE OR REPLACE FUNCTION box2d(geometry)
	RETURNS box2d
	AS 'MODULE_PATHNAME','LWGEOM_to_BOX2DFLOAT4'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION box3d(geometry)
	RETURNS box3d
	AS 'MODULE_PATHNAME','LWGEOM_to_BOX3D'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION box(geometry)
	RETURNS box
	AS 'MODULE_PATHNAME','LWGEOM_to_BOX'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION box2d(box3d)
	RETURNS box2d
	AS 'MODULE_PATHNAME','BOX3D_to_BOX2DFLOAT4'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION box3d(box2d)
	RETURNS box3d
	AS 'MODULE_PATHNAME','BOX2DFLOAT4_to_BOX3D'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION box(box3d)
	RETURNS box
	AS 'MODULE_PATHNAME','BOX3D_to_BOX'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION text(geometry)
	RETURNS text
	AS 'MODULE_PATHNAME','LWGEOM_to_text'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- this is kept for backward-compatibility
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION box3dtobox(box3d)
	RETURNS box
	AS 'SELECT box($1)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry(box2d)
	RETURNS geometry
	AS 'MODULE_PATHNAME','BOX2DFLOAT4_to_LWGEOM'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry(box3d)
	RETURNS geometry
	AS 'MODULE_PATHNAME','BOX3D_to_LWGEOM'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry(text)
	RETURNS geometry
	AS 'MODULE_PATHNAME','parse_WKT_lwgeom'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry(bytea)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_from_bytea'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION bytea(geometry)
	RETURNS bytea
	AS 'MODULE_PATHNAME','LWGEOM_to_bytea'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- 7.3+ explicit casting definitions
CREATE CAST (geometry AS box2d) WITH FUNCTION box2d(geometry) AS IMPLICIT;
CREATE CAST (geometry AS box3d) WITH FUNCTION box3d(geometry) AS IMPLICIT;
CREATE CAST (geometry AS box) WITH FUNCTION box(geometry) AS IMPLICIT;
CREATE CAST (box3d AS box2d) WITH FUNCTION box2d(box3d) AS IMPLICIT;
CREATE CAST (box2d AS box3d) WITH FUNCTION box3d(box2d) AS IMPLICIT;
CREATE CAST (box2d AS geometry) WITH FUNCTION geometry(box2d) AS IMPLICIT;
CREATE CAST (box3d AS box) WITH FUNCTION box(box3d) AS IMPLICIT;
CREATE CAST (box3d AS geometry) WITH FUNCTION geometry(box3d) AS IMPLICIT;
CREATE CAST (text AS geometry) WITH FUNCTION geometry(text) AS IMPLICIT;
CREATE CAST (geometry AS text) WITH FUNCTION text(geometry) AS IMPLICIT;
CREATE CAST (bytea AS geometry) WITH FUNCTION geometry(bytea) AS IMPLICIT;
CREATE CAST (geometry AS bytea) WITH FUNCTION bytea(geometry) AS IMPLICIT;

-- Casts to allow the box3d_extent type to automatically cast to box3d/box2d in queries
CREATE CAST (box3d_extent AS box3d) WITH FUNCTION box3d_extent(box3d_extent) AS IMPLICIT;
CREATE CAST (box3d_extent AS box2d) WITH FUNCTION box2d(box3d_extent) AS IMPLICIT;
CREATE CAST (box3d_extent AS geometry) WITH FUNCTION geometry(box3d_extent) AS IMPLICIT;

---------------------------------------------------------------
-- Algorithms
---------------------------------------------------------------

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Simplify(geometry, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_simplify2d'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- SnapToGrid(input, xoff, yoff, xsize, ysize)
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION SnapToGrid(geometry, float8, float8, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_snaptogrid'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_SnapToGrid(geometry, float8, float8, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_snaptogrid'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- SnapToGrid(input, xsize, ysize) # offsets=0
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION SnapToGrid(geometry, float8, float8)
	RETURNS geometry
	AS 'SELECT ST_SnapToGrid($1, 0, 0, $2, $3)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_SnapToGrid(geometry, float8, float8)
	RETURNS geometry
	AS 'SELECT ST_SnapToGrid($1, 0, 0, $2, $3)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- SnapToGrid(input, size) # xsize=ysize=size, offsets=0
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION SnapToGrid(geometry, float8)
	RETURNS geometry
	AS 'SELECT ST_SnapToGrid($1, 0, 0, $2, $2)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_SnapToGrid(geometry, float8)
	RETURNS geometry
	AS 'SELECT ST_SnapToGrid($1, 0, 0, $2, $2)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- SnapToGrid(input, point_offsets, xsize, ysize, zsize, msize)
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION SnapToGrid(geometry, geometry, float8, float8, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_snaptogrid_pointoff'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_SnapToGrid(geometry, geometry, float8, float8, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_snaptogrid_pointoff'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Segmentize(geometry, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_segmentize2d'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Segmentize(geometry, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_segmentize2d'
	LANGUAGE 'C' IMMUTABLE STRICT;

---------------------------------------------------------------
-- LRS
---------------------------------------------------------------

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION line_interpolate_point(geometry, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_line_interpolate_point'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_line_interpolate_point(geometry, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_line_interpolate_point'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION line_substring(geometry, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_line_substring'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_line_substring(geometry, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_line_substring'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION line_locate_point(geometry, geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_line_locate_point'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_line_locate_point(geometry, geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_line_locate_point'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION locate_between_measures(geometry, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_locate_between_m'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_locate_between_measures(geometry, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_locate_between_m'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION locate_along_measure(geometry, float8)
	RETURNS geometry
	AS $$ SELECT locate_between_measures($1, $2, $2) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_locate_along_measure(geometry, float8)
	RETURNS geometry
	AS $$ SELECT locate_between_measures($1, $2, $2) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION ST_AddMeasure(geometry, float8, float8) 
	RETURNS geometry 
	AS 'MODULE_PATHNAME', 'ST_AddMeasure' 
	LANGUAGE 'C' IMMUTABLE STRICT;
    
---------------------------------------------------------------
-- GEOS
---------------------------------------------------------------

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION intersection(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','intersection'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: intersection(geometry,geometry)
CREATE OR REPLACE FUNCTION ST_Intersection(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','intersection'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION buffer(geometry,float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME','buffer'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- PostGIS equivalent function: buffer(geometry,float8)
CREATE OR REPLACE FUNCTION ST_Buffer(geometry,float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME','buffer'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.5.0 - requires GEOS-3.2 or higher
CREATE OR REPLACE FUNCTION _ST_Buffer(geometry,float8,cstring)
	RETURNS geometry
	AS 'MODULE_PATHNAME','buffer'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Buffer(geometry,float8,integer)
	RETURNS geometry
	AS $$ SELECT _ST_Buffer($1, $2,
		CAST('quad_segs='||CAST($3 AS text) as cstring))
	   $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION ST_buffer(geometry,float8,text)
	RETURNS geometry
	AS $$ SELECT _ST_Buffer($1, $2,
		CAST( regexp_replace($3, '^[0123456789]+$',
			'quad_segs='||$3) AS cstring)
		)
	   $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION buffer(geometry,float8,integer)
	RETURNS geometry
	AS 'SELECT ST_Buffer($1, $2, $3)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION convexhull(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','convexhull'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- PostGIS equivalent function: convexhull(geometry)
CREATE OR REPLACE FUNCTION ST_ConvexHull(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','convexhull'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Only accepts LINESTRING as parameters.
-- Availability: 1.4.0
CREATE OR REPLACE FUNCTION _ST_LineCrossingDirection(geometry, geometry)
	RETURNS integer
	AS 'MODULE_PATHNAME', 'ST_LineCrossingDirection'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.4.0
CREATE OR REPLACE FUNCTION ST_LineCrossingDirection(geometry, geometry)
	RETURNS integer AS
	$$ SELECT CASE WHEN NOT $1 && $2 THEN 0 ELSE _ST_LineCrossingDirection($1,$2) END $$
	LANGUAGE 'sql' IMMUTABLE;


-- Only accepts LINESTRING as parameters.
-- Availability: 1.4.0
CREATE OR REPLACE FUNCTION ST_LocateBetweenElevations(geometry, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'ST_LocateBetweenElevations'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Requires GEOS >= 3.0.0
-- Availability: 1.3.3
CREATE OR REPLACE FUNCTION ST_SimplifyPreserveTopology(geometry, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME','topologypreservesimplify'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Requires GEOS >= 3.1.0
-- Availability: 1.4.0
CREATE OR REPLACE FUNCTION ST_IsValidReason(geometry)
	RETURNS text
	AS 'MODULE_PATHNAME', 'isvalidreason'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 2.0.0
CREATE TYPE valid_detail AS (valid bool, reason varchar, location geometry);

-- Requires GEOS >= 3.3.0
-- Availability: 2.0.0
CREATE OR REPLACE FUNCTION ST_IsValidDetail(geometry)
	RETURNS valid_detail
	AS 'MODULE_PATHNAME', 'isvaliddetail'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

#if POSTGIS_GEOS_VERSION >= 32
-- Requires GEOS >= 3.2.0
-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION ST_HausdorffDistance(geometry, geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'hausdorffdistance'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;
#endif

#if POSTGIS_GEOS_VERSION >= 32
-- Requires GEOS >= 3.2.0
-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION ST_HausdorffDistance(geometry, geometry, float8)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'hausdorffdistancedensify'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;
#endif

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION difference(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','difference'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: difference(geometry,geometry)
CREATE OR REPLACE FUNCTION ST_Difference(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','difference'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION boundary(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','boundary'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: boundary(geometry)
CREATE OR REPLACE FUNCTION ST_Boundary(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','boundary'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION symdifference(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','symdifference'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: symdifference(geometry,geometry)
CREATE OR REPLACE FUNCTION ST_SymDifference(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','symdifference'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION symmetricdifference(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','symdifference'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_symmetricdifference(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','symdifference'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION GeomUnion(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','geomunion'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: GeomUnion(geometry,geometry)
CREATE OR REPLACE FUNCTION ST_Union(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','geomunion'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- ST_RemoveRepeatedPoints(in geometry)
--
-- Removes duplicate vertices in input.
-- Only checks consecutive points for lineal and polygonal geoms.
-- Checks all points for multipoint geoms.
--
-- Availability: 2.0.0
CREATE OR REPLACE FUNCTION ST_RemoveRepeatedPoints(geometry)
       RETURNS geometry
       AS 'MODULE_PATHNAME', 'ST_RemoveRepeatedPoints'
       LANGUAGE 'C' IMMUTABLE STRICT
       COST 100;

--------------------------------------------------------------------------------
-- ST_CleanGeometry / ST_MakeValid
--------------------------------------------------------------------------------

-- ST_MakeValid(in geometry)
--
-- Try to make the input valid maintaining the boundary profile.
-- May return a collection.
-- May return a geometry with inferior dimensions (dimensional collapses).
-- May return NULL if can't handle input.
--
-- Requires: GEOS-3.3.0
-- Availability: 2.0.0
CREATE OR REPLACE FUNCTION ST_MakeValid(geometry)
       RETURNS geometry
       AS 'MODULE_PATHNAME', 'ST_MakeValid'
       LANGUAGE 'C' IMMUTABLE STRICT
       COST 100;

-- ST_CleanGeometry(in geometry)
--
-- Make input:
-- 	- Simple (lineal components)
--	- Valid (polygonal components)
--	- Obeying the RHR (if polygonal)
--	- Simplified of consecutive duplicated points 
-- Ensuring:
--	- No input vertexes are discarded (except consecutive repeated ones)
--	- Output geometry type matches input
--
-- Returns NULL on failure.
-- 
-- Requires: GEOS-3.3.0
-- Availability: 2.0.0
CREATE OR REPLACE FUNCTION ST_CleanGeometry(geometry)
       RETURNS geometry
       AS 'MODULE_PATHNAME', 'ST_CleanGeometry'
       LANGUAGE 'C' IMMUTABLE STRICT
       COST 100;

--------------------------------------------------------------------------------
-- ST_Split
--------------------------------------------------------------------------------

-- ST_Split(in geometry, blade geometry)
--
-- Split a geometry in parts after cutting it with given blade.
-- Returns a collection containing all parts.
--
-- Note that multi-part geometries will be returned exploded,
-- no matter relation to blade.
-- 
-- Availability: 2.0.0
--
CREATE OR REPLACE FUNCTION ST_Split(geometry, geometry)
       RETURNS geometry
       AS 'MODULE_PATHNAME', 'ST_Split'
       LANGUAGE 'C' IMMUTABLE STRICT
       COST 100;

--------------------------------------------------------------------------------
-- ST_SharedPaths
--------------------------------------------------------------------------------

-- ST_SharedPaths(lineal1 geometry, lineal1 geometry)
--
-- Returns a collection containing paths shared by the two
-- input geometries. Those going in the same direction are
-- in the first element of the collection, those going in the
-- opposite direction are in the second element.
--
-- The paths themselves are given in the direction of the
-- first geometry.
-- 
-- Availability: 2.0.0
-- Requires GEOS >= 3.3.0
--
CREATE OR REPLACE FUNCTION ST_SharedPaths(geometry, geometry)
       RETURNS geometry
       AS 'MODULE_PATHNAME', 'ST_SharedPaths'
       LANGUAGE 'C' IMMUTABLE STRICT
       COST 100;

--------------------------------------------------------------------------------
-- ST_Snap
--------------------------------------------------------------------------------

-- ST_Snap(g1 geometry, g2 geometry, tolerance float8)
--
-- Snap first geometry against second.
--
-- Availability: 2.0.0
-- Requires GEOS >= 3.3.0
--
CREATE OR REPLACE FUNCTION ST_Snap(geometry, geometry, float8)
       RETURNS geometry
       AS 'MODULE_PATHNAME', 'ST_Snap'
       LANGUAGE 'C' IMMUTABLE STRICT
       COST 100;

--------------------------------------------------------------------------------
-- ST_RelateMatch
--------------------------------------------------------------------------------

-- ST_RelateMatch(matrix text, pattern text)
--
-- Returns true if pattern 'pattern' matches DE9 intersection matrix 'matrix'
--
-- Availability: 2.0.0
-- Requires GEOS >= 3.3.0
--
CREATE OR REPLACE FUNCTION ST_RelateMatch(text, text)
       RETURNS bool
       AS 'MODULE_PATHNAME', 'ST_RelateMatch'
       LANGUAGE 'C' IMMUTABLE STRICT
       COST 100;


--------------------------------------------------------------------------------
-- Aggregates and their supporting functions
--------------------------------------------------------------------------------

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION collect(geometry, geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_collect'
	LANGUAGE 'C' IMMUTABLE;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Collect(geometry, geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_collect'
	LANGUAGE 'C' IMMUTABLE;

-- Deprecation in 1.2.3
CREATE AGGREGATE memcollect(
	sfunc = ST_collect,
	basetype = geometry,
	stype = geometry
	);

-- Availability: 1.2.2
CREATE AGGREGATE ST_MemCollect(
	sfunc = ST_collect,
	basetype = geometry,
	stype = geometry
	);

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Collect(geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_collect_garray'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE AGGREGATE MemGeomUnion (
	basetype = geometry,
	sfunc = geomunion,
	stype = geometry
	);

-- Availability: 1.2.2
CREATE AGGREGATE ST_MemUnion (
	basetype = geometry,
	sfunc = ST_Union,
	stype = geometry
	);

--
-- pgis_abs
-- Container type to hold the ArrayBuildState pointer as it passes through
-- the geometry array accumulation aggregate.
--
CREATE OR REPLACE FUNCTION pgis_abs_in(cstring)
	RETURNS pgis_abs
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION pgis_abs_out(pgis_abs)
	RETURNS cstring
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE TYPE pgis_abs (
	internallength = 8,
	input = pgis_abs_in,
	output = pgis_abs_out,
	alignment = double
);

-- Availability: 1.4.0
CREATE OR REPLACE FUNCTION pgis_geometry_accum_transfn(pgis_abs, geometry)
	RETURNS pgis_abs
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C';

-- Availability: 1.4.0
CREATE OR REPLACE FUNCTION pgis_geometry_accum_finalfn(pgis_abs)
	RETURNS geometry[]
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C';

-- Availability: 1.4.0
CREATE OR REPLACE FUNCTION pgis_geometry_union_finalfn(pgis_abs)
	RETURNS geometry
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C';

-- Availability: 1.4.0
CREATE OR REPLACE FUNCTION pgis_geometry_collect_finalfn(pgis_abs)
	RETURNS geometry
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C';

-- Availability: 1.4.0
CREATE OR REPLACE FUNCTION pgis_geometry_polygonize_finalfn(pgis_abs)
	RETURNS geometry
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C';

-- Availability: 1.4.0
CREATE OR REPLACE FUNCTION pgis_geometry_makeline_finalfn(pgis_abs)
	RETURNS geometry
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C';

-- Deprecation in: 1.2.3
CREATE AGGREGATE accum (
	sfunc = pgis_geometry_accum_transfn,
	basetype = geometry,
	stype = pgis_abs,
	finalfunc = pgis_geometry_accum_finalfn
	);

-- Availability: 1.2.2
CREATE AGGREGATE ST_Accum (
	sfunc = pgis_geometry_accum_transfn,
	basetype = geometry,
	stype = pgis_abs,
	finalfunc = pgis_geometry_accum_finalfn
	);

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION unite_garray (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'pgis_union_geometry_array'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.4.0
CREATE OR REPLACE FUNCTION ST_unite_garray (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME','pgis_union_geometry_array'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.4.0
CREATE OR REPLACE FUNCTION ST_Union (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME','pgis_union_geometry_array'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE AGGREGATE ST_Union (
	basetype = geometry,
	sfunc = pgis_geometry_accum_transfn,
	stype = pgis_abs,
	finalfunc = pgis_geometry_union_finalfn
	);

-- Deprecation in 1.2.3
CREATE AGGREGATE collect (
	basetype = geometry,
	sfunc = pgis_geometry_accum_transfn,
	stype = pgis_abs,
	finalfunc = pgis_geometry_collect_finalfn
);

-- Availability: 1.2.2
CREATE AGGREGATE ST_Collect (
	BASETYPE = geometry,
	SFUNC = pgis_geometry_accum_transfn,
	STYPE = pgis_abs,
	FINALFUNC = pgis_geometry_collect_finalfn
	);

-- Deprecation in 1.2.3
CREATE AGGREGATE Polygonize (
	BASETYPE = geometry,
	SFUNC = pgis_geometry_accum_transfn,
	STYPE = pgis_abs,
	FINALFUNC = pgis_geometry_polygonize_finalfn
	);

-- Availability: 1.2.2
CREATE AGGREGATE ST_Polygonize (
	BASETYPE = geometry,
	SFUNC = pgis_geometry_accum_transfn,
	STYPE = pgis_abs,
	FINALFUNC = pgis_geometry_polygonize_finalfn
	);

-- Deprecation in 1.2.3
CREATE AGGREGATE makeline (
	BASETYPE = geometry,
	SFUNC = pgis_geometry_accum_transfn,
	STYPE = pgis_abs,
	FINALFUNC = pgis_geometry_makeline_finalfn
	);

-- Availability: 1.2.2
CREATE AGGREGATE ST_MakeLine (
	BASETYPE = geometry,
	SFUNC = pgis_geometry_accum_transfn,
	STYPE = pgis_abs,
	FINALFUNC = pgis_geometry_makeline_finalfn
	);



--------------------------------------------------------------------------------

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION relate(geometry,geometry)
	RETURNS text
	AS 'MODULE_PATHNAME','relate_full'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Relate(geometry,geometry)
	RETURNS text
	AS 'MODULE_PATHNAME','relate_full'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION relate(geometry,geometry,text)
	RETURNS boolean
	AS 'MODULE_PATHNAME','relate_pattern'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: relate(geometry,geometry,text)
CREATE OR REPLACE FUNCTION ST_Relate(geometry,geometry,text)
	RETURNS boolean
	AS 'MODULE_PATHNAME','relate_pattern'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION disjoint(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: disjoint(geometry,geometry)
CREATE OR REPLACE FUNCTION ST_Disjoint(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME','disjoint'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION touches(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: touches(geometry,geometry)
CREATE OR REPLACE FUNCTION _ST_Touches(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME','touches'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.2.2
-- Inlines index magic
CREATE OR REPLACE FUNCTION ST_Touches(geometry,geometry)
	RETURNS boolean
	AS 'SELECT $1 && $2 AND _ST_Touches($1,$2)'
	LANGUAGE 'SQL' IMMUTABLE;

-- Availability: 1.3.4
CREATE OR REPLACE FUNCTION _ST_DWithin(geometry,geometry,float8)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'LWGEOM_dwithin'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_DWithin(geometry, geometry, float8)
	RETURNS boolean
	AS 'SELECT $1 && ST_Expand($2,$3) AND $2 && ST_Expand($1,$3) AND _ST_DWithin($1, $2, $3)'
	LANGUAGE 'SQL' IMMUTABLE;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION intersects(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: intersects(geometry,geometry)
CREATE OR REPLACE FUNCTION _ST_Intersects(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME','intersects'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.2.2
-- Inlines index magic
CREATE OR REPLACE FUNCTION ST_Intersects(geometry,geometry)
	RETURNS boolean
	AS 'SELECT $1 && $2 AND _ST_Intersects($1,$2)'
	LANGUAGE 'SQL' IMMUTABLE;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION crosses(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: crosses(geometry,geometry)
CREATE OR REPLACE FUNCTION _ST_Crosses(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME','crosses'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.2.2
-- Inlines index magic
CREATE OR REPLACE FUNCTION ST_Crosses(geometry,geometry)
	RETURNS boolean
	AS 'SELECT $1 && $2 AND _ST_Crosses($1,$2)'
	LANGUAGE 'SQL' IMMUTABLE;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION within(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: within(geometry,geometry)
CREATE OR REPLACE FUNCTION _ST_Within(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME','within'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.2.2
-- Inlines index magic
CREATE OR REPLACE FUNCTION ST_Within(geometry,geometry)
	RETURNS boolean
	AS 'SELECT $1 && $2 AND _ST_Within($1,$2)'
	LANGUAGE 'SQL' IMMUTABLE;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Contains(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: contains(geometry,geometry)
CREATE OR REPLACE FUNCTION _ST_Contains(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME','contains'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.2.2
-- Inlines index magic
CREATE OR REPLACE FUNCTION ST_Contains(geometry,geometry)
	RETURNS boolean
	AS 'SELECT $1 && $2 AND _ST_Contains($1,$2)'
	LANGUAGE 'SQL' IMMUTABLE;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION _ST_CoveredBy(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'coveredby'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_CoveredBy(geometry,geometry)
	RETURNS boolean
	AS 'SELECT $1 && $2 AND _ST_CoveredBy($1,$2)'
	LANGUAGE 'SQL' IMMUTABLE;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION _ST_Covers(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'covers'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.2.2
-- Inlines index magic
CREATE OR REPLACE FUNCTION ST_Covers(geometry,geometry)
	RETURNS boolean
	AS 'SELECT $1 && $2 AND _ST_Covers($1,$2)'
	LANGUAGE 'SQL' IMMUTABLE;

-- Availability: 1.4.0
CREATE OR REPLACE FUNCTION _ST_ContainsProperly(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME','containsproperly'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.4.0
-- Inlines index magic
CREATE OR REPLACE FUNCTION ST_ContainsProperly(geometry,geometry)
	RETURNS boolean
	AS 'SELECT $1 && $2 AND _ST_ContainsProperly($1,$2)'
	LANGUAGE 'SQL' IMMUTABLE;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION overlaps(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: overlaps(geometry,geometry)
CREATE OR REPLACE FUNCTION _ST_Overlaps(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME','overlaps'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.2.2
-- Inlines index magic
CREATE OR REPLACE FUNCTION ST_Overlaps(geometry,geometry)
	RETURNS boolean
	AS 'SELECT $1 && $2 AND _ST_Overlaps($1,$2)'
	LANGUAGE 'SQL' IMMUTABLE;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION IsValid(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'isvalid'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- PostGIS equivalent function: IsValid(geometry)
-- TODO: change null returns to true
CREATE OR REPLACE FUNCTION ST_IsValid(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'isvalid'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- This is also available w/out GEOS
CREATE OR REPLACE FUNCTION Centroid(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- PostGIS equivalent function: Centroid(geometry)
CREATE OR REPLACE FUNCTION ST_Centroid(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'centroid'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION IsRing(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: IsRing(geometry)
CREATE OR REPLACE FUNCTION ST_IsRing(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'isring'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION PointOnSurface(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: PointOnSurface(geometry)
CREATE OR REPLACE FUNCTION ST_PointOnSurface(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'pointonsurface'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION IsSimple(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'issimple'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: IsSimple(geometry)
CREATE OR REPLACE FUNCTION ST_IsSimple(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'issimple'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 2.0.0
CREATE OR REPLACE FUNCTION ST_IsCollection(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'ST_IsCollection'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Equals(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME','geomequals'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION _ST_Equals(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME','geomequals'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

-- Availability: 1.2.1
CREATE OR REPLACE FUNCTION ST_Equals(geometry,geometry)
	RETURNS boolean
	AS 'SELECT $1 && $2 AND _ST_Equals($1,$2)'
	LANGUAGE 'SQL' IMMUTABLE;


-----------------------------------------------------------------------
-- GML & KML INPUT
-- Availability: 1.5.0
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION ST_GeomFromGML(text)
	RETURNS geometry
	AS 'MODULE_PATHNAME','geom_from_gml'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ST_GMLToSQL(text)
	RETURNS geometry
	AS 'MODULE_PATHNAME','geom_from_gml'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ST_GeomFromKML(text)
	RETURNS geometry
	AS 'MODULE_PATHNAME','geom_from_kml'
	LANGUAGE 'C' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- SVG OUTPUT
-----------------------------------------------------------------------
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsSVG(geometry,int4,int4)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asSVG'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_AsSVG(geometry,int4,int4)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asSVG'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsSVG(geometry,int4)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asSVG'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_AsSVG(geometry,int4)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asSVG'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsSVG(geometry)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asSVG'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_AsSVG(geometry)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asSVG'
	LANGUAGE 'C' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- GML OUTPUT
-----------------------------------------------------------------------
-- _ST_AsGML(version, geom, precision, option, prefix)
CREATE OR REPLACE FUNCTION _ST_AsGML(int4, geometry, int4, int4, text)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asGML'
	LANGUAGE 'C' IMMUTABLE;

-- AsGML(geom, precision) / version=2
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsGML(geometry, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML(2, $1, $2, 0, null)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_AsGML(geometry, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML(2, $1, $2, 0, null)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- AsGML(geom) / precision=15 version=2
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsGML(geometry)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML(2, $1, 15, 0, null)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_AsGML(geometry)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML(2, $1, 15, 0, null)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsGML(version, geom) / precision=15 
-- Availability: 1.3.2
CREATE OR REPLACE FUNCTION ST_AsGML(int4, geometry)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML($1, $2, 15, 0, null)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsGML(version, geom, precision)
-- Availability: 1.3.2
CREATE OR REPLACE FUNCTION ST_AsGML(int4, geometry, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML($1, $2, $3, 0, null)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsGML (geom, precision, option) / version=2
-- Availability: 1.4.0
CREATE OR REPLACE FUNCTION ST_AsGML(geometry, int4, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML(2, $1, $2, $3, null)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsGML(version, geom, precision, option)
-- Availability: 1.4.0
CREATE OR REPLACE FUNCTION ST_AsGML(int4, geometry, int4, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML($1, $2, $3, $4, null)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;


-- ST_AsGML(version, geom, precision, option, prefix)
-- Availability: 2.0.0
CREATE OR REPLACE FUNCTION ST_AsGML(int4, geometry, int4, int4, text)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML($1, $2, $3, $4, $5)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- KML OUTPUT
-----------------------------------------------------------------------
-- _ST_AsKML(version, geom, precision)
CREATE OR REPLACE FUNCTION _ST_AsKML(int4, geometry, int4, text)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asKML'
	LANGUAGE 'C' IMMUTABLE;

-- AsKML(geom, precision) / version=2
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsKML(geometry, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsKML(2, transform($1,4326), $2, null)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_AsKML(geometry, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsKML(2, ST_Transform($1,4326), $2, null)'
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

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_AsKML(geometry)
	RETURNS TEXT
	AS 'SELECT _ST_AsKML(2, ST_Transform($1,4326), 15, null)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsKML(version, geom) / precision=15 version=2
-- Availability: 1.3.2
CREATE OR REPLACE FUNCTION ST_AsKML(int4, geometry)
	RETURNS TEXT
	AS 'SELECT _ST_AsKML($1, ST_Transform($2,4326), 15, null)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsKML(version, geom, precision)
-- Availability: 1.3.2
CREATE OR REPLACE FUNCTION ST_AsKML(int4, geometry, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsKML($1, ST_Transform($2,4326), $3, null)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsKML(version, geom, precision, text)
-- Availability: 2.0.0
CREATE OR REPLACE FUNCTION ST_AsKML(int4, geometry, int4, text)
	RETURNS TEXT
	AS 'SELECT _ST_AsKML($1, ST_Transform($2,4326), $3, $4)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;


-----------------------------------------------------------------------
-- GEOJSON OUTPUT
-- Availability: 1.3.4
-----------------------------------------------------------------------
-- _ST_AsGeoJson(version, geom, precision, options)
CREATE OR REPLACE FUNCTION _ST_AsGeoJson(int4, geometry, int4, int4)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asGeoJson'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- ST_AsGeoJson(geom, precision) / version=1 options=0
CREATE OR REPLACE FUNCTION ST_AsGeoJson(geometry, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGeoJson(1, $1, $2, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsGeoJson(geom) / precision=15 version=1 options=0
CREATE OR REPLACE FUNCTION ST_AsGeoJson(geometry)
	RETURNS TEXT
	AS 'SELECT _ST_AsGeoJson(1, $1, 15, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsGeoJson(version, geom) / precision=15 options=0
CREATE OR REPLACE FUNCTION ST_AsGeoJson(int4, geometry)
	RETURNS TEXT
	AS 'SELECT _ST_AsGeoJson($1, $2, 15, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsGeoJson(version, geom, precision) / options=0
CREATE OR REPLACE FUNCTION ST_AsGeoJson(int4, geometry, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGeoJson($1, $2, $3, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsGeoJson(geom, precision, options) / version=1
CREATE OR REPLACE FUNCTION ST_AsGeoJson(geometry, int4, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGeoJson(1, $1, $2, $3)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsGeoJson(version, geom, precision,options)
CREATE OR REPLACE FUNCTION ST_AsGeoJson(int4, geometry, int4, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGeoJson($1, $2, $3, $4)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

------------------------------------------------------------------------
-- GeoHash (geohash.org)
------------------------------------------------------------------------

-- Availability 1.4.0
CREATE OR REPLACE FUNCTION ST_GeoHash(geometry, int4)
	RETURNS TEXT
		AS 'MODULE_PATHNAME', 'ST_GeoHash'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability 1.4.0
CREATE OR REPLACE FUNCTION ST_GeoHash(geometry)
	RETURNS TEXT
	AS 'SELECT ST_GeoHash($1, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

------------------------------------------------------------------------
-- OGC defined
------------------------------------------------------------------------

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION NumPoints(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_numpoints_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: NumPoints(geometry)
CREATE OR REPLACE FUNCTION ST_NumPoints(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_numpoints_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION NumGeometries(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_numgeometries_collection'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: NumGeometries(geometry)
CREATE OR REPLACE FUNCTION ST_NumGeometries(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_numgeometries_collection'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION GeometryN(geometry,integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_geometryn_collection'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: GeometryN(geometry)
CREATE OR REPLACE FUNCTION ST_GeometryN(geometry,integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_geometryn_collection'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Dimension(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_dimension'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: Dimension(geometry)
CREATE OR REPLACE FUNCTION ST_Dimension(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_dimension'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION ExteriorRing(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_exteriorring_polygon'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: ExteriorRing(geometry)
CREATE OR REPLACE FUNCTION ST_ExteriorRing(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_exteriorring_polygon'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION NumInteriorRings(geometry)
	RETURNS integer
	AS 'MODULE_PATHNAME','LWGEOM_numinteriorrings_polygon'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: NumInteriorRings(geometry)
CREATE OR REPLACE FUNCTION ST_NumInteriorRings(geometry)
	RETURNS integer
	AS 'MODULE_PATHNAME','LWGEOM_numinteriorrings_polygon'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION NumInteriorRing(geometry)
	RETURNS integer
	AS 'MODULE_PATHNAME','LWGEOM_numinteriorrings_polygon'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_NumInteriorRing(geometry)
	RETURNS integer
	AS 'MODULE_PATHNAME','LWGEOM_numinteriorrings_polygon'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION InteriorRingN(geometry,integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_interiorringn_polygon'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: InteriorRingN(geometry)
CREATE OR REPLACE FUNCTION ST_InteriorRingN(geometry,integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_interiorringn_polygon'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION GeometryType(geometry)
	RETURNS text
	AS 'MODULE_PATHNAME', 'LWGEOM_getTYPE'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Not quite equivalent to GeometryType
CREATE OR REPLACE FUNCTION ST_GeometryType(geometry)
	RETURNS text
	AS 'MODULE_PATHNAME', 'geometry_geometrytype'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION PointN(geometry,integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_pointn_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: PointN(geometry,integer)
CREATE OR REPLACE FUNCTION ST_PointN(geometry,integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_pointn_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 2.0.0
CREATE OR REPLACE FUNCTION ST_NumPatches(geometry)
	RETURNS int4
	AS '
	SELECT CASE WHEN ST_GeometryType($1) = ''ST_PolyhedralSurface''
	THEN ST_NumGeometries($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 2.0.0
CREATE OR REPLACE FUNCTION ST_PatchN(geometry, integer)
	RETURNS geometry
	AS '
	SELECT CASE WHEN ST_GeometryType($1) = ''ST_PolyhedralSurface''
	THEN ST_GeometryN($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;


-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION X(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_x_point'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: X(geometry)
CREATE OR REPLACE FUNCTION ST_X(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_x_point'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Y(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_y_point'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: Y(geometry)
CREATE OR REPLACE FUNCTION ST_Y(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_y_point'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Z(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_z_point'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_Z(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_z_point'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION M(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_m_point'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_M(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_m_point'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION StartPoint(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_startpoint_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: StartPoint(geometry))
CREATE OR REPLACE FUNCTION ST_StartPoint(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_startpoint_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION EndPoint(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_endpoint_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: EndPoint(geometry)
CREATE OR REPLACE FUNCTION ST_EndPoint(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_endpoint_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION IsClosed(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'LWGEOM_isclosed'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: IsClosed(geometry)
CREATE OR REPLACE FUNCTION ST_IsClosed(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'LWGEOM_isclosed'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION IsEmpty(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'LWGEOM_isempty'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: IsEmpty(geometry)
CREATE OR REPLACE FUNCTION ST_IsEmpty(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'LWGEOM_isempty'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION SRID(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME','LWGEOM_get_srid'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: getSRID(geometry)
CREATE OR REPLACE FUNCTION ST_SRID(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME','LWGEOM_get_srid'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION SetSRID(geometry,int4)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_set_srid'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_SetSRID(geometry,int4)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_set_srid'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsBinary(geometry)
	RETURNS bytea
	AS 'MODULE_PATHNAME','LWGEOM_asBinary'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: AsBinary(geometry)
CREATE OR REPLACE FUNCTION ST_AsBinary(geometry)
	RETURNS bytea
	AS 'MODULE_PATHNAME','LWGEOM_asBinary'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsBinary(geometry,text)
	RETURNS bytea
	AS 'MODULE_PATHNAME','LWGEOM_asBinary'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_AsBinary(geometry,text)
	RETURNS bytea
	AS 'MODULE_PATHNAME','LWGEOM_asBinary'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsText(geometry)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asText'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- PostGIS equivalent function: AsText(geometry)
CREATE OR REPLACE FUNCTION ST_AsText(geometry)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asText'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION GeometryFromText(text)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_from_text'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_GeometryFromText(text)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_from_text'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION GeometryFromText(text, int4)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_from_text'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_GeometryFromText(text, int4)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_from_text'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION GeomFromText(text)
	RETURNS geometry AS 'SELECT geometryfromtext($1)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_GeomFromText(text)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_from_text'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION GeomFromText(text, int4)
	RETURNS geometry AS 'SELECT geometryfromtext($1, $2)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- PostGIS equivalent function: ST_GeometryFromText(text, int4)
CREATE OR REPLACE FUNCTION ST_GeomFromText(text, int4)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_from_text'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION PointFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1)) = ''POINT''
	THEN GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_PointFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromText($1)) = ''POINT''
	THEN ST_GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION PointFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1, $2)) = ''POINT''
	THEN GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- PostGIS equivalent function: PointFromText(text, int4)
-- TODO: improve this ... by not duplicating constructor time.
CREATE OR REPLACE FUNCTION ST_PointFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromText($1, $2)) = ''POINT''
	THEN ST_GeomFromText($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION LineFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1)) = ''LINESTRING''
	THEN GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_LineFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromText($1)) = ''LINESTRING''
	THEN ST_GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION LineFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1, $2)) = ''LINESTRING''
	THEN GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- PostGIS equivalent function: LineFromText(text, int4)
CREATE OR REPLACE FUNCTION ST_LineFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1, $2)) = ''LINESTRING''
	THEN GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION LineStringFromText(text)
	RETURNS geometry
	AS 'SELECT LineFromText($1)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION LineStringFromText(text, int4)
	RETURNS geometry
	AS 'SELECT LineFromText($1, $2)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION PolyFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1)) = ''POLYGON''
	THEN GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_PolyFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromText($1)) = ''POLYGON''
	THEN ST_GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION PolyFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1, $2)) = ''POLYGON''
	THEN GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- PostGIS equivalent function: ST_PolygonFromText(text, int4)
CREATE OR REPLACE FUNCTION ST_PolyFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromText($1, $2)) = ''POLYGON''
	THEN ST_GeomFromText($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION PolygonFromText(text, int4)
	RETURNS geometry
	AS 'SELECT PolyFromText($1, $2)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_PolygonFromText(text, int4)
	RETURNS geometry
	AS 'SELECT PolyFromText($1, $2)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION PolygonFromText(text)
	RETURNS geometry
	AS 'SELECT PolyFromText($1)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_PolygonFromText(text)
	RETURNS geometry
	AS 'SELECT ST_PolyFromText($1)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MLineFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE
	WHEN geometrytype(GeomFromText($1, $2)) = ''MULTILINESTRING''
	THEN GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- PostGIS equivalent function: MLineFromText(text, int4)
CREATE OR REPLACE FUNCTION ST_MLineFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE
	WHEN geometrytype(GeomFromText($1, $2)) = ''MULTILINESTRING''
	THEN GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MLineFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1)) = ''MULTILINESTRING''
	THEN GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MLineFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromText($1)) = ''MULTILINESTRING''
	THEN ST_GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MultiLineStringFromText(text)
	RETURNS geometry
	AS 'SELECT ST_MLineFromText($1)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MultiLineStringFromText(text)
	RETURNS geometry
	AS 'SELECT ST_MLineFromText($1)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MultiLineStringFromText(text, int4)
	RETURNS geometry
	AS 'SELECT MLineFromText($1, $2)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MultiLineStringFromText(text, int4)
	RETURNS geometry
	AS 'SELECT MLineFromText($1, $2)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MPointFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1,$2)) = ''MULTIPOINT''
	THEN GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- PostGIS equivalent function: MPointFromText(text, int4)
CREATE OR REPLACE FUNCTION ST_MPointFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1, $2)) = ''MULTIPOINT''
	THEN GeomFromText($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MPointFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1)) = ''MULTIPOINT''
	THEN GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MPointFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromText($1)) = ''MULTIPOINT''
	THEN ST_GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MultiPointFromText(text, int4)
	RETURNS geometry
	AS 'SELECT MPointFromText($1, $2)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MultiPointFromText(text)
	RETURNS geometry
	AS 'SELECT MPointFromText($1)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MultiPointFromText(text)
	RETURNS geometry
	AS 'SELECT ST_MPointFromText($1)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MultiPointFromText(text)
	RETURNS geometry
	AS 'SELECT MPointFromText($1)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MultiPointFromText(text)
	RETURNS geometry
	AS 'SELECT MPointFromText($1)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MPolyFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1, $2)) = ''MULTIPOLYGON''
	THEN GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- PostGIS equivalent function: MPolyFromText(text, int4)
CREATE OR REPLACE FUNCTION ST_MPolyFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromText($1, $2)) = ''MULTIPOLYGON''
	THEN ST_GeomFromText($1,$2)
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

--Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MPolyFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromText($1)) = ''MULTIPOLYGON''
	THEN ST_GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MultiPolygonFromText(text, int4)
	RETURNS geometry
	AS 'SELECT MPolyFromText($1, $2)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MultiPolygonFromText(text, int4)
	RETURNS geometry
	AS 'SELECT MPolyFromText($1, $2)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MultiPolygonFromText(text)
	RETURNS geometry
	AS 'SELECT MPolyFromText($1)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MultiPolygonFromText(text)
	RETURNS geometry
	AS 'SELECT MPolyFromText($1)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION GeomCollFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE
	WHEN geometrytype(GeomFromText($1, $2)) = ''GEOMETRYCOLLECTION''
	THEN GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_GeomCollFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE
	WHEN geometrytype(ST_GeomFromText($1, $2)) = ''GEOMETRYCOLLECTION''
	THEN ST_GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION GeomCollFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE
	WHEN geometrytype(GeomFromText($1)) = ''GEOMETRYCOLLECTION''
	THEN GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_GeomCollFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE
	WHEN geometrytype(ST_GeomFromText($1)) = ''GEOMETRYCOLLECTION''
	THEN ST_GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION GeomFromWKB(bytea)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_from_WKB'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_GeomFromWKB(bytea)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_from_WKB'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION GeomFromWKB(bytea, int)
	RETURNS geometry
	AS 'SELECT setSRID(GeomFromWKB($1), $2)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- PostGIS equivalent function: GeomFromWKB(bytea, int)
CREATE OR REPLACE FUNCTION ST_GeomFromWKB(bytea, int)
	RETURNS geometry
	AS 'SELECT ST_SetSRID(ST_GeomFromWKB($1), $2)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION PointFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''POINT''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- PostGIS equivalent function: PointFromWKB(bytea, int)
CREATE OR REPLACE FUNCTION ST_PointFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1, $2)) = ''POINT''
	THEN ST_GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION PointFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''POINT''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_PointFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1)) = ''POINT''
	THEN ST_GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION LineFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''LINESTRING''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- PostGIS equivalent function: LineFromWKB(bytea, int)
CREATE OR REPLACE FUNCTION ST_LineFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1, $2)) = ''LINESTRING''
	THEN ST_GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION LineFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''LINESTRING''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_LineFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1)) = ''LINESTRING''
	THEN ST_GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION LinestringFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''LINESTRING''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_LinestringFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1, $2)) = ''LINESTRING''
	THEN ST_GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION LinestringFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''LINESTRING''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_LinestringFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''LINESTRING''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION PolyFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''POLYGON''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- PostGIS equivalent function: PolyFromWKB(text, int)
CREATE OR REPLACE FUNCTION ST_PolyFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1, $2)) = ''POLYGON''
	THEN ST_GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION PolyFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''POLYGON''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_PolyFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1)) = ''POLYGON''
	THEN ST_GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION PolygonFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1,$2)) = ''POLYGON''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_PolygonFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1,$2)) = ''POLYGON''
	THEN ST_GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION PolygonFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''POLYGON''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_PolygonFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''POLYGON''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MPointFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1,$2)) = ''MULTIPOINT''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- PostGIS equivalent function: MPointFromWKB(text, int)
CREATE OR REPLACE FUNCTION ST_MPointFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''MULTIPOINT''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MPointFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''MULTIPOINT''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MPointFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1)) = ''MULTIPOINT''
	THEN ST_GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MultiPointFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1,$2)) = ''MULTIPOINT''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MultiPointFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1,$2)) = ''MULTIPOINT''
	THEN ST_GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MultiPointFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''MULTIPOINT''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MultiPointFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1)) = ''MULTIPOINT''
	THEN ST_GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MultiLineFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''MULTILINESTRING''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION MultiLineFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''MULTILINESTRING''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MultiLineFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''MULTILINESTRING''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MultiLineFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1)) = ''MULTILINESTRING''
	THEN ST_GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MLineFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''MULTILINESTRING''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- PostGIS equivalent function: MLineFromWKB(text, int)
CREATE OR REPLACE FUNCTION ST_MLineFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1, $2)) = ''MULTILINESTRING''
	THEN ST_GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MLineFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''MULTILINESTRING''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MLineFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1)) = ''MULTILINESTRING''
	THEN ST_GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MPolyFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''MULTIPOLYGON''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- PostGIS equivalent function: MPolyFromWKB(bytea, int)
CREATE OR REPLACE FUNCTION ST_MPolyFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1, $2)) = ''MULTIPOLYGON''
	THEN ST_GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MPolyFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''MULTIPOLYGON''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MPolyFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1)) = ''MULTIPOLYGON''
	THEN ST_GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION MultiPolyFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''MULTIPOLYGON''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MultiPolyFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1, $2)) = ''MULTIPOLYGON''
	THEN ST_GeomFromWKB($1, $2)
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

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_MultiPolyFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1)) = ''MULTIPOLYGON''
	THEN ST_GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

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

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_GeomCollFromWKB(bytea, int)
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

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_GeomCollFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE
	WHEN geometrytype(ST_GeomFromWKB($1)) = ''GEOMETRYCOLLECTION''
	THEN ST_GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

--New functions

-- Maximum distance between linestrings.

CREATE OR REPLACE FUNCTION max_distance(geometry,geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_maxdistance2d_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT; 

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION _ST_MaxDistance(geometry,geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_maxdistance2d_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT; 
	
-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION ST_MaxDistance(geometry,geometry)
	RETURNS float8
	AS 'SELECT _ST_MaxDistance(ST_ConvexHull($1), ST_ConvexHull($2))'
	LANGUAGE 'SQL' IMMUTABLE STRICT; 

CREATE OR REPLACE FUNCTION ST_ClosestPoint(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_closestpoint'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ST_ShortestLine(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_shortestline2d'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION _ST_LongestLine(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_longestline2d'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ST_LongestLine(geometry,geometry)
	RETURNS geometry
	AS 'SELECT _ST_LongestLine(ST_ConvexHull($1), ST_ConvexHull($2))'
	LANGUAGE 'SQL' IMMUTABLE STRICT; 

CREATE OR REPLACE FUNCTION _ST_DFullyWithin(geometry,geometry,float8)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'LWGEOM_dfullywithin'
	LANGUAGE 'C' IMMUTABLE STRICT; 

CREATE OR REPLACE FUNCTION ST_DFullyWithin(geometry, geometry, float8)
	RETURNS boolean
	AS 'SELECT $1 && ST_Expand($2,$3) AND $2 && ST_Expand($1,$3) AND _ST_DFullyWithin(ST_ConvexHull($1), ST_ConvexHull($2), $3)'
	LANGUAGE 'SQL' IMMUTABLE; 
	
CREATE OR REPLACE FUNCTION ST_FlipCoordinates(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'ST_FlipCoordinates'
	LANGUAGE 'C' IMMUTABLE STRICT; 

--
-- SFSQL 1.1
--
-- BdPolyFromText(multiLineStringTaggedText String, SRID Integer): Polygon
--
--  Construct a Polygon given an arbitrary
--  collection of closed linestrings as a
--  MultiLineString text representation.
--
-- This is a PLPGSQL function rather then an SQL function
-- To avoid double call of BuildArea (one to get GeometryType
-- and another to actual return, in a CASE WHEN construct).
-- Also, we profit from plpgsql to RAISE exceptions.
--
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

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_BdPolyFromText(text, integer)
RETURNS geometry
AS $$
DECLARE
	geomtext alias for $1;
	srid alias for $2;
	mline geometry;
	geom geometry;
BEGIN
	mline := ST_MultiLineStringFromText(geomtext, srid);

	IF mline IS NULL
	THEN
		RAISE EXCEPTION 'Input is not a MultiLinestring';
	END IF;

	geom := ST_BuildArea(mline);

	IF GeometryType(geom) != 'POLYGON'
	THEN
		RAISE EXCEPTION 'Input returns more then a single polygon, try using BdMPolyFromText instead';
	END IF;

	RETURN geom;
END;
$$
LANGUAGE 'plpgsql' IMMUTABLE STRICT;

--
-- SFSQL 1.1
--
-- BdMPolyFromText(multiLineStringTaggedText String, SRID Integer): MultiPolygon
--
--  Construct a MultiPolygon given an arbitrary
--  collection of closed linestrings as a
--  MultiLineString text representation.
--
-- This is a PLPGSQL function rather then an SQL function
-- To raise an exception in case of invalid input.
--
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

	geom := multi(BuildArea(mline));

	RETURN geom;
END;
$$
LANGUAGE 'plpgsql' IMMUTABLE STRICT;

-- Availability: 1.2.2
CREATE OR REPLACE FUNCTION ST_BdMPolyFromText(text, integer)
RETURNS geometry
AS $$
DECLARE
	geomtext alias for $1;
	srid alias for $2;
	mline geometry;
	geom geometry;
BEGIN
	mline := ST_MultiLineStringFromText(geomtext, srid);

	IF mline IS NULL
	THEN
		RAISE EXCEPTION 'Input is not a MultiLinestring';
	END IF;

	geom := multi(ST_BuildArea(mline));

	RETURN geom;
END;
$$
LANGUAGE 'plpgsql' IMMUTABLE STRICT;

#include "long_xact.sql.in.c"
#include "sqlmm.sql.in.c"
#include "geography.sql.in.c"


---------------------------------------------------------------
-- 3D-functions
---------------------------------------------------------------

CREATE OR REPLACE FUNCTION ST_3DDistance(geometry,geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_mindistance3d'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;
	
CREATE OR REPLACE FUNCTION ST_3DMaxDistance(geometry,geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_maxdistance3d'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;	

CREATE OR REPLACE FUNCTION ST_3DClosestPoint(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_closestpoint3d'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION ST_3DShortestLine(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_shortestline3d'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION ST_3DLongestLine(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_longestline3d'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;
	
CREATE OR REPLACE FUNCTION _ST_3DDWithin(geometry,geometry,float8)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'LWGEOM_dwithin3d'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;
	
CREATE OR REPLACE FUNCTION ST_3DDWithin(geometry,geometry,float8)
	RETURNS boolean
	AS 'SELECT $1 && ST_Expand($2,$3) AND $2 && ST_Expand($1,$3) AND _ST_3DDWithin($1, $2, $3)'
	LANGUAGE 'SQL' IMMUTABLE
	COST 100;
	
CREATE OR REPLACE FUNCTION _ST_3DDFullyWithin(geometry,geometry,float8)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'LWGEOM_dfullywithin3d'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;
	
CREATE OR REPLACE FUNCTION ST_3DDFullyWithin(geometry,geometry,float8)
	RETURNS boolean
	AS 'SELECT $1 && ST_Expand($2,$3) AND $2 && ST_Expand($1,$3) AND _ST_3DDFullyWithin($1, $2, $3)'
	LANGUAGE 'SQL' IMMUTABLE
	COST 100;
	
CREATE OR REPLACE FUNCTION ST_3DIntersects(geometry,geometry)
	RETURNS boolean
	AS 'SELECT $1 && $2 AND _ST_3DDWithin($1, $2, 0.0)'
	LANGUAGE 'SQL' IMMUTABLE
	COST 100;
	
	
---------------------------------------------------------------
-- SQL-MM
---------------------------------------------------------------

--
-- SQL-MM
--
-- ST_CurveToLine(Geometry geometry, SegmentsPerQuarter integer)
--
-- Converts a given geometry to a linear geometry.  Each curveed
-- geometry or segment is converted into a linear approximation using
-- the given number of segments per quarter circle.
CREATE OR REPLACE FUNCTION ST_CurveToLine(geometry, integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_curve_segmentize'
	LANGUAGE 'C' IMMUTABLE STRICT;
--
-- SQL-MM
--
-- ST_CurveToLine(Geometry geometry, SegmentsPerQuarter integer)
--
-- Converts a given geometry to a linear geometry.  Each curveed
-- geometry or segment is converted into a linear approximation using
-- the default value of 32 segments per quarter circle
CREATE OR REPLACE FUNCTION ST_CurveToLine(geometry)
	RETURNS geometry AS 'SELECT ST_CurveToLine($1, 32)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ST_HasArc(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'LWGEOM_has_arc'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ST_LineToCurve(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_line_desegmentize'
	LANGUAGE 'C' IMMUTABLE STRICT;
---------------------------------------------------------------
-- END
---------------------------------------------------------------


---------------------------------------------------------------
-- USER CONTRIBUTED
---------------------------------------------------------------

-----------------------------------------------------------------------
-- ST_MinimumBoundingCircle(inputgeom geometry, segs_per_quarter integer)
-----------------------------------------------------------------------
-- Returns the smallest circle polygon that can fully contain a geometry
-- Defaults to 48 segs per quarter to approximate a circle
-- Contributed by Bruce Rindahl
-- Availability: 1.4.0
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION ST_MinimumBoundingCircle(inputgeom geometry, segs_per_quarter integer)
	RETURNS geometry AS
$BODY$
	DECLARE
	hull GEOMETRY;
	ring GEOMETRY;
	center GEOMETRY;
	radius DOUBLE PRECISION;
	dist DOUBLE PRECISION;
	d DOUBLE PRECISION;
	idx1 integer;
	idx2 integer;
	l1 GEOMETRY;
	l2 GEOMETRY;
	p1 GEOMETRY;
	p2 GEOMETRY;
	a1 DOUBLE PRECISION;
	a2 DOUBLE PRECISION;


	BEGIN

	-- First compute the ConvexHull of the geometry
	hull = ST_ConvexHull(inputgeom);
	--A point really has no MBC
	IF ST_GeometryType(hull) = 'ST_Point' THEN
		RETURN hull;
	END IF;
	-- convert the hull perimeter to a linestring so we can manipulate individual points
	--If its already a linestring force it to a closed linestring
	ring = CASE WHEN ST_GeometryType(hull) = 'ST_LineString' THEN ST_AddPoint(hull, ST_StartPoint(hull)) ELSE ST_ExteriorRing(hull) END;

	dist = 0;
	-- Brute Force - check every pair
	FOR i in 1 .. (ST_NumPoints(ring)-2)
		LOOP
			FOR j in i .. (ST_NumPoints(ring)-1)
				LOOP
				d = ST_Distance(ST_PointN(ring,i),ST_PointN(ring,j));
				-- Check the distance and update if larger
				IF (d > dist) THEN
					dist = d;
					idx1 = i;
					idx2 = j;
				END IF;
			END LOOP;
		END LOOP;

	-- We now have the diameter of the convex hull.  The following line returns it if desired.
	-- RETURN MakeLine(PointN(ring,idx1),PointN(ring,idx2));

	-- Now for the Minimum Bounding Circle.  Since we know the two points furthest from each
	-- other, the MBC must go through those two points. Start with those points as a diameter of a circle.

	-- The radius is half the distance between them and the center is midway between them
	radius = ST_Distance(ST_PointN(ring,idx1),ST_PointN(ring,idx2)) / 2.0;
	center = ST_Line_interpolate_point(ST_MakeLine(ST_PointN(ring,idx1),ST_PointN(ring,idx2)),0.5);

	-- Loop through each vertex and check if the distance from the center to the point
	-- is greater than the current radius.
	FOR k in 1 .. (ST_NumPoints(ring)-1)
		LOOP
		IF(k <> idx1 and k <> idx2) THEN
			dist = ST_Distance(center,ST_PointN(ring,k));
			IF (dist > radius) THEN
				-- We have to expand the circle.  The new circle must pass trhough
				-- three points - the two original diameters and this point.

				-- Draw a line from the first diameter to this point
				l1 = ST_Makeline(ST_PointN(ring,idx1),ST_PointN(ring,k));
				-- Compute the midpoint
				p1 = ST_line_interpolate_point(l1,0.5);
				-- Rotate the line 90 degrees around the midpoint (perpendicular bisector)
				l1 = ST_Translate(ST_Rotate(ST_Translate(l1,-X(p1),-Y(p1)),pi()/2),X(p1),Y(p1));
				--  Compute the azimuth of the bisector
				a1 = ST_Azimuth(ST_PointN(l1,1),ST_PointN(l1,2));
				--  Extend the line in each direction the new computed distance to insure they will intersect
				l1 = ST_AddPoint(l1,ST_Makepoint(X(ST_PointN(l1,2))+sin(a1)*dist,Y(ST_PointN(l1,2))+cos(a1)*dist),-1);
				l1 = ST_AddPoint(l1,ST_Makepoint(X(ST_PointN(l1,1))-sin(a1)*dist,Y(ST_PointN(l1,1))-cos(a1)*dist),0);

				-- Repeat for the line from the point to the other diameter point
				l2 = ST_Makeline(ST_PointN(ring,idx2),ST_PointN(ring,k));
				p2 = ST_Line_interpolate_point(l2,0.5);
				l2 = ST_Translate(ST_Rotate(ST_Translate(l2,-X(p2),-Y(p2)),pi()/2),X(p2),Y(p2));
				a2 = ST_Azimuth(ST_PointN(l2,1),ST_PointN(l2,2));
				l2 = ST_AddPoint(l2,ST_Makepoint(X(ST_PointN(l2,2))+sin(a2)*dist,Y(ST_PointN(l2,2))+cos(a2)*dist),-1);
				l2 = ST_AddPoint(l2,ST_Makepoint(X(ST_PointN(l2,1))-sin(a2)*dist,Y(ST_PointN(l2,1))-cos(a2)*dist),0);

				-- The new center is the intersection of the two bisectors
				center = ST_Intersection(l1,l2);
				-- The new radius is the distance to any of the three points
				radius = ST_Distance(center,ST_PointN(ring,idx1));
			END IF;
		END IF;
		END LOOP;
	--DONE!!  Return the MBC via the buffer command
	RETURN ST_Buffer(center,radius,segs_per_quarter);

	END;
$BODY$
	LANGUAGE 'plpgsql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ST_MinimumBoundingCircle(geometry)
 RETURNS geometry AS
'SELECT ST_MinimumBoundingCircle($1, 48)'
 LANGUAGE 'sql' IMMUTABLE STRICT;
 
-- ST_ConcaveHull and Helper functions starts here --
-----------------------------------------------------------------------
-- Contributed by Regina Obe and Leo Hsu
-- Availability: 2.0.0
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION _ST_ConcaveHull(param_inputgeom geometry)
  RETURNS geometry AS
$$
	DECLARE     
	vexhull GEOMETRY;
	var_resultgeom geometry;
	vexring GEOMETRY;
	cavering GEOMETRY;
	cavept geometry[];
	seglength double precision;
	var_tempgeom geometry;
	i integer;
	
	BEGIN

		-- First compute the ConvexHull of the geometry
		vexhull := ST_ConvexHull(param_inputgeom);
		--A point really has no concave hull
		IF ST_GeometryType(vexhull) = 'ST_Point' OR ST_GeometryType(vexHull) = 'ST_LineString' THEN
			RETURN vexhull;
		END IF;

		-- convert the hull perimeter to a linestring so we can manipulate individual points
		vexring := CASE WHEN ST_GeometryType(vexhull) = 'ST_LineString' THEN vexhull ELSE ST_ExteriorRing(vexhull) END;
		seglength := ST_Length(vexring)/least(ST_NPoints(vexring)*2,1000) ;

		vexring := ST_Segmentize(vexring, seglength);
		-- find the point on the original geom that is closest to each point of the convex hull and make a new linestring out of it.
		cavering := ST_Collect(
			ARRAY(

				SELECT 
					ST_ClosestPoint(param_inputgeom, pt ) As the_geom
					FROM (
						SELECT  ST_PointN(vexring, n ) As pt, n
							FROM 
							generate_series(1, ST_NPoints(vexring) ) As n
						) As pt
				
				)
			)
		; 

		var_resultgeom := ST_MakePolygon(ST_MakeLine(geom)) 
			FROM ST_Dump(cavering) As foo;
		
		IF NOT ST_IsValid(var_resultgeom) THEN
		    --RAISE NOTICE '_ST_Concavehull invalid %', ST_AsText(var_resultgeom);
		    var_tempgeom := ST_BuildArea(var_resultgeom); -- try to make valid
		    IF NOT ST_IsValid(var_tempgeom) THEN
				var_resultgeom := ST_Buffer(var_resultgeom,ST_Length(cavering)/1000, 'quad_segs=3'); -- try to make valid
		    END IF;
		     --if still invalid or doens't  contain the geometry just return convex hull
		    IF NOT ST_IsValid(var_resultgeom) or ST_GeometryType(var_resultgeom) <> 'ST_Polygon' THEN
				var_resultgeom := ST_ConvexHull(param_inputgeom);
		    ELSIF ST_GeometryType(param_inputgeom) ILIKE '%Geometry%' THEN
				IF EXISTS(SELECT geom FROM ST_Dump(param_inputgeom) WHERE NOT ST_Covers(var_resultgeom,geom) ) THEN 
				--we have to explode inputgeom since geos doesn't support geometrycollections for containment check
				   var_resultgeom := ST_ConvexHull(param_inputgeom);
				END IF;
			ELSIF NOT ST_Contains(var_resultgeom, param_inputgeom) THEN
				var_resultgeom := ST_ConvexHull(param_inputgeom);
			END IF;
		END IF;
		RETURN var_resultgeom;
	
	END;
$$
  LANGUAGE 'plpgsql' IMMUTABLE STRICT
  COST 100;
  
CREATE OR REPLACE FUNCTION ST_ConcaveHull(param_geom geometry, param_pctconvex float, param_allow_holes boolean) RETURNS geometry AS
$$
	DECLARE
		var_convhull geometry := ST_ConvexHull(param_geom);
		var_param_geom geometry := param_geom;
		var_initarea float := ST_Area(var_convhull);
		var_newarea float := var_initarea;
		var_div integer := 6; /** this is the 1/var_div is the percent increase we will allow per triangle to keep speed decent **/
		var_tempgeom geometry;
		var_tempgeom2 geometry;
		var_cent geometry;
		var_geoms geometry[4]; /** We will cut the current geometry into 4 triangular quadrants along the centroid/extent **/
		var_enline geometry;
		var_resultgeom geometry;
		var_atempgeoms geometry[];
		var_buf float := 1; /**tolerance so that geometries that are right on the extent don't get accidentally clipped off **/
	BEGIN
		-- We start with convex hull as our base
		var_resultgeom := var_convhull;
		
		IF param_pctconvex = 1 THEN
			return var_resultgeom;
		ELSIF ST_GeometryType(var_param_geom) = 'ST_Polygon' THEN -- it is as concave as it is going to get
			IF param_allow_holes THEN -- leave the holes
				RETURN var_param_geom;
			ELSE -- remove the holes
				var_resultgeom := ST_MakePolygon(ST_ExteriorRing(var_param_geom));
				RETURN var_resultgeom;
			END IF;
		END IF;
		IF ST_Dimension(var_resultgeom) > 1 AND param_pctconvex BETWEEN 0 and 0.98 THEN
		-- get linestring that forms envelope of geometry
			var_enline := ST_Boundary(ST_Envelope(var_param_geom));
			var_buf := ST_Length(var_enline)/1000.0;
			IF ST_GeometryType(var_param_geom) = 'ST_MultiPoint' AND ST_NumGeometries(var_param_geom) BETWEEN 4 and 200 THEN
			-- we make polygons out of points since they are easier to cave in. 
			-- Note we limit to between 4 and 200 points because this process is slow and gets quadratically slow
				var_buf := sqrt(ST_Area(var_convhull)*0.8/(ST_NumGeometries(var_param_geom)*ST_NumGeometries(var_param_geom)));
				var_atempgeoms := ARRAY(SELECT geom FROM ST_DumpPoints(var_param_geom));
				-- 5 and 10 and just fudge factors
				var_tempgeom := ST_Union(ARRAY(SELECT geom
						FROM (
						-- fuse near neighbors together
						SELECT DISTINCT ON (i) i,  ST_Distance(var_atempgeoms[i],var_atempgeoms[j]), ST_Buffer(ST_MakeLine(var_atempgeoms[i], var_atempgeoms[j]) , var_buf*5, 'quad_segs=3') As geom
								FROM generate_series(1,array_upper(var_atempgeoms, 1)) As i
									INNER JOIN generate_series(1,array_upper(var_atempgeoms, 1)) As j 
										ON (
								 NOT ST_Intersects(var_atempgeoms[i],var_atempgeoms[j])
									AND ST_DWithin(var_atempgeoms[i],var_atempgeoms[j], var_buf*10)
									)
								UNION ALL
						-- catch the ones with no near neighbors
								SELECT i, 0, ST_Buffer(var_atempgeoms[i] , var_buf*10, 'quad_segs=3') As geom
								FROM generate_series(1,array_upper(var_atempgeoms, 1)) As i
									LEFT JOIN generate_series(ceiling(array_upper(var_atempgeoms,1)/2)::integer,array_upper(var_atempgeoms, 1)) As j 
										ON (
								 NOT ST_Intersects(var_atempgeoms[i],var_atempgeoms[j])
									AND ST_DWithin(var_atempgeoms[i],var_atempgeoms[j], var_buf*10) 
									)
									WHERE j IS NULL
								ORDER BY 1, 2
							) As foo	) );
				IF ST_IsValid(var_tempgeom) AND ST_GeometryType(var_tempgeom) = 'ST_Polygon' THEN
					var_tempgeom := ST_Intersection(var_tempgeom, var_convhull);
					IF param_allow_holes THEN
						var_param_geom := var_tempgeom;
					ELSE
						var_param_geom := ST_MakePolygon(ST_ExteriorRing(var_tempgeom));
					END IF;
					return var_param_geom;
				ELSIF ST_IsValid(var_tempgeom) THEN
					var_param_geom := ST_Intersection(var_tempgeom, var_convhull);	
				END IF;
			END IF;

			IF ST_GeometryType(var_param_geom) = 'ST_Polygon' THEN
				IF NOT param_allow_holes THEN
					var_param_geom := ST_MakePolygon(ST_ExteriorRing(var_param_geom));
				END IF;
				return var_param_geom;
			END IF;
            var_cent := ST_Centroid(var_param_geom);
            IF (ST_XMax(var_enline) - ST_XMin(var_enline) ) > var_buf AND (ST_YMax(var_enline) - ST_YMin(var_enline) ) > var_buf THEN
                    IF ST_Dwithin(ST_Centroid(var_convhull) , ST_Centroid(ST_Envelope(var_param_geom)), var_buf/2) THEN
                -- If the geometric dimension is > 1 and the object is symettric (cutting at centroid will not work -- offset a bit)
                        var_cent := ST_Translate(var_cent, (ST_XMax(var_enline) - ST_XMin(var_enline))/1000,  (ST_YMAX(var_enline) - ST_YMin(var_enline))/1000);
                    ELSE
                        -- uses closest point on geometry to centroid. I can't explain why we are doing this
                        var_cent := ST_ClosestPoint(var_param_geom,var_cent);
                    END IF;
                    IF ST_DWithin(var_cent, var_enline,var_buf) THEN
                        var_cent := ST_centroid(ST_Envelope(var_param_geom));
                    END IF;
                    -- break envelope into 4 triangles about the centroid of the geometry and returned the clipped geometry in each quadrant
                    FOR i in 1 .. 4 LOOP
                       var_geoms[i] := ST_MakePolygon(ST_MakeLine(ARRAY[ST_PointN(var_enline,i), ST_PointN(var_enline,i+1), var_cent, ST_PointN(var_enline,i)]));
                       var_geoms[i] := ST_Intersection(var_param_geom, ST_Buffer(var_geoms[i],var_buf));
                       IF ST_IsValid(var_geoms[i]) THEN 
                            
                       ELSE
                            var_geoms[i] := ST_BuildArea(ST_MakeLine(ARRAY[ST_PointN(var_enline,i), ST_PointN(var_enline,i+1), var_cent, ST_PointN(var_enline,i)]));
                       END IF; 
                    END LOOP;
                    var_tempgeom := ST_Union(ARRAY[ST_ConvexHull(var_geoms[1]), ST_ConvexHull(var_geoms[2]) , ST_ConvexHull(var_geoms[3]), ST_ConvexHull(var_geoms[4])]); 
                    --RAISE NOTICE 'Curr vex % ', ST_AsText(var_tempgeom);
                    IF ST_Area(var_tempgeom) <= var_newarea AND ST_IsValid(var_tempgeom)  THEN --AND ST_GeometryType(var_tempgeom) ILIKE '%Polygon'
                        
                        var_tempgeom := ST_Buffer(ST_ConcaveHull(var_geoms[1],least(param_pctconvex + param_pctconvex/var_div),true),var_buf, 'quad_segs=2');
                        FOR i IN 1 .. 4 LOOP
                            var_geoms[i] := ST_Buffer(ST_ConcaveHull(var_geoms[i],least(param_pctconvex + param_pctconvex/var_div),true), var_buf, 'quad_segs=2');
                            IF ST_IsValid(var_geoms[i]) Then
                                var_tempgeom := ST_Union(var_tempgeom, var_geoms[i]);
                            ELSE
                                RAISE NOTICE 'Not valid % %', i, ST_AsText(var_tempgeom);
                                var_tempgeom := ST_Union(var_tempgeom, ST_ConvexHull(var_geoms[i]));
                            END IF; 
                        END LOOP;

                        --RAISE NOTICE 'Curr concave % ', ST_AsText(var_tempgeom);
                        IF ST_IsValid(var_tempgeom) THEN
                            var_resultgeom := var_tempgeom;
                        END IF;
                        var_newarea := ST_Area(var_resultgeom);
                    ELSIF ST_IsValid(var_tempgeom) THEN
                        var_resultgeom := var_tempgeom;
                    END IF;

                    IF ST_NumGeometries(var_resultgeom) > 1  THEN
                        var_tempgeom := _ST_ConcaveHull(var_resultgeom);
                        IF ST_IsValid(var_tempgeom) AND ST_GeometryType(var_tempgeom) ILIKE 'ST_Polygon' THEN
                            var_resultgeom := var_tempgeom;
                        ELSE
                            var_resultgeom := ST_Buffer(var_tempgeom,var_buf, 'quad_segs=2');
                        END IF;
                    END IF;
                    IF param_allow_holes = false THEN 
                    -- only keep exterior ring since we do not want holes
                        var_resultgeom := ST_MakePolygon(ST_ExteriorRing(var_resultgeom));
                    END IF;
                ELSE
                    var_resultgeom := ST_Buffer(var_resultgeom,var_buf);
                END IF;
                var_resultgeom := ST_Intersection(var_resultgeom, ST_ConvexHull(var_param_geom));
            ELSE
                -- dimensions are too small to cut
                var_resultgeom := _ST_ConcaveHull(var_param_geom);
            END IF;
            RETURN var_resultgeom;
	END;
$$
LANGUAGE 'plpgsql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ST_ConcaveHull(param_geom geometry, param_pctconvex float)
	RETURNS geometry AS
$$
	SELECT ST_ConcaveHull($1, $2, false);
$$
LANGUAGE 'sql' IMMUTABLE STRICT;
-- ST_ConcaveHull and Helper functions end here --
COMMIT;

#include "postgis_drop.sql.in.c"
