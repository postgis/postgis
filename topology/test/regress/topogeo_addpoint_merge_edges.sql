--{
CREATE FUNCTION runTest( lbl text, lines geometry[], point geometry, prec float8, debug bool default false )
RETURNS SETOF text AS
$BODY$
DECLARE
  g geometry;
  n int := 0;
  rec record;
BEGIN
  IF EXISTS ( SELECT * FROM topology.topology WHERE name = 'topo' )
  THEN
    PERFORM topology.DropTopology ('topo');
  END IF;

  PERFORM topology.CreateTopology ('topo', 0, prec);
  CREATE TABLE topo.fl(lbl text, g geometry);
  PERFORM topology.AddTopoGeometryColumn('topo','topo','fl','tg','LINESTRING');
  CREATE TABLE topo.fa(lbl text, g geometry);
  PERFORM topology.AddTopoGeometryColumn('topo','topo','fa','tg','POLYGON');

  -- Add a polygon containing all lines
  PERFORM topology.TopoGeo_addPolygon('topo', ST_Expand(ST_Extent(geom), 100))
  FROM unnest(lines) geom;

  -- Add all lines
  FOR g IN SELECT unnest(lines)
  LOOP
    INSERT INTO topo.fl(lbl, tg) VALUES
      ( 'l'||n, topology.toTopoGeom(g, 'topo', 1) );
    n = n+1;
  END LOOP;

  FOR n IN SELECT face_id FROM topo.face WHERE face_id > 0
  LOOP
    INSERT INTO topo.fa(lbl, tg) VALUES
      ( 'a'||n, topology.CreateTopoGeom('topo', 3, 2, ARRAY[ARRAY[n,3]]) );
    n = n+1;
  END LOOP;

  UPDATE topo.fl SET g = tg::geometry;
  UPDATE topo.fa SET g = tg::geometry;

  RETURN QUERY SELECT  array_to_string(ARRAY[
    lbl,
    '-checking-'
  ], '|');

  IF debug THEN
    RETURN QUERY SELECT array_to_string(ARRAY[
      lbl, -- 'topo',
      'bfr',
      'E' || edge_id,
      'next_left:'||next_left_edge,
      'next_right:'||next_right_edge,
      'face_left:'||left_face,
      'face_right:'||right_face
    ], '|')
    FROM topo.edge
    ORDER BY edge_id;
  END IF;

  IF debug THEN
    set client_min_messages to DEBUG;
  END IF;
  PERFORM topology.TopoGeo_addPoint('topo', point);
  IF debug THEN
    set client_min_messages to WARNING;
  END IF;

  IF debug THEN
    RETURN QUERY SELECT array_to_string(ARRAY[
      lbl, --'topo',
      'aft',
      'E' || edge_id,
      'next_left:'||next_left_edge,
      'next_right:'||next_right_edge,
      'face_left:'||left_face,
      'face_right:'||right_face
    ], '|')
    FROM topo.edge
    ORDER BY edge_id;
  END IF;

  RETURN QUERY
    WITH j AS (
      SELECT
        row_number() over () as rn,
        to_json(s) as o
      FROM ValidateTopology('topo') s
    )
    SELECT
      array_to_string(
        ARRAY[lbl,'unexpected','validity issue'] ||
        array_agg(x.value order by x.ordinality),
        '|'
      )
    FROM j, json_each_text(j.o)
    WITH ordinality AS x
    GROUP by j.rn;


  RETURN QUERY SELECT array_to_string(ARRAY[
    lbl,
    'unexpected',
    'lineal drift',
    l,
    dist::text
  ], '|')
  FROM (
    SELECT t.lbl l, ST_HausdorffDistance(t.g, tg::geometry) dist
    FROM topo.fl t
  ) foo WHERE dist >= COALESCE(
      NULLIF(prec,0),
      topology._st_mintolerance(point)
  )
  ORDER BY foo.l;

  SELECT sum(ST_Area(t.g)) as before, sum(ST_Area(tg::geometry)) as after
  FROM topo.fa t
  INTO rec;

  IF rec.before != rec.after THEN
    RETURN QUERY SELECT array_to_string(ARRAY[
      lbl,
      'unexpected total area change',
      rec.before::text,
      rec.after::text
    ], '|')
    ;

    RETURN QUERY SELECT array_to_string(ARRAY[
      lbl,
      'area change',
      l,
      bfr::text,
      aft::text
    ], '|')
    FROM (
      SELECT t.lbl l, ST_Area(t.g) bfr, ST_Area(tg::geometry) aft
      FROM topo.fa t
    ) foo WHERE bfr != aft
    ORDER BY foo.l;

  END IF;

  IF NOT debug THEN
    PERFORM topology.DropTopology ('topo');
  END IF;

END;
$BODY$
LANGUAGE 'plpgsql';
--}

