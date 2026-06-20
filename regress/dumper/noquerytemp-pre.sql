insert into spatial_ref_sys(srid,srtext) values (1,'fake["srs"],text');
create table c (i int, g geometry, b boolean, """qColumn" varchar(20));
insert into c values (1,'SRID=1;POINT(0 0)', true, 'quote test');

-- Query dumps must describe and scan the SELECT without creating temp tables.
-- Revoking TEMP makes the regression fail if pgsql2shp materializes the query.
do $$
begin
  execute format('revoke temporary on database %I from %I', current_database(), current_user);
  execute format('revoke temporary on database %I from public', current_database());
end;
$$;
