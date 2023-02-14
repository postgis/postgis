--
-- AsMARC21 regression test
-- Copyright (C) 2021 University of Münster (WWU), Germany
-- Written by Jim Jones <jim.jones@uni-muenster.de>

CREATE TABLE asmarc21_rt (gid int, geom geometry);
INSERT INTO asmarc21_rt (gid,geom) VALUES
	(1,'SRID=4326;GEOMETRYCOLLECTION(
					POINT(-4.817633628845215 54.05518110962899),
					POINT(-4.827804565429687 54.05374516606875),
					POINT(-4.828877449035644 54.053190928963545),POINT(-4.828662872314452 54.05228397957032))'),
	-- geometry collection with polygons
	(2,'SRID=4326;GEOMETRYCOLLECTION(
					POLYGON((-4.6087646484375 54.12945489870697,-4.610137939453125 54.15760940347411,-4.54833984375 54.138305520107,-4.582672119140625 54.110137887495455,-4.6087646484375 54.12945489870697)),
					POLYGON((-4.590911865234375 54.168865845963495,-4.654083251953125 54.17047365921377,-4.626617431640625 54.19860027702373,-4.632110595703125 54.17851178818134,-4.590911865234375 54.168865845963495)),
					POLYGON((-4.555206298828125 54.18253026689359,-4.557952880859375 54.149567212540546,-4.514007568359375 54.14876290750897,-4.555206298828125 54.18253026689359)))'),
	-- geometry collection with linestrings
	(3,'SRID=4326;GEOMETRYCOLLECTION(
					LINESTRING(-4.68841552734375 54.107722628012425,-4.658203125000001 54.116577891623876,-4.621124267578125 54.07469997904573),
					LINESTRING(-4.599151611328125 54.10933281663398,-4.665069580078125 54.13186889208541,-4.6142578125 54.12945489870697,-4.676055908203125 54.150371501946495),
					LINESTRING(-4.568939208984375 54.147154250568605,-4.4879150390625 54.15519691025378))'),
	-- geometry collection with points, polygons and linestrings
	(4,'SRID=4326;GEOMETRYCOLLECTION(
					POLYGON((-4.617730198522098 54.18456893527372,-4.593268452306278 54.18456893527372,-4.593268452306278 54.1668855796329,-4.617730198522098 54.1668855796329,-4.617730198522098 54.18456893527372)),
					POLYGON((-4.625025807042606 54.16839297853637,-4.6160994154410435 54.16839297853637,-4.6160994154410435 54.16517712781367,-4.625025807042606 54.16517712781367,-4.625025807042606 54.16839297853637)),
					POINT(-4.622064648290165 54.173115806123455),
					LINESTRING(-4.674682617187499 54.110137887495455,-4.59228515625 54.159217654166895),
					LINESTRING(-4.729614257812501 54.1012812481911,-4.58953857421875 54.10047600536083,-4.726867675781251 54.11577294581963),
				  TRIANGLE((-4.5792388916015625 54.18172660239091,-4.56756591796875 54.196993557130355,-4.546623229980469 54.18313300502024,-4.5792388916015625 54.18172660239091)))'),
	(5,'SRID=4326;MULTIPOINT(-4.817633628845215 54.05518110962899,-4.827804565429687 54.05374516606875,-4.828877449035644 54.053190928963545,-4.828662872314452 54.05228397957032)'),
	(6,'SRID=4326;MULTIPOLYGON(((-4.625025807042606 54.16839297853636,-4.616099415441044 54.16839297853636,-4.616099415441044 54.16517712781365,-4.625025807042606 54.16517712781365,-4.625025807042606 54.16839297853636)),((-4.526749668736942 54.28708732448666,-4.48761087479163 54.28708732448666,-4.48761087479163 54.271051739781996,-4.526749668736942 54.271051739781996,-4.526749668736942 54.28708732448666)),((-4.590607700963504 54.23615279107321,-4.539109287877567 54.23615279107321,-4.539109287877567 54.21367347522684,-4.590607700963504 54.21367347522684,-4.590607700963504 54.23615279107321)))'),
	(7,'SRID=4326;MULTILINESTRING((-4.68841552734375 54.107722628012425,-4.658203125000001 54.116577891623876,-4.621124267578125 54.07469997904573),(-4.599151611328125 54.10933281663398,-4.665069580078125 54.13186889208541,-4.6142578125 54.12945489870697,-4.676055908203125 54.150371501946495),(-4.568939208984375 54.147154250568605,-4.4879150390625 54.15519691025378))'),
	(8,'SRID=4326;TIN(((-4.549713134765625 54.14735533610431,-4.5394134521484375 54.161428896826635,-4.5270538330078125 54.14634989865954,-4.549713134765625 54.14735533610431)),((-4.5792388916015625 54.18172660239091,-4.56756591796875 54.196993557130355,-4.546623229980469 54.18313300502024,-4.5792388916015625 54.18172660239091)))'),
	(9,'SRID=4326;POLYGON((-4.617730198522098 54.18456893527372,-4.593268452306278 54.18456893527372,-4.593268452306278 54.1668855796329,-4.617730198522098 54.1668855796329,-4.617730198522098 54.18456893527372))'),
	(10,'SRID=4326;POINT(-4.565591812133789 54.18476537665953)'),
	(11,'SRID=4326;LINESTRING(-4.626617431640625 54.0819511048804,-4.484481811523437 54.15519691025378,-4.6966552734375 54.22349644815273)'),
	(12,'SRID=4326;POLYGON((-4.625025807042606 54.16839297853637,-4.6160994154410435 54.16839297853637,-4.6160994154410435 54.16517712781367,-4.625025807042606 54.16517712781367,-4.625025807042606 54.16839297853637))'),
	(13,'SRID=4326;TRIANGLE((-4.5792388916015625 54.18172660239091,-4.56756591796875 54.196993557130355,-4.546623229980469 54.18313300502024,-4.5792388916015625 54.18172660239091))'),
	-- 3D geometries (Z)
	(14,'SRID=4326;GEOMETRYCOLLECTIONZ(
					 POINT(-4.817633628845215 54.05518110962899 42),
					 POINT(-4.827804565429687 54.05374516606875 42),
					 POINT(-4.828877449035644 54.053190928963545 42),
					 POINT(-4.828662872314452 54.05228397957032 42))'),
	(15,'SRID=4326;MULTIPOINTZ(-4.817633628845215 54.05518110962899 42,-4.827804565429687 54.05374516606875 42,-4.828877449035644 54.053190928963545 42,-4.828662872314452 54.05228397957032 42)'),
	(16,'SRID=4326;MULTIPOLYGONZ(((-4.625025807042606 54.16839297853636 42,-4.616099415441044 54.16839297853636 42,-4.616099415441044 54.16517712781365 42,-4.625025807042606 54.16517712781365 42,-4.625025807042606 54.16839297853636 42)),((-4.526749668736942 54.28708732448666 42,-4.48761087479163 54.28708732448666 42,-4.48761087479163 54.271051739781996 42,-4.526749668736942 54.271051739781996 42,-4.526749668736942 54.28708732448666 42)),((-4.590607700963504 54.23615279107321 42,-4.539109287877567 54.23615279107321 42,-4.539109287877567 54.21367347522684 42,-4.590607700963504 54.21367347522684 42,-4.590607700963504 54.23615279107321 42)))'),
	(17,'SRID=4326;POINTZ(-4.565591812133789 54.18476537665953 42)'),
	(18,'SRID=4326;POLYGONZ((-4.617730198522098 54.18456893527372 42,-4.593268452306278 54.18456893527372 42,-4.593268452306278 54.1668855796329 42,-4.617730198522098 54.1668855796329 42,-4.617730198522098 54.18456893527372 42))'),
	(19,'SRID=4326;LINESTRINGZ(-4.626617431640625 54.0819511048804 42,-4.484481811523437 54.15519691025378 42,-4.6966552734375 54.22349644815273 42)'),
	(20,'SRID=4326;MULTILINESTRINGZ((-4.68841552734375 54.107722628012425 42,-4.658203125000001 54.116577891623876 42,-4.621124267578125 54.07469997904573 42),(-4.599151611328125 54.10933281663398 42,-4.665069580078125 54.13186889208541 42,-4.6142578125 54.12945489870697 42,-4.676055908203125 54.150371501946495 42),(-4.568939208984375 54.147154250568605 42,-4.4879150390625 54.15519691025378 42))'),
	(21,'SRID=4326;TRIANGLEZ((-4.5792388916015625 54.18172660239091 42,-4.56756591796875 54.196993557130355 42,-4.546623229980469 54.18313300502024 42,-4.5792388916015625 54.18172660239091 42))'),
	(22,'SRID=4326;TINZ(((-4.549713134765625 54.14735533610431 42,-4.5394134521484375 54.161428896826635 42,-4.5270538330078125 54.14634989865954 42,-4.549713134765625 54.14735533610431 42)),((-4.5792388916015625 54.18172660239091 42,-4.56756591796875 54.196993557130355 42,-4.546623229980469 54.18313300502024 42,-4.5792388916015625 54.18172660239091 42)))'),
	-- 3D geometries (M)
	(23,'SRID=4326;MULTIPOINTM(-4.817633628845215 54.05518110962899 42,-4.827804565429687 54.05374516606875 42,-4.828877449035644 54.053190928963545 42,-4.828662872314452 54.05228397957032 42)'),
	(24,'SRID=4326;MULTIPOLYGONM(((-4.625025807042606 54.16839297853636 42,-4.616099415441044 54.16839297853636 42,-4.616099415441044 54.16517712781365 42,-4.625025807042606 54.16517712781365 42,-4.625025807042606 54.16839297853636 42)),((-4.526749668736942 54.28708732448666 42,-4.48761087479163 54.28708732448666 42,-4.48761087479163 54.271051739781996 42,-4.526749668736942 54.271051739781996 42,-4.526749668736942 54.28708732448666 42)),((-4.590607700963504 54.23615279107321 42,-4.539109287877567 54.23615279107321 42,-4.539109287877567 54.21367347522684 42,-4.590607700963504 54.21367347522684 42,-4.590607700963504 54.23615279107321 42)))'),
	(25,'SRID=4326;MULTILINESTRINGM((-4.68841552734375 54.107722628012425 42,-4.658203125000001 54.116577891623876 42,-4.621124267578125 54.07469997904573 42),(-4.599151611328125 54.10933281663398 42,-4.665069580078125 54.13186889208541 42,-4.6142578125 54.12945489870697 42,-4.676055908203125 54.150371501946495 42),(-4.568939208984375 54.147154250568605 42,-4.4879150390625 54.15519691025378 42))'),
	(26,'SRID=4326;POLYGONM((-4.617730198522098 54.18456893527372 73,-4.593268452306278 54.18456893527372 73,-4.593268452306278 54.1668855796329 73,-4.617730198522098 54.1668855796329 73,-4.617730198522098 54.18456893527372 73))'),
	(27,'SRID=4326;LINESTRINGM(-4.626617431640625 54.0819511048804 73,-4.484481811523437 54.15519691025378 73,-4.6966552734375 54.22349644815273 73)'),
	(28,'SRID=4326;POINTM(-4.565591812133789 54.18476537665953 73)'),
	(29,'SRID=4326;TRIANGLEM((-4.5792388916015625 54.18172660239091 73,-4.56756591796875 54.196993557130355 73,-4.546623229980469 54.18313300502024 73,-4.5792388916015625 54.18172660239091 73))'),
	(30,'SRID=4326;TINM(((-4.549713134765625 54.14735533610431 42,-4.5394134521484375 54.161428896826635 42,-4.5270538330078125 54.14634989865954 42,-4.549713134765625 54.14735533610431 42)),((-4.5792388916015625 54.18172660239091 42,-4.56756591796875 54.196993557130355 42,-4.546623229980469 54.18313300502024 42,-4.5792388916015625 54.18172660239091 42)))'),
	-- 4D geometries
	(31,'SRID=4326;GEOMETRYCOLLECTIONZM(
					 POINT(-4.817633628845215 54.05518110962899 42 73),
					 POINT(-4.827804565429687 54.05374516606875 42 73),
					 POINT(-4.828877449035644 54.053190928963545 42 73),
					 POINT(-4.828662872314452 54.05228397957032 42 73))'),
	(32,'SRID=4326;TINZM(((-4.549713134765625 54.14735533610431 42 73,-4.5394134521484375 54.161428896826635 42 73,-4.5270538330078125 54.14634989865954 42 73,-4.549713134765625 54.14735533610431 42 73)),((-4.5792388916015625 54.18172660239091 42 73,-4.56756591796875 54.196993557130355 42 73,-4.546623229980469 54.18313300502024 42 73,-4.5792388916015625 54.18172660239091 42 73)))'),
	(34,'SRID=4326;POINTZM(-4.565591812133789 54.18476537665953 42 73)'),
	(35,'SRID=4326;POLYGONZM((-4.617730198522098 54.18456893527372 42 73,-4.593268452306278 54.18456893527372 42 73,-4.593268452306278 54.1668855796329 42 73,-4.617730198522098 54.1668855796329 42 73,-4.617730198522098 54.18456893527372 42 73))'),
	(36,'SRID=4326;LINESTRINGZM(-4.626617431640625 54.0819511048804 42 73,-4.484481811523437 54.15519691025378 42 73,-4.6966552734375 54.22349644815273 42 73)');

