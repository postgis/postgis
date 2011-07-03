--$Id$
\timing
-- Limit 1
SELECT pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('529 Main Street, Boston, MA 02129',1);
SELECT pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('75 State Street, Boston, MA 02109',1);
SELECT pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('100 Federal Street, Boston, MA 02109',1);
-- default
SELECT pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('529 Main Street, Boston, MA 02129');
SELECT pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('75 State Street, Boston, MA 02109');
SELECT pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('100 Federal Street, Boston,MA 02109');

-- 20
SELECT pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('529 Main Street, Boston, MA 02129',20);
SELECT pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('75 State Street, Boston, MA 02109',20);
SELECT pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('100 Federal Street, Boston, MA 02109',20);

-- Limit 1 - Test caching effects
SELECT pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('530 Main Street, Boston MA, 02129',1);
SELECT pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('76 State Street, Boston MA, 02109',1);
SELECT pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('101 Federal Street, Boston MA, 02109',20);

-- Partial address
SELECT pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('101 Federal Street, Boston MA',20);
SELECT pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('101 Federal Street, Boston MA',1);

--Test misspellings and missing zip --
SELECT pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('101 Fedaral Street, Boston, MA',1);
SELECT pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('101 Fedaral Street, Boston, MA',50);

--Geocoding mangled zipcodes
SELECT '#1073' As ticket, pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('212 3rd Ave N, MINNEAPOLIS, MN 553404',5);
SELECT '#1073' As ticket, pprint_addy(addy) As address, ST_AsText(geomout) As pt, rating FROM geocode('212 3rd Ave N, MINNEAPOLIS, MN 55340-',5);
\timing
