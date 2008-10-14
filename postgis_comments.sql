COMMENT ON FUNCTION AddGeometryColumn(varchar , varchar , integer , varchar , integer ) IS 'args: table_name, column_name, srid, type, dimension - Adds a geometry column to an existing table of attributes.';
			
COMMENT ON FUNCTION AddGeometryColumn(varchar , varchar , varchar , integer , varchar , integer ) IS 'args: schema_name, table_name, column_name, srid, type, dimension - Adds a geometry column to an existing table of attributes.';
			
COMMENT ON FUNCTION AddGeometryColumn(varchar , varchar , varchar , varchar , integer , varchar , integer ) IS 'args: catalog_name, schema_name, table_name, column_name, srid, type, dimension - Adds a geometry column to an existing table of attributes.';
			
COMMENT ON FUNCTION DropGeometryColumn(varchar , varchar ) IS 'args: table_name, column_name - Removes a geometry column from a spatial table.';
			
COMMENT ON FUNCTION DropGeometryColumn(varchar , varchar , varchar ) IS 'args: schema_name, table_name, column_name - Removes a geometry column from a spatial table.';
			
COMMENT ON FUNCTION DropGeometryColumn(varchar , varchar , varchar , varchar ) IS 'args: catalog_name, schema_name, table_name, column_name - Removes a geometry column from a spatial table.';
			
COMMENT ON FUNCTION DropGeometryTable(varchar ) IS 'args: table_name - Drops a table and all its references in geometry_columns.';
			
COMMENT ON FUNCTION DropGeometryTable(varchar , varchar ) IS 'args: schema_name, table_name - Drops a table and all its references in geometry_columns.';
			
COMMENT ON FUNCTION DropGeometryTable(varchar , varchar , varchar ) IS 'args: catalog_name, schema_name, table_name - Drops a table and all its references in geometry_columns.';
			
COMMENT ON FUNCTION PostGIS_Full_Version() IS 'Reports full postgis version and build configuration infos.';
			
COMMENT ON FUNCTION PostGIS_GEOS_Version() IS 'Returns the version number of the GEOS library.';
			
COMMENT ON FUNCTION PostGIS_JTS_Version() IS 'Returns the version number of the JTS library.';
			
COMMENT ON FUNCTION Postgis_Lib_Build_Date() IS 'Returns build date of the PostGIS library.';
			
COMMENT ON FUNCTION PostGIS_Lib_Version() IS 'Returns the version number of the PostGIS library.';
			
COMMENT ON FUNCTION PostGIS_PROJ_Version() IS 'Returns the version number of the PROJ4 library.';
			
COMMENT ON FUNCTION PostGIS_Uses_Stats() IS 'Returns TRUE if STATS usage has been enabled.';
			
COMMENT ON FUNCTION PostGIS_Version() IS 'Returns PostGIS version number and compile-time options.';
			
COMMENT ON FUNCTION UpdateGeometrySRID(varchar , varchar , integer ) IS 'args: table_name, column_name, srid - Updates the SRID of all features in a geometry column, geometry_columns metadata and srid table constraint';
			
COMMENT ON FUNCTION UpdateGeometrySRID(varchar , varchar , varchar , integer ) IS 'args: schema_name, table_name, column_name, srid - Updates the SRID of all features in a geometry column, geometry_columns metadata and srid table constraint';
			
COMMENT ON FUNCTION UpdateGeometrySRID(varchar , varchar , varchar , varchar , integer ) IS 'args: catalog_name, schema_name, table_name, column_name, srid - Updates the SRID of all features in a geometry column, geometry_columns metadata and srid table constraint';
			
COMMENT ON FUNCTION ST_BdPolyFromText(text , integer ) IS 'args: WKT, srid - Construct a Polygon given an arbitrary collection of closed linestrings as a MultiLineString Well-Known text representation.';
			
COMMENT ON FUNCTION ST_BdMPolyFromText(text , integer ) IS 'args: WKT, srid - Construct a MultiPolygon given an arbitrary collection of closed linestrings as a MultiLineString text representation Well-Known text representation.';
			
COMMENT ON FUNCTION ST_GeomFromEWKB(bytea ) IS 'args: EWKB - Return a specified ST_Geometry value from Extended Well-Known Binary representation (EWKB).';
			
