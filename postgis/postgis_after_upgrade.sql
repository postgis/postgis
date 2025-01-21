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
SELECT _postgis_drop_function_by_signature('pgis_geometry_accum_finalfn(internal)');

DROP AGGREGATE IF EXISTS st_astwkb_agg(geometry, integer); -- temporarily introduced before 2.2.0 final
DROP AGGREGATE IF EXISTS st_astwkb_agg(geometry, integer, bigint); -- temporarily introduced before 2.2.0 final
DROP AGGREGATE IF EXISTS st_astwkbagg(geometry, integer); -- temporarily introduced before 2.2.0 final
DROP AGGREGATE IF EXISTS st_astwkbagg(geometry, integer, bigint); -- temporarily introduced before 2.2.0 final
DROP AGGREGATE IF EXISTS st_astwkbagg(geometry, integer, bigint, boolean); -- temporarily introduced before 2.2.0 final
DROP AGGREGATE IF EXISTS st_astwkbagg(geometry, integer, bigint, boolean, boolean); -- temporarily introduced before 2.2.0 final

-- BEGIN Management functions that now have default param for typmod --
SELECT _postgis_drop_function_by_signature('AddGeometryColumn(varchar, varchar, varchar, varchar, integer, varchar, integer)');
SELECT _postgis_drop_function_by_signature('AddGeometryColumn(varchar, varchar, varchar, integer, varchar, integer)');
SELECT _postgis_drop_function_by_signature('AddGeometryColumn(varchar, varchar, integer, varchar, integer)');
SELECT _postgis_drop_function_by_signature('populate_geometry_columns()');
SELECT _postgis_drop_function_by_signature('populate_geometry_columns(oid)');

