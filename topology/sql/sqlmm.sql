-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-- 
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.refractions.net
--
-- Copyright (C) 2010, 2011 Sandro Santilli <strk@keybit.net>
-- Copyright (C) 2005 Refractions Research Inc.
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- Author: Sandro Santilli <strk@keybit.net>
--  
-- 

--={ ----------------------------------------------------------------
--  SQL/MM block
--
--  This part contains function in the SQL/MM specification
--
---------------------------------------------------------------------

--
-- Type returned by ST_GetFaceEdges
--
CREATE TYPE topology.GetFaceEdges_ReturnType AS (
  sequence integer,
  edge integer
);


--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.5
--
--  ST_GetFaceEdges(atopology, aface)
--
--
-- 
CREATE OR REPLACE FUNCTION topology.ST_GetFaceEdges(toponame varchar, face_id integer)
  RETURNS SETOF topology.GetFaceEdges_ReturnType
AS
$$
DECLARE
  rec RECORD;
  bounds geometry;
  retrec topology.GetFaceEdges_ReturnType;
  n int;
  sql TEXT;
BEGIN
  --
  -- toponame and face_id are required
  -- 
  IF toponame IS NULL OR face_id IS NULL THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - null argument';
  END IF;

  IF toponame = '' THEN
        RAISE EXCEPTION 'SQL/MM Spatial exception - invalid topology name';
  END IF;

  n := 1;

  -- Construct the face geometry, then for each ring of each polygon:
  sql := 'SELECT (ST_DumpRings((ST_Dump(ST_ForceRHR('
    || 'ST_BuildArea(ST_Collect(geom))))).geom)).* FROM '
    || quote_ident(toponame) || '.edge_data WHERE left_face = '
    || face_id || ' OR right_face = ' || face_id;
  FOR rec IN EXECUTE sql 
  LOOP -- {

    -- Find the edges constituting its boundary
    bounds = ST_Boundary(rec.geom);


    sql := 'WITH er2 AS ( ' 
      || 'WITH er AS ( SELECT ' 
      || 'min(e.edge_id) over (), count(*) over () as cnt, e.edge_id, '
      || 'ST_Line_Locate_Point('
      || quote_literal(bounds::text)
      || ', ST_Line_Interpolate_Point(e.geom, 0.2)) as pos'
      || ', ST_Line_Locate_Point('
      || quote_literal(bounds::text)
      || ', ST_Line_Interpolate_Point(e.geom, 0.8)) as pos2 FROM '
      || quote_ident(toponame)
      || '.edge e WHERE ( e.left_face = ' || face_id
      || ' OR e.right_face = ' || face_id
      || ') AND ST_Covers('
      || quote_literal(bounds::text)
      || ', e.geom)';
    IF face_id = 0 THEN
      sql := sql || ' ORDER BY POS ASC) ';
    ELSE
      sql := sql || ' ORDER BY POS DESC) ';
    END IF;

    -- Reorder rows so to start with the one with smaller edge_id
    sql := sql || 'SELECT row_number() over () - 1 as rn, * FROM er ) '
               || 'SELECT *, ( rn + cnt - ( select rn FROM er2 WHERE edge_id = min ) ) % cnt AS reord FROM er2 ORDER BY reord';


    --RAISE DEBUG 'SQL: %', sql;

    FOR rec IN EXECUTE sql
    LOOP

      RAISE DEBUG 'rn:%, n:%, edg:%, cnt:%, min:%, reord:%',
         rec.rn, n, rec.edge_id, rec.cnt, rec.min, rec.reord;

      retrec.sequence = n;
      retrec.edge = rec.edge_id;

      IF face_id = 0 THEN
        -- if this edge goes in opposite direction to the
        --       ring bounds, make it with negative orientation
        IF rec.pos2 < rec.pos THEN -- edge goes in opposite direction
          retrec.edge = -retrec.edge;
        END IF;
      ELSE
        -- if this edge goes in same direction to the
        --       ring bounds, make it with negative orientation
        IF rec.pos2 > rec.pos THEN -- edge goes in same direction
          retrec.edge = -retrec.edge;
        END IF;
      END IF;

      RETURN NEXT retrec;

      n = n+1;

    END LOOP;
  END LOOP; -- }

  RETURN;
EXCEPTION
  WHEN INVALID_SCHEMA_NAME THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - invalid topology name';
END
$$
LANGUAGE 'plpgsql' VOLATILE;
--} ST_GetFaceEdges

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.10
--
--  ST_NewEdgeHeal(atopology, anedge, anotheredge)
--
-- Not in the specs:
-- * Refuses to heal two edges if any of the two is closed 
-- * Raise an exception when trying to heal an edge with itself
-- * Raise an exception if any TopoGeometry is defined by only one
--   of the two edges
-- * Update references in the Relation table.
-- 
CREATE OR REPLACE FUNCTION topology.ST_NewEdgeHeal(toponame varchar, e1id integer, e2id integer)
  RETURNS int
AS
$$
DECLARE
  e1rec RECORD;
  e2rec RECORD;
  rec RECORD;
  newedgeid int;
  commonnode int;
  caseno int;
  topoid int;
  sql text;
  e2sign int;
  eidary int[];
BEGIN
  --
  -- toponame and face_id are required
  -- 
  IF toponame IS NULL OR e1id IS NULL OR e2id IS NULL THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - null argument';
  END IF;

  -- NOT IN THE SPECS: see if the same edge is given twice..
  IF e1id = e2id THEN
    RAISE EXCEPTION 'Cannot heal edge % with itself, try with another', e1id;
  END IF;

  -- Get topology id
  BEGIN
    SELECT id FROM topology.topology
      INTO STRICT topoid WHERE name = toponame;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN
        RAISE EXCEPTION 'SQL/MM Spatial exception - invalid topology name';
  END;

  BEGIN
    EXECUTE 'SELECT * FROM ' || quote_ident(toponame)
      || '.edge_data WHERE edge_id = ' || e1id
      INTO STRICT e1rec;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN
        RAISE EXCEPTION 'SQL/MM Spatial exception - non-existent edge %', e1id;
      WHEN INVALID_SCHEMA_NAME THEN
        RAISE EXCEPTION 'SQL/MM Spatial exception - invalid topology name';
      WHEN UNDEFINED_TABLE THEN
        RAISE EXCEPTION 'corrupted topology "%" (missing edge_data table)',
          toponame;
  END;

  BEGIN
    EXECUTE 'SELECT * FROM ' || quote_ident(toponame)
      || '.edge_data WHERE edge_id = ' || e2id
      INTO STRICT e2rec;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN
        RAISE EXCEPTION 'SQL/MM Spatial exception - non-existent edge %', e2id;
    -- NOTE: checks for INVALID_SCHEMA_NAME or UNDEFINED_TABLE done before
  END;


  -- NOT IN THE SPECS: See if any of the two edges are closed.
  IF e1rec.start_node = e1rec.end_node THEN
    RAISE EXCEPTION 'Edge % is closed, cannot heal to edge %', e1id, e2id;
  END IF;
  IF e2rec.start_node = e2rec.end_node THEN
    RAISE EXCEPTION 'Edge % is closed, cannot heal to edge %', e2id, e1id;
  END IF;

  -- Find common node
  IF e1rec.end_node = e2rec.start_node THEN
    commonnode = e1rec.end_node;
    caseno = 1;
  ELSIF e1rec.end_node = e2rec.end_node THEN
    commonnode = e1rec.end_node;
    caseno = 2;
  ELSIF e1rec.start_node = e2rec.start_node THEN
    commonnode = e1rec.start_node;
    caseno = 3;
  ELSIF e1rec.start_node = e2rec.end_node THEN
    commonnode = e1rec.start_node;
    caseno = 4;
  ELSE
    RAISE EXCEPTION 'SQL/MM Spatial exception - non-connected edges';
  END IF;

  -- Check if any other edge is connected to the common node
  FOR rec IN EXECUTE 'SELECT edge_id FROM ' || quote_ident(toponame)
    || '.edge_data WHERE ( edge_id != ' || e1id
    || ' AND edge_id != ' || e2id || ') AND ( start_node = '
    || commonnode || ' OR end_node = ' || commonnode || ' )'
  LOOP
    RAISE EXCEPTION
      'SQL/MM Spatial exception - other edges connected (ie: %)', rec.edge_id;
  END LOOP;

  -- NOT IN THE SPECS:
  -- check if any topo_geom is defined only by one of the
  -- input edges. In such case there would be no way to adapt
  -- the definition in case of healing, so we'd have to bail out
  eidary = ARRAY[e1id, e2id];
  sql := 'SELECT t.* from ('
    || 'SELECT r.topogeo_id, r.layer_id'
    || ', l.schema_name, l.table_name, l.feature_column'
    || ', array_agg(abs(r.element_id)) as elems '
    || 'FROM topology.layer l INNER JOIN '
    || quote_ident(toponame)
    || '.relation r ON (l.layer_id = r.layer_id) '
    || 'WHERE l.level = 0 AND l.feature_type = 2 '
    || ' AND l.topology_id = ' || topoid
    || ' AND abs(r.element_id) IN (' || e1id || ',' || e2id || ') '
    || 'group by r.topogeo_id, r.layer_id, l.schema_name, l.table_name, '
    || ' l.feature_column ) t WHERE NOT t.elems @> '
    || quote_literal(eidary);
  --RAISE DEBUG 'SQL: %', sql;
  FOR rec IN EXECUTE sql LOOP
    RAISE EXCEPTION 'TopoGeom % in layer % (%.%.%) cannot be represented healing edges % and %',
          rec.topogeo_id, rec.layer_id,
          rec.schema_name, rec.table_name, rec.feature_column,
          e1id, e2id;
  END LOOP;

  -- Create new edge {
  rec := e1rec;
  rec.geom = ST_LineMerge(ST_Collect(e1rec.geom, e2rec.geom));
  IF caseno = 1 THEN -- e1.end = e2.start
    IF NOT ST_Equals(ST_StartPoint(rec.geom), ST_StartPoint(e1rec.geom)) THEN
      RAISE DEBUG 'caseno=1: LineMerge did not maintain startpoint';
      rec.geom = ST_Reverse(rec.geom);
    END IF;
    rec.end_node = e2rec.end_node;
    rec.next_left_edge = e2rec.next_left_edge;
    e2sign = 1;
  ELSIF caseno = 2 THEN -- e1.end = e2.end
    IF NOT ST_Equals(ST_StartPoint(rec.geom), ST_StartPoint(e1rec.geom)) THEN
      RAISE DEBUG 'caseno=2: LineMerge did not maintain startpoint';
      rec.geom = ST_Reverse(rec.geom);
    END IF;
    rec.end_node = e2rec.start_node;
    rec.next_left_edge = e2rec.next_right_edge;
    e2sign = -1;
  ELSIF caseno = 3 THEN -- e1.start = e2.start
    IF NOT ST_Equals(ST_EndPoint(rec.geom), ST_EndPoint(e1rec.geom)) THEN
      RAISE DEBUG 'caseno=4: LineMerge did not maintain endpoint';
      rec.geom = ST_Reverse(rec.geom);
    END IF;
    rec.start_node = e2rec.end_node;
    rec.next_right_edge = e2rec.next_left_edge;
    e2sign = -1;
  ELSIF caseno = 4 THEN -- e1.start = e2.end
    IF NOT ST_Equals(ST_EndPoint(rec.geom), ST_EndPoint(e1rec.geom)) THEN
      RAISE DEBUG 'caseno=4: LineMerge did not maintain endpoint';
      rec.geom = ST_Reverse(rec.geom);
    END IF;
    rec.start_node = e2rec.start_node;
    rec.next_right_edge = e2rec.next_right_edge;
    e2sign = 1;
  END IF;
  -- }

  -- Insert new edge {
  EXECUTE 'SELECT nextval(' || quote_literal(
      quote_ident(toponame) || '.edge_data_edge_id_seq'
    ) || ')' INTO STRICT newedgeid;
  EXECUTE 'INSERT INTO ' || quote_ident(toponame)
    || '.edge VALUES(' || newedgeid
    || ',' || rec.start_node
    || ',' || rec.end_node
    || ',' || rec.next_left_edge
    || ',' || rec.next_right_edge
    || ',' || rec.left_face
    || ',' || rec.right_face
    || ',' || quote_literal(rec.geom::text)
    || ')';
  -- End of new edge insertion }

  -- Update next_left_edge/next_right_edge for
  -- any edge having them still pointing at the edges being removed
  -- (e2id)
  --
  -- NOTE:
  -- *(next_XXX_edge/e2id) serves the purpose of extracting existing
  -- sign from the value, while *e2sign changes that sign again if we
  -- reverted edge2 direction
  --
  sql := 'UPDATE ' || quote_ident(toponame)
    || '.edge_data SET abs_next_left_edge = ' || newedgeid
    || ', next_left_edge = ' || e2sign*newedgeid
    || '*(next_left_edge/'
    || e2id || ')  WHERE abs_next_left_edge = ' || e2id;
  --RAISE DEBUG 'SQL: %', sql;
  EXECUTE sql;
  sql := 'UPDATE ' || quote_ident(toponame)
    || '.edge_data SET abs_next_right_edge = ' || newedgeid
    || ', next_right_edge = ' || e2sign*newedgeid
    || '*(next_right_edge/'
    || e2id || ') WHERE abs_next_right_edge = ' || e2id;
  --RAISE DEBUG 'SQL: %', sql;
  EXECUTE sql;

  -- New edge has the same direction as old edge 1
  sql := 'UPDATE ' || quote_ident(toponame)
    || '.edge_data SET abs_next_left_edge = ' || newedgeid
    || ', next_left_edge = ' || newedgeid
    || '*(next_left_edge/'
    || e1id || ')  WHERE abs_next_left_edge = ' || e1id;
  --RAISE DEBUG 'SQL: %', sql;
  EXECUTE sql;
  sql := 'UPDATE ' || quote_ident(toponame)
    || '.edge_data SET abs_next_right_edge = ' || newedgeid
    || ', next_right_edge = ' || newedgeid
    || '*(next_right_edge/'
    || e1id || ') WHERE abs_next_right_edge = ' || e1id;
  --RAISE DEBUG 'SQL: %', sql;
  EXECUTE sql;

  --
  -- NOT IN THE SPECS:
  -- Replace composition rows involving the two
  -- edges as one involving the new edge.
  -- It takes a DELETE and an UPDATE to do all
  sql := 'DELETE FROM ' || quote_ident(toponame)
    || '.relation r USING topology.layer l '
    || 'WHERE l.level = 0 AND l.feature_type = 2'
    || ' AND l.topology_id = ' || topoid
    || ' AND l.layer_id = r.layer_id AND abs(r.element_id) = '
    || e2id;
  --RAISE DEBUG 'SQL: %', sql;
  EXECUTE sql;
  sql := 'UPDATE ' || quote_ident(toponame)
    || '.relation r '
    || ' SET element_id = ' || newedgeid || '*(element_id/'
    || e1id
    || ') FROM topology.layer l WHERE l.level = 0 AND l.feature_type = 2'
    || ' AND l.topology_id = ' || topoid
    || ' AND l.layer_id = r.layer_id AND abs(r.element_id) = '
    || e1id
  ;
  RAISE DEBUG 'SQL: %', sql;
  EXECUTE sql;


  -- Delete both edges
  EXECUTE 'DELETE FROM ' || quote_ident(toponame)
    || '.edge_data WHERE edge_id = ' || e2id;
  EXECUTE 'DELETE FROM ' || quote_ident(toponame)
    || '.edge_data WHERE edge_id = ' || e1id;

  -- Delete the common node 
  BEGIN
    EXECUTE 'DELETE FROM ' || quote_ident(toponame)
            || '.node WHERE node_id = ' || commonnode;
    EXCEPTION
      WHEN UNDEFINED_TABLE THEN
        RAISE EXCEPTION 'corrupted topology "%" (missing node table)',
          toponame;
  END;

  RETURN newedgeid;
END
$$
LANGUAGE 'plpgsql' VOLATILE;
--} ST_NewEdgeHeal

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.11
--
--  ST_ModEdgeHeal(atopology, anedge, anotheredge)
--
-- Not in the specs:
-- * Returns the id of the node being removed
-- * Refuses to heal two edges if any of the two is closed 
-- * Raise an exception when trying to heal an edge with itself
-- * Raise an exception if any TopoGeometry is defined by only one
--   of the two edges
-- * Update references in the Relation table.
-- 
CREATE OR REPLACE FUNCTION topology.ST_ModEdgeHeal(toponame varchar, e1id integer, e2id integer)
  RETURNS int
AS
$$
DECLARE
  e1rec RECORD;
  e2rec RECORD;
  rec RECORD;
  commonnode int;
  caseno int;
  topoid int;
  sql text;
  e2sign int;
  eidary int[];
