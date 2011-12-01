--$Id$
 /*** 
 * 
 * Copyright (C) 2011 Regina Obe and Leo Hsu (Paragon Corporation)
 **/
-- This function given two roadways, state and optional city, zip
-- Will return addresses that are at the intersecton of those roadways
-- The address returned will be the address on the first road way
-- Use case example an address at the intersection of 2 streets: 
-- SELECT pprint_addy(addy), st_astext(geomout),rating FROM geocode_intersection('School St', 'Washington St', 'MA', 'Boston','02117');
--DROP FUNCTION tiger.geocode_intersection(text,text,text,text,text,integer);
CREATE OR REPLACE FUNCTION tiger.geocode_intersection(IN roadway1 text, IN roadway2 text, IN in_state text, IN in_city text DEFAULT '', IN in_zip text DEFAULT '', 
IN num_results integer DEFAULT 10,  OUT ADDY NORM_ADDY,
    OUT GEOMOUT GEOMETRY,
    OUT RATING INTEGER) RETURNS SETOF record  AS
$$
DECLARE
    var_na_road norm_addy;
    var_na_inter1 norm_addy;
    var_sql text := '';
    var_zip varchar(5)[];
    in_statefp varchar(2) ; 
    var_debug boolean := false;
BEGIN
    IF COALESCE(roadway1,'') = '' OR COALESCE(roadway2,'') = '' THEN
        -- not enough to give a result just return
        RETURN ;
    ELSE
        var_na_road := normalize_address(roadway1 || ', ' || COALESCE(in_city,'') || ', ' || in_state || ' ' || in_zip);
        var_na_inter1  := normalize_address(roadway2 || ', ' || COALESCE(in_city,'') || ', ' || in_state || ' ' || in_zip);
    END IF;
    in_statefp := statefp FROM state_lookup As s WHERE s.abbrev = in_state;
    IF COALESCE(in_zip,'') > '' THEN -- limit search to 2 plus or minus the input zip
        var_zip := zip_range(in_zip, -2,2);
    END IF;

    IF var_zip IS NULL AND in_city > '' THEN
        var_zip := array_agg(zip ORDER BY zip) FROM zip_lookup_base WHERE statefp = in_statefp AND lower(city) = lower(in_city);
    END IF;
    
    IF var_zip IS NULL THEN
        var_zip := array_agg(zip ORDER BY zip) FROM zip_lookup_base WHERE statefp = in_statefp AND lower(city) LIKE lower(in_city) || '%'  ;
    END IF; 
    IF var_debug THEN
		RAISE NOTICE 'var_zip: %, city: %', quote_nullable(var_zip), quote_nullable(in_city);	
    END IF;
    var_sql := '
    WITH a AS (SELECT * FROM addr WHERE statefp = $1 AND zip = ANY($5::text[]) ),
    	 e1 AS (SELECT e.the_geom, f.*, a.fromhn, a.tohn, 
    	 			CASE WHEN a.side = ''L'' THEN e.tfidl ELSE e.tfidr END AS tfid, a.zip, a.side
    	 			FROM a
    					INNER JOIN  edges AS e ON a.tlid = e.tlid
    					INNER JOIN
    					featnames AS f ON (e.tlid = f.tlid AND f.statefp = $1 AND e.statefp = f.statefp)
    				WHERE e.statefp = $1 AND  (lower(e.fullname) = $6 or lower(f.name) LIKE $2 || ''%'')
    				),
    	e2 AS (SELECT e.the_geom, f.* , a.fromhn, a.tohn, 
    	 			CASE WHEN a.side = ''L'' THEN e.tfidl ELSE e.tfidr END AS tfid, a.zip, a.side
    				FROM e1 INNER JOIN edges AS e ON (e.statefp = $1 AND ST_Intersects(e.the_geom, e1.the_geom) )
    					INNER JOIN a ON a.tlid = e.tlid
    					INNER JOIN
    					(SELECT * FROM featnames WHERE statefp = $1) AS f ON (e.tlid = f.tlid)
    				WHERE e.statefp = $1 AND  (lower(e.fullname) = $7 or lower(f.name) LIKE $4 || ''%'')
    				)
    SELECT ROW(address, predirabrv , fename, fetype,fedirs,NULL,place, $8,zip, true)::norm_addy, geomout, rating 
        FROM (SELECT DISTINCT ON(e1.tlid, e1.side)
                   CASE WHEN ST_Intersects(ST_StartPoint(e1.the_geom), e2.the_geom) THEN
                                e1.fromhn
                            WHEN ST_Intersects(ST_EndPoint(e1.the_geom), e2.the_geom) THEN
                                e1.tohn END As address, e1.predirabrv , COALESCE(e1.prequalabr || '' '','''' ) || e1.name As fename, 
                             COALESCE(e1.suftypabrv,e1.pretypabrv)  As fetype, e1.sufdirabrv AS fedirs, 
                               p.name As place, e1.zip,
                             CASE WHEN ST_Intersects(ST_StartPoint(e1.the_geom), e2.the_geom) THEN
                                ST_StartPoint(e1.the_geom)
                            WHEN ST_Intersects(ST_EndPoint(e1.the_geom), e2.the_geom) THEN
                                ST_EndPoint(e1.the_geom)
                             ELSE ST_EndPoint(e1.the_geom) END AS geomout ,   
                                CASE WHEN lower(p.name) = $3 THEN 0 ELSE 1 END 
                                + levenshtein_ignore_case(e1.name || COALESCE('' '' || e1.sufqualabr, ''''),$2) +
                                CASE WHEN e1.fullname = $6 THEN 0 ELSE levenshtein_ignore_case(e1.fullname, $6) END +
                                + levenshtein_ignore_case(e2.name || COALESCE('' '' || e2.sufqualabr, ''''),$4)
                                AS rating  
                    FROM e1 
                            INNER JOIN (SELECT * FROM faces WHERE statefp = $1) As fa1 ON (e1.tfid = fa1.tfid)
                            INNER JOIN e2 ON (
                                    ST_Intersects(e1.the_geom, e2.the_geom)  ) 
                          LEFT JOIN place AS p ON (fa1.placefp = p.placefp AND p.statefp = $1 )
                       ORDER BY e1.tlid, e1.side, rating LIMIT 20) As foo ORDER BY rating LIMIT 
                        
                       ' || num_results::text;

    /**var_sql := replace(replace(
	replace(replace(replace(
                replace(
                    replace(
                        replace(var_sql, '$1', quote_nullable(in_statefp)), 
                              '$2', quote_nullable(lower(var_na_road.streetName) ) ),
                      '$3', quote_nullable(lower(in_city)) ),
                      '$4', quote_nullable(lower(var_na_inter1.streetName) ) ),
                      '$5', quote_nullable(var_zip) ),
                      '$6', quote_nullable(lower(var_na_road.streetName || ' ' || COALESCE(var_na_road.streetTypeAbbrev,'') )) ) ,
                      '$7', quote_nullable(lower(var_na_inter1.streetName || ' ' || COALESCE(var_na_inter1.streetTypeAbbrev,'') ) ,
		 '$8', quote_nullable(in_state ) ) ); **/

    IF var_debug THEN
        RAISE NOTICE 'sql: %', replace(replace(
	replace(replace(replace(
                replace(
                    replace(
                        replace(var_sql, '$1', quote_nullable(in_statefp)), 
                              '$2', quote_nullable(lower(var_na_road.streetName) ) ),
                      '$3', quote_nullable(lower(in_city)) ),
                      '$4', quote_nullable(lower(var_na_inter1.streetName) ) ),
                      '$5', quote_nullable(var_zip) ),
                      '$6', quote_nullable(lower(var_na_road.streetName || ' ' || COALESCE(var_na_road.streetTypeAbbrev,'') )) ) ,
                      '$7', quote_nullable(lower(var_na_inter1.streetName || ' ' || COALESCE(var_na_inter1.streetTypeAbbrev,'') ) ) ) ,
		 '$8', quote_nullable(in_state ) );
    END IF;

    RETURN QUERY EXECUTE var_sql USING in_statefp, lower(var_na_road.streetName), lower(in_city), lower(var_na_inter1.streetName), var_zip,
		lower(var_na_road.streetName || ' ' || COALESCE(var_na_road.streetTypeAbbrev,'')),
		lower(var_na_inter1.streetName || ' ' || COALESCE(var_na_inter1.streetTypeAbbrev,'')), in_state ;
END;
$$
  LANGUAGE plpgsql IMMUTABLE
  COST 1000
  ROWS 10;

  