-- END Management functions now have default parameter for typmod --
-- Then drop old functions
SELECT _postgis_drop_function_by_signature('box2d_overleft(box2d, box2d)');
SELECT _postgis_drop_function_by_signature('box2d_overright(box2d, box2d)');
SELECT _postgis_drop_function_by_signature('box2d_left(box2d, box2d)');
SELECT _postgis_drop_function_by_signature('box2d_right(box2d, box2d)');
SELECT _postgis_drop_function_by_signature('box2d_contain(box2d, box2d)');
SELECT _postgis_drop_function_by_signature('box2d_contained(box2d, box2d)');
SELECT _postgis_drop_function_by_signature('box2d_overlap(box2d, box2d)');
SELECT _postgis_drop_function_by_signature('box2d_same(box2d, box2d)');
SELECT _postgis_drop_function_by_signature('box2d_intersects(box2d, box2d)');
SELECT _postgis_drop_function_by_signature('st_area(geography)'); -- this one changed to use default parameters
SELECT _postgis_drop_function_by_signature('ST_AsGeoJson(geometry)'); -- this one changed to use default args
SELECT _postgis_drop_function_by_signature('ST_AsGeoJson(geography)'); -- this one changed to use default args
SELECT _postgis_drop_function_by_signature('ST_AsGeoJson(geometry, int4)'); -- this one changed to use default args
SELECT _postgis_drop_function_by_signature('ST_AsGeoJson(geography, int4)'); -- this one changed to use default args
SELECT _postgis_drop_function_by_signature('ST_AsGeoJson(int4, geometry)'); -- this one changed to use default args
SELECT _postgis_drop_function_by_signature('ST_AsGeoJson(int4, geography)'); -- this one changed to use default args
SELECT _postgis_drop_function_by_signature('ST_AsGeoJson(int4, geometry, int4)'); -- this one changed to use default args
SELECT _postgis_drop_function_by_signature('ST_AsGeoJson(int4, geography, int4)'); -- this one changed to use default args
SELECT _postgis_drop_function_by_signature('ST_AsGeoJson(int4, geography, int4, int4)', '3.0.0'); -- dropped because the version-first signature is dumb
SELECT _postgis_drop_function_by_signature('ST_AsGeoJson(int4, geometry, int4, int4)', '3.0.0'); -- dropped because the version-first signature is dumb
SELECT _postgis_drop_function_by_signature('_ST_AsGeoJson(int4, geometry, int4, int4)'); -- dropped in PostGIS-3.0 (r17300)
SELECT _postgis_drop_function_by_signature('_ST_AsGeoJson(int4, geography, int4, int4)'); -- dropped in PostGIS-3.0 (r17300)
SELECT _postgis_drop_function_by_signature('st_asgml(geometry)'); -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_asgml(geometry, int4)');  -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_asgml(int4, geometry)');  -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_asgml(int4, geometry, int4)');  -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_asgml(int4, geometry, int4, int4)');  -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_asgml(int4, geometry, int4, int4, text)'); -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_asgml(geography)'); -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_asgml(geography, int4)');  -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_asgml(int4, geography)');  -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_asgml(int4, geography, int4)');  -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_asgml(int4, geography, int4, int4)');  -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_asgml(int4, geography, int4, int4, text)'); -- changed to use default args
SELECT _postgis_drop_function_by_signature('_st_asgml(int4, geometry, int4, int4, text)'); -- changed to use default args
SELECT _postgis_drop_function_by_signature('_st_asgml(int4, geography, int4, int4, text)'); -- changed to use default args
SELECT _postgis_drop_function_by_signature('_st_asgml(int4, geography, int4, int4, text, text)'); -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_asgml(geography, int4, int4)');
SELECT _postgis_drop_function_by_signature('_st_askml(int4, geography, int4, text)'); -- dropped in PostGIS-3.0 (r17300)
SELECT _postgis_drop_function_by_signature('_st_askml(int4, geometry, int4, text)'); -- dropped in PostGIS-3.0 (r17300)
SELECT _postgis_drop_function_by_signature('st_askml(geometry)'); -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_askml(geography)'); -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_askml(int4, geometry, int4)'); -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_askml(int4, geography, int4)'); -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_askml(int4, geometry, int4, text)', '3.0.0'); -- dropped because the version-first signature is dumb
SELECT _postgis_drop_function_by_signature('st_askml(int4, geography, int4, text)', '3.0.0'); -- dropped because the version-first signature is dumb

