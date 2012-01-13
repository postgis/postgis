SET client_min_messages TO warning;

CREATE SCHEMA tm;

-- Test construction of typed tables

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

SELECT * from geometry_columns ORDER BY 3;

-- TODO: test that you can only insert compatible types

DROP SCHEMA tm CASCADE;