UPDATE asmarc21_rt SET geom = ST_AsEWKT(geom,5);


-- Converts to geometries to MARC21/XML
SELECT gid,ST_AsMARC21(geom,'hdddmmss') AS marc21 FROM asmarc21_rt ORDER BY gid;

-- Converts to geometries to MARC21/XML
SELECT 'hdddmmss',gid,ST_AsText(ST_GeomFromMARC21(ST_AsMARC21(geom,'hdddmmss')),5) AS marc21 FROM asmarc21_rt ORDER BY gid;
SELECT 'hddd.dddddd',gid,ST_AsText(ST_GeomFromMARC21(ST_AsMARC21(geom,'hddd.dddddd')),5) AS marc21 FROM asmarc21_rt ORDER BY gid;
SELECT 'hdddmm.mmmmmm',gid,ST_AsText(ST_GeomFromMARC21(ST_AsMARC21(geom,'hdddmm.mmmmmm')),5) AS marc21 FROM asmarc21_rt ORDER BY gid;
SELECT 'hdddmmss.ssssss',gid,ST_AsText(ST_GeomFromMARC21(ST_AsMARC21(geom,'hdddmmss.ssssss')),5) AS marc21 FROM asmarc21_rt ORDER BY gid;

SELECT 'hdddmmss',gid,ST_AsText(ST_GeomFromMARC21(ST_AsMARC21(geom,'hdddmmss')),5) AS marc21 FROM asmarc21_rt ORDER BY gid;
SELECT 'hddd,dddddd',gid,ST_AsText(ST_GeomFromMARC21(ST_AsMARC21(geom,'hddd,dddddd')),5) AS marc21 FROM asmarc21_rt ORDER BY gid;
SELECT 'hdddmm,mmmmmm',gid,ST_AsText(ST_GeomFromMARC21(ST_AsMARC21(geom,'hdddmm,mmmmmm')),5) AS marc21 FROM asmarc21_rt ORDER BY gid;
SELECT 'hdddmmss,ssssss',gid,ST_AsText(ST_GeomFromMARC21(ST_AsMARC21(geom,'hdddmmss,ssssss')),5) AS marc21 FROM asmarc21_rt ORDER BY gid;

