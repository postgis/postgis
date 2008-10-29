
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
			
COMMENT ON FUNCTION PostGIS_Lib_Build_Date() IS 'Returns build date of the PostGIS library.';
			
COMMENT ON FUNCTION PostGIS_Lib_Version() IS 'Returns the version number of the PostGIS library.';
			
COMMENT ON FUNCTION PostGIS_PROJ_Version() IS 'Returns the version number of the PROJ4 library.';
			
COMMENT ON FUNCTION PostGIS_Scripts_Build_Date() IS 'Returns build date of the PostGIS scripts.';
			
COMMENT ON FUNCTION PostGIS_Scripts_Installed() IS 'Returns version of the postgis scripts installed in this database.';
			
COMMENT ON FUNCTION PostGIS_Scripts_Released() IS 'Returns the version number of the lwpostgis.sql script released with the installed postgis lib.';
			
COMMENT ON FUNCTION PostGIS_Uses_Stats() IS 'Returns TRUE if STATS usage has been enabled.';
			
COMMENT ON FUNCTION PostGIS_Version() IS 'Returns PostGIS version number and compile-time options.';
	
COMMENT ON FUNCTION Probe_Geometry_Columns() IS 'Scans all tables with PostGIS geometry constraints and adds them to the geometry_columns table if they are not there.';
			
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
			
COMMENT ON FUNCTION ST_Point(float , float ) IS 'args: x_lon, y_lat - Returns an ST_Point with the given coordinate values. OGC alias for ST_MakePoint.';
			
COMMENT ON FUNCTION ST_PointFromText(text ) IS 'args: WKT - Makes a point Geometry from WKT with the given SRID. If SRID is not given, it defaults to unknown.';
			
COMMENT ON FUNCTION ST_PointFromText(text , integer ) IS 'args: WKT, srid - Makes a point Geometry from WKT with the given SRID. If SRID is not given, it defaults to unknown.';
			
COMMENT ON FUNCTION ST_WKTToSQL(text ) IS 'args: WKT - Return a specified ST_Geometry value from Well-Known Text representation (WKT). This is an alias name for ST_GeomFromText';
			
COMMENT ON FUNCTION GeometryType(geometry ) IS 'args: geomA - Returns the type of the geometry as a string. Eg: LINESTRING, POLYGON, MULTIPOINT, etc.';
			
COMMENT ON FUNCTION ST_Boundary(geometry ) IS 'args: geomA - Returns the closure of the combinatorial boundary of this Geometry.';
			
COMMENT ON FUNCTION ST_Dimension(geometry ) IS 'args: g - The inherent dimension of this Geometry object, which must be less than or equal to the coordinate dimension.';
			
COMMENT ON FUNCTION ST_EndPoint(geometry ) IS 'args: g - Returns the last point of a LINESTRING geometry as a POINT.';
			
COMMENT ON FUNCTION ST_Envelope(geometry ) IS 'args: g1 - Returns a geometry representing the bounding box of the supplied geometry.';
			
COMMENT ON FUNCTION ST_ExteriorRing(geometry ) IS 'args: a_polygon - Returns a line string representing the exterior ring of the POLYGON geometry. Return NULL if the geometry is not a polygon. Will not work with MULTIPOLYGON';
			
COMMENT ON FUNCTION ST_GeometryN(geometry , integer ) IS 'args: geomA, n - Return the 1-based Nth geometry if the geometry is a GEOMETRYCOLLECTION, MULTIPOINT, MULTILINESTRING or MULTIPOLYGON. Otherwise, return NULL.';
			
COMMENT ON FUNCTION ST_GeometryType(geometry ) IS 'args: g1 - Return the geometry type of the ST_Geometry value.';
			
COMMENT ON FUNCTION ST_InteriorRingN(geometry , integer ) IS 'args: a_polygon, n - Return the Nth interior linestring ring of the polygon geometry. Return NULL if the geometry is not a polygon or the given N is out of range.';
			
COMMENT ON FUNCTION ST_IsClosed(geometry ) IS 'args: g - Returns TRUE if the LINESTRINGs start and end points are coincident.';
			
COMMENT ON FUNCTION ST_IsEmpty(geometry ) IS 'args: geomA - Returns true if this Geometry is an empty geometry . If true, then this Geometry represents the empty point set - i.e. GEOMETRYCOLLECTION(EMPTY).';
			
