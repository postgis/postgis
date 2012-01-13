SET client_min_messages TO warning;

CREATE SCHEMA tm;

-- Test construction of typed tables

CREATE TABLE tm.circularstring (a int, g geometry(circularstring) );
CREATE TABLE tm.circularstring0 (a int, g geometry(circularstring, 0) );
CREATE TABLE tm.circularstring1 (a int, g geometry(circularstring, 1) );
CREATE TABLE tm.circularstringm (a int, g geometry(circularstringm) );
CREATE TABLE tm.circularstringm0 (a int, g geometry(circularstringm, 0) );
CREATE TABLE tm.circularstringm1 (a int, g geometry(circularstringm, 1) );
CREATE TABLE tm.circularstringz (a int, g geometry(circularstringz) );
CREATE TABLE tm.circularstringz0 (a int, g geometry(circularstringz, 0) );
CREATE TABLE tm.circularstringz1 (a int, g geometry(circularstringz, 1) );
CREATE TABLE tm.circularstringzm (a int, g geometry(circularstringzm) );
CREATE TABLE tm.circularstringzm0 (a int, g geometry(circularstringzm, 0) );
CREATE TABLE tm.circularstringzm1 (a int, g geometry(circularstringzm, 1) );

CREATE TABLE tm.compoundcurve (a int, g geometry(compoundcurve) );
CREATE TABLE tm.compoundcurve0 (a int, g geometry(compoundcurve, 0) );
CREATE TABLE tm.compoundcurve1 (a int, g geometry(compoundcurve, 1) );
CREATE TABLE tm.compoundcurvem (a int, g geometry(compoundcurvem) );
CREATE TABLE tm.compoundcurvem0 (a int, g geometry(compoundcurvem, 0) );
CREATE TABLE tm.compoundcurvem1 (a int, g geometry(compoundcurvem, 1) );
CREATE TABLE tm.compoundcurvez (a int, g geometry(compoundcurvez) );
CREATE TABLE tm.compoundcurvez0 (a int, g geometry(compoundcurvez, 0) );
CREATE TABLE tm.compoundcurvez1 (a int, g geometry(compoundcurvez, 1) );
CREATE TABLE tm.compoundcurvezm (a int, g geometry(compoundcurvezm) );
CREATE TABLE tm.compoundcurvezm0 (a int, g geometry(compoundcurvezm, 0) );
CREATE TABLE tm.compoundcurvezm1 (a int, g geometry(compoundcurvezm, 1) );

CREATE TABLE tm.curvepolygon (a int, g geometry(curvepolygon) );
CREATE TABLE tm.curvepolygon0 (a int, g geometry(curvepolygon, 0) );
CREATE TABLE tm.curvepolygon1 (a int, g geometry(curvepolygon, 1) );
CREATE TABLE tm.curvepolygonm (a int, g geometry(curvepolygonm) );
CREATE TABLE tm.curvepolygonm0 (a int, g geometry(curvepolygonm, 0) );
CREATE TABLE tm.curvepolygonm1 (a int, g geometry(curvepolygonm, 1) );
CREATE TABLE tm.curvepolygonz (a int, g geometry(curvepolygonz) );
CREATE TABLE tm.curvepolygonz0 (a int, g geometry(curvepolygonz, 0) );
CREATE TABLE tm.curvepolygonz1 (a int, g geometry(curvepolygonz, 1) );
CREATE TABLE tm.curvepolygonzm (a int, g geometry(curvepolygonzm) );
CREATE TABLE tm.curvepolygonzm0 (a int, g geometry(curvepolygonzm, 0) );
CREATE TABLE tm.curvepolygonzm1 (a int, g geometry(curvepolygonzm, 1) );

CREATE TABLE tm.geometry (a int, g geometry(geometry) );
CREATE TABLE tm.geometry0 (a int, g geometry(geometry, 0) );
CREATE TABLE tm.geometry1 (a int, g geometry(geometry, 1) );
CREATE TABLE tm.geometrym (a int, g geometry(geometrym) );
CREATE TABLE tm.geometrym0 (a int, g geometry(geometrym, 0) );
CREATE TABLE tm.geometrym1 (a int, g geometry(geometrym, 1) );
CREATE TABLE tm.geometryz (a int, g geometry(geometryz) );
CREATE TABLE tm.geometryz0 (a int, g geometry(geometryz, 0) );
CREATE TABLE tm.geometryz1 (a int, g geometry(geometryz, 1) );
CREATE TABLE tm.geometryzm (a int, g geometry(geometryzm) );
CREATE TABLE tm.geometryzm0 (a int, g geometry(geometryzm, 0) );
CREATE TABLE tm.geometryzm1 (a int, g geometry(geometryzm, 1) );

