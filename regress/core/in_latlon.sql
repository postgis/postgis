SELECT 'dms_two_arg', ST_AsEWKT(ST_FromLatLonText('31°55''12.000"N', '98°58''48.000"W'));
SELECT 'dms_one_arg', ST_AsEWKT(ST_FromLatLonText('31°55''12.000"N 98°58''48.000"W'));
SELECT 'decimal_signed', ST_AsEWKT(ST_FromLatLonText('+32.30642, -122.61458'));
SELECT 'lon_lat_by_dir', ST_AsEWKT(ST_FromLatLonText('the coordinates were 122°36''52.5" W and 32°18''23.1" N'));
SELECT 'colon_prefix', round(ST_X(g)::numeric, 9), round(ST_Y(g)::numeric, 9)
FROM (SELECT ST_FromLatLonText('N40:26:46.302 W079:58:55.903') AS g) AS f;
SELECT 'negative_zero_pair', ST_AsEWKT(ST_FromLatLonText('0 0 -0 30'));
SELECT 'negative_zero_two_arg', ST_AsEWKT(ST_FromLatLonText('-0°30''', '-0°30'''));
SELECT 'mixed_prefix_suffix', ST_AsEWKT(ST_FromLatLonText('N40 79W'));
SELECT 'mixed_prefix_suffix_dms', round(ST_X(g)::numeric, 9), round(ST_Y(g)::numeric, 9)
FROM (SELECT ST_FromLatLonText('N40 26 46.302 079 58 55.903W') AS g) AS f;
SELECT 'roundtrip_default', ST_AsEWKT(ST_FromLatLonText(ST_AsLatLonText('SRID=4326;POINT(-3.2342342 -2.32498)'::geometry)));
SELECT 'compact_zero_padded', ST_FromLatLonText('0030N 0045W');
SELECT 'decimal_dms_mix', ST_FromLatLonText('32.5°30'' N 122.5°30'' W');
SELECT 'exponent_notation', ST_FromLatLonText('1e2 2e2');
SELECT 'ambiguous_numbers', ST_FromLatLonText('1.2 3.4 5.6 7.8');
SELECT 'mixed_components', ST_FromLatLonText('32°23.45'' N 122.61458° W');
SELECT 'too_many_directions', ST_FromLatLonText('32.30642° NE 122.61458° W');
