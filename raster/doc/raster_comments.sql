
COMMENT ON FUNCTION AddRasterColumn(varchar , varchar , integer , varchar[] , boolean , boolean , double precision[] , double precision , double precision , integer , integer , geometry ) IS 'args: table_name, column_name, srid, pixel_types, out_db, regular_blocking, no_data_values, pixelsize_x, pixelsize_y, blocksize_x, blocksize_y, envelope - Adds a raster column to an existing table and generates column constraint on the new column to enforce srid.';
			
COMMENT ON FUNCTION AddRasterColumn(varchar , varchar , varchar , integer , varchar[] , boolean , boolean , double precision[] , double precision , double precision , integer , integer , geometry ) IS 'args: schema_name, table_name, column_name, srid, pixel_types, out_db, regular_blocking, no_data_values, pixelsize_x, pixelsize_y, blocksize_x, blocksize_y, envelope - Adds a raster column to an existing table and generates column constraint on the new column to enforce srid.';
			
COMMENT ON FUNCTION AddRasterColumn(varchar , varchar , varchar , varchar , integer , varchar[] , boolean , boolean , double precision[] , double precision , double precision , integer , integer , geometry ) IS 'args: catalog_name, schema_name, table_name, column_name, srid, pixel_types, out_db, regular_blocking, no_data_values, pixelsize_x, pixelsize_y, blocksize_x, blocksize_y, envelope - Adds a raster column to an existing table and generates column constraint on the new column to enforce srid.';
			
COMMENT ON FUNCTION DropRasterColumn(varchar , varchar ) IS 'args: table_name, column_name - Removes a raster column from a table.';
			
COMMENT ON FUNCTION DropRasterColumn(varchar , varchar , varchar ) IS 'args: schema_name, table_name, column_name - Removes a raster column from a table.';
			
COMMENT ON FUNCTION DropRasterColumn(varchar , varchar , varchar , varchar ) IS 'args: catalog_name, schema_name, table_name, column_name - Removes a raster column from a table.';
			
COMMENT ON FUNCTION DropRasterTable(varchar ) IS 'args: table_name - Drops a table and all its references in raster_columns.';
			
COMMENT ON FUNCTION DropRasterTable(varchar , varchar ) IS 'args: schema_name, table_name - Drops a table and all its references in raster_columns.';
			
COMMENT ON FUNCTION DropRasterTable(varchar , varchar , varchar ) IS 'args: catalog_name, schema_name, table_name - Drops a table and all its references in raster_columns.';
			
COMMENT ON FUNCTION PostGIS_Raster_Lib_Build_Date() IS 'Reports full raster library build date';
			
COMMENT ON FUNCTION PostGIS_Raster_Lib_Version() IS 'Reports full raster version and build configuration infos.';
			
COMMENT ON FUNCTION ST_MakeEmptyRaster(integer , integer , float8 , float8 , float8 , float8 , float8 , float8 , integer ) IS 'args: width, height, ipx, ipy, scalex, scaley, skewx, skewy, srid - Returns an empty raster of given dimensions, pixel x y, skew and spatial reference system with no bands';
			
COMMENT ON FUNCTION ST_GeoReference(raster ) IS 'args: rast - Returns the georeference meta data in GDAL or ESRI format as commonly seen in a world file. Default is GDAL.';
			
COMMENT ON FUNCTION ST_GeoReference(raster , text ) IS 'args: rast, format - Returns the georeference meta data in GDAL or ESRI format as commonly seen in a world file. Default is GDAL.';
			
COMMENT ON FUNCTION ST_Height(raster ) IS 'args: rast - Returns the height of the raster in pixels?';
			
COMMENT ON FUNCTION ST_MetaData(raster ) IS 'args: rast - Returns basic meta data about a raster object such as skew, rotation, upper,lower left etc.';
			
COMMENT ON FUNCTION ST_NumBands(raster ) IS 'args: rast - Returns the number of bands in the raster object.';
			
COMMENT ON FUNCTION ST_PixelSizeX(raster ) IS 'args: rast - Returns the x size of pixels in units of coordinate reference system?';
			
COMMENT ON FUNCTION ST_PixelSizeY(raster ) IS 'args: rast - Returns the y size of pixels in units of coordinate reference system?';
			
