select ST_astext('POINT EMPTY');
select ST_astext('POINT(EMPTY)');
select ST_astext('POINT(0 0)');
select ST_astext('POINT((0 0))');

select ST_astext('MULTIPOINT EMPTY');
select ST_astext('MULTIPOINT(EMPTY)');
-- This is supported for backward compatibility
select ST_astext('MULTIPOINT(0 0, 2 0)');
select ST_astext('MULTIPOINT((0 0), (2 0))');
select ST_astext('MULTIPOINT((0 0), (2 0), EMPTY)');

select ST_astext('LINESTRING EMPTY');
select ST_astext('LINESTRING(EMPTY)');
select ST_astext('LINESTRING(0 0, 1 1)');
select ST_astext('LINESTRING((0 0, 1 1))');
select ST_astext('LINESTRING((0 0), (1 1))');

select ST_astext('MULTILINESTRING EMPTY');
select ST_astext('MULTILINESTRING(EMPTY)');
select ST_astext('MULTILINESTRING(0 0, 2 0)');
select ST_astext('MULTILINESTRING((0 0, 2 0))');
select ST_astext('MULTILINESTRING((0 0, 2 0), (1 1, 2 2))');
select ST_astext('MULTILINESTRING((0 0, 2 0), (1 1, 2 2), EMPTY)');
select ST_astext('MULTILINESTRING((0 0, 2 0), (1 1, 2 2), (EMPTY))');

select ST_astext('POLYGON EMPTY');
select ST_astext('POLYGON(EMPTY)');
select ST_astext('POLYGON((0 0,1 0,1 1,0 1,0 0))');
select ST_astext('POLYGON((0 0,10 0,10 10,0 10,0 0),(2 2,2 5,5 5,5 2,2 2))');

select ST_astext('MULTIPOLYGON EMPTY');
select ST_astext('MULTIPOLYGON(EMPTY)');
select ST_astext('MULTIPOLYGON((EMPTY))');
select ST_astext('MULTIPOLYGON(((0 0,10 0,10 10,0 10,0 0),(2 2,2 5,5 5,5 2,2 2)))');

select ST_astext('GEOMETRYCOLLECTION EMPTY');
-- This is supported for backward compatibility
select ST_astext('GEOMETRYCOLLECTION(EMPTY)');
select ST_astext('GEOMETRYCOLLECTION((EMPTY))');