COMMENT ON FUNCTION ST_GeomFromEWKT(text ) IS 'args: EWKT - Return a specified ST_Geometry value from Extended Well-Known Text representation (EWKT).';
			
COMMENT ON FUNCTION ST_GeometryFromText(text ) IS 'args: WKT - Return a specified ST_Geometry value from Well-Known Text representation (WKT). This is an alias name for ST_GeomFromText';
			
COMMENT ON FUNCTION ST_GeometryFromText(text , integer ) IS 'args: WKT, srid - Return a specified ST_Geometry value from Well-Known Text representation (WKT). This is an alias name for ST_GeomFromText';
			
COMMENT ON FUNCTION ST_GeomFromText(text ) IS 'args: WKT - Return a specified ST_Geometry value from Well-Known Text representation (WKT).';
			
COMMENT ON FUNCTION ST_GeomFromText(text , integer ) IS 'args: WKT, srid - Return a specified ST_Geometry value from Well-Known Text representation (WKT).';
			
COMMENT ON AGGREGATE ST_MakeLine(geometry) IS 'args: pointfield - Creates a Linestring from point geometries.';
			
COMMENT ON FUNCTION ST_MakeLine(geometry, geometry) IS 'args: point1, point2 - Creates a Linestring from point geometries.';
			
COMMENT ON FUNCTION ST_MakePoint(float, float) IS 'args: x, y - Creates a 2d,3dz or 4d point geometry.';
			
COMMENT ON FUNCTION ST_MakePoint(float, float, float) IS 'args: x, y, z - Creates a 2d,3dz or 4d point geometry.';
			
COMMENT ON FUNCTION ST_MakePoint(float, float, float, float) IS 'args: x, y, z, m - Creates a 2d,3dz or 4d point geometry.';
			
COMMENT ON FUNCTION ST_MakePointM(float, float, float) IS 'args: x, y, m - Creates a point geometry with an x y and m coordinate.';
			
COMMENT ON FUNCTION ST_PointFromText(text ) IS 'args: WKT - Makes a point Geometry from WKT with the given SRID. If SRID is not given, it defaults to unknown.';
			
COMMENT ON FUNCTION ST_PointFromText(text , integer ) IS 'args: WKT, srid - Makes a point Geometry from WKT with the given SRID. If SRID is not given, it defaults to unknown.';
			
COMMENT ON FUNCTION ST_WKTToSQL(text ) IS 'args: WKT - Return a specified ST_Geometry value from Well-Known Text representation (WKT). This is an alias name for ST_GeomFromText';
			
COMMENT ON FUNCTION ST_Dimension(geometry ) IS 'args: g - The inherent dimension of this Geometry object, which must be less than or equal to the coordinate dimension.';
			
COMMENT ON FUNCTION ST_EndPoint(geometry ) IS 'args: g - Returns the last point of a LINESTRING geometry as a POINT.';
			
COMMENT ON FUNCTION ST_Envelope(geometry ) IS 'args: g1 - Returns a geometry representing the bounding box of the supplied geometry.';
			
COMMENT ON FUNCTION ST_GeometryType(geometry ) IS 'args: g1 - Return the geometry type of the ST_Geometry value.';
			
COMMENT ON FUNCTION ST_IsClosed(geometry ) IS 'args: g - Returns TRUE if the LINESTRINGs start and end points are coincident.';
			
COMMENT ON FUNCTION ST_IsValid(geometry ) IS 'args: g - Returns true if the ST_Geometry is well formed.';
			
COMMENT ON FUNCTION ST_NDims(geometry ) IS 'args: g1 - Returns coordinate dimension of the geometry as a small int. Values are: 2,3 or 4.';
			
COMMENT ON FUNCTION ST_NPoints(geometry ) IS 'args: g1 - Return the number of points (vertexes) in a geometry.';
			
COMMENT ON FUNCTION ST_NumPoints(geometry ) IS 'args: g1 - Return the number of points in an ST_LineString or ST_CircularString value.';
			
COMMENT ON FUNCTION ST_IsRing(geometry ) IS 'args: g - Returns TRUE if this LINESTRING is both closed and simple.';
			
COMMENT ON FUNCTION ST_SRID(geometry ) IS 'args: g1 - Returns the spatial reference identifier for the ST_Geometry as defined in spatial_ref_sys table.';
			
