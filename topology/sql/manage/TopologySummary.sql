-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-- 
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.refractions.net
--
-- Copyright (C) 2011 Sandro Santilli <strk@keybit.net>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

--{
--  TopologySummary(name)
--
-- Print an overview about a topology
--
CREATE OR REPLACE FUNCTION topology.TopologySummary(atopology varchar)
RETURNS text
AS
$$
DECLARE
  rec RECORD;
  rec2 RECORD;
  var_topology_id integer;
  n int4;
  ret text;
BEGIN

  ret := 'Topology ' || quote_ident(atopology) ;

  BEGIN
    SELECT * FROM topology.topology WHERE name = atopology INTO STRICT rec;
    -- TODO: catch <no_rows> to give a nice error message
    var_topology_id := rec.id;

    ret := ret || ' (' || rec.id || '), ';
    ret := ret || 'SRID ' || rec.srid || ', '
               || 'precision ' || rec.precision;
    IF rec.hasz THEN ret := ret || ', has Z'; END IF;
    ret := ret || E'\n';
  EXCEPTION
    WHEN NO_DATA_FOUND THEN
      ret := ret || E' (X)\n';
  END;

  BEGIN

  BEGIN
    EXECUTE 'SELECT count(node_id) FROM ' || quote_ident(atopology)
      || '.node ' INTO STRICT n;
    ret = ret || n || ' nodes, ';
  EXCEPTION
    WHEN UNDEFINED_TABLE OR INVALID_SCHEMA_NAME THEN
      ret = ret || 'X nodes, ';
  END;

  BEGIN
    EXECUTE 'SELECT count(edge_id) FROM ' || quote_ident(atopology)
      || '.edge_data ' INTO STRICT n;
    ret = ret || n || ' edges, ';
  EXCEPTION
    WHEN UNDEFINED_TABLE OR INVALID_SCHEMA_NAME THEN
      ret = ret || 'X edges, ';
  END;

  BEGIN
    EXECUTE 'SELECT count(face_id) FROM ' || quote_ident(atopology)
      || '.face ' INTO STRICT n;
    ret = ret || n || ' faces, ';
  EXCEPTION
    WHEN UNDEFINED_TABLE OR INVALID_SCHEMA_NAME THEN
      ret = ret || 'X faces, ';
  END;

  BEGIN

    EXECUTE 'SELECT count(*) FROM (SELECT DISTINCT layer_id,topogeo_id FROM '
      || quote_ident(atopology) || '.relation ) foo ' INTO STRICT n;
    ret = ret || n || ' topogeoms in ';

    EXECUTE 'SELECT count(*) FROM (SELECT DISTINCT layer_id FROM '
      || quote_ident(atopology) || '.relation ) foo ' INTO STRICT n;
    ret = ret || n || ' layers' || E'\n';
  EXCEPTION
    WHEN UNDEFINED_TABLE OR INVALID_SCHEMA_NAME THEN
      ret = ret || 'X topogeoms in X layers' || E'\n';
  END;

  -- TODO: print informations about layers
  FOR rec IN SELECT * FROM topology.layer l
    WHERE l.topology_id = var_topology_id
    ORDER by layer_id
  LOOP -- {
    ret = ret || 'Layer ' || rec.layer_id || ', type ';
    CASE
      WHEN rec.feature_type = 1 THEN
        ret = ret || 'Puntal';
      WHEN rec.feature_type = 2 THEN
        ret = ret || 'Lineal';
      WHEN rec.feature_type = 3 THEN
        ret = ret || 'Polygonal';
      ELSE 
        ret = ret || '???';
    END CASE;

    ret = ret || ' (' || rec.feature_type || '), ';

    BEGIN

      EXECUTE 'SELECT count(*) FROM ( SELECT DISTINCT topogeo_id FROM '
        || quote_ident(atopology)
        || '.relation r WHERE r.layer_id = ' || rec.layer_id
        || ' ) foo ' INTO STRICT n;

      ret = ret || n || ' topogeoms' || E'\n';

    EXCEPTION WHEN UNDEFINED_TABLE THEN
      ret = ret || 'X topogeoms' || E'\n';
    END;

      IF rec.level > 0 THEN
        ret = ret || ' Hierarchy level ' || rec.level 
                  || ', child layer ' || rec.child_id || E'\n';
      END IF;

      ret = ret || ' Deploy: ';
      IF rec.feature_column != '' THEN
        ret = ret || quote_ident(rec.schema_name) || '.'
                  || quote_ident(rec.table_name) || '.'
                  || quote_ident(rec.feature_column)
                  || E'\n';
      ELSE
        ret = ret || E'NONE (detached)\n';
      END IF;

  END LOOP; -- }

  EXCEPTION
    WHEN INVALID_SCHEMA_NAME THEN
      ret = ret || E'\n- missing schema - ';
    WHEN OTHERS THEN
      RAISE EXCEPTION 'Got % (%)', SQLERRM, SQLSTATE;
  END;


  RETURN ret;
END
$$
LANGUAGE 'plpgsql' STABLE STRICT;

--} TopologySummary
