----------------------------------------------------------------------
-- Copyright (c) 2009-2010 Pierre Racine <pierre.racine@sbf.ulaval.ca>
----------------------------------------------------------------------

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

CREATE OR REPLACE FUNCTION ST_AddBand(rast1 raster, rast2 raster, band int, index int)
    RETURNS raster AS 
    $$
    DECLARE
	newraster raster;
	newnodatavalue int;
	newindex int := index;
	newband int := band;
	x int;
	y int;
    BEGIN
	IF ST_Width(rast1) != ST_Width(rast2) OR ST_Height(rast1) != ST_Height(rast2) THEN
		RAISE EXCEPTION 'ST_AddBand: Attempting to add a band with different width or height';
	END IF;
	IF newindex < 1 THEN
		newindex := 1;
	END IF;
	IF newindex IS NULL OR newindex > ST_NumBands(rast1) THEN
		newindex := ST_NumBands(rast1) + 1;
	END IF;
	IF newband < 1 THEN
		newband := 1;
	END IF;
	IF newband IS NULL OR newband > ST_NumBands(rast2) THEN
		newband := ST_NumBands(rast2);
	END IF;
	
	IF newband = 0 THEN
		RETURN rast1;
	END IF;
raise notice 'aaaa newband=%  newindex=%', newband, newindex;	
	newnodatavalue := ST_BandNodataValue(rast2, newband);
raise notice 'bbbb';	
	newraster := ST_AddBand(rast1, newindex, ST_BandPixelType(rast2, newband), newnodatavalue, newnodatavalue);
raise notice 'cccc';	
	FOR x IN 1..ST_Width(rast2) LOOP
		FOR y IN 1..ST_Height(rast2) LOOP
			newraster := ST_SetValue(newraster, newindex, x, y, ST_Value(rast2, newband, x, y));
		END LOOP;
	END LOOP;
	RETURN newraster;
    END;
    $$
    LANGUAGE 'plpgsql';

--Test
SELECT ST_NumBands(ST_AddBand(ST_TestRaster(0, 0, 1), ST_TestRaster(0, 0, 1), 1, 2))

CREATE OR REPLACE FUNCTION MultiBandMapAlgebra4Union(rast1 raster, rast2 raster, expression text)
	RETURNS raster 
	AS 'SELECT ST_MultiBandMapAlgebra(rast1, rast2, expression, NULL, 'UNION')'
	LANGUAGE 'SQL';

DROP  TYPE rastexpr CASCADE;
CREATE TYPE rastexpr AS (
    rast raster,
    f_expression text,
    f_rast1repl float8, 
    f_rast2repl float8 
);

