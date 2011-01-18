-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-- 
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.refractions.net
--
-- Copyright (C) 2010 Sandro Santilli <strk@keybit.net>
-- Copyright (C) 2005 Refractions Research Inc.
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- Author: Sandro Santilli <strk@refractions.net>
--  
-- 

--={ ----------------------------------------------------------------
--  SQL/MM block
--
--  This part contains function in the SQL/MM specification
--
---------------------------------------------------------------------


--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.5
--
--  ST_GetFaceEdges(atopology, aface)
--
--
-- 
CREATE OR REPLACE FUNCTION topology.ST_GetFaceEdges(varchar, integer)
	RETURNS setof topology.GetFaceEdges_ReturnType
AS
'
DECLARE
	atopology ALIAS FOR $1;
	aface ALIAS FOR $2;
	rec RECORD;
BEGIN
	--
	-- Atopology and aface are required
	-- 
	IF atopology IS NULL OR aface IS NULL THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - null argument'';
	END IF;

	RAISE EXCEPTION
		 ''ST_GetFaceEdges: not implemented yet'';


END
'
LANGUAGE 'plpgsql' VOLATILE;
--} ST_GetFaceEdges


--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.16
--
--  ST_GetFaceGeometry(atopology, aface)
-- 
CREATE OR REPLACE FUNCTION topology.ST_GetFaceGeometry(varchar, integer)
	RETURNS GEOMETRY AS
'
DECLARE
	atopology ALIAS FOR $1;
	aface ALIAS FOR $2;
	rec RECORD;
BEGIN
	--
	-- Atopology and aface are required
	-- 
	IF atopology IS NULL OR aface IS NULL THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - null argument'';
	END IF;

	--
	-- Construct face 
	-- 
	FOR rec IN EXECUTE ''SELECT ST_BuildArea(ST_Collect(geom)) FROM ''
		|| quote_ident(atopology)
		|| ''.edge WHERE left_face = '' || aface || 
		'' OR right_face = '' || aface 
	LOOP
		RETURN rec.st_buildarea;
	END LOOP;


	--
	-- No face found
	-- 
	RAISE EXCEPTION
	''SQL/MM Spatial exception - non-existent face.'';
END
' 
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
'
DECLARE
	atopology ALIAS FOR $1;
	aface ALIAS FOR $2;
	apoint ALIAS FOR $3;
	rec RECORD;
	nodeid integer;
BEGIN

	--
	-- Atopology and apoint are required
	-- 
	IF atopology IS NULL OR apoint IS NULL THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - null argument'';
	END IF;

	--
	-- Apoint must be a point
	--
	IF substring(geometrytype(apoint), 1, 5) != ''POINT''
	THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - invalid point'';
	END IF;

	--
	-- Check if a coincident node already exists
	-- 
	-- We use index AND x/y equality
	--
	FOR rec IN EXECUTE ''SELECT node_id FROM ''
		|| quote_ident(atopology) || ''.node '' ||
		''WHERE geom && '' || quote_literal(apoint::text) || ''::geometry''
		||'' AND ST_X(geom) = ST_X(''||quote_literal(apoint::text)||''::geometry)''
		||'' AND ST_Y(geom) = ST_Y(''||quote_literal(apoint::text)||''::geometry)''
	LOOP
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - coincident node'';
	END LOOP;

	--
	-- Check if any edge crosses (intersects) this node
	-- I used _intersects_ here to include boundaries (endpoints)
	--
	FOR rec IN EXECUTE ''SELECT edge_id FROM ''
		|| quote_ident(atopology) || ''.edge '' 
		|| ''WHERE geom && '' || quote_literal(apoint::text) 
		|| '' AND ST_Intersects(geom, '' || quote_literal(apoint::text)
		|| ''::geometry)''
	LOOP
		RAISE EXCEPTION
		''SQL/MM Spatial exception - edge crosses node.'';
	END LOOP;


	--
	-- Verify that aface contains apoint
	-- if aface is null no check is done
	--
	IF aface IS NOT NULL THEN

	FOR rec IN EXECUTE ''SELECT ST_Within(''
		|| quote_literal(apoint::text) || ''::geometry, 
		topology.ST_GetFaceGeometry(''
		|| quote_literal(atopology) || '', '' || aface ||
		'')) As within''
	LOOP
		IF rec.within = ''f'' THEN
			RAISE EXCEPTION
			''SQL/MM Spatial exception - not within face'';
		ELSIF rec.within IS NULL THEN
			RAISE EXCEPTION
			''SQL/MM Spatial exception - non-existent face'';
		END IF;
	END LOOP;

	END IF;

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
	IF aface IS NOT NULL THEN
		EXECUTE ''INSERT INTO '' || quote_ident(atopology)
			|| ''.node(node_id, geom, containing_face) 
			VALUES(''||nodeid||'',''||quote_literal(apoint::text)||
			'',''||aface||'')'';
	ELSE
		EXECUTE ''INSERT INTO '' || quote_ident(atopology)
			|| ''.node(node_id, geom) 
			VALUES(''||nodeid||'',''||quote_literal(apoint::text)||
			'')'';
	END IF;

	RETURN nodeid;
