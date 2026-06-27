\timing
-- Limit 1
SELECT 'T1', tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('529 Main Street, Boston, MA 02129',1);
SELECT 'T2', tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('75 State Street, Boston, MA 02109',1);
SELECT 'T3', tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('100 Federal Street, Boston, MA 02109',1);
-- default
SELECT 'T4', tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('529 Main Street, Boston, MA 02129');
SELECT 'T5', tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('75 State Street, Boston, MA 02109');
SELECT 'T6', tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('100 Federal Street, Boston,MA 02109');

-- 20
SELECT 'T7', tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('529 Main Street, Boston, MA 02129',20);
SELECT 'T8', tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('75 State Street, Boston, MA 02109',20);
SELECT 'T9', tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('100 Federal Street, Boston, MA 02109',20);

-- Limit 1 - Test caching effects
SELECT 'T10', tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('530 Main Street, Boston MA, 02129',1);
SELECT 'T11', tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('76 State Street, Boston MA, 02109',1);
SELECT 'T12', tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('101 Federal Street, Boston, MA',20);

-- Test batch geocoding along a street
SELECT '#TB1' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target::text,1) As g, target FROM (VALUES ('24 School Street, Boston, MA 02108'), ('20 School Street, Boston, MA 02109')) As f(target) ) As foo;

-- Partial address
SELECT 'T13', tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('101 Federal Street, Boston MA',20);
SELECT 'T14', tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('101 Federal Street, Boston MA',1);

--Test misspellings and missing zip --
SELECT 'T15', tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('101 Fedaral Street, Boston, MA',1);
SELECT 'T16', tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('101 Fedaral Street, Boston, MA',50);

-- needs addr these ones have more than 2 sides
-- my alma mater doesn't geocode right without addr check  --
SELECT 'T17', tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('77 Massachusetts Avenue, Cambridge, MA 02139',1);

-- zip provided but no state - should still be fast under 250ms
SELECT 'T18a', tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('26 Court Street, 02109',1);
SELECT 'T18b', tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('26 Court Street,Boston,02109',1);

-- Ratings wrong for missing or wrong local zips
SELECT '#1087a' As ticket, tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('75 State Street, Boston, MA 02110',3);
SELECT '#1087b' As ticket, tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('75 State Street, Boston, MA',3);
--right zip
SELECT '#1087c' As ticket, tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('75 State Street, Boston, MA 02109',1);

--Geocoding mangled zipcodes
SELECT '#1073a' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,2) As g, target FROM (SELECT '212 3rd Ave N, MINNEAPOLIS, MN 553404'::text As target) AS f)  As foo;
SELECT '#1073b' As ticket, tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('212 3rd Ave N, MINNEAPOLIS, MN 55401-',2);

-- country roads and highways with spaces in street type
SELECT '#1076a' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target) As g, target FROM (SELECT '16725 Co Rd 24, Plymouth, MN 55447'::text As target) As f) As foo;
SELECT '#1076b' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,1) As g, target FROM (SELECT '16725 County Road 24, Plymouth, MN 55447'::text As target) As f) As foo;
SELECT '#1076c' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,1) As g, target FROM (SELECT '13800 County Hwy 9, Andover, MN 55304'::text As target) AS f) As foo;
SELECT '#1076d' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,1) As g, target FROM (SELECT '13800 9, Andover, MN 55304'::text As target) AS f) As foo;
SELECT '#1076e' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,4) As g, target FROM (SELECT '3900 Route 6, Eastham, Massachusetts 02642'::text As target) AS f) As foo;

-- country road that starts with a letter
SELECT '#1076f' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,3) As g, target FROM (SELECT '1940 County Road C W, Roseville, MN 55113'::text As target) AS f) As foo;

-- ad road that in some sections no street range recorded --
SELECT '#1076g' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target) As g, target FROM (SELECT '15709 Rockford Road, Plymouth, MN 55447'::text As target) As f) AS foo;

-- testing RT common abbreviation for route, ensure asking for 1 gives most probable  --
SELECT '#1076h' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,3) As g, target FROM (SELECT '300 Rt 3A, Hingham, MA'::text As target) As f) As foo;

-- alternate spellings
SELECT '#1074a' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target) As g, target FROM (SELECT '8525 COTTAGE WOOD TERR, Blaine, MN 55434'::text As target) As f) AS foo;
SELECT '#1074b' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target) As g, target FROM (SELECT '8525 COTTAGEWOOD TERR, Blaine, MN 55434'::text As target) As f) AS foo;

