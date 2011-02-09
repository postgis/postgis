<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"  xmlns:pgis="http://www.postgis.org/pgis">
<!-- ********************************************************************
 * $Id$
 ********************************************************************
	 Copyright 2010, Regina Obe
	 License: BSD
	 Purpose: This is an xsl transform that generates an sql test script from xml docs to test all the raster related functions we have documented
			using a garden variety of rasters.  Its intent is to flag major crashes.
	 ******************************************************************** -->
	<xsl:output method="text" />
	<xsl:variable name='testversion'>2.0.0</xsl:variable>
	<xsl:variable name='fnexclude'>AddRasterColumn DropRasterColumn DropRasterTable</xsl:variable>
	<!--This is just a place holder to state functions not supported in 1.3 or tested separately -->

	<xsl:variable name='var_format'>'GDAL'</xsl:variable>
	<xsl:variable name='var_srid'>3395</xsl:variable>
	<xsl:variable name='var_band'>1</xsl:variable>
	<xsl:variable name='var_integer1'>1</xsl:variable>
	<xsl:variable name='var_integer2'>2</xsl:variable>
	<xsl:variable name='var_float1'>1.5</xsl:variable>
	<xsl:variable name='var_float2'>1.75</xsl:variable>
	<xsl:variable name='var_distance'>100</xsl:variable>
	<xsl:variable name='var_NDRXDR'>XDR</xsl:variable>
	<xsl:variable name='var_text'>'monkey'</xsl:variable>
	<xsl:variable name='var_varchar'>'test'</xsl:variable>
	<xsl:variable name='var_pixeltype'>'1BB'</xsl:variable>
	<xsl:variable name='var_pixeltypenoq'>8BUI</xsl:variable>
	<xsl:variable name='var_pixelvalue'>0</xsl:variable>
	<xsl:variable name='var_boolean'>false</xsl:variable>
	<xsl:variable name='var_logtable'>raster_garden_log</xsl:variable>
	<xsl:variable name='var_pixeltypes'>{8BUI,1BB}</xsl:variable>
	<xsl:variable name='var_pixelvalues'>{255,0}</xsl:variable>
	<xsl:variable name='var_pt'>ST_Centroid(rast1.rast::geometry)</xsl:variable>
	<xsl:variable name='var_georefcoords'>'2 0 0 3 0.5 0.5'</xsl:variable>
	<xsl:variable name='var_logupdatesql'>UPDATE <xsl:value-of select="$var_logtable" /> SET log_end = clock_timestamp() 
		FROM (SELECT logid FROM <xsl:value-of select="$var_logtable" /> ORDER BY logid DESC limit 1) As foo
		WHERE <xsl:value-of select="$var_logtable" />.logid = foo.logid  AND <xsl:value-of select="$var_logtable" />.log_end IS NULL;</xsl:variable>
		
	<xsl:variable name='var_logresultsasxml'>INSERT INTO <xsl:value-of select="$var_logtable" />_output(logid, log_output)
				SELECT logid, query_to_xml(log_sql, false,false,'') As log_output
				    FROM <xsl:value-of select="$var_logtable" /> ORDER BY logid DESC LIMIT 1;</xsl:variable>
				    
	<pgis:gardens>
		<pgis:gset ID='PointSet' GeometryType='POINT'>(SELECT ST_SetSRID(ST_Point(i,j),4326) As the_geom
		FROM (SELECT a*1.11111111 FROM generate_series(-10,50,10) As a) As i(i)
			CROSS JOIN generate_series(40,70, 15) j
			ORDER BY i,j
			)</pgis:gset>
		<pgis:gset ID='LineSet' GeometryType='LINESTRING'>(SELECT ST_MakeLine(ST_SetSRID(ST_Point(i,j),4326),ST_SetSRID(ST_Point(j,i),4326))  As the_geom
		FROM (SELECT a*1.11111111 FROM generate_series(-10,50,10) As a) As i(i)
			CROSS JOIN generate_series(40,70, 15) As j
			WHERE NOT(i = j)
			ORDER BY i, i*j)</pgis:gset>
		<pgis:gset ID='PolySet' GeometryType='POLYGON'>(SELECT ST_Buffer(ST_SetSRID(ST_Point(i,j),4326), j*0.05)  As the_geom
		FROM (SELECT a*1.11111111 FROM generate_series(-10,50,10) As a) As i(i)
			CROSS JOIN generate_series(40,70, 20) As j
			ORDER BY i, i*j, j)</pgis:gset>

		<pgis:gset ID="SingleNULL" GeometryType="GEOMETRY" createtable="false">(SELECT CAST(Null As geometry) As the_geom)</pgis:gset>
		<pgis:gset ID="MultipleNULLs" GeometryType="GEOMETRY" createtable="false">(SELECT CAST(Null As geometry) As the_geom FROM generate_series(1,4) As foo)</pgis:gset>
	</pgis:gardens>
	<!--changed all to no skew so they pass the world tests -->
	<pgis:pixeltypes>
		 <pgis:pixeltype ID="1BB" PixType="1BB" createtable="true" nodata="0">
		 	(SELECT ST_SetSRID(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster( 100, 100, (i-1)*100, (i-1)*100, 0.0005, -0.0005, 0*i, 0*i), '1BB'), i, (i+1),0),4326) As rast
		 		FROM generate_series(1,10) As i)
		 </pgis:pixeltype>
		 <pgis:pixeltype ID="2BUI" PixType="2BUI" createtable="true" nodata="2">
		 	(SELECT ST_SetSRID(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster( 100, 100, (i-1)*100, (i-1)*100, 0.0005, -0.0005, 0*i, 0*i), '2BUI'), i, (i+1),1),4326) As rast
		 		FROM generate_series(1,10) As i)
		 </pgis:pixeltype>
		 <pgis:pixeltype ID="4BUI" PixType="4BUI" createtable="true" nodata="15">
		 	(SELECT ST_SetSRID(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster( 100, 100, (i-1)*100, (i-1)*100, 0.0005, -0.0005, 0*i, 0*i), '4BUI'), i, (i+1),14),4326) As rast
		 		FROM generate_series(1,10) As i)
		 </pgis:pixeltype>
		 <pgis:pixeltype ID="8BSI" PixType="8BSI" createtable="true" nodata="-56">
		 	(SELECT ST_SetSRID(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster( 100, 100, (i-1)*100, (i-1)*100, 0.0005, -0.0005, 0*i, 0*i), '8BSI'), i, (i+1),-50),4326) As rast
		 		FROM generate_series(1,10) As i)
		 </pgis:pixeltype>
		 <pgis:pixeltype ID="8BUI" PixType="8BUI" createtable="true" nodata="255">
		 	(SELECT ST_SetSRID(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster( 100, 100, (i-1)*100, (i-1)*100, 0.0005, -0.0005, 0*i, 0*i), '8BUI'), i, (i+1),150),4326) As rast
		 		FROM generate_series(1,10) As i)
		 </pgis:pixeltype>
		 <pgis:pixeltype ID="16BSI" PixType="16BSI" createtable="true" nodata="0">
		 	(SELECT ST_SetSRID(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster( 100, 100, (i-1)*100, (i-1)*100, 0.0005, -0.0005, 0*i, 0*i), '16BSI'), i, (i+1),-6000),4326) As rast
		 		FROM generate_series(1,10) As i)
		 </pgis:pixeltype>
		 <pgis:pixeltype ID="16BUI" PixType="16BUI" createtable="true" nodata="65535">
		 	(SELECT ST_SetSRID(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster( 100, 100, (i-1)*100, (i-1)*100, 0.0005, -0.0005, 0*i, 0*i), '16BUI'), i, (i+1),64567),4326) As rast
		 		FROM generate_series(1,10) As i)
		 </pgis:pixeltype>
		 <pgis:pixeltype ID="32BSI" PixType="32BSI" createtable="true" nodata="-4294967295">
		 	(SELECT ST_SetSRID(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster( 100, 100, (i-1)*100, (i-1)*100, 0.0005, -0.0005, 0*i, 0*i), '32BSI'), i, (i+1),-429496),4326) As rast
		 		FROM generate_series(1,10) As i)
		 </pgis:pixeltype>
		 <pgis:pixeltype ID="32BUI" PixType="32BUI" createtable="true" nodata="4294967295">
		 	(SELECT ST_SetSRID(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster( 100, 100, (i-1)*100, (i-1)*100, 0.0005, -0.0005, 0*i, 0*i), '32BUI'), i, (i+1),42949),4326) As rast
		 		FROM generate_series(1,10) As i)
		 </pgis:pixeltype>
		 <pgis:pixeltype ID="32BF" PixType="32BF" createtable="true" nodata="-4294.967295">
		 	(SELECT ST_SetSRID(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster( 100, 100, (i-1)*100, (i-1)*100, 0.0005, -0.0005, 0*i, 0*i), '32BF'), i, (i+1),-4294),4326) As rast
		 		FROM generate_series(1,10) As i)
		 </pgis:pixeltype>
		  <pgis:pixeltype ID="64BF" PixType="64BF" createtable="true" nodata="429496.7295">
		 	(SELECT ST_SetSRID(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster( 100, 100, (i-1)*100, (i-1)*100, 0.0005, -0.0005, 0*i, 0*i), '64BF'), i, (i+1),42949.12345),4326) As rast
		 		FROM generate_series(1,10) As i)
		 </pgis:pixeltype>
	</pgis:pixeltypes>
        <!-- We deal only with the reference chapter -->
        <xsl:template match="/">