SELECT 'dddmmss',gid,ST_AsText(ST_GeomFromMARC21(ST_AsMARC21(geom,'dddmmss')),5) AS marc21 FROM asmarc21_rt ORDER BY gid;
SELECT 'ddd.dddddd',gid,ST_AsText(ST_GeomFromMARC21(ST_AsMARC21(geom,'ddd.dddddd')),5) AS marc21 FROM asmarc21_rt ORDER BY gid;
SELECT 'dddmm.mmmmmm',gid,ST_AsText(ST_GeomFromMARC21(ST_AsMARC21(geom,'dddmm.mmmmmm')),5) AS marc21 FROM asmarc21_rt ORDER BY gid;
SELECT 'dddmmss.ssssss',gid,ST_AsText(ST_GeomFromMARC21(ST_AsMARC21(geom,'dddmmss.ssssss')),5) AS marc21 FROM asmarc21_rt ORDER BY gid;

SELECT 'dddmmss',gid,ST_AsText(ST_GeomFromMARC21(ST_AsMARC21(geom,'dddmmss')),5) AS marc21 FROM asmarc21_rt ORDER BY gid;
SELECT 'ddd,dddddd',gid,ST_AsText(ST_GeomFromMARC21(ST_AsMARC21(geom,'ddd,dddddd')),5) AS marc21 FROM asmarc21_rt ORDER BY gid;
SELECT 'dddmm,mmmmmm',gid,ST_AsText(ST_GeomFromMARC21(ST_AsMARC21(geom,'dddmm,mmmmmm')),5) AS marc21 FROM asmarc21_rt ORDER BY gid;
SELECT 'dddmmss,ssssss',gid,ST_AsText(ST_GeomFromMARC21(ST_AsMARC21(geom,'dddmmss,ssssss')),5) AS marc21 FROM asmarc21_rt ORDER BY gid;

