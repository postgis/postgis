-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-- 
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.refractions.net
--
-- Copyright (C) 2010 Sandro Santilli <strk@keybit.net>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- Functions used to populate a topology
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
-- AddNode(atopology, point)
--
-- Add a node primitive to a topology and get it's identifier.
-- Returns an existing node at the same location, if any.
--
-- When adding a _new_ node it checks for the existance of any
-- edge crossing the given point, raising an exception if found.
--
-- The newly added nodes have no containing face.
--
-- 
CREATE OR REPLACE FUNCTION topology.AddNode(varchar, geometry)
	RETURNS int
AS
$$
DECLARE
	atopology ALIAS FOR $1;
	apoint ALIAS FOR $2;
	nodeid int;
	rec RECORD;
BEGIN
	--
	-- Atopology and apoint are required
	-- 
	IF atopology IS NULL OR apoint IS NULL THEN
		RAISE EXCEPTION 'Invalid null argument';
	END IF;

	--
	-- Apoint must be a point
	--
	IF substring(geometrytype(apoint), 1, 5) != 'POINT'
	THEN
		RAISE EXCEPTION 'Node geometry must be a point';
	END IF;

	--
	-- Check if a coincident node already exists
	-- 
	-- We use index AND x/y equality
	--
	FOR rec IN EXECUTE 'SELECT node_id FROM '
		|| quote_ident(atopology) || '.node ' ||
		'WHERE geom && ' || quote_literal(apoint::text) || '::geometry'
		||' AND x(geom) = x('||quote_literal(apoint::text)||'::geometry)'
		||' AND y(geom) = y('||quote_literal(apoint::text)||'::geometry)'
	LOOP
		RETURN  rec.node_id;
	END LOOP;

	--
	-- Check if any edge crosses this node
	-- (endpoints are fine)
	--
	FOR rec IN EXECUTE 'SELECT edge_id FROM '
		|| quote_ident(atopology) || '.edge ' 
		|| 'WHERE ST_Crosses(' || quote_literal(apoint::text) 
		|| ', geom)'
	LOOP
		RAISE EXCEPTION
		'An edge crosses the given node.';
	END LOOP;

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
	EXECUTE 'INSERT INTO ' || quote_ident(atopology)
		|| '.node(node_id, geom) 
		VALUES('||nodeid||','||quote_literal(apoint::text)||
		')';

	RETURN nodeid;
	
END
$$
LANGUAGE 'plpgsql';
--} AddNode