COMMENT ON FUNCTION ST_IsRing(geometry ) IS 'args: g - Returns TRUE if this LINESTRING is both closed and simple.';
			
COMMENT ON FUNCTION ST_IsSimple(geometry ) IS 'args: geomA - Returns (TRUE) if this Geometry has no anomalous geometric points, such as self intersection or self tangency.';
			
COMMENT ON FUNCTION ST_IsValid(geometry ) IS 'args: g - Returns true if the ST_Geometry is well formed.';
			
COMMENT ON FUNCTION ST_M(geometry ) IS 'args: a_point - Return the M coordinate of the point, or NULL if not available. Input must be a point.';
			
COMMENT ON FUNCTION ST_NDims(geometry ) IS 'args: g1 - Returns coordinate dimension of the geometry as a small int. Values are: 2,3 or 4.';
			
COMMENT ON FUNCTION ST_NPoints(geometry ) IS 'args: g1 - Return the number of points (vertexes) in a geometry.';
			
COMMENT ON FUNCTION ST_NumGeometries(geometry ) IS 'args: a_multi_or_geomcollection - If geometry is a GEOMETRYCOLLECTION (or MULTI*) return the number of geometries, otherwise return NULL.';
			
COMMENT ON FUNCTION ST_NumInteriorRings(geometry ) IS 'args: a_polygon - Return the number of interior rings of the first polygon in the geometry. This will work with both POLYGON and MULTIPOLYGON types but only looks at the first polygon. Return NULL if there is no polygon in the geometry.';
			
COMMENT ON FUNCTION ST_NumInteriorRing(geometry ) IS 'args: a_polygon - Return the number of interior rings of the first polygon in the geometry. Synonym to ST_NumInteriorRings.';
			
COMMENT ON FUNCTION ST_NumPoints(geometry ) IS 'args: g1 - Return the number of points in an ST_LineString or ST_CircularString value.';
			
COMMENT ON FUNCTION ST_PointN(geometry , integer ) IS 'args: a_linestring, n - Return the Nth point in the first linestring or circular linestring in the geometry. Return NULL if there is no linestring in the geometry.';
			
COMMENT ON FUNCTION ST_SRID(geometry ) IS 'args: g1 - Returns the spatial reference identifier for the ST_Geometry as defined in spatial_ref_sys table.';
			
COMMENT ON FUNCTION ST_StartPoint(geometry ) IS 'args: geomA - Returns the first point of a LINESTRING geometry as a POINT.';
			
COMMENT ON FUNCTION ST_Summary(geometry ) IS 'args: g - Returns a text summary of the contents of the ST_Geometry.';
			
COMMENT ON FUNCTION ST_X(geometry ) IS 'args: a_point - Return the X coordinate of the point, or NULL if not available. Input must be a point.';
			
COMMENT ON FUNCTION ST_Y(geometry ) IS 'args: a_point - Return the Y coordinate of the point, or NULL if not available. Input must be a point.';
			
COMMENT ON FUNCTION ST_Z(geometry ) IS 'args: a_point - Return the Z coordinate of the point, or NULL if not available. Input must be a point.';
			
COMMENT ON FUNCTION ST_AddPoint(geometry, geometry) IS 'args: linestring, point - Adds a point to a LineString before point <position> (0-based index).';
			
COMMENT ON FUNCTION ST_AddPoint(geometry, geometry, integer) IS 'args: linestring, point, position - Adds a point to a LineString before point <position> (0-based index).';
			
COMMENT ON FUNCTION ST_Affine(geometry , float , float , float , float , float , float , float , float , float , float , float , float ) IS 'args: geomA, a, b, c, d, e, f, g, h, i, xoff, yoff, zoff - Applies a 3d affine transformation to the geometry to do things like translate, rotate, scale in one step.';
			
COMMENT ON FUNCTION ST_Affine(geometry , float , float , float , float , float , float ) IS 'args: geomA, a, b, d, e, xoff, yoff - Applies a 3d affine transformation to the geometry to do things like translate, rotate, scale in one step.';
			
COMMENT ON FUNCTION ST_ForceRHR(geometry
						) IS 'args: g - Forces the orientation of the vertices in a polygon to follow the Right-Hand-Rule.';
			