<!-- Create logging table -->
<!-- Create logging tables -->
DROP TABLE IF EXISTS <xsl:value-of select="$var_logtable" />;
CREATE TABLE <xsl:value-of select="$var_logtable" />(logid serial PRIMARY KEY, log_label text, spatial_class text DEFAULT 'raster', func text, g1 text, g2 text, log_start timestamp, log_end timestamp, log_sql text);
DROP TABLE IF EXISTS <xsl:value-of select="$var_logtable" />_output;
CREATE TABLE <xsl:value-of select="$var_logtable" />_output(logid integer PRIMARY KEY, log_output xml);
                <xsl:apply-templates select="/book/chapter[@id='RT_reference']" />
        </xsl:template>
	<xsl:template match='chapter'>
<!-- define a table we call pgis_rgarden_mega that will contain a raster column with a band for all types of pixels we support -->
DROP TABLE IF EXISTS pgis_rgarden_mega;
CREATE TABLE pgis_rgarden_mega(rid serial PRIMARY KEY, rast raster);
<!--Start Test table creation -->
		<xsl:for-each select="document('')//pgis:pixeltypes/pgis:pixeltype[not(contains(@createtable,'false'))]">
			<xsl:variable name='log_label'>create table Test <xsl:value-of select="@PixType" /></xsl:variable>
