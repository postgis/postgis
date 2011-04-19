-- $Id$
-- Chip legacy functions --
#include "sqldefines.h"
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
--- Deprecation in 1.5.0
CREATE OR REPLACE FUNCTION st_geometry(chip)
	RETURNS geometry
	AS 'MODULE_PATHNAME','CHIP_to_LWGEOM'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
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

------------------------------------------------
--Begin 3D functions --
------------------------------------------------

-- Renamed in 2.0.0 to ST_3DLength
CREATE OR REPLACE FUNCTION ST_Length3D(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_length_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Renamed in 2.0.0 to ST_3DLength_spheroid
CREATE OR REPLACE FUNCTION ST_Length_spheroid3D(geometry, spheroid)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME','LWGEOM_length_ellipsoid_linestring'
	LANGUAGE 'C' IMMUTABLE STRICT
	COST 100;
	
-- Renamed in 2.0.0 to ST_3DPerimeter
CREATE OR REPLACE FUNCTION ST_Perimeter3D(geometry)
	RETURNS FLOAT8
	AS 'MODULE_PATHNAME', 'LWGEOM_perimeter_poly'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Renamed in 2.0.0 to ST_3DMakeBox
CREATE OR REPLACE FUNCTION ST_MakeBox3D(geometry, geometry)
	RETURNS box3d
	AS 'MODULE_PATHNAME', 'BOX3D_construct'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Renamed in 2.0.0 to ST_3DExtent
CREATE AGGREGATE ST_Extent3D(
	sfunc = ST_combine_bbox,
	basetype = geometry,
	stype = box3d
	);
--END 3D functions--
