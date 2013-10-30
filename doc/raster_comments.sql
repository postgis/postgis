
COMMENT ON FUNCTION AddRasterConstraints(name , name , boolean , boolean , boolean , boolean , boolean , boolean , boolean , boolean , boolean , boolean , boolean , boolean ) IS 'args: rasttable, rastcolumn, srid, scale_x, scale_y, blocksize_x, blocksize_y, same_alignment, regular_blocking, num_bands=true, pixel_types=true, nodata_values=true, out_db=true, extent=true - Adds raster constraints to a loaded raster table for a specific column that constrains spatial ref, scaling, blocksize, alignment, bands, band type and a flag to denote if raster column is regularly blocked. The table must be loaded with data for the constraints to be inferred. Returns true of the constraint setting was accomplished and if issues a notice.';
			
COMMENT ON FUNCTION AddRasterConstraints(name , name , text[] ) IS 'args: rasttable, rastcolumn, VARIADIC constraints - Adds raster constraints to a loaded raster table for a specific column that constrains spatial ref, scaling, blocksize, alignment, bands, band type and a flag to denote if raster column is regularly blocked. The table must be loaded with data for the constraints to be inferred. Returns true of the constraint setting was accomplished and if issues a notice.';
			
COMMENT ON FUNCTION AddRasterConstraints(name , name , name , text[] ) IS 'args: rastschema, rasttable, rastcolumn, VARIADIC constraints - Adds raster constraints to a loaded raster table for a specific column that constrains spatial ref, scaling, blocksize, alignment, bands, band type and a flag to denote if raster column is regularly blocked. The table must be loaded with data for the constraints to be inferred. Returns true of the constraint setting was accomplished and if issues a notice.';
			
COMMENT ON FUNCTION AddRasterConstraints(name , name , name , boolean , boolean , boolean , boolean , boolean , boolean , boolean , boolean , boolean , boolean , boolean , boolean ) IS 'args: rastschema, rasttable, rastcolumn, srid=true, scale_x=true, scale_y=true, blocksize_x=true, blocksize_y=true, same_alignment=true, regular_blocking=false, num_bands=true, pixel_types=true, nodata_values=true, out_db=true, extent=true - Adds raster constraints to a loaded raster table for a specific column that constrains spatial ref, scaling, blocksize, alignment, bands, band type and a flag to denote if raster column is regularly blocked. The table must be loaded with data for the constraints to be inferred. Returns true of the constraint setting was accomplished and if issues a notice.';
			
COMMENT ON FUNCTION DropRasterConstraints(name , name , boolean , boolean , boolean , boolean , boolean , boolean , boolean , boolean , boolean , boolean , boolean , boolean ) IS 'args: rasttable, rastcolumn, srid, scale_x, scale_y, blocksize_x, blocksize_y, same_alignment, regular_blocking, num_bands=true, pixel_types=true, nodata_values=true, out_db=true, extent=true - Drops PostGIS raster constraints that refer to a raster table column. Useful if you need to reload data or update your raster column data.';
			
COMMENT ON FUNCTION DropRasterConstraints(name , name , name , boolean , boolean , boolean , boolean , boolean , boolean , boolean , boolean , boolean , boolean , boolean , boolean ) IS 'args: rastschema, rasttable, rastcolumn, srid=true, scale_x=true, scale_y=true, blocksize_x=true, blocksize_y=true, same_alignment=true, regular_blocking=false, num_bands=true, pixel_types=true, nodata_values=true, out_db=true, extent=true - Drops PostGIS raster constraints that refer to a raster table column. Useful if you need to reload data or update your raster column data.';
			
COMMENT ON FUNCTION DropRasterConstraints(name , name , name , text[] ) IS 'args: rastschema, rasttable, rastcolumn, constraints - Drops PostGIS raster constraints that refer to a raster table column. Useful if you need to reload data or update your raster column data.';
			
COMMENT ON FUNCTION PostGIS_Raster_Lib_Build_Date() IS 'Reports full raster library build date.';
			
COMMENT ON FUNCTION PostGIS_Raster_Lib_Version() IS 'Reports full raster version and build configuration infos.';
			
COMMENT ON FUNCTION ST_GDALDrivers() IS 'args: OUT idx, OUT short_name, OUT long_name, OUT create_options - Returns a list of raster formats supported by your lib gdal. These are the formats you can output your raster using ST_AsGDALRaster.';
			
COMMENT ON FUNCTION UpdateRasterSRID(name , name , name , integer ) IS 'args: schema_name, table_name, column_name, new_srid - Change the SRID of all rasters in the user-specified column and table.';
			
COMMENT ON FUNCTION UpdateRasterSRID(name , name , integer ) IS 'args: table_name, column_name, new_srid - Change the SRID of all rasters in the user-specified column and table.';
			
COMMENT ON FUNCTION ST_AddBand(raster , addbandarg[] ) IS 'args: rast, addbandargset - Returns a raster with the new band(s) of given type added with given initial value in the given index location. If no index is specified, the band is added to the end.';
			
COMMENT ON FUNCTION ST_AddBand(raster , integer , text , double precision , double precision ) IS 'args: rast, index, pixeltype, initialvalue=0, nodataval=NULL - Returns a raster with the new band(s) of given type added with given initial value in the given index location. If no index is specified, the band is added to the end.';
			
COMMENT ON FUNCTION ST_AddBand(raster , text , double precision , double precision ) IS 'args: rast, pixeltype, initialvalue=0, nodataval=NULL - Returns a raster with the new band(s) of given type added with given initial value in the given index location. If no index is specified, the band is added to the end.';
			
COMMENT ON FUNCTION ST_AddBand(raster , raster , integer , integer ) IS 'args: torast, fromrast, fromband=1, torastindex=at_end - Returns a raster with the new band(s) of given type added with given initial value in the given index location. If no index is specified, the band is added to the end.';
			
COMMENT ON FUNCTION ST_AddBand(raster , raster[] , integer , integer ) IS 'args: torast, fromrasts, fromband=1, torastindex=at_end - Returns a raster with the new band(s) of given type added with given initial value in the given index location. If no index is specified, the band is added to the end.';
			
COMMENT ON FUNCTION ST_AddBand(raster , integer , text , integer[] , double precision ) IS 'args: rast, index, outdbfile, outdbindex, nodataval=NULL - Returns a raster with the new band(s) of given type added with given initial value in the given index location. If no index is specified, the band is added to the end.';
			
COMMENT ON FUNCTION ST_AddBand(raster , text , integer[] , integer , double precision ) IS 'args: rast, outdbfile, outdbindex, index=at_end, nodataval=NULL - Returns a raster with the new band(s) of given type added with given initial value in the given index location. If no index is specified, the band is added to the end.';
			
COMMENT ON FUNCTION ST_AsRaster(geometry , raster , text , double precision , double precision , boolean ) IS 'args: geom, ref, pixeltype, value=1, nodataval=0, touched=false - Converts a PostGIS geometry to a PostGIS raster.';
			
COMMENT ON FUNCTION ST_AsRaster(geometry , raster , text[] , double precision[] , double precision[] , boolean ) IS 'args: geom, ref, pixeltype=ARRAY[''8BUI''], value=ARRAY[1], nodataval=ARRAY[0], touched=false - Converts a PostGIS geometry to a PostGIS raster.';
			
COMMENT ON FUNCTION ST_AsRaster(geometry , double precision , double precision , double precision , double precision , text , double precision , double precision , double precision , double precision , boolean ) IS 'args: geom, scalex, scaley, gridx, gridy, pixeltype, value=1, nodataval=0, skewx=0, skewy=0, touched=false - Converts a PostGIS geometry to a PostGIS raster.';
			
COMMENT ON FUNCTION ST_AsRaster(geometry , double precision , double precision , double precision , double precision , text[] , double precision[] , double precision[] , double precision , double precision , boolean ) IS 'args: geom, scalex, scaley, gridx=NULL, gridy=NULL, pixeltype=ARRAY[''8BUI''], value=ARRAY[1], nodataval=ARRAY[0], skewx=0, skewy=0, touched=false - Converts a PostGIS geometry to a PostGIS raster.';
			
COMMENT ON FUNCTION ST_AsRaster(geometry , double precision , double precision , text , double precision , double precision , double precision , double precision , double precision , double precision , boolean ) IS 'args: geom, scalex, scaley, pixeltype, value=1, nodataval=0, upperleftx=NULL, upperlefty=NULL, skewx=0, skewy=0, touched=false - Converts a PostGIS geometry to a PostGIS raster.';
			
