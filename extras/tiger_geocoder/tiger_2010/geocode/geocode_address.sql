--$Id$
CREATE OR REPLACE FUNCTION geocode_address(IN parsed norm_addy, max_results integer DEFAULT 10, restrict_geom geometry DEFAULT NULL, OUT addy norm_addy, OUT geomout geometry, OUT rating integer)
  RETURNS SETOF record AS
$$
DECLARE
  results RECORD;
  zip_info RECORD;
  stmt VARCHAR;
  in_statefp VARCHAR;
  exact_street boolean := false;
  var_debug boolean := false;
  var_sql text := '';
  var_n integer := 0;
  var_restrict_geom geometry := NULL;
  var_bfilter text := null;
BEGIN
  IF parsed.streetName IS NULL THEN
    -- A street name must be given.  Think about it.
    RETURN;
  END IF;

  ADDY.internal := parsed.internal;

  in_statefp := statefp FROM state_lookup As s WHERE s.abbrev = parsed.stateAbbrev;
  
  IF restrict_geom IS NOT NULL THEN
  		IF ST_SRID(restrict_geom) < 1 OR ST_SRID(restrict_geom) = 4236 THEN 
  		-- basically has no srid or if wgs84 close enough to NAD 83 -- assume same as data
  			var_restrict_geom = ST_SetSRID(restrict_geom,4269);
  		ELSE
  		--transform and snap
  			var_restrict_geom = ST_SnapToGrid(ST_Transform(restrict_geom, 4269), 0.000001);
  		END IF;
  END IF;
  var_bfilter := ' SELECT zcta5ce FROM zcta5 AS zc  
                    WHERE zc.statefp = ' || quote_nullable(in_statefp) || ' 
                        AND ST_Intersects(zc.the_geom, ' || quote_literal(var_restrict_geom::text) || '::geometry)  ' ;

  -- There are a couple of different things to try, from the highest preference and falling back
  -- to lower-preference options.
  -- We start out with zip-code matching, where the zip code could possibly be in more than one
  -- state.  We loop through each state its in.
  -- Next, we try to find the location in our side-table, which is based off of the 'place' data exact first then sounds like
  -- Next, we look up the location/city and use the zip code which is returned from that
  -- Finally, if we didn't get a zip code or a city match, we fall back to just a location/street
  -- lookup to try and find *something* useful.
  -- In the end, we *have* to find a statefp, one way or another.
  var_sql := 
  ' SELECT statefp,location,a.zip,exact,min(pref) FROM
    (SELECT zip_state.statefp as statefp,$1 as location, true As exact, ARRAY[zip_state.zip] as zip,1 as pref
        FROM zip_state WHERE zip_state.zip = $2 
            AND (' || quote_nullable(in_statefp) || ' IS NULL OR zip_state.statefp = ' || quote_nullable(in_statefp) || ')
          ' || COALESCE(' AND zip_state.zip IN(' || var_bfilter || ')', '') ||
        ' UNION SELECT zip_state_loc.statefp,zip_state_loc.place As location,false As exact, array_agg(zip_state_loc.zip) AS zip,1 + abs(COALESCE(diff_zip(max(zip), $2),0) - COALESCE(diff_zip(min(zip), $2),0)) As pref
              FROM zip_state_loc
             WHERE zip_state_loc.statefp = ' || quote_nullable(in_statefp) || ' 
                   AND lower($1) = lower(zip_state_loc.place) '  || COALESCE(' AND zip_state_loc.zip IN(' || var_bfilter || ')', '') ||
        '     GROUP BY zip_state_loc.statefp,zip_state_loc.place
      UNION SELECT zip_state_loc.statefp,zip_state_loc.place As location,false As exact, array_agg(zip_state_loc.zip),3
              FROM zip_state_loc
             WHERE zip_state_loc.statefp = ' || quote_nullable(in_statefp) || '
                   AND soundex($1) = soundex(zip_state_loc.place)
             GROUP BY zip_state_loc.statefp,zip_state_loc.place
      UNION SELECT zip_lookup_base.statefp,zip_lookup_base.city As location,false As exact, array_agg(zip_lookup_base.zip),4
              FROM zip_lookup_base
             WHERE zip_lookup_base.statefp = ' || quote_nullable(in_statefp) || '
                         AND (soundex($1) = soundex(zip_lookup_base.city) OR soundex($1) = soundex(zip_lookup_base.county))
             GROUP BY zip_lookup_base.statefp,zip_lookup_base.city
      UNION SELECT ' || quote_nullable(in_statefp) || ' As statefp,$1 As location,false As exact,NULL, 5) as a ' 
      ' WHERE a.statefp IS NOT NULL 
      GROUP BY statefp,location,a.zip,exact, pref ORDER BY exact desc, pref, zip';
  /** FOR zip_info IN     SELECT statefp,location,zip,exact,min(pref) FROM
    (SELECT zip_state.statefp as statefp,parsed.location as location, true As exact, ARRAY[zip_state.zip] as zip,1 as pref
        FROM zip_state WHERE zip_state.zip = parsed.zip 
            AND (in_statefp IS NULL OR zip_state.statefp = in_statefp)
        UNION SELECT zip_state_loc.statefp,parsed.location,false As exact, array_agg(zip_state_loc.zip),2 + diff_zip(zip[1], parsed.zip)
              FROM zip_state_loc
             WHERE zip_state_loc.statefp = in_statefp
                   AND lower(parsed.location) = lower(zip_state_loc.place)
             GROUP BY zip_state_loc.statefp,parsed.location
      UNION SELECT zip_state_loc.statefp,parsed.location,false As exact, array_agg(zip_state_loc.zip),3
              FROM zip_state_loc
             WHERE zip_state_loc.statefp = in_statefp
                   AND soundex(parsed.location) = soundex(zip_state_loc.place)
             GROUP BY zip_state_loc.statefp,parsed.location
      UNION SELECT zip_lookup_base.statefp,parsed.location,false As exact, array_agg(zip_lookup_base.zip),4
              FROM zip_lookup_base
             WHERE zip_lookup_base.statefp = in_statefp
                         AND (soundex(parsed.location) = soundex(zip_lookup_base.city) OR soundex(parsed.location) = soundex(zip_lookup_base.county))
             GROUP BY zip_lookup_base.statefp,parsed.location
      UNION SELECT in_statefp,parsed.location,false As exact,NULL, 5) as a
        --JOIN (VALUES (true),(false)) as b(exact) on TRUE
      WHERE statefp IS NOT NULL
      GROUP BY statefp,location,zip,exact, pref ORDER BY exact desc, pref, zip  **/
  FOR zip_info IN EXECUTE var_sql USING parsed.location, parsed.zip  LOOP
  -- For zip distance metric we consider both the distance of zip based on numeric as well aa levenshtein
    stmt := 'SELECT DISTINCT ON (sub.predirabrv,sub.name,sub.suftypabrv,sub.sufdirabrv,coalesce(p.name,zip.city,cs.name,co.name),s.stusps,sub.zip)'
         || '    sub.predirabrv   as fedirp,'
         || '    sub.name         as fename,'
         || '    sub.suftypabrv   as fetype,'
         || '    sub.sufdirabrv   as fedirs,'
         || '    coalesce(p.name,zip.city,cs.name,co.name)::varchar as place,'
         || '    s.stusps as state,'
         || '    sub.zip as zip,'
         || '    interpolate_from_address($1, to_number(sub.fromhn,''99999999'')::integer,'
         || '        to_number(sub.tohn,''99999999'')::integer, e.the_geom) as address_geom,'
         || '       sub.sub_rating + '
         || CASE WHEN parsed.zip > '' THEN '  least((coalesce(diff_zip($7 , sub.zip),0) *1.00/2)::integer, coalesce(levenshtein_ignore_case($7, sub.zip),0) ) '
            ELSE '3' END::text 
         || ' + coalesce(least(levenshtein_ignore_case($3, coalesce(p.name,zip.city,cs.name,co.name)), levenshtein_ignore_case($3, coalesce(cs.name,co.name))),5)'
         || '    as sub_rating,'
         || '    sub.exact_address as exact_address'
         || ' FROM ('
         || '  SELECT tlid, predirabrv, name, suftypabrv, sufdirabrv, fromhn, tohn, side, statefp, zip, rate_attributes($5, a.predirabrv,'
         || '    $2, a.name, $4,'
         || '    a.suftypabrv, $6,'
         || '    a.sufdirabrv) + '
         || '    CASE '
         || '        WHEN $1::integer IS NULL OR b.fromhn IS NULL THEN 20'
         || '        WHEN $1::integer >= least_hn(b.fromhn, b.tohn) '
         || '            AND $1::integer <= greatest_hn(b.fromhn,b.tohn)'
         || '            AND ($1::integer % 2) = (to_number(b.fromhn,''99999999'') % 2)::integer'
         || '            THEN 0'
         || '        WHEN $1::integer >= least_hn(b.fromhn,b.tohn)'
         || '            AND $1::integer <= greatest_hn(b.fromhn,b.tohn)'
         || '            THEN 2'
         || '        ELSE'
         || '            ((1.0 - '
         ||              '(least_hn($1::text,least_hn(b.fromhn,b.tohn)::text)::numeric /'
         ||              ' greatest(1,greatest_hn($1::text,greatest_hn(b.fromhn,b.tohn)::text)))'
         ||              ') * 5)::integer + 5'
         || '        END'
         || '    as sub_rating,$1::integer >= least_hn(b.fromhn,b.tohn) '
         || '            AND $1::integer <= greatest_hn(b.fromhn,b.tohn) '
         || '            AND ($1 % 2)::numeric::integer = (to_number(b.fromhn,''99999999'') % 2)'
         || '    as exact_address'
         || '  FROM featnames a join addr b using (tlid,statefp)'
         || '  WHERE'
         || '        statefp = ' || quote_literal(zip_info.statefp) || ''
         || coalesce('    AND b.zip IN (''' || array_to_string(zip_info.zip,''',''') || ''') ','')
         || CASE WHEN zip_info.exact
                 THEN '    AND (lower($2) = lower(a.name) OR  numeric_streets_equal($2, a.name) ) '
                 ELSE '    AND (soundex($2) = soundex(a.name) OR  numeric_streets_equal($2, a.name) ) '
            END
         || '  ORDER BY 11'
         || '  LIMIT 20'
         || '    ) AS sub'
         || '  JOIN edges e ON (' || quote_literal(zip_info.statefp) || ' = e.statefp AND sub.tlid = e.tlid ' 
         ||   CASE WHEN var_restrict_geom IS NOT NULL THEN ' AND ST_Intersects(e.the_geom, $8) '  ELSE '' END || ') '
         || '  JOIN state s ON (' || quote_literal(zip_info.statefp) || ' = s.statefp)'
         || '  JOIN faces f ON (' || quote_literal(zip_info.statefp) || ' = f.statefp AND (e.tfidl = f.tfid OR e.tfidr = f.tfid))'
         || '  LEFT JOIN zip_lookup_base zip ON (sub.zip = zip.zip AND zip.statefp=' || quote_literal(zip_info.statefp) || ')'
         || '  LEFT JOIN place p ON (' || quote_literal(zip_info.statefp) || ' = p.statefp AND f.placefp = p.placefp)'
         || '  LEFT JOIN county co ON (' || quote_literal(zip_info.statefp) || ' = co.statefp AND f.countyfp = co.countyfp)'
         || '  LEFT JOIN cousub cs ON (' || quote_literal(zip_info.statefp) || ' = cs.statefp AND cs.cosbidfp = sub.statefp || co.countyfp || f.cousubfp)'
         || ' WHERE'
         || '  (sub.side = ''L'' and e.tfidl = f.tfid) OR (sub.side = ''R'' and e.tfidr = f.tfid)'
         || ' ORDER BY 1,2,3,4,5,6,7,9'
         || ' LIMIT 10'
         ;
    IF var_debug THEN
        RAISE NOTICE '%', stmt;
    END IF;
    -- If we got an exact street match then when we hit the non-exact
    -- set of tests, just drop out.
    IF NOT zip_info.exact AND exact_street THEN
        RETURN;
    END IF;

    FOR results IN EXECUTE stmt USING parsed.address,parsed.streetName, parsed.location, parsed.streetTypeAbbrev, parsed.preDirAbbrev, parsed.postDirAbbrev, parsed.zip, var_restrict_geom LOOP

      -- If we found a match with an exact street, then don't bother
      -- trying to do non-exact matches
      IF zip_info.exact THEN
        exact_street := true;
      END IF;

      IF results.exact_address THEN
        ADDY.address := parsed.address;
      ELSE
        ADDY.address := NULL;
      END IF;

      ADDY.preDirAbbrev     := results.fedirp;
      ADDY.streetName       := results.fename;
      ADDY.streetTypeAbbrev := results.fetype;
      ADDY.postDirAbbrev    := results.fedirs;
      ADDY.location         := results.place;
      ADDY.stateAbbrev      := results.state;
      ADDY.zip              := results.zip;
      ADDY.parsed := TRUE;

      GEOMOUT := results.address_geom;
      RATING := results.sub_rating;
      var_n := var_n + 1;

      RETURN NEXT;

      -- If we get an exact match, then just return that
      IF RATING = 0 THEN
        RETURN;
      END IF;

    END LOOP;
    IF var_n > max_results  THEN --we have exceeded our desired limit
        RETURN;
    END IF;
  END LOOP;

  RETURN;
END;
$$
  LANGUAGE 'plpgsql' STABLE COST 1000 ROWS 50;


