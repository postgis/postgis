-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2009 Sandro Santilli <strk@keybit.net>, David Zwarg <dzwarg@azavea.com>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

-----------------------------------------------------------------------
-- test bounding box (2D)
-----------------------------------------------------------------------
SELECT
	id,
	env as expected,
	rast::box2d as obtained
FROM rt_box2d_test
WHERE
	rast::box2d::text != env::text;

SELECT
	id,
	env as expected,
	box2d(rast) as obtained
FROM rt_box2d_test
WHERE
	box2d(rast)::text != env::text;

SELECT
	id,
	env as expected,
	box2d(st_convexhull(rast)) as obtained
FROM rt_box2d_test
WHERE
	box2d(st_convexhull(rast))::text != env::text;

SELECT
	id,
	env as expected,
	box2d(st_envelope(rast)) as obtained
FROM rt_box2d_test
WHERE
	box2d(st_envelope(rast))::text != env::text;