END
'
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
'
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
		 ''SQL/MM Spatial exception - null argument'';
	END IF;

	--
	-- Check node isolation.
	-- 
	FOR rec IN EXECUTE ''SELECT edge_id FROM ''
		|| quote_ident(atopology) || ''.edge_data '' ||
		'' WHERE start_node =  '' || anode ||
		'' OR end_node = '' || anode 
	LOOP
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - not isolated node'';
	END LOOP;

	EXECUTE ''DELETE FROM '' || quote_ident(atopology) || ''.node ''
		|| '' WHERE node_id = '' || anode;

	RETURN ''Isolated node '' || anode || '' removed'';
END
'
LANGUAGE 'plpgsql' VOLATILE;
--} ST_RemoveIsoNode

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.7 
--
--  ST_RemoveIsoEdge(atopology, anedge)
--
CREATE OR REPLACE FUNCTION topology.ST_RemoveIsoEdge(varchar, integer)
	RETURNS TEXT AS
'
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
		 ''SQL/MM Spatial exception - null argument'';
	END IF;

	--
	-- Check node existance
	-- 
	ok = false;
	FOR edge IN EXECUTE ''SELECT * FROM ''
		|| quote_ident(atopology) || ''.edge_data '' ||
		'' WHERE edge_id =  '' || anedge
	LOOP
		ok = true;
	END LOOP;
	IF NOT ok THEN
		RAISE EXCEPTION
		  ''SQL/MM Spatial exception - non-existent edge'';
	END IF;

	--
	-- Check node isolation
	-- 
	IF edge.left_face != edge.right_face THEN
		RAISE EXCEPTION
		  ''SQL/MM Spatial exception - not isolated edge'';
	END IF;

	FOR rec IN EXECUTE ''SELECT * FROM ''
		|| quote_ident(atopology) || ''.edge_data '' 
		|| '' WHERE edge_id !=  '' || anedge
		|| '' AND ( start_node = '' || edge.start_node
		|| '' OR start_node = '' || edge.end_node
		|| '' OR end_node = '' || edge.start_node
		|| '' OR end_node = '' || edge.end_node
		|| '' ) ''
	LOOP
		RAISE EXCEPTION
		  ''SQL/MM Spatial exception - not isolated edge'';
	END LOOP;

	--
	-- Delete the edge
	--
	EXECUTE ''DELETE FROM '' || quote_ident(atopology) || ''.edge_data ''
		|| '' WHERE edge_id = '' || anedge;

	RETURN ''Isolated edge '' || anedge || '' removed'';
END
'
LANGUAGE 'plpgsql' VOLATILE;
--} ST_RemoveIsoEdge

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.8 
--
--  ST_NewEdgesSplit(atopology, anedge, apoint)
--
CREATE OR REPLACE FUNCTION topology.ST_NewEdgesSplit(varchar, integer, geometry)
	RETURNS INTEGER AS
