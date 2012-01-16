SET client_min_messages TO warning;

CREATE SCHEMA tm;

-- Test construction of typed tables

CREATE TABLE tm.circularstring (id serial, g geometry(circularstring) );
CREATE TABLE tm.circularstring0 (id serial, g geometry(circularstring, 0) );
CREATE TABLE tm.circularstring1 (id serial, g geometry(circularstring, 1) );
CREATE TABLE tm.circularstringm (id serial, g geometry(circularstringm) );
CREATE TABLE tm.circularstringm0 (id serial, g geometry(circularstringm, 0) );
CREATE TABLE tm.circularstringm1 (id serial, g geometry(circularstringm, 1) );
CREATE TABLE tm.circularstringz (id serial, g geometry(circularstringz) );
CREATE TABLE tm.circularstringz0 (id serial, g geometry(circularstringz, 0) );
CREATE TABLE tm.circularstringz1 (id serial, g geometry(circularstringz, 1) );
CREATE TABLE tm.circularstringzm (id serial, g geometry(circularstringzm) );
CREATE TABLE tm.circularstringzm0 (id serial, g geometry(circularstringzm, 0) );
CREATE TABLE tm.circularstringzm1 (id serial, g geometry(circularstringzm, 1) );

CREATE TABLE tm.compoundcurve (id serial, g geometry(compoundcurve) );
CREATE TABLE tm.compoundcurve0 (id serial, g geometry(compoundcurve, 0) );
CREATE TABLE tm.compoundcurve1 (id serial, g geometry(compoundcurve, 1) );
CREATE TABLE tm.compoundcurvem (id serial, g geometry(compoundcurvem) );
CREATE TABLE tm.compoundcurvem0 (id serial, g geometry(compoundcurvem, 0) );
CREATE TABLE tm.compoundcurvem1 (id serial, g geometry(compoundcurvem, 1) );
CREATE TABLE tm.compoundcurvez (id serial, g geometry(compoundcurvez) );
CREATE TABLE tm.compoundcurvez0 (id serial, g geometry(compoundcurvez, 0) );
CREATE TABLE tm.compoundcurvez1 (id serial, g geometry(compoundcurvez, 1) );
CREATE TABLE tm.compoundcurvezm (id serial, g geometry(compoundcurvezm) );
CREATE TABLE tm.compoundcurvezm0 (id serial, g geometry(compoundcurvezm, 0) );
CREATE TABLE tm.compoundcurvezm1 (id serial, g geometry(compoundcurvezm, 1) );

CREATE TABLE tm.curvepolygon (id serial, g geometry(curvepolygon) );
CREATE TABLE tm.curvepolygon0 (id serial, g geometry(curvepolygon, 0) );
CREATE TABLE tm.curvepolygon1 (id serial, g geometry(curvepolygon, 1) );
CREATE TABLE tm.curvepolygonm (id serial, g geometry(curvepolygonm) );
CREATE TABLE tm.curvepolygonm0 (id serial, g geometry(curvepolygonm, 0) );
CREATE TABLE tm.curvepolygonm1 (id serial, g geometry(curvepolygonm, 1) );
CREATE TABLE tm.curvepolygonz (id serial, g geometry(curvepolygonz) );
CREATE TABLE tm.curvepolygonz0 (id serial, g geometry(curvepolygonz, 0) );
CREATE TABLE tm.curvepolygonz1 (id serial, g geometry(curvepolygonz, 1) );
CREATE TABLE tm.curvepolygonzm (id serial, g geometry(curvepolygonzm) );
CREATE TABLE tm.curvepolygonzm0 (id serial, g geometry(curvepolygonzm, 0) );
CREATE TABLE tm.curvepolygonzm1 (id serial, g geometry(curvepolygonzm, 1) );

CREATE TABLE tm.geometry (id serial, g geometry(geometry) );
CREATE TABLE tm.geometry0 (id serial, g geometry(geometry, 0) );
CREATE TABLE tm.geometry1 (id serial, g geometry(geometry, 1) );
CREATE TABLE tm.geometrym (id serial, g geometry(geometrym) );
CREATE TABLE tm.geometrym0 (id serial, g geometry(geometrym, 0) );
CREATE TABLE tm.geometrym1 (id serial, g geometry(geometrym, 1) );
CREATE TABLE tm.geometryz (id serial, g geometry(geometryz) );
CREATE TABLE tm.geometryz0 (id serial, g geometry(geometryz, 0) );
CREATE TABLE tm.geometryz1 (id serial, g geometry(geometryz, 1) );
CREATE TABLE tm.geometryzm (id serial, g geometry(geometryzm) );
CREATE TABLE tm.geometryzm0 (id serial, g geometry(geometryzm, 0) );
CREATE TABLE tm.geometryzm1 (id serial, g geometry(geometryzm, 1) );

