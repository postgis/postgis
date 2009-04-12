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
--          Changes cleanly handled by lwpostgis_uptrade.sql are fine,
--	    other changes will require a bump in Major version.
--	    Currently only function replaceble by CREATE OR REPLACE
--	    are cleanly handled.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include "sqldefines.h"

BEGIN;

-------------------------------------------------------------------
--  SPHEROID TYPE
-------------------------------------------------------------------

-- Deprecation in 1.2.3
CREATEFUNCTION spheroid_in(cstring)
	RETURNS spheroid 
	AS 'MODULE_PATHNAME','ellipsoid_in'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_spheroid_in(cstring)
	RETURNS spheroid 
	AS 'MODULE_PATHNAME','ellipsoid_in'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION spheroid_out(spheroid)
	RETURNS cstring 
	AS 'MODULE_PATHNAME','ellipsoid_out'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_spheroid_out(spheroid)
	RETURNS cstring 
	AS 'MODULE_PATHNAME','ellipsoid_out'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

CREATE TYPE spheroid (
	alignment = double,
	internallength = 65,
	input = ST_spheroid_in,
	output = ST_spheroid_out
);

-------------------------------------------------------------------
--  GEOMETRY TYPE (lwgeom)
-------------------------------------------------------------------

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_in(cstring)
        RETURNS geometry
        AS 'MODULE_PATHNAME','LWGEOM_in'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_in(cstring)
        RETURNS geometry
        AS 'MODULE_PATHNAME','LWGEOM_in'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_out(geometry)
        RETURNS cstring
        AS 'MODULE_PATHNAME','LWGEOM_out'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_out(geometry)
        RETURNS cstring
        AS 'MODULE_PATHNAME','LWGEOM_out'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_analyze(internal)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_analyze'
	LANGUAGE 'C' _VOLATILE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_analyze(internal)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_analyze'
	LANGUAGE 'C' _VOLATILE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_recv(internal)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_recv'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_recv(internal)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_recv'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_send(geometry)
	RETURNS bytea
	AS 'MODULE_PATHNAME','LWGEOM_send'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_send(geometry)
	RETURNS bytea
	AS 'MODULE_PATHNAME','LWGEOM_send'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

CREATE TYPE geometry (
        internallength = variable,
        input = ST_geometry_in,
        output = ST_geometry_out,
	send = ST_geometry_send,
	receive = ST_geometry_recv,
	delimiter = ':',
	analyze = ST_geometry_analyze,
        storage = main
);

-------------------------------------------
-- Affine transforms
-------------------------------------------

-- Availability: 1.1.2
-- Deprecation in 1.2.3
CREATEFUNCTION Affine(geometry,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_affine'
	LANGUAGE 'C' _IMMUTABLE_STRICT; 

-- Availability: 1.2.2
CREATEFUNCTION ST_Affine(geometry,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_affine'
	LANGUAGE 'C' _IMMUTABLE_STRICT; 

-- Availability: 1.1.2
-- Deprecation in 1.2.3
CREATEFUNCTION Affine(geometry,float8,float8,float8,float8,float8,float8)
	RETURNS geometry
	AS 'SELECT affine($1,  $2, $3, 0,  $4, $5, 0,  0, 0, 1,  $6, $7, 0)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; 

-- Availability: 1.2.2
CREATEFUNCTION ST_Affine(geometry,float8,float8,float8,float8,float8,float8)
	RETURNS geometry
	AS 'SELECT affine($1,  $2, $3, 0,  $4, $5, 0,  0, 0, 1,  $6, $7, 0)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; 

-- Availability: 1.1.2
-- Deprecation in 1.2.3
CREATEFUNCTION RotateZ(geometry,float8)
	RETURNS geometry
	AS 'SELECT affine($1,  cos($2), -sin($2), 0,  sin($2), cos($2), 0,  0, 0, 1,  0, 0, 0)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; 

-- Availability: 1.2.2
CREATEFUNCTION ST_RotateZ(geometry,float8)
	RETURNS geometry
	AS 'SELECT affine($1,  cos($2), -sin($2), 0,  sin($2), cos($2), 0,  0, 0, 1,  0, 0, 0)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; 

-- Availability: 1.1.2
-- Deprecation in 1.2.3
CREATEFUNCTION Rotate(geometry,float8)
	RETURNS geometry
	AS 'SELECT rotateZ($1, $2)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; 

-- Availability: 1.2.2
CREATEFUNCTION ST_Rotate(geometry,float8)
	RETURNS geometry
	AS 'SELECT rotateZ($1, $2)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; 

-- Availability: 1.1.2
-- Deprecation in 1.2.3
CREATEFUNCTION RotateX(geometry,float8)
	RETURNS geometry
 	AS 'SELECT affine($1, 1, 0, 0, 0, cos($2), -sin($2), 0, sin($2), cos($2), 0, 0, 0)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; 

-- Availability: 1.2.2
CREATEFUNCTION ST_RotateX(geometry,float8)
	RETURNS geometry
 	AS 'SELECT affine($1, 1, 0, 0, 0, cos($2), -sin($2), 0, sin($2), cos($2), 0, 0, 0)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; 

-- Availability: 1.1.2
-- Deprecation in 1.2.3
CREATEFUNCTION RotateY(geometry,float8)
	RETURNS geometry
 	AS 'SELECT affine($1,  cos($2), 0, sin($2),  0, 1, 0,  -sin($2), 0, cos($2), 0,  0, 0)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; 

-- Availability: 1.2.2
CREATEFUNCTION ST_RotateY(geometry,float8)
	RETURNS geometry
 	AS 'SELECT affine($1,  cos($2), 0, sin($2),  0, 1, 0,  -sin($2), 0, cos($2), 0,  0, 0)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; 

-- Deprecation in 1.2.3
CREATEFUNCTION Translate(geometry,float8,float8,float8)
	RETURNS geometry
 	AS 'SELECT affine($1, 1, 0, 0, 0, 1, 0, 0, 0, 1, $2, $3, $4)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; 

-- Availability: 1.2.2
CREATEFUNCTION ST_Translate(geometry,float8,float8,float8)
	RETURNS geometry
 	AS 'SELECT affine($1, 1, 0, 0, 0, 1, 0, 0, 0, 1, $2, $3, $4)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; 

-- Deprecation in 1.2.3
CREATEFUNCTION Translate(geometry,float8,float8)
	RETURNS geometry
	AS 'SELECT translate($1, $2, $3, 0)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT;

-- Availability: 1.2.2
CREATEFUNCTION ST_Translate(geometry,float8,float8)
	RETURNS geometry
	AS 'SELECT translate($1, $2, $3, 0)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT;

-- Availability: 1.1.0
-- Deprecation in 1.2.3
CREATEFUNCTION Scale(geometry,float8,float8,float8)
	RETURNS geometry
	AS 'SELECT affine($1,  $2, 0, 0,  0, $3, 0,  0, 0, $4,  0, 0, 0)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; 

-- Availability: 1.2.2
CREATEFUNCTION ST_Scale(geometry,float8,float8,float8)
	RETURNS geometry
	AS 'SELECT affine($1,  $2, 0, 0,  0, $3, 0,  0, 0, $4,  0, 0, 0)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; 

-- Availability: 1.1.0
-- Deprecation in 1.2.3
CREATEFUNCTION Scale(geometry,float8,float8)
	RETURNS geometry
	AS 'SELECT scale($1, $2, $3, 1)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; 

-- Availability: 1.2.2
CREATEFUNCTION ST_Scale(geometry,float8,float8)
	RETURNS geometry
	AS 'SELECT scale($1, $2, $3, 1)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; 

-- Availability: 1.1.0 
-- Deprecation in 1.2.3
CREATEFUNCTION transscale(geometry,float8,float8,float8,float8)
        RETURNS geometry
        AS 'SELECT affine($1,  $4, 0, 0,  0, $5, 0, 
		0, 0, 1,  $2 * $4, $3 * $5, 0)'
        LANGUAGE 'SQL' _IMMUTABLE_STRICT;

-- Availability: 1.2.2 
CREATEFUNCTION ST_transscale(geometry,float8,float8,float8,float8)
        RETURNS geometry
        AS 'SELECT affine($1,  $4, 0, 0,  0, $5, 0, 
		0, 0, 1,  $2 * $4, $3 * $5, 0)'
        LANGUAGE 'SQL' _IMMUTABLE_STRICT;

