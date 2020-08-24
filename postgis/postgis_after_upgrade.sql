-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
--
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.net
--
-- Copyright (C) 2011-2020 Sandro Santilli <strk@kbt.io>
-- Copyright (C) 2010-2012 Regina Obe <lr@pcorp.us>
-- Copyright (C) 2009      Paul Ramsey <pramsey@cleverelephant.ca>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- This file contains drop commands for obsoleted items that need
-- to be dropped _after_ upgrade of old functions.
-- Changes to this file affect postgis_upgrade*.sql script.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

-- First drop old aggregates
DROP AGGREGATE IF EXISTS memgeomunion(geometry);
DROP AGGREGATE IF EXISTS geomunion(geometry);
DROP AGGREGATE IF EXISTS polygonize(geometry); -- Deprecated in 1.2.3, Dropped in 2.0.0
DROP AGGREGATE IF EXISTS collect(geometry); -- Deprecated in 1.2.3, Dropped in 2.0.0
DROP AGGREGATE IF EXISTS st_geomunion(geometry);
DROP AGGREGATE IF EXISTS accum_old(geometry);
DROP AGGREGATE IF EXISTS st_accum_old(geometry);
DROP AGGREGATE IF EXISTS st_accum(geometry); -- Dropped in 3.0.0
DROP FUNCTION IF EXISTS pgis_geometry_accum_finalfn(internal);

DROP AGGREGATE IF EXISTS st_astwkb_agg(geometry, integer); -- temporarely introduced before 2.2.0 final
DROP AGGREGATE IF EXISTS st_astwkb_agg(geometry, integer, bigint); -- temporarely introduced before 2.2.0 final
DROP AGGREGATE IF EXISTS st_astwkbagg(geometry, integer); -- temporarely introduced before 2.2.0 final
DROP AGGREGATE IF EXISTS st_astwkbagg(geometry, integer, bigint); -- temporarely introduced before 2.2.0 final
DROP AGGREGATE IF EXISTS st_astwkbagg(geometry, integer, bigint, boolean); -- temporarely introduced before 2.2.0 final
DROP AGGREGATE IF EXISTS st_astwkbagg(geometry, integer, bigint, boolean, boolean); -- temporarely introduced before 2.2.0 final

-- BEGIN Management functions that now have default param for typmod --
DROP FUNCTION IF EXISTS AddGeometryColumn(varchar, varchar, varchar, varchar, integer, varchar, integer);
DROP FUNCTION IF EXISTS AddGeometryColumn(varchar, varchar, varchar, integer, varchar, integer);
DROP FUNCTION IF EXISTS AddGeometryColumn(varchar, varchar, integer, varchar, integer);
DROP FUNCTION IF EXISTS populate_geometry_columns();
DROP FUNCTION IF EXISTS populate_geometry_columns(oid);

