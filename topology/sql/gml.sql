-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-- 
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.refractions.net
--
-- Copyright (C) 2010, 2011 Sandro Santilli <strk@keybit.net>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- Functions used for topology GML output
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- Developed by Sandro Santilli <strk@keybit.net>
-- for Faunalia (http://www.faunalia.it) with funding from
-- Regione Toscana - Sistema Informativo per la Gestione del Territorio
-- e dell' Ambiente [RT-SIGTA].
-- For the project: "Sviluppo strumenti software per il trattamento di dati
-- geografici basati su QuantumGIS e Postgis (CIG 0494241492)"
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

--{
--
-- INTERNAL FUNCTION
-- text _AsGMLNode(id, point, nsprefix, precision, options)
--
-- }{
CREATE OR REPLACE FUNCTION topology._AsGMLNode(int, geometry, text, int, int)
  RETURNS text
AS
$$
DECLARE
  id ALIAS FOR $1;
  point ALIAS FOR $2;
  nsprefix_in ALIAS FOR $3;
  nsprefix text;
  precision ALIAS FOR $4;
  options ALIAS FOR $5;
  gml text;
BEGIN

  nsprefix := 'gml:';
  IF NOT nsprefix_in IS NULL THEN
    IF nsprefix_in = '' THEN
      nsprefix = nsprefix_in;
    ELSE
      nsprefix = nsprefix_in || ':';
    END IF;
  END IF;

  gml := '<' || nsprefix || 'Node ' || nsprefix || 'id="N' || id || '"';
  IF point IS NOT NULL THEN
    gml = gml || '>'
              || '<' || nsprefix || 'pointProperty>'
              || ST_AsGML(3, point, precision, options, nsprefix_in)
              || '</' || nsprefix || 'pointProperty>'
              || '</' || nsprefix || 'Node>';
  ELSE
    gml = gml || '/>';
  END IF;
  RETURN gml;
END
$$
LANGUAGE 'plpgsql';
--} _AsGMLNode(id, point, nsprefix, precision, options)

--{
--
-- INTERNAL FUNCTION
-- text _AsGMLEdge(edge_id, start_node, end_node, line, visitedTable,
--                 nsprefix, precision, options)
--
-- }{
CREATE OR REPLACE FUNCTION topology._AsGMLEdge(int, int, int, geometry, regclass, text, int, int)
  RETURNS text
AS
$$
DECLARE
  edge_id ALIAS FOR $1;
  start_node ALIAS FOR $2;
  end_node ALIAS for $3;
  line ALIAS FOR $4;
  visitedTable ALIAS FOR $5;
  visited bool;
  nsprefix_in ALIAS FOR $6;
  nsprefix text;
  precision ALIAS FOR $7;
  options ALIAS FOR $8;
  gml text;
BEGIN

  nsprefix := 'gml:';
  IF nsprefix_in IS NOT NULL THEN
    IF nsprefix_in = '' THEN
      nsprefix = nsprefix_in;
    ELSE
      nsprefix = nsprefix_in || ':';
    END IF;
  END IF;

  gml := '<' || nsprefix || 'Edge ' || nsprefix || 'id="E' || edge_id || '">';

  -- Start node
  gml = gml || '<' || nsprefix || 'directedNode orientation="-"';
  -- Do visited bookkeeping if visitedTable was given
  visited = NULL;
  IF visitedTable IS NOT NULL THEN
    EXECUTE 'SELECT true FROM '
            || visitedTable::text
            || ' WHERE element_type = 1 AND element_id = '
            || start_node LIMIT 1 INTO visited;
    IF visited IS NOT NULL THEN
      gml = gml || ' xlink:href="#N' || start_node || '" />';
    ELSE
      -- Mark as visited 
      EXECUTE 'INSERT INTO ' || visitedTable::text
        || '(element_type, element_id) VALUES (1, '
        || start_node || ')';
    END IF;
  END IF;
  IF visited IS NULL THEN
    gml = gml || '>';
    gml = gml || topology._AsGMLNode(start_node, NULL, nsprefix_in,
                                     precision, options);
    gml = gml || '</' || nsprefix || 'directedNode>';
  END IF;

  -- End node
  gml = gml || '<' || nsprefix || 'directedNode';
  -- Do visited bookkeeping if visitedTable was given
  visited = NULL;
  IF visitedTable IS NOT NULL THEN
    EXECUTE 'SELECT true FROM '
            || visitedTable::text
            || ' WHERE element_type = 1 AND element_id = '
            || end_node LIMIT 1 INTO visited;
    IF visited IS NOT NULL THEN
      gml = gml || ' xlink:href="#N' || end_node || '" />';
    ELSE
      -- Mark as visited 
      EXECUTE 'INSERT INTO ' || visitedTable::text
        || '(element_type, element_id) VALUES (1, '
        || end_node || ')';
    END IF;
  END IF;
  IF visited IS NULL THEN
    gml = gml || '>';
    gml = gml || topology._AsGMLNode(end_node, NULL, nsprefix_in,
                                     precision, options);
    gml = gml || '</' || nsprefix || 'directedNode>';
  END IF;

  IF line IS NOT NULL THEN
    gml = gml || '<' || nsprefix || 'curveProperty>'
              || ST_AsGML(3, line, precision, options, nsprefix_in)
              || '</' || nsprefix || 'curveProperty>';
  END IF;

  gml = gml || '</' || nsprefix || 'Edge>';

  RETURN gml;
END
$$
LANGUAGE 'plpgsql';
--} _AsGMLEdge(id, start_node, end_node, line, visitedTable, nsprefix, precision, options)

--{
--
-- API FUNCTION
--
-- text AsGML(TopoGeometry, nsprefix, precision, options, visitedTable)
--
-- }{
CREATE OR REPLACE FUNCTION topology.AsGML(topology.TopoGeometry,
    text, int, int, regclass)
  RETURNS text
AS
$$
DECLARE
  tg ALIAS FOR $1;
  nsprefix_in ALIAS FOR $2;
  nsprefix text;
  precision_in ALIAS FOR $3;
  precision int;
  options_in ALIAS FOR $4;
  options int;
  visitedTable ALIAS FOR $5;
  visited bool;
  toponame text;
  gml text;
  sql text;
  rec RECORD;
  rec2 RECORD;
  bounds geometry;
  side int;
BEGIN

  nsprefix := 'gml:';
  IF nsprefix_in IS NOT NULL THEN
    IF nsprefix_in = '' THEN
      nsprefix = nsprefix_in;
    ELSE
      nsprefix = nsprefix_in || ':';
    END IF;
  END IF;

  precision := 15;
  IF precision_in IS NOT NULL THEN
    precision = precision_in;
  END IF;

  options := 1;
  IF options_in IS NOT NULL THEN
    options = options_in;
  END IF;

  -- Get topology name (for subsequent queries)
  SELECT name FROM topology.topology into toponame
              WHERE id = tg.topology_id;

  -- Puntual TopoGeometry
  IF tg.type = 1 THEN
    gml = '<' || nsprefix || 'TopoPoint>';
    -- For each defining node, print a directedNode
    FOR rec IN  EXECUTE 'SELECT r.element_id, n.geom from '
      || quote_ident(toponame) || '.relation r LEFT JOIN '
      || quote_ident(toponame) || '.node n ON (r.element_id = n.node_id)'
      || ' WHERE r.layer_id = ' || tg.layer_id
      || ' AND r.topogeo_id = ' || tg.id
    LOOP
      gml = gml || '<' || nsprefix || 'directedNode';
      -- Do visited bookkeeping if visitedTable was given
      IF visitedTable IS NOT NULL THEN
        EXECUTE 'SELECT true FROM '
                || visitedTable::text
                || ' WHERE element_type = 1 AND element_id = '
                || rec.element_id LIMIT 1 INTO visited;
        IF visited IS NOT NULL THEN
          gml = gml || ' xlink:href="#N' || rec.element_id || '" />';
          CONTINUE;
        ELSE
          -- Mark as visited 
          EXECUTE 'INSERT INTO ' || visitedTable::text
            || '(element_type, element_id) VALUES (1, '
            || rec.element_id || ')';
        END IF;
      END IF;
      gml = gml || '>';
      gml = gml || topology._AsGMLNode(rec.element_id, rec.geom, nsprefix_in, precision, options);
      gml = gml || '</' || nsprefix || 'directedNode>';
    END LOOP;
    gml = gml || '</' || nsprefix || 'TopoPoint>';
    RETURN gml;

  ELSIF tg.type = 2 THEN -- lineal
    gml = '<' || nsprefix || 'TopoCurve>';

    FOR rec IN SELECT (ST_Dump(topology.Geometry(tg))).geom
    LOOP
      FOR rec2 IN EXECUTE
        'SELECT e.*, ST_Line_Locate_Point('
        || quote_literal(rec.geom::text)
        || ', ST_Line_Interpolate_Point(e.geom, 0.2)) as pos FROM '
        || quote_ident(toponame)
        || '.edge e WHERE ST_Covers('
        || quote_literal(rec.geom::text)
        || ', e.geom) ORDER BY pos'
        -- TODO: add relation to the conditional, to reduce load ?
      LOOP

        gml = gml || '<' || nsprefix || 'directedEdge';

        -- if this edge goes in opposite direction to the
        --       line, make it with negative orientation
        SELECT DISTINCT (ST_Dump(
                          ST_SharedPaths(rec2.geom, rec.geom))
                        ).path[1] into side;
        IF side = 2 THEN -- edge goes in opposite direction
          gml = gml || ' orientation="-"';
        END IF;

        -- Do visited bookkeeping if visitedTable was given
        IF visitedTable IS NOT NULL THEN

          EXECUTE 'SELECT true FROM '
            || visitedTable::text
            || ' WHERE element_type = 2 AND element_id = '
            || rec2.edge_id LIMIT 1 INTO visited;
          IF visited THEN
            -- Use xlink:href if visited
            gml = gml || ' xlink:href="#E' || rec2.edge_id || '" />';
            CONTINUE;
          ELSE
            -- Mark as visited otherwise
            EXECUTE 'INSERT INTO ' || visitedTable::text
              || '(element_type, element_id) VALUES (2, '
              || rec2.edge_id || ')';
          END IF;

        END IF;


        gml = gml || '>';

        gml = gml || topology._AsGMLEdge(rec2.edge_id,
                                        rec2.start_node,
                                        rec2.end_node, rec2.geom,
                                        visitedTable,
                                        nsprefix_in, precision,
                                        options);


        gml = gml || '</' || nsprefix || 'directedEdge>';
      END LOOP;
    END LOOP;

    gml = gml || '</' || nsprefix || 'TopoCurve>';
    return gml;

  ELSIF tg.type = 3 THEN -- areal
    gml = '<' || nsprefix || 'TopoSurface>';

    -- Construct the geometry, then for each polygon:
    FOR rec IN SELECT (ST_DumpRings((ST_Dump(topology.Geometry(tg))).geom)).*
    LOOP
      -- print a directedFace for each exterior ring
      -- and a negative directedFace for
      -- each interior ring.
      IF rec.path[1] = 0 THEN
        gml = gml || '<' || nsprefix || 'directedFace>';
      ELSE
        gml = gml || '<' || nsprefix || 'directedFace orientation="-">';
      END IF;

      -- TODO: make all this block in a specialized _AsGMLRing function ?

      -- Contents of a directed face are the list of edges
      -- that cover the specific ring
      bounds = ST_Boundary(rec.geom);

      -- TODO: figure out a way to express an id for a face
      --       and use a reference for an already-seen face ?
      gml = gml || '<' || nsprefix || 'Face>';
      FOR rec2 IN EXECUTE
        'SELECT e.*, ST_Line_Locate_Point('
        || quote_literal(bounds::text)
        || ', ST_Line_Interpolate_Point(e.geom, 0.2)) as pos FROM '
        || quote_ident(toponame)
        || '.edge e WHERE ST_Covers('
        || quote_literal(bounds::text)
        || ', e.geom) ORDER BY pos'
        -- TODO: add left_face/right_face to the conditional, to reduce load ?
      LOOP

        gml = gml || '<' || nsprefix || 'directedEdge';

        -- if this edge goes in opposite direction to the
        --       ring bounds, make it with negative orientation
        SELECT DISTINCT (ST_Dump(
                          ST_SharedPaths(rec2.geom, bounds))
                        ).path[1] into side;
        IF side = 2 THEN -- edge goes in opposite direction
          gml = gml || ' orientation="-"';
        END IF;

        -- Do visited bookkeeping if visitedTable was given
        IF visitedTable IS NOT NULL THEN

          EXECUTE 'SELECT true FROM '
            || visitedTable::text
            || ' WHERE element_type = 2 AND element_id = '
            || rec2.edge_id LIMIT 1 INTO visited;
          IF visited THEN
            -- Use xlink:href if visited
            gml = gml || ' xlink:href="#E' || rec2.edge_id || '" />';
            CONTINUE;
          ELSE
            -- Mark as visited otherwise
            EXECUTE 'INSERT INTO ' || visitedTable::text
              || '(element_type, element_id) VALUES (2, '
              || rec2.edge_id || ')';
          END IF;

        END IF;

        gml = gml || '>';

        gml = gml || topology._AsGMLEdge(rec2.edge_id,
                                        rec2.start_node,
                                        rec2.end_node, rec2.geom,
                                        visitedTable,
                                        nsprefix_in,
                                        precision, options);
        gml = gml || '</' || nsprefix || 'directedEdge>';

      END LOOP;
      gml = gml || '</' || nsprefix || 'Face>';
      gml = gml || '</' || nsprefix || 'directedFace>';
    END LOOP;
  
    gml = gml || '</' || nsprefix || 'TopoSurface>';
    RETURN gml;

  ELSIF tg.type = 4 THEN -- collection
    RAISE EXCEPTION 'Collection TopoGeometries are not supported by AsGML';

  END IF;
	

  RETURN gml;
	
END
$$
LANGUAGE 'plpgsql';
--} AsGML(TopoGeometry, nsprefix, precision, options, visitedTable)

--{
--
-- API FUNCTION 
--
-- text AsGML(TopoGeometry, nsprefix, precision, options)
--
-- }{
CREATE OR REPLACE FUNCTION topology.AsGML(topology.TopoGeometry, text, int, int)
  RETURNS text AS
$$
 SELECT topology.AsGML($1, $2, $3, $4, NULL);
$$ LANGUAGE 'sql';
-- } AsGML(TopoGeometry, nsprefix, precision, options)

--{
--
-- API FUNCTION 
--
-- text AsGML(TopoGeometry, nsprefix)
--
-- }{
CREATE OR REPLACE FUNCTION topology.AsGML(topology.TopoGeometry, text)
  RETURNS text AS
$$
 SELECT topology.AsGML($1, $2, 15, 1, NULL);
$$ LANGUAGE 'sql';
-- } AsGML(TopoGeometry, nsprefix)

--{
--
-- API FUNCTION
--
-- text AsGML(TopoGeometry, visited_table)
--
-- }{
CREATE OR REPLACE FUNCTION topology.AsGML(topology.TopoGeometry, regclass)
  RETURNS text AS
$$
 SELECT topology.AsGML($1, 'gml', 15, 1, $2);
$$ LANGUAGE 'sql';
-- } AsGML(TopoGeometry, visited_table)

--{
--
-- API FUNCTION
--
-- text AsGML(TopoGeometry)
--
-- }{
CREATE OR REPLACE FUNCTION topology.AsGML(topology.TopoGeometry)
  RETURNS text AS
$$
 SELECT topology.AsGML($1, 'gml');
$$ LANGUAGE 'sql';
-- } AsGML(TopoGeometry)