BEGIN
  --
  -- toponame and face_id are required
  -- 
  IF toponame IS NULL OR e1id IS NULL OR e2id IS NULL THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - null argument';
  END IF;

  -- NOT IN THE SPECS: see if the same edge is given twice..
  IF e1id = e2id THEN
    RAISE EXCEPTION 'Cannot heal edge % with itself, try with another', e1id;
  END IF;

  -- Get topology id
  BEGIN
    SELECT id FROM topology.topology
      INTO STRICT topoid WHERE name = toponame;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN
        RAISE EXCEPTION 'SQL/MM Spatial exception - invalid topology name';
  END;

  BEGIN
    EXECUTE 'SELECT * FROM ' || quote_ident(toponame)
      || '.edge_data WHERE edge_id = ' || e1id
      INTO STRICT e1rec;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN
        RAISE EXCEPTION 'SQL/MM Spatial exception - non-existent edge %', e1id;
      WHEN INVALID_SCHEMA_NAME THEN
        RAISE EXCEPTION 'SQL/MM Spatial exception - invalid topology name';
      WHEN UNDEFINED_TABLE THEN
        RAISE EXCEPTION 'corrupted topology "%" (missing edge_data table)',
          toponame;
  END;

  BEGIN
    EXECUTE 'SELECT * FROM ' || quote_ident(toponame)
      || '.edge_data WHERE edge_id = ' || e2id
      INTO STRICT e2rec;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN
        RAISE EXCEPTION 'SQL/MM Spatial exception - non-existent edge %', e2id;
    -- NOTE: checks for INVALID_SCHEMA_NAME or UNDEFINED_TABLE done before
  END;


  -- NOT IN THE SPECS: See if any of the two edges are closed.
  IF e1rec.start_node = e1rec.end_node THEN
    RAISE EXCEPTION 'Edge % is closed, cannot heal to edge %', e1id, e2id;
  END IF;
  IF e2rec.start_node = e2rec.end_node THEN
    RAISE EXCEPTION 'Edge % is closed, cannot heal to edge %', e2id, e1id;
  END IF;

  -- Find common node
  IF e1rec.end_node = e2rec.start_node THEN
    commonnode = e1rec.end_node;
    caseno = 1;
  ELSIF e1rec.end_node = e2rec.end_node THEN
    commonnode = e1rec.end_node;
    caseno = 2;
  ELSIF e1rec.start_node = e2rec.start_node THEN
    commonnode = e1rec.start_node;
    caseno = 3;
  ELSIF e1rec.start_node = e2rec.end_node THEN
    commonnode = e1rec.start_node;
    caseno = 4;
  ELSE
    RAISE EXCEPTION 'SQL/MM Spatial exception - non-connected edges';
  END IF;

  -- Check if any other edge is connected to the common node
  FOR rec IN EXECUTE 'SELECT edge_id FROM ' || quote_ident(toponame)
    || '.edge_data WHERE ( edge_id != ' || e1id
    || ' AND edge_id != ' || e2id || ') AND ( start_node = '
    || commonnode || ' OR end_node = ' || commonnode || ' )'
  LOOP
    RAISE EXCEPTION
      'SQL/MM Spatial exception - other edges connected (ie: %)', rec.edge_id;
  END LOOP;

  -- NOT IN THE SPECS:
  -- check if any topo_geom is defined only by one of the
  -- input edges. In such case there would be no way to adapt
  -- the definition in case of healing, so we'd have to bail out
  eidary = ARRAY[e1id, e2id];
  sql := 'SELECT t.* from ('
    || 'SELECT r.topogeo_id, r.layer_id'
    || ', l.schema_name, l.table_name, l.feature_column'
    || ', array_agg(abs(r.element_id)) as elems '
    || 'FROM topology.layer l INNER JOIN '
    || quote_ident(toponame)
    || '.relation r ON (l.layer_id = r.layer_id) '
    || 'WHERE l.level = 0 AND l.feature_type = 2 '
    || ' AND l.topology_id = ' || topoid
    || ' AND abs(r.element_id) IN (' || e1id || ',' || e2id || ') '
    || 'group by r.topogeo_id, r.layer_id, l.schema_name, l.table_name, '
    || ' l.feature_column ) t WHERE NOT t.elems @> '
    || quote_literal(eidary);
  --RAISE DEBUG 'SQL: %', sql;
  FOR rec IN EXECUTE sql LOOP
    RAISE EXCEPTION 'TopoGeom % in layer % (%.%.%) cannot be represented healing edges % and %',
          rec.topogeo_id, rec.layer_id,
          rec.schema_name, rec.table_name, rec.feature_column,
          e1id, e2id;
  END LOOP;

  -- Update data of the first edge {
  rec := e1rec;
  rec.geom = ST_LineMerge(ST_Collect(e1rec.geom, e2rec.geom));
  IF caseno = 1 THEN -- e1.end = e2.start
    IF NOT ST_Equals(ST_StartPoint(rec.geom), ST_StartPoint(e1rec.geom)) THEN
      RAISE DEBUG 'caseno=1: LineMerge did not maintain startpoint';
      rec.geom = ST_Reverse(rec.geom);
    END IF;
    rec.end_node = e2rec.end_node;
    rec.next_left_edge = e2rec.next_left_edge;
    e2sign = 1;
  ELSIF caseno = 2 THEN -- e1.end = e2.end
    IF NOT ST_Equals(ST_StartPoint(rec.geom), ST_StartPoint(e1rec.geom)) THEN
      RAISE DEBUG 'caseno=2: LineMerge did not maintain startpoint';
      rec.geom = ST_Reverse(rec.geom);
    END IF;
    rec.end_node = e2rec.start_node;
    rec.next_left_edge = e2rec.next_right_edge;
    e2sign = -1;
  ELSIF caseno = 3 THEN -- e1.start = e2.start
    IF NOT ST_Equals(ST_EndPoint(rec.geom), ST_EndPoint(e1rec.geom)) THEN
      RAISE DEBUG 'caseno=4: LineMerge did not maintain endpoint';
      rec.geom = ST_Reverse(rec.geom);
    END IF;
    rec.start_node = e2rec.end_node;
    rec.next_right_edge = e2rec.next_left_edge;
    e2sign = -1;
  ELSIF caseno = 4 THEN -- e1.start = e2.end
    IF NOT ST_Equals(ST_EndPoint(rec.geom), ST_EndPoint(e1rec.geom)) THEN
      RAISE DEBUG 'caseno=4: LineMerge did not maintain endpoint';
      rec.geom = ST_Reverse(rec.geom);
    END IF;
    rec.start_node = e2rec.start_node;
    rec.next_right_edge = e2rec.next_right_edge;
    e2sign = 1;
  END IF;
  EXECUTE 'UPDATE ' || quote_ident(toponame)
    || '.edge_data SET geom = ' || quote_literal(rec.geom::text)
    || ', start_node = ' || rec.start_node
    || ', end_node = ' || rec.end_node
    || ', next_left_edge = ' || rec.next_left_edge
    || ', abs_next_left_edge = ' || abs(rec.next_left_edge)
    || ', next_right_edge = ' || rec.next_right_edge
    || ', abs_next_right_edge = ' || abs(rec.next_right_edge)
    || ' WHERE edge_id = ' || e1id;
  -- End of first edge update }

  -- Update next_left_edge/next_right_edge for
  -- any edge having them still pointing at the edge being removed (e2id)
  --
  -- NOTE:
  -- *(next_XXX_edge/e2id) serves the purpose of extracting existing
  -- sign from the value, while *e2sign changes that sign again if we
  -- reverted edge2 direction
  --
  sql := 'UPDATE ' || quote_ident(toponame)
    || '.edge_data SET abs_next_left_edge = ' || e1id
    || ', next_left_edge = ' || e2sign*e1id
    || '*(next_left_edge/'
    || e2id || ')  WHERE abs_next_left_edge = ' || e2id;
  --RAISE DEBUG 'SQL: %', sql;
  EXECUTE sql;
  sql := 'UPDATE ' || quote_ident(toponame)
    || '.edge_data SET abs_next_right_edge = ' || e1id
    || ', next_right_edge = ' || e2sign*e1id
    || '*(next_right_edge/'
    || e2id || ') WHERE abs_next_right_edge = ' || e2id;
  --RAISE DEBUG 'SQL: %', sql;
  EXECUTE sql;

  -- Delete the second edge
  EXECUTE 'DELETE FROM ' || quote_ident(toponame)
    || '.edge_data WHERE edge_id = ' || e2id;

  -- Delete the common node 
  BEGIN
    EXECUTE 'DELETE FROM ' || quote_ident(toponame)
            || '.node WHERE node_id = ' || commonnode;
    EXCEPTION
      WHEN UNDEFINED_TABLE THEN
        RAISE EXCEPTION 'corrupted topology "%" (missing node table)',
          toponame;
  END;

  --
  -- NOT IN THE SPECS:
  -- Drop composition rows involving second
  -- edge, as the first edge took its space,
  -- and all affected TopoGeom have been previously checked
  -- for being composed by both edges.
  sql := 'DELETE FROM ' || quote_ident(toponame)
    || '.relation r USING topology.layer l '
    || 'WHERE l.level = 0 AND l.feature_type = 2'
    || ' AND l.topology_id = ' || topoid
    || ' AND l.layer_id = r.layer_id AND abs(r.element_id) = '
    || e2id;
  --RAISE DEBUG 'SQL: %', sql;
  EXECUTE sql;

  RETURN commonnode;