SELECT _postgis_drop_function_by_signature('st_asx3d(geometry)'); -- this one changed to use default parameters so full function deals with it
SELECT _postgis_drop_function_by_signature('st_asx3d(geometry, int4)'); -- introduce variant with opts so get rid of other without ops
SELECT _postgis_drop_function_by_signature('st_assvg(geometry)'); -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_assvg(geometry, int4)'); -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_assvg(geography)'); -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_assvg(geography, int4)'); -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_box2d_overleft(box2d, box2d)');
SELECT _postgis_drop_function_by_signature('st_box2d_overright(box2d, box2d)');
SELECT _postgis_drop_function_by_signature('st_box2d_left(box2d, box2d)');
SELECT _postgis_drop_function_by_signature('st_box2d_right(box2d, box2d)');
SELECT _postgis_drop_function_by_signature('st_box2d_contain(box2d, box2d)');
SELECT _postgis_drop_function_by_signature('st_box2d_contained(box2d, box2d)');
SELECT _postgis_drop_function_by_signature('st_box2d_overlap(box2d, box2d)');
SELECT _postgis_drop_function_by_signature('st_box2d_same(box2d, box2d)');
SELECT _postgis_drop_function_by_signature('st_box2d_intersects(box2d, box2d)');
SELECT _postgis_drop_function_by_signature('st_box2d_in(cstring)');
SELECT _postgis_drop_function_by_signature('st_box2d_out(box2d)');
SELECT _postgis_drop_function_by_signature('st_box2d(geometry)');
SELECT _postgis_drop_function_by_signature('st_box2d(box3d)');
SELECT _postgis_drop_function_by_signature('st_box3d(box2d)');
SELECT _postgis_drop_function_by_signature('st_box(box3d)');
SELECT _postgis_drop_function_by_signature('st_box3d(geometry)');
SELECT _postgis_drop_function_by_signature('st_box(geometry)');
SELECT _postgis_drop_function_by_signature('_st_buffer(geometry, float8, cstring)'); -- dropped in PostGIS-3.0 (r17300)
SELECT _postgis_drop_function_by_signature('ST_ConcaveHull(geometry,float)'); -- this one changed to use default parameters
SELECT _postgis_drop_function_by_signature('st_text(geometry)');
SELECT _postgis_drop_function_by_signature('st_geometry(box2d)');
SELECT _postgis_drop_function_by_signature('st_geometry(box3d)');
SELECT _postgis_drop_function_by_signature('st_geometry(text)');
SELECT _postgis_drop_function_by_signature('st_geometry(bytea)');
SELECT _postgis_drop_function_by_signature('st_bytea(geometry)');
SELECT _postgis_drop_function_by_signature('st_addbbox(geometry)');
SELECT _postgis_drop_function_by_signature('_st_distance(geography, geography, float8, boolean)'); -- dropped in PostGIS-3.0 (r17300)
SELECT _postgis_drop_function_by_signature('st_dropbbox(geometry)');
SELECT _postgis_drop_function_by_signature('st_hasbbox(geometry)');
SELECT _postgis_drop_function_by_signature('cache_bbox()');
SELECT _postgis_drop_function_by_signature('st_cache_bbox()');
SELECT _postgis_drop_function_by_signature('ST_GeoHash(geometry)'); -- changed to use default args
SELECT _postgis_drop_function_by_signature('st_length(geography)'); -- this one changed to use default parameters
SELECT _postgis_drop_function_by_signature('st_perimeter(geography)'); -- this one changed to use default parameters
SELECT _postgis_drop_function_by_signature('transform_geometry(geometry, text, text, int)');
SELECT _postgis_drop_function_by_signature('collector(geometry, geometry)');
SELECT _postgis_drop_function_by_signature('st_collector(geometry, geometry)');
SELECT _postgis_drop_function_by_signature('geom_accum (geometry[],geometry)');
SELECT _postgis_drop_function_by_signature('st_geom_accum (geometry[],geometry)');
SELECT _postgis_drop_function_by_signature('collect_garray (geometry[])');
SELECT _postgis_drop_function_by_signature('st_collect_garray (geometry[])');
SELECT _postgis_drop_function_by_signature('geosnoop(geometry)');
SELECT _postgis_drop_function_by_signature('jtsnoop(geometry)');
SELECT _postgis_drop_function_by_signature('st_noop(geometry)');
SELECT _postgis_drop_function_by_signature('st_max_distance(geometry, geometry)');
SELECT _postgis_drop_function_by_signature('ST_MinimumBoundingCircle(geometry)'); --changed to use default parameters
-- Drop internals that should never have existed --
SELECT _postgis_drop_function_by_signature('st_geometry_analyze(internal)');
SELECT _postgis_drop_function_by_signature('st_geometry_in(cstring)');
SELECT _postgis_drop_function_by_signature('st_geometry_out(geometry)');
SELECT _postgis_drop_function_by_signature('st_geometry_recv(internal)');
SELECT _postgis_drop_function_by_signature('st_geometry_send(geometry)');
SELECT _postgis_drop_function_by_signature('st_spheroid_in(cstring)');
SELECT _postgis_drop_function_by_signature('st_spheroid_out(spheroid)');
SELECT _postgis_drop_function_by_signature('st_geometry_lt(geometry, geometry)');
SELECT _postgis_drop_function_by_signature('st_geometry_gt(geometry, geometry)');
SELECT _postgis_drop_function_by_signature('st_geometry_ge(geometry, geometry)');
SELECT _postgis_drop_function_by_signature('st_geometry_eq(geometry, geometry)');
SELECT _postgis_drop_function_by_signature('st_geometry_cmp(geometry, geometry)');
SELECT _postgis_drop_function_by_signature('SnapToGrid(geometry, float8, float8)');
SELECT _postgis_drop_function_by_signature('st_removerepeatedpoints(geometry)');
SELECT _postgis_drop_function_by_signature('st_voronoi(geometry, geometry, double precision, boolean)'); --temporarely introduced before 2.3.0 final

