--$Id$
 /*** 
 * 
 * Copyright (C) 2011 Regina Obe and Leo Hsu (Paragon Corporation)
 **/
-- This function given a point try to determine the approximate street address (norm_addy form)
-- and array of cross streets, as well as interpolated points along the streets
-- Use case example an address at the intersection of 3 streets: SELECT pprint_addy(r.addy[1]) As st1, pprint_addy(r.addy[2]) As st2, pprint_addy(r.addy[3]) As st3, array_to_string(r.street, ',') FROM reverse_geocode(ST_GeomFromText('POINT(-71.057811 42.358274)',4269)) As r;
--set search_path=tiger,public;
CREATE OR REPLACE FUNCTION reverse_geocode(
    IN pt geometry,
    IN include_strnum_range boolean,
    OUT intpt geometry[],
    OUT addy NORM_ADDY[],
    OUT street varchar[]
) RETURNS RECORD
AS $_$
DECLARE
  var_redge RECORD;
  var_states text[];
  var_addy NORM_ADDY;
  var_strnum varchar;
  var_nstrnum numeric(10);
  var_primary_line geometry := NULL;
  var_primary_dist numeric(10,2) ;
  var_pt geometry;
BEGIN
  IF pt IS NULL THEN
    RETURN;
  ELSE
    IF ST_SRID(pt) = 4269 THEN
        var_pt := pt;
    ELSE
        var_pt := ST_Transform(pt, 4269);
    END IF;
  END IF;
  -- Determine state tables to check 
  -- this is needed to take advantage of constraint exclusion
  var_states := ARRAY(SELECT statefp FROM state WHERE ST_Intersects(the_geom, var_pt) );
  IF array_upper(var_states, 1) IS NULL THEN
  -- We don't have any data for this state
  	RETURN;
  END IF;
  
  -- Find the street edges that this point is closest to with tolerance of 0.005 but only consider the edge if the point is contained in the right or left face
  -- Then order addresses by proximity to road
  FOR var_redge IN
        SELECT * 
        FROM (SELECT DISTINCT ON(fullname)  foo.fullname, foo.stusps, foo.zip, 
               (SELECT z.place FROM zip_state_loc AS z WHERE z.zip = foo.zip and z.statefp = foo.statefp LIMIT 1) As place, foo.center_pt,
              side, to_number(fromhn, '999999') As fromhn, to_number(tohn, '999999') As tohn, ST_GeometryN(ST_Multi(line),1) As line, foo.dist
        FROM 
          (SELECT e.the_geom As line, e.fullname, a.zip, s.stusps, ST_ClosestPoint(e.the_geom, var_pt) As center_pt, e.statefp, a.side, a.fromhn, a.tohn, ST_Distance_Sphere(e.the_geom, var_pt) As dist
          		FROM edges AS e INNER JOIN state As s ON (e.statefp = s.statefp AND s.statefp = ANY(var_states) )
          			INNER JOIN faces As fl ON (e.tfidl = fl.tfid AND e.statefp = fl.statefp)
          			INNER JOIN faces As fr ON (e.tfidr = fr.tfid AND e.statefp = fr.statefp)
          			INNER JOIN addr As a ON ( e.tlid = a.tlid AND e.statefp = a.statefp AND  
          			   ( ( ST_Covers(fl.the_geom, var_pt) AND a.side = 'L') OR ( ST_Covers(fr.the_geom, var_pt) AND a.side = 'R' ) ) )
          			-- INNER JOIN zip_state_loc As z ON (a.statefp =  z.statefp AND a.zip = z.zip) /** really slow with this join **/
          		WHERE e.statefp = ANY(var_states) AND a.statefp = ANY(var_states) AND ST_DWithin(e.the_geom, var_pt, 0.005)
          		ORDER BY ST_Distance_Sphere(e.the_geom, var_pt) LIMIT 4) As foo 
          		WHERE dist < 150 --less than 150 m
          		ORDER BY foo.fullname, foo.dist) As f ORDER BY f.dist LOOP
       IF var_primary_line IS NULL THEN --this is the first time in the loop and our primary guess
            var_primary_line := var_redge.line;
            var_primary_dist := var_redge.dist;
       END IF;
       -- We only consider other edges as matches if they intersect our primary edge -- that would mean we are at a corner place
       IF ST_Intersects(var_redge.line, var_primary_line) THEN
           intpt := array_append(intpt,var_redge.center_pt); 
           IF var_redge.fullname IS NOT NULL THEN
                street := array_append(street, (CASE WHEN include_strnum_range THEN COALESCE(var_redge.fromhn::varchar, '')::varchar || ' - ' || COALESCE(var_redge.tohn::varchar,'')::varchar || ' '::varchar  ELSE '' END::varchar ||  var_redge.fullname::varchar)::varchar);
                --interploate the number -- note that if fromhn > tohn we will be subtracting which is what we want
                -- We only consider differential distances are reeally close from our primary pt
                IF var_redge.dist < var_primary_dist*1.1 THEN 
                	var_nstrnum := (var_redge.fromhn + ST_Line_Locate_Point(var_redge.line, var_pt)*(var_redge.tohn - var_redge.fromhn))::numeric(10);
                	-- The odd even street number side of street rule
                	IF (var_nstrnum  % 2)  != (var_redge.tohn % 2) THEN
                		var_nstrnum := CASE WHEN var_nstrnum + 1 NOT BETWEEN var_redge.fromhn AND var_redge.tohn THEN var_nstrnum - 1 ELSE var_nstrnum + 1 END;
                	END IF;
                    var_strnum := var_nstrnum::varchar;
                    var_addy := normalize_address( COALESCE(var_strnum::varchar || ' ', '') || var_redge.fullname || ', ' || var_redge.place || ', ' || var_redge.stusps || ' ' || var_redge.zip);
                    addy := array_append(addy, var_addy);
                END IF;
           END IF;
        END IF;
  END LOOP;
          		
  RETURN;   
END;
$_$ LANGUAGE plpgsql STABLE;

CREATE OR REPLACE FUNCTION reverse_geocode(IN pt geometry, OUT intpt geometry[],
    OUT addy NORM_ADDY[],
    OUT street varchar[]) RETURNS RECORD
AS 
$$
-- default to not include street range in cross streets
    SELECT reverse_geocode($1,false);
$$
language sql STABLE;