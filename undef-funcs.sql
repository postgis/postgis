begin;

delete from pg_amproc where amopclaid = (select oid from pg_opclass where opcname = 'rt_geometry_ops');

delete from pg_amop where amopclaid = (select oid from pg_opclass where opcname = 'rt_geometry_ops');

delete from pg_opclass where opcname = 'rt_geometry_ops';


delete from pg_amproc where amopclaid = (select oid from pg_opclass where opcname = 'gist_geometry_ops');

delete from pg_amop where amopclaid = (select oid from pg_opclass where opcname = 'gist_geometry_ops');

delete from pg_opclass where opcname = 'gist_geometry_ops';


drop OPERATOR > (geometry,geometry);
drop OPERATOR < (geometry,geometry);
drop OPERATOR = (geometry,geometry);
drop OPERATOR ~ (geometry,geometry);
drop OPERATOR @ (geometry,geometry);
drop OPERATOR ~= (geometry,geometry);
drop OPERATOR && (geometry,geometry);
drop OPERATOR &> (geometry,geometry);
drop OPERATOR >> (geometry,geometry);
drop OPERATOR &< (geometry,geometry);

drop OPERATOR << (geometry,geometry);
drop function geometry_size(GEOMETRY,opaque) ; 
drop function geometry_inter(GEOMETRY,GEOMETRY) ; 
drop function geometry_union(GEOMETRY,GEOMETRY) ; 


drop function rtree_decompress(opaque) ;
drop function ggeometry_union(bytea, opaque) ; 
drop function ggeometry_same(opaque, opaque, opaque);
drop function ggeometry_picksplit(opaque, opaque) ; 
drop function ggeometry_penalty(opaque,opaque,opaque) ; 
drop function ggeometry_compress(opaque) ; 
drop function ggeometry_consistent(opaque,GEOMETRY,int4) ; 


drop FUNCTION force_3d(GEOMETRY) ;
drop FUNCTION force_2d(GEOMETRY) ;
drop FUNCTION geometry_eq(GEOMETRY, GEOMETRY) ;
drop FUNCTION geometry_gt(GEOMETRY, GEOMETRY) ;
drop FUNCTION geometry_lt(GEOMETRY, GEOMETRY) ;

drop FUNCTION geometry_same(GEOMETRY, GEOMETRY);
drop FUNCTION geometry_overlap(GEOMETRY, GEOMETRY) ;
drop FUNCTION geometry_contained(GEOMETRY, GEOMETRY) ;
drop function geometry_contain(GEOMETRY, GEOMETRY) ;
drop FUNCTION geometry_right(GEOMETRY, GEOMETRY) ;
drop FUNCTION geometry_left(GEOMETRY, GEOMETRY) ;
drop FUNCTION geometry_overright(GEOMETRY, GEOMETRY) ;
drop FUNCTION geometry_overleft(GEOMETRY, GEOMETRY) ;



drop AGGREGATE extent geometry;
drop FUNCTION combine_bbox(BOX3D,GEOMETRY);
drop FUNCTION truly_inside(GEOMETRY,GEOMETRY);
drop FUNCTION perimeter2d(GEOMETRY);
drop FUNCTION perimeter3d(GEOMETRY);



drop FUNCTION area2d(GEOMETRY);
drop FUNCTION length2d(GEOMETRY);
drop FUNCTION length3d(GEOMETRY);
drop FUNCTION translate(GEOMETRY,float8,float8,float8);
drop FUNCTION summary(GEOMETRY);
drop FUNCTION numb_sub_objs(GEOMETRY);


drop FUNCTION mem_size(GEOMETRY);
drop FUNCTION nrings(GEOMETRY);
drop FUNCTION npoints(GEOMETRY);
drop FUNCTION wkb_XDR(GEOMETRY);
drop FUNCTION wkb_NDR(GEOMETRY);
drop FUNCTION geometry(BOX3D);
drop FUNCTION box3d(GEOMETRY);


drop function geometry_in(opaque);
drop function geometry_out(opaque);
drop FUNCTION BOX3D_in(opaque);
drop FUNCTION BOX3D_out(opaque);

drop TYPE BOX3D ;
drop TYPE WKB ;
drop TYPE GEOMETRY ;


end;