'
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
		 ''SQL/MM Spatial exception - null argument'';
	END IF;

	--
	-- Check node existance
	-- 
	ok = false;
	FOR oldedge IN EXECUTE ''SELECT * FROM ''
		|| quote_ident(atopology) || ''.edge_data '' ||
		'' WHERE edge_id =  '' || anedge
	LOOP
		ok = true;
	END LOOP;
	IF NOT ok THEN
		RAISE EXCEPTION
		  ''SQL/MM Spatial exception - non-existent edge'';
	END IF;

	--
	-- Check that given point is Within(anedge.geom)
	-- 
	IF NOT ST_Within(apoint, oldedge.geom) THEN
		RAISE EXCEPTION
		  ''SQL/MM Spatial exception - point not on edge'';
	END IF;

	--
	-- Check if a coincident node already exists
	--
	FOR rec IN EXECUTE ''SELECT node_id FROM ''
		|| quote_ident(atopology) || ''.node ''
		|| ''WHERE geom && ''
		|| quote_literal(apoint::text) || ''::geometry''
		|| '' AND ST_X(geom) = ST_X(''
		|| quote_literal(apoint::text) || ''::geometry)''
		|| '' AND ST_Y(geom) = ST_Y(''
		|| quote_literal(apoint::text) || ''::geometry)''
	LOOP
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - coincident node'';
	END LOOP;

	--
	-- Get new node id
	--
	FOR rec IN EXECUTE ''SELECT nextval('''''' ||
		atopology || ''.node_node_id_seq'''')''
	LOOP
		nodeid = rec.nextval;
	END LOOP;

	--RAISE NOTICE ''Next node id = % '', nodeid;

	--
	-- Add the new node 
	--
	EXECUTE ''INSERT INTO '' || quote_ident(atopology)
		|| ''.node(node_id, geom) 
		VALUES('' || nodeid || '',''
		|| quote_literal(apoint::text)
		|| '')'';

	--
	-- Delete the old edge
	--
	EXECUTE ''DELETE FROM '' || quote_ident(atopology) || ''.edge_data ''
		|| '' WHERE edge_id = '' || anedge;

	--
	-- Compute new edges
	--
	nodepos = ST_Line_locate_point(oldedge.geom, apoint);
	edge1 = ST_Line_substring(oldedge.geom, 0, nodepos);
	edge2 = ST_Line_substring(oldedge.geom, nodepos, 1);

	--
	-- Get ids for the new edges 
	--
	FOR rec IN EXECUTE ''SELECT nextval('''''' ||
		atopology || ''.edge_data_edge_id_seq'''')''
	LOOP
		edgeid1 = rec.nextval;
	END LOOP;
	FOR rec IN EXECUTE ''SELECT nextval('''''' ||
		atopology || ''.edge_data_edge_id_seq'''')''
	LOOP
		edgeid2 = rec.nextval;
	END LOOP;

	--RAISE NOTICE ''EdgeId1 % EdgeId2 %'', edgeid1, edgeid2;

	--
	-- Insert the two new edges
	--
	EXECUTE ''INSERT INTO '' || quote_ident(atopology)
		|| ''.edge VALUES(''
		||edgeid1||'',''||oldedge.start_node
		||'',''||nodeid
		||'',''||edgeid2
		||'',''||oldedge.next_right_edge
		||'',''||oldedge.left_face
		||'',''||oldedge.right_face
		||'',''||quote_literal(edge1::text)
		||'')'';

	EXECUTE ''INSERT INTO '' || quote_ident(atopology)
		|| ''.edge VALUES(''
		||edgeid2||'',''||nodeid
		||'',''||oldedge.end_node
		||'',''||oldedge.next_left_edge
		||'',-''||edgeid1
		||'',''||oldedge.left_face
		||'',''||oldedge.right_face
		||'',''||quote_literal(edge2::text)
		||'')'';

	--
	-- Update all next edge references to match new layout
	--

	EXECUTE ''UPDATE '' || quote_ident(atopology)
		|| ''.edge_data SET next_right_edge = ''
		|| edgeid2
		|| '',''
		|| '' abs_next_right_edge = '' || edgeid2
		|| '' WHERE next_right_edge = '' || anedge;
	EXECUTE ''UPDATE '' || quote_ident(atopology)
		|| ''.edge_data SET next_right_edge = ''
		|| -edgeid1
		|| '',''
		|| '' abs_next_right_edge = '' || edgeid1
		|| '' WHERE next_right_edge = '' || -anedge;

	EXECUTE ''UPDATE '' || quote_ident(atopology)
		|| ''.edge_data SET next_left_edge = ''
		|| edgeid1
		|| '',''
		|| '' abs_next_left_edge = '' || edgeid1
		|| '' WHERE next_left_edge = '' || anedge;
	EXECUTE ''UPDATE '' || quote_ident(atopology)
		|| ''.edge_data SET ''
		|| '' next_left_edge = '' || -edgeid2
		|| '',''
		|| '' abs_next_left_edge = '' || edgeid2
		|| '' WHERE next_left_edge = '' || -anedge;

	-- Get topology id
        SELECT id FROM topology.topology into topoid
                WHERE name = atopology;
	IF topoid IS NULL THEN
		RAISE EXCEPTION ''No topology % registered'',
			quote_ident(atopology);
	END IF;

	--
	-- Update references in the Relation table.
	-- We only take into considerations non-hierarchical
	-- TopoGeometry here, for obvious reasons.
	--
	FOR rec IN EXECUTE ''SELECT r.* FROM ''
		|| quote_ident(atopology)
		|| ''.relation r, topology.layer l ''
		|| '' WHERE ''
		|| '' l.topology_id = '' || topoid
		|| '' AND l.level = 0 ''
		|| '' AND l.layer_id = r.layer_id ''
		|| '' AND abs(r.element_id) = '' || anedge
		|| '' AND r.element_type = 2''
	LOOP
		--RAISE NOTICE ''TopoGeometry % in layer % contains the edge being split'', rec.topogeo_id, rec.layer_id;

		-- Delete old reference
		EXECUTE ''DELETE FROM '' || quote_ident(atopology)
			|| ''.relation ''
			|| '' WHERE ''
			|| ''layer_id = '' || rec.layer_id
			|| '' AND ''
			|| ''topogeo_id = '' || rec.topogeo_id
			|| '' AND ''
			|| ''element_type = '' || rec.element_type
			|| '' AND ''
			|| ''abs(element_id) = '' || anedge;

		-- Add new reference to edge1
		IF rec.element_id < 0 THEN
			tmp = -edgeid1;
		ELSE
			tmp = edgeid1;
		END IF;
		EXECUTE ''INSERT INTO '' || quote_ident(atopology)
			|| ''.relation ''
			|| '' VALUES( ''
			|| rec.topogeo_id
			|| '',''
			|| rec.layer_id
			|| '',''
			|| tmp
			|| '',''
			|| rec.element_type
			|| '')'';

		-- Add new reference to edge2
		IF rec.element_id < 0 THEN
			tmp = -edgeid2;
		ELSE
			tmp = edgeid2;
		END IF;
		EXECUTE ''INSERT INTO '' || quote_ident(atopology)
			|| ''.relation ''
			|| '' VALUES( ''
			|| rec.topogeo_id
			|| '',''
			|| rec.layer_id
			|| '',''
			|| tmp
			|| '',''
			|| rec.element_type
			|| '')'';
			
	END LOOP;

	--RAISE NOTICE ''Edge % split in edges % and % by node %'',
	--	anedge, edgeid1, edgeid2, nodeid;

	RETURN nodeid; 
END
'
LANGUAGE 'plpgsql' VOLATILE;
--} ST_NewEdgesSplit

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.9 
--
--  ST_ModEdgesSplit(atopology, anedge, apoint)
--
CREATE OR REPLACE FUNCTION topology.ST_ModEdgesSplit(varchar, integer, geometry)
	RETURNS INTEGER AS
'
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
		 ''SQL/MM Spatial exception - null argument'';
	END IF;

	--
	-- Check node existance
	-- 
	ok = false;
	FOR oldedge IN EXECUTE ''SELECT * FROM ''
		|| quote_ident(atopology) || ''.edge_data '' ||
		'' WHERE edge_id =  '' || anedge
	LOOP
		ok = true;
	END LOOP;
	IF NOT ok THEN
		RAISE EXCEPTION
		  ''SQL/MM Spatial exception - non-existent edge'';
	END IF;

	--
	-- Check that given point is Within(anedge.geom)
	-- 
	IF NOT ST_Within(apoint, oldedge.geom) THEN
		RAISE EXCEPTION
		  ''SQL/MM Spatial exception - point not on edge'';
	END IF;

	--
	-- Check if a coincident node already exists
	--
	FOR rec IN EXECUTE ''SELECT node_id FROM ''
		|| quote_ident(atopology) || ''.node '' ||
		''WHERE geom && ''
		|| quote_literal(apoint::text) || ''::geometry''
		||'' AND ST_X(geom) = ST_X(''
		|| quote_literal(apoint::text) || ''::geometry)''
		||'' AND ST_Y(geom) = ST_Y(''
		||quote_literal(apoint::text)||''::geometry)''
	LOOP
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - coincident node'';
	END LOOP;

	--
	-- Get new node id
	--
	FOR rec IN EXECUTE ''SELECT nextval('''''' ||
		atopology || ''.node_node_id_seq'''')''
	LOOP
		nodeid = rec.nextval;
	END LOOP;

	--RAISE NOTICE ''Next node id = % '', nodeid;

	--
	-- Add the new node 
	--
	EXECUTE ''INSERT INTO '' || quote_ident(atopology)
		|| ''.node(node_id, geom) 
		VALUES(''||nodeid||'',''||quote_literal(apoint::text)||
		'')'';

	--
	-- Compute new edge
	--
	nodepos = line_locate_point(oldedge.geom, apoint);
	newedge1 = line_substring(oldedge.geom, 0, nodepos);
	newedge2 = line_substring(oldedge.geom, nodepos, 1);


	--
	-- Get ids for the new edge
	--
	FOR rec IN EXECUTE ''SELECT nextval('''''' ||
		atopology || ''.edge_data_edge_id_seq'''')''
	LOOP
		newedgeid = rec.nextval;
	END LOOP;

	--
	-- Insert the new edge
	--
	EXECUTE ''INSERT INTO '' || quote_ident(atopology)
		|| ''.edge ''
		|| ''(edge_id, start_node, end_node,''
		|| ''next_left_edge, next_right_edge,''
		|| ''left_face, right_face, geom) ''
		|| ''VALUES(''
		||newedgeid||'',''||nodeid
		||'',''||oldedge.end_node
		||'',''||oldedge.next_left_edge
		||'',-''||anedge
		||'',''||oldedge.left_face
		||'',''||oldedge.right_face
		||'',''||quote_literal(newedge2::text)
		||'')'';

	--
	-- Update the old edge
	--
	EXECUTE ''UPDATE '' || quote_ident(atopology) || ''.edge_data ''
		|| '' SET geom = '' || quote_literal(newedge1::text)
		|| '',''
		|| '' next_left_edge = '' || newedgeid
		|| '',''
		|| '' end_node = '' || nodeid
		|| '' WHERE edge_id = '' || anedge;


	--
	-- Update all next edge references to match new layout
	--

	EXECUTE ''UPDATE '' || quote_ident(atopology)
		|| ''.edge_data SET next_right_edge = ''
		|| -newedgeid 
		|| '',''
		|| '' abs_next_right_edge = '' || newedgeid
		|| '' WHERE next_right_edge = '' || -anedge;

	EXECUTE ''UPDATE '' || quote_ident(atopology)
		|| ''.edge_data SET ''
		|| '' next_left_edge = '' || -newedgeid
		|| '',''
		|| '' abs_next_left_edge = '' || newedgeid
		|| '' WHERE next_left_edge = '' || -anedge;

	-- Get topology id
        SELECT id FROM topology.topology into topoid
                WHERE name = atopology;

	--
	-- Update references in the Relation table.
	-- We only take into considerations non-hierarchical
	-- TopoGeometry here, for obvious reasons.
	--
	FOR rec IN EXECUTE ''SELECT r.* FROM ''
		|| quote_ident(atopology)
		|| ''.relation r, topology.layer l ''
		|| '' WHERE ''
		|| '' l.topology_id = '' || topoid
		|| '' AND l.level = 0 ''
		|| '' AND l.layer_id = r.layer_id ''
		|| '' AND abs(r.element_id) = '' || anedge
		|| '' AND r.element_type = 2''
	LOOP
		--RAISE NOTICE ''TopoGeometry % in layer % contains the edge being split (%) - updating to add new edge %'', rec.topogeo_id, rec.layer_id, anedge, newedgeid;

		-- Add new reference to edge1
		IF rec.element_id < 0 THEN
			tmp = -newedgeid;
		ELSE
			tmp = newedgeid;
		END IF;
		query = ''INSERT INTO '' || quote_ident(atopology)
			|| ''.relation ''
			|| '' VALUES( ''
			|| rec.topogeo_id
			|| '',''
			|| rec.layer_id
			|| '',''
			|| tmp
			|| '',''
			|| rec.element_type
			|| '')'';

		--RAISE NOTICE ''%'', query;
		EXECUTE query;
	END LOOP;

	--RAISE NOTICE ''Edge % split in edges % and % by node %'',
	--	anedge, anedge, newedgeid, nodeid;

	RETURN nodeid; 
END
'
LANGUAGE 'plpgsql' VOLATILE;
--} ST_ModEdgesSplit

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.4 
--
--  ST_AddIsoEdge(atopology, anode, anothernode, acurve)
-- 
CREATE OR REPLACE FUNCTION topology.ST_AddIsoEdge(varchar, integer, integer, geometry)
	RETURNS INTEGER AS
'
DECLARE
	atopology ALIAS FOR $1;
	anode ALIAS FOR $2;
	anothernode ALIAS FOR $3;
	acurve ALIAS FOR $4;
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
		 ''SQL/MM Spatial exception - null argument'';
	END IF;

	--
	-- Acurve must be a LINESTRING
	--
	IF substring(geometrytype(acurve), 1, 4) != ''LINE''
	THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - invalid curve'';
	END IF;

	--
	-- Acurve must be a simple
	--
	IF NOT ST_IsSimple(acurve)
	THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - curve not simple'';
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
	FOR rec IN EXECUTE ''SELECT geom, containing_face, node_id FROM ''
		|| quote_ident(atopology) || ''.node
		WHERE node_id = '' || anode ||
		'' OR node_id = '' || anothernode
	LOOP 
		IF count > 0 AND aface != rec.containing_face THEN
			RAISE EXCEPTION
			''SQL/MM Spatial exception - nodes in different faces'';
		ELSE
			aface := rec.containing_face;
		END IF;

		-- Get nodes geom
		IF rec.node_id = anode THEN
			snodegeom = rec.geom;
		ELSE
			enodegeom = rec.geom;
		END IF;

		count = count+1;

	END LOOP;
	IF count < 2 THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - non-existent node'';
	END IF;


	--
	-- Check nodes isolation.
	-- 
	FOR rec IN EXECUTE ''SELECT edge_id FROM ''
		|| quote_ident(atopology) || ''.edge_data '' ||
		'' WHERE start_node =  '' || anode ||
		'' OR end_node = '' || anode ||
		'' OR start_node = '' || anothernode ||
		'' OR end_node = '' || anothernode
	LOOP
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - not isolated node'';
	END LOOP;

	--
	-- Check acurve to be within endpoints containing face 
	-- (unless it is the world face, I suppose)
	-- 
	IF aface IS NOT NULL THEN

		--
		-- Extract endpoints face geometry
		--
		FOR rec IN EXECUTE ''SELECT topology.ST_GetFaceGeometry(''
			|| quote_literal(atopology) ||
			'','' || aface || '') as face''
		LOOP
			face := rec.face;
		END LOOP;

		--
		-- Check acurve to be within face
		--
		IF ! ST_Within(acurve, face) THEN
	RAISE EXCEPTION
	''SQL/MM Spatial exception - geometry not within face.'';
		END IF;

	END IF;

	--
	-- l) Check that start point of acurve match start node
	-- geoms.
	-- 
	IF ST_X(snodegeom) != ST_X(ST_StartPoint(acurve)) OR
	   ST_Y(snodegeom) != ST_Y(ST_StartPoint(acurve)) THEN
  RAISE EXCEPTION
  ''SQL/MM Spatial exception - start node not geometry start point.'';
	END IF;

	--
	-- m) Check that end point of acurve match end node
	-- geoms.
	-- 
	IF ST_X(enodegeom) != ST_X(ST_EndPoint(acurve)) OR
	   ST_Y(enodegeom) != ST_Y(ST_EndPoint(acurve)) THEN
  RAISE EXCEPTION
  ''SQL/MM Spatial exception - end node not geometry end point.'';
	END IF;

	--
	-- n) Check if curve crosses (contains) any node
	-- I used _contains_ here to leave endpoints out
	-- 
	FOR rec IN EXECUTE ''SELECT node_id FROM ''
		|| quote_ident(atopology) || ''.node ''
		|| '' WHERE geom && '' || quote_literal(acurve::text) 
		|| '' AND ST_Contains('' || quote_literal(acurve::text)
		|| '',geom)''
	LOOP
		RAISE EXCEPTION
		''SQL/MM Spatial exception - geometry crosses a node'';
	END LOOP;

	--
	-- o) Check if curve intersects any other edge
	-- 
	FOR rec IN EXECUTE ''SELECT * FROM ''
		|| quote_ident(atopology) || ''.edge_data
		WHERE geom && '' || quote_literal(acurve::text) || ''::geometry
		AND ST_Intersects(geom, '' || quote_literal(acurve::text) || ''::geometry)''
	LOOP
		RAISE EXCEPTION
		''SQL/MM Spatial exception - geometry intersects an edge'';
	END LOOP;

	--
	-- Get new edge id from sequence
	--
	FOR rec IN EXECUTE ''SELECT nextval('''''' ||
		atopology || ''.edge_data_edge_id_seq'''')''
	LOOP
		edgeid = rec.nextval;
	END LOOP;

	--
	-- Insert the new row
	--
	IF aface IS NULL THEN aface := 0; END IF;

	EXECUTE ''INSERT INTO '' || quote_ident(atopology)
		|| ''.edge VALUES(''||edgeid||'',''||anode||
			'',''||anothernode||'',''
			||(-edgeid)||'',''||edgeid||'',''
			||aface||'',''||aface||'',''
			||quote_literal(acurve::text)||'')'';

	RETURN edgeid;

END
'
LANGUAGE 'plpgsql' VOLATILE;
--} ST_AddIsoEdge

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.6
--
--  ST_ChangeEdgeGeom(atopology, anedge, acurve)
-- 
CREATE OR REPLACE FUNCTION topology.ST_ChangeEdgeGeom(varchar, integer, geometry)
	RETURNS TEXT AS
'
DECLARE
	atopology ALIAS FOR $1;
	anedge ALIAS FOR $2;
	acurve ALIAS FOR $3;
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
		 ''SQL/MM Spatial exception - null argument'';
	END IF;

	--
	-- Acurve must be a LINESTRING
	--
	IF substring(geometrytype(acurve), 1, 4) != ''LINE''
	THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - invalid curve'';
	END IF;

	--
	-- Acurve must be a simple
	--
	IF NOT ST_IsSimple(acurve)
	THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - curve not simple'';
	END IF;

	--
	-- e) Check StartPoint consistency
	--
	FOR rec IN EXECUTE ''SELECT * FROM ''
		|| quote_ident(atopology) || ''.edge_data e, ''
		|| quote_ident(atopology) || ''.node n ''
		|| '' WHERE e.edge_id = '' || anedge
		|| '' AND n.node_id = e.start_node ''
		|| '' AND ( ST_X(n.geom) != ''
		|| ST_X(ST_StartPoint(acurve))
		|| '' OR ST_Y(n.geom) != ''
		|| ST_Y(ST_StartPoint(acurve))
		|| '')''
	LOOP 
		RAISE EXCEPTION
		''SQL/MM Spatial exception - start node not geometry start point.'';
	END LOOP;

	--
	-- f) Check EndPoint consistency
	--
	FOR rec IN EXECUTE ''SELECT * FROM ''
		|| quote_ident(atopology) || ''.edge_data e, ''
		|| quote_ident(atopology) || ''.node n ''
		|| '' WHERE e.edge_id = '' || anedge
		|| '' AND n.node_id = e.end_node ''
		|| '' AND ( ST_X(n.geom) != ''
		|| ST_X(ST_EndPoint(acurve))
		|| '' OR ST_Y(n.geom) != ''
		|| ST_Y(ST_EndPoint(acurve))
		|| '')''
	LOOP 
		RAISE EXCEPTION
		''SQL/MM Spatial exception - end node not geometry end point.'';
	END LOOP;

	--
	-- g) Check if curve crosses any node
	-- _within_ used to let endpoints out
	-- 
	FOR rec IN EXECUTE ''SELECT node_id FROM ''
		|| quote_ident(atopology) || ''.node
		WHERE geom && '' || quote_literal(acurve::text) || ''::geometry
		AND ST_Within(geom, '' || quote_literal(acurve::text) || ''::geometry)''
	LOOP
		RAISE EXCEPTION
		''SQL/MM Spatial exception - geometry crosses a node'';
	END LOOP;

	--
	-- h) Check if curve intersects any other edge
	-- 
	FOR rec IN EXECUTE ''SELECT * FROM ''
		|| quote_ident(atopology) || ''.edge_data ''
		|| '' WHERE edge_id != '' || anedge
		|| '' AND geom && ''
		|| quote_literal(acurve::text) || ''::geometry ''
		|| '' AND ST_Intersects(geom, ''
		|| quote_literal(acurve::text) || ''::geometry)''
	LOOP
		RAISE EXCEPTION
		''SQL/MM Spatial exception - geometry intersects an edge'';
	END LOOP;

	--
	-- Update edge geometry
	--
	EXECUTE ''UPDATE '' || quote_ident(atopology) || ''.edge_data ''
		|| '' SET geom = '' || quote_literal(acurve::text) 
		|| '' WHERE edge_id = '' || anedge;

	RETURN ''Edge '' || anedge || '' changed'';

