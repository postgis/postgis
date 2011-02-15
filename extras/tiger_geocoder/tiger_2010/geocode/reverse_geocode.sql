--$Id$
 /*** 
 * 
 * Copyright (C) 2011 Regina Obe and Leo Hsu (Paragon Corporation)
 **/
-- This function given a point try to determine the approximate street address (norm_addy form)
-- and array of cross streets, as well as interpolated points along the streets
-- Use case example: SELECT pprint_addy(r.addy[1]) As st1, array_to_string(r.street, ',') FROM reverse_geocode(ST_GeomFromText('POINT(-71.057811 42.358274)',4269)) As r;
set search_path=tiger,public;
CREATE OR REPLACE FUNCTION reverse_geocode(
    IN pt geometry,
    IN include_strnum_range boolean,
    OUT intpt geometry[],
    OUT addy NORM_ADDY[],
    OUT street text[]
) RETURNS RECORD
AS $_$
DECLARE
  var_redge RECORD;
  var_states text[];
  var_addy NORM_ADDY;
BEGIN
  IF pt IS NULL THEN
    RETURN;
  END IF;
  -- Determine state tables to check 
  -- this is needed to take advantage of constraint exclusion
  var_states := ARRAY(SELECT statefp FROM state WHERE ST_Intersects(the_geom, pt) );
  IF array_upper(var_states, 1) IS NULL THEN
  -- We don't have any data for this state
  	RETURN;
  END IF;
  
  -- Find the street edges that this point is closest to with tolerance of 500 meters (width of road)
  FOR var_redge IN
        SELECT DISTINCT ON(fullname) foo.gid, foo.fullname, foo.stusps, foo.zip, 
               (SELECT z.place FROM zip_state_loc AS z WHERE z.zip = foo.zip and z.statefp = foo.statefp LIMIT 1) As place, foo.intpt,
              side, fromhn, tohn
        FROM 
          (SELECT e.gid, e.fullname, a.zip, s.stusps, ST_ClosestPoint(e.the_geom, pt) As intpt, e.statefp, a.side, a.fromhn, a.tohn
          		FROM edges AS e INNER JOIN state As s ON (e.statefp = s.statefp AND s.statefp = ANY(var_states) )
          			INNER JOIN faces As fl ON (e.tfidl = fl.tfid AND e.statefp = fl.statefp)
          			INNER JOIN faces As fr ON (e.tfidr = fr.tfid AND e.statefp = fr.statefp)
          			INNER JOIN addr As a ON ( e.tlid = a.tlid AND e.statefp = a.statefp AND  
          			   ( ( ST_Contains(fl.the_geom, pt) AND a.side = 'L') OR  ( ST_Contains(fr.the_geom, pt) AND a.side = 'R' ) ) )
          			-- INNER JOIN zip_state_loc As z ON (a.statefp =  z.statefp AND a.zip = z.zip) /** really slow with this join **/
          		WHERE e.statefp = ANY(var_states) AND a.statefp = ANY(var_states) AND ST_DWithin(e.the_geom, pt, 0.005)
          		ORDER BY ST_Distance(e.the_geom, pt) LIMIT 4) As foo 
          		WHERE ST_Distance_Sphere(foo.intpt, pt) < 150 --less than 150 m
          		ORDER BY foo.fullname, ST_Distance_Sphere(foo.intpt, pt) LOOP
       intpt := array_append(intpt,var_redge.intpt); 
       IF var_redge.fullname IS NOT NULL THEN
       		street := array_append(street, CASE WHEN include_strnum_range THEN COALESCE(var_redge.fromhn, '')::text || ' - ' || COALESCE(var_redge.tohn,'')::text || ' '::text  ELSE '' END::text ||  var_redge.fullname::text);
       		var_addy := normalize_address(var_redge.fullname || ', ' || var_redge.place || ', ' || var_redge.stusps || ' ' || var_redge.zip);
       		addy := array_append(addy, var_addy);
       END IF;
  END LOOP;
          		
  RETURN;   
END;
$_$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION reverse_geocode(IN pt geometry, OUT intpt geometry[],
    OUT addy NORM_ADDY[],
    OUT street text[]) RETURNS RECORD
AS 
$$
-- default to not including street range in cross streets
SELECT reverse_geocode($1,false);
$$
language sql;