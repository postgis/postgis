\set VERBOSITY terse
set client_min_messages to ERROR;

select 'create', createtopology('tt') > 0;

-- Create a line layer (will be layer 1)
CREATE TABLE tt.f_line(id serial);
SELECT 'simple_line_layer', AddTopoGeometryColumn('tt', 'tt', 'f_line','g','LINE');

INSERT INTO tt.f_line (g) VALUES
 ( toTopoGeom('LINESTRING(0 0, 10 0)'::geometry, 'tt', 1) );
INSERT INTO tt.f_line (g) VALUES
 ( toTopoGeom('LINESTRING(10 0, 30 0)'::geometry, 'tt', 1) );

-- Sane calls
SELECT id, 'start', id, ST_Length(g) FROM tt.f_line WHERE id = 1;
SELECT id, 'add',  id, ST_Length(TopoGeom_addElement(g, '{2,2}')) FROM tt.f_line WHERE id = 1;
SELECT id, 'rem',  id, ST_Length(TopoGeom_remElement(g, '{1,2}')) FROM tt.f_line WHERE id = 1;
SELECT id, 'dup',  id, ST_Length(TopoGeom_addElement(g, '{2,2}')) FROM tt.f_line WHERE id = 1;
SELECT id, 'mis',  id, ST_Length(TopoGeom_remElement(g, '{1,2}')) FROM tt.f_line WHERE id = 1;

DROP TABLE tt.f_line;
select droptopology('tt');
