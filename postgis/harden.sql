-- This file replaces standard PostGIS functions with their versions
-- actively checking input to be valid.

begin;
create or replace function _ST_FailOnInvalid(geom geometry)
returns boolean as
$$
declare
	v valid_detail;
begin
	v = ST_IsValidDetail(geom);
	if (not v.valid) then
		raise exception 'Input geometry of type % with % points is invalid at location %: %',
		ST_GeometryType(geom), ST_NPoints(geom), ST_AsText(v.location), v.reason;
	end if;
	return true;
end
$$
language plpgsql volatile strict parallel safe cost 1;

alter function ST_Intersection(geometry, geometry) rename to ST_Intersection_real;
create function ST_Intersection(geom1 geometry, geom2 geometry)
returns geometry
as $$
select ST_Intersection_real(geom1, geom2) where _ST_FailOnInvalid(geom1) and _ST_FailOnInvalid(geom2)
$$ language sql parallel safe;

alter function ST_Union(geometry, geometry) rename to ST_Union_real;
create function ST_Union(geom1 geometry, geom2 geometry)
returns geometry
as $$
select ST_Union_real(geom1, geom2) where _ST_FailOnInvalid(geom1) and _ST_FailOnInvalid(geom2)
$$ language sql parallel safe;

alter function ST_Difference(geometry, geometry) rename to ST_Difference_real;
create function ST_Difference(geom1 geometry, geom2 geometry)
returns geometry
as $$
select ST_Difference_real(geom1, geom2) where _ST_FailOnInvalid(geom1) and _ST_FailOnInvalid(geom2)
$$ language sql parallel safe;

alter function ST_SymDifference(geometry, geometry) rename to ST_SymDifference_real;
create function ST_SymDifference(geom1 geometry, geom2 geometry)
returns geometry
as $$
select ST_SymDifference_real(geom1, geom2) where _ST_FailOnInvalid(geom1) and _ST_FailOnInvalid(geom2)
$$ language sql parallel safe;

alter function ST_Buffer(geometry, float) rename to ST_Buffer_real;
create function ST_Buffer(geom1 geometry, r float)
returns geometry
as $$
select ST_Buffer_real(geom1, r) where _ST_FailOnInvalid(geom1)
$$ language sql parallel safe;

alter function ST_PointOnSurface(geometry) rename to ST_PointOnSurface_real;
create function ST_PointOnSurface(geom1 geometry)
returns geometry
as $$
select ST_PointOnSurface_real(geom1) where _ST_FailOnInvalid(geom1)
$$ language sql parallel safe;

commit;