COMMENT ON FUNCTION ST_AsRaster(geometry , double precision , double precision , text[] , double precision[] , double precision[] , double precision , double precision , double precision , double precision , boolean ) IS 'args: geom, scalex, scaley, pixeltype, value=ARRAY[1], nodataval=ARRAY[0], upperleftx=NULL, upperlefty=NULL, skewx=0, skewy=0, touched=false - Converts a PostGIS geometry to a PostGIS raster.';
			
COMMENT ON FUNCTION ST_AsRaster(geometry , integer , integer , double precision , double precision , text , double precision , double precision , double precision , double precision , boolean ) IS 'args: geom, width, height, gridx, gridy, pixeltype, value=1, nodataval=0, skewx=0, skewy=0, touched=false - Converts a PostGIS geometry to a PostGIS raster.';
			
COMMENT ON FUNCTION ST_AsRaster(geometry , integer , integer , double precision , double precision , text[] , double precision[] , double precision[] , double precision , double precision , boolean ) IS 'args: geom, width, height, gridx=NULL, gridy=NULL, pixeltype=ARRAY[''8BUI''], value=ARRAY[1], nodataval=ARRAY[0], skewx=0, skewy=0, touched=false - Converts a PostGIS geometry to a PostGIS raster.';
			
COMMENT ON FUNCTION ST_AsRaster(geometry , integer , integer , text , double precision , double precision , double precision , double precision , double precision , double precision , boolean ) IS 'args: geom, width, height, pixeltype, value=1, nodataval=0, upperleftx=NULL, upperlefty=NULL, skewx=0, skewy=0, touched=false - Converts a PostGIS geometry to a PostGIS raster.';
			
COMMENT ON FUNCTION ST_AsRaster(geometry , integer , integer , text[] , double precision[] , double precision[] , double precision , double precision , double precision , double precision , boolean ) IS 'args: geom, width, height, pixeltype, value=ARRAY[1], nodataval=ARRAY[0], upperleftx=NULL, upperlefty=NULL, skewx=0, skewy=0, touched=false - Converts a PostGIS geometry to a PostGIS raster.';
			
COMMENT ON FUNCTION ST_Band(raster , integer[] ) IS 'args: rast, nbands = ARRAY[1] - Returns one or more bands of an existing raster as a new raster. Useful for building new rasters from existing rasters.';
			
COMMENT ON FUNCTION ST_Band(raster , text , character ) IS 'args: rast, nbands, delimiter=, - Returns one or more bands of an existing raster as a new raster. Useful for building new rasters from existing rasters.';
			
COMMENT ON FUNCTION ST_Band(raster , integer ) IS 'args: rast, nband - Returns one or more bands of an existing raster as a new raster. Useful for building new rasters from existing rasters.';
			
COMMENT ON FUNCTION ST_MakeEmptyRaster(raster ) IS 'args: rast - Returns an empty raster (having no bands) of given dimensions (width & height), upperleft X and Y, pixel size and rotation (scalex, scaley, skewx & skewy) and reference system (srid). If a raster is passed in, returns a new raster with the same size, alignment and SRID. If srid is left out, the spatial ref is set to unknown (0).';
			
COMMENT ON FUNCTION ST_MakeEmptyRaster(integer , integer , float8 , float8 , float8 , float8 , float8 , float8 , integer ) IS 'args: width, height, upperleftx, upperlefty, scalex, scaley, skewx, skewy, srid=unknown - Returns an empty raster (having no bands) of given dimensions (width & height), upperleft X and Y, pixel size and rotation (scalex, scaley, skewx & skewy) and reference system (srid). If a raster is passed in, returns a new raster with the same size, alignment and SRID. If srid is left out, the spatial ref is set to unknown (0).';
			
COMMENT ON FUNCTION ST_MakeEmptyRaster(integer , integer , float8  , float8  , float8  ) IS 'args: width, height, upperleftx, upperlefty, pixelsize - Returns an empty raster (having no bands) of given dimensions (width & height), upperleft X and Y, pixel size and rotation (scalex, scaley, skewx & skewy) and reference system (srid). If a raster is passed in, returns a new raster with the same size, alignment and SRID. If srid is left out, the spatial ref is set to unknown (0).';
			
COMMENT ON FUNCTION ST_Tile(raster , int[] , integer , integer , boolean , double precision ) IS 'args: rast, nband, width, height, padwithnodata=FALSE, nodataval=NULL - Returns a set of rasters resulting from the split of the input raster based upon the desired dimensions of the output rasters.';
			
COMMENT ON FUNCTION ST_Tile(raster , integer , integer , integer , boolean , double precision ) IS 'args: rast, nband, width, height, padwithnodata=FALSE, nodataval=NULL - Returns a set of rasters resulting from the split of the input raster based upon the desired dimensions of the output rasters.';
			
COMMENT ON FUNCTION ST_Tile(raster , integer , integer , boolean , double precision ) IS 'args: rast, width, height, padwithnodata=FALSE, nodataval=NULL - Returns a set of rasters resulting from the split of the input raster based upon the desired dimensions of the output rasters.';
			
COMMENT ON FUNCTION ST_FromGDALRaster(bytea , integer ) IS 'args: gdaldata, srid=NULL - Returns a raster from a supported GDAL raster file.';
			
COMMENT ON FUNCTION ST_GeoReference(raster , text ) IS 'args: rast, format=GDAL - Returns the georeference meta data in GDAL or ESRI format as commonly seen in a world file. Default is GDAL.';
			
COMMENT ON FUNCTION ST_Height(raster ) IS 'args: rast - Returns the height of the raster in pixels.';
			
COMMENT ON FUNCTION ST_IsEmpty(raster ) IS 'args: rast - Returns true if the raster is empty (width = 0 and height = 0). Otherwise, returns false.';
			
COMMENT ON FUNCTION ST_MetaData(raster ) IS 'args: rast - Returns basic meta data about a raster object such as pixel size, rotation (skew), upper, lower left, etc.';
			
COMMENT ON FUNCTION ST_NumBands(raster ) IS 'args: rast - Returns the number of bands in the raster object.';
			
COMMENT ON FUNCTION ST_PixelHeight(raster ) IS 'args: rast - Returns the pixel height in geometric units of the spatial reference system.';
			
COMMENT ON FUNCTION ST_PixelWidth(raster ) IS 'args: rast - Returns the pixel width in geometric units of the spatial reference system.';
			
COMMENT ON FUNCTION ST_ScaleX(raster ) IS 'args: rast - Returns the X component of the pixel width in units of coordinate reference system.';
			
COMMENT ON FUNCTION ST_ScaleY(raster ) IS 'args: rast - Returns the Y component of the pixel height in units of coordinate reference system.';
			
COMMENT ON FUNCTION ST_RasterToWorldCoord(raster , integer , integer ) IS 'args: rast, xcolumn, yrow - Returns the rasters upper left corner as geometric X and Y (longitude and latitude) given a column and row. Column and row starts at 1.';
			
COMMENT ON FUNCTION ST_RasterToWorldCoordX(raster , integer ) IS 'args: rast, xcolumn - Returns the geometric X coordinate upper left of a raster, column and row. Numbering of columns and rows starts at 1.';
			
COMMENT ON FUNCTION ST_RasterToWorldCoordX(raster , integer , integer ) IS 'args: rast, xcolumn, yrow - Returns the geometric X coordinate upper left of a raster, column and row. Numbering of columns and rows starts at 1.';
			
COMMENT ON FUNCTION ST_RasterToWorldCoordY(raster , integer ) IS 'args: rast, yrow - Returns the geometric Y coordinate upper left corner of a raster, column and row. Numbering of columns and rows starts at 1.';
			
COMMENT ON FUNCTION ST_RasterToWorldCoordY(raster , integer , integer ) IS 'args: rast, xcolumn, yrow - Returns the geometric Y coordinate upper left corner of a raster, column and row. Numbering of columns and rows starts at 1.';
			
COMMENT ON FUNCTION ST_Rotation(raster) IS 'args: rast - Returns the rotation of the raster in radian.';
			
COMMENT ON FUNCTION ST_SkewX(raster ) IS 'args: rast - Returns the georeference X skew (or rotation parameter).';
			