CREATE TABLE tm.geometrycollection (id serial, g geometry(geometrycollection) );
CREATE TABLE tm.geometrycollection0 (id serial, g geometry(geometrycollection, 0) );
CREATE TABLE tm.geometrycollection1 (id serial, g geometry(geometrycollection, 1) );
CREATE TABLE tm.geometrycollectionm (id serial, g geometry(geometrycollectionm) );
CREATE TABLE tm.geometrycollectionm0 (id serial, g geometry(geometrycollectionm, 0) );
CREATE TABLE tm.geometrycollectionm1 (id serial, g geometry(geometrycollectionm, 1) );
CREATE TABLE tm.geometrycollectionz (id serial, g geometry(geometrycollectionz) );
CREATE TABLE tm.geometrycollectionz0 (id serial, g geometry(geometrycollectionz, 0) );
CREATE TABLE tm.geometrycollectionz1 (id serial, g geometry(geometrycollectionz, 1) );
CREATE TABLE tm.geometrycollectionzm (id serial, g geometry(geometrycollectionzm) );
CREATE TABLE tm.geometrycollectionzm0 (id serial, g geometry(geometrycollectionzm, 0) );
CREATE TABLE tm.geometrycollectionzm1 (id serial, g geometry(geometrycollectionzm, 1) );

CREATE TABLE tm.linestring (id serial, g geometry(linestring) );
CREATE TABLE tm.linestring0 (id serial, g geometry(linestring, 0) );
CREATE TABLE tm.linestring1 (id serial, g geometry(linestring, 1) );
CREATE TABLE tm.linestringm (id serial, g geometry(linestringm) );
CREATE TABLE tm.linestringm0 (id serial, g geometry(linestringm, 0) );
CREATE TABLE tm.linestringm1 (id serial, g geometry(linestringm, 1) );
CREATE TABLE tm.linestringz (id serial, g geometry(linestringz) );
CREATE TABLE tm.linestringz0 (id serial, g geometry(linestringz, 0) );
CREATE TABLE tm.linestringz1 (id serial, g geometry(linestringz, 1) );
CREATE TABLE tm.linestringzm (id serial, g geometry(linestringzm) );
CREATE TABLE tm.linestringzm0 (id serial, g geometry(linestringzm, 0) );
CREATE TABLE tm.linestringzm1 (id serial, g geometry(linestringzm, 1) );

CREATE TABLE tm.multicurve (id serial, g geometry(multicurve) );
CREATE TABLE tm.multicurve0 (id serial, g geometry(multicurve, 0) );
CREATE TABLE tm.multicurve1 (id serial, g geometry(multicurve, 1) );
CREATE TABLE tm.multicurvem (id serial, g geometry(multicurvem) );
CREATE TABLE tm.multicurvem0 (id serial, g geometry(multicurvem, 0) );
CREATE TABLE tm.multicurvem1 (id serial, g geometry(multicurvem, 1) );
CREATE TABLE tm.multicurvez (id serial, g geometry(multicurvez) );
CREATE TABLE tm.multicurvez0 (id serial, g geometry(multicurvez, 0) );
CREATE TABLE tm.multicurvez1 (id serial, g geometry(multicurvez, 1) );
CREATE TABLE tm.multicurvezm (id serial, g geometry(multicurvezm) );
CREATE TABLE tm.multicurvezm0 (id serial, g geometry(multicurvezm, 0) );
CREATE TABLE tm.multicurvezm1 (id serial, g geometry(multicurvezm, 1) );

