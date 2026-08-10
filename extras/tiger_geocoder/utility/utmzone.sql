CREATE OR REPLACE FUNCTION utmzone(@extschema:postgis@.geometry) RETURNS integer AS
$BODY$
DECLARE
    geomgeog @extschema:postgis@.geometry;
    zone int;
    pref int;
BEGIN
    geomgeog:=@extschema:postgis@.ST_Transform($1,4326);
    IF (@extschema:postgis@.ST_Y(geomgeog))>0 THEN
        pref:=32600;
    ELSE
        pref:=32700;
    END IF;
    zone:=floor((@extschema:postgis@.ST_X(geomgeog)+180)/6)+1;
    RETURN zone+pref;
END;
$BODY$ LANGUAGE 'plpgsql' immutable;
