-- $Id: postgis_drop_before.sql.in.c 7896 2011-09-27 01:55:49Z robe $
-- These are functions where the argument names may have changed  --
-- so have to be dropped before upgrade can happen --
DROP FUNCTION IF EXISTS AddGeometryColumn(varchar,varchar,varchar,varchar,integer,varchar,integer,boolean);