CREATE TABLE tm.multilinestring (id serial, g geometry(multilinestring) );
CREATE TABLE tm.multilinestring0 (id serial, g geometry(multilinestring, 0) );
CREATE TABLE tm.multilinestring1 (id serial, g geometry(multilinestring, 1) );
CREATE TABLE tm.multilinestringm (id serial, g geometry(multilinestringm) );
CREATE TABLE tm.multilinestringm0 (id serial, g geometry(multilinestringm, 0) );
CREATE TABLE tm.multilinestringm1 (id serial, g geometry(multilinestringm, 1) );
CREATE TABLE tm.multilinestringz (id serial, g geometry(multilinestringz) );
CREATE TABLE tm.multilinestringz0 (id serial, g geometry(multilinestringz, 0) );
CREATE TABLE tm.multilinestringz1 (id serial, g geometry(multilinestringz, 1) );
CREATE TABLE tm.multilinestringzm (id serial, g geometry(multilinestringzm) );
CREATE TABLE tm.multilinestringzm0 (id serial, g geometry(multilinestringzm, 0) );
CREATE TABLE tm.multilinestringzm1 (id serial, g geometry(multilinestringzm, 1) );

CREATE TABLE tm.multipolygon (id serial, g geometry(multipolygon) );
CREATE TABLE tm.multipolygon0 (id serial, g geometry(multipolygon, 0) );
CREATE TABLE tm.multipolygon1 (id serial, g geometry(multipolygon, 1) );
CREATE TABLE tm.multipolygonm (id serial, g geometry(multipolygonm) );
CREATE TABLE tm.multipolygonm0 (id serial, g geometry(multipolygonm, 0) );
CREATE TABLE tm.multipolygonm1 (id serial, g geometry(multipolygonm, 1) );
CREATE TABLE tm.multipolygonz (id serial, g geometry(multipolygonz) );
CREATE TABLE tm.multipolygonz0 (id serial, g geometry(multipolygonz, 0) );
CREATE TABLE tm.multipolygonz1 (id serial, g geometry(multipolygonz, 1) );
CREATE TABLE tm.multipolygonzm (id serial, g geometry(multipolygonzm) );
CREATE TABLE tm.multipolygonzm0 (id serial, g geometry(multipolygonzm, 0) );
CREATE TABLE tm.multipolygonzm1 (id serial, g geometry(multipolygonzm, 1) );

CREATE TABLE tm.multipoint (id serial, g geometry(multipoint) );
CREATE TABLE tm.multipoint0 (id serial, g geometry(multipoint, 0) );
CREATE TABLE tm.multipoint1 (id serial, g geometry(multipoint, 1) );
CREATE TABLE tm.multipointm (id serial, g geometry(multipointm) );
CREATE TABLE tm.multipointm0 (id serial, g geometry(multipointm, 0) );
CREATE TABLE tm.multipointm1 (id serial, g geometry(multipointm, 1) );
CREATE TABLE tm.multipointz (id serial, g geometry(multipointz) );
CREATE TABLE tm.multipointz0 (id serial, g geometry(multipointz, 0) );
CREATE TABLE tm.multipointz1 (id serial, g geometry(multipointz, 1) );
CREATE TABLE tm.multipointzm (id serial, g geometry(multipointzm) );
CREATE TABLE tm.multipointzm0 (id serial, g geometry(multipointzm, 0) );
CREATE TABLE tm.multipointzm1 (id serial, g geometry(multipointzm, 1) );

CREATE TABLE tm.multisurface (id serial, g geometry(multisurface) );
CREATE TABLE tm.multisurface0 (id serial, g geometry(multisurface, 0) );
CREATE TABLE tm.multisurface1 (id serial, g geometry(multisurface, 1) );
CREATE TABLE tm.multisurfacem (id serial, g geometry(multisurfacem) );
CREATE TABLE tm.multisurfacem0 (id serial, g geometry(multisurfacem, 0) );
CREATE TABLE tm.multisurfacem1 (id serial, g geometry(multisurfacem, 1) );
CREATE TABLE tm.multisurfacez (id serial, g geometry(multisurfacez) );
CREATE TABLE tm.multisurfacez0 (id serial, g geometry(multisurfacez, 0) );
CREATE TABLE tm.multisurfacez1 (id serial, g geometry(multisurfacez, 1) );
CREATE TABLE tm.multisurfacezm (id serial, g geometry(multisurfacezm) );
CREATE TABLE tm.multisurfacezm0 (id serial, g geometry(multisurfacezm, 0) );
CREATE TABLE tm.multisurfacezm1 (id serial, g geometry(multisurfacezm, 1) );

