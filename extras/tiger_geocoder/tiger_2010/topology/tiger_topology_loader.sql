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
	-- start load in faces
	var_sql := 'INSERT INTO ' || quote_ident(toponame) || '.face(face_id, mbr) 
						SELECT tfid, ST_Envelope(the_geom) As mbr 
							FROM tiger.faces WHERE statefp = $1 AND ST_Intersects(the_geom, $2) 
							AND tfid NOT IN(SELECT face_id FROM ' || quote_ident(toponame) || '.face) ';
	EXECUTE var_sql USING var_statefp, var_rgeom;
	GET DIAGNOSTICS var_rcnt = ROW_COUNT;
	var_result := var_rcnt::text || ' faces added. ';
   -- end load in faces
   
   	-- start load in nodes
	var_sql := 'INSERT INTO ' || quote_ident(toponame) || '.node(node_id, geom)
					SELECT DISTINCT ON(tnid) tnid, geom
						FROM 
						( 
							SELECT tnidf AS tnid, ST_StartPoint(ST_GeometryN(the_geom,1)) As geom 
								FROM tiger.edges WHERE statefp = $1 AND ST_Intersects(the_geom, $2) 
									AND tnidf NOT IN(SELECT node_id FROM ' || quote_ident(toponame) || '.node)
						UNION ALL 
							SELECT tnidt AS tnid, ST_EndPoint(ST_GeometryN(the_geom,1)) As geom 
							FROM tiger.edges WHERE statefp = $1 AND ST_Intersects(the_geom, $2) 
									AND tnidt NOT IN(SELECT node_id FROM ' || quote_ident(toponame) || '.node) 
					) As n ';
	EXECUTE var_sql USING var_statefp, var_rgeom;
	GET DIAGNOSTICS var_rcnt = ROW_COUNT;
	var_result := var_result || ' ' || var_rcnt::text || ' nodes added. ';
   -- end load in nodes
   
   -- TODO: Load in edges --
	RETURN var_result;
END
$$
  LANGUAGE plpgsql VOLATILE
  COST 1000;