-- END Management functions now have default parameter for typmod --
-- Then drop old functions
DROP FUNCTION IF EXISTS box2d_overleft(box2d, box2d);
DROP FUNCTION IF EXISTS box2d_overright(box2d, box2d);
DROP FUNCTION IF EXISTS box2d_left(box2d, box2d);
DROP FUNCTION IF EXISTS box2d_right(box2d, box2d);
DROP FUNCTION IF EXISTS box2d_contain(box2d, box2d);
DROP FUNCTION IF EXISTS box2d_contained(box2d, box2d);
DROP FUNCTION IF EXISTS box2d_overlap(box2d, box2d);
DROP FUNCTION IF EXISTS box2d_same(box2d, box2d);
DROP FUNCTION IF EXISTS box2d_intersects(box2d, box2d);
DROP FUNCTION IF EXISTS st_area(geography); -- this one changed to use default parameters
DROP FUNCTION IF EXISTS ST_AsGeoJson(geometry); -- this one changed to use default args
DROP FUNCTION IF EXISTS ST_AsGeoJson(geography); -- this one changed to use default args
DROP FUNCTION IF EXISTS ST_AsGeoJson(geometry, int4); -- this one changed to use default args
DROP FUNCTION IF EXISTS ST_AsGeoJson(geography, int4); -- this one changed to use default args
DROP FUNCTION IF EXISTS ST_AsGeoJson(int4, geometry); -- this one changed to use default args
DROP FUNCTION IF EXISTS ST_AsGeoJson(int4, geography); -- this one changed to use default args
DROP FUNCTION IF EXISTS ST_AsGeoJson(int4, geometry, int4); -- this one changed to use default args
DROP FUNCTION IF EXISTS ST_AsGeoJson(int4, geography, int4); -- this one changed to use default args
DROP FUNCTION IF EXISTS ST_AsGeoJson(int4, geography, int4, int4); -- dropped because the version-first signature is dumb
DROP FUNCTION IF EXISTS _ST_AsGeoJson(int4, geometry, int4, int4); -- dropped in PostGIS-3.0 (r17300)
DROP FUNCTION IF EXISTS _ST_AsGeoJson(int4, geography, int4, int4); -- dropped in PostGIS-3.0 (r17300)
DROP FUNCTION IF EXISTS st_asgml(geometry); -- changed to use default args
DROP FUNCTION IF EXISTS st_asgml(geometry, int4);  -- changed to use default args
DROP FUNCTION IF EXISTS st_asgml(int4, geometry);  -- changed to use default args
DROP FUNCTION IF EXISTS st_asgml(int4, geometry, int4);  -- changed to use default args
DROP FUNCTION IF EXISTS st_asgml(int4, geometry, int4, int4);  -- changed to use default args
DROP FUNCTION IF EXISTS st_asgml(int4, geometry, int4, int4, text); -- changed to use default args
DROP FUNCTION IF EXISTS st_asgml(geography); -- changed to use default args
DROP FUNCTION IF EXISTS st_asgml(geography, int4);  -- changed to use default args
DROP FUNCTION IF EXISTS st_asgml(int4, geography);  -- changed to use default args
DROP FUNCTION IF EXISTS st_asgml(int4, geography, int4);  -- changed to use default args
DROP FUNCTION IF EXISTS st_asgml(int4, geography, int4, int4);  -- changed to use default args
DROP FUNCTION IF EXISTS st_asgml(int4, geography, int4, int4, text); -- changed to use default args
DROP FUNCTION IF EXISTS _st_asgml(int4, geometry, int4, int4, text); -- changed to use default args
DROP FUNCTION IF EXISTS _st_asgml(int4, geography, int4, int4, text); -- changed to use default args
DROP FUNCTION IF EXISTS _st_asgml(int4, geography, int4, int4, text, text); -- changed to use default args
DROP FUNCTION IF EXISTS st_asgml(geography, int4, int4);
DROP FUNCTION IF EXISTS _st_askml(int4, geography, int4, text); -- dropped in PostGIS-3.0 (r17300)
DROP FUNCTION IF EXISTS _st_askml(int4, geometry, int4, text); -- dropped in PostGIS-3.0 (r17300)
DROP FUNCTION IF EXISTS st_askml(geometry); -- changed to use default args
DROP FUNCTION IF EXISTS st_askml(geography); -- changed to use default args
DROP FUNCTION IF EXISTS st_askml(int4, geometry, int4); -- changed to use default args
DROP FUNCTION IF EXISTS st_askml(int4, geography, int4); -- changed to use default args
DROP FUNCTION IF EXISTS st_askml(int4, geography, int4, text); -- dropped because the version-first signature is dumb

