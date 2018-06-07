
CREATE SCHEMA testschema ;
SET search_path TO testschema,public ;

CREATE OR REPLACE FUNCTION random_int(low int, high int)
	RETURNS int AS $$
BEGIN
	RETURN floor(random()* (high-low + 1) + low);
END;
$$ LANGUAGE 'plpgsql' STRICT;

CREATE OR REPLACE FUNCTION random_float(low float, high float)
	RETURNS float AS $$
BEGIN
	RETURN random()* (high-low + 1) + low;
END;
$$ LANGUAGE 'plpgsql' STRICT;

CREATE OR REPLACE FUNCTION random_geompoint(lowx float, highx float, lowy float, highy float)
	RETURNS geometry AS $$
BEGIN
	RETURN st_point(random_float(lowx, highx), random_float(lowy, highy));
END;
$$ LANGUAGE 'plpgsql' STRICT;

CREATE OR REPLACE FUNCTION random_geomlinestring(lowx float, highx float,
		lowy float, highy float, maxvertices int)
	RETURNS geometry AS $$
DECLARE
	result geometry[];
BEGIN
	SELECT array_agg(random_geompoint(lowx, highx, lowy, highy)) INTO result
	FROM generate_series (1, random_int(2, maxvertices)) AS x;
	RETURN st_geometryn(st_unaryunion(ST_LineFromMultiPoint(st_collect(result))),1);
END;
$$ LANGUAGE 'plpgsql' STRICT;

CREATE OR REPLACE FUNCTION random_geompolygon(lowx float, highx float,
	lowy float, highy float, maxvertices int)
	RETURNS geometry AS $$
DECLARE
	pointarr geometry[];
	noVertices int;
	t timestamptz;
	valid geometry;
BEGIN
	noVertices = random_int(3, maxvertices);
	for i in 1..noVertices
	loop
		pointarr[i] = random_geompoint(lowx, highx, lowy, highy);
	end loop;
	pointarr[noVertices+1] = pointarr[1];
	return st_geometryn(st_makevalid(st_makepolygon(st_makeline(pointarr))),1);
END;
$$ LANGUAGE 'plpgsql' STRICT;

CREATE OR REPLACE FUNCTION random_geommultipoint(lowx float, highx float,
		lowy float, highy float, maxcard int)
	RETURNS geometry AS $$
DECLARE
	result geometry[];
BEGIN
	SELECT array_agg(random_geompoint(lowx, highx, lowy, highy)) INTO result
	FROM generate_series (1, random_int(1, maxcard)) AS x;
	RETURN st_collect(result);
END;
$$ LANGUAGE 'plpgsql' STRICT;

CREATE OR REPLACE FUNCTION random_geommultilinestring(lowx float, highx float,
		lowy float, highy float, maxvertices int, maxcard int)
	RETURNS geometry AS $$
DECLARE
	result geometry[];
BEGIN
	SELECT array_agg(random_geomlinestring(lowx, highx, lowy, highy, maxvertices)) INTO result
	FROM generate_series (1, random_int(2, maxcard)) AS x;
	RETURN st_unaryunion(st_collect(result));
END;
$$ LANGUAGE 'plpgsql' STRICT;


CREATE OR REPLACE FUNCTION random_geommultipolygon(lowx float, highx float,
		lowy float, highy float, maxvertices int, maxcard int)
	RETURNS geometry AS $$
DECLARE
	result geometry[];
BEGIN
	SELECT array_agg(random_geompolygon(lowx, highx, lowy, highy, maxvertices)) INTO result
	FROM generate_series (1, random_int(2, maxcard)) AS x;
	RETURN st_makevalid(st_collect(result));
END;
$$ LANGUAGE 'plpgsql' STRICT;

CREATE OR REPLACE FUNCTION qnodes(q text) RETURNS text
LANGUAGE 'plpgsql' AS
$$
DECLARE
  exp TEXT;
  mat TEXT[];
  ret TEXT;
BEGIN
  FOR exp IN EXECUTE 'EXPLAIN ' || q
  LOOP
    --RAISE NOTICE 'EXP: %', exp;
    mat := regexp_matches(exp, ' *(?:-> *)?(.*Scan)');
    --RAISE NOTICE 'MAT: %', mat;
    IF mat IS NOT NULL THEN
      ret := mat[1];
    END IF;
    --RAISE NOTICE 'RET: %', ret;
  END LOOP;
  RETURN ret;
END;
$$;

-------------------------------------------------------------------------------
/* Create random geometries of various types */

select setseed(0.77777) ;

create table tbl_geompoint as
select k, random_geompoint(0, 100, 0, 100) as g
from generate_series(1,50) k;

create table tbl_geomlinestring as
select k, random_geomlinestring(0, 100, 0, 100, 10) as g
from generate_series(1,50) k;

create table tbl_geompolygon as
select k, random_geompolygon(0, 100, 0, 100, 10) as g
from generate_series(1,50) k;

create table tbl_geommultipoint as
select k, random_geommultipoint(0, 100, 0, 100, 10) as g
from generate_series(1,50) k;

create table tbl_geommultilinestring as
select k, random_geommultilinestring(0, 100, 0, 100, 10, 10) as g
from generate_series(1,50) k;

create table tbl_geommultipolygon as
select k, random_geommultipolygon(0, 100, 0, 100, 10, 10) as g
from generate_series(1,50) k;

create table tbl_geomcollection (
	k serial,
	g geometry
);
insert into tbl_geomcollection (g)
(select g from tbl_geompoint) union
(select g from tbl_geomlinestring)  union
(select g from tbl_geompolygon) union
(select g from tbl_geommultipoint)  union
(select g from tbl_geommultilinestring) union
(select g from tbl_geommultipolygon);

create table test_spgist_idx_2d(
	op char(3),
	noidx bigint,
	noidxscan varchar(32),
	spgistidx bigint,
	spgidxscan varchar(32));

-------------------------------------------------------------------------------

set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '<<', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g << t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g << t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '&<', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &< t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &< t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '&&', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g && t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g && t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '&>', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &> t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &> t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '>>', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g >> t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g >> t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '~=', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~= t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~= t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '~', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~ t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~ t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '@', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g @ t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g @ t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '&<|', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &<| t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &<| t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '<<|', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g <<| t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g <<| t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '|>>', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g |>> t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g |>> t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '|&>', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g |&> t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g |&> t2.g;

------------------------------------------------------------------------------

create index tbl_geomcollection_spgist_2d_idx on tbl_geomcollection using spgist(g);

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g << t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g << t2.g ')
where op = '<<';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &< t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &< t2.g ')
where op = '&<';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g && t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g && t2.g ')
where op = '&&';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &> t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &> t2.g ')
where op = '&>';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g >> t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g >> t2.g ')
where op = '>>';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~= t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~= t2.g ')
where op = '~=';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~ t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~ t2.g ')
where op = '~';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g @ t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g @ t2.g ')
where op = '@';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &<| t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &<| t2.g ')
where op = '&<|';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g <<| t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g <<| t2.g ')
where op = '<<|';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g |>> t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g |>> t2.g ')
where op = '|>>';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g |&> t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g |&> t2.g ')
where op = '|&>';

-------------------------------------------------------------------------------

select * from test_spgist_idx_2d;

-------------------------------------------------------------------------------

SET search_path TO "$user",public ;
DROP SCHEMA testschema CASCADE ;