COMMENT ON FUNCTION ST_LineMerge(geometry ) IS 'args: amultilinestring - Returns a (set of) LineString(s) formed by sewing together a MULTILINESTRING.';
			
COMMENT ON FUNCTION ST_Multi(geometry ) IS 'args: g1 - Returns the geometry as a MULTI* geometry. If the geometry is already a MULTI*, it is returned unchanged.';
			
COMMENT ON FUNCTION ST_RemovePoint(geometry, integer) IS 'args: linestring, offset - Removes point from a linestring. Offset is 0-based.';
			
COMMENT ON FUNCTION ST_Reverse(geometry ) IS 'args: g1 - Returns the geometry with vertex order reversed.';
			
COMMENT ON FUNCTION ST_Rotate(geometry, float) IS 'args: geomA, rotZRadians - This is a synonym for ST_RotateZ';
			
COMMENT ON FUNCTION ST_RotateX(geometry, float) IS 'args: geomA, rotRadians - Rotate a geometry rotRadians about the X axis.';
			
COMMENT ON FUNCTION ST_RotateY(geometry, float) IS 'args: geomA, rotRadians - Rotate a geometry rotRadians about the Y axis.';
			
COMMENT ON FUNCTION ST_RotateZ(geometry, float) IS 'args: geomA, rotRadians - Rotate a geometry rotRadians about the Z axis.';
			
COMMENT ON FUNCTION ST_Scale(geometry , float, float, float) IS 'args: geomA, XFactor, YFactor, ZFactor - Scales the geometry to a new size by multiplying the ordinates with the parameters. Ie: ST_Scale(geom, Xfactor, Yfactor, Zfactor).';
			
COMMENT ON FUNCTION ST_Scale(geometry , float, float) IS 'args: geomA, XFactor, YFactor - Scales the geometry to a new size by multiplying the ordinates with the parameters. Ie: ST_Scale(geom, Xfactor, Yfactor, Zfactor).';
			
COMMENT ON FUNCTION ST_Segmentize(geometry , float ) IS 'args: geomA, max_length - Return a modified geometry having no segment longer than the given distance. Distance computation is performed in 2d only.';
			
COMMENT ON FUNCTION ST_SetPoint(geometry, integer, geometry) IS 'args: linestring, zerobasedposition, point - Replace point N of linestring with given point. Index is 0-based.';
			
COMMENT ON FUNCTION ST_SetSRID(geometry , integer ) IS 'args: geom, srid - Sets the SRID on a geometry to a particular integer value.';
			
COMMENT ON FUNCTION ST_SnapToGrid(geometry , float , float , float , float ) IS 'args: geomA, originX, originY, sizeX, sizeY - Snap all points of the input geometry to the grid defined by its origin and cell size. Remove consecutive points falling on the same cell, eventually returning NULL if output points are not enough to define a geometry of the given type. Collapsed geometries in a collection are stripped from it. Useful for reducing precision.';
			
COMMENT ON FUNCTION ST_SnapToGrid(geometry , float , float ) IS 'args: geomA, sizeX, sizeY - Snap all points of the input geometry to the grid defined by its origin and cell size. Remove consecutive points falling on the same cell, eventually returning NULL if output points are not enough to define a geometry of the given type. Collapsed geometries in a collection are stripped from it. Useful for reducing precision.';
			
COMMENT ON FUNCTION ST_SnapToGrid(geometry , float ) IS 'args: geomA, size - Snap all points of the input geometry to the grid defined by its origin and cell size. Remove consecutive points falling on the same cell, eventually returning NULL if output points are not enough to define a geometry of the given type. Collapsed geometries in a collection are stripped from it. Useful for reducing precision.';
			
COMMENT ON FUNCTION ST_SnapToGrid(geometry , geometry , float , float , float , float ) IS 'args: geomA, pointOrigin, sizeX, sizeY, sizeZ, sizeM - Snap all points of the input geometry to the grid defined by its origin and cell size. Remove consecutive points falling on the same cell, eventually returning NULL if output points are not enough to define a geometry of the given type. Collapsed geometries in a collection are stripped from it. Useful for reducing precision.';
			