COMMENT ON FUNCTION ST_Raster2WorldCoordX(raster , integer ) IS 'args: rast, xcolumn - Returns the geometric x coordinate upper left of a raster , column and row. Numbering of columns and rows starts at 1.';
			
COMMENT ON FUNCTION ST_Raster2WorldCoordX(raster , integer , integer ) IS 'args: rast, xcolumn, yrow - Returns the geometric x coordinate upper left of a raster , column and row. Numbering of columns and rows starts at 1.';
			
COMMENT ON FUNCTION ST_Raster2WorldCoordY(raster , integer ) IS 'args: rast, yrow - Returns the geometric y coordinate upper left corner of a raster , column and row. Numbering of columns and rows starts at 1.';
			
COMMENT ON FUNCTION ST_Raster2WorldCoordY(raster , integer , integer ) IS 'args: rast, xcolumn, yrow - Returns the geometric y coordinate upper left corner of a raster , column and row. Numbering of columns and rows starts at 1.';
			
COMMENT ON FUNCTION ST_SkewX(raster ) IS 'args: rast - Returns the georeference X skew (or rotation parameter)';
			
COMMENT ON FUNCTION ST_SkewY(raster ) IS 'args: rast - Returns the georeference Y skew (or rotation parameter)';
			
COMMENT ON FUNCTION ST_SRID(raster ) IS 'args: rast - Returns the spatial reference identifier of the raster as defined in spatial_ref_sys table.';
			
COMMENT ON FUNCTION ST_UpperLeftX(raster ) IS 'args: rast - Returns the upper left x coordinate of raster in projected spatial ref.';
			
COMMENT ON FUNCTION ST_UpperLeftY(raster ) IS 'args: rast - Returns the upper left y coordinate of raster in projected spatial ref.';
			
COMMENT ON FUNCTION ST_Width(raster ) IS 'args: rast - Returns the width of the raster in pixels?';
			
COMMENT ON FUNCTION ST_BandHasNoDataValue(raster ) IS 'args: rast - Returns whether or not a band has a value that should be considered no data. If no band number provided 1 is assumed.';
			
COMMENT ON FUNCTION ST_BandHasNoDataValue(raster , integer ) IS 'args: rast, bandnum - Returns whether or not a band has a value that should be considered no data. If no band number provided 1 is assumed.';
			
COMMENT ON FUNCTION ST_BandMetaData(raster ) IS 'args: rast - Returns basic meta data for a specific raster band. band num 1 is assumed if none-specified.';
			
COMMENT ON FUNCTION ST_BandMetaData(raster , integer ) IS 'args: rast, bandnum - Returns basic meta data for a specific raster band. band num 1 is assumed if none-specified.';
			
COMMENT ON FUNCTION ST_BandNoDataValue(raster ) IS 'args: rast - Returns the value in a given band that represents no data. If no band num 1 is assumed.';
			
COMMENT ON FUNCTION ST_BandNoDataValue(raster , integer ) IS 'args: rast, bandnum - Returns the value in a given band that represents no data. If no band num 1 is assumed.';
			
COMMENT ON FUNCTION ST_BandPath(raster ) IS 'args: rast - Returns system file path to a band stored in file system. If no bandnum specified, 1 is assumed.';
			
COMMENT ON FUNCTION ST_BandPath(raster , integer ) IS 'args: rast, bandnum - Returns system file path to a band stored in file system. If no bandnum specified, 1 is assumed.';
			
COMMENT ON FUNCTION ST_BandPixelType(raster ) IS 'args: rast - Returns the type of pixel for given band. If no bandnum specified, 1 is assumed.';
			
COMMENT ON FUNCTION ST_BandPixelType(raster , integer ) IS 'args: rast, bandnum - Returns the type of pixel for given band. If no bandnum specified, 1 is assumed.';
			
COMMENT ON FUNCTION ST_PixelAsPolygon(raster , integer , integer ) IS 'args: rast, columnx, rowy - Returns the geometry that bounds the pixel (for now a rectangle) for a particular band, row, column. If no band specified band num 1 is assumed.';
			
