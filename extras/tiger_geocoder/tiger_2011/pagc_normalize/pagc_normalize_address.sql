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
  rec RECORD;
  rawInput VARCHAR;

BEGIN
--$Id$-
  result.parsed := FALSE;

  rawInput := trim(in_rawinput);

  rec := (SELECT standardize_address( 'select seq, word::text, stdword::text, token from tiger.pagc_gaz union all select seq, word::text, stdword::text, token from tiger.pagc_lex '
       , 'select seq, word::text, stdword::text, token from tiger.pagc_gaz order by id'
       , 'select * from tiger.pagc_rules order by id'
, 'select 0::int4 as id, ' || quote_literal(COALESCE(address1,'')) || '::text As micro, 
   ' || quote_literal(COALESCE(city || ', ','') || COALESCE(state || ' ', '') || COALESCE(zip,'')) || '::text As macro') As pagc_addr
 FROM  (SELECT * FROM parse_address(rawInput) ) As a ) ;
 -- For address number only put numbers and stop if reach a non-number e.g. 123-456 will return 123
  result.address := to_number(substring(rec.house_num, '[0-9]+'), '99999999999');
   --get rid of extraneous spaces before we return
  result.zip := rec.postcode;
  result.streetName := trim(rec.name);
  result.location := trim(rec.city);
  result.stateAbbrev := trim(rec.state);
  --this should be broken out separately like pagc, but normalizer doesn't have a slot for it
  result.streettypeAbbrev := trim(COALESCE(rec.suftype, rec.pretype)); 
  result.preDirAbbrev := trim(rec.predir);
  result.postDirAbbrev := trim(rec.sufdir);
  result.internal := trim(rec.unit);
  result.parsed := TRUE;
  RETURN result;
END
$$
  LANGUAGE plpgsql IMMUTABLE
  COST 100;