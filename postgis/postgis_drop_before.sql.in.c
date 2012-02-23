-- $Id$
-- These are functions where the argument names may have changed  --
-- so have to be dropped before upgrade can happen --
DROP FUNCTION IF EXISTS AddGeometryColumn(varchar,varchar,varchar,varchar,integer,varchar,integer,boolean);
DROP FUNCTION IF EXISTS ST_MakeEnvelope(float8, float8, float8, float8);
--changed name of prec arg to be consistent with ST_AsGML/KML
DROP FUNCTION IF EXISTS ST_AsX3D(geometry, integer, integer); 
--changed name of arg: http://trac.osgeo.org/postgis/ticket/1606
DROP FUNCTION IF EXISTS UpdateGeometrySRID(varchar,varchar,varchar,varchar,integer);