-- Converts geometries (except points) to MARC21/XML, converts them back to geometry,
-- and compare their sizes and intersection areas.
SELECT
  'convert geom to marc21 -> convert back to geom',
  ST_GeometryType(geom) AS type_origin,
  gid,
  box2d(geom) AS box_origin,
  ST_GeometryType(
    ST_GeomFromMARC21(
      ST_AsMARC21(geom,'hddd.ddddd'))) AS type_from_marc21,
  box2d(
    ST_GeomFromMARC21(
      ST_AsMARC21(geom,'hddd.ddddd'))) AS box_marc21,

  ST_Area(
    box2d(
      ST_GeomFromMARC21(
        ST_AsMARC21(geom,'hddd.ddddd')))) / ST_Area(box2d(geom)) * 100 AS box_size_overlap,

    ST_Area(
      ST_Intersection(
        box2d(geom),
	    box2d(
	      ST_GeomFromMARC21(
	        ST_AsMARC21(geom,'hddd.ddddd')))
      )
    ) / ST_Area(box2d(geom))*100 AS box_intersection

FROM asmarc21_rt
WHERE ST_GeometryType(geom) <> 'ST_Point'
ORDER BY gid;


-- Converts points to MARC21/XML, converts them back to geometry,
-- and compares them with the original points.
-- Note: ST_GeomFromMARC21 drops the ZM dimensions.
SELECT
  'convert points to marc21 -> convert back to geom',
  ST_AsText(geom) AS point_origin,
  ST_AsText(
    ST_GeomFromMARC21(
      ST_AsMARC21(geom,'hddd.ddddd')),5) AS point_from_marc21,
  ST_Equals(
    geom,
    ST_SetSRID(
      ST_GeomFromMARC21(
        ST_AsMARC21(geom,'hddd.ddddd')),4326)) AS equals,
  ST_AsMARC21(geom,'hdddmmss.ss') AS point_as_marc21