DROP FUNCTION MapAlgebra4Union(rast1 raster, rast2 raster, expression text);
CREATE OR REPLACE FUNCTION MapAlgebra4Union(rast1 raster, 
                                            rast2 raster, 
                                            p_expression text, 
                                            p_rast1repl float8, 
                                            p_rast2repl float8, 
                                            t_expression text, 
                                            t_rast1repl float8, 
                                            t_rast2repl float8)
    RETURNS raster AS 
    $$
    DECLARE
	t_raster raster;
	p_raster raster;
    BEGIN
	IF upper(p_expression) = 'LAST' THEN
		RETURN ST_MapAlgebra(rast1, 1, rast2, 1, 'CASE WHEN rast2 IS NULL THEN rast1 ELSE rast2 END', NULL, 'UNION', NULL);
	ELSIF upper(p_expression) = 'FIRST' THEN
		RETURN ST_MapAlgebra(rast1, 1, rast2, 1, 'CASE WHEN rast1 IS NULL THEN rast2 ELSE rast1 END', NULL, 'UNION', NULL);
	ELSIF upper(p_expression) = 'MIN' THEN
		RETURN ST_MapAlgebra(rast1, 1, rast2, 1, 'CASE WHEN rast2 IS NULL OR rast1 < rast2 THEN rast1 ELSE rast2 END', NULL, 'UNION', NULL);
	ELSIF upper(p_expression) = 'MAX' THEN
		RETURN ST_MapAlgebra(rast1, 1, rast2, 1, 'CASE WHEN rast2 IS NULL OR rast1 > rast2 THEN rast1 ELSE rast2 END', NULL, 'UNION', NULL);
	ELSIF upper(p_expression) = 'COUNT' THEN
		RETURN ST_MapAlgebra(rast1, 1, rast2, 1, 'CASE WHEN rast2 IS NULL THEN rast1 ELSE rast1 + 1 END', NULL, 'UNION', 0, NULL);
	ELSIF upper(p_expression) = 'SUM' THEN
		RETURN ST_MapAlgebra(rast1, 1, rast2, 1, 'CASE WHEN rast2 IS NULL THEN rast1 ELSE rast1 + rast2 END', NULL, 'UNION', 0, NULL);
	ELSIF upper(p_expression) = 'MEAN' THEN
		t_raster = ST_MapAlgebra(rast1, 2, rast2, 1, 'CASE WHEN rast2 IS NULL THEN rast1 ELSE rast1 + 1 END', NULL, 'UNION', 0, NULL);
		p_raster := MapAlgebra4Union(rast1, rast2, 'SUM', NULL, NULL, NULL, NULL, NULL);
		RETURN ST_AddBand(p_raster, t_raster, 1, 2);
	ELSE
		IF t_expression NOTNULL AND t_expression != '' THEN
			t_raster = ST_MapAlgebra(rast1, 2, rast2, 1, t_expression, NULL, 'UNION', t_rast1repl, t_rast2repl);
			p_raster = ST_MapAlgebra(rast1, 1, rast2, 1, p_expression, NULL, 'UNION', p_rast1repl, p_rast2repl);
			RETURN ST_AddBand(p_raster, t_raster, 1, 2);
		END IF;		
		RETURN ST_MapAlgebra(rast1, 1, rast2, 1, p_expression, NULL, 'UNION', NULL);
	END IF;
    END;
    $$
    LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION MapAlgebra4UnionFinal3(rast rastexpr)
    RETURNS raster AS 
    $$
    DECLARE
    BEGIN
	RETURN ST_MapAlgebra(rast.rast, 1, rast.rast, 2, rast.f_expression, NULL, 'UNION', rast.f_rast1repl, rast.f_rast2repl);
    END;
    $$
    LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION MapAlgebra4UnionFinal1(rast rastexpr)
    RETURNS raster AS 
    $$
    DECLARE
    BEGIN
	IF upper(rast.f_expression) = 'MEAN' THEN
		RETURN ST_MapAlgebra(rast.rast, 1, rast.rast, 2, 'CASE WHEN rast2 > 0 THEN rast1 / rast2::float8 ELSE NULL END', NULL, 'UNION', NULL, NULL);
	ELSE
		RETURN rast.rast;
	END IF;
    END;
    $$
    LANGUAGE 'plpgsql';


CREATE OR REPLACE FUNCTION MapAlgebra4Union(rast1 rastexpr, 
					    rast2 raster, 
					    p_expression text, 
					    p_rast1repl float8, 
					    p_rast1repl float8, 
					    t_expression text, 
					    t_rast1repl float8, 
					    t_rast1repl float8, 
					    f_expression text,
					    f_rast1repl float8, 
					    f_rast1repl float8)
	RETURNS rastexpr 
	AS $$
		SELECT (MapAlgebra4Union(($1).rast, $2, $3, $4, $5, $6, $7, $8), $9, $10, $11)::rastexpr
	$$ LANGUAGE 'SQL';

CREATE OR REPLACE FUNCTION MapAlgebra4Union(rast1 rastexpr, 
					    rast2 raster, 
					    p_expression text, 
					    t_expression text, 
					    f_expression text)
	RETURNS rastexpr 
	AS $$
		SELECT (MapAlgebra4Union(($1).rast, $2, $3, NULL, NULL, $4, NULL, NULL), $5, NULL, NULL)::rastexpr
	$$ LANGUAGE 'SQL';

