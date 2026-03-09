-- pagc_normalize_address(addressString)
-- This takes an address string and parses it into address (internal/street)
-- street name, type, direction prefix and suffix, location, state and
-- zip code, depending on what can be found in the string.
-- This is a drop in replacement for packaged normalize_address
-- that uses the pagc address standardizer C library instead
-- Keep the wrapper in plpgsql so the optional address_standardizer extension
-- is only resolved when PAGC parsing is invoked, while still using a single
-- standardize_address() call to preserve the wrapper speedup work.
-- USAGE: SELECT * FROM tiger.pagc_normalize_address('One Devonshire Place, PH 301, Boston, MA 02109');
SELECT tiger.SetSearchPathForInstall('tiger');
CREATE OR REPLACE FUNCTION pagc_normalize_address(in_rawinput character varying)
  RETURNS norm_addy AS
$$
DECLARE
  sa RECORD;
  parsed RECORD;
  raw_input VARCHAR;
BEGIN
  raw_input := trim(in_rawinput);

  -- Preserve parse_address()'s macro-only ZIP and country detection for
  -- non-street inputs while keeping a single standardize_address() call.
  SELECT *
    INTO parsed
  FROM parse_address(raw_input);

  SELECT *
    INTO sa
  FROM standardize_address(
      'pagc_lex',
      'pagc_gaz',
      'pagc_rules',
      raw_input
  ) AS sa;

  RETURN ROW(
      to_number(substring(sa.house_num, '[0-9]+'), '99999999'),
      NULLIF(trim(sa.predir), ''),
      NULLIF(trim(sa.name), ''),
      NULLIF(trim(COALESCE(sa.suftype, sa.pretype)), ''),
      NULLIF(trim(sa.sufdir), ''),
      NULLIF(trim(regexp_replace(replace(COALESCE(sa.unit, ''), '#', ''), '([0-9]+)\s+([A-Za-z]){0,1}', E'\\1\\2')), ''),
      COALESCE(NULLIF(trim(sa.city), ''), NULLIF(trim(parsed.city), '')),
      COALESCE(NULLIF(trim(sa.state), ''), NULLIF(trim(parsed.state), '')),
      COALESCE(NULLIF(split_part(COALESCE(sa.postcode, ''), '-', 1), ''), NULLIF(parsed.zip, '')),
      TRUE,
      COALESCE(NULLIF(split_part(COALESCE(sa.postcode, ''), '-', 2), ''), NULLIF(parsed.zipplus, '')),
      NULLIF(sa.house_num, ''),
      COALESCE(NULLIF(trim(sa.country), ''), NULLIF(trim(parsed.country), ''), 'US')
  )::norm_addy;
END
$$
  LANGUAGE plpgsql IMMUTABLE STRICT
  COST 100;