FROM asmarc21_rt
WHERE ST_GeometryType(geom) = 'ST_Point'
ORDER BY gid;



-- ## Dumping Collections ##

-- Dumps geometries from collections and converts them to MARC21/XML
SELECT
  'dump geom -> convert to marc21',
  gid,(ST_Dump(geom)).path[1],
  ST_AsMARC21(
    (ST_Dump(geom)).geom,'hdddmmss') AS marc21
FROM asmarc21_rt
ORDER BY gid;


-- Dumps geometries from collections, converts them to MARC21/XML,
-- and calculate the percentage of their intersection area.
SELECT
  'dump geom -> convert to marc21 -> convert back to geom',
  ST_GeometryType(geom_origin) AS type_origin,
  gid,path_origin[1],
  ST_GeometryType(geom_origin_dumped) AS type_origin_dumped,
  box2d(geom_origin_dumped) AS box_orign,
  ST_GeometryType(geom_from_marc21_dumped) AS type_from_marc21_dumped,
  box2d(geom_from_marc21_dumped) AS box_marc21,
  ST_Area(box2d(geom_from_marc21_dumped)) / ST_Area(box2d(geom_origin_dumped)) * 100 AS box_size_overlap,
  ST_Area(
    ST_Intersection(
	  box2d(geom_origin_dumped),
	  box2d(geom_from_marc21_dumped)
	 )
   ) / ST_Area(box2d(geom_origin_dumped)) * 100 AS box_intersection
