--$Id$
 /*** 
 * 
 * Copyright (C) 2011 Regina Obe and Leo Hsu (Paragon Corporation)
 **/
-- Note we are wrapping this in a function so we can make it immutable and those useable in an index
-- It also allows us to shorten and possibly better cache the repetitive pattern in the code 
-- greatest(to_number(b.fromhn,''99999999''),to_number(b.tohn,''99999999'')) 
-- and least(to_number(b.fromhn,''99999999''),to_number(b.tohn,''99999999''))
CREATE OR REPLACE FUNCTION tiger.least_hn(fromhn varchar, tohn varchar)
  RETURNS integer AS
$$ SELECT least(to_number( CASE WHEN trim($1) ~ '^[0-9]+$' THEN $1 ELSE '0' END,'99999999'),to_number(CASE WHEN trim($2) ~ '^[0-9]+$' THEN $2 ELSE '0' END,'99999999') )::integer;  $$
  LANGUAGE sql IMMUTABLE
  COST 5;
  
-- Note we are wrapping this in a function so we can make it immutable (for some reason least and greatest aren't considered immutable)
-- and thu useable in an index or cacheable for multiple calls
CREATE OR REPLACE FUNCTION greatest_hn(fromhn varchar, tohn varchar)
  RETURNS integer AS
$$ SELECT greatest(to_number( CASE WHEN trim($1) ~ '^[0-9]+$' THEN $1 ELSE '0' END,'99999999'),to_number(CASE WHEN trim($2) ~ '^[0-9]+$' THEN $2 ELSE '0' END,'99999999') )::integer;  $$
  LANGUAGE sql IMMUTABLE
  COST 5;