COMMENT ON FUNCTION ST_StartPoint(geometry ) IS 'args: geomA - Returns the first point of a LINESTRING geometry as a POINT.';
			
COMMENT ON FUNCTION ST_Summary(geometry ) IS 'args: g - Returns a text summary of the contents of the ST_Geometry.';
			
COMMENT ON FUNCTION ST_AddPoint(geometry, geometry) IS 'args: linestring, point - Adds a point to a LineString before point <position> (0-based index).';
			
COMMENT ON FUNCTION ST_AddPoint(geometry, geometry, integer) IS 'args: linestring, point, position - Adds a point to a LineString before point <position> (0-based index).';
			
COMMENT ON FUNCTION ST_Multi(geometry ) IS 'args: g1 - Returns the geometry as a MULTI* geometry. If the geometry is already a MULTI*, it is returned unchanged.';
			
COMMENT ON FUNCTION ST_RemovePoint(geometry, integer) IS 'args: linestring, offset - Removes point from a linestring. Offset is 0-based.';
			
COMMENT ON FUNCTION ST_Reverse(geometry ) IS 'args: g1 - Returns the geometry with vertex order reversed.';
			
COMMENT ON FUNCTION ST_SetPoint(geometry, integer, geometry) IS 'args: linestring, zerobasedposition, point - Replace point N of linestring with given point. Index is 0-based.';
			
COMMENT ON FUNCTION ST_SetSRID(geometry , integer ) IS 'args: geom, srid - Sets the SRID on a geometry to a particular integer value.';
			
COMMENT ON FUNCTION ST_Transform(geometry , integer ) IS 'args: g1, srid - Returns a new geometry with its coordinates transformed to the SRID referenced by the integer parameter.';
			
COMMENT ON FUNCTION ST_Translate(geometry , float , float ) IS 'args: g1, deltax, deltay - Translates the geometry to a new location using the numeric parameters as offsets. Ie: ST_Translate(geom, X, Y) or ST_Translate(geom, X, Y,Z).';
			
COMMENT ON FUNCTION ST_Translate(geometry , float , float , float ) IS 'args: g1, deltax, deltay, deltaz - Translates the geometry to a new location using the numeric parameters as offsets. Ie: ST_Translate(geom, X, Y) or ST_Translate(geom, X, Y,Z).';
			
COMMENT ON FUNCTION ST_AsBinary(geometry ) IS 'args: g1 - Return the Well-Known Binary (WKB) representation of the geometry without SRID meta data.';
			
COMMENT ON FUNCTION ST_AsBinary(geometry , text ) IS 'args: g1, NDR_or_XDR - Return the Well-Known Binary (WKB) representation of the geometry without SRID meta data.';
			
COMMENT ON FUNCTION ST_AsEWKB(geometry ) IS 'args: g1 - Return the Well-Known Binary (WKB) representation of the geometry with SRID meta data.';
			
COMMENT ON FUNCTION ST_AsEWKB(geometry , text ) IS 'args: g1, NDR_or_XDR - Return the Well-Known Binary (WKB) representation of the geometry with SRID meta data.';
			
COMMENT ON FUNCTION ST_AsEWKT(geometry ) IS 'args: g1 - Return the Well-Known Text (WKT) representation of the geometry with SRID meta data.';
			
COMMENT ON FUNCTION ST_AsGeoJSON(geometry ) IS 'args: g1 - Return the geometry as a GeoJSON element.';
			
COMMENT ON FUNCTION ST_AsGeoJSON(geometry , integer ) IS 'args: g1, max_decimal_digits - Return the geometry as a GeoJSON element.';
			
COMMENT ON FUNCTION ST_AsGeoJSON(geometry , integer , integer ) IS 'args: g1, max_decimal_digits, options - Return the geometry as a GeoJSON element.';
			
COMMENT ON FUNCTION ST_AsGeoJSON(integer , geometry ) IS 'args: version, g1 - Return the geometry as a GeoJSON element.';
			
COMMENT ON FUNCTION ST_AsGeoJSON(integer , geometry , integer ) IS 'args: version, g1, max_decimal_digits - Return the geometry as a GeoJSON element.';
			