SELECT '<xsl:value-of select="$log_label" />: Start Testing';
<xsl:variable name='var_sql'>CREATE TABLE pgis_rgarden_<xsl:value-of select="@ID" />(rid serial PRIMARY KEY);
	SELECT AddRasterColumn('public', lower('pgis_rgarden_<xsl:value-of select="@ID" />'), 'rast',4326, '{<xsl:value-of select="@PixType" />}',false, true, '{<xsl:value-of select="@nodata" />}', 0.25,-0.25,200,300, null);
	SELECT AddRasterColumn('public', lower('pgis_rgarden_<xsl:value-of select="@ID" />'),'r_rasttothrow', 4326, '{<xsl:value-of select="@PixType" />,<xsl:value-of select="$var_pixeltypenoq" />}',false, true, '{<xsl:value-of select="@nodata" />, <xsl:value-of select="$var_pixelvalue" />}', 0.25,-0.25,200,300, null);</xsl:variable>
INSERT INTO <xsl:value-of select="$var_logtable" />(log_label,  func, g1, log_start,log_sql) 
VALUES('<xsl:value-of select="$log_label" /> AddRasterColumn','AddRasterColumn', '<xsl:value-of select="@PixType" />', clock_timestamp(),
	  '<xsl:call-template name="escapesinglequotes"><xsl:with-param name="arg1"><xsl:value-of select="$var_sql" /></xsl:with-param></xsl:call-template>');
BEGIN;
	<xsl:value-of select="$var_sql" />
	<xsl:value-of select="$var_logupdatesql" />