CREATE TABLE tm.point (id serial, g geometry(point) );
CREATE TABLE tm.point0 (id serial, g geometry(point, 0) );
CREATE TABLE tm.point1 (id serial, g geometry(point, 1) );
CREATE TABLE tm.pointm (id serial, g geometry(pointm) );
CREATE TABLE tm.pointm0 (id serial, g geometry(pointm, 0) );
CREATE TABLE tm.pointm1 (id serial, g geometry(pointm, 1) );
CREATE TABLE tm.pointz (id serial, g geometry(pointz) );
CREATE TABLE tm.pointz0 (id serial, g geometry(pointz, 0) );
CREATE TABLE tm.pointz1 (id serial, g geometry(pointz, 1) );
CREATE TABLE tm.pointzm (id serial, g geometry(pointzm) );
CREATE TABLE tm.pointzm0 (id serial, g geometry(pointzm, 0) );
CREATE TABLE tm.pointzm1 (id serial, g geometry(pointzm, 1) );

CREATE TABLE tm.polygon (id serial, g geometry(polygon) );
CREATE TABLE tm.polygon0 (id serial, g geometry(polygon, 0) );
CREATE TABLE tm.polygon1 (id serial, g geometry(polygon, 1) );
CREATE TABLE tm.polygonm (id serial, g geometry(polygonm) );
CREATE TABLE tm.polygonm0 (id serial, g geometry(polygonm, 0) );
CREATE TABLE tm.polygonm1 (id serial, g geometry(polygonm, 1) );
CREATE TABLE tm.polygonz (id serial, g geometry(polygonz) );
CREATE TABLE tm.polygonz0 (id serial, g geometry(polygonz, 0) );
CREATE TABLE tm.polygonz1 (id serial, g geometry(polygonz, 1) );
CREATE TABLE tm.polygonzm (id serial, g geometry(polygonzm) );
CREATE TABLE tm.polygonzm0 (id serial, g geometry(polygonzm, 0) );
CREATE TABLE tm.polygonzm1 (id serial, g geometry(polygonzm, 1) );

CREATE TABLE tm.polyhedralsurface (id serial, g geometry(polyhedralsurface) );
CREATE TABLE tm.polyhedralsurface0 (id serial, g geometry(polyhedralsurface, 0) );
CREATE TABLE tm.polyhedralsurface1 (id serial, g geometry(polyhedralsurface, 1) );
CREATE TABLE tm.polyhedralsurfacem (id serial, g geometry(polyhedralsurfacem) );
CREATE TABLE tm.polyhedralsurfacem0 (id serial, g geometry(polyhedralsurfacem, 0) );
CREATE TABLE tm.polyhedralsurfacem1 (id serial, g geometry(polyhedralsurfacem, 1) );
CREATE TABLE tm.polyhedralsurfacez (id serial, g geometry(polyhedralsurfacez) );
CREATE TABLE tm.polyhedralsurfacez0 (id serial, g geometry(polyhedralsurfacez, 0) );
CREATE TABLE tm.polyhedralsurfacez1 (id serial, g geometry(polyhedralsurfacez, 1) );
CREATE TABLE tm.polyhedralsurfacezm (id serial, g geometry(polyhedralsurfacezm) );
CREATE TABLE tm.polyhedralsurfacezm0 (id serial, g geometry(polyhedralsurfacezm, 0) );
CREATE TABLE tm.polyhedralsurfacezm1 (id serial, g geometry(polyhedralsurfacezm, 1) );

CREATE TABLE tm.tin (id serial, g geometry(tin) );
CREATE TABLE tm.tin0 (id serial, g geometry(tin, 0) );
CREATE TABLE tm.tin1 (id serial, g geometry(tin, 1) );
CREATE TABLE tm.tinm (id serial, g geometry(tinm) );
CREATE TABLE tm.tinm0 (id serial, g geometry(tinm, 0) );
CREATE TABLE tm.tinm1 (id serial, g geometry(tinm, 1) );
CREATE TABLE tm.tinz (id serial, g geometry(tinz) );
CREATE TABLE tm.tinz0 (id serial, g geometry(tinz, 0) );
CREATE TABLE tm.tinz1 (id serial, g geometry(tinz, 1) );
CREATE TABLE tm.tinzm (id serial, g geometry(tinzm) );
CREATE TABLE tm.tinzm0 (id serial, g geometry(tinzm, 0) );
CREATE TABLE tm.tinzm1 (id serial, g geometry(tinzm, 1) );

