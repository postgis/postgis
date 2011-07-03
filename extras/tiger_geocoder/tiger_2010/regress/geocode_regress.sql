--$Id$
\timing
-- Limit 1
SELECT 'T1', pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('529 Main Street, Boston, MA 02129',1);
SELECT 'T2', pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('75 State Street, Boston, MA 02109',1);
SELECT 'T3', pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('100 Federal Street, Boston, MA 02109',1);
-- default
SELECT 'T4', pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('529 Main Street, Boston, MA 02129');
SELECT 'T5', pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('75 State Street, Boston, MA 02109');
SELECT 'T6', pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('100 Federal Street, Boston,MA 02109');

-- 20
SELECT 'T7', pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('529 Main Street, Boston, MA 02129',20);
SELECT 'T8', pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('75 State Street, Boston, MA 02109',20);
SELECT 'T9', pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('100 Federal Street, Boston, MA 02109',20);

-- Limit 1 - Test caching effects
SELECT 'T10', pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('530 Main Street, Boston MA, 02129',1);
SELECT 'T11', pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('76 State Street, Boston MA, 02109',1);
SELECT 'T12', pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('101 Federal Street, Boston, MA',20);

-- Test batch geocoding along a street
SELECT '#TB1' As ticket, pprint_addy((g).addy) As address, target, ST_AsText((g).geomout) As pt, (g).rating FROM (SELECT geocode(target::text,1) As g, target FROM (VALUES ('24 School Street, Boston, MA 02108'), ('20 School Street, Boston, MA 02109')) As f(target) ) As foo;

-- Partial address
SELECT 'T13', pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('101 Federal Street, Boston MA',20);
SELECT 'T14', pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('101 Federal Street, Boston MA',1);

--Test misspellings and missing zip --
SELECT 'T15', pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('101 Fedaral Street, Boston, MA',1);
SELECT 'T16', pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('101 Fedaral Street, Boston, MA',50);

-- Ratings wrong for missing or wrong local zips
SELECT '#1087a' As ticket, pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('75 State Street, Boston, MA 02110',3);
SELECT '#1087b' As ticket, pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('75 State Street, Boston, MA',3);
--right zip
SELECT '#1087c' As ticket, pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('75 State Street, Boston, MA 02109',1);

--Geocoding mangled zipcodes
SELECT '#1073a' As ticket, pprint_addy((g).addy) As address, target, ST_AsText((g).geomout) As pt, (g).rating FROM (SELECT geocode(target,2) As g, target FROM (SELECT '212 3rd Ave N, MINNEAPOLIS, MN 553404'::text As target) As f) As foo;
SELECT '#1073b' As ticket, pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('212 3rd Ave N, MINNEAPOLIS, MN 55401-',2);
\timing