END
$$
LANGUAGE 'plpgsql' VOLATILE;
--} ST_ModEdgeHeal

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.14
--
--  ST_RemEdgeNewFace(atopology, anedge)
--
-- Not in the specs:
-- * Raise an exception if any TopoGeometry is defined by only one
--   of the two faces that will dissolve.
-- * Raise an exception if any TopoGeometry is defined by 
--   the edge being removed.
-- * Properly set containg_face on nodes that remains isolated by the drop
-- * Update containg_face for isolated nodes in the dissolved faces
-- * Update references in the Relation table
-- 
-- }{
CREATE OR REPLACE FUNCTION topology.ST_RemEdgeNewFace(toponame varchar, e1id integer)
  RETURNS int
AS
$$
DECLARE
  e1rec RECORD;
  rec RECORD;
  fidary int[];
  topoid int;
  sql text;
  newfaceid int;
  newfacecreated bool;
  elink int;
BEGIN
  --
  -- toponame and face_id are required
  -- 
  IF toponame IS NULL OR e1id IS NULL THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - null argument';
  END IF;

  -- Get topology id
  BEGIN
    SELECT id FROM topology.topology
      INTO STRICT topoid WHERE name = toponame;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN
        RAISE EXCEPTION 'SQL/MM Spatial exception - invalid topology name';
  END;

  BEGIN
    EXECUTE 'SELECT * FROM ' || quote_ident(toponame)
      || '.edge_data WHERE edge_id = ' || e1id
      INTO STRICT e1rec;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN
        RAISE EXCEPTION 'SQL/MM Spatial exception - non-existent edge %', e1id;
      WHEN INVALID_SCHEMA_NAME THEN
        RAISE EXCEPTION 'SQL/MM Spatial exception - invalid topology name';
      WHEN UNDEFINED_TABLE THEN
        RAISE EXCEPTION 'corrupted topology "%" (missing edge_data table)',
          toponame;
  END;

  -- Check that no TopoGeometry references the edge being removed
  sql := 'SELECT r.topogeo_id, r.layer_id'
      || ', l.schema_name, l.table_name, l.feature_column '
      || 'FROM topology.layer l INNER JOIN '
      || quote_ident(toponame)
      || '.relation r ON (l.layer_id = r.layer_id) '
      || 'WHERE l.level = 0 AND l.feature_type = 2 '
      || ' AND l.topology_id = ' || topoid
      || ' AND abs(r.element_id) = ' || e1id ;
  RAISE DEBUG 'Checking TopoGeometry definitions: %', sql;
  FOR rec IN EXECUTE sql LOOP
    RAISE EXCEPTION 'TopoGeom % in layer % (%.%.%) cannot be represented dropping edge %',
            rec.topogeo_id, rec.layer_id,
            rec.schema_name, rec.table_name, rec.feature_column,
            e1id;
  END LOOP;

  -- Update next_left_edge and next_right_edge face
  -- for all edges bounding the new face
  RAISE NOTICE 'Updating next_{right,left}_face of ring edges...';

  -- TODO: reduce the following to 2 UPDATE rather than 4

  -- Update next_left_edge of previous edges in left face -- {

  elink := e1rec.next_left_edge;

  sql := 'UPDATE ' || quote_ident(toponame)
    || '.edge_data SET next_left_edge = '
    || elink
    || ', abs_next_left_edge = '
    || abs(elink)
    || ' WHERE next_left_edge < 0 AND abs(next_left_edge) = '
    || e1id;
  RAISE DEBUG 'next_left_edge update: %', sql;
  EXECUTE sql;

  -- If the edge being removed links to self,
  -- we use the other face
  IF e1rec.abs_next_right_edge = e1rec.edge_id THEN
    elink := e1rec.next_left_edge;
  ELSE
    elink := e1rec.next_right_edge;
  END IF;

  sql := 'UPDATE ' || quote_ident(toponame)
    || '.edge_data SET next_left_edge = '
    || elink
    || ', abs_next_left_edge = '
    || abs(elink)
    || ' WHERE next_left_edge > 0 AND abs(next_left_edge) = '
    || e1id;
  RAISE DEBUG 'next_left_edge update: %', sql;
  EXECUTE sql;

  -- }

  -- Update next_right_edge of previous edges in right face -- {

  elink := e1rec.next_left_edge;

  sql := 'UPDATE ' || quote_ident(toponame)
    || '.edge_data SET next_right_edge = '
    || elink
    || ', abs_next_right_edge = '
    || abs(elink)
    || ' WHERE next_right_edge < 0 AND abs(next_right_edge) = '
    || e1id;
  RAISE DEBUG 'next_right_edge update: %', sql;
  EXECUTE sql;

  -- If the edge being removed links to self,
  -- we use the other face
  IF e1rec.abs_next_right_edge = e1rec.edge_id THEN
    elink := e1rec.next_left_edge;
  ELSE
    elink := e1rec.next_right_edge;
  END IF;

  sql := 'UPDATE ' || quote_ident(toponame)
    || '.edge_data SET next_right_edge = '
    || elink
    || ', abs_next_right_edge = '
    || abs(elink)
    || ' WHERE next_right_edge > 0 AND abs(next_right_edge) = '
    || e1id;
  RAISE DEBUG 'next_right_edge update: %', sql;
  EXECUTE sql;

  -- }

  IF e1rec.left_face = e1rec.right_face THEN -- {

    RAISE NOTICE 'Deletion of edge % affects no face',
                    e1rec.edge_id;

    newfaceid := e1rec.left_face; -- TODO: or what should we return ?
    newfacecreated := false;

  ELSE -- }{

    RAISE NOTICE 'Deletion of edge % joins faces % and %',
                    e1rec.edge_id, e1rec.left_face, e1rec.right_face;

    -- NOT IN THE SPECS:
    -- check if any topo_geom is defined only by one of the
    -- joined faces. In such case there would be no way to adapt
    -- the definition in case of healing, so we'd have to bail out
    --
    -- We do this only when no universal face is involved
    -- (no surface can be defined by universal face)
    -- 
    IF e1rec.left_face != 0 AND e1rec.right_face != 0
    THEN -- {
      fidary = ARRAY[e1rec.left_face, e1rec.right_face];
      sql := 'SELECT t.* from ('
        || 'SELECT r.topogeo_id, r.layer_id'
        || ', l.schema_name, l.table_name, l.feature_column'
        || ', array_agg(r.element_id) as elems '
        || 'FROM topology.layer l INNER JOIN '
        || quote_ident(toponame)
        || '.relation r ON (l.layer_id = r.layer_id) '
        || 'WHERE l.level = 0 AND l.feature_type = 3 '
        || ' AND l.topology_id = ' || topoid
        || ' AND r.element_id IN (' || e1rec.left_face || ',' || e1rec.right_face || ') '
        || 'group by r.topogeo_id, r.layer_id, l.schema_name, l.table_name, '
        || ' l.feature_column ) t WHERE NOT t.elems @> '
        || quote_literal(fidary);
      RAISE DEBUG 'SQL: %', sql;
      FOR rec IN EXECUTE sql LOOP
        RAISE EXCEPTION 'TopoGeom % in layer % (%.%.%) cannot be represented healing faces % and %',
              rec.topogeo_id, rec.layer_id,
              rec.schema_name, rec.table_name, rec.feature_column,
              e1rec.right_face, e1rec.left_face;
      END LOOP;
    END IF; -- }

    IF e1rec.left_face = 0 OR e1rec.right_face = 0 THEN -- {

      --
      -- We won't add any new face, but rather let the universe
      -- flood the removed face.
      --

      newfaceid := 0;
      newfacecreated := false;

    ELSE -- }{

      --
      -- Insert the new face 
      --

      sql := 'SELECT nextval(' || quote_literal(
          quote_ident(toponame) || '.face_face_id_seq'
        ) || ')';

      EXECUTE sql INTO STRICT newfaceid;
      newfacecreated := true;

      sql := 'INSERT INTO '
        || quote_ident(toponame)
        || '.face(face_id, mbr) SELECT '
        -- face_id
        || newfaceid  || ', '
        -- minimum bounding rectangle is the union of the old faces mbr
        -- (doing this without GEOS would be faster)
        || 'ST_Envelope(ST_Union(mbr)) FROM '
        || quote_ident(toponame)
        || '.face WHERE face_id IN (' 
        || e1rec.left_face || ',' || e1rec.right_face 
        || ')';
      RAISE DEBUG 'SQL: %', sql;
      EXECUTE sql;

    END IF; -- }

    -- Update left_face for all edges still referencing old faces
    sql := 'UPDATE ' || quote_ident(toponame)
      || '.edge_data SET left_face = ' || newfaceid 
      || ' WHERE left_face IN ('
      || e1rec.left_face || ',' || e1rec.right_face 
      || ')';
    RAISE DEBUG 'left_face update: %', sql;
    EXECUTE sql;

    -- Update right_face for all edges still referencing old faces
    sql := 'UPDATE ' || quote_ident(toponame)
      || '.edge_data SET right_face = ' || newfaceid 
      || ' WHERE right_face IN ('
      || e1rec.left_face || ',' || e1rec.right_face 
      || ')';
    RAISE DEBUG 'right_face update: %', sql;
    EXECUTE sql;

    -- Update containing_face for all nodes still referencing old faces
    sql := 'UPDATE ' || quote_ident(toponame)
      || '.node SET containing_face = ' || newfaceid 
      || ' WHERE containing_face IN ('
      || e1rec.left_face || ',' || e1rec.right_face 
      || ')';
    RAISE DEBUG 'Isolated nodes update: %', sql;
    EXECUTE sql;

    -- NOT IN THE SPECS:
    -- Replace composition rows involving the two
    -- faces as one involving the new face.
    -- It takes a DELETE and an UPDATE to do all
    sql := 'DELETE FROM ' || quote_ident(toponame)
      || '.relation r USING topology.layer l '
      || 'WHERE l.level = 0 AND l.feature_type = 3'
      || ' AND l.topology_id = ' || topoid
      || ' AND l.layer_id = r.layer_id AND abs(r.element_id) = '
      || e1rec.left_face;
    RAISE DEBUG 'SQL: %', sql;
    EXECUTE sql;
    sql := 'UPDATE ' || quote_ident(toponame)
      || '.relation r '
      || ' SET element_id = ' || newfaceid 
      || ' FROM topology.layer l WHERE l.level = 0 AND l.feature_type = 3'
      || ' AND l.topology_id = ' || topoid
      || ' AND l.layer_id = r.layer_id AND r.element_id = '
      || e1rec.right_face;
    RAISE DEBUG 'SQL: %', sql;
    EXECUTE sql;

  END IF; -- } two faces healed...

  -- Delete the edge
  sql := 'DELETE FROM ' || quote_ident(toponame)
    || '.edge_data WHERE edge_id = ' || e1id;
  RAISE DEBUG 'Edge deletion: %', sql;
  EXECUTE sql;

  -- Check if any of the edge nodes remains isolated, 
  -- set containing_face  = newfaceid in that case
  sql := 'UPDATE ' || quote_ident(toponame)
    || '.node n SET containing_face = ' || newfaceid
    || ' WHERE node_id IN ('
    || e1rec.start_node || ','
    || e1rec.end_node || ') AND NOT EXISTS (SELECT edge_id FROM '
    || quote_ident(toponame)
    || '.edge_data WHERE start_node = n.node_id OR end_node = n.node_id)';
  RAISE DEBUG 'Checking for nodes left isolated: %', sql;
  EXECUTE sql;

  IF e1rec.right_face != e1rec.left_face THEN -- {

    -- Delete left face, if not universe
    IF e1rec.left_face != 0 THEN
      sql := 'DELETE FROM ' || quote_ident(toponame)
        || '.face WHERE face_id = ' || e1rec.left_face; 
      RAISE DEBUG 'Left face deletion: %', sql;
      EXECUTE sql;
    END IF;

    -- Delete right face, if not universe
    IF e1rec.right_face != 0
    THEN
      sql := 'DELETE FROM ' || quote_ident(toponame)
        || '.face WHERE face_id = ' || e1rec.right_face;
      RAISE DEBUG 'Right face deletion: %', sql;
      EXECUTE sql;
    END IF;

  END IF; -- }

  IF newfacecreated THEN
    RETURN newfaceid;
  ELSE
    RETURN NULL; -- -newfaceid;
  END IF;
END
$$
LANGUAGE 'plpgsql' VOLATILE;
--} ST_RemEdgeNewFace

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.15
--
--  ST_RemEdgeModFace(atopology, anedge)
--
-- Not in the specs:
-- * Raise an exception if any TopoGeometry is defined by only one
--   of the two faces that will dissolve.
-- * Raise an exception if any TopoGeometry is defined by 
--   the edge being removed.
-- * Properly set containg_face on nodes that remains isolated by the drop
-- * Update containg_face for isolated nodes in the dissolved faces
-- * Update references in the Relation table
-- * Return id of the face taking up the removed edge space
--
-- }{
CREATE OR REPLACE FUNCTION topology.ST_RemEdgeModFace(toponame varchar, e1id integer)
  RETURNS int
AS
$$
DECLARE
  e1rec RECORD;
  rec RECORD;
  fidary int[];
  topoid int;
  sql text;
  floodfaceid int;
  elink int;
BEGIN
  --
  -- toponame and face_id are required
  -- 
  IF toponame IS NULL OR e1id IS NULL THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - null argument';
  END IF;

  -- Get topology id
  BEGIN
    SELECT id FROM topology.topology
      INTO STRICT topoid WHERE name = toponame;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN
        RAISE EXCEPTION 'SQL/MM Spatial exception - invalid topology name';
  END;

  BEGIN
    EXECUTE 'SELECT * FROM ' || quote_ident(toponame)
      || '.edge_data WHERE edge_id = ' || e1id
      INTO STRICT e1rec;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN
        RAISE EXCEPTION 'SQL/MM Spatial exception - non-existent edge %', e1id;
      WHEN INVALID_SCHEMA_NAME THEN
        RAISE EXCEPTION 'SQL/MM Spatial exception - invalid topology name';
      WHEN UNDEFINED_TABLE THEN
        RAISE EXCEPTION 'corrupted topology "%" (missing edge_data table)',
          toponame;
  END;

  -- Check that no TopoGeometry references the edge being removed
  sql := 'SELECT r.topogeo_id, r.layer_id'
      || ', l.schema_name, l.table_name, l.feature_column '
      || 'FROM topology.layer l INNER JOIN '
      || quote_ident(toponame)
      || '.relation r ON (l.layer_id = r.layer_id) '
      || 'WHERE l.level = 0 AND l.feature_type = 2 '
      || ' AND l.topology_id = ' || topoid
      || ' AND abs(r.element_id) = ' || e1id ;
  RAISE DEBUG 'Checking TopoGeometry definitions: %', sql;
  FOR rec IN EXECUTE sql LOOP
    RAISE EXCEPTION 'TopoGeom % in layer % (%.%.%) cannot be represented dropping edge %',
            rec.topogeo_id, rec.layer_id,
            rec.schema_name, rec.table_name, rec.feature_column,
            e1id;
  END LOOP;

  -- Update next_left_edge and next_right_edge face
  -- for all edges bounding the new face
  RAISE NOTICE 'Updating next_{right,left}_face of ring edges...';

  -- TODO: reduce the following to 2 UPDATE rather than 4

  -- Update next_left_edge of previous edges in left face -- {

  elink := e1rec.next_left_edge;

  sql := 'UPDATE ' || quote_ident(toponame)
    || '.edge_data SET next_left_edge = '
    || elink
    || ', abs_next_left_edge = '
    || abs(elink)
    || ' WHERE next_left_edge < 0 AND abs(next_left_edge) = '
    || e1id;
  RAISE DEBUG 'next_left_edge update: %', sql;
  EXECUTE sql;

  -- If the edge being removed links to self,
  -- we use the other face
  IF e1rec.abs_next_right_edge = e1rec.edge_id THEN
    elink := e1rec.next_left_edge;
  ELSE
    elink := e1rec.next_right_edge;
  END IF;

  sql := 'UPDATE ' || quote_ident(toponame)
    || '.edge_data SET next_left_edge = '
    || elink
    || ', abs_next_left_edge = '
    || abs(elink)
    || ' WHERE next_left_edge > 0 AND abs(next_left_edge) = '
    || e1id;
  RAISE DEBUG 'next_left_edge update: %', sql;
  EXECUTE sql;

  -- }

  -- Update next_right_edge of previous edges in right face -- {

  elink := e1rec.next_left_edge;

  sql := 'UPDATE ' || quote_ident(toponame)
    || '.edge_data SET next_right_edge = '
    || elink
    || ', abs_next_right_edge = '
    || abs(elink)
    || ' WHERE next_right_edge < 0 AND abs(next_right_edge) = '
    || e1id;
  RAISE DEBUG 'next_right_edge update: %', sql;
  EXECUTE sql;

  -- If the edge being removed links to self,
  -- we use the other face
  IF e1rec.abs_next_right_edge = e1rec.edge_id THEN
    elink := e1rec.next_left_edge;
  ELSE
    elink := e1rec.next_right_edge;
  END IF;

  sql := 'UPDATE ' || quote_ident(toponame)
    || '.edge_data SET next_right_edge = '
    || elink
    || ', abs_next_right_edge = '
    || abs(elink)
    || ' WHERE next_right_edge > 0 AND abs(next_right_edge) = '
    || e1id;
  RAISE DEBUG 'next_right_edge update: %', sql;
  EXECUTE sql;

  -- }

  IF e1rec.left_face = e1rec.right_face THEN -- {

    RAISE NOTICE 'Deletion of edge % affects no face',
                    e1rec.edge_id;

    floodfaceid = e1rec.left_face; 

  ELSE -- }{

    RAISE NOTICE 'Deletion of edge % joins faces % and %',
                    e1rec.edge_id, e1rec.left_face, e1rec.right_face;

    -- NOT IN THE SPECS:
    -- check if any topo_geom is defined only by one of the
    -- joined faces. In such case there would be no way to adapt
    -- the definition in case of healing, so we'd have to bail out
    --
    -- We do this only when no universal face is involved
    -- (no surface can be defined by universal face)
    -- 
    IF e1rec.left_face != 0 AND e1rec.right_face != 0
    THEN -- {
      fidary = ARRAY[e1rec.left_face, e1rec.right_face];
      sql := 'SELECT t.* from ('
        || 'SELECT r.topogeo_id, r.layer_id'
        || ', l.schema_name, l.table_name, l.feature_column'
        || ', array_agg(r.element_id) as elems '
        || 'FROM topology.layer l INNER JOIN '
        || quote_ident(toponame)
        || '.relation r ON (l.layer_id = r.layer_id) '
        || 'WHERE l.level = 0 AND l.feature_type = 3 '
        || ' AND l.topology_id = ' || topoid
        || ' AND r.element_id IN (' || e1rec.left_face || ',' || e1rec.right_face || ') '
        || 'group by r.topogeo_id, r.layer_id, l.schema_name, l.table_name, '
        || ' l.feature_column ) t WHERE NOT t.elems @> '
        || quote_literal(fidary);
      RAISE DEBUG 'SQL: %', sql;
      FOR rec IN EXECUTE sql LOOP
        RAISE EXCEPTION 'TopoGeom % in layer % (%.%.%) cannot be represented healing faces % and %',
              rec.topogeo_id, rec.layer_id,
              rec.schema_name, rec.table_name, rec.feature_column,
              e1rec.right_face, e1rec.left_face;
      END LOOP;
    END IF; -- }

    IF e1rec.left_face = 0 OR e1rec.right_face = 0 THEN -- {

      --
      -- We won't add any new face, but rather let the universe
      -- flood the removed face.
      --

      floodfaceid = 0;

    ELSE -- }{

      -- we choose right face as the face that will remain
      -- to be symmetric with ST_AddEdgeModFace 
      floodfaceid = e1rec.right_face;

      sql := 'UPDATE '
        || quote_ident(toponame)
        || '.face SET mbr = (SELECT '
        -- minimum bounding rectangle is the union of the old faces mbr
        -- (doing this without GEOS would be faster)
        || 'ST_Envelope(ST_Union(mbr)) FROM '
        || quote_ident(toponame)
        || '.face WHERE face_id IN (' 
        || e1rec.left_face || ',' || e1rec.right_face 
        || ') ) WHERE face_id = ' || floodfaceid ;
      RAISE DEBUG 'SQL: %', sql;
      EXECUTE sql;

    END IF; -- }

    -- Update left_face for all edges still referencing old faces
    sql := 'UPDATE ' || quote_ident(toponame)
      || '.edge_data SET left_face = ' || floodfaceid 
      || ' WHERE left_face IN ('
      || e1rec.left_face || ',' || e1rec.right_face 
      || ')';
    RAISE DEBUG 'left_face update: %', sql;
    EXECUTE sql;

    -- Update right_face for all edges still referencing old faces
    sql := 'UPDATE ' || quote_ident(toponame)
      || '.edge_data SET right_face = ' || floodfaceid 
      || ' WHERE right_face IN ('
      || e1rec.left_face || ',' || e1rec.right_face 
      || ')';
    RAISE DEBUG 'right_face update: %', sql;
    EXECUTE sql;

    -- Update containing_face for all nodes still referencing old faces
    sql := 'UPDATE ' || quote_ident(toponame)
      || '.node SET containing_face = ' || floodfaceid 
      || ' WHERE containing_face IN ('
      || e1rec.left_face || ',' || e1rec.right_face 
      || ')';
    RAISE DEBUG 'Isolated nodes update: %', sql;
    EXECUTE sql;

    -- NOT IN THE SPECS:
    -- Replace composition rows involving the two
    -- faces as one involving the new face.
    -- It takes a single DELETE to do that.
    sql := 'DELETE FROM ' || quote_ident(toponame)
      || '.relation r USING topology.layer l '
      || 'WHERE l.level = 0 AND l.feature_type = 3'
      || ' AND l.topology_id = ' || topoid
      || ' AND l.layer_id = r.layer_id AND abs(r.element_id) IN ('
      || e1rec.left_face || ',' || e1rec.right_face
      || ') AND abs(r.element_id) != '
      || floodfaceid; -- could be optimized..
    RAISE DEBUG 'SQL: %', sql;
    EXECUTE sql;

  END IF; -- } two faces healed...

  -- Delete the edge
  sql := 'DELETE FROM ' || quote_ident(toponame)
    || '.edge_data WHERE edge_id = ' || e1id;
  RAISE DEBUG 'Edge deletion: %', sql;
  EXECUTE sql;

  -- Check if any of the edge nodes remains isolated, 
  -- set containing_face  = floodfaceid in that case
  sql := 'UPDATE ' || quote_ident(toponame)
    || '.node n SET containing_face = ' || floodfaceid
    || ' WHERE node_id IN ('
    || e1rec.start_node || ','
    || e1rec.end_node || ') AND NOT EXISTS (SELECT edge_id FROM '
    || quote_ident(toponame)
    || '.edge_data WHERE start_node = n.node_id OR end_node = n.node_id)';
  RAISE DEBUG 'Checking for nodes left isolated: %', sql;
  EXECUTE sql;

  IF e1rec.right_face != e1rec.left_face THEN -- {

    -- Delete left face, if not universe and not "flood" face
    IF e1rec.left_face != 0 AND e1rec.left_face != floodfaceid
    THEN
      sql := 'DELETE FROM ' || quote_ident(toponame)
        || '.face WHERE face_id = ' || e1rec.left_face; 
      RAISE DEBUG 'Left face deletion: %', sql;
      EXECUTE sql;
    END IF;

    -- Delete right face, if not universe and not "flood" face
    IF e1rec.right_face != 0 AND e1rec.right_face != floodfaceid
    THEN
      sql := 'DELETE FROM ' || quote_ident(toponame)
        || '.face WHERE face_id = ' || e1rec.right_face;
      RAISE DEBUG 'Right face deletion: %', sql;
      EXECUTE sql;
    END IF;

  END IF; -- }

  RETURN floodfaceid;
END
$$
LANGUAGE 'plpgsql' VOLATILE;
--} ST_RemEdgeModFace


--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.16
--
--  ST_GetFaceGeometry(atopology, aface)
-- 
CREATE OR REPLACE FUNCTION topology.ST_GetFaceGeometry(toponame varchar, aface integer)
  RETURNS GEOMETRY AS
$$
DECLARE
  rec RECORD;
BEGIN

  --
  -- toponame and aface are required
  -- 
  IF toponame IS NULL OR aface IS NULL THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - null argument';
  END IF;

  IF toponame = '' THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - invalid topology name';
  END IF;

  IF aface = 0 THEN
    RAISE EXCEPTION
      'SQL/MM Spatial exception - universal face has no geometry';
  END IF;

  --
  -- Construct face 
  -- 
  BEGIN
    FOR rec IN EXECUTE 'SELECT ST_BuildArea(ST_Collect(geom)) FROM '
      || quote_ident(toponame)
      || '.edge_data WHERE left_face = ' || aface || 
      ' OR right_face = ' || aface 
    LOOP
      RETURN rec.st_buildarea;
    END LOOP;
  EXCEPTION
    WHEN INVALID_SCHEMA_NAME THEN
      RAISE EXCEPTION 'SQL/MM Spatial exception - invalid topology name';
    WHEN UNDEFINED_TABLE THEN
      RAISE EXCEPTION 'corrupted topology "%" (missing edge_data table)',
        toponame;
  END;


  --
  -- No face found
  -- 
  RAISE EXCEPTION 'SQL/MM Spatial exception - non-existent face.';
END
$$
LANGUAGE 'plpgsql' VOLATILE;
--} ST_GetFaceGeometry


--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.1 
--
--  ST_AddIsoNode(atopology, aface, apoint)
--
CREATE OR REPLACE FUNCTION topology.ST_AddIsoNode(varchar, integer, geometry)
  RETURNS INTEGER AS
$$
DECLARE
  atopology ALIAS FOR $1;
  aface ALIAS FOR $2;
  apoint ALIAS FOR $3;
  rec RECORD;
  nodeid integer;
    sql text;
    containingface integer;