COMMENT ON FUNCTION ST_SkewY(raster ) IS 'args: rast - Returns the georeference Y skew (or rotation parameter).';
			
COMMENT ON FUNCTION ST_SRID(raster ) IS 'args: rast - Returns the spatial reference identifier of the raster as defined in spatial_ref_sys table.';
			
COMMENT ON FUNCTION ST_Summary(raster ) IS 'args: rast - Returns a text summary of the contents of the raster.';
			
COMMENT ON FUNCTION ST_UpperLeftX(raster ) IS 'args: rast - Returns the upper left X coordinate of raster in projected spatial ref.';
			
COMMENT ON FUNCTION ST_UpperLeftY(raster ) IS 'args: rast - Returns the upper left Y coordinate of raster in projected spatial ref.';
			
COMMENT ON FUNCTION ST_Width(raster ) IS 'args: rast - Returns the width of the raster in pixels.';
			
COMMENT ON FUNCTION ST_WorldToRasterCoord(raster , geometry ) IS 'args: rast, pt - Returns the upper left corner as column and row given geometric X and Y (longitude and latitude) or a point geometry expressed in the spatial reference coordinate system of the raster.';
			
COMMENT ON FUNCTION ST_WorldToRasterCoord(raster , double precision , double precision ) IS 'args: rast, longitude, latitude - Returns the upper left corner as column and row given geometric X and Y (longitude and latitude) or a point geometry expressed in the spatial reference coordinate system of the raster.';
			
COMMENT ON FUNCTION ST_WorldToRasterCoordX(raster , geometry ) IS 'args: rast, pt - Returns the column in the raster of the point geometry (pt) or a X and Y world coordinate (xw, yw) represented in world spatial reference system of raster.';
			
COMMENT ON FUNCTION ST_WorldToRasterCoordX(raster , double precision ) IS 'args: rast, xw - Returns the column in the raster of the point geometry (pt) or a X and Y world coordinate (xw, yw) represented in world spatial reference system of raster.';
			
COMMENT ON FUNCTION ST_WorldToRasterCoordX(raster , double precision , double precision ) IS 'args: rast, xw, yw - Returns the column in the raster of the point geometry (pt) or a X and Y world coordinate (xw, yw) represented in world spatial reference system of raster.';
			
COMMENT ON FUNCTION ST_WorldToRasterCoordY(raster , geometry ) IS 'args: rast, pt - Returns the row in the raster of the point geometry (pt) or a X and Y world coordinate (xw, yw) represented in world spatial reference system of raster.';
			
COMMENT ON FUNCTION ST_WorldToRasterCoordY(raster , double precision ) IS 'args: rast, xw - Returns the row in the raster of the point geometry (pt) or a X and Y world coordinate (xw, yw) represented in world spatial reference system of raster.';
			
COMMENT ON FUNCTION ST_WorldToRasterCoordY(raster , double precision , double precision ) IS 'args: rast, xw, yw - Returns the row in the raster of the point geometry (pt) or a X and Y world coordinate (xw, yw) represented in world spatial reference system of raster.';
			
COMMENT ON FUNCTION ST_BandMetaData(raster , integer ) IS 'args: rast, bandnum=1 - Returns basic meta data for a specific raster band. band num 1 is assumed if none-specified.';
			
COMMENT ON FUNCTION ST_BandNoDataValue(raster , integer ) IS 'args: rast, bandnum=1 - Returns the value in a given band that represents no data. If no band num 1 is assumed.';
			
COMMENT ON FUNCTION ST_BandIsNoData(raster , integer , boolean ) IS 'args: rast, band, forceChecking=true - Returns true if the band is filled with only nodata values.';
			
COMMENT ON FUNCTION ST_BandIsNoData(raster , boolean ) IS 'args: rast, forceChecking=true - Returns true if the band is filled with only nodata values.';
			
COMMENT ON FUNCTION ST_BandPath(raster , integer ) IS 'args: rast, bandnum=1 - Returns system file path to a band stored in file system. If no bandnum specified, 1 is assumed.';
			
COMMENT ON FUNCTION ST_BandPixelType(raster , integer ) IS 'args: rast, bandnum=1 - Returns the type of pixel for given band. If no bandnum specified, 1 is assumed.';
			
COMMENT ON FUNCTION ST_HasNoBand(raster , integer ) IS 'args: rast, bandnum=1 - Returns true if there is no band with given band number. If no band number is specified, then band number 1 is assumed.';
			
COMMENT ON FUNCTION ST_PixelAsPolygon(raster , integer , integer ) IS 'args: rast, columnx, rowy - Returns the polygon geometry that bounds the pixel for a particular row and column.';
			
COMMENT ON FUNCTION ST_PixelAsPolygons(raster , integer , boolean ) IS 'args: rast, band=1, exclude_nodata_value=TRUE - Returns the polygon geometry that bounds every pixel of a raster band along with the value, the X and the Y raster coordinates of each pixel.';
			
COMMENT ON FUNCTION ST_PixelAsPoint(raster , integer , integer ) IS 'args: rast, columnx, rowy - Returns a point geometry of the pixels upper-left corner.';
			
COMMENT ON FUNCTION ST_PixelAsPoints(raster , integer , boolean ) IS 'args: rast, band=1, exclude_nodata_value=TRUE - Returns a point geometry for each pixel of a raster band along with the value, the X and the Y raster coordinates of each pixel. The coordinates of the point geometry are of the pixels upper-left corner.';
			
COMMENT ON FUNCTION ST_PixelAsCentroid(raster , integer , integer ) IS 'args: rast, columnx, rowy - Returns the centroid (point geometry) of the area represented by a pixel.';
			
COMMENT ON FUNCTION ST_PixelAsCentroids(raster , integer , boolean ) IS 'args: rast, band=1, exclude_nodata_value=TRUE - Returns the centroid (point geometry) for each pixel of a raster band along with the value, the X and the Y raster coordinates of each pixel. The point geometry is the centroid of the area represented by a pixel.';
			
COMMENT ON FUNCTION ST_Value(raster , geometry , boolean ) IS 'args: rast, pt, exclude_nodata_value=true - Returns the value of a given band in a given columnx, rowy pixel or at a particular geometric point. Band numbers start at 1 and assumed to be 1 if not specified. If exclude_nodata_value is set to false, then all pixels include nodata pixels are considered to intersect and return value. If exclude_nodata_value is not passed in then reads it from metadata of raster.';
			
COMMENT ON FUNCTION ST_Value(raster , integer , geometry , boolean ) IS 'args: rast, bandnum, pt, exclude_nodata_value=true - Returns the value of a given band in a given columnx, rowy pixel or at a particular geometric point. Band numbers start at 1 and assumed to be 1 if not specified. If exclude_nodata_value is set to false, then all pixels include nodata pixels are considered to intersect and return value. If exclude_nodata_value is not passed in then reads it from metadata of raster.';
			
COMMENT ON FUNCTION ST_Value(raster , integer , integer , boolean ) IS 'args: rast, columnx, rowy, exclude_nodata_value=true - Returns the value of a given band in a given columnx, rowy pixel or at a particular geometric point. Band numbers start at 1 and assumed to be 1 if not specified. If exclude_nodata_value is set to false, then all pixels include nodata pixels are considered to intersect and return value. If exclude_nodata_value is not passed in then reads it from metadata of raster.';
			
COMMENT ON FUNCTION ST_Value(raster , integer , integer , integer , boolean ) IS 'args: rast, bandnum, columnx, rowy, exclude_nodata_value=true - Returns the value of a given band in a given columnx, rowy pixel or at a particular geometric point. Band numbers start at 1 and assumed to be 1 if not specified. If exclude_nodata_value is set to false, then all pixels include nodata pixels are considered to intersect and return value. If exclude_nodata_value is not passed in then reads it from metadata of raster.';
			
COMMENT ON FUNCTION ST_NearestValue(raster , integer , geometry , boolean ) IS 'args: rast, bandnum, pt, exclude_nodata_value=true - Returns the nearest non-NODATA value of a given bands pixel specified by a columnx and rowy or a geometric point expressed in the same spatial reference coordinate system as the raster.';
			
COMMENT ON FUNCTION ST_NearestValue(raster , geometry , boolean ) IS 'args: rast, pt, exclude_nodata_value=true - Returns the nearest non-NODATA value of a given bands pixel specified by a columnx and rowy or a geometric point expressed in the same spatial reference coordinate system as the raster.';
			
