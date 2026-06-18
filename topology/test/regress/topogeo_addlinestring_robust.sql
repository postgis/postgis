
--{
CREATE FUNCTION runTest(
  lbl text,
  lines geometry[],
  newline geometry,
  tol float8,
  debug bool default false,
  topo_prec float8 default 0,
  use_totopogeom bool default false
)
RETURNS SETOF text AS
$BODY$
DECLARE
  g geometry;
  n int := 0;
  rec record;
  drift_tol float8 := CASE
    WHEN tol = -1 THEN COALESCE(NULLIF(topo_prec, 0), topology._st_mintolerance(newline))
    ELSE COALESCE(NULLIF(tol, 0), topology._st_mintolerance(newline))
  END;
BEGIN
  IF EXISTS ( SELECT * FROM topology.topology WHERE name = 'topo' )
  THEN
    PERFORM topology.DropTopology ('topo');
  END IF;

  PERFORM topology.CreateTopology ('topo', 0, topo_prec);
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

  BEGIN
    IF use_totopogeom THEN
      PERFORM topology.toTopoGeom(newline, 'topo', 1, tol);
    ELSE
      PERFORM topology.TopoGeo_addLinestring('topo', newline, tol);
    END IF;
  EXCEPTION WHEN OTHERS THEN
    RETURN QUERY SELECT format('%s|addline exception|%s (%s)', lbl, SQLERRM, SQLSTATE);
  END;

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
  ) foo WHERE dist >= drift_tol
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

-- See https://trac.osgeo.org/postgis/ticket/5699
SELECT * FROM runTest('#5699',
  ARRAY[
    'LINESTRING(16.330791631988802 68.75635661578073, 16.332533372319826 68.75496886016562)',
    'LINESTRING(16.345890311070306 68.75803010362833, 16.33167771310482 68.75565061843871, 16.330791631988802 68.75635661578073)'
  ],
  'LINESTRING( 16.30641253121884 68.75189557630306, 16.33167771310482 68.75565061843871)',
  0
) WHERE true ;

-- See https://trac.osgeo.org/postgis/ticket/5711
SELECT * FROM runTest('#5711',
  ARRAY[
    'LINESTRING(10.330696729253674 59.869691324470494, 10.33083177446384 59.86967354547033)',
    'LINESTRING(10.33083177446384 59.86967354547033, 10.330803297725453 59.86967729449583, 10.330803407291219 59.86967760716573)'
  ],
  'LINESTRING(10.330803266175977 59.86967720446249, 10.330803297725453 59.86967729449583)',
  0
) WHERE true ;

-- See https://trac.osgeo.org/postgis/ticket/5886
-- The explicit tol=0 case keeps the historical comparison surface, while the
-- precision case below uses automatic tolerance and exercises edge-interior snap.
SELECT * FROM runTest('#5886',
  ARRAY[
    'LINESTRING(22.780107846871616 70.70515928614921, 22.779899976871615 70.7046262461492)',
    'LINESTRING(22.79217056687162 70.70247684614921, 22.779969266871618 70.70480392614921, 22.780038556871617 70.7049816061492, 22.796764346871615 70.7044482361492)'
  ],
  'LINESTRING(22.780038556871617 70.7049816061492, 22.789676156871618 70.7072799361492)',
  0
) WHERE true ;

SELECT * FROM runTest('#5886-prec',
  ARRAY[
    'LINESTRING(22.780107846871616 70.70515928614921, 22.779899976871615 70.7046262461492)',
    'LINESTRING(22.79217056687162 70.70247684614921, 22.779969266871618 70.70480392614921, 22.780038556871617 70.7049816061492, 22.796764346871615 70.7044482361492)'
  ],
  'LINESTRING(22.780038556871617 70.7049816061492, 22.789676156871618 70.7072799361492)',
  -1,
  false,
  0.0000001
) WHERE true ;

SELECT * FROM runTest('#5886-totopogeom-zero',
  ARRAY[
    'LINESTRING(22.780107846871616 70.70515928614921, 22.779899976871615 70.7046262461492)',
    'LINESTRING(22.79217056687162 70.70247684614921, 22.779969266871618 70.70480392614921, 22.780038556871617 70.7049816061492, 22.796764346871615 70.7044482361492)'
  ],
  'LINESTRING(22.780038556871617 70.7049816061492, 22.789676156871618 70.7072799361492)',
  0,
  false,
  0.0000001,
  true
) WHERE true ;

SELECT * FROM runTest('#5886-near-end',
  ARRAY[
    'LINESTRING(0 0, 10 0)'
  ],
  'LINESTRING(0.5 0.9, 0.5 -2)',
  -1,
  false,
  1
) WHERE true ;

DO $$
BEGIN
  IF EXISTS ( SELECT * FROM topology.topology WHERE name = 'topo' )
  THEN
    PERFORM topology.DropTopology('topo');
  END IF;

  PERFORM topology.CreateTopology('topo', 0, 1);
  PERFORM topology.TopoGeo_addLinestring('topo', 'LINESTRING(0 0, 10 0)', -1);
  PERFORM topology.TopoGeo_addLinestring('topo', 'LINESTRING(0.9 0.9, 0.9 2)', -1);
END;
$$;

SELECT '#5886-projection-not-endpoint',
  count(distinct n.node_id) FILTER (WHERE ST_DWithin(n.geom, 'POINT(0.9 0)'::geometry, 1e-9)),
  count(distinct e.edge_id) FILTER (
    WHERE (ST_DWithin(a.geom, 'POINT(0 0)'::geometry, 1e-9)
        AND ST_DWithin(b.geom, 'POINT(0.9 2)'::geometry, 1e-9))
       OR (ST_DWithin(a.geom, 'POINT(0.9 2)'::geometry, 1e-9)
        AND ST_DWithin(b.geom, 'POINT(0 0)'::geometry, 1e-9))
  )
FROM topo.node n
LEFT JOIN topo.edge_data e
  ON n.node_id = e.start_node OR n.node_id = e.end_node
LEFT JOIN topo.node a
  ON a.node_id = e.start_node
LEFT JOIN topo.node b
  ON b.node_id = e.end_node;

SELECT topology.DropTopology('topo');

DO $$
BEGIN
  IF EXISTS ( SELECT * FROM topology.topology WHERE name = 'topo' )
  THEN
    PERFORM topology.DropTopology('topo');
  END IF;

  PERFORM topology.CreateTopology('topo', 0, 1);
  PERFORM topology.TopoGeo_addLinestring('topo', 'LINESTRING(0 0, 10 0)', -1);
  PERFORM topology.TopoGeo_addPoint('topo', 'POINT(5 1.2)', -1);
  PERFORM topology.TopoGeo_addLinestring('topo', 'LINESTRING(5.4 0.6, 5.4 2)', -1);
END;
$$;

SELECT '#5886-isolated-node',
  count(*) FILTER (WHERE ST_Equals(geom, 'POINT(5 1.2)'::geometry) AND containing_face IS NULL),
  count(*) FILTER (WHERE ST_DWithin(geom, 'POINT(5.4 0)'::geometry, 1e-9))
FROM topo.node;

SELECT topology.DropTopology('topo');

DROP FUNCTION runTest(text, geometry[], geometry, float8, bool, float8, bool);