BEGIN

  --
  -- Atopology and apoint are required
  -- 
  IF atopology IS NULL OR apoint IS NULL THEN
    RAISE EXCEPTION
     'SQL/MM Spatial exception - null argument';
  END IF;

  --
  -- Apoint must be a point
  --
  IF substring(geometrytype(apoint), 1, 5) != 'POINT'
  THEN
    RAISE EXCEPTION
     'SQL/MM Spatial exception - invalid point';
  END IF;

  --
  -- Check if a coincident node already exists
  -- 
  -- We use index AND x/y equality
  --
  FOR rec IN EXECUTE 'SELECT node_id FROM '
    || quote_ident(atopology) || '.node ' ||
    'WHERE geom && ' || quote_literal(apoint::text) || '::geometry'
    ||' AND ST_X(geom) = ST_X('||quote_literal(apoint::text)||'::geometry)'
    ||' AND ST_Y(geom) = ST_Y('||quote_literal(apoint::text)||'::geometry)'
  LOOP
    RAISE EXCEPTION
     'SQL/MM Spatial exception - coincident node';
  END LOOP;

  --
  -- Check if any edge crosses (intersects) this node
  -- I used _intersects_ here to include boundaries (endpoints)
  --
  FOR rec IN EXECUTE 'SELECT edge_id FROM '
    || quote_ident(atopology) || '.edge ' 
    || 'WHERE (geom && ' || quote_literal(apoint::text) ||'::geometry) '
    || ' AND ST_Intersects(geom, ' || quote_literal(apoint::text)
    || '::geometry)'
  LOOP
    RAISE EXCEPTION
    'SQL/MM Spatial exception - edge crosses node.';
  END LOOP;

  -- retrieve the face that contains (eventually) the point
  
  --
  -- first test is to check if there is inside an mbr (more fast)
  --
      sql := 'SELECT a.face_id FROM ' 
        || quote_ident(atopology) 
        || '.face as a WHERE '
        || '(a.mbr && ' || quote_literal(apoint::text)||'::geometry) '
        || 'LIMIT 1;';
--raise notice ' ==>%',sql;
  BEGIN
    EXECUTE sql INTO STRICT containingface;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN
        containingface := 0;
  END;

  IF containingface > 0 THEN
        --
        -- probably there is something so now check the exact test
        --    
    sql := 'SELECT e.face_id FROM ('
      || 'SELECT d.face_id,ST_BuildArea(ST_Union(geom)) as geom FROM ('
      || 'SELECT b.edge_id as edge_id,b.left_face as face_id,b.geom as geom FROM '
      || quote_ident(atopology) || '.edge_data as b,'
      || '(SELECT a.face_id FROM '
      || quote_ident(atopology) || '.face as a '
      || 'WHERE ST_Intersects(a.mbr,' || quote_literal(apoint::text)||'::geometry)=true'
      || ') as c '
      || 'WHERE (b.left_face = c.face_id) '
      || ' UNION ALL '
      || 'SELECT b.edge_id as edge_id, b.right_face as face_id, b.geom as geom FROM '
      || quote_ident(atopology) || '.edge_data as b,'
      || '(SELECT a.face_id FROM '
      || quote_ident(atopology) || '.face as a '
      || 'WHERE ST_Intersects(a.mbr,' || quote_literal(apoint::text)||'::geometry)=true'
      || ') as c '
      || 'WHERE (b.right_face = c.face_id) '
      || ') as d '
      || 'GROUP BY face_id '
      || ') as e '
      || 'WHERE ST_Intersects(e.geom, ' || quote_literal(apoint::text)||'::geometry)=true;';

        --raise notice ' ==> %',sql;
        BEGIN
            EXECUTE sql INTO STRICT containingface;
                EXCEPTION
                    WHEN NO_DATA_FOUND THEN
                        containingface = 0;
                    WHEN TOO_MANY_ROWS THEN
                        RAISE EXCEPTION 'Two or more faces found';
        END;
    END IF;

  --
  -- Verify that aface contains apoint
  --
    IF aface IS NOT NULL THEN
      -- if aface is 0 (UniverseFace) no check is done
        IF (aface <> containingface) THEN
            RAISE EXCEPTION
                'SQL/MM Spatial exception - not within face';
    END IF;
  END IF;
    -- passing last IF mean the containingface is the right value to use.

  --
  -- Get new node id from sequence
  --
  FOR rec IN EXECUTE 'SELECT nextval(''' ||
    atopology || '.node_node_id_seq'')'
  LOOP
    nodeid = rec.nextval;
  END LOOP;

  --
  -- Insert the new row
  --
  sql := 'INSERT INTO ' || quote_ident(atopology)
      || '.node(node_id, geom, containing_face) 
      VALUES('||nodeid||','||quote_literal(apoint::text)||
      ','||containingface||')';
  --raise notice 'insert: %',sql;

  EXECUTE sql;

  RETURN nodeid;
EXCEPTION
  WHEN INVALID_SCHEMA_NAME THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - invalid topology name';
END
$$
LANGUAGE 'plpgsql' VOLATILE;
--} ST_AddIsoNode

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.2 
--
--  ST_MoveIsoNode(atopology, anode, apoint)
--
CREATE OR REPLACE FUNCTION topology.ST_MoveIsoNode(character varying, integer, geometry)
  RETURNS text AS
$$
DECLARE
  atopology ALIAS FOR $1;
  anode ALIAS FOR $2;
  apoint ALIAS FOR $3;
  rec RECORD;
BEGIN

  --
  -- All arguments are required
  -- 
  IF atopology IS NULL OR anode IS NULL OR apoint IS NULL THEN
    RAISE EXCEPTION
     'SQL/MM Spatial exception - null argument';
  END IF;

  --
  -- Apoint must be a point
  --
  IF substring(geometrytype(apoint), 1, 5) != 'POINT'
  THEN
    RAISE EXCEPTION
     'SQL/MM Spatial exception - invalid point';
  END IF;

  --
  -- Check node isolation.
  -- 
  FOR rec IN EXECUTE 'SELECT edge_id FROM '
    || quote_ident(atopology) || '.edge ' ||
    ' WHERE start_node =  ' || anode ||
    ' OR end_node = ' || anode 
  LOOP
    RAISE EXCEPTION
     'SQL/MM Spatial exception - not isolated node';
  END LOOP;

  --
  -- Check if a coincident node already exists
  -- 
  -- We use index AND x/y equality
  --
  FOR rec IN EXECUTE 'SELECT node_id FROM '
    || quote_ident(atopology) || '.node ' ||
    'WHERE geom && ' || quote_literal(apoint::text) || '::geometry'
    ||' AND ST_X(geom) = ST_X('||quote_literal(apoint::text)||'::geometry)'
    ||' AND ST_Y(geom) = ST_Y('||quote_literal(apoint::text)||'::geometry)'
  LOOP
    RAISE EXCEPTION
     'SQL/MM Spatial exception - coincident node';
  END LOOP;

  --
  -- Check if any edge crosses (intersects) this node
  -- I used _intersects_ here to include boundaries (endpoints)
  --
  FOR rec IN EXECUTE 'SELECT edge_id FROM '
    || quote_ident(atopology) || '.edge ' 
    || 'WHERE geom && ' || quote_literal(apoint::text) 
    || ' AND ST_Intersects(geom, ' || quote_literal(apoint::text)
    || '::geometry)'
  LOOP
    RAISE EXCEPTION
    'SQL/MM Spatial exception - edge crosses node.';
  END LOOP;

  --
  -- Update node point
  --
  EXECUTE 'UPDATE ' || quote_ident(atopology) || '.node '
    || ' SET geom = ' || quote_literal(apoint::text) 
    || ' WHERE node_id = ' || anode;

  RETURN 'Isolated Node ' || anode || ' moved to location '
    || ST_X(apoint) || ',' || ST_Y(apoint);
END
$$
  LANGUAGE plpgsql VOLATILE;
--} ST_MoveIsoNode

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.3 
--
--  ST_RemoveIsoNode(atopology, anode)
--
CREATE OR REPLACE FUNCTION topology.ST_RemoveIsoNode(varchar, integer)
  RETURNS TEXT AS
$$
DECLARE
  atopology ALIAS FOR $1;
  anode ALIAS FOR $2;
  rec RECORD;
BEGIN

  --
  -- Atopology and apoint are required
  -- 
  IF atopology IS NULL OR anode IS NULL THEN
    RAISE EXCEPTION
     'SQL/MM Spatial exception - null argument';
  END IF;

  --
  -- Check node isolation.
  -- 
  FOR rec IN EXECUTE 'SELECT edge_id FROM '
    || quote_ident(atopology) || '.edge_data ' ||
    ' WHERE start_node =  ' || anode ||
    ' OR end_node = ' || anode 
  LOOP
    RAISE EXCEPTION
     'SQL/MM Spatial exception - not isolated node';
  END LOOP;

  EXECUTE 'DELETE FROM ' || quote_ident(atopology) || '.node '
    || ' WHERE node_id = ' || anode;

  RETURN 'Isolated node ' || anode || ' removed';
END
$$
LANGUAGE 'plpgsql' VOLATILE;
--} ST_RemoveIsoNode

--{
-- According to http://trac.osgeo.org/postgis/ticket/798
-- ST_RemoveIsoNode was renamed to ST_RemIsoNode in the final ISO
-- document
--
CREATE OR REPLACE FUNCTION topology.ST_RemIsoNode(varchar, integer)
  RETURNS TEXT AS
$$
  SELECT topology.ST_RemoveIsoNode($1, $2)
$$ LANGUAGE 'sql' VOLATILE;

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.7 
--
--  ST_RemoveIsoEdge(atopology, anedge)
--
CREATE OR REPLACE FUNCTION topology.ST_RemoveIsoEdge(varchar, integer)
  RETURNS TEXT AS
$$
DECLARE
  atopology ALIAS FOR $1;
  anedge ALIAS FOR $2;
  edge RECORD;
  rec RECORD;
  ok BOOL;
BEGIN

  --
  -- Atopology and anedge are required
  -- 
  IF atopology IS NULL OR anedge IS NULL THEN
    RAISE EXCEPTION
     'SQL/MM Spatial exception - null argument';
  END IF;

  --
  -- Check node existance
  -- 
  ok = false;
  FOR edge IN EXECUTE 'SELECT * FROM '
    || quote_ident(atopology) || '.edge_data ' ||
    ' WHERE edge_id =  ' || anedge
  LOOP
    ok = true;
  END LOOP;
  IF NOT ok THEN
    RAISE EXCEPTION
      'SQL/MM Spatial exception - non-existent edge';
  END IF;

  --
  -- Check node isolation
  -- 
  IF edge.left_face != edge.right_face THEN
    RAISE EXCEPTION
      'SQL/MM Spatial exception - not isolated edge';
  END IF;

  FOR rec IN EXECUTE 'SELECT * FROM '
    || quote_ident(atopology) || '.edge_data ' 
    || ' WHERE edge_id !=  ' || anedge
    || ' AND ( start_node = ' || edge.start_node
    || ' OR start_node = ' || edge.end_node
    || ' OR end_node = ' || edge.start_node
    || ' OR end_node = ' || edge.end_node
    || ' ) '
  LOOP
    RAISE EXCEPTION
      'SQL/MM Spatial exception - not isolated edge';
  END LOOP;

  --
  -- Delete the edge
  --
  EXECUTE 'DELETE FROM ' || quote_ident(atopology) || '.edge_data '
    || ' WHERE edge_id = ' || anedge;

  RETURN 'Isolated edge ' || anedge || ' removed';
END
$$
LANGUAGE 'plpgsql' VOLATILE;
--} ST_RemoveIsoEdge

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.8 
--
--  ST_NewEdgesSplit(atopology, anedge, apoint)
--
-- Not in the specs:
-- * Update references in the Relation table.
--
CREATE OR REPLACE FUNCTION topology.ST_NewEdgesSplit(varchar, integer, geometry)
  RETURNS INTEGER AS
$$
DECLARE
  atopology ALIAS FOR $1;
  anedge ALIAS FOR $2;
  apoint ALIAS FOR $3;
  oldedge RECORD;
  rec RECORD;
  tmp integer;
  topoid integer;
  nodeid integer;
  nodepos float8;
  edgeid1 integer;
  edgeid2 integer;
  edge1 geometry;
  edge2 geometry;
  ok BOOL;
BEGIN

  --
  -- All args required
  -- 
  IF atopology IS NULL OR anedge IS NULL OR apoint IS NULL THEN
    RAISE EXCEPTION
     'SQL/MM Spatial exception - null argument';
  END IF;

  --
  -- Check node existance
  -- 
  ok = false;
  FOR oldedge IN EXECUTE 'SELECT * FROM '
    || quote_ident(atopology) || '.edge_data ' ||
    ' WHERE edge_id =  ' || anedge
  LOOP
    ok = true;
  END LOOP;
  IF NOT ok THEN
    RAISE EXCEPTION
      'SQL/MM Spatial exception - non-existent edge';
  END IF;

  --
  -- Check that given point is Within(anedge.geom)
  -- 
  IF NOT ST_Within(apoint, oldedge.geom) THEN
    RAISE EXCEPTION
      'SQL/MM Spatial exception - point not on edge';
  END IF;

  --
  -- Check if a coincident node already exists
  --
  FOR rec IN EXECUTE 'SELECT node_id FROM '
    || quote_ident(atopology) || '.node '
    || 'WHERE geom && '
    || quote_literal(apoint::text) || '::geometry'
    || ' AND ST_X(geom) = ST_X('
    || quote_literal(apoint::text) || '::geometry)'
    || ' AND ST_Y(geom) = ST_Y('
    || quote_literal(apoint::text) || '::geometry)'
  LOOP
    RAISE EXCEPTION
     'SQL/MM Spatial exception - coincident node';
  END LOOP;

  --
  -- Get new node id
  --
  FOR rec IN EXECUTE 'SELECT nextval(''' ||
    atopology || '.node_node_id_seq'')'
  LOOP
    nodeid = rec.nextval;
  END LOOP;

  --RAISE NOTICE 'Next node id = % ', nodeid;

  --
  -- Add the new node 
  --
  EXECUTE 'INSERT INTO ' || quote_ident(atopology)
    || '.node(node_id, geom) 
    VALUES(' || nodeid || ','
    || quote_literal(apoint::text)
    || ')';

  --
  -- Delete the old edge
  --
  EXECUTE 'DELETE FROM ' || quote_ident(atopology) || '.edge_data '
    || ' WHERE edge_id = ' || anedge;

  --
  -- Compute new edges
  --
  nodepos = ST_Line_locate_point(oldedge.geom, apoint);
  edge1 = ST_Line_substring(oldedge.geom, 0, nodepos);
  edge2 = ST_Line_substring(oldedge.geom, nodepos, 1);

  --
  -- Get ids for the new edges 
  --
  FOR rec IN EXECUTE 'SELECT nextval(''' ||
    atopology || '.edge_data_edge_id_seq'')'
  LOOP
    edgeid1 = rec.nextval;
  END LOOP;
  FOR rec IN EXECUTE 'SELECT nextval(''' ||
    atopology || '.edge_data_edge_id_seq'')'
  LOOP
    edgeid2 = rec.nextval;
  END LOOP;

  --RAISE NOTICE 'EdgeId1 % EdgeId2 %', edgeid1, edgeid2;

  --
  -- Insert the two new edges
  --
  EXECUTE 'INSERT INTO ' || quote_ident(atopology)
    || '.edge VALUES('
    ||edgeid1||','||oldedge.start_node
    ||','||nodeid
    ||','||edgeid2
    ||','||oldedge.next_right_edge
    ||','||oldedge.left_face
    ||','||oldedge.right_face
    ||','||quote_literal(edge1::text)
    ||')';

  EXECUTE 'INSERT INTO ' || quote_ident(atopology)
    || '.edge VALUES('
    ||edgeid2||','||nodeid
    ||','||oldedge.end_node
    ||','||oldedge.next_left_edge
    ||',-'||edgeid1
    ||','||oldedge.left_face
    ||','||oldedge.right_face
    ||','||quote_literal(edge2::text)
    ||')';

  --
  -- Update all next edge references to match new layout
  --

  EXECUTE 'UPDATE ' || quote_ident(atopology)
    || '.edge_data SET next_right_edge = '
    || edgeid2
    || ','
    || ' abs_next_right_edge = ' || edgeid2
    || ' WHERE next_right_edge = ' || anedge;
  EXECUTE 'UPDATE ' || quote_ident(atopology)
    || '.edge_data SET next_right_edge = '
    || -edgeid1
    || ','
    || ' abs_next_right_edge = ' || edgeid1
    || ' WHERE next_right_edge = ' || -anedge;

  EXECUTE 'UPDATE ' || quote_ident(atopology)
    || '.edge_data SET next_left_edge = '
    || edgeid1
    || ','
    || ' abs_next_left_edge = ' || edgeid1
    || ' WHERE next_left_edge = ' || anedge;
  EXECUTE 'UPDATE ' || quote_ident(atopology)
    || '.edge_data SET '
    || ' next_left_edge = ' || -edgeid2
    || ','
    || ' abs_next_left_edge = ' || edgeid2
    || ' WHERE next_left_edge = ' || -anedge;

  -- Get topology id
        SELECT id FROM topology.topology into topoid
                WHERE name = atopology;
  IF topoid IS NULL THEN
    RAISE EXCEPTION 'No topology % registered',
      quote_ident(atopology);
  END IF;

  --
  -- Update references in the Relation table.
  -- We only take into considerations non-hierarchical
  -- TopoGeometry here, for obvious reasons.
  --
  FOR rec IN EXECUTE 'SELECT r.* FROM '
    || quote_ident(atopology)
    || '.relation r, topology.layer l '
    || ' WHERE '
    || ' l.topology_id = ' || topoid
    || ' AND l.level = 0 '
    || ' AND l.layer_id = r.layer_id '
    || ' AND abs(r.element_id) = ' || anedge
    || ' AND r.element_type = 2'
  LOOP
    --RAISE NOTICE 'TopoGeometry % in layer % contains the edge being split', rec.topogeo_id, rec.layer_id;

    -- Delete old reference
    EXECUTE 'DELETE FROM ' || quote_ident(atopology)
      || '.relation '
      || ' WHERE '
      || 'layer_id = ' || rec.layer_id
      || ' AND '
      || 'topogeo_id = ' || rec.topogeo_id
      || ' AND '
      || 'element_type = ' || rec.element_type
      || ' AND '
      || 'abs(element_id) = ' || anedge;

    -- Add new reference to edge1
    IF rec.element_id < 0 THEN
      tmp = -edgeid1;
    ELSE
      tmp = edgeid1;
    END IF;
    EXECUTE 'INSERT INTO ' || quote_ident(atopology)
      || '.relation '
      || ' VALUES( '
      || rec.topogeo_id
      || ','
      || rec.layer_id
      || ','
      || tmp
      || ','
      || rec.element_type
      || ')';

    -- Add new reference to edge2
    IF rec.element_id < 0 THEN
      tmp = -edgeid2;
    ELSE
      tmp = edgeid2;
    END IF;
    EXECUTE 'INSERT INTO ' || quote_ident(atopology)
      || '.relation '
      || ' VALUES( '
      || rec.topogeo_id
      || ','
      || rec.layer_id
      || ','
      || tmp
      || ','
      || rec.element_type
      || ')';
      
  END LOOP;

  --RAISE NOTICE 'Edge % split in edges % and % by node %',
  --  anedge, edgeid1, edgeid2, nodeid;

  RETURN nodeid; 
END
$$
LANGUAGE 'plpgsql' VOLATILE;
--} ST_NewEdgesSplit

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.9 
--
--  ST_ModEdgeSplit(atopology, anedge, apoint)
--
-- Not in the specs:
-- * Update references in the Relation table.
--
CREATE OR REPLACE FUNCTION topology.ST_ModEdgeSplit(varchar, integer, geometry)
  RETURNS INTEGER AS