COMMENT ON FUNCTION ST_AsGeoJSON(integer , geometry , integer , integer ) IS 'args: version, g1, max_decimal_digits, options - Return the geometry as a GeoJSON element.';
			
COMMENT ON FUNCTION ST_AsGML(geometry ) IS 'args: g1 - Return the geometry as a GML version 2 or 3 element.';
			
COMMENT ON FUNCTION ST_AsGML(geometry , integer ) IS 'args: g1, max_num_decimal_digits - Return the geometry as a GML version 2 or 3 element.';
			
COMMENT ON FUNCTION ST_AsGML(integer , geometry , integer ) IS 'args: version, g1, max_num_decimal_digits - Return the geometry as a GML version 2 or 3 element.';
			
COMMENT ON FUNCTION ST_AsHEXEWKB(geometry , text ) IS 'args: g1, NDRorXDR - Returns a Geometry in HEXEWKB format (as text) using either little-endian (NDR) or big-endian (XDR) encoding.';
			
COMMENT ON FUNCTION ST_AsHEXEWKB(geometry ) IS 'args: g1 - Returns a Geometry in HEXEWKB format (as text) using either little-endian (NDR) or big-endian (XDR) encoding.';
			
COMMENT ON FUNCTION ST_AsKML(geometry ) IS 'args: g1 - Return the geometry as a KML element. Second argument may be used to reduce the maximum number of significant digits used in output (defaults to 15).';
			
COMMENT ON FUNCTION ST_AsKML(geometry , integer ) IS 'args: g1, max_num_decimal_digits - Return the geometry as a KML element. Second argument may be used to reduce the maximum number of significant digits used in output (defaults to 15).';
			
COMMENT ON FUNCTION ST_AsSVG(geometry ) IS 'args: g1 - Returns a Geometry in SVG path data.';
			
COMMENT ON FUNCTION ST_AsSVG(geometry , integer ) IS 'args: g1, rel - Returns a Geometry in SVG path data.';
			
COMMENT ON FUNCTION ST_AsSVG(geometry , integer , integer ) IS 'args: g1, rel, maxdecimaldigits - Returns a Geometry in SVG path data.';
			
COMMENT ON FUNCTION ST_AsText(geometry ) IS 'args: g1 - Return the Well-Known Text (WKT) representation of the geometry without SRID metadata.';
			
COMMENT ON FUNCTION ST_Area(geometry ) IS 'args: g1 - Returns the area of the geometry if it is a polygon or multi-polygon.';
			
COMMENT ON FUNCTION ST_Centroid(geometry ) IS 'args: g1 - Returns the geometric center of a geometry.';
			
COMMENT ON FUNCTION ST_Contains(geometry , geometry ) IS 'args: A, B - Returns true if the geometry B is completely inside geometry A';
			
COMMENT ON FUNCTION ST_Crosses(geometry , geometry ) IS 'args: g1, g2 - Returns TRUE if the supplied geometries have some, but not all, interior points in common.';
			
COMMENT ON FUNCTION ST_Disjoint(geometry, geometry) IS 'args: A, B - Returns TRUE if the Geometries do not "spatially intersect" - if they do not share any space together.';
			
COMMENT ON FUNCTION ST_Distance(geometry , geometry ) IS 'args: g1, g2 - Returns the cartesian distance between two geometries in projected units.';
			
COMMENT ON FUNCTION ST_DWithin(geometry , geometry , double precision ) IS 'args: g1, g2, distance - Returns true if the geometries are within the specified distance of one another';
			
COMMENT ON FUNCTION ST_Equals(geometry , geometry ) IS 'args: A, B - Returns true if the given geometries represent the same geometry. Directionality is ignored.';
			
COMMENT ON FUNCTION ST_Intersects(geometry, geometry) IS 'args: A, B - Returns TRUE if the Geometries "spatially intersect" - (share any portion of space) and FALSE if they dont (they are Disjoint).';
			
COMMENT ON FUNCTION ST_Length(geometry ) IS 'args: g1 - Returns the length of the geometry if it is a linestring or multilinestring.';
			
COMMENT ON FUNCTION ST_OrderingEquals(geometry , geometry ) IS 'args: A, B - Returns true if the given geometries represent the same geometry and points are in the same directional order.';
			