--{
--
-- AddEdge(atopology, line)
--
-- Add an edge primitive to a topology and get it's identifier.
-- Edge endpoints will be added as nodes if missing.
--
-- An exception is raised if the given line crosses an existing
-- node or interects with an existing edge on anything but endnodes.
-- TODO: for symmetry with AddNode, it might be useful to return the
--       id of an equal existing edge (currently raising an exception)
--
-- The newly added edge has "universe" face on both sides
-- and links to itself.
-- Calling code is expected to do further linking.
--
-- 
CREATE OR REPLACE FUNCTION topology.AddEdge(varchar, geometry)
	RETURNS int
AS
$$
DECLARE
	atopology ALIAS FOR $1;
	aline ALIAS FOR $2;
	edgeid int;
	rec RECORD;
BEGIN
	--
	-- Atopology and apoint are required
	-- 
	IF atopology IS NULL OR aline IS NULL THEN
		RAISE EXCEPTION 'Invalid null argument';
	END IF;

	--
	-- Aline must be a linestring
	--
	IF substring(geometrytype(aline), 1, 4) != 'LINE'
	THEN
		RAISE EXCEPTION 'Edge geometry must be a linestring';
	END IF;

	--
	-- Check if the edge crosses an existing node
	--
	FOR rec IN EXECUTE 'SELECT node_id FROM '
		|| quote_ident(atopology) || '.node '
		|| 'WHERE ST_Crosses('
		|| quote_literal(aline::text) || '::geometry, geom'
		|| ')'
	LOOP
		RAISE EXCEPTION 'Edge crosses node %', rec.node_id;
	END LOOP;

	--
	-- Check if the edge intersects an existing edge
	-- on anything but endpoints
	--
	-- Following DE-9 Intersection Matrix represent
	-- the only relation we accept. 
	--
	--    F F 1
	--    F * *
	--    1 * 2
	--
	-- Example1: linestrings touching at one endpoint
	--    FF1F00102
	--    FF1F**1*2 <-- our match
	--
	-- Example2: linestrings touching at both endpoints
	--    FF1F0F1F2
	--    FF1F**1*2 <-- our match
	--
	FOR rec IN EXECUTE 'SELECT edge_id, geom FROM '
		|| quote_ident(atopology) || '.edge '
		|| 'WHERE '
		|| quote_literal(aline::text) || '::geometry && geom'
		|| ' AND NOT ST_Relate('
		|| quote_literal(aline::text)
		|| '::geometry, geom, ''FF1F**1*2'''
		|| ')'

	LOOP
		-- TODO: reuse an EQUAL edge ?
		RAISE EXCEPTION 'Edge intersects (not on endpoints) with existing edge % ', rec.edge_id;
	END LOOP;

	--
	-- Get new edge id from sequence
	--
	FOR rec IN EXECUTE 'SELECT nextval(''' ||
		atopology || '.edge_data_edge_id_seq'')'
	LOOP
		edgeid = rec.nextval;
	END LOOP;

	--
	-- Insert the new row
	--
	EXECUTE 'INSERT INTO '
		|| quote_ident(atopology)
		|| '.edge(edge_id, start_node, end_node, '
		|| 'next_left_edge, next_right_edge, '
		|| 'left_face, right_face, '
		|| 'geom) '
		|| ' VALUES('

		-- edge_id
		|| edgeid ||','

		-- start_node
		|| 'topology.addNode('
		|| quote_literal(atopology)
		|| ', ST_StartPoint('
		|| quote_literal(aline::text)
		|| ')) ,'

		-- end_node
		|| 'topology.addNode('
		|| quote_literal(atopology)
		|| ', ST_EndPoint('
		|| quote_literal(aline::text)
		|| ')) ,'

		-- next_left_edge
		|| edgeid ||','

		-- next_right_edge
		|| edgeid ||','

		-- left_face
		|| '0,'

		-- right_face
		|| '0,'

		-- geom
		||quote_literal(aline::text)
		|| ')';

	RETURN edgeid;
	
END
$$
LANGUAGE 'plpgsql';
--} AddEdge