COMMENT ON FUNCTION ST_PixelAsPolygon(raster , integer , integer , integer ) IS 'args: rast, bandnum, columnx, rowy - Returns the geometry that bounds the pixel (for now a rectangle) for a particular band, row, column. If no band specified band num 1 is assumed.';
			
COMMENT ON FUNCTION ST_Value(raster , geometry ) IS 'args: rast, pt - Returns the value of a given band in a given columnx, rowy pixel or at a particular geometric point. Band numbers start at 1 and assumed to be 1 if not specified.';
			
COMMENT ON FUNCTION ST_Value(raster , integer , geometry ) IS 'args: rast, bandnum, pt - Returns the value of a given band in a given columnx, rowy pixel or at a particular geometric point. Band numbers start at 1 and assumed to be 1 if not specified.';
			
COMMENT ON FUNCTION ST_Value(raster , integer , integer ) IS 'args: rast, columnx, rowy - Returns the value of a given band in a given columnx, rowy pixel or at a particular geometric point. Band numbers start at 1 and assumed to be 1 if not specified.';
			
COMMENT ON FUNCTION ST_Value(raster , integer , integer , integer ) IS 'args: rast, bandnum, columnx, rowy - Returns the value of a given band in a given columnx, rowy pixel or at a particular geometric point. Band numbers start at 1 and assumed to be 1 if not specified.';
			
COMMENT ON FUNCTION ST_SetValue(raster , geometry , double precision ) IS 'args: rast, pt, newvalue - Sets the value of a given band in a given columnx, rowy pixel or at a pixel that intersects a particular geometric point. Band numbers start at 1 and assumed to be 1 if not specified.';
			
COMMENT ON FUNCTION ST_SEtValue(raster , integer , geometry , double precision ) IS 'args: rast, bandnum, pt, newvalue - Sets the value of a given band in a given columnx, rowy pixel or at a pixel that intersects a particular geometric point. Band numbers start at 1 and assumed to be 1 if not specified.';
			
COMMENT ON FUNCTION ST_SetValue(raster , integer , integer , double precision ) IS 'args: rast, columnx, rowy, newvalue - Sets the value of a given band in a given columnx, rowy pixel or at a pixel that intersects a particular geometric point. Band numbers start at 1 and assumed to be 1 if not specified.';
			
COMMENT ON FUNCTION ST_SetValue(raster , integer , integer , integer , double precision ) IS 'args: rast, bandnum, columnx, rowy, newvalue - Sets the value of a given band in a given columnx, rowy pixel or at a pixel that intersects a particular geometric point. Band numbers start at 1 and assumed to be 1 if not specified.';
			
COMMENT ON FUNCTION ST_SetGeoReference(raster , text ) IS 'args: rast, georefcoords - Set Georefence 6 georeference parameters in a single call. Numbers should be separated by white space. Accepts inputs in GDAL or ESRI format. Default is GDAL.';
			
COMMENT ON FUNCTION ST_SetGeoReference(raster , text , text ) IS 'args: rast, georefcoords, format - Set Georefence 6 georeference parameters in a single call. Numbers should be separated by white space. Accepts inputs in GDAL or ESRI format. Default is GDAL.';
			
COMMENT ON FUNCTION ST_SetPixelSize(raster , float8 ) IS 'args: rast, xy - Sets the x and y size of pixels in units of coordinate reference system. Number units/pixel width/height';
			
COMMENT ON FUNCTION ST_SetPixelSize(raster , float8 , float8 ) IS 'args: rast, x, y - Sets the x and y size of pixels in units of coordinate reference system. Number units/pixel width/height';
			
COMMENT ON FUNCTION ST_SetSkew(raster , float8 ) IS 'args: rast, skewxy - Sets the georeference X and Y skew (or rotation parameter). If only one is passed in sets x and y to same number.';
			
COMMENT ON FUNCTION ST_SetSkew(raster , float8 , float8 ) IS 'args: rast, skewx, skewy - Sets the georeference X and Y skew (or rotation parameter). If only one is passed in sets x and y to same number.';
			
COMMENT ON FUNCTION ST_SetSRID(raster , integer ) IS 'args: rast, srid - Sets the SRID of a raster to a particular integer srid defined in the spatial_ref_sys table.';
			
COMMENT ON FUNCTION ST_SetUpperLeft(raster , double precision , double precision ) IS 'args: rast, x, y - Sets the value of the upper left corner of the pixel to projected x,y coordinates';
			