COMMENT ON FUNCTION ST_Transform(geometry , integer ) IS 'args: g1, srid - Returns a new geometry with its coordinates transformed to the SRID referenced by the integer parameter.';
			
COMMENT ON FUNCTION ST_Translate(geometry , float , float ) IS 'args: g1, deltax, deltay - Translates the geometry to a new location using the numeric parameters as offsets. Ie: ST_Translate(geom, X, Y) or ST_Translate(geom, X, Y,Z).';
			
COMMENT ON FUNCTION ST_Translate(geometry , float , float , float ) IS 'args: g1, deltax, deltay, deltaz - Translates the geometry to a new location using the numeric parameters as offsets. Ie: ST_Translate(geom, X, Y) or ST_Translate(geom, X, Y,Z).';
			
COMMENT ON FUNCTION ST_TransScale(geometry , float, float, float, float) IS 'args: geomA, deltaX, deltaY, XFactor, YFactor - Translates the geometry using the deltaX and deltaY args, then scales it using the XFactor, YFactor args, working in 2D only.';
			
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
			
COMMENT ON FUNCTION ST_Azimuth(geometry , geometry ) IS 'args: pointA, pointB - Returns the angle in radians from the horizontal of the vector defined by pointA and pointB';
			
COMMENT ON FUNCTION ST_Centroid(geometry ) IS 'args: g1 - Returns the geometric center of a geometry.';
			
COMMENT ON FUNCTION ST_Contains(geometry , geometry ) IS 'args: geomA, geomB - Returns true if the geometry B is completely inside geometry A.';
			
COMMENT ON FUNCTION ST_Covers(geometry , geometry ) IS 'args: geomA, geomB - Returns 1 (TRUE) if no point in Geometry B is outside Geometry A';
			
COMMENT ON FUNCTION ST_CoveredBy(geometry , geometry ) IS 'args: geomA, geomB - Returns 1 (TRUE) if no point in Geometry A is outside Geometry B';
			
COMMENT ON FUNCTION ST_Crosses(geometry , geometry ) IS 'args: g1, g2 - Returns TRUE if the supplied geometries have some, but not all, interior points in common.';
			
COMMENT ON FUNCTION ST_Disjoint(geometry, geometry) IS 'args: A, B - Returns TRUE if the Geometries do not "spatially intersect" - if they do not share any space together.';
			
COMMENT ON FUNCTION ST_Distance(geometry , geometry ) IS 'args: g1, g2 - Returns the 2-dimensional cartesian minimum distance between two geometries in projected units.';
			
COMMENT ON FUNCTION ST_Distance_Sphere(geometry , geometry ) IS 'args: pointlonlatA, pointlonlatB - Returns linear distance in meters between two lon/lat points. Uses a spherical earth and radius of 6370986 meters. Faster than , but less accurate. Only implemented for points.';
			
COMMENT ON FUNCTION ST_Distance_Spheroid(geometry , geometry , spheroid ) IS 'args: pointlonlatA, pointlonlatB, measurement_spheroid - Returns linear distance between two lon/lat points given a particular spheroid. Currently only implemented for points.';
			
COMMENT ON FUNCTION ST_DWithin(geometry , geometry , double precision ) IS 'args: g1, g2, distance - Returns true if the geometries are within the specified distance of one another';
			
COMMENT ON FUNCTION ST_Equals(geometry , geometry ) IS 'args: A, B - Returns true if the given geometries represent the same geometry. Directionality is ignored.';
			
COMMENT ON FUNCTION ST_Intersects(geometry, geometry) IS 'args: A, B - Returns TRUE if the Geometries "spatially intersect" - (share any portion of space) and FALSE if they dont (they are Disjoint).';
			
COMMENT ON FUNCTION ST_Length(geometry ) IS 'args: a_2dlinestring - Returns the 2d length of the geometry if it is a linestring or multilinestring.';
			
COMMENT ON FUNCTION ST_Length2D(geometry ) IS 'args: a_2dlinestring - Returns the 2-dimensional length of the geometry if it is a linestring or multi-linestring. This is an alias for ST_Length';
			
COMMENT ON FUNCTION ST_Length3D(geometry ) IS 'args: a_3dlinestring - Returns the 3-dimensional or 2-dimensional length of the geometry if it is a linestring or multi-linestring.';
			