DROP FUNCTION IF EXISTS st_asx3d(geometry); -- this one changed to use default parameters so full function deals with it
DROP FUNCTION IF EXISTS st_asx3d(geometry, int4); -- introduce variant with opts so get rid of other without ops
DROP FUNCTION IF EXISTS st_assvg(geometry); -- changed to use default args
DROP FUNCTION IF EXISTS st_assvg(geometry, int4); -- changed to use default args
DROP FUNCTION IF EXISTS st_assvg(geography); -- changed to use default args
DROP FUNCTION IF EXISTS st_assvg(geography, int4); -- changed to use default args
DROP FUNCTION IF EXISTS st_box2d_overleft(box2d, box2d);
DROP FUNCTION IF EXISTS st_box2d_overright(box2d, box2d);
DROP FUNCTION IF EXISTS st_box2d_left(box2d, box2d);
DROP FUNCTION IF EXISTS st_box2d_right(box2d, box2d);
DROP FUNCTION IF EXISTS st_box2d_contain(box2d, box2d);
DROP FUNCTION IF EXISTS st_box2d_contained(box2d, box2d);
DROP FUNCTION IF EXISTS st_box2d_overlap(box2d, box2d);
DROP FUNCTION IF EXISTS st_box2d_same(box2d, box2d);
DROP FUNCTION IF EXISTS st_box2d_intersects(box2d, box2d);
DROP FUNCTION IF EXISTS st_box2d_in(cstring);
DROP FUNCTION IF EXISTS st_box2d_out(box2d);
DROP FUNCTION IF EXISTS st_box2d(geometry);
DROP FUNCTION IF EXISTS st_box2d(box3d);
DROP FUNCTION IF EXISTS st_box3d(box2d);
DROP FUNCTION IF EXISTS st_box(box3d);
DROP FUNCTION IF EXISTS st_box3d(geometry);
DROP FUNCTION IF EXISTS st_box(geometry);
DROP FUNCTION IF EXISTS _st_buffer(geometry, float8, cstring); -- dropped in PostGIS-3.0 (r17300)
DROP FUNCTION IF EXISTS ST_ConcaveHull(geometry,float); -- this one changed to use default parameters
DROP FUNCTION IF EXISTS ST_DWithin(geography, geography, float8); -- this one changed to use default parameters
DROP FUNCTION IF EXISTS st_text(geometry);
DROP FUNCTION IF EXISTS st_geometry(box2d);
DROP FUNCTION IF EXISTS st_geometry(box3d);
DROP FUNCTION IF EXISTS st_geometry(text);
DROP FUNCTION IF EXISTS st_geometry(bytea);
DROP FUNCTION IF EXISTS st_bytea(geometry);
DROP FUNCTION IF EXISTS st_addbbox(geometry);
DROP FUNCTION IF EXISTS _st_distance(geography, geography, float8, boolean); -- dropped in PostGIS-3.0 (r17300)
DROP FUNCTION IF EXISTS st_dropbbox(geometry);
DROP FUNCTION IF EXISTS st_hasbbox(geometry);
DROP FUNCTION IF EXISTS cache_bbox();
DROP FUNCTION IF EXISTS st_cache_bbox();
DROP FUNCTION IF EXISTS ST_GeoHash(geometry); -- changed to use default args
DROP FUNCTION IF EXISTS st_length(geography); -- this one changed to use default parameters
DROP FUNCTION IF EXISTS st_perimeter(geography); -- this one changed to use default parameters
DROP FUNCTION IF EXISTS transform_geometry(geometry, text, text, int);
DROP FUNCTION IF EXISTS collector(geometry, geometry);
DROP FUNCTION IF EXISTS st_collector(geometry, geometry);
DROP FUNCTION IF EXISTS geom_accum (geometry[],geometry);
DROP FUNCTION IF EXISTS st_geom_accum (geometry[],geometry);
DROP FUNCTION IF EXISTS collect_garray (geometry[]);
DROP FUNCTION IF EXISTS st_collect_garray (geometry[]);
DROP FUNCTION IF EXISTS geosnoop(geometry);
DROP FUNCTION IF EXISTS jtsnoop(geometry);
DROP FUNCTION IF EXISTS st_noop(geometry);
DROP FUNCTION IF EXISTS st_max_distance(geometry, geometry);
DROP FUNCTION IF EXISTS  ST_MinimumBoundingCircle(geometry); --changed to use default parameters
-- Drop internals that should never have existed --
DROP FUNCTION IF EXISTS st_geometry_analyze(internal);
DROP FUNCTION IF EXISTS st_geometry_in(cstring);
DROP FUNCTION IF EXISTS st_geometry_out(geometry);
DROP FUNCTION IF EXISTS st_geometry_recv(internal);
DROP FUNCTION IF EXISTS st_geometry_send(geometry);
DROP FUNCTION IF EXISTS st_spheroid_in(cstring);
DROP FUNCTION IF EXISTS st_spheroid_out(spheroid);
DROP FUNCTION IF EXISTS st_geometry_lt(geometry, geometry);
DROP FUNCTION IF EXISTS st_geometry_gt(geometry, geometry);
DROP FUNCTION IF EXISTS st_geometry_ge(geometry, geometry);
DROP FUNCTION IF EXISTS st_geometry_eq(geometry, geometry);
DROP FUNCTION IF EXISTS st_geometry_cmp(geometry, geometry);
DROP FUNCTION IF EXISTS SnapToGrid(geometry, float8, float8);
DROP FUNCTION IF EXISTS st_removerepeatedpoints(geometry);
DROP FUNCTION IF EXISTS st_voronoi(geometry, geometry, double precision, boolean); --temporarely introduced before 2.3.0 final

