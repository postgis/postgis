CREATE OR REPLACE FUNCTION utmzone(geometry) RETURNS integer AS
$BODY$
DECLARE
    geomgeog geometry;
    zone int;
    pref int;
BEGIN
    geomgeog:=transform($1,4326);
    IF (y(geomgeog))>0 THEN
        pref:=32600;
    ELSE
        pref:=32700;
    END IF;
    zone:=floor((x(geomgeog)+180)/6)+1;
    RETURN zone+pref;
END;
$BODY$ LANGUAGE 'plpgsql' immutable;