SET client_min_messages to WARNING;


--
-- Tests start here
--

-- Clockwise merge on start node to backward edge

SELECT * FROM runTest('sn.CW-back-merge',
  ARRAY[
    'LINESTRING(100 0, 0 0)',
    'LINESTRING(0 0,100 1,100 0)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('sn.CW-back-merge.ep-edges', ARRAY[
    'LINESTRING(100 0, 0 0)',
    'LINESTRING(0 0,100 1,100 0)',
    'LINESTRING(0 0, 50 -10, 100 0)',
    'LINESTRING(0 0, 110 10, 100 0)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('sn.CW-back-merge.ep-edges-back', ARRAY[
    'LINESTRING(100 0, 0 0)',
    'LINESTRING(0 0,100 1,100 0)',
    ST_Reverse('LINESTRING(0 0, 50 -10, 100 0)'),
    ST_Reverse('LINESTRING(0 0, 110 10, 100 0)')
  ], 'POINT(50 0)', 2
) WHERE true ;

-- Counterclockwise merge on start node to backward edge

SELECT * FROM runTest('sn.CCW-back-merge', ARRAY[
    'LINESTRING(100 0, 0 0)',
    'LINESTRING(0 0,100 -1,100 0)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('sn.CCW-back-merge.ep-edges', ARRAY[
    'LINESTRING(100 0, 0 0)',
    'LINESTRING(0 0,100 -1,100 0)',
    'LINESTRING(0 0, 50 10, 100 0)',
    'LINESTRING(0 0, 110 -10, 100 0)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('sn.CCW-back-merge.ep-edges-back', ARRAY[
    'LINESTRING(100 0, 0 0)',
    'LINESTRING(0 0,100 -1,100 0)',
    ST_Reverse('LINESTRING(0 0, 50 10, 100 0)'),
    ST_Reverse('LINESTRING(0 0, 110 -10, 100 0)')
  ], 'POINT(50 0)', 2
) WHERE true ;

-- Clockwise merge on start node to forward edge

SELECT * FROM runTest('sn.CW-forw-merge', ARRAY[
    'LINESTRING(0 0, 100 0)',
    'LINESTRING(0 0, 100 1, 100 0)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('sn.CW-forw-merge.ep-edges', ARRAY[
    'LINESTRING(0 0, 100 0)',
    'LINESTRING(0 0,100 1,100 0)',
    'LINESTRING(0 0, 50 -10, 100 0)',
    'LINESTRING(0 0, 110 10, 100 0)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('sn.CW-forw-merge.ep-edges-back', ARRAY[
    'LINESTRING(0 0, 100 0)',
    'LINESTRING(0 0,100 1,100 0)',
    ST_Reverse('LINESTRING(0 0, 50 -10, 100 0)'),
    ST_Reverse('LINESTRING(0 0, 110 10, 100 0)')
  ], 'POINT(50 0)', 2
) WHERE true ;

-- Counterclockwise merge on start node to forward edge