DROP FUNCTION IF EXISTS geometry_gist_sel_2d (internal, oid, internal, int4);
DROP FUNCTION IF EXISTS geometry_gist_joinsel_2d(internal, oid, internal, smallint);
DROP FUNCTION IF EXISTS geography_gist_selectivity (internal, oid, internal, int4);
DROP FUNCTION IF EXISTS geography_gist_join_selectivity(internal, oid, internal, smallint);

DROP FUNCTION IF EXISTS ST_AsBinary(text); -- deprecated in 2.0
DROP FUNCTION IF EXISTS postgis_uses_stats(); -- deprecated in 2.0
DROP FUNCTION IF EXISTS ST_GeneratePoints(geometry, numeric); -- numeric -> integer

-- Old accum aggregate support type, removed in 2.5.0
-- See #4035
DROP TYPE IF EXISTS pgis_abs CASCADE;

DROP FUNCTION IF EXISTS st_astwkb(geometry, integer, bigint, bool, bool); -- temporarely introduced before 2.2.0 final
DROP FUNCTION IF EXISTS pgis_twkb_accum_transfn(internal, geometry, integer); -- temporarely introduced before 2.2.0 final
DROP FUNCTION IF EXISTS pgis_twkb_accum_transfn(internal, geometry, integer, bigint); -- temporarely introduced before 2.2.0 final
DROP FUNCTION IF EXISTS pgis_twkb_accum_transfn(internal, geometry, integer, bigint, bool); -- temporarely introduced before 2.2.0 final
DROP FUNCTION IF EXISTS pgis_twkb_accum_transfn(internal, geometry, integer, bigint, bool, bool); -- temporarely introduced before 2.2.0 final
DROP FUNCTION IF EXISTS pgis_twkb_accum_finalfn(internal); -- temporarely introduced before 2.2.0 final

DROP FUNCTION IF EXISTS st_seteffectivearea(geometry, double precision); -- temporarely introduced before 2.2.0 final

DROP FUNCTION IF EXISTS geometry_distance_box_nd(geometry, geometry); -- temporarely introduced before 2.2.0 final

DROP FUNCTION IF EXISTS _ST_DumpPoints(geometry, integer[]); -- removed 2.4.0, but really should have been removed 2.1.0 when ST_DumpPoints got reimpmented in C