SELECT _postgis_drop_function_by_signature('geometry_gist_sel_2d (internal, oid, internal, int4)');
SELECT _postgis_drop_function_by_signature('geometry_gist_joinsel_2d(internal, oid, internal, smallint)');
SELECT _postgis_drop_function_by_signature('geography_gist_selectivity (internal, oid, internal, int4)');
SELECT _postgis_drop_function_by_signature('geography_gist_join_selectivity(internal, oid, internal, smallint)');

SELECT _postgis_drop_function_by_signature('ST_AsBinary(text)'); -- deprecated in 2.0
SELECT _postgis_drop_function_by_signature('postgis_uses_stats()'); -- deprecated in 2.0
SELECT _postgis_drop_function_by_signature('ST_GeneratePoints(geometry, numeric)'); -- numeric -> integer

-- Old accum aggregate support type, removed in 2.5.0 See #4035
SELECT _postgis_drop_function_by_signature('pgis_abs_in(cstring)', '2.5.0');
SELECT _postgis_drop_function_by_signature('pgis_abs_out(pgis_abs)', '2.5.0');
SELECT _postgis_drop_function_by_signature('pgis_geometry_accum_finalfn(pgis_abs)', '2.5.0');
SELECT _postgis_drop_function_by_signature('pgis_geometry_accum_transfn(pgis_abs, geometry)', '2.5.0');
SELECT _postgis_drop_function_by_signature('pgis_geometry_collect_finalfn(pgis_abs)', '2.5.0');
SELECT _postgis_drop_function_by_signature('pgis_geometry_makeline_finalfn(pgis_abs)', '2.5.0');
SELECT _postgis_drop_function_by_signature('pgis_geometry_polygonize_finalfn(pgis_abs)', '2.5.0');
SELECT _postgis_drop_function_by_signature('pgis_geometry_union_finalfn(pgis_abs)', '2.5.0');
DROP TYPE IF EXISTS pgis_abs CASCADE; -- TODO: use a _postgis_drop_type ?


SELECT _postgis_drop_function_by_signature('st_astwkb(geometry, integer, bigint, bool, bool)'); -- temporarily introduced before 2.2.0 final
SELECT _postgis_drop_function_by_signature('pgis_twkb_accum_transfn(internal, geometry, integer)'); -- temporarily introduced before 2.2.0 final
SELECT _postgis_drop_function_by_signature('pgis_twkb_accum_transfn(internal, geometry, integer, bigint)'); -- temporarily introduced before 2.2.0 final
SELECT _postgis_drop_function_by_signature('pgis_twkb_accum_transfn(internal, geometry, integer, bigint, bool)'); -- temporarily introduced before 2.2.0 final
SELECT _postgis_drop_function_by_signature('pgis_twkb_accum_transfn(internal, geometry, integer, bigint, bool, bool)'); -- temporarily introduced before 2.2.0 final
SELECT _postgis_drop_function_by_signature('pgis_twkb_accum_finalfn(internal)'); -- temporarily introduced before 2.2.0 final

SELECT _postgis_drop_function_by_signature('st_seteffectivearea(geometry, double precision)'); -- temporarily introduced before 2.2.0 final

SELECT _postgis_drop_function_by_signature('geometry_distance_box_nd(geometry, geometry)'); -- temporarily introduced before 2.2.0 final