COMMENT ON FUNCTION ST_NearestValue(raster , integer , integer , integer , boolean ) IS 'args: rast, bandnum, columnx, rowy, exclude_nodata_value=true - Returns the nearest non-NODATA value of a given bands pixel specified by a columnx and rowy or a geometric point expressed in the same spatial reference coordinate system as the raster.';
			
COMMENT ON FUNCTION ST_NearestValue(raster , integer , integer , boolean ) IS 'args: rast, columnx, rowy, exclude_nodata_value=true - Returns the nearest non-NODATA value of a given bands pixel specified by a columnx and rowy or a geometric point expressed in the same spatial reference coordinate system as the raster.';
			
COMMENT ON FUNCTION ST_Neighborhood(raster , integer , integer , integer , integer , integer , boolean ) IS 'args: rast, bandnum, columnX, rowY, distanceX, distanceY, exclude_nodata_value=true - Returns a 2-D double precision array of the non-NODATA values around a given bands pixel specified by either a columnX and rowY or a geometric point expressed in the same spatial reference coordinate system as the raster.';
			
COMMENT ON FUNCTION ST_Neighborhood(raster , integer , integer , integer , integer , boolean ) IS 'args: rast, columnX, rowY, distanceX, distanceY, exclude_nodata_value=true - Returns a 2-D double precision array of the non-NODATA values around a given bands pixel specified by either a columnX and rowY or a geometric point expressed in the same spatial reference coordinate system as the raster.';
			
COMMENT ON FUNCTION ST_Neighborhood(raster , integer , geometry , integer , integer , boolean ) IS 'args: rast, bandnum, pt, distanceX, distanceY, exclude_nodata_value=true - Returns a 2-D double precision array of the non-NODATA values around a given bands pixel specified by either a columnX and rowY or a geometric point expressed in the same spatial reference coordinate system as the raster.';
			
COMMENT ON FUNCTION ST_Neighborhood(raster , geometry , integer , integer , boolean ) IS 'args: rast, pt, distanceX, distanceY, exclude_nodata_value=true - Returns a 2-D double precision array of the non-NODATA values around a given bands pixel specified by either a columnX and rowY or a geometric point expressed in the same spatial reference coordinate system as the raster.';
			
COMMENT ON FUNCTION ST_SetValue(raster , integer , geometry , double precision ) IS 'args: rast, bandnum, geom, newvalue - Returns modified raster resulting from setting the value of a given band in a given columnx, rowy pixel or the pixels that intersect a particular geometry. Band numbers start at 1 and assumed to be 1 if not specified.';
			
COMMENT ON FUNCTION ST_SetValue(raster , geometry , double precision ) IS 'args: rast, geom, newvalue - Returns modified raster resulting from setting the value of a given band in a given columnx, rowy pixel or the pixels that intersect a particular geometry. Band numbers start at 1 and assumed to be 1 if not specified.';
			
COMMENT ON FUNCTION ST_SetValue(raster , integer , integer , integer , double precision ) IS 'args: rast, bandnum, columnx, rowy, newvalue - Returns modified raster resulting from setting the value of a given band in a given columnx, rowy pixel or the pixels that intersect a particular geometry. Band numbers start at 1 and assumed to be 1 if not specified.';
			
COMMENT ON FUNCTION ST_SetValue(raster , integer , integer , double precision ) IS 'args: rast, columnx, rowy, newvalue - Returns modified raster resulting from setting the value of a given band in a given columnx, rowy pixel or the pixels that intersect a particular geometry. Band numbers start at 1 and assumed to be 1 if not specified.';
			
COMMENT ON FUNCTION ST_SetValues(raster , integer , integer , integer , double precision[][] , boolean[][] , boolean ) IS 'args: rast, nband, columnx, rowy, newvalueset, noset=NULL, keepnodata=FALSE - Returns modified raster resulting from setting the values of a given band.';
			
COMMENT ON FUNCTION ST_SetValues(raster , integer , integer , integer , double precision[][] , double precision , boolean ) IS 'args: rast, nband, columnx, rowy, newvalueset, nosetvalue, keepnodata=FALSE - Returns modified raster resulting from setting the values of a given band.';
			
COMMENT ON FUNCTION ST_SetValues(raster , integer , integer , integer , integer , integer , double precision , boolean ) IS 'args: rast, nband, columnx, rowy, width, height, newvalue, keepnodata=FALSE - Returns modified raster resulting from setting the values of a given band.';
			
COMMENT ON FUNCTION ST_SetValues(raster , integer , integer , integer , integer , double precision , boolean ) IS 'args: rast, columnx, rowy, width, height, newvalue, keepnodata=FALSE - Returns modified raster resulting from setting the values of a given band.';
			
COMMENT ON FUNCTION ST_SetValues(raster , integer , geomval[] , boolean ) IS 'args: rast, nband, geomvalset, keepnodata=FALSE - Returns modified raster resulting from setting the values of a given band.';
			
COMMENT ON FUNCTION ST_DumpValues(raster , integer[] , boolean ) IS 'args: rast, nband, exclude_nodata_value=true - Get the values of the specified band as a 2-dimension array.';
			
COMMENT ON FUNCTION ST_DumpValues(raster , integer , boolean ) IS 'args: rast, nband, exclude_nodata_value=true - Get the values of the specified band as a 2-dimension array.';
			
COMMENT ON FUNCTION ST_PixelOfValue(raster , integer , double precision[] , boolean ) IS 'args: rast, nband, search, exclude_nodata_value=true - Get the columnx, rowy coordinates of the pixel whose value equals the search value.';
			
COMMENT ON FUNCTION ST_PixelOfValue(raster , double precision[] , boolean ) IS 'args: rast, search, exclude_nodata_value=true - Get the columnx, rowy coordinates of the pixel whose value equals the search value.';
			
COMMENT ON FUNCTION ST_PixelOfValue(raster , integer , double precision , boolean ) IS 'args: rast, nband, search, exclude_nodata_value=true - Get the columnx, rowy coordinates of the pixel whose value equals the search value.';
			
COMMENT ON FUNCTION ST_PixelOfValue(raster , double precision , boolean ) IS 'args: rast, search, exclude_nodata_value=true - Get the columnx, rowy coordinates of the pixel whose value equals the search value.';
			
COMMENT ON FUNCTION ST_SetGeoReference(raster , text , text ) IS 'args: rast, georefcoords, format=GDAL - Set Georeference 6 georeference parameters in a single call. Numbers should be separated by white space. Accepts inputs in GDAL or ESRI format. Default is GDAL.';
			
COMMENT ON FUNCTION ST_SetGeoReference(raster , double precision , double precision , double precision , double precision , double precision , double precision ) IS 'args: rast, upperleftx, upperlefty, scalex, scaley, skewx, skewy - Set Georeference 6 georeference parameters in a single call. Numbers should be separated by white space. Accepts inputs in GDAL or ESRI format. Default is GDAL.';
			
COMMENT ON FUNCTION ST_SetRotation(raster, float8) IS 'args: rast, rotation - Set the rotation of the raster in radian.';
			
COMMENT ON FUNCTION ST_SetScale(raster , float8 ) IS 'args: rast, xy - Sets the X and Y size of pixels in units of coordinate reference system. Number units/pixel width/height.';
			
COMMENT ON FUNCTION ST_SetScale(raster , float8 , float8 ) IS 'args: rast, x, y - Sets the X and Y size of pixels in units of coordinate reference system. Number units/pixel width/height.';
			
COMMENT ON FUNCTION ST_SetSkew(raster , float8 ) IS 'args: rast, skewxy - Sets the georeference X and Y skew (or rotation parameter). If only one is passed in, sets X and Y to the same value.';
			
COMMENT ON FUNCTION ST_SetSkew(raster , float8 , float8 ) IS 'args: rast, skewx, skewy - Sets the georeference X and Y skew (or rotation parameter). If only one is passed in, sets X and Y to the same value.';
			
COMMENT ON FUNCTION ST_SetSRID(raster , integer ) IS 'args: rast, srid - Sets the SRID of a raster to a particular integer srid defined in the spatial_ref_sys table.';
			
COMMENT ON FUNCTION ST_SetUpperLeft(raster , double precision , double precision ) IS 'args: rast, x, y - Sets the value of the upper left corner of the pixel to projected X and Y coordinates.';
			
