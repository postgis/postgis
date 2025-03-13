/** set search path to schema postgis is installed in **/
DO Language plpgsql
$$
DECLARE var_sql text;
BEGIN
 var_sql = 'SET search_path=' || quote_ident( (SELECT n.nspname FROM pg_catalog.pg_extension AS e INNER JOIN pg_catalog.pg_namespace AS n ON e.extnamespace = n.oid WHERE e.extname = 'postgis')); 
 EXECUTE var_sql;
END;
$$;


DO language plpgsql
$$
BEGIN
IF NOT EXISTS(SELECT
    1
    FROM
        pg_catalog.pg_operator AS o
    INNER JOIN
        pg_catalog.pg_type AS tl ON o.oprleft = tl.oid
    INNER JOIN
        pg_catalog.pg_type AS tr ON o.oprright = tr.oid
    WHERE
        o.oprname = '~'
        AND tl.typname = 'box2df'
        AND tr.typname = 'geometry'
        AND o.oprcode::text LIKE '%contains_2d') THEN
    -- Availability: 2.3.0
    CREATE OPERATOR ~ (
        LEFTARG    = box2df,
        RIGHTARG   = geometry,
        PROCEDURE  = contains_2d,
        COMMUTATOR = @
    );

    ALTER EXTENSION postgis ADD OPERATOR ~ (box2df , geometry);

END IF;
END;
$$;

DO language plpgsql
$$
BEGIN
IF NOT EXISTS(SELECT
    1
    FROM
        pg_catalog.pg_operator AS o
    INNER JOIN
        pg_catalog.pg_type AS tl ON o.oprleft = tl.oid
    INNER JOIN
        pg_catalog.pg_type AS tr ON o.oprright = tr.oid
    WHERE
        o.oprname = '@'
        AND tl.typname = 'box2df'
        AND tr.typname = 'geometry'
        AND o.oprcode::text LIKE '%is_contained_2d') THEN

    -- Availability: 2.3.0
    CREATE OPERATOR @ (
        LEFTARG    = box2df,
        RIGHTARG   = geometry,
        PROCEDURE  = is_contained_2d,
        COMMUTATOR = ~
    );

    ALTER EXTENSION postgis ADD OPERATOR @ (box2df , geometry);
END IF;

END;
$$;

DO language plpgsql
$$
BEGIN
IF NOT EXISTS(SELECT
    1
    FROM
        pg_catalog.pg_operator AS o
    INNER JOIN
        pg_catalog.pg_type AS tl ON o.oprleft = tl.oid
    INNER JOIN
        pg_catalog.pg_type AS tr ON o.oprright = tr.oid
    WHERE
        o.oprname = '&&'
        AND tl.typname = 'box2df'
        AND tr.typname = 'geometry'
        AND o.oprcode::text LIKE '%overlaps_2d') THEN

    -- Availability: 2.3.0
    CREATE OPERATOR && (
        LEFTARG    = box2df,
        RIGHTARG   = geometry,
        PROCEDURE  = overlaps_2d,
        COMMUTATOR = &&
    );

    ALTER EXTENSION postgis ADD OPERATOR && (box2df , geometry);

END IF;
END;
$$;


DO language plpgsql
$$
BEGIN
IF NOT EXISTS(SELECT
    1
    FROM
        pg_catalog.pg_operator AS o
    INNER JOIN
        pg_catalog.pg_type AS tl ON o.oprleft = tl.oid
    INNER JOIN
        pg_catalog.pg_type AS tr ON o.oprright = tr.oid
    WHERE
        o.oprname = '~'
        AND tl.typname = 'geometry'
        AND tr.typname = 'box2df'
        AND o.oprcode::text LIKE '%contains_2d') THEN

    -- Availability: 2.3.0
    CREATE OPERATOR ~ (
        LEFTARG = geometry,
        RIGHTARG = box2df,
        COMMUTATOR = @,
        PROCEDURE  = contains_2d
    );

    ALTER EXTENSION postgis ADD OPERATOR ~ (geometry , box2df);

END IF;
END;
$$;

DO language plpgsql
$$
BEGIN
IF NOT EXISTS(SELECT
    1
    FROM
        pg_catalog.pg_operator AS o
    INNER JOIN
        pg_catalog.pg_type AS tl ON o.oprleft = tl.oid
    INNER JOIN
        pg_catalog.pg_type AS tr ON o.oprright = tr.oid
    WHERE
        o.oprname = '@'
        AND tl.typname = 'geometry'
        AND tr.typname = 'box2df'
        AND o.oprcode::text LIKE '%is_contained_2d') THEN

    -- Availability: 2.3.0
    CREATE OPERATOR @ (
        LEFTARG = geometry,
        RIGHTARG = box2df,
        COMMUTATOR = ~,
        PROCEDURE = is_contained_2d
    );

    ALTER EXTENSION postgis ADD OPERATOR @ (geometry , box2df);

