----------------------------------------------------------------------
--
-- $Id$
--
-- Copyright (c) 2009-2010 Pierre Racine <pierre.racine@sbf.ulaval.ca>
--
----------------------------------------------------------------------

-- Note: The functions provided in this script are in developement. Do not use.

-- Fix a bug in ST_HasNoBand
CREATE OR REPLACE FUNCTION ST_HasNoBand(raster, int)
    RETURNS boolean
    AS 'SELECT $1 IS NULL OR ST_NumBands($1) < $2'
    LANGUAGE 'SQL' IMMUTABLE;

CREATE OR REPLACE FUNCTION ST_MultiBandMapAlgebra(rast1 raster, 
                                            rast2 raster, 
                                            expression text, 
                                            extentexpr text) 
    RETURNS raster AS 
    $$
    DECLARE
    numband int;
    newrast raster;
    pixeltype text;
    nodataval float;
    BEGIN
        numband := ST_NumBands(rast1);
        IF numband != ST_NumBands(rast2) THEN
            RAISE EXCEPTION 'ST_MultiBandMapAlgebra: Rasters do not have the same number of band';
        END IF;
        newrast := ST_MakeEmptyRaster(rast1);
        FOR b IN 1..numband LOOP
            pixeltype := ST_BandPixelType(rast1, b);
            nodataval := ST_BandNodataValue(rast1, b);
            newrast := ST_AddBand(newrast, NULL, ST_MapAlgebra(rast1, b, rast2, b, expression, pixeltype, extentexpr, nodataval), 1);
        END LOOP;
    END;
    $$
    LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION MultiBandMapAlgebra4Union(rast1 raster, rast2 raster, expression text)
    RETURNS raster 
    AS 'SELECT ST_MultiBandMapAlgebra(rast1, rast2, expression, NULL, 'UNION')'
    LANGUAGE 'SQL';

DROP TYPE rastexpr CASCADE;
CREATE TYPE rastexpr AS (
    rast raster,
    f_expression text,
    f_nodata1expr text, 
    f_nodata2expr text,
    f_nodatanodataexpr text 
);

