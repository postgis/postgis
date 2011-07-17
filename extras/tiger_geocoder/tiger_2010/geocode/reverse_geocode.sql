--$Id$
 /*** 
 * 
 * Copyright (C) 2011 Regina Obe and Leo Hsu (Paragon Corporation)
 **/
-- This function given a point try to determine the approximate street address (norm_addy form)
-- and array of cross streets, as well as interpolated points along the streets
-- Use case example an address at the intersection of 3 streets: SELECT pprint_addy(r.addy[1]) As st1, pprint_addy(r.addy[2]) As st2, pprint_addy(r.addy[3]) As st3, array_to_string(r.street, ',') FROM reverse_geocode(ST_GeomFromText('POINT(-71.057811 42.358274)',4269)) As r;
--set search_path=tiger,public;

CREATE OR REPLACE FUNCTION reverse_geocode(IN pt geometry, IN include_strnum_range boolean DEFAULT false, OUT intpt geometry[], OUT addy norm_addy[], OUT street character varying[])
  RETURNS record AS
$$
DECLARE
  var_redge RECORD;
  var_state text := NULL;
  var_stusps text := NULL;
  var_countyfp text := NULL;
  var_addy NORM_ADDY;
  var_addy_alt NORM_ADDY;
  var_nstrnum numeric(10);
  var_primary_line geometry := NULL;
  var_primary_dist numeric(10,2) ;
  var_pt geometry;
  var_place varchar;
  var_stmt text;
  var_debug boolean = false;
  var_zip varchar := NULL;
  var_primary_fullname varchar := '';