END
'
LANGUAGE 'plpgsql' VOLATILE;
--} ST_ChangeEdgeGeom

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.12
--
--  ST_AddEdgeNewFaces(atopology, anode, anothernode, acurve)
--
CREATE OR REPLACE FUNCTION topology.ST_AddEdgeNewFaces(varchar, integer, integer, geometry)
	RETURNS INTEGER AS
'
DECLARE
	atopology ALIAS FOR $1;
	anode ALIAS FOR $2;
	anothernode ALIAS FOR $3;
	acurve ALIAS FOR $4;
	rec RECORD;
	i INTEGER;
	az FLOAT8;
	azdif FLOAT8;
	myaz FLOAT8;
	minazimuth FLOAT8;
	maxazimuth FLOAT8;
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
		RAISE EXCEPTION ''SQL/MM Spatial exception - null argument'';
	END IF;

	--
	-- Acurve must be a LINESTRING
	--
	IF substring(geometrytype(acurve), 1, 4) != ''LINE''
	THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - invalid curve'';
	END IF;
	
	--
	-- Curve must be simple
	--
	IF NOT ST_IsSimple(acurve) THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - curve not simple'';
	END IF;

	-- 
	-- Check endpoints existance and match with Curve geometry
	--
	i=0;
	FOR rec IN EXECUTE ''SELECT ''
		|| '' CASE WHEN node_id = '' || anode
		|| '' THEN 1 WHEN node_id = '' || anothernode
		|| '' THEN 0 END AS start, geom FROM ''
		|| quote_ident(atopology)
		|| ''.node ''
		|| '' WHERE node_id IN ( ''
		|| anode || '','' || anothernode
		|| '')''
	LOOP
		IF rec.start THEN
			IF NOT Equals(rec.geom, ST_StartPoint(acurve))
			THEN
	RAISE EXCEPTION
	''SQL/MM Spatial exception - start node not geometry start point.'';
			END IF;
		ELSE
			IF NOT Equals(rec.geom, ST_EndPoint(acurve))
			THEN
	RAISE EXCEPTION
	''SQL/MM Spatial exception - end node not geometry end point.'';
			END IF;
		END IF;

		i=i+1;
	END LOOP;

	IF i < 2 THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - non-existent node'';
	END IF;

	--
	-- Check if this geometry crosses any node
	--
	FOR rec IN EXECUTE ''SELECT node_id FROM ''
		|| quote_ident(atopology) || ''.node
		WHERE geom && '' || quote_literal(acurve::text) || ''::geometry
		AND ST_Within(geom, '' || quote_literal(acurve::text) || ''::geometry)''
	LOOP
		RAISE EXCEPTION
		''SQL/MM Spatial exception - geometry crosses a node'';
	END LOOP;

	--
	-- Check if this geometry crosses any existing edge
	--
	FOR rec IN EXECUTE ''SELECT * FROM ''
		|| quote_ident(atopology) || ''.edge_data
		WHERE geom && '' || quote_literal(acurve::text) || ''::geometry
		AND crosses(geom, '' || quote_literal(acurve::text) || ''::geometry)''
	LOOP
		RAISE EXCEPTION
		''SQL/MM Spatial exception - geometry crosses an edge'';
	END LOOP;

	--
	-- Check if another edge share this edge endpoints
	--
	FOR rec IN EXECUTE ''SELECT * FROM ''
		|| quote_ident(atopology) || ''.edge_data ''
		|| '' WHERE ''
		|| '' geom && '' || quote_literal(acurve::text) || ''::geometry ''
		|| '' AND ''
		|| '' ( (''
		|| '' start_node = '' || anode
		|| '' AND ''
		|| '' end_node = '' || anothernode
		|| '' ) OR ( ''
		|| '' end_node = '' || anode
		|| '' AND ''
		|| '' start_node = '' || anothernode
		|| '' ) )''
		|| '' AND ''
		|| ''equals(geom,'' || quote_literal(acurve::text) || ''::geometry)''
	LOOP
		RAISE EXCEPTION
		''SQL/MM Spatial exception - coincident edge'';
	END LOOP;

	---------------------------------------------------------------
	--
	-- All checks passed, time to extract informations about
	-- endpoints:
	--
	--      next_left_edge
	--      next_right_edge
	--	left_face
	--	right_face
	--
	---------------------------------------------------------------

	--
	--
	-- Compute next_left_edge 
	--
	-- We fetch all edges with an endnode equal to
	-- this edge end_node (anothernode).
	-- For each edge we compute azimuth of the segment(s).
	-- Of interest are the edges with closest (smaller
	-- and bigger) azimuths then the azimuth of
	-- this edge last segment.
	--

	myaz = ST_Azimuth(ST_EndPoint(acurve), ST_PointN(acurve, ST_NumPoints(acurve)-1));
	RAISE NOTICE ''My end-segment azimuth: %'', myaz;
	FOR rec IN EXECUTE ''SELECT ''
		|| ''edge_id, end_node, start_node, geom''
		|| '' FROM ''
		|| quote_ident(atopology)
		|| ''.edge_data ''
		|| '' WHERE ''
		|| '' end_node = '' || anothernode
		|| '' OR ''
		|| '' start_node = '' || anothernode
	LOOP

		IF rec.start_node = anothernode THEN
			--
			-- Edge starts at our node, we compute
			-- azimuth from node to its second point
			--
			az = ST_Azimuth(ST_EndPoint(acurve),
				ST_PointN(rec.geom, 2));

			RAISE NOTICE ''Edge % starts at node % - azimuth %'',
				rec.edge_id, rec.start_node, az;
		END IF;

		IF rec.end_node = anothernode THEN
			--
			-- Edge ends at our node, we compute
			-- azimuth from node to its second-last point
			--
			az = ST_Azimuth(ST_EndPoint(acurve),
				ST_PointN(rec.geom, ST_NumPoints(rec.geom)-1));

			RAISE NOTICE ''Edge % ends at node % - azimuth %'',
				rec.edge_id, rec.end_node, az;
		END IF;
	END LOOP;


	RAISE EXCEPTION ''Not implemented yet'';