FROM
  (SELECT gid,
    (ST_Dump(geom)).*,
     ST_SetSRID(
       (ST_Dump(
         ST_GeomFromMARC21(
           ST_AsMARC21(geom,'hddd.ddddd')))).geom,
       4326),
     geom
   FROM asmarc21_rt) i (gid, path_origin, geom_origin_dumped, geom_from_marc21_dumped, geom_origin)
WHERE ST_GeometryType(geom_origin_dumped) <> 'ST_Point'
ORDER BY gid,path_origin[1];


SELECT 'self-intersection polygon',ST_AsMARC21('SRID=4326;POLYGON((0 0, 0 1, 2 1, 2 2, 1 2, 1 0, 0 0))','hdddmmss');
SELECT 'point(0 0)',ST_AsMARC21('SRID=4326;POINT(0 0)','hdddmmss');
SELECT 'empty_polygon',ST_AsMARC21('SRID=4326;POLYGON EMPTY','hdddmmss');
SELECT 'empty_point',ST_AsMARC21('SRID=4326;POINT EMPTY','hdddmmss');
SELECT 'empty_linstring',ST_AsMARC21('SRID=4326;LINESTRING EMPTY','hdddmmss');
SELECT 'empty_triangle',ST_AsMARC21('SRID=4326;TRIANGLE EMPTY');
SELECT 'null_parameter',ST_AsMARC21(NULL);


-- ERROR: ST_AsMARC21: Input geometry has unknown (0) SRID
SELECT 'srid 0',ST_AsMARC21('POINT(1 1)');

-- ERROR: Unsupported SRID (3587). Only lon/lat coordinate systems are supported in MARC21/XML Documents.
SELECT 'non lon/lat srs',ST_AsMARC21('SRID=3587;POINT(10472963.740359407 3586596.632100969)','hdddmmss');


SELECT 'polygon_01',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
			  <subfield code="a">a</subfield>
			  <subfield code="d">W0584745</subfield>
			  <subfield code="e">W0582451</subfield>
			  <subfield code="f">S0513416</subfield>
			  <subfield code="g">S0514450</subfield>
			</datafield>
		  </record>'),
	  4326));


SELECT 'polygon_02',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
			  <subfield code="a">a</subfield>
			  <subfield code="d">W0584745</subfield>
			  <subfield code="e">W0582451</subfield>
			  <subfield code="f">S0513416</subfield>
			  <subfield code="g">S0514450</subfield>
			</datafield>
		  </record>'),
	  4326),'hdddmmss');

