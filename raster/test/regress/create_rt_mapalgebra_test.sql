CREATE OR REPLACE FUNCTION ST_TestRaster(ulx float8, uly float8, val float8) 
    RETURNS raster AS 
    $$
    DECLARE
    BEGIN
        RETURN ST_AddBand(ST_MakeEmptyRaster(10, 10, ulx, uly, 1, 1, 0, 0, 0), '32BF', val, -1);
    END;
    $$
    LANGUAGE 'plpgsql';



CREATE OR REPLACE FUNCTION raster_plus_twenty(pixel FLOAT, VARIADIC args TEXT[])
    RETURNS FLOAT AS 
    $$
    BEGIN
        IF pixel IS NULL THEN
            RAISE NOTICE 'Pixel value is null.';
        END IF;
        RETURN pixel + 20;
    END;
    $$ 
    LANGUAGE 'plpgsql' IMMUTABLE;

CREATE OR REPLACE FUNCTION raster_plus_arg1(pixel FLOAT, VARIADIC args TEXT[])
    RETURNS FLOAT AS 
    $$
    DECLARE
        x float := 0;
    BEGIN
        IF NOT args[1] IS NULL THEN
            x := args[1]::float;
        END IF;
        RETURN pixel + x;
    END;
    $$ 
    LANGUAGE 'plpgsql' IMMUTABLE;

CREATE OR REPLACE FUNCTION raster_polynomial(pixel FLOAT, VARIADIC args TEXT[])
    RETURNS FLOAT AS
    $$
    DECLARE
        m float := 1;
        b float := 0;
    BEGIN
        IF NOT args[1] is NULL THEN
            m := args[1]::float;
        END IF;
        IF NOT args[2] is NULL THEN
            b := args[2]::float;
        END IF;
        RETURN m * pixel + b;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE;

 CREATE OR REPLACE FUNCTION raster_nullage(pixel FLOAT, VARIADIC args TEXT[])
    RETURNS FLOAT AS
    $$
    BEGIN
        RETURN NULL;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE;

 CREATE OR REPLACE FUNCTION raster_x_plus_arg(pixel FLOAT, pos INT[], VARIADIC args TEXT[])
    RETURNS FLOAT AS
    $$
    DECLARE
        x float := 0;
    BEGIN
        IF NOT args[1] IS NULL THEN
            x := args[1]::float;
        END IF;
        RETURN pixel + pos[1] + x;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE;

 CREATE OR REPLACE FUNCTION raster_y_plus_arg(pixel FLOAT, pos INT[], VARIADIC args TEXT[])
    RETURNS FLOAT AS
    $$
    DECLARE
        x float := 0;
    BEGIN
        IF NOT args[1] IS NULL THEN
            x := args[1]::float;
        END IF;
        RETURN pixel + pos[2] + x;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE;