--{
--
-- AddFace(atopology, poly)
--
-- Add a face primitive to a topology and get it's identifier.
-- Returns an existing face at the same location, if any.
--
-- For a newly added face, its edges will be appropriately
-- linked (marked as left-face or right-face).
--
-- The target topology is assumed to be valid (containing no
-- self-intersecting edges).
--
-- An exception is raised if:
--  o The polygon boundary is not fully defined by existing edges.
--  o The polygon overlaps an existing face.
--
-- 
CREATE OR REPLACE FUNCTION topology.AddFace(varchar, geometry)
	RETURNS int
AS
$$
DECLARE
	atopology ALIAS FOR $1;
	apoly ALIAS FOR $2;
	bounds geometry;
	symdif geometry;
	faceid int;
	rec RECORD;
	relate text;
	right_edges int[];
	left_edges int[];
	all_edges geometry;
	old_faceid int;
	old_edgeid int;
BEGIN
	--
	-- Atopology and apoly are required
	-- 
	IF atopology IS NULL OR apoly IS NULL THEN
		RAISE EXCEPTION 'Invalid null argument';
	END IF;

	--
	-- Aline must be a polygon
	--
	IF substring(geometrytype(apoly), 1, 4) != 'POLY'
	THEN
		RAISE EXCEPTION 'Face geometry must be a polygon';
	END IF;

	--
	-- Find all bounds edges, forcing right-hand-rule
	-- to know what's left and what's right...
	--
	bounds = ST_Boundary(ST_ForceRHR(apoly));

	FOR rec IN EXECUTE 'SELECT geom, edge_id, '
		|| 'left_face, right_face, '
		|| '(st_dump(st_sharedpaths(geom, '
		|| quote_literal(bounds::text)
		|| '))).path[1] as side FROM '
		|| quote_ident(atopology) || '.edge '
		|| 'WHERE '
		|| quote_literal(bounds::text)
		|| '::geometry && geom AND ST_Relate(geom, '
		|| quote_literal(bounds::text)
		|| ', ''1FF******'')'
	LOOP
		RAISE DEBUG 'Edge % (left:%, right:%) - side : %',
			rec.edge_id, rec.left_face, rec.right_face, rec.side;

		IF rec.side = 1 THEN
			-- This face is on the right
			right_edges := array_append(right_edges, rec.edge_id);
			old_faceid = rec.right_face;
		ELSE
			-- This face is on the left
			left_edges := array_append(left_edges, rec.edge_id);
			old_faceid = rec.left_face;
		END IF;

		IF faceid IS NULL OR faceid = 0 THEN
			faceid = old_faceid;
			old_edgeid = rec.edge_id;
		ELSIF faceid != old_faceid THEN
			RAISE EXCEPTION 'Edge % has face % registered on the side of this face, while edge % has face % on the same side', rec.edge_id, old_faceid, old_edgeid, faceid;
		END IF;

		-- Collect all edges for final full coverage check
		all_edges = ST_Collect(all_edges, rec.geom);

	END LOOP;

	IF all_edges IS NULL THEN
		RAISE EXCEPTION 'Found no edges on the polygon boundary';
	END IF;

	RAISE DEBUG 'Left edges: %', left_edges;
	RAISE DEBUG 'Right edges: %', right_edges;

	--
	-- Check that all edges found, taken togheter,
	-- fully match the polygon boundary and nothing more
	--
	-- If the test fail either we need to add more edges
	-- from the polygon boundary or we need to split
	-- some of the existing ones.
	-- 
	IF NOT ST_isEmpty(ST_SymDifference(bounds, all_edges)) THEN
	  IF NOT ST_isEmpty(ST_Difference(bounds, all_edges)) THEN
	    RAISE EXCEPTION 'Polygon boundary is not fully defined by existing edges';
	  ELSE
	    RAISE EXCEPTION 'Existing edges cover polygon boundary and more! (invalid topology?)';
	  END IF;
	END IF;

--	EXECUTE 'SELECT ST_Collect(geom) FROM'
--		|| quote_ident(atopology)
--		|| '.edge_data '
--		|| ' WHERE edge_id = ANY('
--		|| quote_literal(array_append(left_edges, right_edges))
--		|| ') ';

	--
	-- TODO:
	-- Check that NO edge is contained in the face ?
	--
	RAISE WARNING 'Not checking if face contains any edge';
 

	IF faceid IS NOT NULL AND faceid != 0 THEN
		RAISE DEBUG 'Face already known as %', faceid;
		RETURN faceid;
	END IF;

	--
	-- Get new face id from sequence
	--
	FOR rec IN EXECUTE 'SELECT nextval(''' ||
		atopology || '.face_face_id_seq'')'
	LOOP
		faceid = rec.nextval;
	END LOOP;

	--
	-- Insert new face 
	--
	EXECUTE 'INSERT INTO '
		|| quote_ident(atopology)
		|| '.face(face_id, mbr) VALUES('
		-- face_id
		|| faceid || ','
		-- minimum bounding rectangle
		|| quote_literal(Box2d(apoly))
		|| ')';

	--
	-- Update all edges having this face on the left
	--
	EXECUTE 'UPDATE '
		|| quote_ident(atopology)
		|| '.edge_data SET left_face = '
		|| quote_literal(faceid)
		|| ' WHERE edge_id = ANY('
		|| quote_literal(left_edges)
		|| ') ';

	--
	-- Update all edges having this face on the right
	--
	EXECUTE 'UPDATE '
		|| quote_ident(atopology)
		|| '.edge_data SET right_face = '
		|| quote_literal(faceid)
		|| ' WHERE edge_id = ANY('
		|| quote_literal(right_edges)
		|| ') ';


	RETURN faceid;
	
END
$$
LANGUAGE 'plpgsql';
--} AddFace