COMMENT ON FUNCTION ST_Resample(raster , integer , integer , double precision , double precision , double precision , double precision , text , double precision ) IS 'args: rast, width, height, gridx=NULL, gridy=NULL, skewx=0, skewy=0, algorithm=NearestNeighbour, maxerr=0.125 - Resample a raster using a specified resampling algorithm, new dimensions, an arbitrary grid corner and a set of raster georeferencing attributes defined or borrowed from another raster.';
			
COMMENT ON FUNCTION ST_Resample(raster , double precision , double precision , double precision , double precision , double precision , double precision , text , double precision ) IS 'args: rast, scalex=0, scaley=0, gridx=NULL, gridy=NULL, skewx=0, skewy=0, algorithm=NearestNeighbor, maxerr=0.125 - Resample a raster using a specified resampling algorithm, new dimensions, an arbitrary grid corner and a set of raster georeferencing attributes defined or borrowed from another raster.';
			
COMMENT ON FUNCTION ST_Resample(raster , raster , text , double precision , boolean ) IS 'args: rast, ref, algorithm=NearestNeighbour, maxerr=0.125, usescale=true - Resample a raster using a specified resampling algorithm, new dimensions, an arbitrary grid corner and a set of raster georeferencing attributes defined or borrowed from another raster.';
			
COMMENT ON FUNCTION ST_Resample(raster , raster , boolean , text , double precision ) IS 'args: rast, ref, usescale, algorithm=NearestNeighbour, maxerr=0.125 - Resample a raster using a specified resampling algorithm, new dimensions, an arbitrary grid corner and a set of raster georeferencing attributes defined or borrowed from another raster.';
			
COMMENT ON FUNCTION ST_Rescale(raster , double precision , text , double precision ) IS 'args: rast, scalexy, algorithm=NearestNeighbour, maxerr=0.125 - Resample a raster by adjusting only its scale (or pixel size). New pixel values are computed using the NearestNeighbor (english or american spelling), Bilinear, Cubic, CubicSpline or Lanczos resampling algorithm. Default is NearestNeighbor.';
			
COMMENT ON FUNCTION ST_Rescale(raster , double precision , double precision , text , double precision ) IS 'args: rast, scalex, scaley, algorithm=NearestNeighbour, maxerr=0.125 - Resample a raster by adjusting only its scale (or pixel size). New pixel values are computed using the NearestNeighbor (english or american spelling), Bilinear, Cubic, CubicSpline or Lanczos resampling algorithm. Default is NearestNeighbor.';
			
COMMENT ON FUNCTION ST_Reskew(raster , double precision , text , double precision ) IS 'args: rast, skewxy, algorithm=NearestNeighbour, maxerr=0.125 - Resample a raster by adjusting only its skew (or rotation parameters). New pixel values are computed using the NearestNeighbor (english or american spelling), Bilinear, Cubic, CubicSpline or Lanczos resampling algorithm. Default is NearestNeighbor.';
			
COMMENT ON FUNCTION ST_Reskew(raster , double precision , double precision , text , double precision ) IS 'args: rast, skewx, skewy, algorithm=NearestNeighbour, maxerr=0.125 - Resample a raster by adjusting only its skew (or rotation parameters). New pixel values are computed using the NearestNeighbor (english or american spelling), Bilinear, Cubic, CubicSpline or Lanczos resampling algorithm. Default is NearestNeighbor.';
			
COMMENT ON FUNCTION ST_SnapToGrid(raster , double precision , double precision , text , double precision , double precision , double precision ) IS 'args: rast, gridx, gridy, algorithm=NearestNeighbour, maxerr=0.125, scalex=DEFAULT 0, scaley=DEFAULT 0 - Resample a raster by snapping it to a grid. New pixel values are computed using the NearestNeighbor (english or american spelling), Bilinear, Cubic, CubicSpline or Lanczos resampling algorithm. Default is NearestNeighbor.';
			
COMMENT ON FUNCTION ST_SnapToGrid(raster , double precision , double precision , double precision , double precision , text , double precision ) IS 'args: rast, gridx, gridy, scalex, scaley, algorithm=NearestNeighbour, maxerr=0.125 - Resample a raster by snapping it to a grid. New pixel values are computed using the NearestNeighbor (english or american spelling), Bilinear, Cubic, CubicSpline or Lanczos resampling algorithm. Default is NearestNeighbor.';
			
COMMENT ON FUNCTION ST_SnapToGrid(raster , double precision , double precision , double precision , text , double precision ) IS 'args: rast, gridx, gridy, scalexy, algorithm=NearestNeighbour, maxerr=0.125 - Resample a raster by snapping it to a grid. New pixel values are computed using the NearestNeighbor (english or american spelling), Bilinear, Cubic, CubicSpline or Lanczos resampling algorithm. Default is NearestNeighbor.';
			
COMMENT ON FUNCTION ST_Resize(raster , integer , integer , text , double precision ) IS 'args: rast, width, height, algorithm=NearestNeighbor, maxerr=0.125 - Resize a raster to a new width/height';
			
COMMENT ON FUNCTION ST_Resize(raster , double precision , double precision , text , double precision ) IS 'args: rast, percentwidth, percentheight, algorithm=NearestNeighbor, maxerr=0.125 - Resize a raster to a new width/height';
			
COMMENT ON FUNCTION ST_Resize(raster , text , text , text , double precision ) IS 'args: rast, width, height, algorithm=NearestNeighbor, maxerr=0.125 - Resize a raster to a new width/height';
			
COMMENT ON FUNCTION ST_Transform(raster , integer , text , double precision , double precision , double precision ) IS 'args: rast, srid, algorithm=NearestNeighbor, maxerr=0.125, scalex, scaley - Reprojects a raster in a known spatial reference system to another known spatial reference system using specified resampling algorithm. Options are NearestNeighbor, Bilinear, Cubic, CubicSpline, Lanczos defaulting to NearestNeighbor.';
			
COMMENT ON FUNCTION ST_Transform(raster , integer , double precision , double precision , text , double precision ) IS 'args: rast, srid, scalex, scaley, algorithm=NearestNeighbor, maxerr=0.125 - Reprojects a raster in a known spatial reference system to another known spatial reference system using specified resampling algorithm. Options are NearestNeighbor, Bilinear, Cubic, CubicSpline, Lanczos defaulting to NearestNeighbor.';
			
COMMENT ON FUNCTION ST_Transform(raster , raster , text , double precision ) IS 'args: rast, alignto, algorithm=NearestNeighbor, maxerr=0.125 - Reprojects a raster in a known spatial reference system to another known spatial reference system using specified resampling algorithm. Options are NearestNeighbor, Bilinear, Cubic, CubicSpline, Lanczos defaulting to NearestNeighbor.';
			
COMMENT ON FUNCTION ST_SetBandNoDataValue(raster , double precision ) IS 'args: rast, nodatavalue - Sets the value for the given band that represents no data. Band 1 is assumed if no band is specified. To mark a band as having no nodata value, set the nodata value = NULL.';
			
COMMENT ON FUNCTION ST_SetBandNoDataValue(raster , integer , double precision , boolean ) IS 'args: rast, band, nodatavalue, forcechecking=false - Sets the value for the given band that represents no data. Band 1 is assumed if no band is specified. To mark a band as having no nodata value, set the nodata value = NULL.';
			
COMMENT ON FUNCTION ST_SetBandIsNoData(raster , integer ) IS 'args: rast, band=1 - Sets the isnodata flag of the band to TRUE.';
			
COMMENT ON FUNCTION ST_Count(raster , integer , boolean ) IS 'args: rast, nband=1, exclude_nodata_value=true - Returns the number of pixels in a given band of a raster or raster coverage. If no band is specified defaults to band 1. If exclude_nodata_value is set to true, will only count pixels that are not equal to the nodata value.';
			
COMMENT ON FUNCTION ST_Count(raster , boolean ) IS 'args: rast, exclude_nodata_value - Returns the number of pixels in a given band of a raster or raster coverage. If no band is specified defaults to band 1. If exclude_nodata_value is set to true, will only count pixels that are not equal to the nodata value.';
			
COMMENT ON FUNCTION ST_Count(text , text , integer , boolean ) IS 'args: rastertable, rastercolumn, nband=1, exclude_nodata_value=true - Returns the number of pixels in a given band of a raster or raster coverage. If no band is specified defaults to band 1. If exclude_nodata_value is set to true, will only count pixels that are not equal to the nodata value.';
			