SELECT * FROM runTest('sn.CCW-forw-merge', ARRAY[
    'LINESTRING(0 0, 100 0)',
    'LINESTRING(0 0,100 -1,100 0)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('sn.CCW-forw-merge.ep-edges', ARRAY[
    'LINESTRING(0 0, 100 0)',
    'LINESTRING(0 0,100 -1,100 0)',
    'LINESTRING(0 0, 50 10, 100 0)',
    'LINESTRING(0 0, 110 -10, 100 0)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('sn.CCW-forw-merge.ep-edges-back', ARRAY[
    'LINESTRING(0 0, 100 0)',
    'LINESTRING(0 0,100 -1,100 0)',
    ST_Reverse('LINESTRING(0 0, 50 10, 100 0)'),
    ST_Reverse('LINESTRING(0 0, 110 -10, 100 0)')
  ], 'POINT(50 0)', 2
) WHERE true ;

-- Clockwise merge on end node to backward edge

SELECT * FROM runTest('en.CW-back-merge',
  ARRAY[
    'LINESTRING(0 0, 100 0)',
    'LINESTRING(100 0, 100 1, 0 0)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('en.CW-back-merge.ep-edges',
  ARRAY[
    'LINESTRING(0 0, 100 0)',
    'LINESTRING(100 0, 100 1, 0 0)',
    'LINESTRING(0 0, 50 -10, 100 0)',
    'LINESTRING(0 0, 110 10, 100 0)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('en.CW-back-merge.ep-edges-back',
  ARRAY[
    'LINESTRING(0 0, 100 0)',
    'LINESTRING(100 0, 100 1, 0 0)',
    ST_Reverse('LINESTRING(0 0, 50 -10, 100 0)'),
    ST_Reverse('LINESTRING(0 0, 110 10, 100 0)')
  ], 'POINT(50 0)', 2
) WHERE true ;

-- Clockwise merge on end node to forward edge

SELECT * FROM runTest('en.CW-forw-merge',
  ARRAY[
    'LINESTRING(100 0, 0 0)',
    'LINESTRING(100 0, 100 1, 0 0)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('en.CW-forw-merge.ep-edges',
  ARRAY[
    'LINESTRING(100 0, 0 0)',
    'LINESTRING(100 0, 100 1, 0 0)',
    'LINESTRING(0 0, 50 -10, 100 0)',
    'LINESTRING(0 0, 110 10, 100 0)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('en.CW-forw-merge.ep-edges-back',
  ARRAY[
    'LINESTRING(100 0, 0 0)',
    'LINESTRING(100 0, 100 1, 0 0)',
    ST_Reverse('LINESTRING(0 0, 50 -10, 100 0)'),
    ST_Reverse('LINESTRING(0 0, 110 10, 100 0)')
  ], 'POINT(50 0)', 2
) WHERE true ;

-- No merge, two new edges formed by snapping the existing one

SELECT * FROM runTest('snap-CW',
  ARRAY[
    'LINESTRING(0 1, 100 1)',
    'LINESTRING(0 -10,50 0,100 -10)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('snap-CW-newface-sp',
  ARRAY[
    'LINESTRING(0 1, 100 1)',
    'LINESTRING(0 -10,50 0,100 -10)',
    'LINESTRING(0 1, 0 -10)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('snap-CW-newface-ep',
  ARRAY[
    'LINESTRING(0 1, 100 1)',
    'LINESTRING(0 -10,50 0,100 -10)',
    'LINESTRING(100 1, 100 -10)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('snap-CW-split',
  ARRAY[
    'LINESTRING(0 1, 100 1)',
    'LINESTRING(0 -10,50 0,100 -10)',
    'LINESTRING(0 1, 0 -10)',
    'LINESTRING(100 1, 100 -10)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('snap-CCW',
  ARRAY[
    'LINESTRING(0 -1, 100 -1)',
    'LINESTRING(0 10,50 0,100 10)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('snap-CCW-newface-sp',
  ARRAY[
    'LINESTRING(0 -1, 100 -1)',
    'LINESTRING(0 10,50 0,100 10)',
    'LINESTRING(0 -1, 0 10)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('snap-CCW-newface-ep',
  ARRAY[
    'LINESTRING(0 -1, 100 -1)',
    'LINESTRING(0 10,50 0,100 10)',
    'LINESTRING(100 -1, 100 10)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('snap-CCW-split',
  ARRAY[
    'LINESTRING(0 -1, 100 -1)',
    'LINESTRING(0 10,50 0,100 10)',
    'LINESTRING(0 -1, 0 10)',
    'LINESTRING(100 -1, 100 10)'
  ], 'POINT(50 0)', 2
) WHERE true ;

-- Merges both sides to existing edge

SELECT * FROM runTest('full-merge',
  ARRAY[
    'LINESTRING(0 0, 100 0)',
    'LINESTRING(0 0,50 1,100 0)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('full-merge-exterior-face',
  ARRAY[
    'LINESTRING(0 0, 100 0)',
    'LINESTRING(0 0,50 1,100 0)',
    'LINESTRING(0 0,50 100,100 0)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('double-merge-forward-backward',
  ARRAY[
    'LINESTRING(0 0, 100 0)',
    'LINESTRING(0 0, 100 0.6)',
    ST_Reverse('LINESTRING(0 0, 100 -0.4)')
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('multi-merge-forward-backward',
  ARRAY[
    'LINESTRING(0 0, 100 0)',
    'LINESTRING(0 0, 100 0.2)',
    'LINESTRING(0 0, 100 0.4)',
    ST_Reverse('LINESTRING(0 0, 100 0.6)'),
    'LINESTRING(0 0, 100 0.8)',
    'LINESTRING(0 0, 100 1)',
    'LINESTRING(0 0, 100 -0.2)',
    ST_Reverse('LINESTRING(0 0, 100 -0.4)'),
    'LINESTRING(0 0, 100 -0.6)',
    'LINESTRING(0 0, 100 -0.8)',
    'LINESTRING(0 0, 100 -1)',
    'LINESTRING(100 -1, 100 1)'
  ], 'POINT(50 0)', 2
) WHERE true ;

SELECT * FROM runTest('multi-merge-closest-not-containing-projected',
  ARRAY[
    'LINESTRING(16.293250027137326 90.5156789741379, 12.472410859513678 70.77020397757295)',
    'LINESTRING(12.472410859513678 70.77020397757295, 15.961698572988352 90.56891057892607, 12.386215634911599 70.78492225080777)'
  ], 'POINT(12.472410859513998 70.77020397757477)', 0
) WHERE true ;

SELECT * FROM runTest('#5792.0',
  ARRAY[
    'LINESTRING(11.812075769533624 59.77938755222866,11.811862389533625 59.77938237222866)',
    'LINESTRING(11.811862389533625 59.77938237222866,11.811969079533624 59.77938496222866,11.812075769533624 59.77938755222866)'
  ], 'POINT(11.812029186127067 59.7793864213727)', 0
) WHERE true ;

SELECT * FROM runTest('#5792.1',
  ARRAY[
    'LINESTRING(11.812075769533624 59.77938755222866,11.811862389533625 59.77938237222866)',
    'LINESTRING(11.811862389533625 59.77938237222866,11.811969079533624 59.77938496222866,11.812075769533624 59.77938755222866)',
    'LINESTRING(11.811862389533625 59.77938237222866,11.811969079533624 59.77938,11.812075769533624 59.77938755222866)'
  ], 'POINT(11.812029186127067 59.7793864213727)', 0
) WHERE true ;

-- See https://trac.osgeo.org/postgis/ticket/5862
SELECT * FROM runTest('#5862',
  ARRAY[
    'LINESTRING(22.780107846871616 70.70515928614921, 22.779899976871615 70.7046262461492)',
    'LINESTRING(22.792170566871620 70.70247684614921, 22.779969266871618 70.70480392614921, 22.780038556871617 70.7049816061492, 22.796764346871615 70.7044482361492)'
  ], 'POINT(22.780038556871617 70.7049816061492)', 0
) WHERE true ;

DROP FUNCTION runTest(text, geometry[], geometry, float8, bool);
