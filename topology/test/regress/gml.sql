set client_min_messages to WARNING;

\i ../load_topology-4326.sql
\i ../load_features.sql
\i ../more_features.sql

--- Puntual single element {

-- Output simple puntual features (composed by single topo-element)
SELECT feature_name||'-vanilla', topology.AsGML(feature)
 FROM features.traffic_signs
 WHERE feature_name IN ('S1', 'S2', 'S3', 'S4' )
 ORDER BY feature_name;

-- Output again but with no prefix
SELECT feature_name||'-noprefix', topology.AsGML(feature, '')
 FROM features.traffic_signs
 WHERE feature_name IN ('S1', 'S2', 'S3', 'S4' )
 ORDER BY feature_name;

-- Output again with custom prefix
SELECT feature_name||'-customprefix', topology.AsGML(feature, 'cstm')
 FROM features.traffic_signs
 WHERE feature_name IN ('S1', 'S2', 'S3', 'S4' )
 ORDER BY feature_name;

-- Again with no prefix, no srsDimension (opt+=2)
-- and swapped lat/lon (opt+=16) and short CRS
SELECT feature_name||'-latlon', topology.AsGML(feature, '', 15, 18)
 FROM features.traffic_signs
 WHERE feature_name IN ('S4');

--- } Puntual single-element

--- Puntual multi element {

SELECT feature_name||'-noprefix', topology.AsGML(feature, '')
 FROM features.traffic_signs
 WHERE feature_name IN ('N1N2N3');

--- } Puntual multi-element

--- Lineal single element {

-- Output simple lineal features (composed by single topo element)
SELECT feature_name||'-vanilla', topology.AsGML(feature)
 FROM features.city_streets
 WHERE feature_name IN ('R3', 'R4' )
 ORDER BY feature_name;

-- Output again but with no prefix
SELECT feature_name||'-noprefix', topology.AsGML(feature, '')
 FROM features.city_streets
 WHERE feature_name IN ('R3', 'R4' )
 ORDER BY feature_name;

-- Output again with custom prefix
SELECT feature_name||'-customprefix', topology.AsGML(feature, 'cstm')
 FROM features.city_streets
 WHERE feature_name IN ('R3', 'R4' )
 ORDER BY feature_name;

--- } Lineal single-element

--- Lineal multi-element  {

-- Output simple lineal features (composed by single topo element)
SELECT feature_name||'-vanilla', topology.AsGML(feature)
 FROM features.city_streets
 WHERE feature_name IN ('R1', 'R2' )
 ORDER BY feature_name;

-- Output again but with no prefix
SELECT feature_name||'-noprefix', topology.AsGML(feature, '')
 FROM features.city_streets
 WHERE feature_name IN ('R1', 'R2' )
 ORDER BY feature_name;

-- Output again with custom prefix
SELECT feature_name||'-customprefix', topology.AsGML(feature, 'cstm')
 FROM features.city_streets
 WHERE feature_name IN ('R1', 'R2' )
 ORDER BY feature_name;

--- } Lineal multi-element

--- Areal single-element {

-- Output simple lineal features (composed by single topo element)
SELECT feature_name||'-vanilla', topology.AsGML(feature)
 FROM features.land_parcels
 WHERE feature_name IN ('P4', 'P5' )
 ORDER BY feature_name;

-- Output again but with no prefix
SELECT feature_name||'-noprefix', topology.AsGML(feature, '')
 FROM features.land_parcels WHERE feature_name IN ('P4', 'P5')
 ORDER BY feature_name;

-- Output again with custom prefix
SELECT feature_name||'-customprefix', topology.AsGML(feature, 'cstm')
 FROM features.land_parcels WHERE feature_name IN ('P4', 'P5')
 ORDER BY feature_name;

--- } Areal single-element

--- Areal multi-element {

-- Output simple lineal features (composed by single topo element)
SELECT feature_name||'-vanilla', topology.AsGML(feature)
 FROM features.land_parcels
 WHERE feature_name IN ('P1', 'P2', 'P3' )
 ORDER BY feature_name;

-- Output again but with no prefix
SELECT feature_name||'-noprefix', topology.AsGML(feature, '')
 FROM features.land_parcels
 WHERE feature_name IN ('P1', 'P2', 'P3' )
 ORDER BY feature_name;

-- Output again with custom prefix
SELECT feature_name||'-customprefix', topology.AsGML(feature, 'cstm')
 FROM features.land_parcels
 WHERE feature_name IN ('P1', 'P2', 'P3' )
 ORDER BY feature_name;

--- } Areal multi-element