-- testing region --
SELECT '#1070a' As ticket, tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('100 Federal Street, Boston, MA 02109',3, (SELECT ST_Union(the_geom) FROM tiger.place WHERE statefp = '25' AND name = 'Lynn')::geometry);
SELECT '#1070b' As ticket, tiger.pprint_addy(addy) As address, ST_AsText(ST_SnapToGrid(geomout,0.00001)) As pt, rating FROM tiger.geocode('100 Federal Street, MA',3, (SELECT ST_Union(the_geom) FROM tiger.place WHERE statefp = '25' AND name = 'Lynn')::geometry);

-- service roads and interstates
SELECT '#1112a' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,2) As g, target FROM (SELECT '8401 W 35W Service Dr NE, Blaine, MN 55449'::text As target) As f) As foo;
SELECT '#1112b' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,1) As g, target FROM (SELECT '8401 35W, Blaine, MN 55449'::text As target) As f) As foo;
SELECT '#1112c' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,1) As g, target FROM (SELECT '8401 35W West, Blaine, MN 55449'::text As target) As f) As foo;
SELECT '#1112d' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,1) As g, target FROM (SELECT '8401 West 35W, Blaine, MN 55449'::text As target) As f) As foo;
SELECT '#1112e' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,1) As g, target FROM (SELECT '8401 W 35W, Blaine, MN 55449'::text As target) As f) As foo;

-- working with prequalabrv such as Old .. something or other
SELECT '#1113a' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,2) As g, target FROM (SELECT '8040 OLD CEDAR AVE S, BLOOMINGTON, MN 55425'::text As target) As f) As foo;
SELECT '#1113b' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,2) As g, target FROM (SELECT '8040 CEDAR AVE S, BLOOMINGTON, MN 55425'::text As target) As f) As foo;
SELECT '#1113c' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,2) As g, target FROM (SELECT '17405 Old Rockford Rd, Plymouth, MN 55446'::text As target) As f) As foo;
SELECT '#1113d' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,2) As g, target FROM (SELECT '17405 Rockford Rd, Plymouth, MN 55446'::text As target) As f) As foo;
SELECT '#1113e' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,2) As g, target FROM (SELECT '198 OLD CONSTANCE BLVD, ANDOVER, MN 55304'::text As target) As f) As foo;
SELECT '#1113f' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,2) As g, target FROM (SELECT '198 CONSTANCE BLVD, ANDOVER, MN 55304'::text As target) As f) As foo;

-- #1145 addresses used to be slow to geocode took minutes
SELECT '#1145a' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,2) As g, target FROM (SELECT '4051 27th Ave S Minneapolis MN 55405'::text As target) As f) As foo;
SELECT '#1145b' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,2) As g, target FROM (SELECT '3625 18th Ave S Minneapolis MN 55406'::text As target) As f) As foo;
SELECT '#1145c' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,2) As g, target FROM (SELECT '4057 10th Ave S Minneapolis MN 55406'::text As target) As f) As foo;
SELECT '#1145d' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target,2) As g, target FROM (SELECT '8512 141 St Ct Apple Valley MN 55124'::text As target) As f) As foo;
SELECT '#1145e' As ticket, tiger.pprint_addy((g).addy) As address, target, ST_AsText(ST_SnapToGrid((g).geomout, 0.00001)) As pt, (g).rating FROM (SELECT tiger.geocode(target) As g, target FROM (SELECT '103 36th St W Minneapolis MN 55409'::text As target) As f) As foo;

-- cross street intersection
SELECT '#1333a' AS ticket, tiger.pprint_addy(addy), st_astext(geomout),rating FROM geocode_intersection('Weld', 'School', 'MA', 'Boston');
SELECT '#1333b' AS ticket, tiger.pprint_addy(addy), st_astext(geomout),rating FROM geocode_intersection('Haverford St','Germania St', 'MA', 'Boston', '02130',1);

-- crossing highways fails -- zip check
SELECT '#1392a' AS ticket, tiger.pprint_addy(addy), st_astext(geomout),rating FROM geocode_intersection('State Hwy 121', 'N Denton Tap Rd', 'TX', 'Coppell', '', 2);
SELECT '#1392b' AS ticket, tiger.pprint_addy(addy), st_astext(geomout),rating FROM geocode_intersection('State Hwy 121', 'N Denton Tap Rd', 'TX','', '', 2);
--

-- Geocode 1 not returning best answer
SELECT '#2899' AS ticket, st_astext(ST_SnapToGrid(geomout,0.0001)),rating FROM tiger.geocode('22 Minnow Ln, Westbrook, CT 06498',1);
\timing