--DROP FUNCTION MapAlgebra4Union(rast1 raster, rast2 raster, expression text);
CREATE OR REPLACE FUNCTION MapAlgebra4Union(rast1 raster, 
                                            rast2 raster, 
                                            p_expression text, 
                                            p_nodata1expr text, 
                                            p_nodata2expr text, 
                                            p_nodatanodataexpr text, 
                                            t_expression text, 
                                            t_nodata1expr text, 
                                            t_nodata2expr text, 
                                            t_nodatanodataexpr text)
    RETURNS raster AS 
    $$
    DECLARE
        t_raster raster;
        p_raster raster;
    BEGIN
        -- With the new ST_MapAlgebra we must split the main expression in three expressions: expression, nodata1expr, nodata2expr
        -- ST_MapAlgebra(rast1 raster, band1 integer, rast2 raster, band2 integer, expression text, pixeltype text, extentexpr text, nodata1expr text, nodata2expr text, nodatanodatadaexpr text)
        -- We must make sure that when NULL is passed as the first raster to ST_MapAlgebra, ST_MapAlgebra resolve the nodata1expr
        IF upper(p_expression) = 'LAST' THEN
            RETURN ST_MapAlgebra(rast1, 1, rast2, 1, 'rast2'::text, NULL::text, 'UNION'::text, 'rast2'::text, 'rast1'::text, NULL::text);
        ELSIF upper(p_expression) = 'FIRST' THEN
            RETURN ST_MapAlgebra(rast1, 1, rast2, 1, 'rast1'::text, NULL::text, 'UNION'::text, 'rast2'::text, 'rast1'::text, NULL::text);
        ELSIF upper(p_expression) = 'MIN' THEN
            RETURN ST_MapAlgebra(rast1, 1, rast2, 1, 'LEAST(rast1, rast2)'::text, NULL::text, 'UNION'::text, 'rast2'::text, 'rast1'::text, NULL::text);
        ELSIF upper(p_expression) = 'MAX' THEN
            RETURN ST_MapAlgebra(rast1, 1, rast2, 1, 'GREATEST(rast1, rast2)'::text, NULL::text, 'UNION'::text, 'rast2'::text, 'rast1'::text, NULL::text);
        ELSIF upper(p_expression) = 'COUNT' THEN
            RETURN ST_MapAlgebra(rast1, 1, rast2, 1, 'rast1 + 1'::text, NULL::text, 'UNION'::text, '1'::text, 'rast1'::text, '0'::text);
        ELSIF upper(p_expression) = 'SUM' THEN
            RETURN ST_MapAlgebra(rast1, 1, rast2, 1, 'rast1 + rast2'::text, NULL::text, 'UNION'::text, 'rast2'::text, 'rast1'::text, NULL::text);
        ELSIF upper(p_expression) = 'RANGE' THEN
            t_raster = ST_MapAlgebra(rast1, 2, rast2, 1, 'LEAST(rast1, rast2)'::text, NULL::text, 'UNION'::text, 'rast2'::text, 'rast1'::text, NULL::text);
            p_raster := MapAlgebra4Union(rast1, rast2, 'MAX'::text, NULL::text, NULL::text, NULL::text, NULL::text, NULL::text, NULL::text, NULL::text);
            RETURN ST_AddBand(p_raster, t_raster, 1, 2);
        ELSIF upper(p_expression) = 'MEAN' THEN
            t_raster = ST_MapAlgebra(rast1, 2, rast2, 1, 'rast1 + 1'::text, NULL::text, 'UNION'::text, '1'::text, 'rast1'::text, '0'::text);
            p_raster := MapAlgebra4Union(rast1, rast2, 'SUM'::text, NULL::text, NULL::text, NULL::text, NULL::text, NULL::text, NULL::text, NULL::text);
            RETURN ST_AddBand(p_raster, t_raster, 1, 2);
        ELSE
            IF t_expression NOTNULL AND t_expression != '' THEN
                t_raster = ST_MapAlgebra(rast1, 2, rast2, 1, t_expression, NULL::text, 'UNION'::text, t_nodata1expr, t_nodata2expr, t_nodatanodataexpr::text);
                p_raster = ST_MapAlgebra(rast1, 1, rast2, 1, p_expression, NULL::text, 'UNION'::text, p_nodata1expr, p_nodata2expr, p_nodatanodataexpr::text);
                RETURN ST_AddBand(p_raster, t_raster, 1, 2);
            END IF;     
            RETURN ST_MapAlgebra(rast1, 1, rast2, 1, p_expression, NULL, 'UNION'::text, NULL::text, NULL::text, NULL::text);
        END IF;
    END;
    $$
    LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION MapAlgebra4UnionFinal3(rast rastexpr)
    RETURNS raster AS 
    $$
    DECLARE
    BEGIN
        RETURN ST_MapAlgebra(rast.rast, 1, rast.rast, 2, rast.f_expression, NULL::text, 'UNION'::text, rast.f_nodata1expr, rast.f_nodata2expr, rast.f_nodatanodataexpr);
    END;
    $$
    LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION MapAlgebra4UnionFinal1(rast rastexpr)
    RETURNS raster AS 
    $$
    DECLARE
    BEGIN
        IF upper(rast.f_expression) = 'RANGE' THEN
            RETURN ST_MapAlgebra(rast.rast, 1, rast.rast, 2, 'rast1 - rast2'::text, NULL::text, 'UNION'::text, NULL::text, NULL::text, NULL::text);
        ELSEIF upper(rast.f_expression) = 'MEAN' THEN
            RETURN ST_MapAlgebra(rast.rast, 1, rast.rast, 2, 'CASE WHEN rast2 > 0 THEN rast1 / rast2::float8 ELSE NULL END'::text, NULL::text, 'UNION'::text, NULL::text, NULL::text, NULL::text);
        ELSE
            RETURN rast.rast;
        END IF;
    END;
    $$
    LANGUAGE 'plpgsql';


CREATE OR REPLACE FUNCTION MapAlgebra4Union(rast1 rastexpr, 
                        rast2 raster, 
                        p_expression text, 
                        p_nodata1expr text, 
                        p_nodata2expr text, 
                        p_nodatanodataexpr text, 
                        t_expression text, 
                        t_nodata1expr text, 
                        t_nodata2expr text, 
                        t_nodatanodataexpr text, 
                        f_expression text,
                        f_nodata1expr text, 
                        f_nodata2expr text, 
                        f_nodatanodataexpr text)
    RETURNS rastexpr 
    AS $$
        SELECT (MapAlgebra4Union(($1).rast, $2, $3, $4, $5, $6, $7, $8, $9, $10), $11, $12, $13, $14)::rastexpr
    $$ LANGUAGE 'SQL';