END
'
LANGUAGE 'plpgsql' VOLATILE;
--} ST_AddEdgeNewFaces

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.17
--
--  ST_InitTopoGeo(atopology)
--
CREATE OR REPLACE FUNCTION topology.ST_InitTopoGeo(varchar)
RETURNS text
AS '
DECLARE
	atopology alias for $1;
	rec RECORD;
	topology_id numeric;
BEGIN
	IF atopology IS NULL THEN
		RAISE EXCEPTION ''SQL/MM Spatial exception - null argument'';
	END IF;

	FOR rec IN SELECT * FROM pg_namespace WHERE text(nspname) = atopology
	LOOP
		RAISE EXCEPTION ''SQL/MM Spatial exception - schema already exists'';
	END LOOP;

	FOR rec IN EXECUTE ''SELECT topology.CreateTopology(''
		||quote_literal(atopology)|| '') as id''
	LOOP
		topology_id := rec.id;
	END LOOP;

	RETURN ''Topology-Geometry '' || quote_literal(atopology)
		|| '' (id:'' || topology_id || '') created. '';
END
'
LANGUAGE 'plpgsql' VOLATILE;
--} ST_InitTopoGeo

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.18
--
--  ST_CreateTopoGeo(atopology, acollection)
--
CREATE OR REPLACE FUNCTION topology.ST_CreateTopoGeo(varchar, geometry)
RETURNS text
AS '
DECLARE
	atopology alias for $1;
	acollection alias for $2;
	typ char(4);
	rec RECORD;
	ret int;
	schemaoid oid;