CREATE OR REPLACE FUNCTION MapAlgebra4Union(rast1 rastexpr, 
					    rast2 raster, 
					    p_expression text, 
					    p_rast1repl float8, 
					    p_rast1repl float8, 
					    t_expression text, 
					    t_rast1repl float8, 
					    t_rast1repl float8)
	RETURNS rastexpr 
	AS $$
		SELECT (MapAlgebra4Union(($1).rast, $2, $3, $4, $5, $6, $7, $8), NULL, NULL, NULL)::rastexpr
	$$ LANGUAGE 'SQL';

CREATE OR REPLACE FUNCTION MapAlgebra4Union(rast1 rastexpr, 
					    rast2 raster, 
					    p_expression text, 
					    t_expression text)
	RETURNS rastexpr 
	AS $$
		SELECT (MapAlgebra4Union(($1).rast, $2, $3, NULL, NULL, $4, NULL, NULL), NULL, NULL, NULL)::rastexpr
	$$ LANGUAGE 'SQL';

CREATE OR REPLACE FUNCTION MapAlgebra4Union(rast1 rastexpr, 
					    rast2 raster, 
					    p_expression text, 
					    p_rast1repl float8, 
					    p_rast1repl float8)
	RETURNS rastexpr 
	AS $$
		SELECT (MapAlgebra4Union(($1).rast, $2, $3, $4, $5, NULL, NULL, NULL), NULL, NULL, NULL)::rastexpr
	$$ LANGUAGE 'SQL';

CREATE OR REPLACE FUNCTION MapAlgebra4Union(rast1 rastexpr, 
					    rast2 raster, 
					    p_expression text)
	RETURNS rastexpr 
	AS $$
		SELECT (MapAlgebra4Union(($1).rast, $2, $3, NULL, NULL, NULL, NULL, NULL), $3, NULL, NULL)::rastexpr
	$$ LANGUAGE 'SQL';

CREATE OR REPLACE FUNCTION MapAlgebra4Union(rast1 rastexpr, 
					    rast2 raster)
	RETURNS rastexpr 
	AS $$
		SELECT (MapAlgebra4Union(($1).rast, $2, 'LAST', NULL, NULL, NULL, NULL, NULL), NULL, NULL, NULL)::rastexpr
	$$ LANGUAGE 'SQL';



DROP AGGREGATE ST_Union(raster, text, float8, float8, text, float8, float8, text, float8, float8);
CREATE AGGREGATE ST_Union(raster, text, float8, float8, text, float8, float8, text, float8, float8) (
	SFUNC = MapAlgebra4Union,
	STYPE = rastexpr,
	FINALFUNC = MapAlgebra4UnionFinal3
);

DROP AGGREGATE ST_Union(raster, text, text, text);
CREATE AGGREGATE ST_Union(raster, text, text, text) (
	SFUNC = MapAlgebra4Union,
	STYPE = rastexpr,
	FINALFUNC = MapAlgebra4UnionFinal3
);

DROP AGGREGATE ST_Union(raster, text, float8, float8, text, float8, float8);
CREATE AGGREGATE ST_Union(raster, text, float8, float8, text, float8, float8) (
	SFUNC = MapAlgebra4Union,
	STYPE = rastexpr
);

DROP AGGREGATE ST_Union(raster, text, text);
CREATE AGGREGATE ST_Union(raster, text, text) (
	SFUNC = MapAlgebra4Union,
	STYPE = rastexpr
);

DROP AGGREGATE ST_Union(raster, text, float8, float8);
CREATE AGGREGATE ST_Union(raster, text, float8, float8) (
	SFUNC = MapAlgebra4Union,
	STYPE = rastexpr,
	FINALFUNC = MapAlgebra4UnionFinal1
);