COMMENT ON FUNCTION ST_Count(text , text , boolean ) IS 'args: rastertable, rastercolumn, exclude_nodata_value - Returns the number of pixels in a given band of a raster or raster coverage. If no band is specified defaults to band 1. If exclude_nodata_value is set to true, will only count pixels that are not equal to the nodata value.';
			
COMMENT ON FUNCTION ST_Histogram(raster , integer , boolean , integer , double precision[] , boolean ) IS 'args: rast, nband=1, exclude_nodata_value=true, bins=autocomputed, width=NULL, right=false - Returns a set of record summarizing a raster or raster coverage data distribution separate bin ranges. Number of bins are autocomputed if not specified.';
			
COMMENT ON FUNCTION ST_Histogram(raster , integer , integer , double precision[] , boolean ) IS 'args: rast, nband, bins, width=NULL, right=false - Returns a set of record summarizing a raster or raster coverage data distribution separate bin ranges. Number of bins are autocomputed if not specified.';
			
COMMENT ON FUNCTION ST_Histogram(raster , integer , boolean , integer , boolean ) IS 'args: rast, nband, exclude_nodata_value, bins, right - Returns a set of record summarizing a raster or raster coverage data distribution separate bin ranges. Number of bins are autocomputed if not specified.';
			
COMMENT ON FUNCTION ST_Histogram(raster , integer , integer , boolean ) IS 'args: rast, nband, bins, right - Returns a set of record summarizing a raster or raster coverage data distribution separate bin ranges. Number of bins are autocomputed if not specified.';
			
COMMENT ON FUNCTION ST_Histogram(text , text , integer , integer , boolean ) IS 'args: rastertable, rastercolumn, nband, bins, right - Returns a set of record summarizing a raster or raster coverage data distribution separate bin ranges. Number of bins are autocomputed if not specified.';
			
COMMENT ON FUNCTION ST_Histogram(text , text , integer , boolean , integer , boolean ) IS 'args: rastertable, rastercolumn, nband, exclude_nodata_value, bins, right - Returns a set of record summarizing a raster or raster coverage data distribution separate bin ranges. Number of bins are autocomputed if not specified.';
			
COMMENT ON FUNCTION ST_Histogram(text , text , integer , boolean , integer , double precision[] , boolean ) IS 'args: rastertable, rastercolumn, nband=1, exclude_nodata_value=true, bins=autocomputed, width=NULL, right=false - Returns a set of record summarizing a raster or raster coverage data distribution separate bin ranges. Number of bins are autocomputed if not specified.';
			
COMMENT ON FUNCTION ST_Histogram(text , text , integer , integer , double precision[] , boolean ) IS 'args: rastertable, rastercolumn, nband=1, bins, width=NULL, right=false - Returns a set of record summarizing a raster or raster coverage data distribution separate bin ranges. Number of bins are autocomputed if not specified.';
			
COMMENT ON FUNCTION ST_Quantile(raster , integer , boolean , double precision[] ) IS 'args: rast, nband=1, exclude_nodata_value=true, quantiles=NULL - Compute quantiles for a raster or raster table coverage in the context of the sample or population. Thus, a value could be examined to be at the rasters 25%, 50%, 75% percentile.';
			
COMMENT ON FUNCTION ST_Quantile(raster , double precision[] ) IS 'args: rast, quantiles - Compute quantiles for a raster or raster table coverage in the context of the sample or population. Thus, a value could be examined to be at the rasters 25%, 50%, 75% percentile.';
			
COMMENT ON FUNCTION ST_Quantile(raster , integer , double precision[] ) IS 'args: rast, nband, quantiles - Compute quantiles for a raster or raster table coverage in the context of the sample or population. Thus, a value could be examined to be at the rasters 25%, 50%, 75% percentile.';
			
COMMENT ON FUNCTION ST_Quantile(raster , double precision ) IS 'args: rast, quantile - Compute quantiles for a raster or raster table coverage in the context of the sample or population. Thus, a value could be examined to be at the rasters 25%, 50%, 75% percentile.';
			
COMMENT ON FUNCTION ST_Quantile(raster , boolean , double precision ) IS 'args: rast, exclude_nodata_value, quantile=NULL - Compute quantiles for a raster or raster table coverage in the context of the sample or population. Thus, a value could be examined to be at the rasters 25%, 50%, 75% percentile.';
			
COMMENT ON FUNCTION ST_Quantile(raster , integer , double precision ) IS 'args: rast, nband, quantile - Compute quantiles for a raster or raster table coverage in the context of the sample or population. Thus, a value could be examined to be at the rasters 25%, 50%, 75% percentile.';
			
COMMENT ON FUNCTION ST_Quantile(raster , integer , boolean , double precision ) IS 'args: rast, nband, exclude_nodata_value, quantile - Compute quantiles for a raster or raster table coverage in the context of the sample or population. Thus, a value could be examined to be at the rasters 25%, 50%, 75% percentile.';
			
COMMENT ON FUNCTION ST_Quantile(raster , integer , double precision ) IS 'args: rast, nband, quantile - Compute quantiles for a raster or raster table coverage in the context of the sample or population. Thus, a value could be examined to be at the rasters 25%, 50%, 75% percentile.';
			
COMMENT ON FUNCTION ST_Quantile(text , text , integer , boolean , double precision[] ) IS 'args: rastertable, rastercolumn, nband=1, exclude_nodata_value=true, quantiles=NULL - Compute quantiles for a raster or raster table coverage in the context of the sample or population. Thus, a value could be examined to be at the rasters 25%, 50%, 75% percentile.';
			
COMMENT ON FUNCTION ST_Quantile(text , text , integer , double precision[] ) IS 'args: rastertable, rastercolumn, nband, quantiles - Compute quantiles for a raster or raster table coverage in the context of the sample or population. Thus, a value could be examined to be at the rasters 25%, 50%, 75% percentile.';
			
COMMENT ON FUNCTION ST_SummaryStats(text , text , boolean ) IS 'args: rastertable, rastercolumn, exclude_nodata_value - Returns record consisting of count, sum, mean, stddev, min, max for a given raster band of a raster or raster coverage. Band 1 is assumed is no band is specified.';
			
COMMENT ON FUNCTION ST_SummaryStats(raster , boolean ) IS 'args: rast, exclude_nodata_value - Returns record consisting of count, sum, mean, stddev, min, max for a given raster band of a raster or raster coverage. Band 1 is assumed is no band is specified.';
			
COMMENT ON FUNCTION ST_SummaryStats(text , text , integer , boolean ) IS 'args: rastertable, rastercolumn, nband=1, exclude_nodata_value=true - Returns record consisting of count, sum, mean, stddev, min, max for a given raster band of a raster or raster coverage. Band 1 is assumed is no band is specified.';
			
COMMENT ON FUNCTION ST_SummaryStats(raster , integer , boolean ) IS 'args: rast, nband, exclude_nodata_value - Returns record consisting of count, sum, mean, stddev, min, max for a given raster band of a raster or raster coverage. Band 1 is assumed is no band is specified.';
			
COMMENT ON FUNCTION ST_ValueCount(raster , integer , boolean , double precision[] , double precision ) IS 'args: rast, nband=1, exclude_nodata_value=true, searchvalues=NULL, roundto=0, OUT value, OUT count - Returns a set of records containing a pixel band value and count of the number of pixels in a given band of a raster (or a raster coverage) that have a given set of values. If no band is specified defaults to band 1. By default nodata value pixels are not counted. and all other values in the pixel are output and pixel band values are rounded to the nearest integer.';
			
COMMENT ON FUNCTION ST_ValueCount(raster , integer , double precision[] , double precision ) IS 'args: rast, nband, searchvalues, roundto=0, OUT value, OUT count - Returns a set of records containing a pixel band value and count of the number of pixels in a given band of a raster (or a raster coverage) that have a given set of values. If no band is specified defaults to band 1. By default nodata value pixels are not counted. and all other values in the pixel are output and pixel band values are rounded to the nearest integer.';
			
COMMENT ON FUNCTION ST_ValueCount(raster , double precision[] , double precision ) IS 'args: rast, searchvalues, roundto=0, OUT value, OUT count - Returns a set of records containing a pixel band value and count of the number of pixels in a given band of a raster (or a raster coverage) that have a given set of values. If no band is specified defaults to band 1. By default nodata value pixels are not counted. and all other values in the pixel are output and pixel band values are rounded to the nearest integer.';
			