COMMENT ON FUNCTION ST_Overlaps(geometry , geometry ) IS 'args: A, B - Returns TRUE if the Geometries share space, are of the same dimension, but are not completely contained by each other.';
			
COMMENT ON FUNCTION ST_Perimeter(geometry ) IS 'args: g1 - Return the length measurement of the boundary of an ST_Surface or ST_MultiSurface value. (Polygon, Multipolygon)';
			
COMMENT ON FUNCTION ST_PointOnSurface(geometry ) IS 'args: g1 - Returns a POINT guaranteed to lie on the surface.';
			
COMMENT ON FUNCTION ST_Touches(geometry , geometry ) IS 'args: g1, g2 - Returns TRUE if the geometries have at least one point in common, but their interiors do not intersect.';
			
COMMENT ON FUNCTION ST_Within(geometry , geometry ) IS 'args: A, B - Returns true if the geometry A is completely inside geometry B';
			
COMMENT ON FUNCTION ST_Buffer(geometry , float ) IS 'args: g1, radius_of_buffer - Returns a geometry that represents all points whose distance from this Geometry is less than or equal to distance. Calculations are in the Spatial Reference System of this Geometry. The optional third parameter sets the number of segments used to approximate a quarter circle (defaults to 8).';
			
COMMENT ON FUNCTION ST_Buffer(geometry , float , integer ) IS 'args: g1, radius_of_buffer, num_seg_quarter_circle - Returns a geometry that represents all points whose distance from this Geometry is less than or equal to distance. Calculations are in the Spatial Reference System of this Geometry. The optional third parameter sets the number of segments used to approximate a quarter circle (defaults to 8).';
			
COMMENT ON FUNCTION ST_BuildArea(geometry ) IS 'args: A - Creates an areal geometry formed by the constituent linework of given geometry';
			
COMMENT ON AGGREGATE ST_Collect(geometry) IS 'args: g1field - Return a specified ST_Geometry value from a collection of other geometries.';
			
COMMENT ON FUNCTION ST_Collect(geometry, geometry) IS 'args: g1, g2 - Return a specified ST_Geometry value from a collection of other geometries.';
			
COMMENT ON FUNCTION ST_ConvexHull(geometry ) IS 'args: geomA - The convex hull of a geometry represents the minimum closed geometry that encloses all geometries within the set.';
			
COMMENT ON FUNCTION ST_Dump(geometry ) IS 'args: g1 - Returns a set of geometry_dump rows, formed by a geometry (geom).';
			
COMMENT ON FUNCTION ST_MakePolygon(geometry) IS 'args: linestring - Creates a Polygon formed by the given shell. Input geometries must be closed LINESTRINGS.';
			
COMMENT ON FUNCTION ST_MakePolygon(geometry, geometry[]) IS 'args: outerlinestring, interiorlinestrings - Creates a Polygon formed by the given shell. Input geometries must be closed LINESTRINGS.';
			
COMMENT ON FUNCTION ST_Intersection(geometry, geometry) IS 'args: geomA, geomB - Returns a geometry that represents the shared portion of geomA and geomB';
			
COMMENT ON AGGREGATE ST_Polygonize(geometry) IS 'args: geomfield - Aggregate. Creates a GeometryCollection containing possible polygons formed from the constituent linework of a set of geometries.';
			
COMMENT ON AGGREGATE ST_Union(geometry) IS 'args: g1field - Returns a geometry that represents the point set union of the Geometries.';
			
COMMENT ON FUNCTION ST_Union(geometry) IS 'args: g1 - Returns a geometry that represents the point set union of the Geometries.';
			
COMMENT ON FUNCTION ST_Expand(geometry , float) IS 'args: g1, units_to_expand - Returns bounding box expanded in all directions from the bounding box of the input geometry';
			
COMMENT ON FUNCTION ST_Expand(box2d , float) IS 'args: g1, units_to_expand - Returns bounding box expanded in all directions from the bounding box of the input geometry';
			
COMMENT ON FUNCTION ST_Expand(box3d , float) IS 'args: g1, units_to_expand - Returns bounding box expanded in all directions from the bounding box of the input geometry';
			
COMMENT ON AGGREGATE ST_Extent(geometry) IS 'args: geomfield - an aggregate function that returns the bounding box that bounds rows of geometries.';		