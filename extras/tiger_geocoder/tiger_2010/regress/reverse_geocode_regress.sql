\timing
SELECT pprint_addy(addy[1]), addy FROM reverse_geocode(ST_Point(-71.27593,42.33891));  -- I 90 Exit 14
SELECT pprint_addy(addy[1]), addy FROM reverse_geocode(ST_Point(-71.85335,42.19262));  -- I 90 Exit 10, Worcester MA
SELECT pprint_addy(addy[1]), addy FROM reverse_geocode(ST_Point(-71.057811,42.358274)); -- 1 Devonshire Place (washington st area)
SELECT pprint_addy(addy[1]), addy FROM reverse_geocode(ST_Point(-71.123848,42.41115)); --30 capen
\timing