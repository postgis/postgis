<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"  xmlns:pgis="http://postgis.refractions.net/pgis">
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

	<xsl:variable name='var_srid'>3395</xsl:variable>
	<xsl:variable name='var_band'>1</xsl:variable>
	<xsl:variable name='var_integer1'>3</xsl:variable>
	<xsl:variable name='var_integer2'>5</xsl:variable>
	<xsl:variable name='var_float1'>0.5</xsl:variable>
	<xsl:variable name='var_float2'>0.75</xsl:variable>
	<xsl:variable name='var_distance'>100</xsl:variable>
	<xsl:variable name='var_NDRXDR'>XDR</xsl:variable>
	<xsl:variable name='var_text'>'monkey'</xsl:variable>
	<xsl:variable name='var_varchar'>'test'</xsl:variable>
	<xsl:variable name='var_pixeltype'>'1BB'</xsl:variable>
	<xsl:variable name='var_boolean'>false</xsl:variable>
	<xsl:variable name='var_logtable'>raster_garden_log</xsl:variable>
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
	<pgis:pixeltypes>
		 <pgis:pixeltype ID="1BB" PixType="1BB" createtable="false" nodata="0"/>
		 <pgis:pixeltype ID="2BUI" PixType="2BUI" createtable="false" nodata="0"/>
	</pgis:pixeltypes>

        <!-- We deal only with the reference chapter -->
        <xsl:template match="/">
<!-- Create logging table -->
DROP TABLE IF EXISTS <xsl:value-of select="$var_logtable" />;
CREATE TABLE <xsl:value-of select="$var_logtable" />(logid serial PRIMARY KEY, log_label text, func text, g1 text, g2 text, log_start timestamp, log_end timestamp);
                <xsl:apply-templates select="/book/chapter[@id='reference_RT']" />
        </xsl:template>

	<xsl:template match='chapter'>
<!--Start Test table creation, insert, analyze crash test, drop -->
		<xsl:for-each select="document('')//pgis:pixeltypes/pgis:pixeltyp[not(contains(@createtable,'false'))]">
			<xsl:variable name='log_label'>create,insert,drop Test <xsl:value-of select="@PixType" /></xsl:variable>
SELECT '<xsl:value-of select="$log_label" />: Start Testing';
INSERT INTO <xsl:value-of select="$var_logtable" />(log_label, func, g1, log_start) 
VALUES('<xsl:value-of select="$log_label" /> AddRasterColumn','AddRasterColumn', '<xsl:value-of select="@PixType" />', clock_timestamp());
BEGIN;
	CREATE TABLE pgis_rgarden (rid serial);
	SELECT AddRasterColumn('public', 'rast_<xsl:value-of select="@PixType" />', 'rast',4326, '{8BUI,8BUI,8BUI,8BUI}',false, true, '{<xsl:value-of select="@nodata" />}', 0.25,-0.25,200,300, null);

	UPDATE <xsl:value-of select="$var_logtable" /> SET log_end = clock_timestamp() 
		WHERE log_label = '<xsl:value-of select="$log_label" /> AddRasterColumn' AND log_end IS NULL;
COMMIT;

	<xsl:text>

	</xsl:text>
		</xsl:for-each>
<!--End Test table creation, insert, drop  -->
	</xsl:template>

	<!--macro to replace func args with dummy var args -->
	<xsl:template name="replaceparams">
		<xsl:param name="func" />
		<xsl:for-each select="$func">
			<xsl:for-each select="paramdef">
				<xsl:choose>
					<xsl:when test="contains(parameter, 'matrix') or contains(parameter, 'Matrix')">
						<xsl:value-of select="$var_matrix" />
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
					<xsl:when test="(contains(parameter, 'version'))">
						<xsl:value-of select="$var_version2" />
					</xsl:when>
					<xsl:when test="(contains(parameter,'geomgml'))">
						<xsl:text>ST_AsGML(foo1.the_geom)</xsl:text>
					</xsl:when>
					<xsl:when test="(contains(parameter,'geomkml'))">
						<xsl:text>ST_AsKML(foo1.the_geom)</xsl:text>
					</xsl:when>
					<xsl:when test="(contains(type,'box') or type = 'geometry' or type = 'geometry ' or contains(type,'geometry set')) and (position() = 1 or count($func/paramdef/type[contains(text(),'geometry') or contains(text(),'box') or contains(text(), 'WKT') or contains(text(), 'bytea')]) = '1')">
						<xsl:text>foo1.the_geom</xsl:text>
					</xsl:when>
					<xsl:when test="(type = 'geography' or type = 'geography ' or contains(type,'geography set')) and (position() = 1 or count($func/paramdef/type[contains(text(),'geography')]) = '1' )">
						<xsl:text>geography(foo1.the_geom)</xsl:text>
					</xsl:when>
					<xsl:when test="contains(type,'box') or type = 'geometry' or type = 'geometry '">
						<xsl:text>foo2.the_geom</xsl:text>
					</xsl:when>
					<xsl:when test="type = 'geography' or type = 'geography '">
						<xsl:text>geography(foo2.the_geom)</xsl:text>
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
</xsl:stylesheet>