CREATE OR REPLACE FUNCTION MapAlgebra4Union(rast1 rastexpr, 
                        rast2 raster, 
                        p_expression text, 
                        t_expression text, 
                        f_expression text)
    RETURNS rastexpr 
    AS $$
        SELECT (MapAlgebra4Union(($1).rast, $2, $3, NULL, NULL, NULL, $4, NULL, NULL, NULL), $5, NULL, NULL, NULL)::rastexpr
    $$ LANGUAGE 'SQL';

CREATE OR REPLACE FUNCTION MapAlgebra4Union(rast1 rastexpr, 
                        rast2 raster, 
                        p_expression text, 
                        p_nodata1expr text, 
                        p_nodata2expr text, 
                        p_nodatanodataexpr text, 
                        t_expression text, 
                        t_nodata1expr text, 
                        t_nodata2expr text, 
                        t_nodatanodataexpr text)
    RETURNS rastexpr 
    AS $$
        SELECT (MapAlgebra4Union(($1).rast, $2, $3, $4, $5, $6, $7, $8, $9, $10), NULL, NULL, NULL, NULL)::rastexpr
    $$ LANGUAGE 'SQL';

CREATE OR REPLACE FUNCTION MapAlgebra4Union(rast1 rastexpr, 
                        rast2 raster, 
                        p_expression text, 
                        t_expression text)
    RETURNS rastexpr 
    AS $$
        SELECT (MapAlgebra4Union(($1).rast, $2, $3, NULL, NULL, NULL, $4, NULL, NULL, NULL), NULL, NULL, NULL, NULL)::rastexpr
    $$ LANGUAGE 'SQL';

CREATE OR REPLACE FUNCTION MapAlgebra4Union(rast1 rastexpr, 
                        rast2 raster, 
                        p_expression text, 
                        p_nodata1expr text, 
                        p_nodata2expr text, 
                        p_nodatanodataexpr text)
    RETURNS rastexpr 
    AS $$
        SELECT (MapAlgebra4Union(($1).rast, $2, $3, $4, $5, $6, NULL, NULL, NULL, NULL), NULL, NULL, NULL, NULL)::rastexpr
    $$ LANGUAGE 'SQL';

CREATE OR REPLACE FUNCTION MapAlgebra4Union(rast1 rastexpr, 
                        rast2 raster, 
                        p_expression text)
    RETURNS rastexpr 
    AS $$
        SELECT (MapAlgebra4Union(($1).rast, $2, $3, NULL, NULL, NULL, NULL, NULL, NULL, NULL), $3, NULL, NULL, NULL)::rastexpr
    $$ LANGUAGE 'SQL';

CREATE OR REPLACE FUNCTION MapAlgebra4Union(rast1 rastexpr, 
                        rast2 raster)
    RETURNS rastexpr 
    AS $$
        SELECT (MapAlgebra4Union(($1).rast, $2, 'LAST', NULL, NULL, NULL, NULL, NULL, NULL, NULL), NULL, NULL, NULL, NULL)::rastexpr
    $$ LANGUAGE 'SQL';

--DROP AGGREGATE ST_Union(raster, text, text, text, text, text, text, text, text, text, text, text, text);
CREATE AGGREGATE ST_Union(raster, text, text, text, text, text, text, text, text, text, text, text, text) (
    SFUNC = MapAlgebra4Union,
    STYPE = rastexpr,
    FINALFUNC = MapAlgebra4UnionFinal3
);

--DROP AGGREGATE ST_Union(raster, text, text, text);
CREATE AGGREGATE ST_Union(raster, text, text, text) (
    SFUNC = MapAlgebra4Union,
    STYPE = rastexpr,
    FINALFUNC = MapAlgebra4UnionFinal3
);

--DROP AGGREGATE ST_Union(raster, text, text, text, text, text, text, text, text);
CREATE AGGREGATE ST_Union(raster, text, text, text, text, text, text, text, text) (
    SFUNC = MapAlgebra4Union,
    STYPE = rastexpr
);

--DROP AGGREGATE ST_Union(raster, text, text);
CREATE AGGREGATE ST_Union(raster, text, text) (
    SFUNC = MapAlgebra4Union,
    STYPE = rastexpr
);

--DROP AGGREGATE ST_Union(raster, text, text, text, text);
CREATE AGGREGATE ST_Union(raster, text, text, text, text) (
    SFUNC = MapAlgebra4Union,
    STYPE = rastexpr,
    FINALFUNC = MapAlgebra4UnionFinal1
);

