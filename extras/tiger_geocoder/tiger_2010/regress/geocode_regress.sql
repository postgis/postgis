--$Id$
\timing
-- Limit 1
SELECT 'T1', pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM geocode('529 Main Street, Boston, MA 02129',1);
SELECT 'T2', pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM geocode('75 State Street, Boston, MA 02109',1);
SELECT 'T3', pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM geocode('100 Federal Street, Boston, MA 02109',1);
-- default
SELECT 'T4', pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM geocode('529 Main Street, Boston, MA 02129');
SELECT 'T5', pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM geocode('75 State Street, Boston, MA 02109');
SELECT 'T6', pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM geocode('100 Federal Street, Boston,MA 02109');

-- 20
SELECT 'T7', pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM geocode('529 Main Street, Boston, MA 02129',20);
SELECT 'T8', pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM geocode('75 State Street, Boston, MA 02109',20);
SELECT 'T9', pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM geocode('100 Federal Street, Boston, MA 02109',20);

-- Limit 1 - Test caching effects
SELECT 'T10', pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM geocode('530 Main Street, Boston MA, 02129',1);
SELECT 'T11', pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM geocode('76 State Street, Boston MA, 02109',1);
SELECT 'T12', pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM geocode('101 Federal Street, Boston, MA',20);

-- Test batch geocoding along a street
SELECT '#TB1' As ticket, pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT geocode(target::text,1) As g, target FROM (VALUES ('24 School Street, Boston, MA 02108'), ('20 School Street, Boston, MA 02109')) As f(target) ) As foo;

-- Partial address
SELECT 'T13', pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM geocode('101 Federal Street, Boston MA',20);
SELECT 'T14', pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM geocode('101 Federal Street, Boston MA',1);

--Test misspellings and missing zip --
SELECT 'T15', pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM geocode('101 Fedaral Street, Boston, MA',1);
SELECT 'T16', pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM geocode('101 Fedaral Street, Boston, MA',50);

-- Ratings wrong for missing or wrong local zips
SELECT '#1087a' As ticket, pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM geocode('75 State Street, Boston, MA 02110',3);
SELECT '#1087b' As ticket, pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM geocode('75 State Street, Boston, MA',3);
--right zip
SELECT '#1087c' As ticket, pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM geocode('75 State Street, Boston, MA 02109',1);

--Geocoding mangled zipcodes
SELECT '#1073a' As ticket, pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT geocode(target,2) As g, target FROM (SELECT '212 3rd Ave N, MINNEAPOLIS, MN 553404'::text As target) AS f)  As foo;
SELECT '#1073b' As ticket, pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM geocode('212 3rd Ave N, MINNEAPOLIS, MN 55401-',2);

-- country roads and highways with spaces in street type
SELECT '#1076a' As ticket, pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT geocode(target) As g, target FROM (SELECT '16725 Co Rd 24, Plymouth, MN 55447'::text As target) As f) As foo;  
SELECT '#1076b' As ticket, pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT geocode(target,2) As g, target FROM (SELECT '16725 County Road 24, Plymouth, MN 55447'::text As target) As f) As foo;
SELECT '#1076c' As ticket, pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT geocode(target,1) As g, target FROM (SELECT '13800 County Hwy 9, Andover, MN 55304'::text As target) AS f) As foo; 
SELECT '#1076d' As ticket, pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT geocode(target,1) As g, target FROM (SELECT '13800 9, Andover, MN 55304'::text As target) AS f) As foo; 
SELECT '#1076e' As ticket, pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT geocode(target,1) As g, target FROM (SELECT '3900 Route 6, Eastham, Massachusetts 02642'::text As target) AS f) As foo; 

\timing