END IF;
END;
$$;


DO language plpgsql
$$
BEGIN
IF NOT EXISTS(SELECT
    1
    FROM
        pg_catalog.pg_operator AS o
    INNER JOIN
        pg_catalog.pg_type AS tl ON o.oprleft = tl.oid
    INNER JOIN
        pg_catalog.pg_type AS tr ON o.oprright = tr.oid
    WHERE
        o.oprname = '&&'
        AND tl.typname = 'geometry'
        AND tr.typname = 'box2df'
        AND o.oprcode::text LIKE '%overlaps_2d') THEN

    -- Availability: 2.3.0
    CREATE OPERATOR && (
        LEFTARG    = geometry,
        RIGHTARG   = box2df,
        PROCEDURE  = overlaps_2d,
        COMMUTATOR = &&
    );

    ALTER EXTENSION postgis ADD OPERATOR && (geometry , box2df);

END IF;
END;
$$;


DO language plpgsql
$$
BEGIN
IF NOT EXISTS(SELECT
    1
    FROM
        pg_catalog.pg_operator AS o
    INNER JOIN
        pg_catalog.pg_type AS tl ON o.oprleft = tl.oid
    INNER JOIN
        pg_catalog.pg_type AS tr ON o.oprright = tr.oid
    WHERE
        o.oprname = '&&'
        AND tl.typname = 'box2df'
        AND tr.typname = 'box2df'
        AND o.oprcode::text LIKE '%overlaps_2d') THEN

    -- Availability: 2.3.0
    CREATE OPERATOR && (
        LEFTARG   = box2df,
        RIGHTARG  = box2df,
        PROCEDURE = overlaps_2d,
        COMMUTATOR = &&
    );
    ALTER EXTENSION postgis ADD OPERATOR && (box2df , box2df);

END IF;
END;
$$;


DO language plpgsql
$$
BEGIN
IF NOT EXISTS(SELECT
    1
    FROM
        pg_catalog.pg_operator AS o
    INNER JOIN
        pg_catalog.pg_type AS tl ON o.oprleft = tl.oid
    INNER JOIN
        pg_catalog.pg_type AS tr ON o.oprright = tr.oid
    WHERE
        o.oprname = '@'
        AND tl.typname = 'box2df'
        AND tr.typname = 'box2df'
        AND o.oprcode::text LIKE '%is_contained_2d') THEN

    -- Availability: 2.3.0
    CREATE OPERATOR @ (
        LEFTARG   = box2df,
        RIGHTARG  = box2df,
        PROCEDURE = is_contained_2d,
        COMMUTATOR = ~
    );

    ALTER EXTENSION postgis ADD OPERATOR @ (box2df , box2df);

END IF;
END;
$$;

DO language plpgsql
$$
BEGIN
IF NOT EXISTS(SELECT
    1
    FROM
        pg_catalog.pg_operator AS o
    INNER JOIN
        pg_catalog.pg_type AS tl ON o.oprleft = tl.oid
    INNER JOIN
        pg_catalog.pg_type AS tr ON o.oprright = tr.oid
    WHERE
        o.oprname = '~'
        AND tl.typname = 'box2df'
        AND tr.typname = 'box2df'
        AND o.oprcode::text LIKE '%contains_2d') THEN

    -- Availability: 2.3.0
    CREATE OPERATOR ~ (
        LEFTARG   = box2df,
        RIGHTARG  = box2df,
        PROCEDURE = contains_2d,
        COMMUTATOR = @
    );

    ALTER EXTENSION postgis ADD OPERATOR ~ (box2df , box2df);

END IF;
END;
$$;


----------------------------
-- nd operators           --
----------------------------

DO language plpgsql
$$
BEGIN
IF NOT EXISTS(SELECT
    1
    FROM
        pg_catalog.pg_operator AS o
    INNER JOIN
        pg_catalog.pg_type AS tl ON o.oprleft = tl.oid
    INNER JOIN
        pg_catalog.pg_type AS tr ON o.oprright = tr.oid
    WHERE
        o.oprname = '&&&'
        AND tl.typname = 'gidx'
        AND tr.typname = 'geometry'
        AND o.oprcode::text LIKE '%overlaps_nd') THEN

    -- Availability: 2.3.0
    CREATE OPERATOR &&& (
        LEFTARG    = gidx,
        RIGHTARG   = geometry,
        PROCEDURE  = overlaps_nd,
        COMMUTATOR = &&&
    );

    ALTER EXTENSION postgis ADD OPERATOR &&& (gidx , geometry);