COMMENT ON FUNCTION ST_Length_Spheroid(geometry , spheroid ) IS 'args: a_linestring, a_spheroid - Calculates the 2D or 3D length of a linestring/multilinestring on an ellipsoid. This is useful if the coordinates of the geometry are in longitude/latitude and a length is desired without reprojection.';
			
COMMENT ON FUNCTION ST_Length2D_Spheroid(geometry , spheroid ) IS 'args: a_linestring, a_spheroid - Calculates the 2D length of a linestring/multilinestring on an ellipsoid. This is useful if the coordinates of the geometry are in longitude/latitude and a length is desired without reprojection.';
			
COMMENT ON FUNCTION ST_Length3D_Spheroid(geometry , spheroid ) IS 'args: a_linestring, a_spheroid - Calculates the length of a geometry on an ellipsoid, taking the elevation into account. This is just an alias for ST_Length_Spheroid.';
			
COMMENT ON FUNCTION ST_Max_Distance(geometry , geometry ) IS 'args: g1, g2 - Returns the 2-dimensional largest distance between two geometries in projected units.';
			
COMMENT ON FUNCTION ST_OrderingEquals(geometry , geometry ) IS 'args: A, B - Returns true if the given geometries represent the same geometry and points are in the same directional order.';
			
COMMENT ON FUNCTION ST_Overlaps(geometry , geometry ) IS 'args: A, B - Returns TRUE if the Geometries share space, are of the same dimension, but are not completely contained by each other.';
			
COMMENT ON FUNCTION ST_Perimeter(geometry ) IS 'args: g1 - Return the length measurement of the boundary of an ST_Surface or ST_MultiSurface value. (Polygon, Multipolygon)';
			
COMMENT ON FUNCTION ST_Perimeter2D(geometry ) IS 'args: geomA - Returns the 2-dimensional perimeter of the geometry, if it is a polygon or multi-polygon. This is currently an alias for ST_Perimeter.';
			
COMMENT ON FUNCTION ST_Perimeter3D(geometry ) IS 'args: geomA - Returns the 3-dimensional perimeter of the geometry, if it is a polygon or multi-polygon.';
			
COMMENT ON FUNCTION ST_PointOnSurface(geometry ) IS 'args: g1 - Returns a POINT guaranteed to lie on the surface.';
			
COMMENT ON FUNCTION ST_Relate(geometry , geometry ) IS 'args: geomA, geomB - Returns true if this Geometry is spatially related to anotherGeometry, by testing for intersections between the Interior, Boundary and Exterior of the two geometries as specified by the values in the intersectionPatternMatrix. If no intersectionPatternMatrix is passed in, then returns the maximum intersectionPatternMatrix that relates the 2 geometries.';
			
COMMENT ON FUNCTION ST_Relate(geometry , geometry , text ) IS 'args: geomA, geomB, intersectionPatternMatrix - Returns true if this Geometry is spatially related to anotherGeometry, by testing for intersections between the Interior, Boundary and Exterior of the two geometries as specified by the values in the intersectionPatternMatrix. If no intersectionPatternMatrix is passed in, then returns the maximum intersectionPatternMatrix that relates the 2 geometries.';
			
COMMENT ON FUNCTION ST_Touches(geometry , geometry ) IS 'args: g1, g2 - Returns TRUE if the geometries have at least one point in common, but their interiors do not intersect.';
			
COMMENT ON FUNCTION ST_Within(geometry , geometry ) IS 'args: A, B - Returns true if the geometry A is completely inside geometry B';
			
COMMENT ON FUNCTION ST_Buffer(geometry , float ) IS 'args: g1, radius_of_buffer - Returns a geometry that represents all points whose distance from this Geometry is less than or equal to distance. Calculations are in the Spatial Reference System of this Geometry. The optional third parameter sets the number of segments used to approximate a quarter circle (defaults to 8).';
			
COMMENT ON FUNCTION ST_Buffer(geometry , float , integer ) IS 'args: g1, radius_of_buffer, num_seg_quarter_circle - Returns a geometry that represents all points whose distance from this Geometry is less than or equal to distance. Calculations are in the Spatial Reference System of this Geometry. The optional third parameter sets the number of segments used to approximate a quarter circle (defaults to 8).';
			