SELECT _postgis_drop_function_by_signature('_ST_DumpPoints(geometry, integer[])'); -- removed 2.4.0, but really should have been removed 2.1.0 when ST_DumpPoints got reimpmented in C

-- Temporary clean-up while we wait to return these to action in dev
SELECT _postgis_drop_function_by_identity('_ST_DistanceRectTree','g1 geometry, g2 geometry');
SELECT _postgis_drop_function_by_identity('_ST_DistanceRectTreeCached','g1 geometry, g2 geometry');

-- Deplicative signatures removed
SELECT _postgis_drop_function_by_signature('ST_Distance(geography, geography)'); -- dropped in PostGIS-3.0 (r17300 aka ce70e4906)
SELECT _postgis_drop_function_by_signature('ST_Distance(geography, geography, float8, boolean)');
SELECT _postgis_drop_function_by_signature('ST_Buffer(geometry, float8, cstring)');
SELECT _postgis_drop_function_by_signature('ST_IsValidDetail(geometry)');
SELECT _postgis_drop_function_by_signature('ST_RemoveIrrelevantPointsForView(geometry, box2d)'); -- temporarily introduced in 3.5.0dev, replaced by ST_RemoveIrrelevantPointsForView(geometry, box2d, boolean) in 3.5.0dev
SELECT _postgis_drop_function_by_identity('ST_AsKML','int4, geometry, int4, text');
SELECT _postgis_drop_function_by_identity('ST_AsGeoJson','int4, geometry, int4, int4');
SELECT _postgis_drop_function_by_identity('_ST_AsGeoJson','int4, geometry, int4, int4');

-- Underscore_signatures removed for CamelCase
SELECT _postgis_drop_function_by_signature('st_shift_longitude(geometry)');
SELECT _postgis_drop_function_by_signature('st_estimated_extent(text,text,text)');
SELECT _postgis_drop_function_by_signature('st_estimated_extent(text,text)');
SELECT _postgis_drop_function_by_signature('st_find_extent(text,text,text)');
SELECT _postgis_drop_function_by_signature('st_find_extent(text,text)');
SELECT _postgis_drop_function_by_signature('st_mem_size(geometry)');
SELECT _postgis_drop_function_by_signature('st_3dlength_spheroid(geometry, spheroid)');
SELECT _postgis_drop_function_by_signature('st_length_spheroid(geometry, spheroid)');
SELECT _postgis_drop_function_by_signature('st_length2d_spheroid(geometry, spheroid)');
SELECT _postgis_drop_function_by_signature('st_distance_spheroid(geometry, geometry, spheroid)');
SELECT _postgis_drop_function_by_signature('st_point_inside_circle(geometry, float8, float8, float8)');
SELECT _postgis_drop_function_by_signature('st_force_2d(geometry)');
SELECT _postgis_drop_function_by_signature('st_force_3dz(geometry)');
SELECT _postgis_drop_function_by_signature('st_force_3dm(geometry)');
SELECT _postgis_drop_function_by_signature('st_force_collection(geometry)');
SELECT _postgis_drop_function_by_signature('st_force_4d(geometry)');
SELECT _postgis_drop_function_by_signature('st_force_3d(geometry)');
SELECT _postgis_drop_function_by_signature('st_line_interpolate_point(geometry, float8)');
SELECT _postgis_drop_function_by_signature('st_line_substring(geometry, float8, float8)');
SELECT _postgis_drop_function_by_signature('st_line_locate_point(geometry, geometry)');
SELECT _postgis_drop_function_by_signature('st_locate_between_measures(geometry, float8, float8)');
SELECT _postgis_drop_function_by_signature('st_locate_along_measure(geometry, float8)');
SELECT _postgis_drop_function_by_signature('st_combine_bbox(box3d, geometry)');
SELECT _postgis_drop_function_by_signature('st_combine_bbox(box2d, geometry)');
SELECT _postgis_drop_function_by_signature('st_distance_sphere(geometry, geometry)');

