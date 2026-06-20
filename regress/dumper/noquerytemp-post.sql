do $$
begin
  execute format('grant temporary on database %I to %I', current_database(), current_user);
  execute format('grant temporary on database %I to public', current_database());
end;
$$;
drop table c;
delete from spatial_ref_sys where srid = 1;
