/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * Copyright 2011 Leo Hsu and Regina Obe <lr@pcorp.us> 
 * Paragon Corporation
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 * This file contains helper functions for loading tiger data
 * into postgis topology structure
 **********************************************************************/

 /** topology_load_tiger: Will load all edges, faces, nodes into 
 *  topology named toponame
 *	that intersect the specified region
 *  region_type: 'place', 'county'
 *  region_id: the respective fully qualified geoid
 *	 place - plcidfp
 *	 county - cntyidfp 
 * USE CASE: 
 *	The following will create a topology called topo_boston and load Boston, MA
 * SELECT topology.CreateTopology('topo_boston', 4269);
 * SELECT tiger.topology_load_tiger('topo_boston', 'place', '2507000'); 
 ****/
CREATE OR REPLACE FUNCTION tiger.topology_load_tiger(IN toponame varchar,  
	region_type varchar, region_id varchar)
  RETURNS text AS
$$
DECLARE
 	var_sql text;
 	var_rgeom geometry;
 	var_statefp text;
 	var_rcnt bigint;
 	var_result text := '';
BEGIN
	--$Id$
	CASE region_type
		WHEN 'place' THEN
			SELECT the_geom , statefp FROM place INTO var_rgeom, var_statefp WHERE plcidfp = region_id;
		WHEN 'county' THEN
			SELECT the_geom, statefp FROM county INTO var_rgeom, var_statefp WHERE cntyidfp = region_id;
		ELSE
			RAISE EXCEPTION 'Region type % IS NOT SUPPORTED', region_type;
	END CASE;
	
	var_sql := '
	CREATE TEMPORARY TABLE tmp_edge
   				AS 
	WITH te AS 
   			(SELECT tlid,  ST_GeometryN(the_geom,1) As the_geom, tnidf, tnidt, tfidl, tfidr 
									FROM tiger.edges 
									WHERE statefp = $1 AND ST_Intersects(the_geom, $2)
										)
					SELECT DISTINCT ON (t.tlid) t.tlid As edge_id,t.the_geom As geom, t.tnidf As start_node, t.tnidt As end_node, t.tfidl As left_face, t.tfidr As right_face, tl.tlid AS next_left_edge,  tr.tlid As next_right_egdge
						FROM 
							te AS t INNER JOIN te As tl ON (t.tnidf = tl.tnidt) 
							 INNER JOIN te As tr ON (t.tnidt = tr.tnidf) 				
						';
	EXECUTE var_sql USING var_statefp, var_rgeom;
	GET DIAGNOSTICS var_rcnt = ROW_COUNT;
	var_result := var_rcnt::text || ' edges holding in temporary. ';
	var_sql := 'ALTER TABLE tmp_edge ADD CONSTRAINT pk_tmp_edge PRIMARY KEY(edge_id );';
	EXECUTE var_sql;
	-- CREATE node indexes on temporary edges
	var_sql := 'CREATE INDEX idx_tmp_edge_start_node ON tmp_edge USING btree (start_node ); CREATE INDEX idx_tmp_edge_end_node ON tmp_edge USING btree (end_node );';

	EXECUTE var_sql;

	-- CREATE face indexes on temporary edges
	var_sql := 'CREATE INDEX idx_tmp_edge_left_face ON tmp_edge USING btree (left_face ); CREATE INDEX idx_tmp_edge_right_face ON tmp_edge USING btree (right_face );';

	EXECUTE var_sql;
	-- start load in faces
	var_sql := 'INSERT INTO ' || quote_ident(toponame) || '.face(face_id, mbr) 
						SELECT f.tfid, ST_Envelope(f.the_geom) As mbr 
							FROM tiger.faces AS f
								WHERE statefp = $1 AND 
								( ST_Intersects(the_geom, $2) 
									OR tfid IN(SELECT left_face FROM tmp_edge)
									OR tfid IN(SELECT right_face FROM tmp_edge) )
							AND tfid NOT IN(SELECT face_id FROM ' || quote_ident(toponame) || '.face) ';
	EXECUTE var_sql USING var_statefp, var_rgeom;
	GET DIAGNOSTICS var_rcnt = ROW_COUNT;
	var_result := var_result || var_rcnt::text || ' faces added. ';
   -- end load in faces
   
   	-- start load in nodes
	var_sql := 'INSERT INTO ' || quote_ident(toponame) || '.node(node_id, geom)
					SELECT DISTINCT ON(tnid) tnid, geom
						FROM 
						( 
							SELECT start_node AS tnid, ST_StartPoint(geom) As geom 
								FROM tmp_edges As e LEFT JOIN ' || quote_ident(toponame) || '.node AS n ON e.start_node = n.node_id
						UNION ALL 
							SELECT end_node AS tnid, ST_EndPoint(geom) As geom 
							FROM tmp_edges As e LEFT JOIN ' || quote_ident(toponame) || '.node AS n ON e.end_node = n.node_id 
							WHERE n.node_id IS NULL) As f
					 ';
	EXECUTE var_sql USING var_statefp, var_rgeom;
	GET DIAGNOSTICS var_rcnt = ROW_COUNT;
	var_result := var_result || ' ' || var_rcnt::text || ' nodes added. ';

   -- end load in nodes
   -- start Mark which nodes are contained in faces
   	var_sql := 'UPDATE ' || quote_ident(toponame) || '.node AS n
					SET containing_face = f.tfid
						FROM (SELECT tfid, the_geom
							FROM tiger.faces WHERE statefp = $1 AND ST_Intersects(the_geom, $2) 
							) As f
						WHERE ST_Contains(f.the_geom, n.geom) ';
	EXECUTE var_sql USING var_statefp, var_rgeom;
	GET DIAGNOSTICS var_rcnt = ROW_COUNT;
	var_result := var_result || ' ' || var_rcnt::text || ' nodes contained in a face. ';
   -- end Mark nodes contained in faces
   
   -- TODO: Load in edges --
   var_sql := '
   	INSERT INTO ' || quote_ident(toponame) || '.edge(edge_id, geom, start_node, end_node, left_face, right_face, next_left_edge, next_right_edge)
					SELECT edge_id, geom, start_node, end_node, left_face, right_face, next_left_edge, next_right_edge
						FROM 
							tmp_edge
							WHERE edge_id NOT IN(SELECT edge_id FROM ' || quote_ident(toponame) || '.edge) 				
						';
	EXECUTE var_sql USING var_statefp, var_rgeom;
	var_sql = 'DROP TABLE tmp_edge;';
	EXECUTE var_sql;
	RETURN var_result;
END
$$
  LANGUAGE plpgsql VOLATILE
  COST 1000;