-- dev function 3.0 cycle
SELECT _postgis_drop_function_by_signature('pgis_geometry_union_transfn(internal, geometry)');
SELECT _postgis_drop_function_by_signature('pgis_geometry_union_finalfn(internal)');

-- Long Xact support dropped in 3.5.0
-- See https://trac.osgeo.org/postgis/ticket/5723
SELECT _postgis_drop_function_by_signature('UnlockRows(text)');
SELECT _postgis_drop_function_by_signature('LockRow(text, text, text, text, timestamp)');
SELECT _postgis_drop_function_by_signature('LockRow(text, text, text, text)');
SELECT _postgis_drop_function_by_signature('LockRow(text, text, text)');
SELECT _postgis_drop_function_by_signature('LockRow(text, text, text, timestamp)');
SELECT _postgis_drop_function_by_signature('AddAuth(text)');
SELECT _postgis_drop_function_by_signature('CheckAuth(text, text, text)');
SELECT _postgis_drop_function_by_signature('CheckAuth(text, text)');
SELECT _postgis_drop_function_by_signature('CheckAuthTrigger()');
SELECT _postgis_drop_function_by_signature('GetTransactionID()');
SELECT _postgis_drop_function_by_signature('EnableLongTransactions()');
SELECT _postgis_drop_function_by_signature('LongTransactionsEnabled()');
SELECT _postgis_drop_function_by_signature('DisableLongTransactions()');


-- #4394
update pg_operator set oprcanhash = true, oprcanmerge = true where oprname = '=' and oprcode = 'geometry_eq'::regproc;


DO language 'plpgsql'
$$
BEGIN
IF _postgis_scripts_pgsql_version()::integer >= 96 THEN
-- mark ST_Union agg as parallel safe if it is not already
        BEGIN
            UPDATE pg_catalog.pg_proc SET proparallel = 's'
            WHERE oid = 'st_union(geometry)'::regprocedure AND proparallel = 'u';
        EXCEPTION WHEN OTHERS THEN
            RAISE DEBUG 'Could not update st_union(geometry): %', SQLERRM;
        END;
END IF;
END;
$$;