COMMIT;<xsl:text> 
</xsl:text>
INSERT INTO <xsl:value-of select="$var_logtable" />(log_label, func, g1, log_start,log_sql) 
VALUES('<xsl:value-of select="$log_label" /> insert data raster','insert data', '<xsl:value-of select="@PixType" />', clock_timestamp(),
  '<xsl:call-template name="escapesinglequotes"><xsl:with-param name="arg1">INSERT INTO pgis_rgarden_mega(rast)
	SELECT rast
	FROM (<xsl:value-of select="." />) As foo;</xsl:with-param></xsl:call-template>');
BEGIN;
	INSERT INTO pgis_rgarden_mega(rast)
	SELECT rast
	FROM (<xsl:value-of select="." />) As foo;
	<xsl:value-of select="$var_logupdatesql" />
COMMIT;	
		</xsl:for-each>
<!--End Test table creation  -->

<!--Start Test table drop -->
		<xsl:for-each select="document('')//pgis:pixeltypes/pgis:pixeltype[not(contains(@createtable,'false'))]">
			<xsl:variable name='log_label'>drop table Test <xsl:value-of select="@PixType" /></xsl:variable>
			<xsl:variable name='var_sql'>SELECT DropRasterTable('public', lower('pgis_rgarden_<xsl:value-of select="@ID" />'));</xsl:variable>
SELECT '<xsl:value-of select="$log_label" />: Start Testing';
INSERT INTO <xsl:value-of select="$var_logtable" />(log_label, func, g1, log_start, log_sql) 
VALUES('<xsl:value-of select="$log_label" /> DropRasterTable','DropRasterTable', '<xsl:value-of select="@PixType" />', clock_timestamp(),
'<xsl:call-template name="escapesinglequotes"><xsl:with-param name="arg1"><xsl:value-of select="$var_sql" /></xsl:with-param></xsl:call-template>');
BEGIN;
	<xsl:value-of select="$var_sql" />
	<xsl:value-of select="$var_logupdatesql" />
COMMIT;<xsl:text> 
</xsl:text>
		</xsl:for-each>
<!--End Test table drop -->

<!--Start test on operators  -->
	<xsl:for-each select="sect1[contains(@id,'RT_Operator')]/refentry">
		<xsl:sort select="@id"/>
		<xsl:for-each select="refsynopsisdiv/funcsynopsis/funcprototype">
			<xsl:variable name='fnname'><xsl:value-of select="funcdef/function"/></xsl:variable>
			<xsl:variable name='fndef'><xsl:value-of select="funcdef"/></xsl:variable>
			<xsl:for-each select="document('')//pgis:pixeltypes/pgis:pixeltype">
			<!--Store first garden sql raster from -->
					<xsl:variable name="from1"><xsl:value-of select="." /></xsl:variable>
					<xsl:variable name='pix1type'><xsl:value-of select="@PixType"/></xsl:variable>
					<xsl:variable name='log_label'><xsl:value-of select="$fnname" /><xsl:text> </xsl:text><xsl:value-of select="@ID" /> against other types</xsl:variable>
		SELECT '<xsl:value-of select="$log_label" />: Start Testing ';
						<xsl:for-each select="document('')//pgis:pixeltypes/pgis:pixeltype">
		<xsl:choose>
			  <xsl:when test="contains($fndef, 'geometry')">
			 SELECT 'Geometry <xsl:value-of select="$fnname" /><xsl:text> </xsl:text><xsl:value-of select="@ID" />: Start Testing <xsl:value-of select="$geom1type" />, <xsl:value-of select="@GeometryTypeype" />';
			 <xsl:variable name='var_sql'>SELECT foo1.the_geom <xsl:value-of select="$fnname" /> foo2.the_geom
						FROM (<xsl:value-of select="$from1" />) As foo1 CROSS JOIN (<xsl:value-of select="." />) As foo2;</xsl:variable>
			 INSERT INTO <xsl:value-of select="$var_logtable" />(log_label, func, g1, g2, log_start,log_sql) 
			  	VALUES('<xsl:value-of select="$log_label" /> Geometry <xsl:value-of select="$geom1type" /><xsl:text> </xsl:text><xsl:value-of select="@PixType" />','<xsl:value-of select="$fnname" />', '<xsl:value-of select="$geom1type" />','<xsl:value-of select="@GeometryType" />', clock_timestamp(),
			  	'<xsl:call-template name="escapesinglequotes"><xsl:with-param name="arg1"><xsl:value-of select="$var_sql" /></xsl:with-param></xsl:call-template>');

			BEGIN;
				<!--  log query result to output table -->
				<xsl:value-of select="$var_logresultsasxml" />
				<xsl:value-of select="$var_logupdatesql" />
			COMMIT;
			</xsl:when>
			<xsl:otherwise>
			<xsl:variable name='var_sql'>SELECT rast1.rast <xsl:value-of select="$fnname" /> rast2.rast
				FROM (<xsl:value-of select="$from1" />) As rast1 CROSS JOIN (<xsl:value-of select="." />) As rast2;</xsl:variable>
			SELECT 'Raster <xsl:value-of select="$fnname" /><xsl:text> </xsl:text><xsl:value-of select="@ID" />: Start Testing <xsl:value-of select="$pix1type" />, <xsl:value-of select="@PixType" />';
			 INSERT INTO <xsl:value-of select="$var_logtable" />(log_label, func, g1, g2, log_start, log_sql) 
			  	VALUES('<xsl:value-of select="$log_label" /> Raster <xsl:value-of select="$pix1type" /><xsl:text> </xsl:text><xsl:value-of select="@PixType" />','<xsl:value-of select="$fnname" />', '<xsl:value-of select="$pix1type" />','<xsl:value-of select="@PixType" />', clock_timestamp(),
			  		'<xsl:call-template name="escapesinglequotes"><xsl:with-param name="arg1"><xsl:value-of select="$var_sql" /></xsl:with-param></xsl:call-template>');

			BEGIN;
				<!--  log query result to output table -->
				<xsl:value-of select="$var_logresultsasxml" />
				<xsl:value-of select="$var_logupdatesql" />
			COMMIT;
			</xsl:otherwise>
		</xsl:choose>
					</xsl:for-each>
		SELECT '<xsl:value-of select="$fnname" /><xsl:text> </xsl:text><xsl:value-of select="@ID" />: End Testing <xsl:value-of select="@PixType" /> against other types';
				</xsl:for-each>
			</xsl:for-each>
	</xsl:for-each>
<!--End test on operators -->

<!-- Start regular function checks excluding operators -->
		<xsl:for-each select="sect1[not(contains(@id,'Operator'))]/refentry">
		<xsl:sort select="@id"/>

			<xsl:for-each select="refsynopsisdiv/funcsynopsis/funcprototype">
<!--Create dummy parameters to be used later -->
				<xsl:variable name='fnfakeparams'><xsl:call-template name="replaceparams"><xsl:with-param name="func" select="." /></xsl:call-template></xsl:variable>
				<xsl:variable name='fnargs'><xsl:call-template name="listparams"><xsl:with-param name="func" select="." /></xsl:call-template></xsl:variable>
				<xsl:variable name='fnname'><xsl:value-of select="funcdef/function"/></xsl:variable>
				<xsl:variable name='fndef'><xsl:value-of select="funcdef"/></xsl:variable>
				<xsl:variable name='numparams'><xsl:value-of select="count(paramdef/parameter)" /></xsl:variable>
				<xsl:variable name='numparamgeoms'><xsl:value-of select="count(paramdef/type[contains(text(),'geometry') or contains(text(),'geography') or contains(text(),'box') or contains(text(), 'bytea')]) + count(paramdef/parameter[contains(text(),'WKT')]) + count(paramdef/parameter[contains(text(),'geomgml')])" /></xsl:variable>
				<xsl:variable name='numparamrasts'><xsl:value-of select="count(paramdef/type[contains(text(),'raster')] )" /></xsl:variable>
				<xsl:variable name='log_label'><xsl:value-of select="funcdef/function" />(<xsl:value-of select="$fnargs" />)</xsl:variable>

				<xsl:variable name="geoftype">
				  <!--Conditionally instantiate a value to be assigned to the variable -->
				  <xsl:choose>
					<xsl:when test="contains(paramdef, 'geometry ')">
					  <xsl:value-of select="Geometry"/>
					</xsl:when>
					<xsl:when test="contains(paramdef, 'geography ')">
					  <xsl:value-of select="Geography"/>
					</xsl:when>
					<xsl:when test="contains(paramdef, 'raster ')">
					  <xsl:value-of select="Raster"/>
					</xsl:when>
					<xsl:otherwise>
					  <xsl:value-of select="Other"/>
					</xsl:otherwise>
				  </xsl:choose>
				</xsl:variable>

				<!-- For each function prototype generate a test sql statement -->
				<xsl:choose>
<!--Test functions that take no arguments or take no geometries -->
	<xsl:when test="$numparamrasts = '0' and not(contains($fnexclude,funcdef/function))">SELECT  'Starting <xsl:value-of select="funcdef/function" />(<xsl:value-of select="$fnargs" />)';
	<xsl:variable name='var_sql'>SELECT  <xsl:value-of select="funcdef/function" />(<xsl:value-of select="$fnfakeparams" />);</xsl:variable>
INSERT INTO <xsl:value-of select="$var_logtable" />(log_label, func, log_start, log_sql) 
			  	VALUES('<xsl:value-of select="$log_label" />','<xsl:value-of select="$fnname" />', clock_timestamp(),
			  	'<xsl:call-template name="escapesinglequotes"><xsl:with-param name="arg1"><xsl:value-of select="$var_sql" /></xsl:with-param></xsl:call-template>');
	
	BEGIN;
		<!--  log query result to output table -->
		<xsl:value-of select="$var_logresultsasxml" />
		<!-- log completion -->
		<xsl:value-of select="$var_logupdatesql" />
	COMMIT;
SELECT  'Ending <xsl:value-of select="funcdef/function" />(<xsl:value-of select="$fnargs" />)';
	</xsl:when>
<!--Start Test aggregate and unary functions -->
<!-- put functions that take only one raster no need to cross with another raster collection, these are unary raster, aggregates, and so forth -->
	<xsl:when test="$numparamrasts = '1' and $numparamgeoms = '0'  and not(contains($fnexclude,funcdef/function))" >
		<xsl:for-each select="document('')//pgis:pixeltypes/pgis:pixeltype">
		SELECT '<xsl:value-of select="$geoftype" /> <xsl:value-of select="$fnname" /><xsl:text> </xsl:text><xsl:value-of select="@ID" />: Start Testing <xsl:value-of select="@PixType" />';
			<xsl:choose>
			  <xsl:when test="contains(paramdef, 'raster ')">
	 <!-- If output is raster show ewkt convexhull rep -->
	 		INSERT INTO <xsl:value-of select="$var_logtable" />(log_label, func, g1, log_start, log_sql) 
			  	VALUES('<xsl:value-of select="$log_label" /> <xsl:value-of select="$geoftype" /> <xsl:text> </xsl:text><xsl:value-of select="@ID" /><xsl:text> </xsl:text><xsl:value-of select="@PixType" />','<xsl:value-of select="$fnname" />', '<xsl:value-of select="@PixType" />', clock_timestamp(),
			  		'<xsl:call-template name="escapesinglequotes"><xsl:with-param name="arg1">SELECT ST_AsEWKT(ST_ConvexHull(<xsl:value-of select="$fnname" />(<xsl:value-of select="$fnfakeparams" />))) FROM (<xsl:value-of select="." />) As rast1 LIMIT 3;</xsl:with-param></xsl:call-template>'
			  	);
			  </xsl:when>
			  <xsl:otherwise>
				SELECT 'Other <xsl:value-of select="$fnname" /><xsl:text> </xsl:text><xsl:value-of select="@ID" />: Start Testing <xsl:value-of select="@GeometryType" />';
				 <!-- If output is geometry show ewkt rep -->
				INSERT INTO <xsl:value-of select="$var_logtable" />(log_label, func, g1, log_start, log_sql) 
			  	VALUES('<xsl:value-of select="$log_label" /> <xsl:value-of select="$geoftype" /> <xsl:text> </xsl:text><xsl:value-of select="@ID" /><xsl:text> </xsl:text><xsl:value-of select="@PixType" />','<xsl:value-of select="$fnname" />', '<xsl:value-of select="@PixType" />', clock_timestamp(),
			  		'<xsl:call-template name="escapesinglequotes"><xsl:with-param name="arg1">SELECT <xsl:value-of select="$fnname" />(<xsl:value-of select="$fnfakeparams" />) FROM (<xsl:value-of select="." />) As rast1 LIMIT 3;</xsl:with-param></xsl:call-template>'
			  	);
			  </xsl:otherwise>
			</xsl:choose>
		
		BEGIN;
		<!--  log query result to output table -->
		<xsl:value-of select="$var_logresultsasxml" />		
			<!-- log completion -->
		<xsl:value-of select="$var_logupdatesql" />
		  COMMIT;
		  SELECT '<xsl:value-of select="$fnname" /><xsl:text> </xsl:text> <xsl:value-of select="@ID" />: End Testing <xsl:value-of select="@PixType" />';
			<xsl:text>
	
			</xsl:text>
		</xsl:for-each>
	</xsl:when>

<!--Functions more than 1 args not already covered this will cross every raster pixel type with every other -->
	<xsl:when test="not(contains($fnexclude,funcdef/function))">
		<xsl:for-each select="document('')//pgis:pixeltypes/pgis:pixeltype">
<!-- log to results table -->
		SELECT '<xsl:value-of select="$geoftype" /> <xsl:value-of select="$fnname" /><xsl:text> </xsl:text><xsl:value-of select="@ID" />: Start Testing <xsl:value-of select="@PixType" />';

	<!--Store first garden sql geometry from -->
					<xsl:variable name="from1"><xsl:value-of select="." /></xsl:variable>
					<xsl:variable name='pix1type'><xsl:value-of select="@PixType"/></xsl:variable>
					
		SELECT '<xsl:value-of select="$fnname" /> <xsl:text> </xsl:text><xsl:value-of select="@ID" />(<xsl:value-of select="$fnargs" />): Start Testing <xsl:value-of select="$pix1type" /> against other types';
						<xsl:for-each select="document('')//pgis:gardens/pgis:gset">
						
			INSERT INTO <xsl:value-of select="$var_logtable" />(log_label, func, g1, g2, log_start) 
			  	VALUES('<xsl:value-of select="$log_label" /> <xsl:value-of select="$pix1type" /> <xsl:text> </xsl:text><xsl:value-of select="@ID" /><xsl:text> </xsl:text>','<xsl:value-of select="$fnname" />', '<xsl:value-of select="$pix1type" />','<xsl:value-of select="@GeometryType" />', clock_timestamp());
			BEGIN;
					<xsl:choose>
						<xsl:when test="$numparamrasts > '1'">
						SELECT 'Raster <xsl:value-of select="$fnname" /><xsl:text> </xsl:text><xsl:value-of select="$pix1type" />(<xsl:value-of select="$fnargs" />): Start Testing <xsl:value-of select="$pix1type" />, <xsl:value-of select="@GeometryType" />';
			<!-- If input is raster show wkt rep -->
			SELECT <xsl:value-of select="$fnname" />(<xsl:value-of select="$fnfakeparams" />), ST_AsText(ST_ConvexHull(rast1.rast)) As ref1_geom, ST_AsText(ST_ConvexHull(rast2.rast)) As ref2_geom
					  </xsl:when>
					  <xsl:when test="$numparamgeoms > '0'">
						SELECT 'Geometry <xsl:value-of select="$fnname" /><xsl:text> </xsl:text><xsl:value-of select="@ID" />(<xsl:value-of select="$fnargs" />): Start Testing <xsl:value-of select="$pix1type" />, <xsl:value-of select="@GeometryType" />';
			<!-- If input is geometry show ewkt rep -->
			SELECT <xsl:value-of select="$fnname" />(<xsl:value-of select="$fnfakeparams" />), ST_AsEWKT(rast1.rast::geometry) As ref1_geom, ST_AsEWKT(foo2.the_geom) As ref2_geom
					  </xsl:when>
					  <xsl:otherwise>
						SELECT 'Other <xsl:value-of select="$fnname" /><xsl:text> </xsl:text><xsl:value-of select="@ID" />(<xsl:value-of select="$fnargs" />): Start Testing <xsl:value-of select="$pix1type" />, <xsl:value-of select="@GeometryType" />';
			<!-- If input is geography show wkt rep -->
			SELECT <xsl:value-of select="$fnname" />(<xsl:value-of select="$fnfakeparams" />)
					  </xsl:otherwise>
					</xsl:choose>
					FROM (<xsl:value-of select="$from1" />) As rast1 CROSS JOIN (<xsl:value-of select="." />) As foo2
					LIMIT 2;
			<!-- log completion -->
			<xsl:value-of select="$var_logupdatesql" />
	COMMIT;
	SELECT '<xsl:value-of select="$fnname" />(<xsl:value-of select="$fnargs" />) <xsl:text> </xsl:text> <xsl:value-of select="@ID" />: End Testing <xsl:value-of select="$pix1type" />, <xsl:value-of select="@PixType" />';
		<xsl:text>

		</xsl:text>
			</xsl:for-each>
SELECT '<xsl:value-of select="$fnname" /><xsl:text> </xsl:text><xsl:value-of select="@ID" />(<xsl:value-of select="$fnargs" />): End Testing <xsl:value-of select="@PixType" /> against other types';
		</xsl:for-each>
		</xsl:when>
	</xsl:choose>
			</xsl:for-each>
		</xsl:for-each>
	</xsl:template>

	<!--macro to replace func args with dummy var args -->
	<xsl:template name="replaceparams">
		<xsl:param name="func" />
		<xsl:for-each select="$func">
			<xsl:for-each select="paramdef">
				<xsl:choose>
					<xsl:when test="contains(parameter, 'georefcoords')">
						<xsl:value-of select="$var_georefcoords" />
					</xsl:when>
					<xsl:when test="contains(parameter, 'format')">
						<xsl:value-of select="$var_format" />
					</xsl:when>
					<xsl:when test="contains(parameter, 'pt')">
						<xsl:value-of select="$var_pt" />
					</xsl:when>
					<xsl:when test="contains(parameter, 'pixeltype')">
						<xsl:value-of select="$var_pixeltype" />
					</xsl:when>
					<xsl:when test="contains(parameter, 'distance')">
						<xsl:value-of select="$var_distance" />
					</xsl:when>
					<xsl:when test="contains(parameter, 'srid')">
						<xsl:value-of select="$var_srid" />
					</xsl:when>
					<xsl:when test="contains(parameter, 'position')">
						<xsl:value-of select="$var_position" />
					</xsl:when>
					<xsl:when test="contains(parameter, 'NDR')">
						'<xsl:value-of select="$var_NDRXDR" />'
					</xsl:when>
					<xsl:when test="contains(parameter, 'version') and position() = 2">
						<xsl:value-of select="$var_version1" />
					</xsl:when>
					<xsl:when test="(contains(type,'box') or type = 'geometry' or type = 'geometry ' or contains(type,'geometry set')) and (position() = 1 or count($func/paramdef/type[contains(text(),'geometry') or contains(text(),'box') or contains(text(), 'WKT') or contains(text(), 'bytea')]) = '1')">
						<xsl:text>foo2.the_geom</xsl:text>
					</xsl:when>
					<xsl:when test="(type = 'geography' or type = 'geography ' or contains(type,'geography set')) and (position() = 1 or count($func/paramdef/type[contains(text(),'geography')]) = '1' )">
						<xsl:text>rast1.rast::geometry::geography</xsl:text>
					</xsl:when>
					<xsl:when test="contains(type,'box') or type = 'geometry' or type = 'geometry '">
						<xsl:text>rast1.rast::geometry</xsl:text>
					</xsl:when>
					
					<xsl:when test="contains(type,'box') or type = 'geometry' or type = 'geometry '">
						<xsl:text>foo2.the_geom</xsl:text>
					</xsl:when>
					<xsl:when test="type = 'geography' or type = 'geography '">
						<xsl:text>geography(foo2.the_geom)</xsl:text>
					</xsl:when>
					<xsl:when test="type = 'raster' or type = 'raster '">
						<xsl:text>rast1.rast</xsl:text>
					</xsl:when>
					<xsl:when test="type = 'raster' or type = 'raster '">
						<xsl:text>rast2.rast</xsl:text>
					</xsl:when>
					<xsl:when test="contains(type, 'geometry[]') and count($func/paramdef/type[contains(text(),'geometry') or contains(text(),'box') or contains(text(), 'WKT') or contains(text(), 'bytea')]) = '1'">
						ARRAY[foo1.the_geom]
					</xsl:when>
					<xsl:when test="contains(type, 'geometry[]')">
						ARRAY[foo2.the_geom]
					</xsl:when>
					<xsl:when test="contains(parameter, 'EWKT')">
						<xsl:text>ST_AsEWKT(foo1.the_geom)</xsl:text>
					</xsl:when>
					<xsl:when test="contains(parameter, 'WKT')">
						<xsl:text>ST_AsText(foo1.the_geom)</xsl:text>
					</xsl:when>
					<xsl:when test="contains(parameter, 'EWKB')">
						<xsl:text>ST_AsEWKB(foo1.the_geom)</xsl:text>
					</xsl:when>
					<xsl:when test="contains(type, 'bytea')">
						<xsl:text>ST_AsBinary(foo1.the_geom)</xsl:text>
					</xsl:when>
					<xsl:when test="contains(type, 'float') or contains(type, 'double')">
						<xsl:value-of select="$var_float1" />
					</xsl:when>
					<xsl:when test="contains(type, 'spheroid')">
						<xsl:value-of select="$var_spheroid" />
					</xsl:when>
					<xsl:when test="contains(type, 'integer') and position() = 2">
						<xsl:value-of select="$var_integer1" />
					</xsl:when>
					<xsl:when test="contains(type, 'integer')">
						<xsl:value-of select="$var_integer2" />
					</xsl:when>
					<xsl:when test="contains(type, 'text')">
						<xsl:value-of select="$var_text" />
					</xsl:when>
					<xsl:when test="contains(type, 'varchar')">
						<xsl:value-of select="$var_varchar" />
					</xsl:when>
					<xsl:when test="contains(type,'timestamp') or type = 'date'">
						<xsl:text>'2009-01-01'</xsl:text>
					</xsl:when>
					<xsl:when test="contains(type,'boolean')">
						<xsl:value-of select="$var_boolean" />
					</xsl:when>
				</xsl:choose>
				<xsl:if test="position()&lt;last()"><xsl:text>, </xsl:text></xsl:if>
			</xsl:for-each>
		</xsl:for-each>
	</xsl:template>

	<!--macro to pull out function parameter names so we can provide a pretty arg list prefix for each function -->
	<xsl:template name="listparams">
		<xsl:param name="func" />
		<xsl:for-each select="$func">
			<xsl:if test="count(paramdef/parameter) &gt; 0"> </xsl:if>
			<xsl:for-each select="paramdef">
				<xsl:choose>
					<xsl:when test="count(parameter) &gt; 0">
						<xsl:value-of select="parameter" />
					</xsl:when>
				</xsl:choose>
				<xsl:if test="position()&lt;last()"><xsl:text>, </xsl:text></xsl:if>
			</xsl:for-each>
		</xsl:for-each>
	</xsl:template>
	
	<!-- copied from http://www.thedumbterminal.co.uk/php/knowledgebase/?action=view&id=94 -->
    <xsl:template name="escapesinglequotes">
     <xsl:param name="arg1"/>
     <xsl:variable name="apostrophe">'</xsl:variable>
     <xsl:choose>
      <!-- this string has at least on single quote -->
      <xsl:when test="contains($arg1, $apostrophe)">
      <xsl:if test="string-length(normalize-space(substring-before($arg1, $apostrophe))) > 0"><xsl:value-of select="substring-before($arg1, $apostrophe)" disable-output-escaping="yes"/>''</xsl:if>
       <xsl:call-template name="escapesinglequotes">
        <xsl:with-param name="arg1"><xsl:value-of select="substring-after($arg1, $apostrophe)" disable-output-escaping="yes"/></xsl:with-param>
       </xsl:call-template>
      </xsl:when>
      <!-- no quotes found in string, just print it -->
      <xsl:when test="string-length(normalize-space($arg1)) > 0"><xsl:value-of select="normalize-space($arg1)"/></xsl:when>
     </xsl:choose>
    </xsl:template>
</xsl:stylesheet>
