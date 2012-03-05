-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-- 
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.refractions.net
--
-- Copyright (C) 2011-2012 Sandro Santilli <strk@keybit.net>
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
CREATE OR REPLACE FUNCTION topology.toTopoGeom(ageom Geometry, atopology varchar, alayer int, atolerance float8 DEFAULT 0)
  RETURNS topology.TopoGeometry
AS
$$
DECLARE
  layer_info RECORD;
  topology_info RECORD;
  rec RECORD;
  tg topology.TopoGeometry;
  elems topology.TopoElementArray = '{{0,0}}';
  sql TEXT;
  typ TEXT;
  tolerance FLOAT8;
BEGIN

  -- Get topology information
  BEGIN
    SELECT *
    FROM topology.topology
      INTO STRICT topology_info WHERE name = atopology;
  EXCEPTION
    WHEN NO_DATA_FOUND THEN
      RAISE EXCEPTION 'No topology with name "%" in topology.topology',
        atopology;
  END;

  -- Get tolerance, if 0 was given
  tolerance := COALESCE( NULLIF(tolerance, 0), topology._st_mintolerance(atopology, ageom) );

  -- Get layer information
  BEGIN
    SELECT *, CASE
      WHEN feature_type = 1 THEN 'puntal'
      WHEN feature_type = 2 THEN 'lineal'
      WHEN feature_type = 3 THEN 'areal'
      WHEN feature_type = 4 THEN 'mixed'
      ELSE 'unexpected_'||feature_type
      END as typename
    FROM topology.layer l
      INTO STRICT layer_info
      WHERE l.layer_id = alayer
      AND l.topology_id = topology_info.id;
  EXCEPTION
    WHEN NO_DATA_FOUND THEN
      RAISE EXCEPTION 'No layer with id "%" in topology "%"',
        alayer, atopology;
  END;

  -- Can't convert to a hierarchical topogeometry
  IF layer_info.level > 0 THEN
      RAISE EXCEPTION 'Layer "%" of topology "%" is hierarchical, cannot convert to it.',
        alayer, atopology;
  END IF;


  -- 
  -- Check type compatibility and create empty TopoGeometry
  -- 1:puntal, 2:lineal, 3:areal, 4:collection
  --
  typ = geometrytype(ageom);
  IF typ = 'GEOMETRYCOLLECTION' THEN
    --  A collection can only go collection layer
    IF layer_info.feature_type != 4 THEN
      RAISE EXCEPTION
        'Layer "%" of topology "%" is %, cannot hold a collection feature.',
        layer_info.layer_id, topology_info.name, layer_info.typename;
    END IF;
    tg := topology.CreateTopoGeom(atopology, 4, alayer);
  ELSIF typ = 'POINT' OR typ = 'MULTIPOINT' THEN -- puntal
    --  A point can go in puntal or collection layer
    IF layer_info.feature_type != 4 and layer_info.feature_type != 1 THEN
      RAISE EXCEPTION
        'Layer "%" of topology "%" is %, cannot hold a puntal feature.',
        layer_info.layer_id, topology_info.name, layer_info.typename;
    END IF;
    tg := topology.CreateTopoGeom(atopology, 1, alayer);
  ELSIF typ = 'LINESTRING' or typ = 'MULTILINESTRING' THEN -- lineal
    --  A line can go in lineal or collection layer
    IF layer_info.feature_type != 4 and layer_info.feature_type != 2 THEN
      RAISE EXCEPTION
        'Layer "%" of topology "%" is %, cannot hold a lineal feature.',
        layer_info.layer_id, topology_info.name, layer_info.typename;
    END IF;
    tg := topology.CreateTopoGeom(atopology, 2, alayer);
  ELSIF typ = 'POLYGON' OR typ = 'MULTIPOLYGON' THEN -- areal
    --  An area can go in areal or collection layer
    IF layer_info.feature_type != 4 and layer_info.feature_type != 3 THEN
      RAISE EXCEPTION
        'Layer "%" of topology "%" is %, cannot hold an areal feature.',
        layer_info.layer_id, topology_info.name, layer_info.typename;
    END IF;
    tg := topology.CreateTopoGeom(atopology, 3, alayer);
  ELSE
      -- Should never happen
      RAISE EXCEPTION
        'Unexpected feature dimension %', ST_Dimension(ageom);
  END IF;

  -- Now that we have a topogeometry, we loop over components 
  -- and add them to the definition of it. We add them as soon
  -- as possible so that each element can further edit the
  -- definition by splitting
  FOR rec IN SELECT (ST_Dump(ageom)).geom LOOP
    IF ST_IsEmpty(rec.geom) THEN
      RAISE DEBUG 'Skipped empty component';
    ELSIF ST_Dimension(rec.geom) = 0 THEN
      sql := 'INSERT INTO ' || quote_ident(atopology)
        || '.relation(topogeo_id, layer_id, element_type, element_id) SELECT '
        || id(tg) || ', ' || alayer || ', 1, topogeo_addPoint('
        || quote_literal(atopology) || ', '
        || quote_literal(rec.geom::text) || '::geometry, ' || tolerance
        || ');';
      --RAISE DEBUG '%', sql;
      EXECUTE sql;
    ELSIF ST_Dimension(rec.geom) = 1 THEN
      sql := 'INSERT INTO ' || quote_ident(atopology)
        || '.relation(topogeo_id, layer_id, element_type, element_id) SELECT '
        || id(tg) || ', ' || alayer || ', 2, topogeo_addLineString('
        || quote_literal(atopology) || ', '
        || quote_literal(rec.geom::text) || '::geometry, ' || tolerance
        || ');';
      --RAISE DEBUG '%', sql;
      EXECUTE sql;
    ELSIF ST_Dimension(rec.geom) = 2 THEN
      sql := 'INSERT INTO ' || quote_ident(atopology)
        || '.relation(topogeo_id, layer_id, element_type, element_id) SELECT '
        || id(tg) || ', ' || alayer || ', 3, topogeo_addPolygon('
        || quote_literal(atopology) || ', '
        || quote_literal(rec.geom::text) || '::geometry, ' || tolerance
        || ');';
      --RAISE DEBUG '%', sql;
      EXECUTE sql;
    ELSE
      RAISE EXCEPTION 'Unexpected dimension % for component %', ST_Dimension(rec.geom), ST_AsText(rec.geom);
    END IF;

  END LOOP;

  RETURN tg;

END
$$
LANGUAGE 'plpgsql' VOLATILE STRICT;
-- }
