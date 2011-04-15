-- This function converts string addresses to integers and passes them to
-- the other interpolate_from_address function.
CREATE OR REPLACE FUNCTION interpolate_from_address(given_address INTEGER, in_addr1 VARCHAR, in_addr2 VARCHAR, road GEOMETRY) RETURNS GEOMETRY
AS $_$
DECLARE
  addr1 INTEGER;
  addr2 INTEGER;
  result GEOMETRY;
BEGIN
  addr1 := to_number(in_addr1, '999999');
  addr2 := to_number(in_addr2, '999999');
  result = interpolate_from_address(given_address, addr1, addr2, road);
  RETURN result;
END
$_$ LANGUAGE plpgsql;

-- interpolate_from_address(local_address, from_address_l, to_address_l, from_address_r, to_address_r, local_road)
-- This function returns a point along the given geometry (must be linestring)
-- corresponding to the given address.  If the given address is not within
-- the address range of the road, null is returned.
-- This function requires that the address be grouped, such that the second and
-- third arguments are from one side of the street, while the fourth and
-- fifth are from the other.
CREATE OR REPLACE FUNCTION interpolate_from_address(given_address INTEGER, addr1 INTEGER, addr2 INTEGER, in_road GEOMETRY) RETURNS GEOMETRY
AS $_$
DECLARE
  addrwidth INTEGER;
  part DOUBLE PRECISION;
  road GEOMETRY;
  result GEOMETRY;
BEGIN
    IF in_road IS NULL THEN
        RETURN NULL;
    END IF;

    IF geometrytype(in_road) = 'LINESTRING' THEN
      road := in_road;
    ELSIF geometrytype(in_road) = 'MULTILINESTRING' THEN
      road := ST_GeometryN(in_road,1);
    ELSE
      RETURN NULL;
    END IF;

    addrwidth := greatest(addr1,addr2) - least(addr1,addr2);
    part := (given_address - least(addr1,addr2)) / trunc(addrwidth, 1);

    IF addr1 > addr2 THEN
        part := 1 - part;
    END IF;

    IF part < 0 OR part > 1 OR part IS NULL THEN
        part := 0.5;
    END IF;

    result = ST_Line_Interpolate_Point(road, part);
    RETURN result;
END;
$_$ LANGUAGE plpgsql;