-- Availability: 1.1.0
-- Deprecation in 1.2.3
CREATEFUNCTION shift_longitude(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_longitude_shift'
	LANGUAGE 'C' _IMMUTABLE_STRICT; 

-- Availability: 1.2.2
CREATEFUNCTION ST_shift_longitude(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_longitude_shift'
	LANGUAGE 'C' _IMMUTABLE_STRICT; 

-------------------------------------------------------------------
--  BOX3D TYPE
-------------------------------------------------------------------

-- Deprecation in 1.2.3
CREATEFUNCTION box3d_in(cstring)
	RETURNS box3d 
	AS 'MODULE_PATHNAME', 'BOX3D_in'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION box3d_out(box3d)
	RETURNS cstring 
	AS 'MODULE_PATHNAME', 'BOX3D_out'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_box3d_in(cstring)
	RETURNS box3d 
	AS 'MODULE_PATHNAME', 'BOX3D_in'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_box3d_out(box3d)
	RETURNS cstring 
	AS 'MODULE_PATHNAME', 'BOX3D_out'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

CREATE TYPE box3d (
	alignment = double,
	internallength = 48,
	input = ST_box3d_in,
	output = ST_box3d_out
);

-- Temporary box3d aggregate type to retain full double precision
-- for ST_Extent(). Should be removed when we change the output 
-- type of ST_Extent() to return something other than BOX2DFLOAT4.
CREATEFUNCTION box3d_extent_in(cstring)
        RETURNS box3d_extent
        AS '$libdir/postgis-1.4', 'BOX3D_in'
        LANGUAGE 'C' IMMUTABLE STRICT; -- WITH (isstrict);

CREATEFUNCTION box3d_extent_out(box3d_extent)
	RETURNS cstring 
	AS 'MODULE_PATHNAME', 'BOX3D_extent_out'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

CREATE TYPE box3d_extent (
	alignment = double,
	internallength = 48,
	input = box3d_extent_in,
	output = box3d_extent_out
);

-- Availability: 1.4.0
CREATEFUNCTION ST_box3d_extent(box3d_extent)
        RETURNS box3d
        AS 'MODULE_PATHNAME', 'BOX3D_extent_to_BOX3D'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

CREATE OR REPLACE FUNCTION ST_box2d(box3d_extent)
        RETURNS box2d
        AS 'MODULE_PATHNAME', 'BOX3D_to_BOX2DFLOAT4'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

CREATEFUNCTION ST_geometry(box3d_extent)
        RETURNS geometry
        AS 'MODULE_PATHNAME','BOX3D_to_LWGEOM'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);
-- End of temporary hack

-- Deprecation in 1.2.3
CREATEFUNCTION xmin(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_xmin'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_XMin(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_xmin'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION ymin(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_ymin'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_YMin(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_ymin'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION zmin(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_zmin'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_ZMin(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_zmin'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION xmax(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_xmax'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_XMax(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_xmax'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION ymax(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_ymax'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_YMax(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_ymax'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION zmax(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_zmax'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_ZMax(box3d)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','BOX3D_zmax'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-------------------------------------------------------------------
--  CHIP TYPE
-------------------------------------------------------------------

-- Deprecation in 1.2.3
CREATEFUNCTION chip_in(cstring)
	RETURNS chip 
	AS 'MODULE_PATHNAME','CHIP_in'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);
        
-- Availability: 1.2.2
CREATEFUNCTION ST_chip_in(cstring)
	RETURNS chip 
	AS 'MODULE_PATHNAME','CHIP_in'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION chip_out(chip)
	RETURNS cstring 
	AS 'MODULE_PATHNAME','CHIP_out'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_chip_out(chip)
	RETURNS cstring 
	AS 'MODULE_PATHNAME','CHIP_out'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

CREATE TYPE chip (
	alignment = double,
	internallength = variable,
	input = ST_chip_in,
	output = ST_chip_out,
	storage = extended
);

-----------------------------------------------------------------------
-- BOX2D
-----------------------------------------------------------------------

-- Deprecation in 1.2.3
CREATEFUNCTION box2d_in(cstring)
        RETURNS box2d
        AS 'MODULE_PATHNAME','BOX2DFLOAT4_in'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_box2d_in(cstring)
        RETURNS box2d
        AS 'MODULE_PATHNAME','BOX2DFLOAT4_in'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION box2d_out(box2d)
        RETURNS cstring
        AS 'MODULE_PATHNAME','BOX2DFLOAT4_out'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_box2d_out(box2d)
        RETURNS cstring
        AS 'MODULE_PATHNAME','BOX2DFLOAT4_out'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

CREATE TYPE box2d (
        internallength = 16,
        input = ST_box2d_in,
        output = ST_box2d_out,
        storage = plain
);

---- BOX2D  support functions

-- Deprecation in 1.2.3
CREATEFUNCTION box2d_overleft(box2d, box2d) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'BOX2D_overleft'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_box2d_overleft(box2d, box2d) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'BOX2D_overleft'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION box2d_overright(box2d, box2d) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'BOX2D_overright' 
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_box2d_overright(box2d, box2d) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'BOX2D_overright' 
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION box2d_left(box2d, box2d) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'BOX2D_left' 
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_box2d_left(box2d, box2d) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'BOX2D_left' 
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION box2d_right(box2d, box2d) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'BOX2D_right' 
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_box2d_right(box2d, box2d) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'BOX2D_right' 
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION box2d_contain(box2d, box2d) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'BOX2D_contain'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_box2d_contain(box2d, box2d) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'BOX2D_contain'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION box2d_contained(box2d, box2d) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'BOX2D_contained'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_box2d_contained(box2d, box2d) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'BOX2D_contained'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION box2d_overlap(box2d, box2d) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'BOX2D_overlap'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_box2d_overlap(box2d, box2d) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'BOX2D_overlap'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION box2d_same(box2d, box2d) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'BOX2D_same'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_box2d_same(box2d, box2d) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'BOX2D_same'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION box2d_intersects(box2d, box2d) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'BOX2D_intersects'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_box2d_intersects(box2d, box2d) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'BOX2D_intersects'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);


-- lwgeom  operator support functions

-------------------------------------------------------------------
-- BTREE indexes
-------------------------------------------------------------------

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_lt(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'lwgeom_lt'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_lt(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'lwgeom_lt'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_le(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'lwgeom_le'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_le(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'lwgeom_le'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_gt(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'lwgeom_gt'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_gt(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'lwgeom_gt'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_ge(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'lwgeom_ge'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_ge(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'lwgeom_ge'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_eq(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'lwgeom_eq'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_eq(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'lwgeom_eq'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_cmp(geometry, geometry) 
	RETURNS integer
	AS 'MODULE_PATHNAME', 'lwgeom_cmp'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_cmp(geometry, geometry) 
	RETURNS integer
	AS 'MODULE_PATHNAME', 'lwgeom_cmp'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

--
-- Sorting operators for Btree
--

CREATE OPERATOR < (
   LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = ST_geometry_lt,
   COMMUTATOR = '>', NEGATOR = '>=',
   RESTRICT = contsel, JOIN = contjoinsel
);

CREATE OPERATOR <= (
   LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = ST_geometry_le,
   COMMUTATOR = '>=', NEGATOR = '>',
   RESTRICT = contsel, JOIN = contjoinsel
);

CREATE OPERATOR = (
   LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = ST_geometry_eq,
   COMMUTATOR = '=', -- we might implement a faster negator here
   RESTRICT = contsel, JOIN = contjoinsel
);

CREATE OPERATOR >= (
   LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = ST_geometry_ge,
   COMMUTATOR = '<=', NEGATOR = '<',
   RESTRICT = contsel, JOIN = contjoinsel
);
CREATE OPERATOR > (
   LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = ST_geometry_gt,
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



-------------------------------------------------------------------
-- GiST indexes
-------------------------------------------------------------------
-- Deprecation in 1.2.3
CREATEFUNCTION postgis_gist_sel (internal, oid, internal, int4)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_gist_sel'
	LANGUAGE 'C';

-- Availability: 1.2.2
CREATEFUNCTION ST_postgis_gist_sel (internal, oid, internal, int4)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_gist_sel'
	LANGUAGE 'C';

-- Deprecation in 1.2.3
CREATEFUNCTION postgis_gist_joinsel(internal, oid, internal, smallint)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_gist_joinsel'
	LANGUAGE 'C';

-- Availability: 1.2.2
CREATEFUNCTION ST_postgis_gist_joinsel(internal, oid, internal, smallint)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_gist_joinsel'
	LANGUAGE 'C';

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_overleft(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_overleft'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_overleft(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_overleft'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_overright(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_overright'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_overright(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_overright'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_overabove(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_overabove'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_overabove(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_overabove'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_overbelow(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_overbelow'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_overbelow(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_overbelow'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_left(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_left'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_left(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_left'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_right(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_right'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_right(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_right'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_above(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_above'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_above(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_above'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_below(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_below'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_below(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_below'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_contain(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_contain'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_contain(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_contain'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_contained(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_contained'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_contained(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_contained'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_overlap(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_overlap'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry_overlap(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_overlap'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry_same(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_same'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

--Availability: 1.2.2
CREATEFUNCTION ST_geometry_same(geometry, geometry) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_same'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- GEOMETRY operators

CREATE OPERATOR << (
   LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = ST_geometry_left,
   COMMUTATOR = '>>',
   RESTRICT = positionsel, JOIN = positionjoinsel
);

CREATE OPERATOR &< (
   LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = ST_geometry_overleft,
   COMMUTATOR = '&>',
   RESTRICT = positionsel, JOIN = positionjoinsel
);

CREATE OPERATOR <<| (
   LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = ST_geometry_below,
   COMMUTATOR = '|>>',
   RESTRICT = positionsel, JOIN = positionjoinsel
);

CREATE OPERATOR &<| (
   LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = ST_geometry_overbelow,
   COMMUTATOR = '|&>',
   RESTRICT = positionsel, JOIN = positionjoinsel
);

CREATE OPERATOR && (
   LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = ST_geometry_overlap,
   COMMUTATOR = '&&',
   RESTRICT = ST_postgis_gist_sel, JOIN = ST_postgis_gist_joinsel
);

CREATE OPERATOR &> (
   LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = ST_geometry_overright,
   COMMUTATOR = '&<',
   RESTRICT = positionsel, JOIN = positionjoinsel
);

CREATE OPERATOR >> (
   LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = ST_geometry_right,
   COMMUTATOR = '<<',
   RESTRICT = positionsel, JOIN = positionjoinsel
);

CREATE OPERATOR |&> (
   LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = ST_geometry_overabove,
   COMMUTATOR = '&<|',
   RESTRICT = positionsel, JOIN = positionjoinsel
);

CREATE OPERATOR |>> (
   LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = ST_geometry_above,
   COMMUTATOR = '<<|',
   RESTRICT = positionsel, JOIN = positionjoinsel
);

CREATE OPERATOR ~= (
   LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = ST_geometry_same,
   COMMUTATOR = '~=', 
   RESTRICT = eqsel, JOIN = eqjoinsel
);

CREATE OPERATOR @ (
   LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = ST_geometry_contained,
   COMMUTATOR = '~',
   RESTRICT = contsel, JOIN = contjoinsel
);

CREATE OPERATOR ~ (
   LEFTARG = geometry, RIGHTARG = geometry, PROCEDURE = ST_geometry_contain,
   COMMUTATOR = '@',
   RESTRICT = contsel, JOIN = contjoinsel
);

-- gist support functions

CREATEFUNCTION LWGEOM_gist_consistent(internal,geometry,int4) 
	RETURNS bool 
	AS 'MODULE_PATHNAME' ,'LWGEOM_gist_consistent'
	LANGUAGE 'C';

CREATEFUNCTION LWGEOM_gist_compress(internal) 
	RETURNS internal 
	AS 'MODULE_PATHNAME','LWGEOM_gist_compress'
	LANGUAGE 'C';

CREATEFUNCTION LWGEOM_gist_penalty(internal,internal,internal) 
	RETURNS internal 
	AS 'MODULE_PATHNAME' ,'LWGEOM_gist_penalty'
	LANGUAGE 'C';

CREATEFUNCTION LWGEOM_gist_picksplit(internal, internal) 
	RETURNS internal 
	AS 'MODULE_PATHNAME' ,'LWGEOM_gist_picksplit'
	LANGUAGE 'C';

CREATEFUNCTION LWGEOM_gist_union(bytea, internal) 
	RETURNS internal 
	AS 'MODULE_PATHNAME' ,'LWGEOM_gist_union'
	LANGUAGE 'C';

CREATEFUNCTION LWGEOM_gist_same(box2d, box2d, internal) 
	RETURNS internal 
	AS 'MODULE_PATHNAME' ,'LWGEOM_gist_same'
	LANGUAGE 'C';

CREATEFUNCTION LWGEOM_gist_decompress(internal) 
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
        OPERATOR        1        << 	,
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
	
-------------------------------------------
-- other lwgeom functions
-------------------------------------------

-- Deprecation in 1.2.3
CREATEFUNCTION addBBOX(geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_addBBOX'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_addBBOX(geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_addBBOX'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION dropBBOX(geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_dropBBOX'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_dropBBOX(geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_dropBBOX'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);
	
-- Deprecation in 1.2.3
CREATEFUNCTION getSRID(geometry) 
	RETURNS int4
	AS 'MODULE_PATHNAME','LWGEOM_getSRID'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION getSRID(geometry) 
	RETURNS int4
	AS 'MODULE_PATHNAME','LWGEOM_getSRID'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION getBBOX(geometry)
        RETURNS box2d
        AS 'MODULE_PATHNAME','LWGEOM_to_BOX2DFLOAT4'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION getBBOX(geometry)
        RETURNS box2d
        AS 'MODULE_PATHNAME','LWGEOM_to_BOX2DFLOAT4'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-------------------------------------------
--- CHIP functions
-------------------------------------------

-- Deprecation in 1.2.3
CREATEFUNCTION srid(chip)
	RETURNS int4
	AS 'MODULE_PATHNAME','CHIP_getSRID'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_srid(chip)
	RETURNS int4
	AS 'MODULE_PATHNAME','CHIP_getSRID'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION height(chip)
	RETURNS int4
	AS 'MODULE_PATHNAME','CHIP_getHeight'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_height(chip)
	RETURNS int4
	AS 'MODULE_PATHNAME','CHIP_getHeight'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION factor(chip)
	RETURNS FLOAT4
	AS 'MODULE_PATHNAME','CHIP_getFactor'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_factor(chip)
	RETURNS FLOAT4
	AS 'MODULE_PATHNAME','CHIP_getFactor'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION width(chip)
	RETURNS int4
	AS 'MODULE_PATHNAME','CHIP_getWidth'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_width(chip)
	RETURNS int4
	AS 'MODULE_PATHNAME','CHIP_getWidth'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION datatype(chip)
	RETURNS int4
	AS 'MODULE_PATHNAME','CHIP_getDatatype'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_datatype(chip)
	RETURNS int4
	AS 'MODULE_PATHNAME','CHIP_getDatatype'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION compression(chip)
	RETURNS int4
	AS 'MODULE_PATHNAME','CHIP_getCompression'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_compression(chip)
	RETURNS int4
	AS 'MODULE_PATHNAME','CHIP_getCompression'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION setSRID(chip,int4)
	RETURNS chip
	AS 'MODULE_PATHNAME','CHIP_setSRID'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION setFactor(chip,float4)
	RETURNS chip
	AS 'MODULE_PATHNAME','CHIP_setFactor'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_setFactor(chip,float4)
	RETURNS chip
	AS 'MODULE_PATHNAME','CHIP_setFactor'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

------------------------------------------------------------------------
-- DEBUG
------------------------------------------------------------------------

-- Deprecation in 1.2.3
CREATEFUNCTION mem_size(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_mem_size'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_mem_size(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_mem_size'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION summary(geometry)
	RETURNS text
	AS 'MODULE_PATHNAME', 'LWGEOM_summary'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_summary(geometry)
	RETURNS text
	AS 'MODULE_PATHNAME', 'LWGEOM_summary'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION npoints(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_npoints'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_npoints(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_npoints'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION nrings(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_nrings'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_nrings(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_nrings'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

------------------------------------------------------------------------
-- Misures
------------------------------------------------------------------------

-- this is a fake (for back-compatibility)
-- uses 3d if 3d is available, 2d otherwise
-- Deprecation in 1.2.3
CREATEFUNCTION length3d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_length_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_length3d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_length_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION length2d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_length2d_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_length2d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_length2d_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

CREATEFUNCTION length(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_length_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- PostGIS equivalent function: length2d(geometry)
CREATEFUNCTION ST_Length(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_length2d_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- this is a fake (for back-compatibility)
-- uses 3d if 3d is available, 2d otherwise
-- Deprecation in 1.2.3
CREATEFUNCTION length3d_spheroid(geometry, spheroid)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_length_ellipsoid_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_length3d_spheroid(geometry, spheroid)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_length_ellipsoid_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION length_spheroid(geometry, spheroid)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_length_ellipsoid_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_length_spheroid(geometry, spheroid)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_length_ellipsoid_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION length2d_spheroid(geometry, spheroid)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_length2d_ellipsoid_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_length2d_spheroid(geometry, spheroid)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_length2d_ellipsoid_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- this is a fake (for back-compatibility)
-- uses 3d if 3d is available, 2d otherwise
-- Deprecation in 1.2.3
CREATEFUNCTION perimeter3d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_perimeter_poly'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_perimeter3d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_perimeter_poly'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION perimeter2d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_perimeter2d_poly'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_perimeter2d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_perimeter2d_poly'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION perimeter(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_perimeter_poly'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- PostGIS equivalent function: perimeter2d(geometry)
CREATEFUNCTION ST_Perimeter(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_perimeter2d_poly'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- this is an alias for 'area(geometry)'
-- there is nothing such an 'area3d'...
-- Deprecation in 1.2.3
CREATEFUNCTION area2d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_area_polygon'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
-- Deprecation in 1.3.4
CREATEFUNCTION ST_area2d(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_area_polygon'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION area(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_area_polygon'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- PostGIS equivalent function: area(geometry)
CREATEFUNCTION ST_Area(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_area_polygon'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION distance_spheroid(geometry,geometry,spheroid)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_distance_ellipsoid_point'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_distance_spheroid(geometry,geometry,spheroid)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_distance_ellipsoid_point'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION distance_sphere(geometry,geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_distance_sphere'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_distance_sphere(geometry,geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_distance_sphere'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Minimum distance. 2d only.
-- Deprecation in 1.2.3
CREATEFUNCTION distance(geometry,geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_mindistance2d'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: distance(geometry,geometry)
CREATEFUNCTION ST_Distance(geometry,geometry)
    RETURNS float8
    AS 'MODULE_PATHNAME', 'LWGEOM_mindistance2d'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Maximum distance between linestrings. 2d only. Very bogus.
-- Deprecation in 1.2.3
CREATEFUNCTION max_distance(geometry,geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_maxdistance2d_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_max_distance(geometry,geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_maxdistance2d_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION point_inside_circle(geometry,float8,float8,float8)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_inside_circle_point'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_point_inside_circle(geometry,float8,float8,float8)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_inside_circle_point'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION azimuth(geometry,geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_azimuth'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_azimuth(geometry,geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'LWGEOM_azimuth'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

------------------------------------------------------------------------
-- MISC
------------------------------------------------------------------------

-- Deprecation in 1.2.3
CREATEFUNCTION force_2d(geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_2d'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_force_2d(geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_2d'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION force_3dz(geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_3dz'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_force_3dz(geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_3dz'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- an alias for force_3dz
-- Deprecation in 1.2.3
CREATEFUNCTION force_3d(geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_3dz'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_force_3d(geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_3dz'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION force_3dm(geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_3dm'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_force_3dm(geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_3dm'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION force_4d(geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_4d'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_force_4d(geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_4d'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION force_collection(geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_collection'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_force_collection(geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_collection'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION multi(geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_multi'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_multi(geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_force_multi'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);


-- Deprecation in 1.2.3
CREATEFUNCTION expand(box3d,float8)
	RETURNS box3d
	AS 'MODULE_PATHNAME', 'BOX3D_expand'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_Expand(box3d,float8)
	RETURNS box3d
	AS 'MODULE_PATHNAME', 'BOX3D_expand'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION expand(box2d,float8)
	RETURNS box2d
	AS 'MODULE_PATHNAME', 'BOX2DFLOAT4_expand'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_expand(box2d,float8)
	RETURNS box2d
	AS 'MODULE_PATHNAME', 'BOX2DFLOAT4_expand'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION expand(geometry,float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_expand'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_expand(geometry,float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_expand'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION envelope(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_envelope'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- PostGIS equivalent function: envelope(geometry)
CREATEFUNCTION ST_Envelope(geometry)
        RETURNS geometry
        AS 'MODULE_PATHNAME', 'LWGEOM_envelope'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION reverse(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_reverse'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_Reverse(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_reverse'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION ForceRHR(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_forceRHR_poly'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_ForceRHR(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_forceRHR_poly'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION noop(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_noop'
	LANGUAGE 'C' _VOLATILE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_noop(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_noop'
	LANGUAGE 'C' _VOLATILE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION zmflag(geometry)
	RETURNS smallint
	AS 'MODULE_PATHNAME', 'LWGEOM_zmflag'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_zmflag(geometry)
	RETURNS smallint
	AS 'MODULE_PATHNAME', 'LWGEOM_zmflag'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION hasBBOX(geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_hasBBOX'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availabitily: 1.2.2
CREATEFUNCTION ST_HasBBOX(geometry)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'LWGEOM_hasBBOX'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION ndims(geometry)
	RETURNS smallint
	AS 'MODULE_PATHNAME', 'LWGEOM_ndims'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_NDims(geometry)
	RETURNS smallint
	AS 'MODULE_PATHNAME', 'LWGEOM_ndims'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION AsEWKT(geometry)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asEWKT'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_AsEWKT(geometry)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asEWKT'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION AsEWKB(geometry)
	RETURNS BYTEA
	AS 'MODULE_PATHNAME','WKBFromLWGEOM'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_AsEWKB(geometry)
	RETURNS BYTEA
	AS 'MODULE_PATHNAME','WKBFromLWGEOM'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION AsHEXEWKB(geometry)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asHEXEWKB'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_AsHEXEWKB(geometry)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asHEXEWKB'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION AsHEXEWKB(geometry, text)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asHEXEWKB'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_AsHEXEWKB(geometry, text)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asHEXEWKB'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION AsEWKB(geometry,text)
	RETURNS bytea
	AS 'MODULE_PATHNAME','WKBFromLWGEOM'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_AsEWKB(geometry,text)
	RETURNS bytea
	AS 'MODULE_PATHNAME','WKBFromLWGEOM'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION GeomFromEWKB(bytea)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOMFromWKB'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_GeomFromEWKB(bytea)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOMFromWKB'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION GeomFromEWKT(text)
	RETURNS geometry
	AS 'MODULE_PATHNAME','parse_WKT_lwgeom'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_GeomFromEWKT(text)
	RETURNS geometry
	AS 'MODULE_PATHNAME','parse_WKT_lwgeom'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION cache_bbox()
        RETURNS trigger
        AS 'MODULE_PATHNAME'
        LANGUAGE 'C';

-- Availability: 1.2.2
CREATEFUNCTION ST_Cache_BBox()
	RETURNS trigger
	AS 'MODULE_PATHNAME','cache_bbox'
	LANGUAGE 'C';

------------------------------------------------------------------------
-- CONSTRUCTORS
------------------------------------------------------------------------

-- Deprecation in 1.2.3
CREATEFUNCTION MakePoint(float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_MakePoint(float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION MakePoint(float8, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_MakePoint(float8, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION MakePoint(float8, float8, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_MakePoint(float8, float8, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION MakePointM(float8, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint3dm'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);
	
-- Availability: 1.3.4
CREATEFUNCTION ST_MakePointM(float8, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoint3dm'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION MakeBox2d(geometry, geometry)
	RETURNS box2d
	AS 'MODULE_PATHNAME', 'BOX2DFLOAT4_construct'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_MakeBox2d(geometry, geometry)
	RETURNS box2d
	AS 'MODULE_PATHNAME', 'BOX2DFLOAT4_construct'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION MakeBox3d(geometry, geometry)
	RETURNS box3d
	AS 'MODULE_PATHNAME', 'BOX3D_construct'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_MakeBox3d(geometry, geometry)
	RETURNS box3d
	AS 'MODULE_PATHNAME', 'BOX3D_construct'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION makeline_garray (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makeline_garray'
	LANGUAGE 'C' _IMMUTABLE_STRICT;

-- Availability: 1.2.2
CREATEFUNCTION ST_MakeLine_GArray (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makeline_garray'
	LANGUAGE 'C' _IMMUTABLE_STRICT;

-- Availability: 1.4.0
CREATEFUNCTION ST_MakeLine (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makeline_garray'
	LANGUAGE 'C' _IMMUTABLE_STRICT;

-- Deprecation in 1.2.3
CREATEFUNCTION LineFromMultiPoint(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_line_from_mpoint'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_LineFromMultiPoint(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_line_from_mpoint'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION MakeLine(geometry, geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makeline'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_MakeLine(geometry, geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makeline'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION AddPoint(geometry, geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_addpoint'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_AddPoint(geometry, geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_addpoint'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION AddPoint(geometry, geometry, integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_addpoint'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_AddPoint(geometry, geometry, integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_addpoint'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION RemovePoint(geometry, integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_removepoint'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_RemovePoint(geometry, integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_removepoint'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION SetPoint(geometry, integer, geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_setpoint_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_SetPoint(geometry, integer, geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_setpoint_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);


-- Deprecation in 1.2.3
CREATEFUNCTION MakePolygon(geometry, geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoly'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_MakePolygon(geometry, geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoly'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION MakePolygon(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoly'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_MakePolygon(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_makepoly'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION BuildArea(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_buildarea'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_BuildArea(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_buildarea'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION Polygonize_GArray (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'polygonize_garray'
	LANGUAGE 'C' _IMMUTABLE_STRICT;

-- Availability: 1.2.2
CREATEFUNCTION ST_Polygonize_GArray (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'polygonize_garray'
	LANGUAGE 'C' _IMMUTABLE_STRICT;

-- Availability: 1.4.0
CREATEFUNCTION ST_Polygonize (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'polygonize_garray'
	LANGUAGE 'C' _IMMUTABLE_STRICT;

-- Deprecation in 1.2.3
CREATEFUNCTION LineMerge(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'linemerge'
	LANGUAGE 'C' _IMMUTABLE_STRICT;

-- Availability: 1.2.2
CREATEFUNCTION ST_LineMerge(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'linemerge'
	LANGUAGE 'C' _IMMUTABLE_STRICT;


CREATE TYPE geometry_dump AS (path integer[], geom geometry);

-- Deprecation in 1.2.3
CREATEFUNCTION Dump(geometry)
	RETURNS SETOF geometry_dump
	AS 'MODULE_PATHNAME', 'LWGEOM_dump'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_Dump(geometry)
	RETURNS SETOF geometry_dump
	AS 'MODULE_PATHNAME', 'LWGEOM_dump'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION DumpRings(geometry)
	RETURNS SETOF geometry_dump
	AS 'MODULE_PATHNAME', 'LWGEOM_dump_rings'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_DumpRings(geometry)
	RETURNS SETOF geometry_dump
	AS 'MODULE_PATHNAME', 'LWGEOM_dump_rings'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

------------------------------------------------------------------------

--
-- Aggregate functions
--

-- Deprecation in 1.2.3
CREATEFUNCTION combine_bbox(box2d,geometry)
	RETURNS box2d
	AS 'MODULE_PATHNAME', 'BOX2DFLOAT4_combine'
	LANGUAGE 'C' _IMMUTABLE;

-- Availability: 1.2.2
CREATEFUNCTION ST_Combine_BBox(box2d,geometry)
	RETURNS box2d
	AS 'MODULE_PATHNAME', 'BOX2DFLOAT4_combine'
	LANGUAGE 'C' _IMMUTABLE;

-- Temporary hack function
CREATEFUNCTION combine_bbox(box3d_extent,geometry)
	RETURNS box3d_extent
	AS 'MODULE_PATHNAME', 'BOX3D_combine'
	LANGUAGE 'C' _IMMUTABLE;

-- Temporary hack function
CREATEFUNCTION ST_Combine_BBox(box3d_extent,geometry)
	RETURNS box3d_extent
	AS 'MODULE_PATHNAME', 'BOX3D_combine'
	LANGUAGE 'C' _IMMUTABLE;

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
CREATEFUNCTION combine_bbox(box3d,geometry)
	RETURNS box3d
	AS 'MODULE_PATHNAME', 'BOX3D_combine'
	LANGUAGE 'C' _IMMUTABLE;

-- Availability: 1.2.2
CREATEFUNCTION ST_Combine_BBox(box3d,geometry)
	RETURNS box3d
	AS 'MODULE_PATHNAME', 'BOX3D_combine'
	LANGUAGE 'C' _IMMUTABLE;

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

-----------------------------------------------------------------------
-- ESTIMATED_EXTENT( <schema name>, <table name>, <column name> )
-----------------------------------------------------------------------
-- Deprecation in 1.2.3
CREATEFUNCTION estimated_extent(text,text,text) RETURNS box2d AS
	'MODULE_PATHNAME', 'LWGEOM_estimated_extent'
	LANGUAGE 'C' _IMMUTABLE_STRICT _SECURITY_DEFINER;

-- Availability: 1.2.2
CREATEFUNCTION ST_estimated_extent(text,text,text) RETURNS box2d AS
	'MODULE_PATHNAME', 'LWGEOM_estimated_extent'
	LANGUAGE 'C' _IMMUTABLE_STRICT _SECURITY_DEFINER;

-----------------------------------------------------------------------
-- ESTIMATED_EXTENT( <table name>, <column name> )
-----------------------------------------------------------------------
-- Deprecation in 1.2.3
CREATEFUNCTION estimated_extent(text,text) RETURNS box2d AS
	'MODULE_PATHNAME', 'LWGEOM_estimated_extent'
	LANGUAGE 'C' _IMMUTABLE_STRICT _SECURITY_DEFINER; 

-- Availability: 1.2.2
CREATEFUNCTION ST_estimated_extent(text,text) RETURNS box2d AS
	'MODULE_PATHNAME', 'LWGEOM_estimated_extent'
	LANGUAGE 'C' _IMMUTABLE_STRICT _SECURITY_DEFINER; 

-----------------------------------------------------------------------
-- FIND_EXTENT( <schema name>, <table name>, <column name> )
-----------------------------------------------------------------------
-- Deprecation in 1.2.3
CREATEFUNCTION find_extent(text,text,text) RETURNS box2d AS
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
LANGUAGE 'plpgsql' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_find_extent(text,text,text) RETURNS box2d AS
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
LANGUAGE 'plpgsql' _IMMUTABLE_STRICT; -- WITH (isstrict);


-----------------------------------------------------------------------
-- FIND_EXTENT( <table name>, <column name> )
-----------------------------------------------------------------------
-- Deprecation in 1.2.3
CREATEFUNCTION find_extent(text,text) RETURNS box2d AS
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
LANGUAGE 'plpgsql' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_find_extent(text,text) RETURNS box2d AS
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
LANGUAGE 'plpgsql' _IMMUTABLE_STRICT; -- WITH (isstrict);

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
CREATEFUNCTION rename_geometry_table_constraints() RETURNS text
AS 
$$
SELECT 'rename_geometry_table_constraint() is obsoleted'::text
$$
LANGUAGE 'SQL' _IMMUTABLE;

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
CREATEFUNCTION fix_geometry_columns() RETURNS text
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
LANGUAGE 'plpgsql' _VOLATILE;

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
CREATEFUNCTION populate_geometry_columns()
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
LANGUAGE 'plpgsql' _VOLATILE;
  
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
CREATEFUNCTION populate_geometry_columns(tbl_oid oid)
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
            EXECUTE 'SELECT public.srid(' || quote_ident(gcs.attname) || ') 
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
                        RAISE WARNING 'Not inserting \'%\' in \'%.%\' into geometry_columns: could not apply constraint CHECK (srid(%) = %)', quote_ident(gcs.attname), quote_ident(gcs.nspname), quote_ident(gcs.relname), quote_ident(gcs.attname), gsrid;
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
            EXECUTE 'SELECT public.ndims(' || quote_ident(gcs.attname) || ') 
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
                        RAISE WARNING 'Not inserting \'%\' in \'%.%\' into geometry_columns: could not apply constraint CHECK (ndims(%) = %)', quote_ident(gcs.attname), quote_ident(gcs.nspname), quote_ident(gcs.relname), quote_ident(gcs.attname), gndims;
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
            EXECUTE 'SELECT public.geometrytype(' || quote_ident(gcs.attname) || ') 
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
            RAISE WARNING 'Not inserting \'%\' in \'%.%\' into geometry_columns: could not determine the srid', quote_ident(gcs.attname), quote_ident(gcs.nspname), quote_ident(gcs.relname);
        ELSIF (gndims IS NULL) THEN
            RAISE WARNING 'Not inserting \'%\' in \'%.%\' into geometry_columns: could not determine the number of dimensions', quote_ident(gcs.attname), quote_ident(gcs.nspname), quote_ident(gcs.relname);
        ELSIF (gtype IS NULL) THEN
            RAISE WARNING 'Not inserting \'%\' in \'%.%\' into geometry_columns: could not determine the geometry type', quote_ident(gcs.attname), quote_ident(gcs.nspname), quote_ident(gcs.relname);
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

            EXECUTE 'SELECT public.ndims(' || quote_ident(gcs.attname) || ') 
                     FROM ' || quote_ident(gcs.nspname) || '.' || quote_ident(gcs.relname) || ' 
                     WHERE ' || quote_ident(gcs.attname) || ' IS NOT NULL LIMIT 1' 
                INTO gc;
            gndims := gc.ndims;
            
            EXECUTE 'SELECT public.srid(' || quote_ident(gcs.attname) || ') 
                     FROM ' || quote_ident(gcs.nspname) || '.' || quote_ident(gcs.relname) || ' 
                     WHERE ' || quote_ident(gcs.attname) || ' IS NOT NULL LIMIT 1' 
                INTO gc;
            gsrid := gc.srid;
            
            EXECUTE 'SELECT public.geometrytype(' || quote_ident(gcs.attname) || ') 
                     FROM ' || quote_ident(gcs.nspname) || '.' || quote_ident(gcs.relname) || ' 
                     WHERE ' || quote_ident(gcs.attname) || ' IS NOT NULL LIMIT 1' 
                INTO gc;
            gtype := gc.geometrytype;
            
	    IF (gndims IS NULL) THEN
                RAISE WARNING 'Not inserting \'%\' in \'%.%\' into geometry_columns: could not determine ndims', quote_ident(gcs.attname), quote_ident(gcs.nspname), quote_ident(gcs.relname);
            ELSIF (gsrid IS NULL) THEN
                RAISE WARNING 'Not inserting \'%\' in \'%.%\' into geometry_columns: could not determine srid', quote_ident(gcs.attname), quote_ident(gcs.nspname), quote_ident(gcs.relname);
            ELSIF (gtype IS NULL) THEN
                RAISE WARNING 'Not inserting \'%\' in \'%.%\' into geometry_columns: could not determine gtype', quote_ident(gcs.attname), quote_ident(gcs.nspname), quote_ident(gcs.relname);
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
LANGUAGE 'plpgsql' _VOLATILE;


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
CREATEFUNCTION probe_geometry_columns() RETURNS text AS
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
LANGUAGE 'plpgsql' _VOLATILE;

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
CREATEFUNCTION AddGeometryColumn(varchar,varchar,varchar,varchar,integer,varchar,integer)
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
			   (new_type = 'MULTISURFACEM')) )
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
	CURVEPOLYGONM, MULTISURFACEM,
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
LANGUAGE 'plpgsql' _VOLATILE_STRICT; -- WITH (isstrict);

----------------------------------------------------------------------------
-- ADDGEOMETRYCOLUMN ( <schema>, <table>, <column>, <srid>, <type>, <dim> )
----------------------------------------------------------------------------
--
-- This is a wrapper to the real AddGeometryColumn, for use
-- when catalogue is undefined
--
----------------------------------------------------------------------------
CREATEFUNCTION AddGeometryColumn(varchar,varchar,varchar,integer,varchar,integer) RETURNS text AS $$ 
DECLARE
	ret  text;
BEGIN
	SELECT AddGeometryColumn('',$1,$2,$3,$4,$5,$6) into ret;
	RETURN ret;
END;
$$
LANGUAGE 'plpgsql' _STABLE_STRICT; -- WITH (isstrict);

----------------------------------------------------------------------------
-- ADDGEOMETRYCOLUMN ( <table>, <column>, <srid>, <type>, <dim> )
----------------------------------------------------------------------------
--
-- This is a wrapper to the real AddGeometryColumn, for use
-- when catalogue and schema are undefined
--
----------------------------------------------------------------------------
CREATEFUNCTION AddGeometryColumn(varchar,varchar,integer,varchar,integer) RETURNS text AS $$ 
DECLARE
	ret  text;
BEGIN
	SELECT AddGeometryColumn('','',$1,$2,$3,$4,$5) into ret;
	RETURN ret;
END;
$$
LANGUAGE 'plpgsql' _VOLATILE_STRICT; -- WITH (isstrict);

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
CREATEFUNCTION DropGeometryColumn(varchar, varchar,varchar,varchar)
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
LANGUAGE 'plpgsql' _VOLATILE_STRICT; -- WITH (isstrict);

-----------------------------------------------------------------------
-- DROPGEOMETRYCOLUMN
--   <schema>, <table>, <column>
-----------------------------------------------------------------------
--
-- This is a wrapper to the real DropGeometryColumn, for use
-- when catalogue is undefined
--
-----------------------------------------------------------------------
CREATEFUNCTION DropGeometryColumn(varchar,varchar,varchar)
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
LANGUAGE 'plpgsql' _VOLATILE_STRICT; -- WITH (isstrict);

-----------------------------------------------------------------------
-- DROPGEOMETRYCOLUMN
--   <table>, <column>
-----------------------------------------------------------------------
--
-- This is a wrapper to the real DropGeometryColumn, for use
-- when catalogue and schema is undefined. 
--
-----------------------------------------------------------------------
CREATEFUNCTION DropGeometryColumn(varchar,varchar)
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
LANGUAGE 'plpgsql' _VOLATILE_STRICT; -- WITH (isstrict);

-----------------------------------------------------------------------
-- DROPGEOMETRYTABLE
--   <catalogue>, <schema>, <table>
-----------------------------------------------------------------------
--
-- Drop a table and all its references in geometry_columns
--
-----------------------------------------------------------------------
CREATEFUNCTION DropGeometryTable(varchar, varchar,varchar)
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
LANGUAGE 'plpgsql' _VOLATILE_STRICT; -- WITH (isstrict);

-----------------------------------------------------------------------
-- DROPGEOMETRYTABLE
--   <schema>, <table>
-----------------------------------------------------------------------
--
-- Drop a table and all its references in geometry_columns
--
-----------------------------------------------------------------------
CREATEFUNCTION DropGeometryTable(varchar,varchar) RETURNS text AS 
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
CREATEFUNCTION DropGeometryTable(varchar) RETURNS text AS 
$$ SELECT DropGeometryTable('','',$1) $$
LANGUAGE 'sql' _VOLATILE_STRICT; -- WITH (isstrict);

-----------------------------------------------------------------------
-- UPDATEGEOMETRYSRID
--   <catalogue>, <schema>, <table>, <column>, <srid>
-----------------------------------------------------------------------
--
-- Change SRID of all features in a spatially-enabled table
--
-----------------------------------------------------------------------
CREATEFUNCTION UpdateGeometrySRID(varchar,varchar,varchar,varchar,integer)
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
LANGUAGE 'plpgsql' _VOLATILE_STRICT; -- WITH (isstrict);

-----------------------------------------------------------------------
-- UPDATEGEOMETRYSRID
--   <schema>, <table>, <column>, <srid>
-----------------------------------------------------------------------
CREATEFUNCTION UpdateGeometrySRID(varchar,varchar,varchar,integer)
	RETURNS text
	AS $$ 
DECLARE
	ret  text;
BEGIN
	SELECT UpdateGeometrySRID('',$1,$2,$3,$4) into ret;
	RETURN ret;
END;
$$
LANGUAGE 'plpgsql' _VOLATILE_STRICT; -- WITH (isstrict);

-----------------------------------------------------------------------
-- UPDATEGEOMETRYSRID
--   <table>, <column>, <srid>
-----------------------------------------------------------------------
CREATEFUNCTION UpdateGeometrySRID(varchar,varchar,integer)
	RETURNS text
	AS $$ 
DECLARE
	ret  text;
BEGIN
	SELECT UpdateGeometrySRID('','',$1,$2,$3) into ret;
	RETURN ret;
END;
$$
LANGUAGE 'plpgsql' _VOLATILE_STRICT; -- WITH (isstrict);

-----------------------------------------------------------------------
-- FIND_SRID( <schema>, <table>, <geom col> )
-----------------------------------------------------------------------
CREATEFUNCTION find_srid(varchar,varchar,varchar) RETURNS int4 AS
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
LANGUAGE 'plpgsql' _IMMUTABLE_STRICT; -- WITH (iscachable); 


---------------------------------------------------------------
-- PROJ support
---------------------------------------------------------------

CREATEFUNCTION get_proj4_from_srid(integer) RETURNS text AS
$$
BEGIN
	RETURN proj4text::text FROM spatial_ref_sys WHERE srid= $1;
END;
$$
LANGUAGE 'plpgsql' _IMMUTABLE_STRICT; -- WITH (iscachable,isstrict);



CREATEFUNCTION transform_geometry(geometry,text,text,int)
	RETURNS geometry
	AS 'MODULE_PATHNAME','transform_geom'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

CREATEFUNCTION transform(geometry,integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME','transform'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: transform(geometry,integer)
CREATEFUNCTION ST_Transform(geometry,integer)
    RETURNS geometry
    AS 'MODULE_PATHNAME','transform'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);


-----------------------------------------------------------------------
-- POSTGIS_VERSION()
-----------------------------------------------------------------------

CREATEFUNCTION postgis_version() RETURNS text
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' _IMMUTABLE;

CREATEFUNCTION postgis_proj_version() RETURNS text
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' _IMMUTABLE;

--
-- IMPORTANT:
-- Starting at 1.1.0 this function is used by postgis_proc_upgrade.pl
-- to extract version of postgis being installed.
-- Do not modify this w/out also changing postgis_proc_upgrade.pl
--
CREATEFUNCTION postgis_scripts_installed() RETURNS text
        AS _POSTGIS_SQL_SELECT_POSTGIS_SCRIPTS_VERSION 
        LANGUAGE 'sql' _IMMUTABLE;

CREATEFUNCTION postgis_lib_version() RETURNS text
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' _IMMUTABLE; -- a new lib will require a new session

-- NOTE: starting at 1.1.0 this is the same of postgis_lib_version()
CREATEFUNCTION postgis_scripts_released() RETURNS text
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' _IMMUTABLE;

CREATEFUNCTION postgis_uses_stats() RETURNS bool
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' _IMMUTABLE;

CREATEFUNCTION postgis_geos_version() RETURNS text
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' _IMMUTABLE;

CREATEFUNCTION postgis_scripts_build_date() RETURNS text
        AS _POSTGIS_SQL_SELECT_POSTGIS_BUILD_DATE
        LANGUAGE 'sql' _IMMUTABLE;

CREATEFUNCTION postgis_lib_build_date() RETURNS text
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' _IMMUTABLE;



CREATEFUNCTION postgis_full_version() RETURNS text
AS $$ 
DECLARE
	libver text;
	projver text;
	geosver text;
	usestats bool;
	dbproc text;
	relproc text;
	fullver text;
BEGIN
	SELECT postgis_lib_version() INTO libver;
	SELECT postgis_proj_version() INTO projver;
	SELECT postgis_geos_version() INTO geosver;
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
LANGUAGE 'plpgsql' _IMMUTABLE;

---------------------------------------------------------------
-- CASTS
---------------------------------------------------------------

-- Deprecation in 1.2.3
CREATEFUNCTION box2d(geometry)
        RETURNS box2d
        AS 'MODULE_PATHNAME','LWGEOM_to_BOX2DFLOAT4'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_box2d(geometry)
        RETURNS box2d
        AS 'MODULE_PATHNAME','LWGEOM_to_BOX2DFLOAT4'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION box3d(geometry)
        RETURNS box3d
        AS 'MODULE_PATHNAME','LWGEOM_to_BOX3D'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_box3d(geometry)
        RETURNS box3d
        AS 'MODULE_PATHNAME','LWGEOM_to_BOX3D'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION box(geometry)
        RETURNS box
        AS 'MODULE_PATHNAME','LWGEOM_to_BOX'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_box(geometry)
        RETURNS box
        AS 'MODULE_PATHNAME','LWGEOM_to_BOX'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION box2d(box3d)
        RETURNS box2d
        AS 'MODULE_PATHNAME','BOX3D_to_BOX2DFLOAT4'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_box2d(box3d)
        RETURNS box2d
        AS 'MODULE_PATHNAME','BOX3D_to_BOX2DFLOAT4'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION box3d(box2d)
        RETURNS box3d
        AS 'MODULE_PATHNAME','BOX2DFLOAT4_to_BOX3D'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_box3d(box2d)
        RETURNS box3d
        AS 'MODULE_PATHNAME','BOX2DFLOAT4_to_BOX3D'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION box(box3d)
        RETURNS box
        AS 'MODULE_PATHNAME','BOX3D_to_BOX'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_box(box3d)
        RETURNS box
        AS 'MODULE_PATHNAME','BOX3D_to_BOX'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION text(geometry)
        RETURNS text
        AS 'MODULE_PATHNAME','LWGEOM_to_text'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_text(geometry)
        RETURNS text
        AS 'MODULE_PATHNAME','LWGEOM_to_text'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- this is kept for backward-compatibility
-- Deprecation in 1.2.3
CREATEFUNCTION box3dtobox(box3d)
        RETURNS box
        AS 'SELECT box($1)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry(box2d)
        RETURNS geometry
        AS 'MODULE_PATHNAME','BOX2DFLOAT4_to_LWGEOM'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry(box2d)
        RETURNS geometry
        AS 'MODULE_PATHNAME','BOX2DFLOAT4_to_LWGEOM'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry(box3d)
        RETURNS geometry
        AS 'MODULE_PATHNAME','BOX3D_to_LWGEOM'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry(box3d)
        RETURNS geometry
        AS 'MODULE_PATHNAME','BOX3D_to_LWGEOM'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry(text)
        RETURNS geometry
        AS 'MODULE_PATHNAME','parse_WKT_lwgeom'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry(text)
        RETURNS geometry
        AS 'MODULE_PATHNAME','parse_WKT_lwgeom'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry(chip)
	RETURNS geometry
	AS 'MODULE_PATHNAME','CHIP_to_LWGEOM'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry(chip)
	RETURNS geometry
	AS 'MODULE_PATHNAME','CHIP_to_LWGEOM'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION geometry(bytea)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_from_bytea'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_geometry(bytea)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_from_bytea'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION bytea(geometry)
	RETURNS bytea
	AS 'MODULE_PATHNAME','LWGEOM_to_bytea'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_bytea(geometry)
	RETURNS bytea
	AS 'MODULE_PATHNAME','LWGEOM_to_bytea'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION text(bool)
	RETURNS text
	AS 'MODULE_PATHNAME','BOOL_to_text'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_text(bool)
	RETURNS text
	AS 'MODULE_PATHNAME','BOOL_to_text'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- 7.3+ explicit casting definitions
CREATE CAST (geometry AS box2d) WITH FUNCTION ST_box2d(geometry) AS IMPLICIT;
CREATE CAST (geometry AS box3d) WITH FUNCTION ST_box3d(geometry) AS IMPLICIT;
CREATE CAST (geometry AS box) WITH FUNCTION ST_box(geometry) AS IMPLICIT;
CREATE CAST (box3d AS box2d) WITH FUNCTION ST_box2d(box3d) AS IMPLICIT;
CREATE CAST (box2d AS box3d) WITH FUNCTION ST_box3d(box2d) AS IMPLICIT;
CREATE CAST (box2d AS geometry) WITH FUNCTION ST_geometry(box2d) AS IMPLICIT;
CREATE CAST (box3d AS box) WITH FUNCTION ST_box(box3d) AS IMPLICIT;
CREATE CAST (box3d AS geometry) WITH FUNCTION ST_geometry(box3d) AS IMPLICIT;
CREATE CAST (text AS geometry) WITH FUNCTION ST_geometry(text) AS IMPLICIT;
CREATE CAST (geometry AS text) WITH FUNCTION ST_text(geometry) AS IMPLICIT;
CREATE CAST (chip AS geometry) WITH FUNCTION ST_geometry(chip) AS IMPLICIT;
CREATE CAST (bytea AS geometry) WITH FUNCTION ST_geometry(bytea) AS IMPLICIT;
CREATE CAST (geometry AS bytea) WITH FUNCTION ST_bytea(geometry) AS IMPLICIT;

-- Casts to allow the box3d_extent type to automatically cast to box3d/box2d in queries
CREATE CAST (box3d_extent AS box3d) WITH FUNCTION ST_box3d_extent(box3d_extent) AS IMPLICIT;
CREATE CAST (box3d_extent AS box2d) WITH FUNCTION ST_box2d(box3d_extent) AS IMPLICIT;
CREATE CAST (box3d_extent AS geometry) WITH FUNCTION ST_geometry(box3d_extent) AS IMPLICIT;

---------------------------------------------------------------
-- Algorithms
---------------------------------------------------------------

-- Deprecation in 1.2.3
CREATEFUNCTION Simplify(geometry, float8)
   RETURNS geometry
   AS 'MODULE_PATHNAME', 'LWGEOM_simplify2d'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_Simplify(geometry, float8)
   RETURNS geometry
   AS 'MODULE_PATHNAME', 'LWGEOM_simplify2d'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- SnapToGrid(input, xoff, yoff, xsize, ysize)
-- Deprecation in 1.2.3
CREATEFUNCTION SnapToGrid(geometry, float8, float8, float8, float8)
   RETURNS geometry
   AS 'MODULE_PATHNAME', 'LWGEOM_snaptogrid'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_SnapToGrid(geometry, float8, float8, float8, float8)
   RETURNS geometry
   AS 'MODULE_PATHNAME', 'LWGEOM_snaptogrid'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- SnapToGrid(input, xsize, ysize) # offsets=0
-- Deprecation in 1.2.3
CREATEFUNCTION SnapToGrid(geometry, float8, float8)
   RETURNS geometry
   AS 'SELECT SnapToGrid($1, 0, 0, $2, $3)'
   LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_SnapToGrid(geometry, float8, float8)
   RETURNS geometry
   AS 'SELECT ST_SnapToGrid($1, 0, 0, $2, $3)'
   LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- SnapToGrid(input, size) # xsize=ysize=size, offsets=0
-- Deprecation in 1.2.3
CREATEFUNCTION SnapToGrid(geometry, float8)
   RETURNS geometry
   AS 'SELECT SnapToGrid($1, 0, 0, $2, $2)'
   LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_SnapToGrid(geometry, float8)
   RETURNS geometry
   AS 'SELECT ST_SnapToGrid($1, 0, 0, $2, $2)'
   LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- SnapToGrid(input, point_offsets, xsize, ysize, zsize, msize)
-- Deprecation in 1.2.3
CREATEFUNCTION SnapToGrid(geometry, geometry, float8, float8, float8, float8)
   RETURNS geometry
   AS 'MODULE_PATHNAME', 'LWGEOM_snaptogrid_pointoff'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_SnapToGrid(geometry, geometry, float8, float8, float8, float8)
   RETURNS geometry
   AS 'MODULE_PATHNAME', 'LWGEOM_snaptogrid_pointoff'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION Segmentize(geometry, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_segmentize2d'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_Segmentize(geometry, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_segmentize2d'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

---------------------------------------------------------------
-- LRS
---------------------------------------------------------------

-- Deprecation in 1.2.3
CREATEFUNCTION line_interpolate_point(geometry, float8)
   RETURNS geometry
   AS 'MODULE_PATHNAME', 'LWGEOM_line_interpolate_point'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_line_interpolate_point(geometry, float8)
   RETURNS geometry
   AS 'MODULE_PATHNAME', 'LWGEOM_line_interpolate_point'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION line_substring(geometry, float8, float8)
   RETURNS geometry
   AS 'MODULE_PATHNAME', 'LWGEOM_line_substring'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_line_substring(geometry, float8, float8)
   RETURNS geometry
   AS 'MODULE_PATHNAME', 'LWGEOM_line_substring'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION line_locate_point(geometry, geometry)
   RETURNS float8
   AS 'MODULE_PATHNAME', 'LWGEOM_line_locate_point'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_line_locate_point(geometry, geometry)
   RETURNS float8
   AS 'MODULE_PATHNAME', 'LWGEOM_line_locate_point'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION locate_between_measures(geometry, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_locate_between_m'
	LANGUAGE 'C' _IMMUTABLE_STRICT;

-- Availability: 1.2.2
CREATEFUNCTION ST_locate_between_measures(geometry, float8, float8)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_locate_between_m'
	LANGUAGE 'C' _IMMUTABLE_STRICT;

-- Deprecation in 1.2.3
CREATEFUNCTION locate_along_measure(geometry, float8)
	RETURNS geometry
	AS $$ SELECT locate_between_measures($1, $2, $2) $$
	LANGUAGE 'sql' _IMMUTABLE_STRICT;

-- Availability: 1.2.2
CREATEFUNCTION ST_locate_along_measure(geometry, float8)
	RETURNS geometry
	AS $$ SELECT locate_between_measures($1, $2, $2) $$
	LANGUAGE 'sql' _IMMUTABLE_STRICT;

---------------------------------------------------------------
-- GEOS
---------------------------------------------------------------

-- Deprecation in 1.2.3
CREATEFUNCTION intersection(geometry,geometry)
   RETURNS geometry
   AS 'MODULE_PATHNAME','intersection'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: intersection(geometry,geometry)
CREATEFUNCTION ST_Intersection(geometry,geometry)
    RETURNS geometry
    AS 'MODULE_PATHNAME','intersection'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION buffer(geometry,float8)
   RETURNS geometry
   AS 'MODULE_PATHNAME','buffer'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: buffer(geometry,float8)
CREATEFUNCTION ST_Buffer(geometry,float8)
    RETURNS geometry
    AS 'MODULE_PATHNAME','buffer'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION buffer(geometry,float8,integer)
   RETURNS geometry
   AS 'MODULE_PATHNAME','buffer'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_buffer(geometry,float8,integer)
   RETURNS geometry
   AS 'MODULE_PATHNAME','buffer'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);
   
-- Deprecation in 1.2.3
CREATEFUNCTION convexhull(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','convexhull'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: convexhull(geometry)
CREATEFUNCTION ST_ConvexHull(geometry)
    RETURNS geometry
    AS 'MODULE_PATHNAME','convexhull'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Only accepts LINESTRING as parameters.
-- Availability: 1.4.0
CREATEFUNCTION _ST_LineCrossingDirection(geometry, geometry)
    RETURNS integer
    AS 'MODULE_PATHNAME', 'ST_LineCrossingDirection'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.4.0
CREATEFUNCTION ST_LineCrossingDirection(geometry, geometry)
  RETURNS integer AS 
  $$ SELECT CASE WHEN NOT $1 && $2 THEN 0 ELSE _ST_LineCrossingDirection($1,$2) END $$
  LANGUAGE 'sql' IMMUTABLE;


-- Only accepts LINESTRING as parameters.
-- Availability: 1.4.0
CREATEFUNCTION ST_LocateBetweenElevations(geometry, float8, float8)
    RETURNS geometry
    AS 'MODULE_PATHNAME', 'ST_LocateBetweenElevations'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

#if POSTGIS_GEOS_VERSION >= 30
-- Requires GEOS >= 3.0.0
-- Availability: 1.3.3
CREATEFUNCTION ST_SimplifyPreserveTopology(geometry, float8)
    RETURNS geometry
    AS 'MODULE_PATHNAME','topologypreservesimplify'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);
#endif

#if POSTGIS_GEOS_VERSION >= 31
-- Requires GEOS >= 3.1.0
-- Availability: 1.4.0
CREATEFUNCTION ST_IsValidReason(geometry)
    RETURNS text
    AS 'MODULE_PATHNAME', 'isvalidreason'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);
#endif

-- Deprecation in 1.2.3
CREATEFUNCTION difference(geometry,geometry)
	RETURNS geometry
        AS 'MODULE_PATHNAME','difference'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: difference(geometry,geometry)
CREATEFUNCTION ST_Difference(geometry,geometry)
    RETURNS geometry
    AS 'MODULE_PATHNAME','difference'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION boundary(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','boundary'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: boundary(geometry)
CREATEFUNCTION ST_Boundary(geometry)
        RETURNS geometry
        AS 'MODULE_PATHNAME','boundary'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION symdifference(geometry,geometry)
        RETURNS geometry
        AS 'MODULE_PATHNAME','symdifference'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: symdifference(geometry,geometry)
CREATEFUNCTION ST_SymDifference(geometry,geometry)
    RETURNS geometry
    AS 'MODULE_PATHNAME','symdifference'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION symmetricdifference(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','symdifference'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_symmetricdifference(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','symdifference'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION GeomUnion(geometry,geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','geomunion'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: GeomUnion(geometry,geometry)
CREATEFUNCTION ST_Union(geometry,geometry)
    RETURNS geometry
    AS 'MODULE_PATHNAME','geomunion'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

--------------------------------------------------------------------------------
-- Aggregates and their supporting functions
--------------------------------------------------------------------------------

-- Deprecation in 1.2.3
CREATEFUNCTION collector(geometry, geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_collect'
	LANGUAGE 'C' _IMMUTABLE;

-- Availability: 1.2.2
CREATEFUNCTION ST_collector(geometry, geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_collect'
	LANGUAGE 'C' _IMMUTABLE;

-- Deprecation in 1.2.3
CREATEFUNCTION collect(geometry, geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_collect'
	LANGUAGE 'C' _IMMUTABLE;

-- Availability: 1.2.2
CREATEFUNCTION ST_collect(geometry, geometry) 
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_collect'
	LANGUAGE 'C' _IMMUTABLE;

-- Deprecation in 1.2.3
CREATE AGGREGATE memcollect(
	sfunc = ST_collect,
	basetype = geometry,
	stype = geometry
	);

-- Availability: 1.2.2
CREATE AGGREGATE ST_memcollect(
	sfunc = ST_collect,
	basetype = geometry,
	stype = geometry
	);

-- Deprecation in 1.2.3
CREATEFUNCTION geom_accum (geometry[],geometry)
	RETURNS geometry[]
	AS 'MODULE_PATHNAME', 'LWGEOM_accum'
	LANGUAGE 'C' _IMMUTABLE;

-- Availability: 1.2.2
CREATEFUNCTION ST_geom_accum (geometry[],geometry)
	RETURNS geometry[]
	AS 'MODULE_PATHNAME', 'LWGEOM_accum'
	LANGUAGE 'C' _IMMUTABLE;



-- Deprecation in 1.2.3
CREATEFUNCTION collect_garray (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_collect_garray'
	LANGUAGE 'C' _IMMUTABLE_STRICT;

-- Availability: 1.2.2
CREATEFUNCTION ST_collect_garray (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_collect_garray'
	LANGUAGE 'C' _IMMUTABLE_STRICT;

-- Availability: 1.2.2
CREATEFUNCTION ST_collect (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_collect_garray'
	LANGUAGE 'C' _IMMUTABLE_STRICT;

-- Deprecation in 1.2.3
CREATE AGGREGATE MemGeomUnion (
	basetype = geometry,
	sfunc = geomunion,
	stype = geometry
	);

-- Availability: 1.2.2
CREATE AGGREGATE ST_MemUnion (
	basetype = geometry,
	sfunc = ST_union,
	stype = geometry
	);

--
-- pgis_abs
-- Container type to hold the ArrayBuildState pointer as it passes through
-- the geometry array accumulation aggregate.
--
CREATE FUNCTION pgis_abs_in(cstring)
    RETURNS pgis_abs
    AS 'MODULE_PATHNAME'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE FUNCTION pgis_abs_out(pgis_abs)
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
CREATE FUNCTION pgis_geometry_accum_transfn(pgis_abs, geometry)
    RETURNS pgis_abs
    AS 'MODULE_PATHNAME' LANGUAGE 'C';
    
-- Availability: 1.4.0
CREATE FUNCTION pgis_geometry_accum_finalfn(pgis_abs)
    RETURNS geometry[]
    AS 'MODULE_PATHNAME' LANGUAGE 'C';

-- Availability: 1.4.0
CREATE FUNCTION pgis_geometry_union_finalfn(pgis_abs)
    RETURNS geometry
    AS 'MODULE_PATHNAME' LANGUAGE 'C';
    
-- Availability: 1.4.0
CREATE FUNCTION pgis_geometry_collect_finalfn(pgis_abs)
    RETURNS geometry
    AS 'MODULE_PATHNAME' LANGUAGE 'C';
    
-- Availability: 1.4.0
CREATE FUNCTION pgis_geometry_polygonize_finalfn(pgis_abs)
    RETURNS geometry
    AS 'MODULE_PATHNAME' LANGUAGE 'C';
    
-- Availability: 1.4.0
CREATE FUNCTION pgis_geometry_makeline_finalfn(pgis_abs)
    RETURNS geometry
    AS 'MODULE_PATHNAME' LANGUAGE 'C';
    
-- Deprecation in: 1.2.3
CREATE AGGREGATE accum (
    BASETYPE = geometry,
    SFUNC = pgis_geometry_accum_transfn,
    STYPE = pgis_abs,
    FINALFUNC = pgis_geometry_accum_finalfn
    );

-- Availability: 1.2.2
CREATE AGGREGATE ST_Accum (
    BASETYPE = geometry,
    SFUNC = pgis_geometry_accum_transfn,
    STYPE = pgis_abs,
    FINALFUNC = pgis_geometry_accum_finalfn
    );

-- TO BE REMOVED BEFORE RELEASE
CREATE AGGREGATE accum_old (
	sfunc = ST_geom_accum,
	basetype = geometry,
	stype = geometry[]
	);

-- TO BE REMOVED BEFORE RELEASE
CREATE AGGREGATE ST_accum_old (
	sfunc = ST_geom_accum,
	basetype = geometry,
	stype = geometry[]
	);

-- Deprecation in 1.2.3
CREATEFUNCTION unite_garray (geometry[])
	RETURNS geometry
        AS 'MODULE_PATHNAME', 'pgis_union_geometry_array'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable); 

-- Deprecation in 1.4.0
CREATEFUNCTION ST_unite_garray (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME','pgis_union_geometry_array'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable); 

CREATEFUNCTION ST_Union (geometry[])
	RETURNS geometry
	AS 'MODULE_PATHNAME','pgis_union_geometry_array'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable); 

-- TO BE REMOVED BEFORE RELEASE
CREATE AGGREGATE GeomUnion_Old (
	sfunc = geom_accum,
	basetype = geometry,
	stype = geometry[],
	finalfunc = ST_unite_garray
	);

-- TO BE REMOVED BEFORE RELEASE
CREATE AGGREGATE ST_Union_Old (
	sfunc = ST_geom_accum,
	basetype = geometry,
	stype = geometry[],
	finalfunc = ST_unite_garray
	);
	
CREATE AGGREGATE ST_Union (
    BASETYPE = geometry,
    SFUNC = pgis_geometry_accum_transfn,
    STYPE = pgis_abs,
	FINALFUNC = pgis_geometry_union_finalfn
	);

-- Deprecation in 1.2.3
CREATE AGGREGATE collect (
    BASETYPE = geometry,
    SFUNC = pgis_geometry_accum_transfn,
    STYPE = pgis_abs,
	FINALFUNC = pgis_geometry_collect_finalfn
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
--	finalfunc = polygonize_garray
	);

-- Availability: 1.2.2
CREATE AGGREGATE ST_Polygonize (
    BASETYPE = geometry,
    SFUNC = pgis_geometry_accum_transfn,
    STYPE = pgis_abs,
	FINALFUNC = pgis_geometry_polygonize_finalfn
--	finalfunc = polygonize_garray
	);

-- Deprecation in 1.2.3
CREATE AGGREGATE makeline (
    BASETYPE = geometry,
    SFUNC = pgis_geometry_accum_transfn,
    STYPE = pgis_abs,
	FINALFUNC = pgis_geometry_makeline_finalfn
--	finalfunc = makeline_garray
	);

-- Availability: 1.2.2
CREATE AGGREGATE ST_MakeLine (
    BASETYPE = geometry,
    SFUNC = pgis_geometry_accum_transfn,
    STYPE = pgis_abs,
	FINALFUNC = pgis_geometry_makeline_finalfn
--	finalfunc = ST_makeline_garray
	);



--------------------------------------------------------------------------------

-- Deprecation in 1.2.3
CREATEFUNCTION relate(geometry,geometry)
   RETURNS text
   AS 'MODULE_PATHNAME','relate_full'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_relate(geometry,geometry)
   RETURNS text
   AS 'MODULE_PATHNAME','relate_full'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION relate(geometry,geometry,text)
   RETURNS boolean
   AS 'MODULE_PATHNAME','relate_pattern'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: relate(geometry,geometry,text)
CREATEFUNCTION ST_Relate(geometry,geometry,text)
    RETURNS boolean
    AS 'MODULE_PATHNAME','relate_pattern'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION disjoint(geometry,geometry)
   RETURNS boolean
   AS 'MODULE_PATHNAME'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);
   
-- PostGIS equivalent function: disjoint(geometry,geometry)
CREATEFUNCTION ST_Disjoint(geometry,geometry)
    RETURNS boolean
    AS 'MODULE_PATHNAME','disjoint'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION touches(geometry,geometry)
   RETURNS boolean
   AS 'MODULE_PATHNAME'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: touches(geometry,geometry)
CREATEFUNCTION _ST_Touches(geometry,geometry)
    RETURNS boolean
    AS 'MODULE_PATHNAME','touches'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
-- Inlines index magic
CREATEFUNCTION ST_Touches(geometry,geometry)
    RETURNS boolean
    AS 'SELECT $1 && $2 AND _ST_Touches($1,$2)'
    LANGUAGE 'SQL' _IMMUTABLE; -- WITH (iscachable);

-- Availability: 1.3.4
CREATEFUNCTION _ST_DWithin(geometry,geometry,float8)
    RETURNS boolean
    AS 'MODULE_PATHNAME', 'LWGEOM_dwithin'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_DWithin(geometry, geometry, float8)
    RETURNS boolean
    AS 'SELECT $1 && ST_Expand($2,$3) AND $2 && ST_Expand($1,$3) AND _ST_DWithin($1, $2, $3)'
    LANGUAGE 'SQL' _IMMUTABLE; -- WITH (iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION intersects(geometry,geometry)
   RETURNS boolean
   AS 'MODULE_PATHNAME'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: intersects(geometry,geometry)
CREATEFUNCTION _ST_Intersects(geometry,geometry)
    RETURNS boolean
    AS 'MODULE_PATHNAME','intersects'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
-- Inlines index magic
CREATEFUNCTION ST_Intersects(geometry,geometry)
    RETURNS boolean
    AS 'SELECT $1 && $2 AND _ST_Intersects($1,$2)'
    LANGUAGE 'SQL' _IMMUTABLE; -- WITH (iscachable);
-- Deprecation in 1.2.3
CREATEFUNCTION crosses(geometry,geometry)
   RETURNS boolean
   AS 'MODULE_PATHNAME'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: crosses(geometry,geometry)
CREATEFUNCTION _ST_Crosses(geometry,geometry)
    RETURNS boolean
    AS 'MODULE_PATHNAME','crosses'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
-- Inlines index magic
CREATEFUNCTION ST_Crosses(geometry,geometry)
    RETURNS boolean
    AS 'SELECT $1 && $2 AND _ST_Crosses($1,$2)'
    LANGUAGE 'SQL' _IMMUTABLE; -- WITH (iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION within(geometry,geometry)
   RETURNS boolean
   AS 'MODULE_PATHNAME'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: within(geometry,geometry)
CREATEFUNCTION _ST_Within(geometry,geometry)
    RETURNS boolean
    AS 'MODULE_PATHNAME','within'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
-- Inlines index magic
CREATEFUNCTION ST_Within(geometry,geometry)
    RETURNS boolean
    AS 'SELECT $1 && $2 AND _ST_Within($1,$2)'
    LANGUAGE 'SQL' _IMMUTABLE; -- WITH (iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION Contains(geometry,geometry)
   RETURNS boolean
   AS 'MODULE_PATHNAME'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: contains(geometry,geometry)
CREATEFUNCTION _ST_Contains(geometry,geometry)
    RETURNS boolean
    AS 'MODULE_PATHNAME','contains'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
-- Inlines index magic
CREATEFUNCTION ST_Contains(geometry,geometry)
    RETURNS boolean
    AS 'SELECT $1 && $2 AND _ST_Contains($1,$2)'
    LANGUAGE 'SQL' _IMMUTABLE; -- WITH (iscachable);

#if POSTGIS_GEOS_VERSION >= 30
-- Availability: 1.2.2
CREATEFUNCTION _ST_CoveredBy(geometry,geometry)
   RETURNS boolean
   AS 'MODULE_PATHNAME', 'coveredby'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_CoveredBy(geometry,geometry)
   RETURNS boolean
   AS 'SELECT $1 && $2 AND _ST_CoveredBy($1,$2)'
   LANGUAGE 'SQL' _IMMUTABLE; -- WITH(iscachable);

-- Availability: 1.2.2
CREATEFUNCTION _ST_Covers(geometry,geometry)
   RETURNS boolean
   AS 'MODULE_PATHNAME', 'covers'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
-- Inlines index magic
CREATEFUNCTION ST_Covers(geometry,geometry)
   RETURNS boolean
   AS 'SELECT $1 && $2 AND _ST_Covers($1,$2)'
   LANGUAGE 'SQL' _IMMUTABLE; -- WITH (iscachable);
#endif

#if POSTGIS_GEOS_VERSION >= 31
-- Availability: 1.4.0
CREATEFUNCTION _ST_ContainsProperly(geometry,geometry)
    RETURNS boolean
    AS 'MODULE_PATHNAME','containsproperly'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.4.0
-- Inlines index magic
CREATEFUNCTION ST_ContainsProperly(geometry,geometry)
    RETURNS boolean
    AS 'SELECT $1 && $2 AND _ST_ContainsProperly($1,$2)'
    LANGUAGE 'SQL' _IMMUTABLE; -- WITH (iscachable);
#endif

-- Deprecation in 1.2.3
CREATEFUNCTION overlaps(geometry,geometry)
   RETURNS boolean
   AS 'MODULE_PATHNAME'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: overlaps(geometry,geometry)
CREATEFUNCTION _ST_Overlaps(geometry,geometry)
    RETURNS boolean
    AS 'MODULE_PATHNAME','overlaps'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
-- Inlines index magic
CREATEFUNCTION ST_Overlaps(geometry,geometry)
    RETURNS boolean
    AS 'SELECT $1 && $2 AND _ST_Overlaps($1,$2)'
    LANGUAGE 'SQL' _IMMUTABLE; -- WITH (iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION IsValid(geometry)
   RETURNS boolean
   AS 'MODULE_PATHNAME', 'isvalid'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: IsValid(geometry)
-- TODO: change null returns to true
CREATEFUNCTION ST_IsValid(geometry)
    RETURNS boolean
    AS 'MODULE_PATHNAME', 'isvalid'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION GEOSnoop(geometry)
   RETURNS geometry
   AS 'MODULE_PATHNAME', 'GEOSnoop'
   LANGUAGE 'C' _VOLATILE_STRICT; -- WITH (isstrict,iscachable);

-- This is also available w/out GEOS 
CREATEFUNCTION Centroid(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: Centroid(geometry)
CREATEFUNCTION ST_Centroid(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'centroid'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION IsRing(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: IsRing(geometry)
CREATEFUNCTION ST_IsRing(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'isring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION PointOnSurface(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: PointOnSurface(geometry)
CREATEFUNCTION ST_PointOnSurface(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'pointonsurface'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION IsSimple(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'issimple'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: IsSimple(geometry)	
CREATEFUNCTION ST_IsSimple(geometry)
        RETURNS boolean
        AS 'MODULE_PATHNAME', 'issimple'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION Equals(geometry,geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME','geomequals'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: Equals(geometry,geometry)
CREATEFUNCTION ST_Equals(geometry,geometry)
    RETURNS boolean
    AS 'MODULE_PATHNAME','geomequals'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-----------------------------------------------------------------------
-- SVG OUTPUT
-----------------------------------------------------------------------
-- Deprecation in 1.2.3
CREATEFUNCTION AsSVG(geometry,int4,int4)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','assvg_geometry'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_AsSVG(geometry,int4,int4)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','assvg_geometry'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION AsSVG(geometry,int4)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','assvg_geometry'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_AsSVG(geometry,int4)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','assvg_geometry'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION AsSVG(geometry)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','assvg_geometry'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_AsSVG(geometry)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','assvg_geometry'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-----------------------------------------------------------------------
-- GML OUTPUT
-----------------------------------------------------------------------
-- _ST_AsGML(version, geom, precision, option)
CREATEFUNCTION _ST_AsGML(int4, geometry, int4, int4)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asGML'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- AsGML(geom, precision) / version=2
-- Deprecation in 1.2.3
CREATEFUNCTION AsGML(geometry, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML(2, $1, $2, 0)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_AsGML(geometry, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML(2, $1, $2, 0)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- AsGML(geom) / precision=15 version=2
-- Deprecation in 1.2.3
CREATEFUNCTION AsGML(geometry)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML(2, $1, 15, 0)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_AsGML(geometry)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML(2, $1, 15, 0)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- ST_AsGML(version, geom) / precision=15 version=2
-- Availability: 1.3.2
CREATEFUNCTION ST_AsGML(int4, geometry)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML($1, $2, 15, 0)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- ST_AsGML(version, geom, precision)
-- Availability: 1.3.2
CREATEFUNCTION ST_AsGML(int4, geometry, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML($1, $2, $3, 0)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- ST_AsGML (geom, precision, option) / version=2
-- Availability: 1.4.0
CREATEFUNCTION ST_AsGML(geometry, int4, int4)
        RETURNS TEXT
        AS 'SELECT _ST_AsGML(2, $1, $2, $3)'
        LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- ST_AsGML(version, geom, precision, option)
-- Availability: 1.4.0
CREATEFUNCTION ST_AsGML(int4, geometry, int4, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML($1, $2, $3, $4)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-----------------------------------------------------------------------
-- KML OUTPUT
-----------------------------------------------------------------------
-- _ST_AsKML(version, geom, precision)
CREATEFUNCTION _ST_AsKML(int4, geometry, int4)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asKML'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- AsKML(geom, precision) / version=2
-- Deprecation in 1.2.3
CREATEFUNCTION AsKML(geometry, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsKML(2, transform($1,4326), $2)' 
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_AsKML(geometry, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsKML(2, ST_Transform($1,4326), $2)' 
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- AsKML(geom) / precision=15 version=2
-- Deprecation in 1.2.3
CREATEFUNCTION AsKML(geometry) 
	RETURNS TEXT
	AS 'SELECT _ST_AsKML(2, transform($1,4326), 15)' 
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- AsKML(version, geom, precision)
-- Deprecation in 1.2.3
CREATEFUNCTION AsKML(int4, geometry, int4) 
	RETURNS TEXT
	AS 'SELECT _ST_AsKML($1, transform($2,4326), $3)' 
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_AsKML(geometry)
	RETURNS TEXT
	AS 'SELECT _ST_AsKML(2, ST_Transform($1,4326), 15)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- ST_AsKML(version, geom) / precision=15 version=2
-- Availability: 1.3.2
CREATEFUNCTION ST_AsKML(int4, geometry)
	RETURNS TEXT
	AS 'SELECT _ST_AsKML($1, ST_Transform($2,4326), 15)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- ST_AsKML(version, geom, precision)
-- Availability: 1.3.2
CREATEFUNCTION ST_AsKML(int4, geometry, int4) 
	RETURNS TEXT
	AS 'SELECT _ST_AsKML($1, ST_Transform($2,4326), $3)' 
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-----------------------------------------------------------------------
-- GEOJSON OUTPUT
-- Availability: 1.3.4
-----------------------------------------------------------------------
-- _ST_AsGeoJson(version, geom, precision, options)
CREATEFUNCTION _ST_AsGeoJson(int4, geometry, int4, int4)
        RETURNS TEXT
        AS 'MODULE_PATHNAME','LWGEOM_asGeoJson'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- ST_AsGeoJson(geom, precision) / version=1 options=0
CREATEFUNCTION ST_AsGeoJson(geometry, int4)
        RETURNS TEXT
        AS 'SELECT _ST_AsGeoJson(1, $1, $2, 0)'
        LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- ST_AsGeoJson(geom) / precision=15 version=1 options=0
CREATEFUNCTION ST_AsGeoJson(geometry)
        RETURNS TEXT
        AS 'SELECT _ST_AsGeoJson(1, $1, 15, 0)'
        LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- ST_AsGeoJson(version, geom) / precision=15 options=0
CREATEFUNCTION ST_AsGeoJson(int4, geometry)
        RETURNS TEXT
        AS 'SELECT _ST_AsGeoJson($1, $2, 15, 0)'
        LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- ST_AsGeoJson(version, geom, precision) / options=0
CREATEFUNCTION ST_AsGeoJson(int4, geometry, int4)
        RETURNS TEXT
        AS 'SELECT _ST_AsGeoJson($1, $2, $3, 0)'
        LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- ST_AsGeoJson(geom, precision, options) / version=1
CREATEFUNCTION ST_AsGeoJson(geometry, int4, int4)
        RETURNS TEXT
        AS 'SELECT _ST_AsGeoJson(1, $1, $2, $3)'
        LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- ST_AsGeoJson(version, geom, precision,options)
CREATEFUNCTION ST_AsGeoJson(int4, geometry, int4, int4)
        RETURNS TEXT
        AS 'SELECT _ST_AsGeoJson($1, $2, $3, $4)'
        LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

------------------------------------------------------------------------
-- GeoHash (geohash.org)
------------------------------------------------------------------------

-- Availability 1.4.0
CREATEFUNCTION ST_GeoHash(geometry, int4)
        RETURNS TEXT
	    AS 'MODULE_PATHNAME', 'ST_GeoHash'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrictl,iscachable);

-- Availability 1.4.0
CREATEFUNCTION ST_GeoHash(geometry)
        RETURNS TEXT
        AS 'SELECT ST_GeoHash($1, 0)'
        LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

------------------------------------------------------------------------
-- OGC defined
------------------------------------------------------------------------

-- Deprecation in 1.2.3
CREATEFUNCTION NumPoints(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_numpoints_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- PostGIS equivalent function: NumPoints(geometry)
CREATEFUNCTION ST_NumPoints(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_numpoints_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION NumGeometries(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_numgeometries_collection'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- PostGIS equivalent function: NumGeometries(geometry)
CREATEFUNCTION ST_NumGeometries(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_numgeometries_collection'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION GeometryN(geometry,integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_geometryn_collection'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- PostGIS equivalent function: GeometryN(geometry)
CREATEFUNCTION ST_GeometryN(geometry,integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_geometryn_collection'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION Dimension(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME', 'LWGEOM_dimension'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- PostGIS equivalent function: Dimension(geometry)
CREATEFUNCTION ST_Dimension(geometry)
    RETURNS int4
    AS 'MODULE_PATHNAME', 'LWGEOM_dimension'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION ExteriorRing(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_exteriorring_polygon'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- PostGIS equivalent function: ExteriorRing(geometry)
CREATEFUNCTION ST_ExteriorRing(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_exteriorring_polygon'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION NumInteriorRings(geometry)
	RETURNS integer
	AS 'MODULE_PATHNAME','LWGEOM_numinteriorrings_polygon'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- PostGIS equivalent function: NumInteriorRings(geometry)
CREATEFUNCTION ST_NumInteriorRings(geometry)
	RETURNS integer
	AS 'MODULE_PATHNAME','LWGEOM_numinteriorrings_polygon'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION NumInteriorRing(geometry)
	RETURNS integer
	AS 'MODULE_PATHNAME','LWGEOM_numinteriorrings_polygon'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_NumInteriorRing(geometry)
	RETURNS integer
	AS 'MODULE_PATHNAME','LWGEOM_numinteriorrings_polygon'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION InteriorRingN(geometry,integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_interiorringn_polygon'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- PostGIS equivalent function: InteriorRingN(geometry)
CREATEFUNCTION ST_InteriorRingN(geometry,integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_interiorringn_polygon'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION GeometryType(geometry)
	RETURNS text
	AS 'MODULE_PATHNAME', 'LWGEOM_getTYPE'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Not quite equivalent to GeometryType
CREATEFUNCTION ST_GeometryType(geometry)
    RETURNS text
    AS $$ 
    DECLARE
        gtype text := geometrytype($1);
    BEGIN
        IF (gtype IN ('POINT', 'POINTM')) THEN
            gtype := 'Point';
        ELSIF (gtype IN ('LINESTRING', 'LINESTRINGM')) THEN
            gtype := 'LineString';
        ELSIF (gtype IN ('POLYGON', 'POLYGONM')) THEN
            gtype := 'Polygon';
        ELSIF (gtype IN ('MULTIPOINT', 'MULTIPOINTM')) THEN
            gtype := 'MultiPoint';
        ELSIF (gtype IN ('MULTILINESTRING', 'MULTILINESTRINGM')) THEN
            gtype := 'MultiLineString';
        ELSIF (gtype IN ('MULTIPOLYGON', 'MULTIPOLYGONM')) THEN
            gtype := 'MultiPolygon';
        ELSE
            gtype := 'Geometry';
        END IF;
        RETURN 'ST_' || gtype;
    END
$$
LANGUAGE 'plpgsql' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION PointN(geometry,integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_pointn_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- PostGIS equivalent function: PointN(geometry,integer)
CREATEFUNCTION ST_PointN(geometry,integer)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_pointn_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION X(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_x_point'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- PostGIS equivalent function: X(geometry)
CREATEFUNCTION ST_X(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_x_point'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION Y(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_y_point'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);
        
-- PostGIS equivalent function: Y(geometry)
CREATEFUNCTION ST_Y(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_y_point'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION Z(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_z_point'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- PostGIS equivalent function: Z(geometry)
CREATEFUNCTION SE_Z(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_z_point'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_Z(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_z_point'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION M(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_m_point'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Availability: 1.2.2
CREATEFUNCTION ST_M(geometry)
	RETURNS float8
	AS 'MODULE_PATHNAME','LWGEOM_m_point'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION StartPoint(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_startpoint_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- PostGIS equivalent function: StartPoint(geometry))
CREATEFUNCTION ST_StartPoint(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_startpoint_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION EndPoint(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_endpoint_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- PostGIS equivalent function: EndPoint(geometry))
CREATEFUNCTION ST_EndPoint(geometry)
	RETURNS geometry
	AS 'MODULE_PATHNAME', 'LWGEOM_endpoint_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION IsClosed(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'LWGEOM_isclosed_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- PostGIS equivalent function: IsClosed(geometry)
CREATEFUNCTION ST_IsClosed(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'LWGEOM_isclosed_linestring'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION IsEmpty(geometry)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'LWGEOM_isempty'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- PostGIS equivalent function: IsEmpty(geometry)
CREATEFUNCTION ST_IsEmpty(geometry)
    RETURNS boolean
    AS 'MODULE_PATHNAME', 'LWGEOM_isempty'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

-- Deprecation in 1.2.3
CREATEFUNCTION SRID(geometry) 
	RETURNS int4
	AS 'MODULE_PATHNAME','LWGEOM_getSRID'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: getSRID(geometry)
CREATEFUNCTION ST_SRID(geometry) 
    RETURNS int4
    AS 'MODULE_PATHNAME','LWGEOM_getSRID'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION SetSRID(geometry,int4) 
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_setSRID'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);	

-- Availability: 1.2.2
CREATEFUNCTION ST_SetSRID(geometry,int4) 
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_setSRID'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);	
	
-- Deprecation in 1.2.3
CREATEFUNCTION AsBinary(geometry)
	RETURNS bytea
	AS 'MODULE_PATHNAME','LWGEOM_asBinary'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: AsBinary(geometry)
CREATEFUNCTION ST_AsBinary(geometry)
    RETURNS bytea
    AS 'MODULE_PATHNAME','LWGEOM_asBinary'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION AsBinary(geometry,text)
	RETURNS bytea
	AS 'MODULE_PATHNAME','LWGEOM_asBinary'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_AsBinary(geometry,text)
	RETURNS bytea
	AS 'MODULE_PATHNAME','LWGEOM_asBinary'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION AsText(geometry)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asText'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: AsText(geometry)
CREATEFUNCTION ST_AsText(geometry)
    RETURNS TEXT
    AS 'MODULE_PATHNAME','LWGEOM_asText'
    LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION GeometryFromText(text)
        RETURNS geometry
        AS 'MODULE_PATHNAME','LWGEOM_from_text'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_GeometryFromText(text)
        RETURNS geometry
        AS 'MODULE_PATHNAME','LWGEOM_from_text'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION GeometryFromText(text, int4)
        RETURNS geometry
        AS 'MODULE_PATHNAME','LWGEOM_from_text'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_GeometryFromText(text, int4)
        RETURNS geometry
        AS 'MODULE_PATHNAME','LWGEOM_from_text'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION GeomFromText(text)
	RETURNS geometry AS 'SELECT geometryfromtext($1)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_GeomFromText(text)
        RETURNS geometry
        AS 'MODULE_PATHNAME','LWGEOM_from_text'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION GeomFromText(text, int4)
	RETURNS geometry AS 'SELECT geometryfromtext($1, $2)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: ST_GeometryFromText(text, int4)
CREATEFUNCTION ST_GeomFromText(text, int4)
        RETURNS geometry
        AS 'MODULE_PATHNAME','LWGEOM_from_text'
        LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION PointFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1)) = ''POINT''
	THEN GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_PointFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromText($1)) = ''POINT''
	THEN ST_GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION PointFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1, $2)) = ''POINT''
	THEN GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: PointFromText(text, int4)
-- TODO: improve this ... by not duplicating constructor time.
CREATEFUNCTION ST_PointFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromText($1, $2)) = ''POINT''
	THEN ST_GeomFromText($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION LineFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1)) = ''LINESTRING''
	THEN GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_LineFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromText($1)) = ''LINESTRING''
	THEN ST_GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION LineFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1, $2)) = ''LINESTRING''
	THEN GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: LineFromText(text, int4)
CREATEFUNCTION ST_LineFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1, $2)) = ''LINESTRING''
	THEN GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION LineStringFromText(text)
	RETURNS geometry
	AS 'SELECT LineFromText($1)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION LineStringFromText(text, int4)
	RETURNS geometry
	AS 'SELECT LineFromText($1, $2)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION PolyFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1)) = ''POLYGON''
	THEN GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_PolyFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromText($1)) = ''POLYGON''
	THEN ST_GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION PolyFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1, $2)) = ''POLYGON''
	THEN GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);
        
-- PostGIS equivalent function: ST_PolygonFromText(text, int4)
CREATEFUNCTION ST_PolyFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromText($1, $2)) = ''POLYGON''
	THEN ST_GeomFromText($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION PolygonFromText(text, int4)
	RETURNS geometry
	AS 'SELECT PolyFromText($1, $2)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_PolygonFromText(text, int4)
	RETURNS geometry
	AS 'SELECT PolyFromText($1, $2)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION PolygonFromText(text)
	RETURNS geometry
	AS 'SELECT PolyFromText($1)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_PolygonFromText(text)
	RETURNS geometry
	AS 'SELECT ST_PolyFromText($1)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MLineFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE
	WHEN geometrytype(GeomFromText($1, $2)) = ''MULTILINESTRING''
	THEN GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: MLineFromText(text, int4)
CREATEFUNCTION ST_MLineFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE
	WHEN geometrytype(GeomFromText($1, $2)) = ''MULTILINESTRING''
	THEN GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MLineFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1)) = ''MULTILINESTRING''
	THEN GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_MLineFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromText($1)) = ''MULTILINESTRING''
	THEN ST_GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MultiLineStringFromText(text)
	RETURNS geometry
	AS 'SELECT ST_MLineFromText($1)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_MultiLineStringFromText(text)
	RETURNS geometry
	AS 'SELECT ST_MLineFromText($1)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MultiLineStringFromText(text, int4)
	RETURNS geometry
	AS 'SELECT MLineFromText($1, $2)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_MultiLineStringFromText(text, int4)
	RETURNS geometry
	AS 'SELECT MLineFromText($1, $2)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MPointFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1,$2)) = ''MULTIPOINT''
	THEN GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: MPointFromText(text, int4)
CREATEFUNCTION ST_MPointFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1, $2)) = ''MULTIPOINT''
	THEN GeomFromText($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MPointFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1)) = ''MULTIPOINT''
	THEN GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_MPointFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromText($1)) = ''MULTIPOINT''
	THEN ST_GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MultiPointFromText(text, int4)
	RETURNS geometry
	AS 'SELECT MPointFromText($1, $2)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MultiPointFromText(text)
	RETURNS geometry
	AS 'SELECT MPointFromText($1)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_MultiPointFromText(text)
	RETURNS geometry
	AS 'SELECT ST_MPointFromText($1)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MultiPointFromText(text)
	RETURNS geometry
	AS 'SELECT MPointFromText($1)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_MultiPointFromText(text)
	RETURNS geometry
	AS 'SELECT MPointFromText($1)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MPolyFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1, $2)) = ''MULTIPOLYGON''
	THEN GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: MPolyFromText(text, int4)
CREATEFUNCTION ST_MPolyFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromText($1, $2)) = ''MULTIPOLYGON''
	THEN ST_GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MPolyFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromText($1)) = ''MULTIPOLYGON''
	THEN GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

--Availability: 1.2.2
CREATEFUNCTION ST_MPolyFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromText($1)) = ''MULTIPOLYGON''
	THEN ST_GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MultiPolygonFromText(text, int4)
	RETURNS geometry
	AS 'SELECT MPolyFromText($1, $2)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_MultiPolygonFromText(text, int4)
	RETURNS geometry
	AS 'SELECT MPolyFromText($1, $2)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MultiPolygonFromText(text)
	RETURNS geometry
	AS 'SELECT MPolyFromText($1)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_MultiPolygonFromText(text)
	RETURNS geometry
	AS 'SELECT MPolyFromText($1)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION GeomCollFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE
	WHEN geometrytype(GeomFromText($1, $2)) = ''GEOMETRYCOLLECTION''
	THEN GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_GeomCollFromText(text, int4)
	RETURNS geometry
	AS '
	SELECT CASE
	WHEN geometrytype(ST_GeomFromText($1, $2)) = ''GEOMETRYCOLLECTION''
	THEN ST_GeomFromText($1,$2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION GeomCollFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE
	WHEN geometrytype(GeomFromText($1)) = ''GEOMETRYCOLLECTION''
	THEN GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_GeomCollFromText(text)
	RETURNS geometry
	AS '
	SELECT CASE
	WHEN geometrytype(ST_GeomFromText($1)) = ''GEOMETRYCOLLECTION''
	THEN ST_GeomFromText($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION GeomFromWKB(bytea)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_from_WKB'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_GeomFromWKB(bytea)
	RETURNS geometry
	AS 'MODULE_PATHNAME','LWGEOM_from_WKB'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION GeomFromWKB(bytea, int)
	RETURNS geometry
	AS 'SELECT setSRID(GeomFromWKB($1), $2)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: GeomFromWKB(bytea, int)
CREATEFUNCTION ST_GeomFromWKB(bytea, int)
	RETURNS geometry
	AS 'SELECT ST_SetSRID(ST_GeomFromWKB($1), $2)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION PointFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''POINT''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: PointFromWKB(bytea, int)
CREATEFUNCTION ST_PointFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1, $2)) = ''POINT''
	THEN ST_GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION PointFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''POINT''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_PointFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1)) = ''POINT''
	THEN ST_GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION LineFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''LINESTRING''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: LineFromWKB(bytea, int)
CREATEFUNCTION ST_LineFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1, $2)) = ''LINESTRING''
	THEN ST_GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION LineFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''LINESTRING''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_LineFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1)) = ''LINESTRING''
	THEN ST_GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION LinestringFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''LINESTRING''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_LinestringFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1, $2)) = ''LINESTRING''
	THEN ST_GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION LinestringFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''LINESTRING''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_LinestringFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''LINESTRING''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION PolyFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''POLYGON''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: PolyFromWKB(text, int)
CREATEFUNCTION ST_PolyFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1, $2)) = ''POLYGON''
	THEN ST_GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION PolyFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''POLYGON''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_PolyFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1)) = ''POLYGON''
	THEN ST_GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION PolygonFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1,$2)) = ''POLYGON''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_PolygonFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1,$2)) = ''POLYGON''
	THEN ST_GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION PolygonFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''POLYGON''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_PolygonFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''POLYGON''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MPointFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1,$2)) = ''MULTIPOINT''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: MPointFromWKB(text, int)
CREATEFUNCTION ST_MPointFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''MULTIPOINT''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MPointFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''MULTIPOINT''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_MPointFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1)) = ''MULTIPOINT''
	THEN ST_GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MultiPointFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1,$2)) = ''MULTIPOINT''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_MultiPointFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1,$2)) = ''MULTIPOINT''
	THEN ST_GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MultiPointFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''MULTIPOINT''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_MultiPointFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1)) = ''MULTIPOINT''
	THEN ST_GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MultiLineFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''MULTILINESTRING''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION MultiLineFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''MULTILINESTRING''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MultiLineFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''MULTILINESTRING''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_MultiLineFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1)) = ''MULTILINESTRING''
	THEN ST_GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MLineFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''MULTILINESTRING''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: MLineFromWKB(text, int)
CREATEFUNCTION ST_MLineFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1, $2)) = ''MULTILINESTRING''
	THEN ST_GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MLineFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''MULTILINESTRING''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_MLineFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1)) = ''MULTILINESTRING''
	THEN ST_GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MPolyFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''MULTIPOLYGON''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- PostGIS equivalent function: MPolyFromWKB(bytea, int)
CREATEFUNCTION ST_MPolyFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1, $2)) = ''MULTIPOLYGON''
	THEN ST_GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MPolyFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''MULTIPOLYGON''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_MPolyFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1)) = ''MULTIPOLYGON''
	THEN ST_GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MultiPolyFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1, $2)) = ''MULTIPOLYGON''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_MultiPolyFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1, $2)) = ''MULTIPOLYGON''
	THEN ST_GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION MultiPolyFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(GeomFromWKB($1)) = ''MULTIPOLYGON''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_MultiPolyFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE WHEN geometrytype(ST_GeomFromWKB($1)) = ''MULTIPOLYGON''
	THEN ST_GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION GeomCollFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE
	WHEN geometrytype(GeomFromWKB($1, $2)) = ''GEOMETRYCOLLECTION''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_GeomCollFromWKB(bytea, int)
	RETURNS geometry
	AS '
	SELECT CASE
	WHEN geometrytype(GeomFromWKB($1, $2)) = ''GEOMETRYCOLLECTION''
	THEN GeomFromWKB($1, $2)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Deprecation in 1.2.3
CREATEFUNCTION GeomCollFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE
	WHEN geometrytype(GeomFromWKB($1)) = ''GEOMETRYCOLLECTION''
	THEN GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

-- Availability: 1.2.2
CREATEFUNCTION ST_GeomCollFromWKB(bytea)
	RETURNS geometry
	AS '
	SELECT CASE
	WHEN geometrytype(ST_GeomFromWKB($1)) = ''GEOMETRYCOLLECTION''
	THEN ST_GeomFromWKB($1)
	ELSE NULL END
	'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

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
CREATEFUNCTION BdPolyFromText(text, integer)
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
LANGUAGE 'plpgsql' _IMMUTABLE_STRICT; 

-- Availability: 1.2.2
CREATEFUNCTION ST_BdPolyFromText(text, integer)
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
LANGUAGE 'plpgsql' _IMMUTABLE_STRICT; 

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
CREATEFUNCTION BdMPolyFromText(text, integer)
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
LANGUAGE 'plpgsql' _IMMUTABLE_STRICT; 

-- Availability: 1.2.2
CREATEFUNCTION ST_BdMPolyFromText(text, integer)
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
LANGUAGE 'plpgsql' _IMMUTABLE_STRICT; 

#include "long_xact.sql.in"
#include "sqlmm.sql.in"

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
CREATEFUNCTION ST_CurveToLine(geometry, integer)
   RETURNS geometry
   AS 'MODULE_PATHNAME', 'LWGEOM_curve_segmentize'
   LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);
--
-- SQL-MM
--
-- ST_CurveToLine(Geometry geometry, SegmentsPerQuarter integer)
--
-- Converts a given geometry to a linear geometry.  Each curveed
-- geometry or segment is converted into a linear approximation using
-- the default value of 32 segments per quarter circle
CREATEFUNCTION ST_CurveToLine(geometry)
	RETURNS geometry AS 'SELECT ST_CurveToLine($1, 32)'
	LANGUAGE 'SQL' _IMMUTABLE_STRICT; -- WITH (isstrict,iscachable);

CREATEFUNCTION ST_HasArc(geometry)
 	RETURNS boolean
	AS 'MODULE_PATHNAME', 'LWGEOM_has_arc'
	LANGUAGE 'C' _IMMUTABLE_STRICT; -- WITH (isstrict);

CREATEFUNCTION ST_LineToCurve(geometry)
        RETURNS geometry
        AS 'MODULE_PATHNAME', 'LWGEOM_line_desegmentize'
        LANGUAGE 'C' _IMMUTABLE_STRICT; 
---------------------------------------------------------------
-- END
---------------------------------------------------------------


---------------------------------------------------------------
-- USER CONTRIUBUTED
---------------------------------------------------------------

-----------------------------------------------------------------------
-- ST_MinimumBoundingCircle(inputgeom geometry, segs_per_quarter integer) 
-----------------------------------------------------------------------
-- Returns the smallest circle polygon that can fully contain a geometry
-- Defaults to 48 segs per quarter to approximate a circle
-- Contributed by Bruce Rindahl
-- Availability: 1.4.0
-----------------------------------------------------------------------
CREATEFUNCTION ST_MinimumBoundingCircle(inputgeom geometry, segs_per_quarter integer)
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
COMMIT;

