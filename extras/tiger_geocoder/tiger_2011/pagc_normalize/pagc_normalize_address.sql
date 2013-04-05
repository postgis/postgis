--$Id$-
-- pagc_normalize_address(addressString)
-- This takes an address string and parses it into address (internal/street)
-- street name, type, direction prefix and suffix, location, state and
-- zip code, depending on what can be found in the string.
-- This is a drop in replacement for packaged normalize_address
-- that uses the pagc address standardizer C library instead
-- USAGE: SELECT * FROM tiger.pagc_normalize_address('One Devonshire Place, PH 301, Boston, MA 02109');
SELECT tiger.SetSearchPathForInstall('tiger');
CREATE OR REPLACE FUNCTION pagc_normalize_address(in_rawinput character varying)
  RETURNS norm_addy AS
$$
DECLARE
  result norm_addy;
  var_rec RECORD;
  var_parse_rec RECORD;
  rawInput VARCHAR;

BEGIN
--$Id$-
  result.parsed := FALSE;

  rawInput := trim(in_rawinput);
  var_parse_rec := parse_address(rawInput);
  result.location := var_parse_rec.city;
  result.stateAbbrev := trim(var_parse_rec.state);
  result.zip := var_parse_rec.zip;

  var_rec := (SELECT standardize_address( 'select seq, word::text, stdword::text, token from tiger.pagc_gaz union all select seq, word::text, stdword::text, token from tiger.pagc_lex '
       , 'select seq, word::text, stdword::text, token from tiger.pagc_gaz order by id'
       , 'select * from tiger.pagc_rules order by id'
, 'select 0::int4 as id, ' || quote_literal(COALESCE(var_parse_rec.address1,'')) || '::text As micro, 
   ' || quote_literal(COALESCE(var_parse_rec.city || ', ','') || COALESCE(var_parse_rec.state || ' ', '') || COALESCE(var_parse_rec.zip,'')) || '::text As macro') As pagc_addr ) ;
 -- For address number only put numbers and stop if reach a non-number e.g. 123-456 will return 123
  result.address := to_number(substring(var_rec.house_num, '[0-9]+'), '99999999999');
   --get rid of extraneous spaces before we return
  --result.zip := var_rec.postcode;
  result.streetName := trim(var_rec.name);
  --result.location := trim(var_rec.city);
  --result.stateAbbrev := trim(rec.state);
  --this should be broken out separately like pagc, but normalizer doesn't have a slot for it
  result.streettypeAbbrev := trim(COALESCE(var_rec.suftype, var_rec.pretype)); 
  result.preDirAbbrev := trim(var_rec.predir);
  result.postDirAbbrev := trim(var_rec.sufdir);
  result.internal := trim(var_rec.unit);
  result.parsed := TRUE;
  RETURN result;
END
$$
  LANGUAGE plpgsql IMMUTABLE
  COST 100;