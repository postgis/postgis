select astext('POINT EMPTY');
select astext('POINT(EMPTY)');
select astext('POINT(0 0)');
select astext('POINT((0 0))');

select astext('MULTIPOINT EMPTY');
select astext('MULTIPOINT(EMPTY)');
-- This is supported for backward compatibility
select astext('MULTIPOINT(0 0, 2 0)');
select astext('MULTIPOINT((0 0), (2 0))');
select astext('MULTIPOINT((0 0), (2 0), EMPTY)');

select astext('LINESTRING EMPTY');
select astext('LINESTRING(EMPTY)');
select astext('LINESTRING(0 0, 1 1)');
select astext('LINESTRING((0 0, 1 1))');
select astext('LINESTRING((0 0), (1 1))');

select astext('MULTILINESTRING EMPTY');
select astext('MULTILINESTRING(EMPTY)');
select astext('MULTILINESTRING(0 0, 2 0)');
select astext('MULTILINESTRING((0 0, 2 0))');
select astext('MULTILINESTRING((0 0, 2 0), (1 1, 2 2))');
select astext('MULTILINESTRING((0 0, 2 0), (1 1, 2 2), EMPTY)');
select astext('MULTILINESTRING((0 0, 2 0), (1 1, 2 2), (EMPTY))');

select astext('POLYGON EMPTY');
select astext('POLYGON(EMPTY)');
select astext('POLYGON((0 0,1 0,1 1,0 1,0 0))');
select astext('POLYGON((0 0,10 0,10 10,0 10,0 0),(2 2,2 5,5 5,5 2,2 2))');

select astext('MULTIPOLYGON EMPTY');
select astext('MULTIPOLYGON(EMPTY)');
select astext('MULTIPOLYGON((EMPTY))');
select astext('MULTIPOLYGON(((0 0,10 0,10 10,0 10,0 0),(2 2,2 5,5 5,5 2,2 2)))');

select astext('GEOMETRYCOLLECTION EMPTY');
-- This is supported for backward compatibility
select astext('GEOMETRYCOLLECTION(EMPTY)');
select astext('GEOMETRYCOLLECTION((EMPTY))');