CREATE TABLE tm.triangle (id serial, g geometry(triangle) );
CREATE TABLE tm.triangle0 (id serial, g geometry(triangle, 0) );
CREATE TABLE tm.triangle1 (id serial, g geometry(triangle, 1) );
CREATE TABLE tm.trianglem (id serial, g geometry(trianglem) );
CREATE TABLE tm.trianglem0 (id serial, g geometry(trianglem, 0) );
CREATE TABLE tm.trianglem1 (id serial, g geometry(trianglem, 1) );
CREATE TABLE tm.trianglez (id serial, g geometry(trianglez) );
CREATE TABLE tm.trianglez0 (id serial, g geometry(trianglez, 0) );
CREATE TABLE tm.trianglez1 (id serial, g geometry(trianglez, 1) );
CREATE TABLE tm.trianglezm (id serial, g geometry(trianglezm) );
CREATE TABLE tm.trianglezm0 (id serial, g geometry(trianglezm, 0) );
CREATE TABLE tm.trianglezm1 (id serial, g geometry(trianglezm, 1) );

SELECT * from geometry_columns ORDER BY 3;

CREATE TABLE tm.types (id serial, g geometry);

INSERT INTO tm.types(g) values ('SRID=0;POINT EMPTY');
INSERT INTO tm.types(g) values ('SRID=0;LINESTRING EMPTY');
INSERT INTO tm.types(g) values ('SRID=0;POLYGON EMPTY');
INSERT INTO tm.types(g) values ('SRID=0;MULTIPOINT EMPTY');
INSERT INTO tm.types(g) values ('SRID=0;MULTILINESTRING EMPTY');
INSERT INTO tm.types(g) values ('SRID=0;MULTIPOLYGON EMPTY');
INSERT INTO tm.types(g) values ('SRID=0;GEOMETRYCOLLECTION EMPTY');
INSERT INTO tm.types(g) values ('SRID=0;CIRCULARSTRING EMPTY');
INSERT INTO tm.types(g) values ('SRID=0;COMPOUNDCURVE EMPTY');
INSERT INTO tm.types(g) values ('SRID=0;CURVEPOLYGON EMPTY');
INSERT INTO tm.types(g) values ('SRID=0;MULTICURVE EMPTY');
INSERT INTO tm.types(g) values ('SRID=0;MULTISURFACE EMPTY');
INSERT INTO tm.types(g) values ('SRID=0;POLYHEDRALSURFACE EMPTY');
INSERT INTO tm.types(g) values ('SRID=0;TRIANGLE EMPTY');
INSERT INTO tm.types(g) values ('SRID=0;TIN EMPTY');

-- all zm flags
INSERT INTO tm.types(g)
SELECT st_force_3dz(g) FROM tm.types WHERE id < 15 ORDER BY id;
INSERT INTO tm.types(g)
SELECT st_force_3dm(g) FROM tm.types WHERE id < 15 ORDER BY id;
INSERT INTO tm.types(g)
SELECT st_force_4d(g) FROM tm.types WHERE id < 15 ORDER BY id;

-- known srid
INSERT INTO tm.types(g)
SELECT st_setsrid(g,1) FROM tm.types ORDER BY id;


-- Now try to insert each type into each table
CREATE FUNCTION tm.insert_all()
RETURNS TABLE(out_where varchar, out_what varchar, out_status text)
AS
$$
DECLARE
	sql text;
	rec RECORD;
	rec2 RECORD;
BEGIN
	FOR rec IN SELECT * from geometry_columns
		WHERE f_table_name != 'types' ORDER BY 3 
	LOOP
		out_where := rec.f_table_name;
		FOR rec2 IN SELECT * from tm.types ORDER BY id 
		LOOP
			out_what := 'SRID=' || ST_Srid(rec2.g) || ';' || ST_AsText(rec2.g);
			BEGIN
				sql := 'INSERT INTO '
					|| quote_ident(rec.f_table_schema)
					|| '.' || quote_ident(rec.f_table_name)
					|| '(g) VALUES ('
					|| quote_literal(rec2.g::text)
					|| ');';
				EXECUTE sql;
				out_status := 'OK';
			EXCEPTION
			WHEN OTHERS THEN
				out_status := SQLERRM;
			END;
			RETURN NEXT;
		END LOOP;
	END LOOP;
END;
$$ LANGUAGE 'plpgsql';

select * FROM tm.insert_all();

DROP SCHEMA tm CASCADE;

