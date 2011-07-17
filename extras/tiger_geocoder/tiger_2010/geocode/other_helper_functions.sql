--$Id$
 /*** 
 * 
 * Copyright (C) 2011 Regina Obe and Leo Hsu (Paragon Corporation)
 **/
-- Note we are wrapping this in a function so we can make it immutable and those useable in an index
-- It also allows us to shorten and possibly better cache the repetitive pattern in the code 
-- greatest(to_number(b.fromhn,''99999999''),to_number(b.tohn,''99999999'')) 
-- and least(to_number(b.fromhn,''99999999''),to_number(b.tohn,''99999999''))
CREATE OR REPLACE FUNCTION least_hn(fromhn varchar, tohn varchar)
  RETURNS integer AS
$$ SELECT least(to_number( CASE WHEN trim($1) ~ '^[0-9]+$' THEN $1 ELSE '0' END,'9999999'),to_number(CASE WHEN trim($2) ~ '^[0-9]+$' THEN $2 ELSE '0' END,'9999999') )::integer;  $$
  LANGUAGE sql IMMUTABLE
  COST 200;
  
-- Note we are wrapping this in a function so we can make it immutable (for some reason least and greatest aren't considered immutable)
-- and thu useable in an index or cacheable for multiple calls
CREATE OR REPLACE FUNCTION greatest_hn(fromhn varchar, tohn varchar)
  RETURNS integer AS
$$ SELECT greatest(to_number( CASE WHEN trim($1) ~ '^[0-9]+$' THEN $1 ELSE '0' END,'99999999'),to_number(CASE WHEN trim($2) ~ '^[0-9]+$' THEN $2 ELSE '0' END,'99999999') )::integer;  $$
  LANGUAGE sql IMMUTABLE
  COST 200;
  
-- Returns an absolute difference between two zips
-- This is generally more efficient than doing levenshtein
-- Since when people get the wrong zip, its usually off by one or 2 numeric distance
-- We only consider the first 5 digits
CREATE OR REPLACE FUNCTION diff_zip(zip1 varchar, zip2 varchar)
  RETURNS integer AS
$$ SELECT abs(to_number( CASE WHEN trim(substring($1,1,5)) ~ '^[0-9]+$' THEN $1 ELSE '0' END,'99999')::integer - to_number( CASE WHEN trim(substring($2,1,5)) ~ '^[0-9]+$' THEN $2 ELSE '0' END,'99999')::integer )::integer;  $$
  LANGUAGE sql IMMUTABLE STRICT
  COST 200;
  
-- function return  true or false if 2 numeric streets are equal such as 15th St, 23rd st
-- it compares just the numeric part of the street for equality
-- PURPOSE: handle bad formats such as 23th St so 23th St = 23rd St
-- as described in: http://trac.osgeo.org/postgis/ticket/1068
-- This will always return false if one of the streets is not a numeric street
-- By numeric it must start with numbers (allow fractions such as 1/2 and spaces such as 12 1/2th) and be less than 10 characters
CREATE OR REPLACE FUNCTION numeric_streets_equal(input_street varchar, output_street varchar)
    RETURNS boolean AS
$$
    SELECT COALESCE(length($1) < 10 AND length($2) < 10 
            AND $1 ~ E'^[0-9\/\s]+' AND $2 ~ E'^[0-9\/\s]+' 
            AND  trim(substring($1, E'^[0-9\/\s]+')) = trim(substring($2, E'^[0-9\/\s]+')), false); 
$$
LANGUAGE sql IMMUTABLE
COST 5;

-- Generate script to create missing indexes in tiger tables. 
-- This will generate sql you can run to index commonly used join columns in geocoder for tiger and tiger_data schemas --
CREATE OR REPLACE FUNCTION missing_indexes_generate_script()
RETURNS text AS
$$
SELECT array_to_string(ARRAY(
-- basic btree regular indexes
SELECT 'CREATE INDEX idx_' || c.table_schema || '_' || c.table_name || '_' || c.column_name || ' ON ' || c.table_schema || '.' || c.table_name || ' USING btree(' || c.column_name || ')' As index
FROM (SELECT table_name, table_schema  FROM 
	information_schema.tables WHERE table_type = 'BASE TABLE') As t  INNER JOIN
	(SELECT * FROM information_schema.columns WHERE column_name IN('countyfp','tlid', 'tfidl', 'tfidr', 'tfid', 'zip') ) AS c  
		ON (t.table_name = c.table_name AND t.table_schema = c.table_schema)
		LEFT JOIN pg_catalog.pg_indexes i ON 
			(i.tablename = c.table_name AND i.schemaname = c.table_schema 
				AND  indexdef LIKE '%' || c.column_name || '%') 
WHERE i.tablename IS NULL AND c.table_schema IN('tiger','tiger_data')
-- Gist spatial indexes --
UNION ALL
SELECT 'CREATE INDEX idx_' || c.table_schema || '_' || c.table_name || '_' || c.column_name || '_gist ON ' || c.table_schema || '.' || c.table_name || ' USING gist(' || c.column_name || ')' As index
FROM (SELECT table_name, table_schema FROM 
	information_schema.tables WHERE table_type = 'BASE TABLE') As t  INNER JOIN
	(SELECT * FROM information_schema.columns WHERE column_name IN('the_geom', 'geom') ) AS c  
		ON (t.table_name = c.table_name AND t.table_schema = c.table_schema)
		LEFT JOIN pg_catalog.pg_indexes i ON 
			(i.tablename = c.table_name AND i.schemaname = c.table_schema 
				AND  indexdef LIKE '%' || c.column_name || '%') 
WHERE i.tablename IS NULL AND c.table_schema IN('tiger','tiger_data')
-- Soundex indexes --
UNION ALL
SELECT 'CREATE INDEX idx_' || c.table_schema || '_' || c.table_name || '_snd_' || c.column_name || ' ON ' || c.table_schema || '.' || c.table_name || ' USING btree(soundex(' || c.column_name || '))' As index
FROM (SELECT table_name, table_schema FROM 
	information_schema.tables WHERE table_type = 'BASE TABLE') As t  INNER JOIN
	(SELECT * FROM information_schema.columns WHERE column_name IN('name', 'place', 'city') ) AS c  
		ON (t.table_name = c.table_name AND t.table_schema = c.table_schema)
		LEFT JOIN pg_catalog.pg_indexes i ON 
			(i.tablename = c.table_name AND i.schemaname = c.table_schema 
				AND  indexdef LIKE '%soundex(%' || c.column_name || '%' AND indexdef LIKE '%_snd_' || c.column_name || '%' ) 
WHERE i.tablename IS NULL AND c.table_schema IN('tiger','tiger_data') 
    AND (c.table_name LIKE '%county%' OR c.table_name LIKE '%featnames'
    OR c.table_name  LIKE '%place' or c.table_name LIKE '%zip%') 
-- Lower indexes --
UNION ALL
SELECT 'CREATE INDEX idx_' || c.table_schema || '_' || c.table_name || '_lower_' || c.column_name || ' ON ' || c.table_schema || '.' || c.table_name || ' USING btree(lower(' || c.column_name || '))' As index
FROM (SELECT table_name, table_schema FROM 
	information_schema.tables WHERE table_type = 'BASE TABLE') As t  INNER JOIN
	(SELECT * FROM information_schema.columns WHERE column_name IN('name', 'place', 'city') ) AS c  
		ON (t.table_name = c.table_name AND t.table_schema = c.table_schema)
		LEFT JOIN pg_catalog.pg_indexes i ON 
			(i.tablename = c.table_name AND i.schemaname = c.table_schema 
				AND  indexdef LIKE '%lower(%' || c.column_name || '%') 
WHERE i.tablename IS NULL AND c.table_schema IN('tiger','tiger_data') 
    AND (c.table_name LIKE '%county%' OR c.table_name LIKE '%featnames' OR c.table_name  LIKE '%place' or c.table_name LIKE '%zip%') 
-- Least address index btree least_hn(fromhn, tohn)
UNION ALL
SELECT 'CREATE INDEX idx_' || c.table_schema || '_' || c.table_name || '_least_address' || ' ON ' || c.table_schema || '.' || c.table_name || ' USING btree(least_hn(fromhn, tohn))' As index
FROM (SELECT table_name, table_schema FROM 
	information_schema.tables WHERE table_type = 'BASE TABLE' AND table_name LIKE '%addr' AND table_schema IN('tiger','tiger_data')) As t  INNER JOIN
	(SELECT * FROM information_schema.columns WHERE column_name IN('fromhn') ) AS c  
		ON (t.table_name = c.table_name AND t.table_schema = c.table_schema)
		LEFT JOIN pg_catalog.pg_indexes i ON 
			(i.tablename = c.table_name AND i.schemaname = c.table_schema 
				AND  indexdef LIKE '%least_hn(%' || c.column_name || '%') 
WHERE i.tablename IS NULL
-- var_ops fullname --
UNION ALL
SELECT 'CREATE INDEX idx_' || c.table_schema || '_' || c.table_name || '_l' || c.column_name || '_var_ops' || ' ON ' || c.table_schema || '.' || c.table_name || ' USING btree(lower(' || c.column_name || ') varchar_pattern_ops);' As index
FROM (SELECT table_name, table_schema FROM 
	information_schema.tables WHERE table_type = 'BASE TABLE' AND table_name LIKE '%featnames' AND table_schema IN('tiger','tiger_data')) As t  INNER JOIN
	(SELECT * FROM information_schema.columns WHERE column_name IN('fullname') ) AS c  
		ON (t.table_name = c.table_name AND t.table_schema = c.table_schema)
		LEFT JOIN pg_catalog.pg_indexes i ON 
			(i.tablename = c.table_name AND i.schemaname = c.table_schema 
				AND  indexdef LIKE '%btree%(%lower%' || c.column_name || ')%varchar_pattern_ops%') 
WHERE i.tablename IS NULL
-- var_ops mtfcc --
UNION ALL
SELECT 'CREATE INDEX idx_' || c.table_schema || '_' || c.table_name || '_' || c.column_name || '_var_ops' || ' ON ' || c.table_schema || '.' || c.table_name || ' USING btree(' || c.column_name || ' varchar_pattern_ops);' As index
FROM (SELECT table_name, table_schema FROM 
	information_schema.tables WHERE table_type = 'BASE TABLE' AND table_name LIKE '%featnames' or table_name LIKE '%edges' AND table_schema IN('tiger','tiger_data')) As t  INNER JOIN
	(SELECT * FROM information_schema.columns WHERE column_name IN('mtfcc') ) AS c  
		ON (t.table_name = c.table_name AND t.table_schema = c.table_schema)
		LEFT JOIN pg_catalog.pg_indexes i ON 
			(i.tablename = c.table_name AND i.schemaname = c.table_schema 
				AND  indexdef LIKE '%btree%(' || c.column_name || '%varchar_pattern_ops%') 
WHERE i.tablename IS NULL
--full text indexes on name field--
/**UNION ALL
SELECT 'CREATE INDEX idx_' || c.table_schema || '_' || c.table_name || '_fullname_ft_gist' || ' ON ' || c.table_schema || '.' || c.table_name || ' USING gist(to_tsvector(''english'',fullname))' As index
FROM (SELECT table_name, table_schema FROM 
	information_schema.tables WHERE table_type = 'BASE TABLE' AND table_name LIKE '%featnames' AND table_schema IN('tiger','tiger_data')) As t  INNER JOIN
	(SELECT * FROM information_schema.columns WHERE column_name IN('fullname') ) AS c  
		ON (t.table_name = c.table_name AND t.table_schema = c.table_schema)
		LEFT JOIN pg_catalog.pg_indexes i ON 
			(i.tablename = c.table_name AND i.schemaname = c.table_schema 
				AND  indexdef LIKE '%to_tsvector(%' || c.column_name || '%') 
WHERE i.tablename IS NULL **/

-- trigram index --
/**UNION ALL
SELECT 'CREATE INDEX idx_' || c.table_schema || '_' || c.table_name || '_' || c.column_name || '_trgm_gist' || ' ON ' || c.table_schema || '.' || c.table_name || ' USING gist(' || c.column_name || ' gist_trgm_ops);' As index
FROM (SELECT table_name, table_schema FROM 
	information_schema.tables WHERE table_type = 'BASE TABLE' AND table_name LIKE '%featnames' AND table_schema IN('tiger','tiger_data')) As t  INNER JOIN
	(SELECT * FROM information_schema.columns WHERE column_name IN('fullname', 'name') ) AS c  
		ON (t.table_name = c.table_name AND t.table_schema = c.table_schema)
		LEFT JOIN pg_catalog.pg_indexes i ON 
			(i.tablename = c.table_name AND i.schemaname = c.table_schema 
				AND  indexdef LIKE '%gist%(' || c.column_name || '%gist_trgm_ops%') 
WHERE i.tablename IS NULL **/ 
ORDER BY 1), E';\r');
$$
LANGUAGE sql VOLATILE;


CREATE OR REPLACE FUNCTION install_missing_indexes() RETURNS boolean
AS
$$
DECLARE var_sql text = missing_indexes_generate_script();
BEGIN
	EXECUTE(var_sql);
	RETURN true;
END
$$
language plpgsql;