COMMENT ON FUNCTION ST_ValueCount(raster , double precision , double precision ) IS 'args: rast, searchvalue, roundto=0 - Returns a set of records containing a pixel band value and count of the number of pixels in a given band of a raster (or a raster coverage) that have a given set of values. If no band is specified defaults to band 1. By default nodata value pixels are not counted. and all other values in the pixel are output and pixel band values are rounded to the nearest integer.';
			
COMMENT ON FUNCTION ST_ValueCount(raster , integer , boolean , double precision , double precision ) IS 'args: rast, nband, exclude_nodata_value, searchvalue, roundto=0 - Returns a set of records containing a pixel band value and count of the number of pixels in a given band of a raster (or a raster coverage) that have a given set of values. If no band is specified defaults to band 1. By default nodata value pixels are not counted. and all other values in the pixel are output and pixel band values are rounded to the nearest integer.';
			
COMMENT ON FUNCTION ST_ValueCount(raster , integer , double precision , double precision ) IS 'args: rast, nband, searchvalue, roundto=0 - Returns a set of records containing a pixel band value and count of the number of pixels in a given band of a raster (or a raster coverage) that have a given set of values. If no band is specified defaults to band 1. By default nodata value pixels are not counted. and all other values in the pixel are output and pixel band values are rounded to the nearest integer.';
			
COMMENT ON FUNCTION ST_ValueCount(text , text , integer , boolean , double precision[] , double precision ) IS 'args: rastertable, rastercolumn, nband=1, exclude_nodata_value=true, searchvalues=NULL, roundto=0, OUT value, OUT count - Returns a set of records containing a pixel band value and count of the number of pixels in a given band of a raster (or a raster coverage) that have a given set of values. If no band is specified defaults to band 1. By default nodata value pixels are not counted. and all other values in the pixel are output and pixel band values are rounded to the nearest integer.';
			
COMMENT ON FUNCTION ST_ValueCount(text , text , double precision[] , double precision ) IS 'args: rastertable, rastercolumn, searchvalues, roundto=0, OUT value, OUT count - Returns a set of records containing a pixel band value and count of the number of pixels in a given band of a raster (or a raster coverage) that have a given set of values. If no band is specified defaults to band 1. By default nodata value pixels are not counted. and all other values in the pixel are output and pixel band values are rounded to the nearest integer.';
			
COMMENT ON FUNCTION ST_ValueCount(text , text , integer , double precision[] , double precision ) IS 'args: rastertable, rastercolumn, nband, searchvalues, roundto=0, OUT value, OUT count - Returns a set of records containing a pixel band value and count of the number of pixels in a given band of a raster (or a raster coverage) that have a given set of values. If no band is specified defaults to band 1. By default nodata value pixels are not counted. and all other values in the pixel are output and pixel band values are rounded to the nearest integer.';
			
COMMENT ON FUNCTION ST_ValueCount(text , text , integer , boolean , double precision , double precision ) IS 'args: rastertable, rastercolumn, nband, exclude_nodata_value, searchvalue, roundto=0 - Returns a set of records containing a pixel band value and count of the number of pixels in a given band of a raster (or a raster coverage) that have a given set of values. If no band is specified defaults to band 1. By default nodata value pixels are not counted. and all other values in the pixel are output and pixel band values are rounded to the nearest integer.';
			
COMMENT ON FUNCTION ST_ValueCount(text , text , double precision , double precision ) IS 'args: rastertable, rastercolumn, searchvalue, roundto=0 - Returns a set of records containing a pixel band value and count of the number of pixels in a given band of a raster (or a raster coverage) that have a given set of values. If no band is specified defaults to band 1. By default nodata value pixels are not counted. and all other values in the pixel are output and pixel band values are rounded to the nearest integer.';
			
COMMENT ON FUNCTION ST_ValueCount(text , text , integer , double precision , double precision ) IS 'args: rastertable, rastercolumn, nband, searchvalue, roundto=0 - Returns a set of records containing a pixel band value and count of the number of pixels in a given band of a raster (or a raster coverage) that have a given set of values. If no band is specified defaults to band 1. By default nodata value pixels are not counted. and all other values in the pixel are output and pixel band values are rounded to the nearest integer.';
			
COMMENT ON FUNCTION ST_AsBinary(raster , boolean ) IS 'args: rast, outasin=FALSE - Return the Well-Known Binary (WKB) representation of the raster without SRID meta data.';
			
COMMENT ON FUNCTION ST_AsGDALRaster(raster , text , text[] , integer ) IS 'args: rast, format, options=NULL, srid=sameassource - Return the raster tile in the designated GDAL Raster format. Raster formats are one of those supported by your compiled library. Use ST_GDALRasters() to get a list of formats supported by your library.';
			
COMMENT ON FUNCTION ST_AsJPEG(raster , text[] ) IS 'args: rast, options=NULL - Return the raster tile selected bands as a single Joint Photographic Exports Group (JPEG) image (byte array). If no band is specified and 1 or more than 3 bands, then only the first band is used. If only 3 bands then all 3 bands are used and mapped to RGB.';
			
COMMENT ON FUNCTION ST_AsJPEG(raster , integer , integer ) IS 'args: rast, nband, quality - Return the raster tile selected bands as a single Joint Photographic Exports Group (JPEG) image (byte array). If no band is specified and 1 or more than 3 bands, then only the first band is used. If only 3 bands then all 3 bands are used and mapped to RGB.';
			
COMMENT ON FUNCTION ST_AsJPEG(raster , integer , text[] ) IS 'args: rast, nband, options=NULL - Return the raster tile selected bands as a single Joint Photographic Exports Group (JPEG) image (byte array). If no band is specified and 1 or more than 3 bands, then only the first band is used. If only 3 bands then all 3 bands are used and mapped to RGB.';
			
COMMENT ON FUNCTION ST_AsJPEG(raster , integer[] , text[] ) IS 'args: rast, nbands, options=NULL - Return the raster tile selected bands as a single Joint Photographic Exports Group (JPEG) image (byte array). If no band is specified and 1 or more than 3 bands, then only the first band is used. If only 3 bands then all 3 bands are used and mapped to RGB.';
			
COMMENT ON FUNCTION ST_AsJPEG(raster , integer[] , integer ) IS 'args: rast, nbands, quality - Return the raster tile selected bands as a single Joint Photographic Exports Group (JPEG) image (byte array). If no band is specified and 1 or more than 3 bands, then only the first band is used. If only 3 bands then all 3 bands are used and mapped to RGB.';
			
COMMENT ON FUNCTION ST_AsPNG(raster , text[] ) IS 'args: rast, options=NULL - Return the raster tile selected bands as a single portable network graphics (PNG) image (byte array). If 1, 3, or 4 bands in raster and no bands are specified, then all bands are used. If more 2 or more than 4 bands and no bands specified, then only band 1 is used. Bands are mapped to RGB or RGBA space.';
			
COMMENT ON FUNCTION ST_AsPNG(raster , integer , integer ) IS 'args: rast, nband, compression - Return the raster tile selected bands as a single portable network graphics (PNG) image (byte array). If 1, 3, or 4 bands in raster and no bands are specified, then all bands are used. If more 2 or more than 4 bands and no bands specified, then only band 1 is used. Bands are mapped to RGB or RGBA space.';
			
COMMENT ON FUNCTION ST_AsPNG(raster , integer , text[] ) IS 'args: rast, nband, options=NULL - Return the raster tile selected bands as a single portable network graphics (PNG) image (byte array). If 1, 3, or 4 bands in raster and no bands are specified, then all bands are used. If more 2 or more than 4 bands and no bands specified, then only band 1 is used. Bands are mapped to RGB or RGBA space.';
			
COMMENT ON FUNCTION ST_AsPNG(raster , integer[] , integer ) IS 'args: rast, nbands, compression - Return the raster tile selected bands as a single portable network graphics (PNG) image (byte array). If 1, 3, or 4 bands in raster and no bands are specified, then all bands are used. If more 2 or more than 4 bands and no bands specified, then only band 1 is used. Bands are mapped to RGB or RGBA space.';
			
COMMENT ON FUNCTION ST_AsPNG(raster , integer[] , text[] ) IS 'args: rast, nbands, options=NULL - Return the raster tile selected bands as a single portable network graphics (PNG) image (byte array). If 1, 3, or 4 bands in raster and no bands are specified, then all bands are used. If more 2 or more than 4 bands and no bands specified, then only band 1 is used. Bands are mapped to RGB or RGBA space.';
			