-- #5564 fix up the brin op classes for upgrades
DO language plpgsql
$$
BEGIN
    -- Check if the function is already associated with any operator class
    IF NOT EXISTS (
        SELECT 1 FROM pg_catalog.pg_amproc
        WHERE amproc::text = 'geom2d_brin_inclusion_merge' AND
              amprocfamily IN (
                  SELECT oid FROM pg_catalog.pg_opfamily
                  WHERE opfname = 'brin_geometry_inclusion_ops_2d'
              )
    ) THEN
        BEGIN
            -- Create a temporary operator class for 'brin_geometry_inclusion_ops_2d_temp'
            CREATE OPERATOR CLASS brin_geometry_inclusion_ops_2d_temp
                FOR TYPE geometry USING brin AS
                    FUNCTION 11 geom2d_brin_inclusion_merge(internal, internal);

            -- find current and new operator family OIDs
            WITH a AS (
                SELECT n.oid AS oid_nfamily, o.oid AS oid_ofamily
                FROM pg_catalog.pg_opfamily AS n
                CROSS JOIN (
                    SELECT oid
                    FROM pg_catalog.pg_opfamily
                    WHERE opfname = 'brin_geometry_inclusion_ops_2d_temp'
                ) AS o
                WHERE n.opfname = 'brin_geometry_inclusion_ops_2d'
            ),
            -- Update the amprocfamily in pg_amproc
            amupdate AS (
                UPDATE pg_catalog.pg_amproc AS amp
                SET amprocfamily = a.oid_nfamily
                FROM a
                WHERE amp.amprocfamily = a.oid_ofamily AND
                      amp.amproc::text = 'geom2d_brin_inclusion_merge'
                RETURNING amp.*
            )
            -- Update dependencies in pg_depend to the existing operator class
            UPDATE pg_depend AS d
            SET refobjid = (
                SELECT n.oid
                FROM pg_catalog.pg_opclass AS n
                WHERE opcname = 'brin_geometry_inclusion_ops_2d'
            )
            FROM amupdate
            WHERE d.objid = amupdate.oid AND
                  refobjid = (
                      SELECT o.oid
                      FROM pg_catalog.pg_opclass AS o
                      WHERE opcname = 'brin_geometry_inclusion_ops_2d_temp'
                  );

            -- Drop the temporary operator family after use
            DROP OPERATOR FAMILY brin_geometry_inclusion_ops_2d_temp USING brin;
        EXCEPTION WHEN OTHERS THEN
            RAISE EXCEPTION
                'Could not add geom2d_brin_inclusion_merge to brin_geometry_inclusion_ops_2d class: %',
                SQLERRM;
        END;
    END IF;

    -- Check if the function is already associated with any operator class
    IF NOT EXISTS (
        SELECT 1
        FROM pg_catalog.pg_amproc
        WHERE amproc::text = 'geom3d_brin_inclusion_merge'
    ) THEN
        BEGIN
            -- Create a temporary operator class with the new function
            CREATE OPERATOR CLASS brin_geometry_inclusion_ops_3d_temp
            FOR TYPE geometry USING brin AS
                FUNCTION 11 geom3d_brin_inclusion_merge(internal, internal);

            -- Change the pg_amproc association to the existing opclass
             WITH a AS (
                SELECT n.oid AS oid_nfamily, o.oid AS oid_ofamily
                FROM pg_catalog.pg_opfamily AS n
                CROSS JOIN (
                    SELECT oid FROM pg_catalog.pg_opfamily
                    WHERE opfname = 'brin_geometry_inclusion_ops_3d_temp'
                ) AS o
                WHERE n.opfname = 'brin_geometry_inclusion_ops_3d'
            ), amupdate AS (
                UPDATE pg_catalog.pg_amproc AS amp
                SET amprocfamily = a.oid_nfamily
                FROM a
                WHERE
                    amp.amprocfamily = a.oid_ofamily
                    AND amp.amproc::text = 'geom3d_brin_inclusion_merge'
                RETURNING amp.*
            )
            -- Change the opclass dependency to the existing one
            UPDATE pg_depend AS d
            SET refobjid = (
                SELECT n.oid
                FROM pg_catalog.pg_opclass AS n
                WHERE opcname = 'brin_geometry_inclusion_ops_3d'
            )
            FROM amupdate
            WHERE d.objid = amupdate.oid AND
                  refobjid = (
                    SELECT o.oid
                    FROM pg_catalog.pg_opclass AS o
                    WHERE opcname = 'brin_geometry_inclusion_ops_3d_temp'
                );

            -- Dropping the autogenerated temporary family which cascades to temporary class
            DROP OPERATOR FAMILY brin_geometry_inclusion_ops_3d_temp USING brin;
        EXCEPTION WHEN OTHERS THEN
            RAISE EXCEPTION 'Could not add geom3d_brin_inclusion_merge to brin_geometry_inclusion_ops_3d class: %', SQLERRM;
        END;
    END IF;

    -- Check if the function is already associated with any operator class
    IF NOT EXISTS (
        SELECT 1
        FROM pg_catalog.pg_amproc
        WHERE amproc::text = 'geom4d_brin_inclusion_merge'
    ) THEN
        BEGIN
            -- Create a temporary operator class with the new function
            CREATE OPERATOR CLASS brin_geometry_inclusion_ops_4d_temp
            FOR TYPE geometry USING brin AS
                FUNCTION 11 geom4d_brin_inclusion_merge(internal, internal);

            -- Change the pg_amproc association to the existing opclass
             WITH a AS (
                SELECT n.oid AS oid_nfamily, o.oid AS oid_ofamily
                FROM pg_catalog.pg_opfamily AS n
                CROSS JOIN (
                    SELECT oid FROM pg_catalog.pg_opfamily
                    WHERE opfname = 'brin_geometry_inclusion_ops_4d_temp'
                ) AS o
                WHERE n.opfname = 'brin_geometry_inclusion_ops_4d'
            ) , amupdate AS (
                UPDATE pg_catalog.pg_amproc AS amp
                SET amprocfamily = a.oid_nfamily
                FROM a
                WHERE
                    amp.amprocfamily = a.oid_ofamily
                    AND amp.amproc::text = 'geom4d_brin_inclusion_merge'
                RETURNING amp.*
            )
            -- Change the opclass dependency to the existing one
            UPDATE pg_depend AS d
            SET refobjid = (
                SELECT n.oid
                FROM pg_catalog.pg_opclass AS n
                WHERE opcname = 'brin_geometry_inclusion_ops_4d'
            )
            FROM amupdate
            WHERE d.objid = amupdate.oid AND
                  refobjid = (
                    SELECT o.oid
                    FROM pg_catalog.pg_opclass AS o
                    WHERE opcname = 'brin_geometry_inclusion_ops_4d_temp'
                );

            -- Dropping the autogenerated temporary family also drops the temporary class
            DROP OPERATOR FAMILY brin_geometry_inclusion_ops_4d_temp USING brin;
        EXCEPTION WHEN OTHERS THEN
            RAISE EXCEPTION
                'Could not add geom4d_brin_inclusion_merge to brin_geometry_inclusion_ops_4d class: %',
                SQLERRM;
        END;
    END IF;

