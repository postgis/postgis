\timing
SELECT pprint_addy(addy[1]), addy FROM reverse_geocode(ST_Point(-71.27593,42.33891));  -- I 90 Exit 14, Weston MA
SELECT pprint_addy(addy[1]), addy FROM reverse_geocode(ST_Point(-71.85335,42.19262));  -- I 90 Exit 10, Auburn, MA 01501
SELECT pprint_addy(addy[1]), addy FROM reverse_geocode(ST_Point(-71.057811,42.358274)); -- 1 Devonshire Place (washington st area)
SELECT pprint_addy(addy[1]), addy FROM reverse_geocode(ST_Point(-71.123848,42.41115)); -- 30 capen, Medford, MA 02155
SELECT pprint_addy(addy[1]), addy FROM reverse_geocode(ST_Point(-71.09436,42.35981)); -- 77 Massachusetts Ave, Cambridge, MA 02139
\timing