END IF;
END;
$$;



DO language plpgsql
$$
BEGIN
IF NOT EXISTS(SELECT
    1
    FROM
        pg_catalog.pg_operator AS o
    INNER JOIN
        pg_catalog.pg_type AS tl ON o.oprleft = tl.oid
    INNER JOIN
        pg_catalog.pg_type AS tr ON o.oprright = tr.oid
    WHERE
        o.oprname = '&&&'
        AND tl.typname = 'gidx'
        AND tr.typname = 'gidx'
        AND o.oprcode::text LIKE '%overlaps_nd') THEN

    -- Availability: 2.3.0
    CREATE OPERATOR &&& (
        LEFTARG    = gidx,
        RIGHTARG   = gidx,
        PROCEDURE  = overlaps_nd,
        COMMUTATOR = &&&
    );

    ALTER EXTENSION postgis ADD OPERATOR &&& (gidx , gidx);

END IF;
END;
$$;

DO language plpgsql
$$
BEGIN
IF NOT EXISTS(SELECT 1 
    FROM pg_opclass 
        WHERE opcname = 'brin_geometry_inclusion_ops_2d') THEN

    -- Availability: 2.3.0
    CREATE OPERATOR CLASS brin_geometry_inclusion_ops_2d
    DEFAULT FOR TYPE geometry
    USING brin AS
        FUNCTION      1        brin_inclusion_opcinfo(internal),
        FUNCTION      2        geom2d_brin_inclusion_add_value(internal, internal, internal, internal),
        FUNCTION      3        brin_inclusion_consistent(internal, internal, internal),
        FUNCTION      4        brin_inclusion_union(internal, internal, internal),
        -- this will be added as part of PostGIS upgrade
        -- FUNCTION      11       geom2d_brin_inclusion_merge(internal, internal),
        OPERATOR      3         &&(box2df, box2df),
        OPERATOR      3         &&(box2df, geometry),
        OPERATOR      3         &&(geometry, box2df),
        OPERATOR      3        &&(geometry, geometry),
        OPERATOR      7         ~(box2df, box2df),
        OPERATOR      7         ~(box2df, geometry),
        OPERATOR      7         ~(geometry, box2df),
        OPERATOR      7        ~(geometry, geometry),
        OPERATOR      8         @(box2df, box2df),
        OPERATOR      8         @(box2df, geometry),
        OPERATOR      8         @(geometry, box2df),
        OPERATOR      8        @(geometry, geometry),
    STORAGE box2df;

    ALTER EXTENSION postgis ADD OPERATOR CLASS brin_geometry_inclusion_ops_2d USING brin;
    ALTER EXTENSION postgis ADD OPERATOR FAMILY brin_geometry_inclusion_ops_2d USING brin;

END IF;
END;
$$;

-------------
-- 3D case --
-------------
DO language plpgsql
$$
BEGIN
IF NOT EXISTS(SELECT 1 
    FROM pg_opclass 
        WHERE opcname = 'brin_geometry_inclusion_ops_3d') THEN

    -- Availability: 2.3.0
    CREATE OPERATOR CLASS brin_geometry_inclusion_ops_3d
    FOR TYPE geometry
    USING brin AS
        FUNCTION      1        brin_inclusion_opcinfo(internal) ,
        FUNCTION      2        geom3d_brin_inclusion_add_value(internal, internal, internal, internal),
        FUNCTION      3        brin_inclusion_consistent(internal, internal, internal),
        FUNCTION      4        brin_inclusion_union(internal, internal, internal),
        -- FUNCTION      11       geom3d_brin_inclusion_merge(internal, internal),
        OPERATOR      3        &&&(geometry, geometry),
        OPERATOR      3        &&&(geometry, gidx),
        OPERATOR      3        &&&(gidx, geometry),
        OPERATOR      3        &&&(gidx, gidx),
    STORAGE gidx;

    ALTER EXTENSION postgis ADD OPERATOR CLASS brin_geometry_inclusion_ops_3d USING brin;
    ALTER EXTENSION postgis ADD OPERATOR FAMILY brin_geometry_inclusion_ops_3d USING brin;

END IF;
END;
$$;


