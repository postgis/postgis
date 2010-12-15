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
-- Author: Sandro Santilli <strk@refractions.net>
--  
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- Functions used to populate a topology
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


--{
--
-- AddNode(atopology, point)
--
-- Add a node primitive to a topology and get it's identifier.
-- Returns an existing node at the same location, if any.
--
-- When adding a _new_ node, it checks for existance of any
-- edge crossing the given point, and bails out if any was found.
--
-- 
CREATEFUNCTION topology.AddNode(varchar, geometry)
	RETURNS int
AS
'
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
		RAISE EXCEPTION ''Invalid null argument'';
	END IF;

	--
	-- Apoint must be a point
	--
	IF substring(geometrytype(apoint), 1, 5) != ''POINT''
	THEN
		RAISE EXCEPTION ''Node geometry must be a point'';
	END IF;

	--
	-- Check if a coincident node already exists
	-- 
	-- We use index AND x/y equality
	--
	FOR rec IN EXECUTE ''SELECT node_id FROM ''
		|| quote_ident(atopology) || ''.node '' ||
		''WHERE geom && '' || quote_literal(apoint::text) || ''::geometry''
		||'' AND x(geom) = x(''||quote_literal(apoint::text)||''::geometry)''
		||'' AND y(geom) = y(''||quote_literal(apoint::text)||''::geometry)''
	LOOP
		RETURN  rec.node_id;
	END LOOP;

	--
	-- Check if any edge crosses (intersects) this node
	-- I used _intersects_ here to include boundaries (endpoints)
	--
	FOR rec IN EXECUTE ''SELECT edge_id FROM ''
		|| quote_ident(atopology) || ''.edge '' 
		|| ''WHERE geom && '' || quote_literal(apoint::text) 
		|| '' AND intersects(geom, '' || quote_literal(apoint::text)
		|| '')''
	LOOP
		RAISE EXCEPTION
		''An edge crosses the given node.'';
	END LOOP;

	--
	-- Get new node id from sequence
	--
	FOR rec IN EXECUTE ''SELECT nextval('''''' ||
		atopology || ''.node_node_id_seq'''')''
	LOOP
		nodeid = rec.nextval;
	END LOOP;

	--
	-- Insert the new row
	--
	EXECUTE ''INSERT INTO '' || quote_ident(atopology)
		|| ''.node(node_id, geom) 
		VALUES(''||nodeid||'',''||quote_literal(apoint::text)||
		'')'';

	RETURN nodeid;
	
END
'
LANGUAGE 'plpgsql';
--} AddNode