--- { Visited table bookkeeping

CREATE TABLE visited (element_type int, element_id int);

-- R2 visits E4,E5
--           N5,N6,N7
SELECT feature_name||'-visited', topology.AsGML(feature,
       '', 15, 2, 'visited'::regclass) FROM features.city_streets
       WHERE feature_name IN ('R2')
       ORDER BY feature_name;

-- S1 visits N14
-- S3 visits (N6)
SELECT feature_name||'-visited', topology.AsGML(feature,
       '', 15, 2, 'visited'::regclass) FROM features.traffic_signs
       WHERE feature_name IN ('S1', 'S3')
       ORDER BY feature_name;

-- R1 visits E9,E10,
--           N13,(N14),N15
SELECT feature_name||'-visited', topology.AsGML(feature,
       '', 15, 2, 'visited'::regclass) FROM features.city_streets
       WHERE feature_name IN ('R1');

-- N1N6N14 visits N1,(N6),(N14)
SELECT feature_name||'-visited', topology.AsGML(feature,
       '', 15, 2, 'visited'::regclass) FROM features.traffic_signs
       WHERE feature_name IN ('N1N6N14')
       ORDER BY feature_name;

-- P1 visits F3,F6
--           F3-> E6,E19,(E9),E21
--           F6-> (E9),E20,E12,E22
--           E6-> N16,N17
--           E19-> (N14),(N17) # N14 visited by S1-visited, N17 by E6 above
--           E21-> (N15),(N16) # N15 visited by R1-visited, N16 by E6 above
--           E20-> N9,(N14) # N14 visited by S1-visited
--           E12-> N8,(N9) # N9 visited by E20 above
--           E22-> (N8),(N15) # N8 visited above, N15 by R1-visited
--
-- P2 visits F4,F7
--           F4-> E7,E17,(E10),(E19) # E10 by R1-visited, E19 by P1-visited
--           F7-> (E10),E18,E13,(E20) # E10 by R1-visited, E20 by P1-visited
--           E7-> (N17),N18 # N17 visited by P1-visited.E6
--           E17-> (N13),(N18) # N13 by R1-visited, N18 by E7 above
--           E18-> N10,(N13) # N13 by R1-visited
--           E13-> (N9),(N10) # N9 visited by P1-visited.E20, N10 above
--
SELECT feature_name||'-visited', topology.AsGML(feature,
       '', 15, 2, 'visited'::regclass) FROM features.land_parcels
        WHERE feature_name IN ('P1', 'P2')
        ORDER BY feature_name;

-- F3F4 visits (F3),(F4)
SELECT feature_name||'-visited', topology.AsGML(feature,
       '', 15, 2, 'visited'::regclass) FROM features.land_parcels
       WHERE feature_name IN ('F3F4')
       ORDER BY feature_name DESC;

-- E7E8 visits: (E7),E8
--              (N17),(N18),N19
SELECT feature_name||'-visited', topology.AsGML(feature,
       '', 15, 2, 'visited'::regclass) FROM features.city_streets
       WHERE feature_name IN ('E7E8');

-- Test custom identifier prefix
-- P3 visits (E18),(E17),(E8),E15,E16,E14
--           (N10),(N13),(N18),N19,N12,N11
SELECT feature_name||'-visited-idprefix', topology.AsGML(feature,
       '', 15, 2, 'visited'::regclass, 'cd-') FROM features.land_parcels
       WHERE feature_name IN ('P3');

--- } Visited table bookkeeping

--- { GML2 output

-- Output in GML2
SELECT feature_name||'-gml2' as name, topology.AsGML(feature,'',0,2,NULL,'',2)
 FROM features.city_streets
 WHERE feature_name IN ('R1', 'R2', 'R3', 'R4' )
UNION
SELECT feature_name||'-gml2', topology.AsGML(feature,'',0,2,NULL,'',2)
 FROM features.traffic_signs
 WHERE feature_name IN ('S1', 'S2', 'S3', 'S4' )
ORDER BY name;

--- } GML2 output

SELECT topology.DropTopology('city_data');
DROP SCHEMA features CASCADE;
DROP TABLE visited;