BEGIN
	IF atopology IS NULL OR acollection IS NULL THEN
		RAISE EXCEPTION ''SQL/MM Spatial exception - null argument'';
	END IF;

	-- Verify existance of the topology schema 
	FOR rec in EXECUTE ''SELECT oid FROM pg_namespace WHERE ''
		|| '' nspname = '' || quote_literal(atopology)
		|| '' GROUP BY oid''
		
	LOOP
		schemaoid := rec.oid;
	END LOOP;

	IF schemaoid IS NULL THEN
	RAISE EXCEPTION ''SQL/MM Spatial exception - non-existent schema'';
	END IF;

	-- Verify existance of the topology views in the topology schema 
	FOR rec in EXECUTE ''SELECT count(*) FROM pg_class WHERE ''
		|| '' relnamespace = '' || schemaoid 
		|| '' and relname = ''''node''''''
		|| '' OR relname = ''''edge''''''
		|| '' OR relname = ''''face''''''
	LOOP
		IF rec.count < 3 THEN
	RAISE EXCEPTION ''SQL/MM Spatial exception - non-existent view'';
		END IF;
	END LOOP;

	-- Verify the topology views in the topology schema to be empty
	FOR rec in EXECUTE
		''SELECT count(*) FROM ''
		|| quote_ident(atopology) || ''.edge_data ''
		|| '' UNION '' ||
		''SELECT count(*) FROM ''
		|| quote_ident(atopology) || ''.node ''
	LOOP
		IF rec.count > 0 THEN
	RAISE EXCEPTION ''SQL/MM Spatial exception - non-empty view'';
		END IF;
	END LOOP;

	-- face check is separated as it will contain a single (world)
	-- face record
	FOR rec in EXECUTE
		''SELECT count(*) FROM ''
		|| quote_ident(atopology) || ''.face ''
	LOOP
		IF rec.count != 1 THEN
	RAISE EXCEPTION ''SQL/MM Spatial exception - non-empty face view'';
		END IF;
	END LOOP;

	-- 
	-- LOOP through the elements invoking the specific function
	-- 
	FOR rec IN SELECT geom(ST_Dump(acollection))
	LOOP
		typ := substring(geometrytype(rec.geom), 1, 3);

		IF typ = ''LIN'' THEN
	SELECT topology.TopoGeo_addLinestring(atopology, rec.geom) INTO ret;
		ELSIF typ = ''POI'' THEN
	SELECT topology.TopoGeo_AddPoint(atopology, rec.geom) INTO ret;
		ELSIF typ = ''POL'' THEN
	SELECT topology.TopoGeo_AddPolygon(atopology, rec.geom) INTO ret;
		ELSE
	RAISE EXCEPTION ''ST_CreateTopoGeo got unknown geometry type: %'', typ;
		END IF;

	END LOOP;

	RETURN ''Topology '' || atopology || '' populated'';

	RAISE EXCEPTION ''ST_CreateTopoGeo not implemente yet'';
END
'
LANGUAGE 'plpgsql' VOLATILE;
--} ST_CreateTopoGeo

--=}  SQL/MM block