SELECT 'polygon_03',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
			  <subfield code="a">a</subfield>
			  <subfield code="d">W0584745</subfield>
			  <subfield code="e">W0582451</subfield>
			  <subfield code="f">S0513416</subfield>
			  <subfield code="g">S0514450</subfield>
			</datafield>
		  </record>'),
	  4326),'hddd.dddddd');

SELECT 'polygon_04',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
			  <subfield code="a">a</subfield>
			  <subfield code="d">W0584745</subfield>
			  <subfield code="e">W0582451</subfield>
			  <subfield code="f">S0513416</subfield>
			  <subfield code="g">S0514450</subfield>
			</datafield>
		  </record>'),
	  4326),'hdddmm.mmmmmm');

SELECT 'polygon_05',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
			  <subfield code="a">a</subfield>
			  <subfield code="d">W0584745</subfield>
			  <subfield code="e">W0582451</subfield>
			  <subfield code="f">S0513416</subfield>
			  <subfield code="g">S0514450</subfield>
			</datafield>
		  </record>'),
	  4326),'hdddmmss.ssssss');

/** TODO: https://trac.osgeo.org/postgis/ticket/5142 These tests are failing under mingw64 gcc 8.1, so removing for now

SELECT 'polygon_06',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
			  <subfield code="a">a</subfield>
			  <subfield code="d">W0584745</subfield>
			  <subfield code="e">W0582451</subfield>
			  <subfield code="f">S0513416</subfield>
			  <subfield code="g">S0514450</subfield>
			</datafield>
		  </record>'),
	  4326),'hddd.dddddddddddddddddddddddddddddddddddddddddd');


SELECT 'polygon_07',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
			  <subfield code="a">a</subfield>
			  <subfield code="d">W0584745</subfield>
			  <subfield code="e">W0582451</subfield>
			  <subfield code="f">S0513416</subfield>
			  <subfield code="g">S0514450</subfield>
			</datafield>
		  </record>'),
	  4326),'hdddmm.mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm');

SELECT 'polygon_08',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
			  <subfield code="a">a</subfield>
			  <subfield code="d">W0584745</subfield>
			  <subfield code="e">W0582451</subfield>
			  <subfield code="f">S0513416</subfield>
			  <subfield code="g">S0514450</subfield>
			</datafield>
		  </record>'),
	  4326),'hdddmmss.sssssssssssssssssssssssssssssssssssssssssss');
***/
SELECT 'polygon_09',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
			  <subfield code="a">a</subfield>
			  <subfield code="d">W0584745</subfield>
			  <subfield code="e">W0582451</subfield>
			  <subfield code="f">S0513416</subfield>
			  <subfield code="g">S0514450</subfield>
			</datafield>
		  </record>'),
	  4326),'dddmmss');

SELECT 'polygon_10',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
			  <subfield code="a">a</subfield>
			  <subfield code="d">W0584745</subfield>
			  <subfield code="e">W0582451</subfield>
			  <subfield code="f">S0513416</subfield>
			  <subfield code="g">S0514450</subfield>
			</datafield>
		  </record>'),
	  4326),'ddd.ddddd');


SELECT 'polygon_11',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
			  <subfield code="a">a</subfield>
			  <subfield code="d">W0584745</subfield>
			  <subfield code="e">W0582451</subfield>
			  <subfield code="f">S0513416</subfield>
			  <subfield code="g">S0514450</subfield>
			</datafield>
		  </record>'),
	  4326),'dddmm.mmmm');

SELECT 'polygon_12',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
			  <subfield code="a">a</subfield>
			  <subfield code="d">W0584745</subfield>
			  <subfield code="e">W0582451</subfield>
			  <subfield code="f">S0513416</subfield>
			  <subfield code="g">S0514450</subfield>
			</datafield>
		  </record>'),
	  4326),'dddmmss.ssss');




SELECT 'polygon_12',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	  4326));