COMMENT ON FUNCTION ST_AsTIFF(raster , text[] , integer ) IS 'args: rast, options='', srid=sameassource - Return the raster selected bands as a single TIFF image (byte array). If no band is specified, then will try to use all bands.';
			
COMMENT ON FUNCTION ST_AsTIFF(raster , text , integer ) IS 'args: rast, compression='', srid=sameassource - Return the raster selected bands as a single TIFF image (byte array). If no band is specified, then will try to use all bands.';
			
COMMENT ON FUNCTION ST_AsTIFF(raster , integer[] , text , integer ) IS 'args: rast, nbands, compression='', srid=sameassource - Return the raster selected bands as a single TIFF image (byte array). If no band is specified, then will try to use all bands.';
			
COMMENT ON FUNCTION ST_AsTIFF(raster , integer[] , text[] , integer ) IS 'args: rast, nbands, options, srid=sameassource - Return the raster selected bands as a single TIFF image (byte array). If no band is specified, then will try to use all bands.';
			
COMMENT ON FUNCTION ST_Contains(raster , integer , raster , integer ) IS 'args: rastA, nbandA, rastB, nbandB - Return true if no points of raster rastB lie in the exterior of raster rastA and at least one point of the interior of rastB lies in the interior of rastA.';
			
COMMENT ON FUNCTION ST_Contains(raster , raster ) IS 'args: rastA, rastB - Return true if no points of raster rastB lie in the exterior of raster rastA and at least one point of the interior of rastB lies in the interior of rastA.';
			
COMMENT ON FUNCTION ST_ContainsProperly(raster , integer , raster , integer ) IS 'args: rastA, nbandA, rastB, nbandB - Return true if rastB intersects the interior of rastA but not the boundary or exterior of rastA.';
			
COMMENT ON FUNCTION ST_ContainsProperly(raster , raster ) IS 'args: rastA, rastB - Return true if rastB intersects the interior of rastA but not the boundary or exterior of rastA.';
			
COMMENT ON FUNCTION ST_Covers(raster , integer , raster , integer ) IS 'args: rastA, nbandA, rastB, nbandB - Return true if no points of raster rastB lie outside raster rastA.';
			
COMMENT ON FUNCTION ST_Covers(raster , raster ) IS 'args: rastA, rastB - Return true if no points of raster rastB lie outside raster rastA.';
			
COMMENT ON FUNCTION ST_CoveredBy(raster , integer , raster , integer ) IS 'args: rastA, nbandA, rastB, nbandB - Return true if no points of raster rastA lie outside raster rastB.';
			
COMMENT ON FUNCTION ST_CoveredBy(raster , raster ) IS 'args: rastA, rastB - Return true if no points of raster rastA lie outside raster rastB.';
			
COMMENT ON FUNCTION ST_Disjoint(raster , integer , raster , integer ) IS 'args: rastA, nbandA, rastB, nbandB - Return true if raster rastA does not spatially intersect rastB.';
			
COMMENT ON FUNCTION ST_Disjoint(raster , raster ) IS 'args: rastA, rastB - Return true if raster rastA does not spatially intersect rastB.';
			
COMMENT ON FUNCTION ST_Intersects(raster , integer , raster , integer ) IS 'args: rastA, nbandA, rastB, nbandB - Return true if raster rastA spatially intersects raster rastB.';
			
COMMENT ON FUNCTION ST_Intersects(raster , raster ) IS 'args: rastA, rastB - Return true if raster rastA spatially intersects raster rastB.';
			
COMMENT ON FUNCTION ST_Intersects(raster , integer , geometry ) IS 'args: rast, nband, geommin - Return true if raster rastA spatially intersects raster rastB.';
			
COMMENT ON FUNCTION ST_Intersects(raster , geometry , integer ) IS 'args: rast, geommin, nband=NULL - Return true if raster rastA spatially intersects raster rastB.';
			
COMMENT ON FUNCTION ST_Intersects(geometry , raster , integer ) IS 'args: geommin, rast, nband=NULL - Return true if raster rastA spatially intersects raster rastB.';
			
COMMENT ON FUNCTION ST_Overlaps(raster , integer , raster , integer ) IS 'args: rastA, nbandA, rastB, nbandB - Return true if raster rastA and rastB intersect but one does not completely contain the other.';
			
COMMENT ON FUNCTION ST_Overlaps(raster , raster ) IS 'args: rastA, rastB - Return true if raster rastA and rastB intersect but one does not completely contain the other.';
			
COMMENT ON FUNCTION ST_Touches(raster , integer , raster , integer ) IS 'args: rastA, nbandA, rastB, nbandB - Return true if raster rastA and rastB have at least one point in common but their interiors do not intersect.';
			
COMMENT ON FUNCTION ST_Touches(raster , raster ) IS 'args: rastA, rastB - Return true if raster rastA and rastB have at least one point in common but their interiors do not intersect.';
			
COMMENT ON FUNCTION ST_SameAlignment(raster , raster ) IS 'args: rastA, rastB - Returns true if rasters have same skew, scale, spatial ref and false if they dont with notice detailing issue.';
			
COMMENT ON FUNCTION ST_SameAlignment(double precision , double precision , double precision , double precision , double precision , double precision , double precision , double precision , double precision , double precision , double precision , double precision ) IS 'args: ulx1, uly1, scalex1, scaley1, skewx1, skewy1, ulx2, uly2, scalex2, scaley2, skewx2, skewy2 - Returns true if rasters have same skew, scale, spatial ref and false if they dont with notice detailing issue.';
			
COMMENT ON AGGREGATE ST_SameAlignment(raster) IS 'args: rastfield - Returns true if rasters have same skew, scale, spatial ref and false if they dont with notice detailing issue.';
			
COMMENT ON FUNCTION ST_SameAlignment(raster , raster ) IS 'args: rastA, rastB - Returns text stating if rasters are aligned and if not aligned, a reason why.';
			
COMMENT ON FUNCTION ST_Within(raster , integer , raster , integer ) IS 'args: rastA, nbandA, rastB, nbandB - Return true if no points of raster rastA lie in the exterior of raster rastB and at least one point of the interior of rastA lies in the interior of rastB.';
			
COMMENT ON FUNCTION ST_Within(raster , raster ) IS 'args: rastA, rastB - Return true if no points of raster rastA lie in the exterior of raster rastB and at least one point of the interior of rastA lies in the interior of rastB.';
			
COMMENT ON FUNCTION ST_DWithin(raster , integer , raster , integer , double precision ) IS 'args: rastA, nbandA, rastB, nbandB, distance_of_srid - Return true if rasters rastA and rastB are within the specified distance of each other.';
			
COMMENT ON FUNCTION ST_DWithin(raster , raster , double precision ) IS 'args: rastA, rastB, distance_of_srid - Return true if rasters rastA and rastB are within the specified distance of each other.';
			
COMMENT ON FUNCTION ST_DFullyWithin(raster , integer , raster , integer , double precision ) IS 'args: rastA, nbandA, rastB, nbandB, distance_of_srid - Return true if rasters rastA and rastB are fully within the specified distance of each other.';
			
COMMENT ON FUNCTION ST_DFullyWithin(raster , raster , double precision ) IS 'args: rastA, rastB, distance_of_srid - Return true if rasters rastA and rastB are fully within the specified distance of each other.';
			
    COMMENT ON TYPE geomval IS 'postgis raster type: A spatial datatype with two fields - geom (holding a geometry object) and val (holding a double precision pixel value from a raster band).';
            
        
    COMMENT ON TYPE addbandarg IS 'postgis raster type: A composite type used as input into the ST_AddBand function defining the attributes and initial value of the new band.';
            
        
    COMMENT ON TYPE rastbandarg IS 'postgis raster type: A composite type for use when needing to express a raster and a band index of that raster.';
            
        
    COMMENT ON TYPE raster IS 'postgis raster type: raster spatial data type.';
            
        
    COMMENT ON TYPE reclassarg IS 'postgis raster type: A composite type used as input into the ST_Reclass function defining the behavior of reclassification.';
            
        
    COMMENT ON TYPE unionarg IS 'postgis raster type: A composite type used as input into the ST_Union function defining the bands to be processed and behavior of the UNION operation.';
            
        