COMMENT ON FUNCTION ST_BuildArea(geometry ) IS 'args: A - Creates an areal geometry formed by the constituent linework of given geometry';
			
COMMENT ON AGGREGATE ST_Collect(geometry) IS 'args: g1field - Return a specified ST_Geometry value from a collection of other geometries.';
			
COMMENT ON FUNCTION ST_Collect(geometry, geometry) IS 'args: g1, g2 - Return a specified ST_Geometry value from a collection of other geometries.';
			
COMMENT ON FUNCTION ST_ConvexHull(geometry ) IS 'args: geomA - The convex hull of a geometry represents the minimum closed geometry that encloses all geometries within the set.';
			
COMMENT ON FUNCTION ST_Difference(geometry , geometry ) IS 'args: geomA, geomB - Returns a geometry that represents that part of geometry A that does not intersect with geometry B.';
			
COMMENT ON FUNCTION ST_Dump(geometry ) IS 'args: g1 - Returns a set of geometry_dump rows, formed by a geometry (geom).';
			
COMMENT ON FUNCTION ST_MakePolygon(geometry) IS 'args: linestring - Creates a Polygon formed by the given shell. Input geometries must be closed LINESTRINGS.';
			
COMMENT ON FUNCTION ST_MakePolygon(geometry, geometry[]) IS 'args: outerlinestring, interiorlinestrings - Creates a Polygon formed by the given shell. Input geometries must be closed LINESTRINGS.';
			
COMMENT ON FUNCTION ST_Intersection(geometry, geometry) IS 'args: geomA, geomB - Returns a geometry that represents the shared portion of geomA and geomB';
			
COMMENT ON AGGREGATE ST_MemUnion(geometry) IS 'args: geomfield - Same as ST_Union, only memory-friendly (uses less memory and more processor time).';
			
COMMENT ON AGGREGATE ST_Polygonize(geometry) IS 'args: geomfield - Aggregate. Creates a GeometryCollection containing possible polygons formed from the constituent linework of a set of geometries.';
			
COMMENT ON FUNCTION ST_Shift_Longitude(geometry ) IS 'args: geomA - Reads every point/vertex in every component of every feature in a geometry, and if the longitude coordinate is <0, adds 360 to it. The result would be a 0-360 version of the data to be plotted in a 180 centric map';
			
COMMENT ON FUNCTION ST_Simplify(geometry, float) IS 'args: geomA, tolerance - Returns a "simplified" version of the given geometry using the Douglas-Peuker algorithm.';
			
COMMENT ON FUNCTION ST_SimplifyPreserveTopology(geometry, float) IS 'args: geomA, tolerance - Returns a "simplified" version of the given geometry using the Douglas-Peuker algorithm. Will avoid creating derived geometries (polygons in particular) that are invalid.';
			
COMMENT ON FUNCTION ST_SymDifference(geometry , geometry ) IS 'args: geomA, geomB - Returns a geometry that represents the portions of A and B that do not intersect. It is called a symmetric difference because ST_SymDifference(A,B) = ST_SymDifference(B,A).';
			
COMMENT ON AGGREGATE ST_Union(geometry) IS 'args: g1field - Returns a geometry that represents the point set union of the Geometries.';
			
COMMENT ON FUNCTION ST_Union(geometry) IS 'args: g1 - Returns a geometry that represents the point set union of the Geometries.';
			
COMMENT ON FUNCTION ST_Line_Interpolate_Point(geometry , float ) IS 'args: a_linestring, a_fraction - Returns a point interpolated along a line. Second argument is a float8 between 0 and 1 representing fraction of total length of linestring the point has to be located.';
			
COMMENT ON FUNCTION ST_Expand(geometry , float) IS 'args: g1, units_to_expand - Returns bounding box expanded in all directions from the bounding box of the input geometry';
			
COMMENT ON FUNCTION ST_Expand(box2d , float) IS 'args: g1, units_to_expand - Returns bounding box expanded in all directions from the bounding box of the input geometry';
			
COMMENT ON FUNCTION ST_Expand(box3d , float) IS 'args: g1, units_to_expand - Returns bounding box expanded in all directions from the bounding box of the input geometry';
			
COMMENT ON AGGREGATE ST_Extent(geometry) IS 'args: geomfield - an aggregate function that returns the bounding box that bounds rows of geometries.';
			