-- Temporary clean-up while we wait to return these to action in dev
DROP FUNCTION IF EXISTS _ST_DistanceRectTree(g1 geometry, g2 geometry);
DROP FUNCTION IF EXISTS _ST_DistanceRectTreeCached(g1 geometry, g2 geometry);

-- Deplicative signatures removed
DROP FUNCTION IF EXISTS ST_Distance(geography, geography);
DROP FUNCTION IF EXISTS ST_Distance(geography, geography, float8, boolean);
DROP FUNCTION IF EXISTS ST_Buffer(geometry, float8, cstring);
DROP FUNCTION IF EXISTS ST_IsValidDetail(geometry);
DROP FUNCTION IF EXISTS ST_AsKML(int4, geometry, int4, text);
DROP FUNCTION IF EXISTS ST_AsKML(geometry, int4);
DROP FUNCTION IF EXISTS ST_AsGeoJson(int4, geometry, int4, int4);
DROP FUNCTION IF EXISTS _ST_AsGeoJson(int4, geometry, int4, int4);

-- Underscore_signatures removed for CamelCase
DROP FUNCTION IF EXISTS st_shift_longitude(geometry);
DROP FUNCTION IF EXISTS st_estimated_extent(text,text,text);
DROP FUNCTION IF EXISTS st_estimated_extent(text,text);
DROP FUNCTION IF EXISTS st_find_extent(text,text,text);
DROP FUNCTION IF EXISTS st_find_extent(text,text);
DROP FUNCTION IF EXISTS st_mem_size(geometry);
DROP FUNCTION IF EXISTS st_3dlength_spheroid(geometry, spheroid);
DROP FUNCTION IF EXISTS st_length_spheroid(geometry, spheroid);
DROP FUNCTION IF EXISTS st_length2d_spheroid(geometry, spheroid);
DROP FUNCTION IF EXISTS st_distance_spheroid(geometry, geometry, spheroid);
DROP FUNCTION IF EXISTS st_point_inside_circle(geometry, float8, float8, float8);
DROP FUNCTION IF EXISTS st_force_2d(geometry);
DROP FUNCTION IF EXISTS st_force_3dz(geometry);
DROP FUNCTION IF EXISTS st_force_3dm(geometry);
DROP FUNCTION IF EXISTS st_force_collection(geometry);
DROP FUNCTION IF EXISTS st_force_4d(geometry);
DROP FUNCTION IF EXISTS st_force_3d(geometry);
DROP FUNCTION IF EXISTS st_line_interpolate_point(geometry, float8);
DROP FUNCTION IF EXISTS st_line_substring(geometry, float8, float8);
DROP FUNCTION IF EXISTS st_line_locate_point(geometry, geometry);
DROP FUNCTION IF EXISTS st_locate_between_measures(geometry, float8, float8);
DROP FUNCTION IF EXISTS st_locate_along_measure(geometry, float8);
DROP FUNCTION IF EXISTS st_combine_bbox(box3d, geometry);
DROP FUNCTION IF EXISTS st_combine_bbox(box2d, geometry);
DROP FUNCTION IF EXISTS st_distance_sphere(geometry, geometry);

-- dev function 3.0 cycle
DROP FUNCTION IF EXISTS pgis_geometry_union_transfn(internal, geometry);

-- #4394
update pg_operator set oprcanhash = true, oprcanmerge = true where oprname = '=' and oprcode = 'geometry_eq'::regproc;


DO language 'plpgsql'
$$
BEGIN
IF _postgis_scripts_pgsql_version()::integer >= 96 THEN
-- mark ST_Union agg as parallel safe if it is not already
        BEGIN
            UPDATE pg_proc SET proparallel = 's'
            WHERE oid = 'st_union(geometry)'::regprocedure AND proparallel = 'u';
        EXCEPTION WHEN OTHERS THEN
            RAISE DEBUG 'Could not update st_union(geometry): %', SQLERRM;
        END;
END IF;
END;
$$;
