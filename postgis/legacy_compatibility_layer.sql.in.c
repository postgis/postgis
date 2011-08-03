-- $Id: legacy.sql.in.c 7548 2011-07-02 08:58:38Z robe $
-- Legacy functions without chip functions --
#include "sqldefines.h"
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsBinary(geometry)
	RETURNS bytea
	AS 'MODULE_PATHNAME','LWGEOM_asBinary'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsBinary(geometry,text)
	RETURNS bytea
	AS 'MODULE_PATHNAME','LWGEOM_asBinary'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION AsText(geometry)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','LWGEOM_asText'
	LANGUAGE 'C' IMMUTABLE STRICT;	

-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Estimated_Extent(text,text,text) RETURNS box2d AS
#ifdef GSERIALIZED_ON
	'MODULE_PATHNAME', 'geometry_estimated_extent'
#else
	'MODULE_PATHNAME', 'LWGEOM_estimated_extent'
#endif
	LANGUAGE 'C' IMMUTABLE STRICT SECURITY DEFINER;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION Estimated_Extent(text,text) RETURNS box2d AS
#ifdef GSERIALIZED_ON
	'MODULE_PATHNAME', 'geometry_estimated_extent'
#else
	'MODULE_PATHNAME', 'LWGEOM_estimated_extent'
#endif
	LANGUAGE 'C' IMMUTABLE STRICT SECURITY DEFINER;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION ndims(geometry)
	RETURNS smallint
	AS 'MODULE_PATHNAME', 'LWGEOM_ndims'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Deprecation in 1.2.3
CREATE OR REPLACE FUNCTION SRID(geometry)
	RETURNS int4
	AS 'MODULE_PATHNAME','LWGEOM_get_srid'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