--DROP AGGREGATE ST_Union(raster, text);
CREATE AGGREGATE ST_Union(raster, text) (
    SFUNC = MapAlgebra4Union,
    STYPE = rastexpr,
    FINALFUNC = MapAlgebra4UnionFinal1
);

--DROP AGGREGATE ST_Union(raster);
CREATE AGGREGATE ST_Union(raster) (
    SFUNC = MapAlgebra4Union,
    STYPE = rastexpr
);

-- Test ST_TestRaster
SELECT ST_AsBinary((rast).geom), (rast).val 
FROM (SELECT ST_PixelAsPolygons(ST_TestRaster(0, 0, 2)) rast) foo

-- Test St_Union
SELECT ST_AsBinary((rast).geom), (rast).val 
FROM (SELECT ST_PixelAsPolygons(ST_Union(rast, 'mean'), 1) rast 
      FROM (SELECT ST_TestRaster(1, 0, 6) AS rast 
            UNION ALL 
            SELECT ST_TestRaster(1, 1, 4) AS rast 
            UNION ALL 
            SELECT ST_TestRaster(-1, 0, 6) AS rast
            UNION ALL 
            SELECT ST_TestRaster(0, 0, 2) AS rast 
            ) foi) foo

-- Test St_Union merging a blending merge of disjoint raster
SELECT ST_AsBinary((rast).geom), (rast).val 
FROM (SELECT ST_PixelAsPolygons(ST_Union(rast, 'last'), 1) rast 
      FROM (SELECT ST_TestRaster(0, 0, 1) AS rast 
            UNION ALL 
            SELECT ST_TestRaster(3, 0, 2) AS rast 
            UNION ALL 
            SELECT ST_TestRaster(3, 3, 4) AS rast
            UNION ALL 
            SELECT ST_TestRaster(0, 3, 3) AS rast 
            ) foi) foo

      
-- Explicit implementation of 'MEAN' to make sure directly passing expressions works properly
SELECT ST_AsBinary((rast).geom), (rast).val 
FROM (SELECT ST_PixelAsPolygons(ST_Union(rast, 'rast1 + rast2'::text, 'rast2'::text, 'rast1'::text, NULL::text,
                                               'rast1 + 1'::text, '1'::text, 'rast1'::text, '0'::text,
                                               'CASE WHEN rast2 > 0 THEN rast1 / rast2::float8 ELSE NULL END'::text, NULL::text, NULL::text, NULL::text), 1) rast 
      FROM (SELECT ST_TestRaster(0, 0, 2) AS rast 
            UNION ALL 
            SELECT ST_TestRaster(1, 1, 4) AS rast 
            UNION ALL 
            SELECT ST_TestRaster(1, 0, 6) AS rast 
            UNION ALL 
            SELECT ST_TestRaster(-1, 0, 6) AS rast
            ) foi) foo

-- Pseudo-explicit implementation of 'MEAN' using other predefined functions. Do not work yet...
SELECT ST_AsBinary((rast).geom), (rast).val 
FROM (SELECT ST_PixelAsPolygons(ST_Union(rast, 'SUM'::text, 
                                               'COUNT'::text, 
                                               'CASE WHEN rast2 > 0 THEN rast1 / rast2::float8 ELSE NULL END'::text), 1) rast 
      FROM (SELECT ST_TestRaster(0, 0, 2) AS rast 
            UNION ALL 
            SELECT ST_TestRaster(1, 1, 4) AS rast 
            UNION ALL 
            SELECT ST_TestRaster(1, 0, 6) AS rast 
            UNION ALL 
            SELECT ST_TestRaster(-1, 0, 6) AS rast
            ) foi) foo


SELECT AsBinary((rast).geom), (rast).val 
FROM (SELECT ST_PixelAsPolygons(ST_Union(rast), 1) AS rast 
      FROM (SELECT ST_TestRaster(0, 0, 1) AS rast UNION ALL SELECT ST_TestRaster(2, 0, 2)
           ) foi
     ) foo


SELECT AsBinary((rast).geom), (rast).val 
FROM (SELECT ST_PixelAsPolygons(ST_Union(rast, 'mean'), 1) AS rast 
      FROM (SELECT ST_TestRaster(0, 0, 1) AS rast UNION ALL SELECT ST_TestRaster(1, 0, 2) UNION ALL SELECT ST_TestRaster(0, 1, 6)
           ) foi
     ) foo