COMMENT ON FUNCTION ST_SetBandHasNoDataValue(raster , integer , boolean ) IS 'args: rast, bandnum, has_nodatavalue - Sets whether or not a band has a value that should be considered no data.';
			
COMMENT ON FUNCTION ST_SetBandNoDataValue(raster , integer , double precision ) IS 'args: rast, bandnum, val - Sets the value for the given band that represents no data.';
			
COMMENT ON FUNCTION ST_AsBinary(raster ) IS 'args: rast - Return the Well-Known Binary (WKB) representation of the raster without SRID meta data.';
			
COMMENT ON FUNCTION ST_Box2D(raster ) IS 'args: rast - Returns the box 2d representation of the enclosing box of the raster';
			
COMMENT ON FUNCTION ST_ConvexHull(raster ) IS 'args: rast - Return the convex hull geometry of the raster including pixel values equal to BandNoDataValue. For regular shaped and non-skewed rasters, this gives the same answer as ST_Envelope so only useful for irregularly shaped or skewed rasters.';
			
COMMENT ON FUNCTION ST_DumpAsPolygons(raster ) IS 'args: rast - Returns a set of geomval (geom,val) rows, from a given raster band. If no band number is specified, band num defaults to 1.';
			
COMMENT ON FUNCTION ST_DumpAsPolygons(raster , integer ) IS 'args: rast, band_num - Returns a set of geomval (geom,val) rows, from a given raster band. If no band number is specified, band num defaults to 1.';
			
COMMENT ON FUNCTION ST_Envelope(raster ) IS 'args: rast - Returns the polygon representation of the extent of the raster.';
			
COMMENT ON FUNCTION ST_Intersection(geometry , raster ) IS 'args: geom, rast - Returns a set of geometry-pixelvalue pairs resulting from intersection of a raster band with a geometry. If no band number is specified, band 1 is assumed.';
			
COMMENT ON FUNCTION ST_Intersection(geometry , raster , integer ) IS 'args: geom, rast, band_num - Returns a set of geometry-pixelvalue pairs resulting from intersection of a raster band with a geometry. If no band number is specified, band 1 is assumed.';
			
COMMENT ON FUNCTION ST_Intersection(raster , geometry ) IS 'args: rast, geom - Returns a set of geometry-pixelvalue pairs resulting from intersection of a raster band with a geometry. If no band number is specified, band 1 is assumed.';
			
COMMENT ON FUNCTION ST_Intersection(raster , integer , geometry ) IS 'args: rast, band_num, geom - Returns a set of geometry-pixelvalue pairs resulting from intersection of a raster band with a geometry. If no band number is specified, band 1 is assumed.';
			
COMMENT ON FUNCTION ST_Polygon(raster ) IS 'args: rast - Returns a polygon geometry formed by the union of pixels that have a pixel value that is not no data value. If no band number is specified, band num defaults to 1.';
			
COMMENT ON FUNCTION ST_Polygon(raster , integer ) IS 'args: rast, band_num - Returns a polygon geometry formed by the union of pixels that have a pixel value that is not no data value. If no band number is specified, band num defaults to 1.';
			
COMMENT ON FUNCTION ST_Intersects(raster , geometry ) IS 'args: rastA, geomB - Returns TRUE if rastA pixel in a band with non-no data band value intersects with a geometry/raster. If band number is not specified it defaults to 1.';
			
COMMENT ON FUNCTION ST_Intersects(raster , integer , geometry ) IS 'args: rastA, band_num, geomB - Returns TRUE if rastA pixel in a band with non-no data band value intersects with a geometry/raster. If band number is not specified it defaults to 1.';
			
COMMENT ON FUNCTION ST_Intersects(geometry , raster ) IS 'args: geomA, rastB - Returns TRUE if rastA pixel in a band with non-no data band value intersects with a geometry/raster. If band number is not specified it defaults to 1.';
			
COMMENT ON FUNCTION ST_Intersects(geometry , raster , integer ) IS 'args: geomA, rastB, band_num - Returns TRUE if rastA pixel in a band with non-no data band value intersects with a geometry/raster. If band number is not specified it defaults to 1.';
			