BEGIN
	--$Id$
	IF pt IS NULL THEN
		RETURN;
	ELSE
		IF ST_SRID(pt) = 4269 THEN
			var_pt := pt;
		ELSIF ST_SRID(pt) > 0 THEN
			var_pt := ST_Transform(pt, 4269); 
		ELSE --If srid is unknown, assume its 4269
			var_pt := ST_SetSRID(pt, 4269);
		END IF;
		var_pt := ST_SnapToGrid(var_pt, 0.00005); /** Get rid of floating point junk that would prevent intersections **/
	END IF;
	-- Determine state tables to check 
	-- this is needed to take advantage of constraint exclusion
	IF var_debug THEN
		RAISE NOTICE 'Get matching states start: %', clock_timestamp();
	END IF;
	SELECT statefp, stusps INTO var_state, var_stusps FROM state WHERE ST_Intersects(the_geom, var_pt) LIMIT 1;
	IF var_debug THEN
		RAISE NOTICE 'Get matching states end: % -  %', var_state, clock_timestamp();
	END IF;
	IF var_state IS NULL THEN
		-- We don't have any data for this state
		RETURN;
	END IF;
	IF var_debug THEN
		RAISE NOTICE 'Get matching counties start: %', clock_timestamp();
	END IF;
	-- locate county
	SELECT countyfp INTO var_countyfp FROM county WHERE statefp = var_state AND ST_Intersects(the_geom, var_pt)  LIMIT 1;
	
	--locate zip
	SELECT zcta5ce INTO var_zip FROM zcta5 WHERE statefp = var_state AND ST_Intersects(the_geom, var_pt)  LIMIT 1;
	
	-- lcoate city
	IF var_zip > '' THEN
	      var_addy.zip := var_zip ;
	      SELECT z.place INTO var_place FROM zip_state_loc AS z WHERE z.zip = var_zip and z.statefp =  var_state LIMIT 1;
	      var_addy.location := var_place;
	END IF;

	IF var_debug THEN
		RAISE NOTICE 'Get matching counties end: % - %',var_countyfp,  clock_timestamp();
	END IF;
	IF var_countyfp IS NULL THEN
		-- We don't have any data for this county
		RETURN;
	END IF;
	
	var_addy.stateAbbrev = var_stusps;

	-- Find the street edges that this point is closest to with tolerance of 0.005 but only consider the edge if the point is contained in the right or left face
	-- Then order addresses by proximity to road
	IF var_debug THEN
		RAISE NOTICE 'Get matching edges start: %', clock_timestamp();
	END IF;

	var_stmt := '
	    WITH ref AS (
	        SELECT ' || quote_literal(var_pt::text) || '::geometry As ref_geom ) , 
			f AS 
			( SELECT * FROM faces CROSS JOIN ref
			WHERE statefp = ' || quote_literal(var_state) || ' AND countyfp = ' || quote_literal(var_countyfp) || ' 
				AND ST_Intersects(faces.the_geom, ref_geom)
				    ),
			e AS 
			( SELECT edges.* , CASE WHEN edges.tfidr = f.tfid THEN ''R'' WHEN edges.tfidl = f.tfid THEN ''L'' ELSE NULL END::varchar As eside
				FROM edges INNER JOIN f ON (f.statefp = edges.statefp AND (edges.tfidr = f.tfid OR edges.tfidl = f.tfid)) 
				    CROSS JOIN ref
			WHERE edges.statefp = ' || quote_literal(var_state) || ' AND edges.countyfp = ' || quote_literal(var_countyfp) || ' 
				AND ST_DWithin(edges.the_geom, ref.ref_geom, 0.01) AND (edges.mtfcc LIKE ''S%'') --only consider streets and roads
				  ),
			a AS (SELECT addr.* FROM addr INNER JOIN e ON (addr.statefp = e.statefp AND addr.tlid = e.tlid)
				WHERE addr.statefp = ' || quote_literal(var_state) || '),
			n AS (SELECT featnames.* FROM featnames INNER JOIN e ON(featnames.statefp = e.statefp AND featnames.tlid = e.tlid)
			    WHERE featnames.statefp = ' || quote_literal(var_state) || ' AND e.mtfcc LIKE ''S%''  )
				
		SELECT * 
		FROM (SELECT DISTINCT ON(tlid,eside)  foo.fullname, foo.streetname, foo.streettypeabbrev, foo.zip,  foo.center_pt,
			  eside, to_number(fromhn, ''999999'') As fromhn, to_number(tohn, ''999999'') As tohn, ST_GeometryN(ST_Multi(line),1) As line, 
			  ST_Distance_Sphere(foo.line, ' || quote_literal(var_pt::text) || '::geometry) As dist
		FROM 
		  (SELECT e.tlid, e.the_geom As line, COALESCE(n.fullname,e.fullname) As fullname, COALESCE(n.prequalabr || '' '','''')  || n.name AS streetname, n.predirabrv, COALESCE(suftypabrv, pretypabrv) As streettypeabbrev,
		      n.sufdirabrv, a.zip, ST_ClosestPoint(e.the_geom,' || quote_literal(var_pt::text) || '::geometry) As center_pt, e.eside, a.fromhn, a.tohn ,
		          ST_Distance_Sphere(e.the_geom, ' || quote_literal(var_pt::text) || '::geometry) As dist
				FROM e 
					LEFT JOIN a ON    
					   ( a.tlid = e.tlid 
					       AND  a.side = e.eside
					       )  
					LEFT JOIN n ON (n.statefp =  e.statefp AND n.tlid = e.tlid) 
				ORDER BY dist LIMIT 50 ) As foo 
				ORDER BY foo.tlid, foo.eside, foo.fullname ASC NULLS LAST, dist LIMIT 15) As f ORDER BY f.dist ';
				
	IF var_debug = true THEN
	    RAISE NOTICE 'Statement 1: %', var_stmt;
	END IF;
	/** FOR var_redge IN
		SELECT * 
		FROM (SELECT DISTINCT ON(fullname)  foo.fullname, foo.stusps, foo.zip, 
			   (SELECT z.place FROM zip_state_loc AS z WHERE z.zip = foo.zip and z.statefp = foo.statefp LIMIT 1) As place, foo.center_pt,
			  side, to_number(fromhn, '999999') As fromhn, to_number(tohn, '999999') As tohn, ST_GeometryN(ST_Multi(line),1) As line, foo.dist
		FROM 
		  (SELECT e.the_geom As line, e.fullname, a.zip, s.abbrev As stusps, ST_ClosestPoint(e.the_geom, var_pt) As center_pt, e.statefp, a.side, a.fromhn, a.tohn, ST_Distance_Sphere(e.the_geom, var_pt) As dist
				FROM (SELECT * FROM edges WHERE statefp = var_state AND countyfp = var_countyfp ) AS e INNER JOIN (SELECT * FROM state_lookup WHERE statefp = var_state ) As s ON (e.statefp = s.statefp )
					INNER JOIN (SELECT * FROM faces WHERE statefp = var_state AND countyfp = var_countyfp ) As fl ON (e.tfidl = fl.tfid AND e.statefp = fl.statefp)
					INNER JOIN (SELECT * FROM faces WHERE statefp = var_state AND countyfp = var_countyfp ) As fr ON (e.tfidr = fr.tfid AND e.statefp = fr.statefp)
					INNER JOIN (SELECT * FROM addr WHERE statefp = var_state ) As a ON ( e.tlid = a.tlid AND e.statefp = a.statefp AND  
					   ( ( ST_Covers(fl.the_geom, var_pt) AND a.side = 'L') OR ( ST_Covers(fr.the_geom, var_pt) AND a.side = 'R' ) ) )
					-- INNER JOIN zip_state_loc As z ON (a.statefp =  z.statefp AND a.zip = z.zip) 
				WHERE ST_DWithin(e.the_geom, var_pt, 0.005)
				ORDER BY ST_Distance(e.the_geom, var_pt) LIMIT 4) As foo 
				WHERE dist < 150 --less than 150 m
				ORDER BY foo.fullname, foo.dist) As f ORDER BY f.dist LOOP **/

    FOR var_redge IN EXECUTE var_stmt LOOP
        IF var_debug THEN
            RAISE NOTICE 'Start Get matching edges loop: %,%', var_primary_line, clock_timestamp();
        END IF;
        IF var_primary_line IS NULL THEN --this is the first time in the loop and our primary guess
            var_primary_line := var_redge.line;
            var_primary_dist := var_redge.dist;
        END IF;
  
        IF var_redge.fullname IS NOT NULL AND COALESCE(var_primary_fullname,'') = '' THEN -- this is the first non-blank name we are hitting grab info
            var_primary_fullname := var_redge.fullname;
            var_addy.streetname = var_redge.streetname;
            var_addy.streettypeabbrev := var_redge.streettypeabbrev;
        END IF;
       
        IF ST_Intersects(var_redge.line, var_primary_line) THEN
            var_addy.streetname := var_redge.streetname; 
            
            var_addy.streettypeabbrev := var_redge.streettypeabbrev;
            var_addy.address := var_nstrnum;
            IF  var_redge.fromhn IS NOT NULL THEN
                --interpolate the number -- note that if fromhn > tohn we will be subtracting which is what we want
                var_nstrnum := (var_redge.fromhn + ST_Line_Locate_Point(var_redge.line, var_pt)*(var_redge.tohn - var_redge.fromhn))::numeric(10);
                -- The odd even street number side of street rule
                IF (var_nstrnum  % 2)  != (var_redge.tohn % 2) THEN
                    var_nstrnum := CASE WHEN var_nstrnum + 1 NOT BETWEEN var_redge.fromhn AND var_redge.tohn THEN var_nstrnum - 1 ELSE var_nstrnum + 1 END;
                END IF;
                var_addy.address := var_nstrnum;
            END IF;
            IF var_redge.zip > '' AND COALESCE(var_addy.zip,'') <>  var_redge.zip THEN
                var_addy.zip := var_redge.zip; 
                SELECT z.place INTO var_place FROM zip_state_loc AS z WHERE z.zip = var_redge.zip and z.statefp =  var_state LIMIT 1;
                var_addy.location := var_place;
            END IF;  
            
            -- This is a cross streets - only add if not the primary adress street
            IF var_redge.fullname > '' AND var_redge.fullname <> var_primary_fullname THEN
                street := array_append(street, (CASE WHEN include_strnum_range THEN COALESCE(var_redge.fromhn::varchar, '')::varchar || COALESCE(' - ' || var_redge.tohn::varchar,'')::varchar || ' '::varchar  ELSE '' END::varchar ||  COALESCE(var_redge.fullname::varchar,''))::varchar);
            END IF;    
            
            -- consider this a potential address
            IF (var_redge.dist < var_primary_dist*1.1 OR var_redge.dist < 20)   THEN
                 -- We only consider this a possible address if it is really close to our point
                
                 intpt := array_append(intpt,var_redge.center_pt); 
                 addy := array_append(addy, var_addy);
                -- note that ramps don't have names or addresses but they connect at the edge of a range
                -- so for ramps the address of connecting is still useful
            
                -- Determine city if zip is different from previous
            
            END IF;
        END IF;
    END LOOP;
 
    -- not matching roads or streets, just return basic info
    IF NOT FOUND THEN
        addy := array_append(addy,var_addy);
    END IF;
    
    IF var_addy.streetname > '' AND addy[1].streetname IS NULL THEN
        --there were no intersecting addresses with names, just pick the best candidate - the match is proably an offshoot of some sort
        var_addy_alt := addy[1];
        var_addy_alt.streetname := var_addy.streetname;
        var_addy_alt.streettypeabbrev := var_addy.streettypeabbrev;
        addy[1] := var_addy_alt; 
    END IF;
    	IF var_debug THEN
		RAISE NOTICE 'End Get matching edges loop: %', clock_timestamp();
	END IF;
    


          		
	RETURN;   
END;
$$
  LANGUAGE plpgsql STABLE
  COST 1000;
