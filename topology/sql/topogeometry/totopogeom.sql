-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-- 
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.refractions.net
--
-- Copyright (C) 2012 Sandro Santilli <strk@keybit.net>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

-- {
--  Convert a simple geometry to a topologically-defined one
--
--  See http://trac.osgeo.org/postgis/ticket/1017
--
-- }{
CREATE OR REPLACE FUNCTION topology.toTopoGeom(geom Geometry, toponame varchar, layer_id int, tolerance float8 DEFAULT 0)
  RETURNS topology.TopoGeometry
AS
$$
DECLARE
  layer_info RECORD;
  topology_info RECORD;
BEGIN

  -- Get topology information
  BEGIN
    SELECT * FROM topology.topology
      INTO STRICT topology_info WHERE name = toponame;
  EXCEPTION
    WHEN NO_DATA_FOUND THEN
      RAISE EXCEPTION 'No topology with name "%" in topology.topology',
        toponame;
  END;

  -- Get layer information
  BEGIN
    SELECT * FROM topology.layer l
      INTO STRICT layer_info
      WHERE l.layer_id = layer_id
      AND l.topology_id = topology_info.id;
  EXCEPTION
    WHEN NO_DATA_FOUND THEN
      RAISE EXCEPTION 'No layer with id "%" in topology "%"',
        layer_id, toponame;
  END;

  -- Can't convert to a hierarchical topogeometry
  IF layer_info.level > 0 THEN
      RAISE EXCEPTION 'Layer "%" of topology "%" is hierarchical, cannot convert to it',
        layer_id, toponame;
  END IF;

  -- 
  -- TODO: Check type compatibility 
  --  A point can go in puntal or collection layer
  --  A line can go in lineal or collection layer
  --  An area can go in areal or collection layer
  --  A collection can only go collection layer
  --  What to do with EMPTies ?
  -- 

  RAISE EXCEPTION 'toTopoGeometry not implemented yet';

END
$$
LANGUAGE 'plpgsql' STABLE STRICT;
-- }