$$
DECLARE
  atopology ALIAS FOR $1;
  anedge ALIAS FOR $2;
  apoint ALIAS FOR $3;
  oldedge RECORD;
  rec RECORD;
  tmp integer;
  topoid integer;
  nodeid integer;
  nodepos float8;
  newedgeid integer;
  newedge1 geometry;
  newedge2 geometry;
  query text;
  ok BOOL;
BEGIN

  --
  -- All args required
  -- 
  IF atopology IS NULL OR anedge IS NULL OR apoint IS NULL THEN
    RAISE EXCEPTION
     'SQL/MM Spatial exception - null argument';
  END IF;

  --
  -- Check node existance
  -- 
  ok = false;
  FOR oldedge IN EXECUTE 'SELECT * FROM '
    || quote_ident(atopology) || '.edge_data ' ||
    ' WHERE edge_id =  ' || anedge
  LOOP
    ok = true;
  END LOOP;
  IF NOT ok THEN
    RAISE EXCEPTION
      'SQL/MM Spatial exception - non-existent edge';
  END IF;

  --
  -- Check that given point is Within(anedge.geom)
  -- 
  IF NOT ST_Within(apoint, oldedge.geom) THEN
    RAISE EXCEPTION
      'SQL/MM Spatial exception - point not on edge';
  END IF;

  --
  -- Check if a coincident node already exists
  --
  FOR rec IN EXECUTE 'SELECT node_id FROM '
    || quote_ident(atopology) || '.node ' ||
    'WHERE geom && '
    || quote_literal(apoint::text) || '::geometry'
    ||' AND ST_X(geom) = ST_X('
    || quote_literal(apoint::text) || '::geometry)'
    ||' AND ST_Y(geom) = ST_Y('
    ||quote_literal(apoint::text)||'::geometry)'
  LOOP
    RAISE EXCEPTION
     'SQL/MM Spatial exception - coincident node';
  END LOOP;

  --
  -- Get new node id
  --
  FOR rec IN EXECUTE 'SELECT nextval(''' ||
    atopology || '.node_node_id_seq'')'
  LOOP
    nodeid = rec.nextval;
  END LOOP;

  --RAISE NOTICE 'Next node id = % ', nodeid;

  --
  -- Add the new node 
  --
  EXECUTE 'INSERT INTO ' || quote_ident(atopology)
    || '.node(node_id, geom) 
    VALUES('||nodeid||','||quote_literal(apoint::text)||
    ')';

  --
  -- Compute new edge
  --
  nodepos = ST_Line_Locate_Point(oldedge.geom, apoint);
  newedge1 = ST_Line_Substring(oldedge.geom, 0, nodepos);
  newedge2 = ST_Line_Substring(oldedge.geom, nodepos, 1);


  --
  -- Get ids for the new edge
  --
  FOR rec IN EXECUTE 'SELECT nextval(''' ||
    atopology || '.edge_data_edge_id_seq'')'
  LOOP
    newedgeid = rec.nextval;
  END LOOP;

  --
  -- Insert the new edge
  --
  EXECUTE 'INSERT INTO ' || quote_ident(atopology)
    || '.edge '
    || '(edge_id, start_node, end_node,'
    || 'next_left_edge, next_right_edge,'
    || 'left_face, right_face, geom) '
    || 'VALUES('
    ||newedgeid||','||nodeid
    ||','||oldedge.end_node
    ||','||oldedge.next_left_edge
    ||',-'||anedge
    ||','||oldedge.left_face
    ||','||oldedge.right_face
    ||','||quote_literal(newedge2::text)
    ||')';

  --
  -- Update the old edge
  --
  EXECUTE 'UPDATE ' || quote_ident(atopology) || '.edge_data '
    || ' SET geom = ' || quote_literal(newedge1::text)
    || ','
    || ' next_left_edge = ' || newedgeid
    || ','
    || ' end_node = ' || nodeid
    || ' WHERE edge_id = ' || anedge;


  --
  -- Update all next edge references to match new layout
  --

  EXECUTE 'UPDATE ' || quote_ident(atopology)
    || '.edge_data SET next_right_edge = '
    || -newedgeid 
    || ','
    || ' abs_next_right_edge = ' || newedgeid
    || ' WHERE next_right_edge = ' || -anedge;

  EXECUTE 'UPDATE ' || quote_ident(atopology)
    || '.edge_data SET '
    || ' next_left_edge = ' || -newedgeid
    || ','
    || ' abs_next_left_edge = ' || newedgeid
    || ' WHERE next_left_edge = ' || -anedge;

  -- Get topology id
        SELECT id FROM topology.topology into topoid
                WHERE name = atopology;

  --
  -- Update references in the Relation table.
  -- We only take into considerations non-hierarchical
  -- TopoGeometry here, for obvious reasons.
  --
  FOR rec IN EXECUTE 'SELECT r.* FROM '
    || quote_ident(atopology)
    || '.relation r, topology.layer l '
    || ' WHERE '
    || ' l.topology_id = ' || topoid
    || ' AND l.level = 0 '
    || ' AND l.layer_id = r.layer_id '
    || ' AND abs(r.element_id) = ' || anedge
    || ' AND r.element_type = 2'
  LOOP
    --RAISE NOTICE 'TopoGeometry % in layer % contains the edge being split (%) - updating to add new edge %', rec.topogeo_id, rec.layer_id, anedge, newedgeid;

    -- Add new reference to edge1
    IF rec.element_id < 0 THEN
      tmp = -newedgeid;
    ELSE
      tmp = newedgeid;
    END IF;
    query = 'INSERT INTO ' || quote_ident(atopology)
      || '.relation '
      || ' VALUES( '
      || rec.topogeo_id
      || ','
      || rec.layer_id
      || ','
      || tmp
      || ','
      || rec.element_type
      || ')';

    --RAISE NOTICE '%', query;
    EXECUTE query;
  END LOOP;

  --RAISE NOTICE 'Edge % split in edges % and % by node %',
  --  anedge, anedge, newedgeid, nodeid;

  RETURN nodeid; 
END
$$
LANGUAGE 'plpgsql' VOLATILE;
--} ST_ModEdgesSplit

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.4 
--
--  ST_AddIsoEdge(atopology, anode, anothernode, acurve)
-- 
-- Not in the specs:
-- * Reset containing_face for starting and ending point,
--   as they stop being isolated nodes
-- * Refuse to add a closed edge, as it would not be isolated
--   (ie: would create a ring)
--
-- }{
--
CREATE OR REPLACE FUNCTION topology.ST_AddIsoEdge(atopology varchar, anode integer, anothernode integer, acurve geometry)
  RETURNS INTEGER AS
$$
DECLARE
  aface INTEGER;
  face GEOMETRY;
  snodegeom GEOMETRY;
  enodegeom GEOMETRY;
  count INTEGER;
  rec RECORD;
  edgeid INTEGER;
BEGIN

  --
  -- All arguments required
  -- 
  IF atopology IS NULL
     OR anode IS NULL
     OR anothernode IS NULL
     OR acurve IS NULL
  THEN
    RAISE EXCEPTION
     'SQL/MM Spatial exception - null argument';
  END IF;

  -- NOT IN THE SPECS:
  -- A closed edge is never isolated (as it forms a face)
  IF anode = anothernode THEN
      RAISE EXCEPTION
       'Closed edges would not be isolated, try ST_AddEdgeNewFaces';
  END IF;

  --
  -- Acurve must be a LINESTRING
  --
  IF substring(geometrytype(acurve), 1, 4) != 'LINE'
  THEN
    RAISE EXCEPTION
     'SQL/MM Spatial exception - invalid curve';
  END IF;

  --
  -- Acurve must be simple
  --
  IF NOT ST_IsSimple(acurve)
  THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - curve not simple';
  END IF;

  --
  -- Check for:
  --    existence of nodes
  --    nodes faces match
  -- Extract:
  --    nodes face id
  --    nodes geoms
  --
  aface := NULL;
  count := 0;
  FOR rec IN EXECUTE 'SELECT geom, containing_face, node_id FROM '
    || quote_ident(atopology) || '.node
    WHERE node_id = ' || anode ||
    ' OR node_id = ' || anothernode
  LOOP 

    IF rec.containing_face IS NULL THEN
      RAISE EXCEPTION 'SQL/MM Spatial exception - not isolated node';
    END IF;

    IF aface IS NULL THEN
      aface := rec.containing_face;
    ELSE
      IF aface != rec.containing_face THEN
        RAISE EXCEPTION 'SQL/MM Spatial exception - nodes in different faces';
      END IF;
    END IF;

    -- Get nodes geom
    IF rec.node_id = anode THEN
      snodegeom = rec.geom;
    ELSE
      enodegeom = rec.geom;
    END IF;

    count = count+1;

  END LOOP;

  -- TODO: don't need count, can do with snodegeom/enodegeom instead..
  IF count < 2 THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - non-existent node';
  END IF;


  --
  -- l) Check that start point of acurve match start node
  -- geoms.
  -- 
  IF ST_X(snodegeom) != ST_X(ST_StartPoint(acurve)) OR
     ST_Y(snodegeom) != ST_Y(ST_StartPoint(acurve))
  THEN
    RAISE EXCEPTION
      'SQL/MM Spatial exception - start node not geometry start point.';
  END IF;

  --
  -- m) Check that end point of acurve match end node
  -- geoms.
  -- 
  IF ST_X(enodegeom) != ST_X(ST_EndPoint(acurve)) OR
     ST_Y(enodegeom) != ST_Y(ST_EndPoint(acurve))
  THEN
    RAISE EXCEPTION
      'SQL/MM Spatial exception - end node not geometry end point.';
  END IF;

  --
  -- n) Check if curve crosses (contains) any node
  -- I used _contains_ here to leave endpoints out
  -- 
  FOR rec IN EXECUTE 'SELECT node_id FROM '
    || quote_ident(atopology) || '.node '
    || ' WHERE geom && ' || quote_literal(acurve::text) 
    || ' AND ST_Contains(' || quote_literal(acurve::text)
    || ',geom)'
  LOOP
    RAISE EXCEPTION
      'SQL/MM Spatial exception - geometry crosses a node';
  END LOOP;

  --
  -- o) Check if curve intersects any other edge
  -- 
  FOR rec IN EXECUTE 'SELECT * FROM '
    || quote_ident(atopology) || '.edge_data
    WHERE ST_Intersects(geom, ' || quote_literal(acurve::text) || '::geometry)'
  LOOP
    RAISE EXCEPTION 'SQL/MM Spatial exception - geometry intersects an edge';
  END LOOP;

  --
  -- Get new edge id from sequence
  --
  FOR rec IN EXECUTE 'SELECT nextval(''' ||
    atopology || '.edge_data_edge_id_seq'')'
  LOOP
    edgeid = rec.nextval;
  END LOOP;

  -- TODO: this should likely be an exception instead !
  IF aface IS NULL THEN
    aface := 0;
  END IF;

  --
  -- Insert the new row
  --
  EXECUTE 'INSERT INTO ' || quote_ident(atopology)
    || '.edge VALUES(' || edgeid || ',' || anode
    || ',' || anothernode || ',' || (-edgeid)
    || ',' || edgeid || ','
    || aface || ',' || aface || ','
    || quote_literal(acurve::text) || ')';

  --
  -- Update Node containing_face values
  --
  -- the nodes anode and anothernode are no more isolated
  -- because now there is an edge connecting them
  -- 
  EXECUTE 'UPDATE ' || quote_ident(atopology)
    || '.node SET containing_face = NULL where (node_id ='
    || anode
    || ' OR node_id='
    || anothernode
    || ')';

  RETURN edgeid;

END
$$
LANGUAGE 'plpgsql' VOLATILE;
--} ST_AddIsoEdge

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.6
--
--  ST_ChangeEdgeGeom(atopology, anedge, acurve)
--
-- Not in the specs:
-- * Raise an exception if given a non-existent edge
--
-- TODO: allow changing geometry of a closed edge (#982)
-- 
CREATE OR REPLACE FUNCTION topology.ST_ChangeEdgeGeom(atopology varchar, anedge integer, acurve geometry)
  RETURNS TEXT AS
$$
DECLARE
  aface INTEGER;
  face GEOMETRY;
  snodegeom GEOMETRY;
  enodegeom GEOMETRY;
  count INTEGER;
  rec RECORD;
  edgeid INTEGER;
  oldedge RECORD;
BEGIN

  --
  -- All arguments required
  -- 
  IF atopology IS NULL
     OR anedge IS NULL
     OR acurve IS NULL
  THEN
    RAISE EXCEPTION
     'SQL/MM Spatial exception - null argument';
  END IF;

  --
  -- Acurve must be a LINESTRING
  --
  IF substring(geometrytype(acurve), 1, 4) != 'LINE'
  THEN
    RAISE EXCEPTION
     'SQL/MM Spatial exception - invalid curve';
  END IF;

  --
  -- Acurve must be a simple
  --
  IF NOT ST_IsSimple(acurve)
  THEN
    RAISE EXCEPTION
     'SQL/MM Spatial exception - curve not simple';
  END IF;

  --
  -- Get data about existing edge
  --
  BEGIN
    EXECUTE 'SELECT * FROM ' || quote_ident(atopology) || '.edge_data  '
      || ' WHERE edge_id = ' || anedge
    INTO STRICT oldedge;
  EXCEPTION
    -- NOT IN THE SPECS: check given edge existance
    WHEN NO_DATA_FOUND THEN
      RAISE EXCEPTION 'SQL/MM Spatial exception - non-existent edge %', anedge;
  END;

  --
  -- e) Check StartPoint consistency
  --
  IF NOT ST_Equals(ST_StartPoint(acurve), ST_StartPoint(oldedge.geom)) THEN
    RAISE EXCEPTION
      'SQL/MM Spatial exception - start node not geometry start point.';
  END IF;

  --
  -- f) Check EndPoint consistency
  --
  IF NOT ST_Equals(ST_EndPoint(acurve), ST_EndPoint(oldedge.geom)) THEN
    RAISE EXCEPTION
      'SQL/MM Spatial exception - end node not geometry end point.';
  END IF;

  --
  -- g) Check if curve crosses any node
  -- _within_ used to let endpoints out
  -- 
  FOR rec IN EXECUTE 'SELECT node_id FROM '
    || quote_ident(atopology) || '.node
    WHERE ST_Within(geom, ' || quote_literal(acurve::text) || '::geometry)'
  LOOP
    RAISE EXCEPTION
    'SQL/MM Spatial exception - geometry crosses a node';
  END LOOP;

  --
  -- h) Check if curve intersects any other edge
  -- 
  FOR rec IN EXECUTE 'SELECT * FROM '
    || quote_ident(atopology) || '.edge_data '
    || ' WHERE edge_id != ' || anedge
    || ' AND ST_Intersects(geom, '
    || quote_literal(acurve::text) || '::geometry)'
  LOOP
    RAISE EXCEPTION
    'SQL/MM Spatial exception - geometry intersects an edge';
  END LOOP;

  --
  -- Update edge geometry
  --
  EXECUTE 'UPDATE ' || quote_ident(atopology) || '.edge_data '
    || ' SET geom = ' || quote_literal(acurve::text) 
    || ' WHERE edge_id = ' || anedge;

  RETURN 'Edge ' || anedge || ' changed';

END
$$
LANGUAGE 'plpgsql' VOLATILE;
--} ST_ChangeEdgeGeom

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.12
--
--  ST_AddEdgeNewFaces(atopology, anode, anothernode, acurve)
--
-- Not in the specs:
-- * Reset containing_face for starting and ending point,
--   as they stop being isolated nodes
-- * Update references in the Relation table.
--
CREATE OR REPLACE FUNCTION topology.ST_AddEdgeNewFaces(atopology varchar, anode integer, anothernode integer, acurve geometry)
  RETURNS INTEGER AS