DROP AGGREGATE ST_Union(raster, text);
CREATE AGGREGATE ST_Union(raster, text) (
	SFUNC = MapAlgebra4Union,
	STYPE = rastexpr,
	FINALFUNC = MapAlgebra4UnionFinal1
);

DROP AGGREGATE ST_Union(raster);
CREATE AGGREGATE ST_Union(raster) (
	SFUNC = MapAlgebra4Union,
	STYPE = rastexpr
);

-- Test St_Union with MEAN
SELECT AsBinary((rast).geom), (rast).val 
FROM (SELECT ST_PixelAsPolygons(ST_Union(rast, 'mean'), 1) rast 
      FROM (SELECT ST_TestRaster(0, 0, 2) AS rast 
            UNION ALL 
            SELECT ST_TestRaster(1, 1, 4) AS rast 
            UNION ALL 
            SELECT ST_TestRaster(1, 0, 6) AS rast 
            UNION ALL 
            SELECT ST_TestRaster(-1, 0, 6) AS rast) foi) foo
      
SELECT ST_Union(rast, 'mean') rast 
FROM (SELECT ST_TestRaster(0, 0, 2) AS rast 
    UNION ALL 
    SELECT ST_TestRaster(1, 1, 4) AS rast 
    UNION ALL 
    SELECT ST_TestRaster(1, 0, 6) AS rast 
    UNION ALL 
    SELECT ST_TestRaster(-1, 0, 6) AS rast) foi

    
SELECT ST_Union(rast, 'CASE WHEN rast2 IS NULL THEN rast1 ELSE rast1 + rast2 END', 0, NULL, 
                      'CASE WHEN rast2 IS NULL THEN rast1 ELSE rast1 + 1 END', 0, NULL,
                      'CASE WHEN rast2 > 0 THEN rast1 / rast2::float8 ELSE NULL END', NULL, NULL) rast 
FROM (SELECT ST_TestRaster(0, 0, 2) AS rast 
    UNION ALL 
    SELECT ST_TestRaster(1, 1, 4) AS rast 
    UNION ALL 
    SELECT ST_TestRaster(1, 0, 6) AS rast 
    UNION ALL 
    SELECT ST_TestRaster(-1, 0, 6) AS rast) foi

SELECT AsBinary((rast).geom), (rast).val 
FROM (SELECT ST_PixelAsPolygons(ST_Union(rast, 'CASE WHEN rast2 IS NULL THEN rast1 ELSE rast1 + rast2 END', 0, NULL, 
                      'CASE WHEN rast2 IS NULL THEN rast1 ELSE rast1 + 1 END', 0, NULL,
                      'CASE WHEN rast2 > 0 THEN rast1 / rast2::float8 ELSE NULL END', NULL, NULL), 1) rast 
      FROM (SELECT ST_TestRaster(0, 0, 2) AS rast 
            UNION ALL 
            SELECT ST_TestRaster(1, 1, 4) AS rast 
            UNION ALL 
            SELECT ST_TestRaster(1, 0, 6) AS rast 
            UNION ALL 
            SELECT ST_TestRaster(-1, 0, 6) AS rast) foi) foo
      

SELECT ST_NumBands(ST_Union(rast, 'CASE WHEN rast2 IS NULL THEN rast1 ELSE rast1 + rast2 END', 0, NULL, 
                      'CASE WHEN rast2 IS NULL THEN rast1 ELSE rast1 + 1 END', 0, NULL,
                      'CASE WHEN rast2 > 0 THEN rast1 / rast2::float8 ELSE NULL END', NULL, NULL)) rast 
FROM (SELECT ST_TestRaster(0, 0, 2) AS rast 
    UNION ALL 
    SELECT ST_TestRaster(1, 1, 4) AS rast 
    UNION ALL 
    SELECT ST_TestRaster(1, 0, 6) AS rast 
    UNION ALL 
    SELECT ST_TestRaster(-1, 0, 6) AS rast) foi


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