-- geography brin
    -- Check if the function 'geog_brin_inclusion_merge' already exists
    IF NOT EXISTS (
        SELECT 1
        FROM pg_catalog.pg_amproc
        WHERE amproc::text = 'geog_brin_inclusion_merge'
    ) THEN
        BEGIN
            -- Create a temporary operator class for 'brin_geography_inclusion_ops_temp'
            CREATE OPERATOR CLASS brin_geography_inclusion_ops_temp
                FOR TYPE geography USING brin AS
                    FUNCTION 11 geog_brin_inclusion_merge(internal, internal);

            -- find current and new operator family OIDs
            WITH a AS (
                SELECT n.oid AS oid_nfamily, o.oid AS oid_ofamily
                FROM pg_catalog.pg_opfamily AS n
                CROSS JOIN (
                    SELECT oid
                    FROM pg_catalog.pg_opfamily
                    WHERE opfname = 'brin_geography_inclusion_ops_temp'
                ) AS o
                WHERE n.opfname = 'brin_geography_inclusion_ops'
            ),
            -- Update the amprocfamily in pg_amproc
            amupdate AS (
                UPDATE pg_catalog.pg_amproc AS amp
                SET amprocfamily = a.oid_nfamily
                FROM a
                WHERE amp.amprocfamily = a.oid_ofamily AND
                      amp.amproc::text = 'geog_brin_inclusion_merge'
                RETURNING amp.*
            )
            -- Update dependencies in pg_depend to the existing operator class
            UPDATE pg_depend AS d
            SET refobjid = (
                SELECT n.oid
                FROM pg_catalog.pg_opclass AS n
                WHERE opcname = 'brin_geography_inclusion_ops'
            )
            FROM amupdate
            WHERE d.objid = amupdate.oid AND
                  refobjid = (
                      SELECT o.oid
                      FROM pg_catalog.pg_opclass AS o
                      WHERE opcname = 'brin_geography_inclusion_ops_temp'
                  );

            -- Drop the temporary operator family after use
            DROP OPERATOR FAMILY brin_geography_inclusion_ops_temp USING brin;
        EXCEPTION WHEN OTHERS THEN
            RAISE EXCEPTION
                'Could not add geog_brin_inclusion_merge to brin_geography_inclusion_ops class: %',
                SQLERRM;
        END;
    END IF;
END;

$$;