$$
DECLARE
  rec RECORD;
  i INTEGER;
  topoid INTEGER;
  az FLOAT8;
  span RECORD; -- start point analysis data
  epan RECORD; --   end point analysis data
  fan RECORD; -- face analisys
  newedge RECORD; -- informations about new edge
  sql TEXT;
  newfaces INTEGER[];
  newface INTEGER;
  p1 GEOMETRY;
  p2 GEOMETRY;
  seg GEOMETRY;
  p1b GEOMETRY;
  p2b GEOMETRY;
BEGIN

  --
  -- All args required
  -- 
  IF atopology IS NULL
    OR anode IS NULL
    OR anothernode IS NULL
    OR acurve IS NULL
  THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - null argument';
  END IF;

  --
  -- Acurve must be a LINESTRING
  --
  IF substring(geometrytype(acurve), 1, 4) != 'LINE'
  THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - invalid curve';
  END IF;
  
  --
  -- Curve must be simple
  --
  IF NOT ST_IsSimple(acurve) THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - curve not simple';
  END IF;

  --
  -- Get topology id
  --
  BEGIN
    SELECT id FROM topology.topology
      INTO STRICT topoid WHERE name = atopology;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN
        RAISE EXCEPTION 'SQL/MM Spatial exception - invalid topology name';
  END;

  -- Initialize new edge info (will be filled up more later)
  SELECT anode as start_node, anothernode as end_node, acurve as geom,
    NULL::int as next_left_edge, NULL::int as next_right_edge,
    NULL::int as left_face, NULL::int as right_face, NULL::int as edge_id,
    NULL::int as prev_left_edge, NULL::int as prev_right_edge, -- convenience
    anode = anothernode as isclosed, -- convenience
    false as start_node_isolated, -- convenience
    false as end_node_isolated, -- convenience
    ST_RemoveRepeatedPoints(acurve) as cleangeom -- convenience
  INTO newedge;

  -- Compute azimut of first edge end on start node
  SELECT null::int AS nextCW, null::int AS nextCCW,
         null::float8 AS minaz, null::float8 AS maxaz,
         false AS was_isolated,
         ST_Azimuth(ST_StartPoint(newedge.cleangeom),
                    ST_PointN(newedge.cleangeom, 2)) AS myaz
  INTO span;
  IF span.myaz IS NULL THEN
    RAISE EXCEPTION 'Invalid edge (no two distinct vertices exist)';
  END IF;

  -- Compute azimuth of last edge end on end node
  SELECT null::int AS nextCW, null::int AS nextCCW,
         null::float8 AS minaz, null::float8 AS maxaz,
         false AS was_isolated,
         ST_Azimuth(ST_EndPoint(newedge.cleangeom),
                    ST_PointN(newedge.cleangeom,
                              ST_NumPoints(newedge.cleangeom)-1)) AS myaz
  INTO epan;
  IF epan.myaz IS NULL THEN
    RAISE EXCEPTION 'Invalid edge (no two distinct vertices exist)';
  END IF;


  -- 
  -- Check endpoints existance, match with Curve geometry
  -- and get face information (if any)
  --
  i := 0;
  FOR rec IN EXECUTE 'SELECT node_id, '
    || ' CASE WHEN node_id = ' || anode
    || ' THEN 1 WHEN node_id = ' || anothernode
    || ' THEN 0 END AS start, containing_face, geom FROM '
    || quote_ident(atopology)
    || '.node '
    || ' WHERE node_id IN ( '
    || anode || ',' || anothernode
    || ')'
  LOOP
    IF rec.containing_face IS NOT NULL THEN
      RAISE DEBUG  'containing_face for node %:%',
        rec.node_id, rec.containing_face;
      IF newedge.left_face IS NULL THEN
        newedge.left_face := rec.containing_face;
        newedge.right_face := rec.containing_face;
      ELSE
        IF newedge.left_face != rec.containing_face THEN
          RAISE EXCEPTION
            'SQL/MM Spatial exception - geometry crosses an edge (endnodes in faces % and %)', newedge.left_face, rec.containing_face;
        END IF;
      END IF;
    END IF;

    IF rec.start THEN
      IF NOT Equals(rec.geom, ST_StartPoint(acurve))
      THEN
        RAISE EXCEPTION
          'SQL/MM Spatial exception - start node not geometry start point.';
      END IF;
    ELSE
      IF NOT Equals(rec.geom, ST_EndPoint(acurve))
      THEN
        RAISE EXCEPTION
          'SQL/MM Spatial exception - end node not geometry end point.';
      END IF;
    END IF;

    i := i + 1;
  END LOOP;

  IF NOT newedge.isclosed THEN
    IF i < 2 THEN
    RAISE EXCEPTION
     'SQL/MM Spatial exception - non-existent node';
    END IF;
  ELSE
    IF i < 1 THEN
    RAISE EXCEPTION
     'SQL/MM Spatial exception - non-existent node';
    END IF;
  END IF;

  --
  -- Check if this geometry crosses any node
  --
  FOR rec IN EXECUTE
    'SELECT node_id, ST_Relate(geom, '
    || quote_literal(acurve::text) || '::geometry, 2) as relate FROM '
    || quote_ident(atopology)
    || '.node WHERE geom && '
    || quote_literal(acurve::text)
    || '::geometry'
  LOOP
    IF ST_RelateMatch(rec.relate, 'T********') THEN
      RAISE EXCEPTION 'SQL/MM Spatial exception - geometry crosses a node';
    END IF;
  END LOOP;

  --
  -- Check if this geometry has any interaction with any existing edge
  --
  FOR rec IN EXECUTE 'SELECT edge_id, ST_Relate(geom,' 
    || quote_literal(acurve::text)
    || '::geometry, 2) as im FROM '
    || quote_ident(atopology)
    || '.edge_data WHERE geom && '
    || quote_literal(acurve::text) || '::geometry'
  LOOP

    --RAISE DEBUG 'IM=%',rec.im;

    IF ST_RelateMatch(rec.im, 'F********') THEN
      CONTINUE; -- no interior intersection
    END IF;

    IF ST_RelateMatch(rec.im, '1FFF*FFF2') THEN
      RAISE EXCEPTION
        'SQL/MM Spatial exception - coincident edge';
    END IF;

    -- NOT IN THE SPECS: geometry touches an edge
    IF ST_RelateMatch(rec.im, '1********') THEN
      RAISE EXCEPTION
        'Spatial exception - geometry intersects edge %', rec.edge_id;
    END IF;

    IF ST_RelateMatch(rec.im, 'T********') THEN
      RAISE EXCEPTION
        'SQL/MM Spatial exception - geometry crosses an edge';
    END IF;

  END LOOP;

  ---------------------------------------------------------------
  --
  -- All checks passed, time to prepare the new edge
  --
  ---------------------------------------------------------------

  EXECUTE 'SELECT nextval(' || quote_literal(
      quote_ident(atopology) || '.edge_data_edge_id_seq') || ')'
  INTO STRICT newedge.edge_id;


  -- Find links on start node -- {

  RAISE DEBUG 'My start-segment azimuth: %', span.myaz;

  sql :=
    'SELECT edge_id, -1 AS end_node, start_node, left_face, right_face, '
    || 'ST_RemoveRepeatedPoints(geom) as geom FROM '
    || quote_ident(atopology)
    || '.edge_data WHERE start_node = ' || anode
    || ' UNION SELECT edge_id, end_node, -1, left_face, right_face, '
    || 'ST_RemoveRepeatedPoints(geom) FROM '
    || quote_ident(atopology)
    || '.edge_data WHERE end_node = ' || anode;
  IF newedge.isclosed THEN
    sql := sql || ' UNION SELECT '
      || newedge.edge_id || ',' || newedge.end_node
      || ',-1,0,0,' -- pretend we start elsewhere
      || quote_literal(newedge.cleangeom::text);
  END IF;
  i := 0;
  FOR rec IN EXECUTE sql
  LOOP -- incident edges {

    i := i + 1;

    IF rec.start_node = anode THEN
      --
      -- Edge starts at our node, we compute
      -- azimuth from node to its second point
      --
      az := ST_Azimuth(ST_StartPoint(rec.geom), ST_PointN(rec.geom, 2));

    ELSE
      --
      -- Edge ends at our node, we compute
      -- azimuth from node to its second-last point
      --
      az := ST_Azimuth(ST_EndPoint(rec.geom),
                       ST_PointN(rec.geom, ST_NumPoints(rec.geom)-1));
      rec.edge_id := -rec.edge_id;

    END IF;

    IF az IS NULL THEN
      RAISE EXCEPTION 'Invalid edge % found (no two distinct nodes exist)',
        rec.edge_id;
    END IF;

    RAISE DEBUG 'Edge % - az % (%) - fl:% fr:%',
      rec.edge_id, az, az - span.myaz, rec.left_face, rec.right_face;

    az = az - span.myaz;
    IF az < 0 THEN
      az := az + 2*PI();
    END IF;

    -- RAISE DEBUG ' normalized az %', az;

    IF span.maxaz IS NULL OR az > span.maxaz THEN
      span.maxaz := az;
      span.nextCCW := rec.edge_id;
      IF abs(rec.edge_id) != newedge.edge_id THEN
        IF rec.edge_id < 0 THEN
          -- TODO: check for mismatch ?
          newedge.left_face := rec.left_face;
        ELSE
          -- TODO: check for mismatch ?
          newedge.left_face := rec.right_face;
        END IF;
      END IF;
    END IF;

    IF span.minaz IS NULL OR az < span.minaz THEN
      span.minaz := az;
      span.nextCW := rec.edge_id;
      IF abs(rec.edge_id) != newedge.edge_id THEN
        IF rec.edge_id < 0 THEN
          -- TODO: check for mismatch ?
          newedge.right_face := rec.right_face;
        ELSE
          -- TODO: check for mismatch ?
          newedge.right_face := rec.left_face;
        END IF;
      END IF;
    END IF;

    --RAISE DEBUG 'Closest edges: CW:%(%) CCW:%(%)', span.nextCW, span.minaz, span.nextCCW, span.maxaz;

  END LOOP; -- incident edges }

  RAISE DEBUG 'span ROW_COUNT: %', i;
  IF newedge.isclosed THEN
    IF i < 2 THEN span.was_isolated = true; END IF;
  ELSE
    IF i < 1 THEN span.was_isolated = true; END IF;
  END IF;

  IF span.nextCW IS NULL THEN
    -- This happens if the destination node is isolated
    newedge.next_right_edge := newedge.edge_id;
    newedge.prev_left_edge := -newedge.edge_id;
  ELSE
    newedge.next_right_edge := span.nextCW;
    newedge.prev_left_edge := -span.nextCCW;
  END IF;

  RAISE DEBUG 'edge:%', newedge.edge_id;
  RAISE DEBUG ' left:%, next:%, prev:%',
    newedge.left_face, newedge.next_left_edge, newedge.prev_left_edge;
  RAISE DEBUG ' right:%, next:%, prev:%',
    newedge.right_face, newedge.next_right_edge, newedge.prev_right_edge;

  -- } start_node analysis


  -- Find links on end_node {
      
  RAISE DEBUG 'My end-segment azimuth: %', epan.myaz;

  sql :=
    'SELECT edge_id, -1 as end_node, start_node, left_face, right_face, '
    || 'ST_RemoveRepeatedPoints(geom) as geom FROM '
    || quote_ident(atopology)
    || '.edge_data WHERE start_node = ' || anothernode
    || 'UNION SELECT edge_id, end_node, -1, left_face, right_face, '
    || 'ST_RemoveRepeatedPoints(geom) FROM '
    || quote_ident(atopology)
    || '.edge_data WHERE end_node = ' || anothernode;
  IF newedge.isclosed THEN
    sql := sql || ' UNION SELECT '
      || newedge.edge_id || ',' || -1 -- pretend we end elsewhere
      || ',' || newedge.start_node || ',0,0,'
      || quote_literal(newedge.cleangeom::text);
  END IF;
  i := 0;
  FOR rec IN EXECUTE sql
  LOOP -- incident edges {

    i := i + 1;

    IF rec.start_node = anothernode THEN
      --
      -- Edge starts at our node, we compute
      -- azimuth from node to its second point
      --
      az := ST_Azimuth(ST_StartPoint(rec.geom),
                       ST_PointN(rec.geom, 2));

    ELSE
      --
      -- Edge ends at our node, we compute
      -- azimuth from node to its second-last point
      --
      az := ST_Azimuth(ST_EndPoint(rec.geom),
        ST_PointN(rec.geom, ST_NumPoints(rec.geom)-1));
      rec.edge_id := -rec.edge_id;

    END IF;

    RAISE DEBUG 'Edge % - az % (%)', rec.edge_id, az, az - epan.myaz;

    az := az - epan.myaz;
    IF az < 0 THEN
      az := az + 2*PI();
    END IF;

    -- RAISE DEBUG ' normalized az %', az;

    IF epan.maxaz IS NULL OR az > epan.maxaz THEN
      epan.maxaz := az;
      epan.nextCCW := rec.edge_id;
      IF abs(rec.edge_id) != newedge.edge_id THEN
        IF rec.edge_id < 0 THEN
          -- TODO: check for mismatch ?
          newedge.right_face := rec.left_face;
        ELSE
          -- TODO: check for mismatch ?
          newedge.right_face := rec.right_face;
        END IF;
      END IF;
    END IF;

    IF epan.minaz IS NULL OR az < epan.minaz THEN
      epan.minaz := az;
      epan.nextCW := rec.edge_id;
      IF abs(rec.edge_id) != newedge.edge_id THEN
        IF rec.edge_id < 0 THEN
          -- TODO: check for mismatch ?
          newedge.left_face := rec.right_face;
        ELSE
          -- TODO: check for mismatch ?
          newedge.left_face := rec.left_face;
        END IF;
      END IF;
    END IF;

    --RAISE DEBUG 'Closest edges: CW:%(%) CCW:%(%)', epan.nextCW, epan.minaz, epan.nextCCW, epan.maxaz;

  END LOOP; -- incident edges }

  RAISE DEBUG 'epan ROW_COUNT: %', i;
  IF newedge.isclosed THEN
    IF i < 2 THEN epan.was_isolated = true; END IF;
  ELSE
    IF i < 1 THEN epan.was_isolated = true; END IF;
  END IF;

  IF epan.nextCW IS NULL THEN
    -- This happens if the destination node is isolated
    newedge.next_left_edge := -newedge.edge_id;
    newedge.prev_right_edge := newedge.edge_id;
  ELSE
    newedge.next_left_edge := epan.nextCW;
    newedge.prev_right_edge := -epan.nextCCW;
  END IF;

  -- } end_node analysis

  RAISE DEBUG 'edge:%', newedge.edge_id;
  RAISE DEBUG ' left:%, next:%, prev:%',
    newedge.left_face, newedge.next_left_edge, newedge.prev_left_edge;
  RAISE DEBUG ' right:%, next:%, prev:%',
    newedge.right_face, newedge.next_right_edge, newedge.prev_right_edge;

  ----------------------------------------------------------------------
  --
  -- If we don't have faces setup by now we must have encountered
  -- a malformed topology (no containing_face on isolated nodes, no
  -- left/right faces on adjacent edges or mismatching values)
  --
  ----------------------------------------------------------------------
  IF newedge.left_face != newedge.right_face THEN
    RAISE EXCEPTION 'Left(%)/right(%) faces mismatch: invalid topology ?', 
      newedge.left_face, newedge.right_face;
  END IF;
  IF newedge.left_face IS NULL THEN
    RAISE EXCEPTION 'Could not derive edge face from linked primitives: invalid topology ?';
  END IF;

  ----------------------------------------------------------------------
  --
  -- Polygonize the current edges (to see later if the addition
  -- of the new one created another ring)
  --
  ----------------------------------------------------------------------

  SELECT null::geometry as post, null::geometry as pre INTO fan;

  EXECUTE
    'SELECT ST_Polygonize(geom) FROM '
    || quote_ident(atopology) || '.edge_data WHERE left_face = '
    || newedge.left_face || ' OR right_face = ' || newedge.right_face
    INTO STRICT fan.pre;

  ----------------------------------------------------------------------
  --
  -- Insert the new edge, and update all linking
  --
  ----------------------------------------------------------------------

  -- Insert the new edge with what we have so far
  EXECUTE 'INSERT INTO ' || quote_ident(atopology) 
    || '.edge VALUES(' || newedge.edge_id
    || ',' || newedge.start_node
    || ',' || newedge.end_node
    || ',' || newedge.next_left_edge
    || ',' || newedge.next_right_edge
    || ',' || newedge.left_face
    || ',' || newedge.right_face
    || ',' || quote_literal(newedge.geom::geometry::text)
    || ')';

  -- Link prev_left_edge to us 
  -- (if it's not us already)
  IF abs(newedge.prev_left_edge) != newedge.edge_id THEN
    IF newedge.prev_left_edge > 0 THEN
      -- its next_left_edge is us
      EXECUTE 'UPDATE ' || quote_ident(atopology)
        || '.edge_data SET next_left_edge = '
        || newedge.edge_id
        || ', abs_next_left_edge = '
        || newedge.edge_id
        || ' WHERE edge_id = ' 
        || newedge.prev_left_edge;
    ELSE
      -- its next_right_edge is us
      EXECUTE 'UPDATE ' || quote_ident(atopology)
        || '.edge_data SET next_right_edge = '
        || newedge.edge_id
        || ', abs_next_right_edge = '
        || newedge.edge_id
        || ' WHERE edge_id = ' 
        || -newedge.prev_left_edge;
    END IF;
  END IF;

  -- Link prev_right_edge to us 
  -- (if it's not us already)
  IF abs(newedge.prev_right_edge) != newedge.edge_id THEN
    IF newedge.prev_right_edge > 0 THEN
      -- its next_left_edge is -us
      EXECUTE 'UPDATE ' || quote_ident(atopology)
        || '.edge_data SET next_left_edge = '
        || -newedge.edge_id
        || ', abs_next_left_edge = '
        || newedge.edge_id
        || ' WHERE edge_id = ' 
        || newedge.prev_right_edge;
    ELSE
      -- its next_right_edge is -us
      EXECUTE 'UPDATE ' || quote_ident(atopology)
        || '.edge_data SET next_right_edge = '
        || -newedge.edge_id
        || ', abs_next_right_edge = '
        || newedge.edge_id
        || ' WHERE edge_id = ' 
        || -newedge.prev_right_edge;
    END IF;
  END IF;

  -- NOT IN THE SPECS...
  -- set containing_face = null for start_node and end_node
  -- if they where isolated 
  IF span.was_isolated OR epan.was_isolated THEN
      EXECUTE 'UPDATE ' || quote_ident(atopology)
        || '.node SET containing_face = null WHERE node_id IN ('
        || anode || ',' || anothernode || ')';
  END IF;

  ----------------------------------------------------------------------
  --
  -- Polygonize the new edges and see if the addition created a new ring
  --
  ----------------------------------------------------------------------

  EXECUTE 'SELECT ST_Polygonize(geom) FROM '
    || quote_ident(atopology) || '.edge_data WHERE left_face = '
    || newedge.left_face || ' OR right_face = ' || newedge.right_face
    INTO STRICT fan.post;

  IF ST_NumGeometries(fan.pre) = ST_NumGeometries(fan.post) THEN
    -- No splits, all done
    RETURN newedge.edge_id;
  END IF;

  RAISE NOTICE 'ST_AddEdgeNewFaces: edge % splitted face %',
      newedge.edge_id, newedge.left_face;

  -- Call topology.AddFace for every face containing the new edge
  -- 
  -- The ORDER serves predictability of which face is added first
  -- See http://trac.osgeo.org/postgis/ticket/1205
  --
  p1 := ST_StartPoint(newedge.cleangeom);
  p2 := ST_PointN(newedge.cleangeom, 2);
  seg := ST_MakeLine(p1, p2);
  -- The new rings might start on one of the new edge endpoints
  -- we we sample two more inner points for the sake of computing
  -- face side.
  p1b := ST_Line_Interpolate_Point(seg, 0.1);
  p2b := ST_Line_Interpolate_Point(seg, 0.9);
  FOR rec IN
    WITH faces AS ( SELECT * FROM ST_Dump(ST_ForceRHR(fan.post)) ),
         rings AS ( SELECT path,
                      ST_Boundary((ST_DumpRings(geom)).geom) as bnd
                    FROM faces ),
         paths AS ( SELECT path,
                      ST_Line_Locate_Point(bnd, p1b) as pos1,
                      ST_Line_Locate_Point(bnd, p2b) as pos2,
                      ST_Line_Locate_Point(bnd, p1b) < 
                      ST_Line_Locate_Point(bnd, p2b) as right_side
                    FROM rings
                    WHERE ST_Contains(bnd, seg) )
    SELECT
      CASE WHEN p.right_side THEN 'right' ELSE 'left' END as side,
      p.pos1, p.pos2,
      f.geom
    FROM paths p INNER JOIN faces f ON (p.path = f.path)
    ORDER BY p.right_side DESC
  LOOP -- {
    RAISE DEBUG 'Adding % face', rec.side;
    SELECT topology.AddFace(atopology, rec.geom, true) INTO newface;
    newfaces := array_append(newfaces, newface);
  END LOOP; --}

  RAISE DEBUG 'Added faces: %', newfaces;

  IF array_upper(newfaces, 1) > 2 THEN
    -- Sanity check..
    RAISE EXCEPTION 'More than 2 faces found to be formed by new edge';
  END IF;

  IF newedge.left_face != 0 THEN -- {

    -- NOT IN THE SPECS:
    -- update TopoGeometry compositions to substitute oldface with newfaces
    sql := 'UPDATE '
      || quote_ident(atopology)
      || '.relation r set element_id = ' || newfaces[1]
      || ' FROM topology.layer l '
      || ' WHERE l.topology_id = ' || topoid
      || ' AND l.level = 0 '
      || ' AND l.layer_id = r.layer_id '
      || ' AND r.element_id = ' || newedge.left_face
      || ' AND r.element_type = 3 RETURNING r.topogeo_id, r.layer_id';
    --RAISE DEBUG 'SQL: %', sql;
    FOR rec IN EXECUTE sql
    LOOP
      RAISE DEBUG 'TopoGeometry % in layer % contained the face being split (%) - updating to contain both new faces %', rec.topogeo_id, rec.layer_id, newedge.left_face, newfaces;

      -- Add reference to the other face
      sql := 'INSERT INTO ' || quote_ident(atopology)
        || '.relation VALUES( ' || rec.topogeo_id
        || ',' || rec.layer_id || ',' || newfaces[2] || ', 3)';
      --RAISE DEBUG 'SQL: %', sql;
      EXECUTE sql;

    END LOOP;

    -- drop old face from faces table
    sql := 'DELETE FROM ' || quote_ident(atopology)
      || '.face WHERE face_id = ' || newedge.left_face;
    EXECUTE sql;

  END IF; -- }

  RETURN newedge.edge_id;
END
$$
LANGUAGE 'plpgsql' VOLATILE;
--} ST_AddEdgeNewFaces

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.13
--
--  ST_AddEdgeModFace(atopology, anode, anothernode, acurve)
--
-- Not in the specs:
-- * Reset containing_face for starting and ending point,
--   as they stop being isolated nodes
-- * Update references in the Relation table.
--
CREATE OR REPLACE FUNCTION topology.ST_AddEdgeModFace(atopology varchar, anode integer, anothernode integer, acurve geometry)
  RETURNS INTEGER AS
$$
DECLARE
  rec RECORD;
  rrec RECORD;
  i INTEGER;
  topoid INTEGER;
  az FLOAT8;
  span RECORD; -- start point analysis data
  epan RECORD; --   end point analysis data
  fan RECORD; -- face analisys
  newedge RECORD; -- informations about new edge
  sql TEXT;
  newfaces INTEGER[];
  newface INTEGER;
  do_add BOOLEAN;
  bounds GEOMETRY;
  p1 GEOMETRY;
  p2 GEOMETRY;
BEGIN

  --
  -- All args required
  -- 
  IF atopology IS NULL
    OR anode IS NULL
    OR anothernode IS NULL
    OR acurve IS NULL
  THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - null argument';
  END IF;

  --
  -- Acurve must be a LINESTRING
  --
  IF substring(geometrytype(acurve), 1, 4) != 'LINE'
  THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - invalid curve';
  END IF;
  
  --
  -- Curve must be simple
  --
  IF NOT ST_IsSimple(acurve) THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - curve not simple';
  END IF;

  --
  -- Get topology id
  --
  BEGIN
    SELECT id FROM topology.topology
      INTO STRICT topoid WHERE name = atopology;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN
        RAISE EXCEPTION 'SQL/MM Spatial exception - invalid topology name';
  END;

  -- Initialize new edge info (will be filled up more later)
  SELECT anode as start_node, anothernode as end_node, acurve as geom,
    NULL::int as next_left_edge, NULL::int as next_right_edge,
    NULL::int as left_face, NULL::int as right_face, NULL::int as edge_id,
    NULL::int as prev_left_edge, NULL::int as prev_right_edge, -- convenience
    anode = anothernode as isclosed, -- convenience
    false as start_node_isolated, -- convenience
    false as end_node_isolated, -- convenience
    ST_RemoveRepeatedPoints(acurve) as cleangeom -- convenience
  INTO newedge;

  -- Compute azimut of first edge end on start node
  SELECT null::int AS nextCW, null::int AS nextCCW,
         null::float8 AS minaz, null::float8 AS maxaz,
         false AS was_isolated,
         ST_Azimuth(ST_StartPoint(newedge.cleangeom),
                    ST_PointN(newedge.cleangeom, 2)) AS myaz
  INTO span;
  IF span.myaz IS NULL THEN
    RAISE EXCEPTION 'Invalid edge (no two distinct vertices exist)';
  END IF;

  -- Compute azimuth of last edge end on end node
  SELECT null::int AS nextCW, null::int AS nextCCW,
         null::float8 AS minaz, null::float8 AS maxaz,
         false AS was_isolated,
         ST_Azimuth(ST_EndPoint(newedge.cleangeom),
                    ST_PointN(newedge.cleangeom,
                              ST_NumPoints(newedge.cleangeom)-1)) AS myaz
  INTO epan;
  IF epan.myaz IS NULL THEN
    RAISE EXCEPTION 'Invalid edge (no two distinct vertices exist)';
  END IF;


  -- 
  -- Check endpoints existance, match with Curve geometry
  -- and get face information (if any)
  --
  i := 0;
  FOR rec IN EXECUTE 'SELECT node_id, '
    || ' CASE WHEN node_id = ' || anode
    || ' THEN 1 WHEN node_id = ' || anothernode
    || ' THEN 0 END AS start, containing_face, geom FROM '
    || quote_ident(atopology)
    || '.node '
    || ' WHERE node_id IN ( '
    || anode || ',' || anothernode
    || ')'
  LOOP
    IF rec.containing_face IS NOT NULL THEN
      RAISE DEBUG  'containing_face for node %:%',
        rec.node_id, rec.containing_face;
      IF newedge.left_face IS NULL THEN
        newedge.left_face := rec.containing_face;
        newedge.right_face := rec.containing_face;
      ELSE
        IF newedge.left_face != rec.containing_face THEN
          RAISE EXCEPTION
            'SQL/MM Spatial exception - geometry crosses an edge (endnodes in faces % and %)', newedge.left_face, rec.containing_face;
        END IF;
      END IF;
    END IF;

    IF rec.start THEN
      IF NOT Equals(rec.geom, ST_StartPoint(acurve))
      THEN
        RAISE EXCEPTION
          'SQL/MM Spatial exception - start node not geometry start point.';
      END IF;
    ELSE
      IF NOT Equals(rec.geom, ST_EndPoint(acurve))
      THEN
        RAISE EXCEPTION
          'SQL/MM Spatial exception - end node not geometry end point.';
      END IF;
    END IF;

    i := i + 1;
  END LOOP;

  IF NOT newedge.isclosed THEN
    IF i < 2 THEN
    RAISE EXCEPTION
     'SQL/MM Spatial exception - non-existent node';
    END IF;
  ELSE
    IF i < 1 THEN
    RAISE EXCEPTION
     'SQL/MM Spatial exception - non-existent node';
    END IF;
  END IF;

  --
  -- Check if this geometry crosses any node
  --
  FOR rec IN EXECUTE
    'SELECT node_id, ST_Relate(geom, '
    || quote_literal(acurve::text) || '::geometry, 2) as relate FROM '
    || quote_ident(atopology)
    || '.node WHERE geom && '
    || quote_literal(acurve::text)
    || '::geometry'
  LOOP
    IF ST_RelateMatch(rec.relate, 'T********') THEN
      RAISE EXCEPTION 'SQL/MM Spatial exception - geometry crosses a node';
    END IF;
  END LOOP;

  --
  -- Check if this geometry has any interaction with any existing edge
  --
  FOR rec IN EXECUTE 'SELECT edge_id, ST_Relate(geom,' 
    || quote_literal(acurve::text)
    || '::geometry, 2) as im FROM '
    || quote_ident(atopology)
    || '.edge_data WHERE geom && '
    || quote_literal(acurve::text) || '::geometry'
  LOOP

    --RAISE DEBUG 'IM=%',rec.im;

    IF ST_RelateMatch(rec.im, 'F********') THEN
      CONTINUE; -- no interior intersection
    END IF;

    IF ST_RelateMatch(rec.im, '1FFF*FFF2') THEN
      RAISE EXCEPTION
        'SQL/MM Spatial exception - coincident edge';
    END IF;

    -- NOT IN THE SPECS: geometry touches an edge
    IF ST_RelateMatch(rec.im, '1********') THEN
      RAISE EXCEPTION
        'Spatial exception - geometry intersects edge %', rec.edge_id;
    END IF;

    IF ST_RelateMatch(rec.im, 'T********') THEN
      RAISE EXCEPTION
        'SQL/MM Spatial exception - geometry crosses an edge';
    END IF;

  END LOOP;

  ---------------------------------------------------------------
  --
  -- All checks passed, time to prepare the new edge
  --
  ---------------------------------------------------------------

  EXECUTE 'SELECT nextval(' || quote_literal(
      quote_ident(atopology) || '.edge_data_edge_id_seq') || ')'
  INTO STRICT newedge.edge_id;


  -- Find links on start node -- {

  RAISE DEBUG 'My start-segment azimuth: %', span.myaz;

  sql :=
    'SELECT edge_id, -1 AS end_node, start_node, left_face, right_face, '
    || 'ST_RemoveRepeatedPoints(geom) as geom FROM '
    || quote_ident(atopology)
    || '.edge_data WHERE start_node = ' || anode
    || ' UNION SELECT edge_id, end_node, -1, left_face, right_face, '
    || 'ST_RemoveRepeatedPoints(geom) FROM '
    || quote_ident(atopology)
    || '.edge_data WHERE end_node = ' || anode;
  IF newedge.isclosed THEN
    sql := sql || ' UNION SELECT '
      || newedge.edge_id || ',' || newedge.end_node
      || ',-1,0,0,' -- pretend we start elsewhere
      || quote_literal(newedge.cleangeom::text);
  END IF;
  i := 0;
  FOR rec IN EXECUTE sql
  LOOP -- incident edges {

    i := i + 1;

    IF rec.start_node = anode THEN
      --
      -- Edge starts at our node, we compute
      -- azimuth from node to its second point
      --
      az := ST_Azimuth(ST_StartPoint(rec.geom), ST_PointN(rec.geom, 2));

    ELSE
      --
      -- Edge ends at our node, we compute
      -- azimuth from node to its second-last point
      --
      az := ST_Azimuth(ST_EndPoint(rec.geom),
                       ST_PointN(rec.geom, ST_NumPoints(rec.geom)-1));
      rec.edge_id := -rec.edge_id;

    END IF;

    IF az IS NULL THEN
      RAISE EXCEPTION 'Invalid edge % found (no two distinct nodes exist)',
        rec.edge_id;
    END IF;

    RAISE DEBUG 'Edge % - az % (%) - fl:% fr:%',
      rec.edge_id, az, az - span.myaz, rec.left_face, rec.right_face;

    az = az - span.myaz;
    IF az < 0 THEN
      az := az + 2*PI();
    END IF;

    -- RAISE DEBUG ' normalized az %', az;

    IF span.maxaz IS NULL OR az > span.maxaz THEN
      span.maxaz := az;
      span.nextCCW := rec.edge_id;
      IF abs(rec.edge_id) != newedge.edge_id THEN
        IF rec.edge_id < 0 THEN
          -- TODO: check for mismatch ?
          newedge.left_face := rec.left_face;
        ELSE
          -- TODO: check for mismatch ?
          newedge.left_face := rec.right_face;
        END IF;
      END IF;
    END IF;

    IF span.minaz IS NULL OR az < span.minaz THEN
      span.minaz := az;
      span.nextCW := rec.edge_id;
      IF abs(rec.edge_id) != newedge.edge_id THEN
        IF rec.edge_id < 0 THEN
          -- TODO: check for mismatch ?
          newedge.right_face := rec.right_face;
        ELSE
          -- TODO: check for mismatch ?
          newedge.right_face := rec.left_face;
        END IF;
      END IF;
    END IF;

    --RAISE DEBUG 'Closest edges: CW:%(%) CCW:%(%)', span.nextCW, span.minaz, span.nextCCW, span.maxaz;

  END LOOP; -- incident edges }

  RAISE DEBUG 'span ROW_COUNT: %', i;
  IF newedge.isclosed THEN
    IF i < 2 THEN span.was_isolated = true; END IF;
  ELSE
    IF i < 1 THEN span.was_isolated = true; END IF;
  END IF;

  IF span.nextCW IS NULL THEN
    -- This happens if the destination node is isolated
    newedge.next_right_edge := newedge.edge_id;
    newedge.prev_left_edge := -newedge.edge_id;
  ELSE
    newedge.next_right_edge := span.nextCW;
    newedge.prev_left_edge := -span.nextCCW;
  END IF;

  RAISE DEBUG 'edge:%', newedge.edge_id;
  RAISE DEBUG ' left:%, next:%, prev:%',
    newedge.left_face, newedge.next_left_edge, newedge.prev_left_edge;
  RAISE DEBUG ' right:%, next:%, prev:%',
    newedge.right_face, newedge.next_right_edge, newedge.prev_right_edge;

  -- } start_node analysis


  -- Find links on end_node {
      
  RAISE DEBUG 'My end-segment azimuth: %', epan.myaz;

  sql :=
    'SELECT edge_id, -1 as end_node, start_node, left_face, right_face, '
    || 'ST_RemoveRepeatedPoints(geom) as geom FROM '
    || quote_ident(atopology)
    || '.edge_data WHERE start_node = ' || anothernode
    || 'UNION SELECT edge_id, end_node, -1, left_face, right_face, '
    || 'ST_RemoveRepeatedPoints(geom) FROM '
    || quote_ident(atopology)
    || '.edge_data WHERE end_node = ' || anothernode;
  IF newedge.isclosed THEN
    sql := sql || ' UNION SELECT '
      || newedge.edge_id || ',' || -1 -- pretend we end elsewhere
      || ',' || newedge.start_node || ',0,0,'
      || quote_literal(newedge.cleangeom::text);
  END IF;
  i := 0;
  FOR rec IN EXECUTE sql
  LOOP -- incident edges {

    i := i + 1;

    IF rec.start_node = anothernode THEN
      --
      -- Edge starts at our node, we compute
      -- azimuth from node to its second point
      --
      az := ST_Azimuth(ST_StartPoint(rec.geom),
                       ST_PointN(rec.geom, 2));

    ELSE
      --
      -- Edge ends at our node, we compute
      -- azimuth from node to its second-last point
      --
      az := ST_Azimuth(ST_EndPoint(rec.geom),
        ST_PointN(rec.geom, ST_NumPoints(rec.geom)-1));
      rec.edge_id := -rec.edge_id;

    END IF;

    RAISE DEBUG 'Edge % - az % (%)', rec.edge_id, az, az - epan.myaz;

    az := az - epan.myaz;
    IF az < 0 THEN
      az := az + 2*PI();
    END IF;

    -- RAISE DEBUG ' normalized az %', az;

    IF epan.maxaz IS NULL OR az > epan.maxaz THEN
      epan.maxaz := az;
      epan.nextCCW := rec.edge_id;
      IF abs(rec.edge_id) != newedge.edge_id THEN
        IF rec.edge_id < 0 THEN
          -- TODO: check for mismatch ?
          newedge.right_face := rec.left_face;
        ELSE
          -- TODO: check for mismatch ?
          newedge.right_face := rec.right_face;
        END IF;
      END IF;
    END IF;

    IF epan.minaz IS NULL OR az < epan.minaz THEN
      epan.minaz := az;
      epan.nextCW := rec.edge_id;
      IF abs(rec.edge_id) != newedge.edge_id THEN
        IF rec.edge_id < 0 THEN
          -- TODO: check for mismatch ?
          newedge.left_face := rec.right_face;
        ELSE
          -- TODO: check for mismatch ?
          newedge.left_face := rec.left_face;
        END IF;
      END IF;
    END IF;

    --RAISE DEBUG 'Closest edges: CW:%(%) CCW:%(%)', epan.nextCW, epan.minaz, epan.nextCCW, epan.maxaz;

  END LOOP; -- incident edges }

  RAISE DEBUG 'epan ROW_COUNT: %', i;
  IF newedge.isclosed THEN
    IF i < 2 THEN epan.was_isolated = true; END IF;
  ELSE
    IF i < 1 THEN epan.was_isolated = true; END IF;
  END IF;

  IF epan.nextCW IS NULL THEN
    -- This happens if the destination node is isolated
    newedge.next_left_edge := -newedge.edge_id;
    newedge.prev_right_edge := newedge.edge_id;
  ELSE
    newedge.next_left_edge := epan.nextCW;
    newedge.prev_right_edge := -epan.nextCCW;
  END IF;

  -- } end_node analysis

  RAISE DEBUG 'edge:%', newedge.edge_id;
  RAISE DEBUG ' left:%, next:%, prev:%',
    newedge.left_face, newedge.next_left_edge, newedge.prev_left_edge;
  RAISE DEBUG ' right:%, next:%, prev:%',
    newedge.right_face, newedge.next_right_edge, newedge.prev_right_edge;

  ----------------------------------------------------------------------
  --
  -- If we don't have faces setup by now we must have encountered
  -- a malformed topology (no containing_face on isolated nodes, no
  -- left/right faces on adjacent edges or mismatching values)
  --
  ----------------------------------------------------------------------
  IF newedge.left_face != newedge.right_face THEN
    RAISE EXCEPTION 'Left(%)/right(%) faces mismatch: invalid topology ?', 
      newedge.left_face, newedge.right_face;
  END IF;
  IF newedge.left_face IS NULL THEN
    RAISE EXCEPTION 'Could not derive edge face from linked primitives: invalid topology ?';
  END IF;

  ----------------------------------------------------------------------
  --
  -- Polygonize the current edges (to see later if the addition
  -- of the new one created another ring)
  --
  ----------------------------------------------------------------------

  SELECT null::geometry as post, null::geometry as pre INTO fan;

  EXECUTE
    'SELECT ST_Polygonize(geom) FROM '
    || quote_ident(atopology) || '.edge_data WHERE left_face = '
    || newedge.left_face || ' OR right_face = ' || newedge.right_face
    INTO STRICT fan.pre;

  ----------------------------------------------------------------------
  --
  -- Insert the new edge, and update all linking
  --
  ----------------------------------------------------------------------

  -- Insert the new edge with what we have so far
  EXECUTE 'INSERT INTO ' || quote_ident(atopology) 
    || '.edge VALUES(' || newedge.edge_id
    || ',' || newedge.start_node
    || ',' || newedge.end_node
    || ',' || newedge.next_left_edge
    || ',' || newedge.next_right_edge
    || ',' || newedge.left_face
    || ',' || newedge.right_face
    || ',' || quote_literal(newedge.geom::geometry::text)
    || ')';

  -- Link prev_left_edge to us 
  -- (if it's not us already)
  IF abs(newedge.prev_left_edge) != newedge.edge_id THEN
    IF newedge.prev_left_edge > 0 THEN
      -- its next_left_edge is us
      EXECUTE 'UPDATE ' || quote_ident(atopology)
        || '.edge_data SET next_left_edge = '
        || newedge.edge_id
        || ', abs_next_left_edge = '
        || newedge.edge_id
        || ' WHERE edge_id = ' 
        || newedge.prev_left_edge;
    ELSE
      -- its next_right_edge is us
      EXECUTE 'UPDATE ' || quote_ident(atopology)
        || '.edge_data SET next_right_edge = '
        || newedge.edge_id
        || ', abs_next_right_edge = '
        || newedge.edge_id
        || ' WHERE edge_id = ' 
        || -newedge.prev_left_edge;
    END IF;
  END IF;

  -- Link prev_right_edge to us 
  -- (if it's not us already)
  IF abs(newedge.prev_right_edge) != newedge.edge_id THEN
    IF newedge.prev_right_edge > 0 THEN
      -- its next_left_edge is -us
      EXECUTE 'UPDATE ' || quote_ident(atopology)
        || '.edge_data SET next_left_edge = '
        || -newedge.edge_id
        || ', abs_next_left_edge = '
        || newedge.edge_id
        || ' WHERE edge_id = ' 
        || newedge.prev_right_edge;
    ELSE
      -- its next_right_edge is -us
      EXECUTE 'UPDATE ' || quote_ident(atopology)
        || '.edge_data SET next_right_edge = '
        || -newedge.edge_id
        || ', abs_next_right_edge = '
        || newedge.edge_id
        || ' WHERE edge_id = ' 
        || -newedge.prev_right_edge;
    END IF;
  END IF;

  -- NOT IN THE SPECS...
  -- set containing_face = null for start_node and end_node
  -- if they where isolated 
  IF span.was_isolated OR epan.was_isolated THEN
      EXECUTE 'UPDATE ' || quote_ident(atopology)
        || '.node SET containing_face = null WHERE node_id IN ('
        || anode || ',' || anothernode || ')';
  END IF;

  ----------------------------------------------------------------------
  --
  -- Polygonize the new edges and see if the addition created a new ring
  --
  ----------------------------------------------------------------------

  EXECUTE 'SELECT ST_Polygonize(geom) FROM '
    || quote_ident(atopology) || '.edge_data WHERE left_face = '
    || newedge.left_face || ' OR right_face = ' || newedge.right_face
    INTO STRICT fan.post;

  IF ST_NumGeometries(fan.pre) = ST_NumGeometries(fan.post) THEN
    -- No splits, all done
    RETURN newedge.edge_id;
  END IF;

  RAISE NOTICE 'ST_AddEdgeModFace: edge % splitted face %',
      newedge.edge_id, newedge.left_face;

  -- Call topology.AddFace for every face whose boundary contains the new edge
  --
  -- TODO: in presence of holes every hole would share a boundary
  --       with its shell, research on improving performance by avoiding
  --       the multiple scans.
  --
  p1 = ST_StartPoint(newedge.cleangeom);
  p2 = ST_PointN(newedge.cleangeom, 2);
  FOR rec IN SELECT geom FROM ST_Dump(fan.post)
             WHERE ST_Contains(
                ST_Boundary(geom),
                ST_MakeLine(p1, p2)
                )
  --
  -- TODO: order so to have the face on the _right_ side created first,
  --       see http://trac.osgeo.org/postgis/ticket/1205
  --
  LOOP -- {

    -- NOTE: the only difference with ST_AddEdgeNewFace here is
    --       that we want to retain the face on the right side of
    --       the new edge 
    --
    IF newedge.left_face != 0 THEN -- {

     RAISE DEBUG 'Checking face %', ST_AsText(rec.geom);

     -- Skip this if our edge is on the right side
     IF ST_IsEmpty(ST_GeometryN(
            ST_SharedPaths(ST_Boundary(ST_ForceRHR(rec.geom)),
                           ST_MakeLine(p1, p2)), 2))
     THEN
          -- We keep this face, but update its MBR
          sql := 'UPDATE ' || quote_ident(atopology)
            || '.face set mbr = ' || quote_literal(ST_Envelope(rec.geom)::text)
            || ' WHERE face_id = ' || newedge.left_face;
          EXECUTE sql;
          CONTINUE;
     END IF;

    END IF; -- }

    RAISE DEBUG 'Adding % face', ST_AsText(rec.geom);
    SELECT topology.AddFace(atopology, rec.geom, true) INTO newface;
    newfaces := array_append(newfaces, newface);

  END LOOP; --}
  RAISE DEBUG 'Added face: %', newface;

  IF newedge.left_face != 0 THEN -- {

    -- NOT IN THE SPECS:
    -- update TopoGeometry compositions to add newface
    sql := 'SELECT r.topogeo_id, r.layer_id FROM '
      || quote_ident(atopology)
      || '.relation r, topology.layer l '
      || ' WHERE l.topology_id = ' || topoid
      || ' AND l.level = 0 '
      || ' AND l.layer_id = r.layer_id '
      || ' AND r.element_id = ' || newedge.left_face
      || ' AND r.element_type = 3 ';
    --RAISE DEBUG 'SQL: %', sql;
    FOR rec IN EXECUTE sql
    LOOP
      RAISE DEBUG 'TopoGeometry % in layer % contained the face being split (%) - updating to contain also new face %', rec.topogeo_id, rec.layer_id, newedge.left_face, newface;

      -- Add reference to the other face
      sql := 'INSERT INTO ' || quote_ident(atopology)
        || '.relation VALUES( ' || rec.topogeo_id
        || ',' || rec.layer_id || ',' || newface || ', 3)';
      --RAISE DEBUG 'SQL: %', sql;
      EXECUTE sql;

    END LOOP;

  END IF; -- }

  RETURN newedge.edge_id;
END
$$
LANGUAGE 'plpgsql' VOLATILE;
--} ST_AddEdgeModFace

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.17
--
--  ST_InitTopoGeo(atopology)
--
CREATE OR REPLACE FUNCTION topology.ST_InitTopoGeo(atopology varchar)
RETURNS text
AS
$$
DECLARE
  rec RECORD;
  topology_id numeric;
BEGIN
  IF atopology IS NULL THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - null argument';
  END IF;

  FOR rec IN SELECT * FROM pg_namespace WHERE text(nspname) = atopology
  LOOP
    RAISE EXCEPTION 'SQL/MM Spatial exception - schema already exists';
  END LOOP;

  FOR rec IN EXECUTE 'SELECT topology.CreateTopology('
    ||quote_literal(atopology)|| ') as id'
  LOOP
    topology_id := rec.id;
  END LOOP;

  RETURN 'Topology-Geometry ' || quote_literal(atopology)
    || ' (id:' || topology_id || ') created.';
END
$$
LANGUAGE 'plpgsql' VOLATILE;
--} ST_InitTopoGeo

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.18
--
--  ST_CreateTopoGeo(atopology, acollection)
--}{
CREATE OR REPLACE FUNCTION topology.ST_CreateTopoGeo(atopology varchar, acollection geometry)
RETURNS text
AS
$$
DECLARE
  typ char(4);
  rec RECORD;
  ret int;
  nodededges GEOMETRY;
  points GEOMETRY;
  snode_id int;
  enode_id int;
  tolerance FLOAT8;
  topoinfo RECORD;
BEGIN

  IF atopology IS NULL OR acollection IS NULL THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - null argument';
  END IF;

  -- Get topology information
  BEGIN
    SELECT * FROM topology.topology
      INTO STRICT topoinfo WHERE name = atopology;
  EXCEPTION
    WHEN NO_DATA_FOUND THEN
      RAISE EXCEPTION 'SQL/MM Spatial exception - invalid topology name';
  END;

  -- Check SRID compatibility
  IF ST_SRID(acollection) != topoinfo.SRID THEN
    RAISE EXCEPTION 'Geometry SRID (%) does not match topology SRID (%)',
      ST_SRID(acollection), topoinfo.SRID;
  END IF;

  -- Verify pre-conditions (valid, empty topology schema exists)
  BEGIN -- {

    -- Verify the topology views in the topology schema to be empty
    FOR rec in EXECUTE
      'SELECT count(*) FROM '
      || quote_ident(atopology) || '.edge_data '
      || ' UNION ' ||
      'SELECT count(*) FROM '
      || quote_ident(atopology) || '.node '
    LOOP
      IF rec.count > 0 THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - non-empty view';
      END IF;
    END LOOP;

    -- face check is separated as it will contain a single (world)
    -- face record
    FOR rec in EXECUTE
      'SELECT count(*) FROM '
      || quote_ident(atopology) || '.face '
    LOOP
      IF rec.count != 1 THEN
    RAISE EXCEPTION 'SQL/MM Spatial exception - non-empty face view';
      END IF;
    END LOOP;

  EXCEPTION
    WHEN INVALID_SCHEMA_NAME THEN
      RAISE EXCEPTION 'SQL/MM Spatial exception - invalid topology name';
    WHEN UNDEFINED_TABLE THEN
      RAISE EXCEPTION 'SQL/MM Spatial exception - non-existent view';

  END; -- }

  RAISE DEBUG 'Noding input linework';

  --
  -- Node input linework with itself
  --
  WITH components AS ( SELECT geom FROM ST_Dump(acollection) )
  SELECT ST_UnaryUnion(ST_Collect(geom)) FROM (
    SELECT geom FROM components
      WHERE ST_Dimension(geom) = 1
    UNION ALL
    SELECT ST_Boundary(geom) FROM components
      WHERE ST_Dimension(geom) = 2
  ) as linework INTO STRICT nodededges;

  RAISE DEBUG 'Computed % noded edges', ST_NumGeometries(nodededges);

  --
  -- Linemerge the resulting edges, to reduce the working set
  -- NOTE: this is more of a workaround for GEOS splitting overlapping
  --       lines to each of the segments.
  --
  SELECT ST_LineMerge(nodededges) INTO STRICT nodededges;

  RAISE DEBUG 'Merged edges: %', ST_NumGeometries(nodededges);


  --
  -- Collect input points and input lines endpoints
  --
  WITH components AS ( SELECT geom FROM ST_Dump(acollection) )
  SELECT ST_Union(geom) FROM (
    SELECT geom FROM components
      WHERE ST_Dimension(geom) = 0
    UNION ALL
    SELECT ST_Boundary(geom) FROM components
      WHERE ST_Dimension(geom) = 1
  ) as nodes INTO STRICT points;

  RAISE DEBUG 'Collected % input points', ST_NumGeometries(points);

  --
  -- Further split edges by points
  -- TODO: optimize this adding ST_Split support for multiline/multipoint
  --
  FOR rec IN SELECT geom FROM ST_Dump(points)
  LOOP
    -- Use the node to split edges
    SELECT ST_Collect(geom) 
    FROM ST_Dump(ST_Split(nodededges, rec.geom))
    INTO STRICT nodededges;
  END LOOP;
  SELECT ST_UnaryUnion(nodededges) INTO STRICT nodededges;

  RAISE DEBUG 'Noded edges became % after point-split',
    ST_NumGeometries(nodededges);

  --
  -- Collect all nodes (from points and noded linework endpoints)
  --

  WITH edges AS ( SELECT geom FROM ST_Dump(nodededges) )
  SELECT ST_Union( -- TODO: ST_UnaryUnion ?
          COALESCE(ST_UnaryUnion(ST_Collect(geom)), 
            ST_SetSRID('POINT EMPTY'::geometry, topoinfo.SRID)),
          COALESCE(points,
            ST_SetSRID('POINT EMPTY'::geometry, topoinfo.SRID))
         )
  FROM (
    SELECT ST_StartPoint(geom) as geom FROM edges
      UNION
    SELECT ST_EndPoint(geom) FROM edges
  ) as endpoints INTO points;

  RAISE DEBUG 'Total nodes count: %', ST_NumGeometries(points);

  --
  -- Add all nodes as isolated so that 
  -- later calls to AddEdgeModFace will tweak their being
  -- isolated or not...
  --
  FOR rec IN SELECT geom FROM ST_Dump(points)
  LOOP
    PERFORM topology.ST_AddIsoNode(atopology, 0, rec.geom);
  END LOOP;
  

  FOR rec IN SELECT geom FROM ST_Dump(nodededges)
  LOOP
    SELECT topology.GetNodeByPoint(atopology, st_startpoint(rec.geom), 0)
      INTO STRICT snode_id;
    SELECT topology.GetNodeByPoint(atopology, st_endpoint(rec.geom), 0)
      INTO STRICT enode_id;
    PERFORM topology.ST_AddEdgeModFace(atopology, snode_id, enode_id, rec.geom);
  END LOOP;

  RETURN 'Topology ' || atopology || ' populated';

END
$$
LANGUAGE 'plpgsql' VOLATILE;
--} ST_CreateTopoGeo

--=}  SQL/MM block