CREATE TABLE tm.geometrycollection (a int, g geometry(geometrycollection) );
CREATE TABLE tm.geometrycollection0 (a int, g geometry(geometrycollection, 0) );
CREATE TABLE tm.geometrycollection1 (a int, g geometry(geometrycollection, 1) );
CREATE TABLE tm.geometrycollectionm (a int, g geometry(geometrycollectionm) );
CREATE TABLE tm.geometrycollectionm0 (a int, g geometry(geometrycollectionm, 0) );
CREATE TABLE tm.geometrycollectionm1 (a int, g geometry(geometrycollectionm, 1) );
CREATE TABLE tm.geometrycollectionz (a int, g geometry(geometrycollectionz) );
CREATE TABLE tm.geometrycollectionz0 (a int, g geometry(geometrycollectionz, 0) );
CREATE TABLE tm.geometrycollectionz1 (a int, g geometry(geometrycollectionz, 1) );
CREATE TABLE tm.geometrycollectionzm (a int, g geometry(geometrycollectionzm) );
CREATE TABLE tm.geometrycollectionzm0 (a int, g geometry(geometrycollectionzm, 0) );
CREATE TABLE tm.geometrycollectionzm1 (a int, g geometry(geometrycollectionzm, 1) );

CREATE TABLE tm.linestring (a int, g geometry(linestring) );
CREATE TABLE tm.linestring0 (a int, g geometry(linestring, 0) );
CREATE TABLE tm.linestring1 (a int, g geometry(linestring, 1) );
CREATE TABLE tm.linestringm (a int, g geometry(linestringm) );
CREATE TABLE tm.linestringm0 (a int, g geometry(linestringm, 0) );
CREATE TABLE tm.linestringm1 (a int, g geometry(linestringm, 1) );
CREATE TABLE tm.linestringz (a int, g geometry(linestringz) );
CREATE TABLE tm.linestringz0 (a int, g geometry(linestringz, 0) );
CREATE TABLE tm.linestringz1 (a int, g geometry(linestringz, 1) );
CREATE TABLE tm.linestringzm (a int, g geometry(linestringzm) );
CREATE TABLE tm.linestringzm0 (a int, g geometry(linestringzm, 0) );
CREATE TABLE tm.linestringzm1 (a int, g geometry(linestringzm, 1) );

CREATE TABLE tm.multicurve (a int, g geometry(multicurve) );
CREATE TABLE tm.multicurve0 (a int, g geometry(multicurve, 0) );
CREATE TABLE tm.multicurve1 (a int, g geometry(multicurve, 1) );
CREATE TABLE tm.multicurvem (a int, g geometry(multicurvem) );
CREATE TABLE tm.multicurvem0 (a int, g geometry(multicurvem, 0) );
CREATE TABLE tm.multicurvem1 (a int, g geometry(multicurvem, 1) );
CREATE TABLE tm.multicurvez (a int, g geometry(multicurvez) );
CREATE TABLE tm.multicurvez0 (a int, g geometry(multicurvez, 0) );
CREATE TABLE tm.multicurvez1 (a int, g geometry(multicurvez, 1) );
CREATE TABLE tm.multicurvezm (a int, g geometry(multicurvezm) );
CREATE TABLE tm.multicurvezm0 (a int, g geometry(multicurvezm, 0) );
CREATE TABLE tm.multicurvezm1 (a int, g geometry(multicurvezm, 1) );

CREATE TABLE tm.multilinestring (a int, g geometry(multilinestring) );
CREATE TABLE tm.multilinestring0 (a int, g geometry(multilinestring, 0) );
CREATE TABLE tm.multilinestring1 (a int, g geometry(multilinestring, 1) );
CREATE TABLE tm.multilinestringm (a int, g geometry(multilinestringm) );
CREATE TABLE tm.multilinestringm0 (a int, g geometry(multilinestringm, 0) );
CREATE TABLE tm.multilinestringm1 (a int, g geometry(multilinestringm, 1) );
CREATE TABLE tm.multilinestringz (a int, g geometry(multilinestringz) );
CREATE TABLE tm.multilinestringz0 (a int, g geometry(multilinestringz, 0) );
CREATE TABLE tm.multilinestringz1 (a int, g geometry(multilinestringz, 1) );
CREATE TABLE tm.multilinestringzm (a int, g geometry(multilinestringzm) );
CREATE TABLE tm.multilinestringzm0 (a int, g geometry(multilinestringzm, 0) );
CREATE TABLE tm.multilinestringzm1 (a int, g geometry(multilinestringzm, 1) );