SELECT 'polygon_13',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	  4326),'hdddmmss');

SELECT 'polygon_14',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	  4326),'hddd.dddddd');

SELECT 'polygon_15',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	  4326),'hdddmm.mmmmmm');

SELECT 'polygon_16',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	  4326),'hdddmmss.ssssss');

SELECT 'polygon_17',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	  4326),'hddd.ddddd');

SELECT 'polygon_18',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	  4326),'hdddmm.mmmmm');

SELECT 'polygon_19',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	  4326),'hdddmmss.sssss');

SELECT 'polygon_20',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	  4326),'dddmmss');

SELECT 'polygon_21',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	  4326),'ddd.ddddd');

SELECT 'polygon_22',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	  4326),'dddmm.mmmm');

SELECT 'polygon_23',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	  4326),'dddmmss.ssss');


SELECT 'polygon_24',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	  4326),'hddd,ddddd');

SELECT 'polygon_25',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	  4326),'hdddmm,mmmm');

SELECT 'polygon_26',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
			  <subfield code="a">a</subfield>
			  <subfield code="d">W0584745</subfield>
			  <subfield code="e">W0582451</subfield>
			  <subfield code="f">S0513416</subfield>
			  <subfield code="g">S0514450</subfield>
			</datafield>
		  </record>'),
	  4326),'hdddmmss,ssss');

SELECT 'polygon_27',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
			  <subfield code="a">a</subfield>
			  <subfield code="d">W0584745</subfield>
			  <subfield code="e">W0582451</subfield>
			  <subfield code="f">S0513416</subfield>
			  <subfield code="g">S0514450</subfield>
			</datafield>
		  </record>'),
	  4326),'dddmmss,ssss');

SELECT 'polygon_28',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
			  <subfield code="a">a</subfield>
			  <subfield code="d">W0584745</subfield>
			  <subfield code="e">W0582451</subfield>
			  <subfield code="f">S0513416</subfield>
			  <subfield code="g">S0514450</subfield>
			</datafield>
		  </record>'),
	  4326),'ddd,ddddd');


SELECT 'polygon_29',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	  4326),'ddd,ddddd');

SELECT 'polygon_30',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
			  <subfield code="a">a</subfield>
			  <subfield code="d">W0584745</subfield>
			  <subfield code="e">W0582451</subfield>
			  <subfield code="f">S0513416</subfield>
			  <subfield code="g">S0514450</subfield>
			</datafield>
		  </record>'),
	  4326),'hdddmmss.sssss');


SELECT 'polygon_31',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	4326),'hdddmmss.sssss');


-- ERROR: invalid format

SELECT 'error_1',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	4326),'h.dddmmss.sssss');

SELECT 'error_2',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	4326),'hddd.mmmm');

SELECT 'error_3',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	4326),'ddd.mmmm');

SELECT 'error_4',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	4326),'hdddmmmss');

SELECT 'error_5',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	4326),'hdddmmsss');

SELECT 'error_6',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	4326),'dddmmmss');

SELECT 'error_7',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	4326),'dddmmsss');

SELECT 'error_8',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	4326),'dddmm.ss');

SELECT 'error_9',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	4326),'hdddmm.ss');

SELECT 'error_10',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	4326),'hdddmm,ss');


SELECT 'error_11',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	4326),'ddd,mmmmm');



SELECT 'null_01',
  ST_AsMARC21(
	ST_SetSRID(
	  ST_GeomFromMARC21('
		  <record xmlns="http://www.loc.gov/MARC21/slim">
			<datafield tag="034" ind1="1" ind2=" ">
  			  <subfield code="a">a</subfield>
			  <subfield code="d">E1262500</subfield>
			  <subfield code="e">E1291530</subfield>
			  <subfield code="f">N0360738</subfield>
			  <subfield code="g">N0350200</subfield>
			</datafield>
		  </record>'),
	4326),NULL);


DROP TABLE asmarc21_rt;

