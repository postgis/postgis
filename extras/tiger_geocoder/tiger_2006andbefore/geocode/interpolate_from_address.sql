-- This function converts string addresses to integers and passes them to
-- the other interpolate_from_address function.
CREATE OR REPLACE FUNCTION interpolate_from_address(INTEGER, VARCHAR, VARCHAR, VARCHAR, VARCHAR, GEOMETRY) RETURNS GEOMETRY
AS $_$
DECLARE
  given_address INTEGER;
  addr1 INTEGER;
  addr2 INTEGER;
  addr3 INTEGER;
  addr4 INTEGER;
  road GEOMETRY;
  result GEOMETRY;
BEGIN
  given_address := $1;
  addr1 := to_number($2, '999999');
  addr2 := to_number($3, '999999');
  addr3 := to_number($4, '999999');
  addr4 := to_number($5, '999999');
  road := $6;
  result = interpolate_from_address(given_address, addr1, addr2, addr3, addr4, road);
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
CREATE OR REPLACE FUNCTION interpolate_from_address(INTEGER, INTEGER, INTEGER, INTEGER, INTEGER, GEOMETRY) RETURNS GEOMETRY
AS $_$
DECLARE
  given_address INTEGER;
  lmaxaddr INTEGER := -1;
  rmaxaddr INTEGER := -1;
  lminaddr INTEGER := -1;
  rminaddr INTEGER := -1;
  lfrgreater BOOLEAN;
  rfrgreater BOOLEAN;
  frgreater BOOLEAN;
  addrwidth INTEGER;
  part DOUBLE PRECISION;
  road GEOMETRY;
  result GEOMETRY;
BEGIN
  IF $1 IS NULL THEN
    RETURN NULL;
  ELSE
    given_address := $1;
  END IF;

  IF $6 IS NULL THEN
    RETURN NULL;
  ELSE
    IF geometrytype($6) = 'LINESTRING' THEN
      road := $6;
    ELSIF geometrytype($6) = 'MULTILINESTRING' THEN
      road := geometryn($6,1);
    ELSE
      RETURN NULL;
    END IF;
  END IF;

  IF $2 IS NOT NULL THEN
    lfrgreater := TRUE;
    lmaxaddr := $2;
    lminaddr := $2;
  END IF;

  IF $3 IS NOT NULL THEN
    IF $3 > lmaxaddr OR lmaxaddr = -1 THEN
      lmaxaddr := $3;
      lfrgreater := FALSE;
    END IF;
    IF $3 < lminaddr OR lminaddr = -1 THEN
      lminaddr := $3;
    END IF;
  END IF;

  IF $4 IS NOT NULL THEN
    rmaxaddr := $4;
    rminaddr := $4;
    rfrgreater := TRUE;
  END IF;

  IF $5 IS NOT NULL THEN
    IF $5 > rmaxaddr OR rmaxaddr = -1 THEN
      rmaxaddr := $5;
      rfrgreater := FALSE;
    END IF;
    IF $5 < rminaddr OR rminaddr = -1 THEN
      rminaddr := $5;
    END IF;
  END IF;

  IF given_address >= lminaddr AND given_address <= lmaxaddr THEN
    IF (given_address % 2) = (lminaddr % 2)
        OR (given_address % 2) = (lmaxaddr % 2) THEN
      addrwidth := lmaxaddr - lminaddr;
      part := (given_address - lminaddr) / trunc(addrwidth, 1);
      frgreater := lfrgreater;
    END IF;
  END IF;

  IF given_address >= rminaddr AND given_address <= rmaxaddr THEN
    IF (given_address % 2) = (rminaddr % 2)
        OR (given_address % 2) = (rmaxaddr % 2) THEN
      addrwidth := rmaxaddr - rminaddr;
      part := (given_address - rminaddr) / trunc(addrwidth, 1);
      frgreater := rfrgreater;
    END IF;
  END IF;

  IF frgreater THEN
    part := 1 - part;
  END IF;

  result = line_interpolate_point(road, part);
  RETURN result;
END;
$_$ LANGUAGE plpgsql;