CREATE TABLE tm.multipolygon (a int, g geometry(multipolygon) );
CREATE TABLE tm.multipolygon0 (a int, g geometry(multipolygon, 0) );
CREATE TABLE tm.multipolygon1 (a int, g geometry(multipolygon, 1) );
CREATE TABLE tm.multipolygonm (a int, g geometry(multipolygonm) );
CREATE TABLE tm.multipolygonm0 (a int, g geometry(multipolygonm, 0) );
CREATE TABLE tm.multipolygonm1 (a int, g geometry(multipolygonm, 1) );
CREATE TABLE tm.multipolygonz (a int, g geometry(multipolygonz) );
CREATE TABLE tm.multipolygonz0 (a int, g geometry(multipolygonz, 0) );
CREATE TABLE tm.multipolygonz1 (a int, g geometry(multipolygonz, 1) );
CREATE TABLE tm.multipolygonzm (a int, g geometry(multipolygonzm) );
CREATE TABLE tm.multipolygonzm0 (a int, g geometry(multipolygonzm, 0) );
CREATE TABLE tm.multipolygonzm1 (a int, g geometry(multipolygonzm, 1) );

CREATE TABLE tm.multipoint (a int, g geometry(multipoint) );
CREATE TABLE tm.multipoint0 (a int, g geometry(multipoint, 0) );
CREATE TABLE tm.multipoint1 (a int, g geometry(multipoint, 1) );
CREATE TABLE tm.multipointm (a int, g geometry(multipointm) );
CREATE TABLE tm.multipointm0 (a int, g geometry(multipointm, 0) );
CREATE TABLE tm.multipointm1 (a int, g geometry(multipointm, 1) );
CREATE TABLE tm.multipointz (a int, g geometry(multipointz) );
CREATE TABLE tm.multipointz0 (a int, g geometry(multipointz, 0) );
CREATE TABLE tm.multipointz1 (a int, g geometry(multipointz, 1) );
CREATE TABLE tm.multipointzm (a int, g geometry(multipointzm) );
CREATE TABLE tm.multipointzm0 (a int, g geometry(multipointzm, 0) );
CREATE TABLE tm.multipointzm1 (a int, g geometry(multipointzm, 1) );

CREATE TABLE tm.multisurface (a int, g geometry(multisurface) );
CREATE TABLE tm.multisurface0 (a int, g geometry(multisurface, 0) );
CREATE TABLE tm.multisurface1 (a int, g geometry(multisurface, 1) );
CREATE TABLE tm.multisurfacem (a int, g geometry(multisurfacem) );
CREATE TABLE tm.multisurfacem0 (a int, g geometry(multisurfacem, 0) );
CREATE TABLE tm.multisurfacem1 (a int, g geometry(multisurfacem, 1) );
CREATE TABLE tm.multisurfacez (a int, g geometry(multisurfacez) );
CREATE TABLE tm.multisurfacez0 (a int, g geometry(multisurfacez, 0) );
CREATE TABLE tm.multisurfacez1 (a int, g geometry(multisurfacez, 1) );
CREATE TABLE tm.multisurfacezm (a int, g geometry(multisurfacezm) );
CREATE TABLE tm.multisurfacezm0 (a int, g geometry(multisurfacezm, 0) );
CREATE TABLE tm.multisurfacezm1 (a int, g geometry(multisurfacezm, 1) );

CREATE TABLE tm.point (a int, g geometry(point) );
CREATE TABLE tm.point0 (a int, g geometry(point, 0) );
CREATE TABLE tm.point1 (a int, g geometry(point, 1) );
CREATE TABLE tm.pointm (a int, g geometry(pointm) );
CREATE TABLE tm.pointm0 (a int, g geometry(pointm, 0) );
CREATE TABLE tm.pointm1 (a int, g geometry(pointm, 1) );
CREATE TABLE tm.pointz (a int, g geometry(pointz) );
CREATE TABLE tm.pointz0 (a int, g geometry(pointz, 0) );
CREATE TABLE tm.pointz1 (a int, g geometry(pointz, 1) );
CREATE TABLE tm.pointzm (a int, g geometry(pointzm) );
CREATE TABLE tm.pointzm0 (a int, g geometry(pointzm, 0) );
CREATE TABLE tm.pointzm1 (a int, g geometry(pointzm, 1) );