-------------
-- 4D case --
-------------
DO language plpgsql
$$
BEGIN
IF NOT EXISTS(SELECT 1 
    FROM pg_opclass 
        WHERE opcname = 'brin_geometry_inclusion_ops_3d') THEN

    -- Availability: 2.3.0
    CREATE OPERATOR CLASS brin_geometry_inclusion_ops_4d
    FOR TYPE geometry
    USING brin AS
        FUNCTION      1        brin_inclusion_opcinfo(internal),
        FUNCTION      2        geom4d_brin_inclusion_add_value(internal, internal, internal, internal),
        FUNCTION      3        brin_inclusion_consistent(internal, internal, internal),
        FUNCTION      4        brin_inclusion_union(internal, internal, internal),
        -- FUNCTION      11       geom4d_brin_inclusion_merge(internal, internal),
        OPERATOR      3        &&&(geometry, geometry),
        OPERATOR      3        &&&(geometry, gidx),
        OPERATOR      3        &&&(gidx, geometry),
        OPERATOR      3        &&&(gidx, gidx),
    STORAGE gidx;

    ALTER EXTENSION postgis ADD OPERATOR CLASS brin_geometry_inclusion_ops_4d USING brin;
    ALTER EXTENSION postgis ADD OPERATOR FAMILY brin_geometry_inclusion_ops_4d USING brin;

END IF;
END;
$$;

--------------------------------------------------------------------
-- BRIN support for geographies                                   --
--------------------------------------------------------------------

--------------------------------
-- the needed cross-operators --
--------------------------------

DO language plpgsql
$$
BEGIN
IF NOT EXISTS(SELECT
    1
    FROM
        pg_catalog.pg_operator AS o
    INNER JOIN
        pg_catalog.pg_type AS tl ON o.oprleft = tl.oid
    INNER JOIN
        pg_catalog.pg_type AS tr ON o.oprright = tr.oid
    WHERE
        o.oprname = '&&'
        AND tl.typname = 'gidx'
        AND tr.typname = 'geography'
        AND o.oprcode::text LIKE '%overlaps_geog') THEN

    -- Availability: 2.3.0
    CREATE OPERATOR && (
    LEFTARG    = gidx,
    RIGHTARG   = geography,
    PROCEDURE  = overlaps_geog,
    COMMUTATOR = &&
    );

    ALTER EXTENSION postgis ADD OPERATOR && (gidx , overlaps_geog);

END IF;
END;
$$;

DO language plpgsql
$$
BEGIN
IF NOT EXISTS(SELECT
    1
    FROM
        pg_catalog.pg_operator AS o
    INNER JOIN
        pg_catalog.pg_type AS tl ON o.oprleft = tl.oid
    INNER JOIN
        pg_catalog.pg_type AS tr ON o.oprright = tr.oid
    WHERE
        o.oprname = '&&'
        AND tl.typname = 'gidx'
        AND tr.typname = 'gidx'
        AND o.oprcode::text LIKE '%overlaps_geog') THEN

    -- Availability: 2.3.0
    CREATE OPERATOR && (
    LEFTARG   = gidx,
    RIGHTARG  = gidx,
    PROCEDURE = overlaps_geog,
    COMMUTATOR = &&
    );

    ALTER EXTENSION postgis ADD OPERATOR && (gidx , gidx);

END IF;
END;
$$;

DO language plpgsql
$$
BEGIN
IF NOT EXISTS(SELECT 1 
    FROM pg_opclass 
        WHERE opcname = 'brin_geography_inclusion_ops') THEN

    -- Availability: 2.3.0
    CREATE OPERATOR CLASS brin_geography_inclusion_ops
    DEFAULT FOR TYPE geography
    USING brin AS
        FUNCTION      1        brin_inclusion_opcinfo(internal),
        FUNCTION      2        geog_brin_inclusion_add_value(internal, internal, internal, internal),
        FUNCTION      3        brin_inclusion_consistent(internal, internal, internal),
        FUNCTION      4        brin_inclusion_union(internal, internal, internal),
        -- FUNCTION      11       geog_brin_inclusion_merge(internal, internal),
        OPERATOR      3        &&(geography, geography),
        OPERATOR      3        &&(geography, gidx),
        OPERATOR      3        &&(gidx, geography),
        OPERATOR      3        &&(gidx, gidx),
    STORAGE gidx;

    ALTER EXTENSION postgis ADD OPERATOR CLASS brin_geography_inclusion_ops USING brin;
    ALTER EXTENSION postgis ADD OPERATOR FAMILY brin_geography_inclusion_ops USING brin;

END IF;
END;
$$;

