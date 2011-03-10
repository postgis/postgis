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
  topology_id integer;
  n int4;
  ret text;
BEGIN

  SELECT * FROM topology.topology WHERE name = atopology INTO STRICT rec;
  -- TODO: catch <no_rows> to give a nice error message
  topology_id := rec.id;

  ret := 'Topology ' || quote_ident(atopology) 
      || ' (' || rec.id || '), ';
  ret := ret || 'SRID ' || rec.srid || ', '
             || 'precision: ' || rec.precision || E'\n';

  EXECUTE 'SELECT count(node_id) FROM ' || quote_ident(atopology)
    || '.node ' INTO STRICT n;
  ret = ret || n || ' nodes, ';

  EXECUTE 'SELECT count(edge_id) FROM ' || quote_ident(atopology)
    || '.edge_data ' INTO STRICT n;
  ret = ret || n || ' edges, ';

  EXECUTE 'SELECT count(face_id) FROM ' || quote_ident(atopology)
    || '.face ' INTO STRICT n;
  ret = ret || n || ' faces, ';

  EXECUTE 'SELECT count(*) FROM (SELECT DISTINCT layer_id,topogeo_id FROM '
    || quote_ident(atopology) || '.relation ) foo ' INTO STRICT n;
  ret = ret || n || ' topogeoms in ';

  EXECUTE 'SELECT count(*) FROM (SELECT DISTINCT layer_id FROM '
    || quote_ident(atopology) || '.relation ) foo ' INTO STRICT n;
  ret = ret || n || ' layers' || E'\n';

  -- TODO: print informations about layers
  FOR rec IN SELECT * FROM topology.layer l
    WHERE l.topology_id = topology_id
    ORDER by layer_id
  LOOP
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

    EXECUTE 'SELECT count(*) FROM ( SELECT DISTINCT topogeo_id FROM '
      || quote_ident(atopology)
      || '.relation r WHERE r.layer_id = ' || rec.layer_id
      || ' ) foo ' INTO STRICT n;

    ret = ret || ' (' || rec.feature_type || E'), '
              || n || ' topogeoms' || E'\n';

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
  END LOOP;

  RETURN ret;
END
$$
LANGUAGE 'plpgsql' VOLATILE STRICT;

--} TopologySummary