CREATE TABLE tm.polygon (a int, g geometry(polygon) );
CREATE TABLE tm.polygon0 (a int, g geometry(polygon, 0) );
CREATE TABLE tm.polygon1 (a int, g geometry(polygon, 1) );
CREATE TABLE tm.polygonm (a int, g geometry(polygonm) );
CREATE TABLE tm.polygonm0 (a int, g geometry(polygonm, 0) );
CREATE TABLE tm.polygonm1 (a int, g geometry(polygonm, 1) );
CREATE TABLE tm.polygonz (a int, g geometry(polygonz) );
CREATE TABLE tm.polygonz0 (a int, g geometry(polygonz, 0) );
CREATE TABLE tm.polygonz1 (a int, g geometry(polygonz, 1) );
CREATE TABLE tm.polygonzm (a int, g geometry(polygonzm) );
CREATE TABLE tm.polygonzm0 (a int, g geometry(polygonzm, 0) );
CREATE TABLE tm.polygonzm1 (a int, g geometry(polygonzm, 1) );

CREATE TABLE tm.polyhedralsurface (a int, g geometry(polyhedralsurface) );
CREATE TABLE tm.polyhedralsurface0 (a int, g geometry(polyhedralsurface, 0) );
CREATE TABLE tm.polyhedralsurface1 (a int, g geometry(polyhedralsurface, 1) );
CREATE TABLE tm.polyhedralsurfacem (a int, g geometry(polyhedralsurfacem) );
CREATE TABLE tm.polyhedralsurfacem0 (a int, g geometry(polyhedralsurfacem, 0) );
CREATE TABLE tm.polyhedralsurfacem1 (a int, g geometry(polyhedralsurfacem, 1) );
CREATE TABLE tm.polyhedralsurfacez (a int, g geometry(polyhedralsurfacez) );
CREATE TABLE tm.polyhedralsurfacez0 (a int, g geometry(polyhedralsurfacez, 0) );
CREATE TABLE tm.polyhedralsurfacez1 (a int, g geometry(polyhedralsurfacez, 1) );
CREATE TABLE tm.polyhedralsurfacezm (a int, g geometry(polyhedralsurfacezm) );
CREATE TABLE tm.polyhedralsurfacezm0 (a int, g geometry(polyhedralsurfacezm, 0) );
CREATE TABLE tm.polyhedralsurfacezm1 (a int, g geometry(polyhedralsurfacezm, 1) );

CREATE TABLE tm.tin (a int, g geometry(tin) );
CREATE TABLE tm.tin0 (a int, g geometry(tin, 0) );
CREATE TABLE tm.tin1 (a int, g geometry(tin, 1) );
CREATE TABLE tm.tinm (a int, g geometry(tinm) );
CREATE TABLE tm.tinm0 (a int, g geometry(tinm, 0) );
CREATE TABLE tm.tinm1 (a int, g geometry(tinm, 1) );
CREATE TABLE tm.tinz (a int, g geometry(tinz) );
CREATE TABLE tm.tinz0 (a int, g geometry(tinz, 0) );
CREATE TABLE tm.tinz1 (a int, g geometry(tinz, 1) );
CREATE TABLE tm.tinzm (a int, g geometry(tinzm) );
CREATE TABLE tm.tinzm0 (a int, g geometry(tinzm, 0) );
CREATE TABLE tm.tinzm1 (a int, g geometry(tinzm, 1) );

CREATE TABLE tm.triangle (a int, g geometry(triangle) );
CREATE TABLE tm.triangle0 (a int, g geometry(triangle, 0) );
CREATE TABLE tm.triangle1 (a int, g geometry(triangle, 1) );
CREATE TABLE tm.trianglem (a int, g geometry(trianglem) );
CREATE TABLE tm.trianglem0 (a int, g geometry(trianglem, 0) );
CREATE TABLE tm.trianglem1 (a int, g geometry(trianglem, 1) );
CREATE TABLE tm.trianglez (a int, g geometry(trianglez) );
CREATE TABLE tm.trianglez0 (a int, g geometry(trianglez, 0) );
CREATE TABLE tm.trianglez1 (a int, g geometry(trianglez, 1) );
CREATE TABLE tm.trianglezm (a int, g geometry(trianglezm) );
CREATE TABLE tm.trianglezm0 (a int, g geometry(trianglezm, 0) );
CREATE TABLE tm.trianglezm1 (a int, g geometry(trianglezm, 1) );

SELECT * from geometry_columns ORDER BY 3;

-- TODO: test that you can only insert compatible types

DROP SCHEMA tm CASCADE;

