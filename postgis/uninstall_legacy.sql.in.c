-- $Id: uninstall_legacy.sql.in.c 7065 2011-04-26 12:35:02Z robe $
-- Uninstall Legacy functions useful when you have your applications finally set up to not use legacy functions  --
#include "sqldefines.h"
--- start functions that in theory should never have been used or internal like stuff deprecated
#include "postgis_drop.sql.in.c"
-- Drop Affine family of deprecated functions --
DROP FUNCTION IF EXISTS Affine(geometry,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8,float8);
DROP FUNCTION IF EXISTS Affine(geometry,float8,float8,float8,float8,float8,float8);
DROP FUNCTION IF EXISTS RotateZ(geometry,float8);
DROP FUNCTION IF EXISTS Rotate(geometry,float8);
DROP FUNCTION IF EXISTS RotateX(geometry,float8);
DROP FUNCTION IF EXISTS RotateY(geometry,float8);
DROP FUNCTION IF EXISTS Scale(geometry,float8,float8,float8);
DROP FUNCTION IF EXISTS Scale(geometry,float8,float8);
DROP FUNCTION IF EXISTS Translate(geometry,float8,float8,float8);
DROP FUNCTION IF EXISTS Translate(geometry,float8,float8);
DROP FUNCTION IF EXISTS TransScale(geometry,float8,float8,float8,float8);

-- Other functions --
DROP FUNCTION IF EXISTS AddBBox(geometry);
DROP FUNCTION IF EXISTS AddPoint(geometry,geometry);
DROP FUNCTION IF EXISTS AddPoint(geometry,geometry, integer);
DROP FUNCTION IF EXISTS Area(geometry);
DROP FUNCTION IF EXISTS Area2D(geometry);
DROP FUNCTION IF EXISTS AsBinary(geometry);
DROP FUNCTION IF EXISTS AsBinary(geometry,text);
DROP FUNCTION IF EXISTS AsEWKB(geometry);
DROP FUNCTION IF EXISTS AsEWKB(geometry,text);
DROP FUNCTION IF EXISTS AsEWKT(geometry);
DROP FUNCTION IF EXISTS AsGML(geometry);
DROP FUNCTION IF EXISTS AsGML(geometry,int4);
DROP FUNCTION IF EXISTS AsKML(geometry);
DROP FUNCTION IF EXISTS AsKML(geometry,int4);
DROP FUNCTION IF EXISTS AsKML(int4,geometry,int4);
DROP FUNCTION IF EXISTS AsHEXEWKB(geometry);
DROP FUNCTION IF EXISTS AsHEXEWKB(geometry,text);
DROP FUNCTION IF EXISTS AsSVG(geometry);
DROP FUNCTION IF EXISTS AsSVG(geometry,int4);
DROP FUNCTION IF EXISTS AsSVG(geometry,int4,int4);
DROP FUNCTION IF EXISTS AsText(geometry);
DROP FUNCTION IF EXISTS AZimuth(geometry,geometry);
DROP FUNCTION IF EXISTS BdPolyFromText(text, integer);
DROP FUNCTION IF EXISTS BdMPolyFromText(text, integer);
DROP FUNCTION IF EXISTS Boundary(geometry);
DROP FUNCTION IF EXISTS Buffer(geometry,float8,integer);
DROP FUNCTION IF EXISTS Buffer(geometry,float8);
DROP FUNCTION IF EXISTS BuildArea(geometry);
DROP FUNCTION IF EXISTS Centroid(geometry);
DROP FUNCTION IF EXISTS Combine_Bbox(box3d_extent,geometry);
DROP FUNCTION IF EXISTS Contains(geometry,geometry);
DROP FUNCTION IF EXISTS ConvexHull(geometry);
DROP FUNCTION IF EXISTS Crosses(geometry,geometry);
DROP FUNCTION IF EXISTS Distance(geometry,geometry);
DROP FUNCTION IF EXISTS Difference(geometry,geometry);
DROP FUNCTION IF EXISTS Dimension(geometry);
DROP FUNCTION IF EXISTS Disjoint(geometry,geometry);
DROP FUNCTION IF EXISTS Distance_Sphere(geometry,geometry);
DROP FUNCTION IF EXISTS Distance_Spheroid(geometry,geometry,spheroid);
DROP FUNCTION IF EXISTS DropBBox(geometry);
DROP FUNCTION IF EXISTS Dump(geometry);
DROP FUNCTION IF EXISTS DumpRings(geometry);
DROP FUNCTION IF EXISTS Envelope(geometry);
-- Extent related functions  --
DROP FUNCTION IF EXISTS Estimated_Extent(text,text,text);
DROP FUNCTION IF EXISTS Estimated_Extent(text,text);
DROP FUNCTION IF EXISTS Expand(box2d,float8);
DROP FUNCTION IF EXISTS Expand(box3d,float8);
DROP FUNCTION IF EXISTS Expand(geometry,float8);
DROP AGGREGATE IF EXISTS Extent(geometry);
DROP FUNCTION IF EXISTS Find_Extent(text,text);
DROP FUNCTION IF EXISTS Find_Extent(text,text,text);
--- End Extent related functions --
DROP FUNCTION IF EXISTS EndPoint(geometry);
DROP FUNCTION IF EXISTS ExteriorRing(geometry);
DROP FUNCTION IF EXISTS Force_2d(geometry);
DROP FUNCTION IF EXISTS Force_3d(geometry);
DROP FUNCTION IF EXISTS Force_3dm(geometry);
DROP FUNCTION IF EXISTS Force_3dz(geometry);
DROP FUNCTION IF EXISTS Force_4d(geometry);
DROP FUNCTION IF EXISTS Force_Collection(geometry);
DROP FUNCTION IF EXISTS ForceRHR(geometry);
DROP FUNCTION IF EXISTS SnapToGrid(geometry, float8, float8, float8, float8);
DROP FUNCTION IF EXISTS SnapToGrid(geometry, float8);
DROP FUNCTION IF EXISTS SnapToGrid(geometry, geometry, float8, float8, float8, float8);
DROP FUNCTION IF EXISTS SnapToGrid(geometry, float8, float8);
DROP FUNCTION IF FUNCTION transform(geometry,integer);

