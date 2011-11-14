-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-- 
-- $Id: postgis_extension_helper.sql 7936 2011-11-12 14:33:23Z robe $
----
-- PostGIS - Spatial Types for PostgreSQL
-- http://www.postgis.org
--
-- Copyright (C) 2011 Regina Obe <lr@pcorp.us>
-- Copyright (C) 2005 Refractions Research Inc.
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- Author: Regina Obe <lr@pcorp.us>
--  
-- This is a suite of SQL helper functions for use during a PostGIS extension install/upgrade
-- The functions get uninstalled after the extention install/upgrade process
---------------------------
-- postgis_extension_remove_objects: This function removes objects of a particular class from an extension
-- this is needed because there is no ALTER EXTENSION DROP FUNCTION/AGGREGATE command
-- and we can't CREATE OR REPALCe functions whose signatures have changed and we can drop them if they are part of an extention
-- So we use this to remove it from extension first before we drop
CREATE OR REPLACE FUNCTION postgis_extension_remove_objects(param_extension text, param_type text)
  RETURNS boolean AS
$$
DECLARE 
	var_sql text := '';
	var_r record;
	var_result boolean := false;
	var_class text := '';
	var_is_aggregate boolean := false;
	var_sql_list text := '';
BEGIN
		var_class := CASE WHEN lower(param_type) = 'function' OR lower(param_type) = 'aggregate' THEN 'pg_proc' ELSE '' END; 
		var_is_aggregate := CASE WHEN lower(param_type) = 'aggregate' THEN true ELSE false END;
		var_sql_list := 'SELECT ''ALTER EXTENSION '' || e.extname || '' DROP '' || $3 || '' '' || COALESCE(proc.proname || ''('' || oidvectortypes(proc.proargtypes) || '')'',typ.typname, cd.relname, op.oprname, 
				cs.typname || '' AS '' || ct.typname || '') '', opcname, opfname) || '';'' AS remove_command
		FROM pg_depend As d INNER JOIN pg_extension As e
			ON d.refobjid = e.oid INNER JOIN pg_class As c ON
				c.oid = d.classid
				LEFT JOIN pg_proc AS proc ON proc.oid = d.objid
				LEFT JOIN pg_type AS typ ON typ.oid = d.objid
				LEFT JOIN pg_class As cd ON cd.oid = d.objid
				LEFT JOIN pg_operator As op ON op.oid = d.objid
				LEFT JOIN pg_cast AS ca ON ca.oid = d.objid
				LEFT JOIN pg_type AS cs ON ca.castsource = cs.oid
				LEFT JOIN pg_type AS ct ON ca.casttarget = ct.oid
				LEFT JOIN pg_opclass As oc ON oc.oid = d.objid
				LEFT JOIN pg_opfamily As ofa ON ofa.oid = d.objid
		WHERE d.deptype = ''e'' and e.extname = $1 and c.relname = $2 AND COALESCE(proc.proisagg, false) = $4
		ORDER BY item_type, item_name;';
		FOR var_r IN EXECUTE var_sql_list  USING param_extension, var_class, param_type, var_is_aggregate
        LOOP
            var_sql := var_sql || var_r.remove_command || ';';
        END LOOP;
        IF var_sql > '' THEN
            EXECUTE var_sql;
        END IF;
END;
$$
LANGUAGE plpgsql VOLATILE;
