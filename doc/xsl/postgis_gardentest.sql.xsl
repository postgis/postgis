<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"  xmlns:pgis="http://postgis.refractions.net/pgis">
<!-- ********************************************************************
 * $Id$
 ********************************************************************
	 Copyright 2008, Regina Obe
	 License: BSD
	 Purpose: This is an xsl transform that generates an sql test script from xml docs to test all the functions we have documented
			using a garden variety of geometries.  Its intent is to flag major crashes.
	 ******************************************************************** -->
	<xsl:output method="text" />
	<!--Exclude this from testing - it crashes or already tested in special section -->
	<xsl:variable name='fnexclude'>AddGeometryColumn DropGeometryColumn DropGeometryTable</xsl:variable>
	<!--This is just a place holder to state functions not supported in 1.3 branch -->
	<xsl:variable name='fnexclude13'>Populate_Geometry_Columns ST_IsValidReason ST_ContainsProperly</xsl:variable>
	<xsl:variable name='var_srid'>3395</xsl:variable>
	<xsl:variable name='var_position'>1</xsl:variable>
	<xsl:variable name='var_integer1'>3</xsl:variable>
	<xsl:variable name='var_integer2'>5</xsl:variable>
	<xsl:variable name='var_float1'>0.5</xsl:variable>
	<xsl:variable name='var_float2'>0.75</xsl:variable>
	<xsl:variable name='var_version1'>1</xsl:variable>
	<xsl:variable name='var_version2'>2</xsl:variable>
	<xsl:variable name='var_NDRXDR'>XDR</xsl:variable>
	<xsl:variable name='var_text'>'monkey'</xsl:variable>
	<xsl:variable name='var_varchar'>'test'</xsl:variable>
	<xsl:variable name='var_spheroid'>'SPHEROID["GRS_1980",6378137,298.257222101]'</xsl:variable>
	<xsl:variable name='var_matrix'>'FF1FF0102'</xsl:variable>
	<pgis:gardens>
		<pgis:gset ID='PointSet' GeometryType='POINT'>(SELECT ST_SetSRID(ST_Point(i,j),4326) As the_geom
		FROM generate_series(-10,50,15) As i
			CROSS JOIN generate_series(40,70, 15) j)</pgis:gset>
		<pgis:gset ID='LineSet' GeometryType='LINESTRING'>(SELECT ST_MakeLine(ST_SetSRID(ST_Point(i,j),4326),ST_SetSRID(ST_Point(j,i),4326))  As the_geom
		FROM generate_series(-10,50,10) As i
			CROSS JOIN generate_series(40,70, 15) As j
			WHERE NOT(i = j))</pgis:gset>
		<pgis:gset ID='PolySet' GeometryType='POLYGON'>(SELECT ST_Buffer(ST_SetSRID(ST_Point(i,j),4326), j)  As the_geom
		FROM generate_series(-10,50,10) As i
			CROSS JOIN generate_series(40,70, 20) As j)</pgis:gset>
		<pgis:gset ID='PointMSet' GeometryType='POINTM'>(SELECT ST_SetSRID(ST_MakePointM(i,j,m),4326) As the_geom
		FROM generate_series(-10,50,10) As i
			CROSS JOIN generate_series(50,70, 20) AS j
			CROSS JOIN generate_series(1,2) As m)</pgis:gset>
		<pgis:gset ID='LineMSet' GeometryType='LINESTRINGM'>(SELECT ST_MakeLine(ST_SetSRID(ST_MakePointM(i,j,m),4326),ST_SetSRID(ST_MakePointM(j,i,m),4326))  As the_geom
		FROM generate_series(-10,50,10) As i
			CROSS JOIN generate_series(50,70, 20) As j
			CROSS JOIN generate_series(1,2) As m
			WHERE NOT(i = j))</pgis:gset>
		<pgis:gset ID='PolygonMSet' GeometryType='POLYGONM'>(SELECT ST_MakePolygon(ST_AddPoint(ST_AddPoint(ST_MakeLine(ST_SetSRID(ST_MakePointM(i+m,j,m),4326),ST_SetSRID(ST_MakePointM(j+m,i-m,m),4326)),ST_SetSRID(ST_MakePointM(i,j,m),4326)),ST_SetSRID(ST_MakePointM(i+m,j,m),4326)))  As the_geom
		FROM generate_series(-10,50,20) As i
			CROSS JOIN generate_series(50,70, 20) As j
			CROSS JOIN generate_series(1,2) As m
			)</pgis:gset>
		<pgis:gset ID='PointSet3D' GeometryType='POINT'>(SELECT ST_SetSRID(ST_MakePoint(i,j,k),4326) As the_geom
		FROM generate_series(-10,50,20) As i
			CROSS JOIN generate_series(40,70, 20) j
			CROSS JOIN generate_series(1,2) k
			)</pgis:gset>
		<pgis:gset ID='LineSet3D' GeometryType='LINESTRING'>(SELECT ST_SetSRID(ST_MakeLine(ST_MakePoint(i,j,k), ST_MakePoint(i+k,j+k,k)),4326) As the_geom
		FROM generate_series(-10,50,20) As i
			CROSS JOIN generate_series(40,70, 20) j
			CROSS JOIN generate_series(1,2) k
			)</pgis:gset>
		<pgis:gset ID='PolygonSet3D' GeometryType='POLYGON'>(SELECT ST_SetSRID(ST_MakePolygon(ST_AddPoint(ST_AddPoint(ST_MakeLine(ST_MakePoint(i+m,j,m),ST_MakePoint(j+m,i-m,m)),ST_MakePoint(i,j,m)),ST_MakePointM(i+m,j,m))),4326)  As the_geom
		FROM generate_series(-10,50,20) As i
			CROSS JOIN generate_series(50,70, 20) As j
			CROSS JOIN generate_series(1,2) As m)</pgis:gset>

		<pgis:gset ID='GCSet3D' GeometryType='GEOMETRYCOLLECTION' SkipUnary='1'>(SELECT ST_Collect(ST_Collect(ST_SetSRID(ST_MakePoint(i,j,m),4326),ST_SetSRID(ST_MakePolygon(ST_AddPoint(ST_AddPoint(ST_MakeLine(ST_MakePoint(i+m,j,m),ST_MakePoint(j+m,i-m,m)),ST_MakePoint(i,j,m)),ST_MakePointM(i+m,j,m))),4326)))  As the_geom
		FROM generate_series(-10,50,20) As i
			CROSS JOIN generate_series(50,70, 20) As j
			CROSS JOIN generate_series(1,2) As m
			GROUP BY m)</pgis:gset>

		<pgis:gset ID='CurvePolySet' GeometryType='CURVEPOLYGON'>(SELECT ST_LineToCurve(ST_Buffer(ST_SetSRID(ST_Point(i,j),4326), j))  As the_geom
		FROM generate_series(-10,50,10) As i
			CROSS JOIN generate_series(40,70, 20) As j)</pgis:gset>
		<pgis:gset ID='CircularStringSet' GeometryType='CIRCULARSTRING'>(SELECT ST_LineToCurve(ST_Boundary(ST_Buffer(ST_SetSRID(ST_Point(i,j),4326), j)))  As the_geom
		FROM generate_series(-10,50,10) As i
			CROSS JOIN generate_series(40,70, 20) As j)</pgis:gset>

<!-- MULTIs start here -->
		<pgis:gset ID='MultiPointSet' GeometryType='MULTIPOINT'>(SELECT ST_Collect(s.the_geom) As the_geom
		FROM (SELECT ST_SetSRID(ST_Point(i,j),4326) As the_geom
		FROM generate_series(-10,50,15) As i
			CROSS JOIN generate_series(40,70, 15) j) As s)</pgis:gset>

		<pgis:gset ID='MultiLineSet' GeometryType='MULTILINESTRING'>(SELECT ST_Collect(s.the_geom) As the_geom
		FROM (SELECT ST_MakeLine(ST_SetSRID(ST_Point(i,j),4326),ST_SetSRID(ST_Point(j,i),4326))  As the_geom
		FROM generate_series(-10,50,10) As i
			CROSS JOIN generate_series(40,70, 15) As j
			WHERE NOT(i = j)) As s)</pgis:gset>

		<pgis:gset ID='MultiPolySet' GeometryType='POLYGON'>(SELECT ST_Multi(ST_Union(ST_Buffer(ST_SetSRID(ST_Point(i,j),4326), j)))  As the_geom
		FROM generate_series(-10,50,10) As i
			CROSS JOIN generate_series(40,70, 20) As j)</pgis:gset>

		<pgis:gset ID='MultiPointSet3D' GeometryType='MULTIPOINT'>(SELECT ST_Collect(ST_SetSRID(ST_MakePoint(i,j,k),4326)) As the_geom
		FROM generate_series(-10,50,20) As i
			CROSS JOIN generate_series(40,70, 20) j
			CROSS JOIN generate_series(1,3) k
			)</pgis:gset>

		<pgis:gset ID='MultiLineSet3D' GeometryType='MULTILINESTRING'>(SELECT ST_Multi(ST_Union(ST_SetSRID(ST_MakeLine(ST_MakePoint(i,j,k), ST_MakePoint(i+k,j+k,k)),4326))) As the_geom
		FROM generate_series(-10,50,20) As i
			CROSS JOIN generate_series(40,70, 20) j
			CROSS JOIN generate_series(1,2) k
			)</pgis:gset>

		<pgis:gset ID='MultiPolySet3D' GeometryType='MULTIPOLYGON'>(SELECT ST_Multi(ST_Union(s.the_geom)) As the_geom
		FROM (SELECT ST_MakePolygon(ST_AddPoint(ST_AddPoint(ST_MakeLine(ST_SetSRID(ST_MakePointM(i+m,j,m),4326),ST_SetSRID(ST_MakePointM(j+m,i-m,m),4326)),ST_SetSRID(ST_MakePointM(i,j,m),4326)),ST_SetSRID(ST_MakePointM(i+m,j,m),4326)))  As the_geom
		FROM generate_series(-10,50,20) As i
			CROSS JOIN generate_series(50,70, 20) As j
			CROSS JOIN generate_series(1,2) As m
			) As s)</pgis:gset>

		<pgis:gset ID='MultiPointMSet' GeometryType='MULTIPOINTM'>(SELECT ST_Collect(s.the_geom) As the_geom
		FROM (SELECT ST_SetSRID(ST_MakePointM(i,j,m),4326) As the_geom
		FROM generate_series(-10,50,10) As i
			CROSS JOIN generate_series(50,70, 20) AS j
			CROSS JOIN generate_series(1,2) As m) As s)</pgis:gset>

		<pgis:gset ID='MultiLineMSet' GeometryType='MULTILINESTRINGM'>(SELECT ST_Collect(s.the_geom) As the_geom
		FROM (SELECT ST_MakeLine(ST_SetSRID(ST_MakePointM(i,j,m),4326),ST_SetSRID(ST_MakePointM(j,i,m),4326))  As the_geom
		FROM generate_series(-10,50,10) As i
			CROSS JOIN generate_series(50,70, 20) As j
			CROSS JOIN generate_series(1,2) As m
			WHERE NOT(i = j)) As s)</pgis:gset>

		<pgis:gset ID='MultiPolygonMSet' GeometryType='MULTIPOLYGONM'>(SELECT ST_Multi(ST_Union(ST_MakePolygon(ST_AddPoint(ST_AddPoint(ST_MakeLine(ST_SetSRID(ST_MakePointM(i+m,j,m),4326),ST_SetSRID(ST_MakePointM(j+m,i-m,m),4326)),ST_SetSRID(ST_MakePointM(i,j,m),4326)),ST_SetSRID(ST_MakePointM(i+m,j,m),4326)))))  As the_geom
		FROM generate_series(-10,50,20) As i
			CROSS JOIN generate_series(50,70, 20) As j
			CROSS JOIN generate_series(1,2) As m
			)</pgis:gset>
	<!-- TODO: Finish off MULTI list -->
	</pgis:gardens>
	<!--This is just a placeholder to hold geometries that will crash server when hitting against some functions
		We'll fix these crashers in 1.4 -->
	<pgis:gardencrashers>

	</pgis:gardencrashers>

	<xsl:template match='/chapter'>
<!--Start Test table creation, insert, drop -->
		<xsl:for-each select="document('')//pgis:gardens/pgis:gset">
SELECT 'create,insert,drop Test: Start Testing Multi/<xsl:value-of select="@GeometryType" />';
BEGIN;
	CREATE TABLE pgis_garden (gid serial);
	SELECT AddGeometryColumn('pgis_garden','the_geom',ST_SRID(the_geom),GeometryType(the_geom),ST_CoordDim(the_geom))
			FROM (<xsl:value-of select="." />) As foo limit 1;
	SELECT AddGeometryColumn('pgis_garden','the_geom_multi',ST_SRID(the_geom),GeometryType(ST_Multi(the_geom)),ST_CoordDim(the_geom))
			FROM (<xsl:value-of select="." />) As foo limit 1;
	INSERT INTO pgis_garden(the_geom, the_geom_multi)
	SELECT the_geom, ST_Multi(the_geom)
	FROM (<xsl:value-of select="." />) As foo;

	SELECT UpdateGeometrySRID('pgis_garden', 'the_geom', 4269);

	SELECT DropGeometryColumn ('pgis_garden','the_geom');
	SELECT DropGeometryTable ('pgis_garden');
COMMIT;
SELECT 'create,insert,drop Test: Start Testing Multi/<xsl:value-of select="@GeometryType" />';
	<xsl:text>

	</xsl:text>
		</xsl:for-each>
<!--End Test table creation, insert, drop  -->

<!--Start test on operators  -->
	<xsl:for-each select="sect1[contains(@id,'Operator')]/refentry">
		<xsl:sort select="@id"/>
		<xsl:for-each select="refsynopsisdiv/funcsynopsis/funcprototype">
			<xsl:variable name='fnname'><xsl:value-of select="funcdef/function"/></xsl:variable>
			<xsl:for-each select="document('')//pgis:gardens/pgis:gset">
			<!--Store first garden sql geometry from -->
					<xsl:variable name="from1"><xsl:value-of select="." /></xsl:variable>
					<xsl:variable name='geom1type'><xsl:value-of select="@ID"/></xsl:variable>
		SELECT '<xsl:value-of select="$fnname" /><xsl:text> </xsl:text><xsl:value-of select="@ID" /> : Start Testing <xsl:value-of select="$geom1type" /> against other types';
						<xsl:for-each select="document('')//pgis:gardens/pgis:gset">
			SELECT '<xsl:value-of select="$fnname" /><xsl:text> </xsl:text><xsl:value-of select="@ID" />: Start Testing <xsl:value-of select="$geom1type" />, <xsl:value-of select="@GeometryType" />';
			BEGIN;
			SELECT foo1.the_geom <xsl:value-of select="$fnname" /> foo2.the_geom
					FROM (<xsl:value-of select="$from1" />) As foo1 CROSS JOIN (<xsl:value-of select="." />) As foo2
					LIMIT 3;
			COMMIT;
					</xsl:for-each>
		SELECT '<xsl:value-of select="$fnname" /><xsl:text> </xsl:text><xsl:value-of select="@ID" />: End Testing <xsl:value-of select="@GeometryType" /> against other types';
				</xsl:for-each>
			</xsl:for-each>
	</xsl:for-each>
<!--End test on operators -->
<!-- Start regular function checks excluding operators -->
		<xsl:for-each select="sect1[not(contains(@id,'Operator'))]/refentry">
		<xsl:sort select="@id"/>

			<xsl:for-each select="refsynopsisdiv/funcsynopsis/funcprototype">
<!--Create dummy paramaters to be used later -->
				<xsl:variable name='fnfakeparams'><xsl:call-template name="replaceparams"><xsl:with-param name="func" select="." /></xsl:call-template></xsl:variable>
				<xsl:variable name='fnargs'><xsl:call-template name="listparams"><xsl:with-param name="func" select="." /></xsl:call-template></xsl:variable>
				<xsl:variable name='fnname'><xsl:value-of select="funcdef/function"/></xsl:variable>
				<xsl:variable name='fndef'><xsl:value-of select="funcdef"/></xsl:variable>
				<xsl:variable name='numparams'><xsl:value-of select="count(paramdef/parameter)" /></xsl:variable>
				<xsl:variable name='numparamgeoms'><xsl:value-of select="count(paramdef/type[contains(text(),'geometry') or contains(text(),'box') or contains(text(), 'bytea')]) + count(paramdef/parameter[contains(text(),'WKT')])" /></xsl:variable>
				<!-- For each function prototype generate a test sql statement -->
				<xsl:choose>
<!--Test functions that take no arguments or take no geometries -->
	<xsl:when test="$numparamgeoms = '0' and not(contains($fnexclude,funcdef/function))">SELECT  'Starting <xsl:value-of select="funcdef/function" />(<xsl:value-of select="$fnargs" />)';BEGIN;
SELECT  <xsl:value-of select="funcdef/function" />(<xsl:value-of select="$fnfakeparams" />);
COMMIT;
SELECT  'Ending <xsl:value-of select="funcdef/function" />(<xsl:value-of select="$fnargs" />)';
	</xsl:when>
<!--Start Test aggregate and unary functions -->
<!-- put functions that take only one geometry no need to cross with another geom collection, these are unary geom, aggregates, and so forth -->
	<xsl:when test="$numparamgeoms = '1' and not(contains($fnexclude,funcdef/function))" >
		<xsl:for-each select="document('')//pgis:gardens/pgis:gset">
	SELECT '<xsl:value-of select="$fnname" /><xsl:text> </xsl:text><xsl:value-of select="@ID" />: Start Testing <xsl:value-of select="@GeometryType" />';
	BEGIN; <!-- If output is geometry show ewkt rep -->
			<xsl:choose>
			  <xsl:when test="contains($fndef, 'geometry ')">
	SELECT ST_AsEWKT(<xsl:value-of select="$fnname" />(<xsl:value-of select="$fnfakeparams" />))
			  </xsl:when>
			  <xsl:otherwise>
	SELECT <xsl:value-of select="$fnname" />(<xsl:value-of select="$fnfakeparams" />)
			  </xsl:otherwise>
			</xsl:choose>
			FROM (<xsl:value-of select="." />) As foo1;
	COMMIT;
	SELECT '<xsl:value-of select="$fnname" /><xsl:text> </xsl:text> <xsl:value-of select="@ID" />: End Testing <xsl:value-of select="@GeometryType" />';
		<xsl:text>

		</xsl:text>
		</xsl:for-each>
	</xsl:when>

<!--Functions more than 1 args not already covered this will cross every geometry type with every other -->
	<xsl:when test="not(contains($fnexclude,funcdef/function))">
		<xsl:for-each select="document('')//pgis:gardens/pgis:gset">
	<!--Store first garden sql geometry from -->
			<xsl:variable name="from1"><xsl:value-of select="." /></xsl:variable>
			<xsl:variable name='geom1type'><xsl:value-of select="@ID"/></xsl:variable>
SELECT '<xsl:value-of select="$fnname" /><xsl:text> </xsl:text><xsl:value-of select="@ID" />(<xsl:value-of select="$fnargs" />): Start Testing <xsl:value-of select="$geom1type" /> against other types';
				<xsl:for-each select="document('')//pgis:gardens/pgis:gset">
	SELECT '<xsl:value-of select="$fnname" /><xsl:text> </xsl:text><xsl:value-of select="@ID" />(<xsl:value-of select="$fnargs" />): Start Testing <xsl:value-of select="$geom1type" />, <xsl:value-of select="@GeometryType" />';
	BEGIN; <!-- If output is geometry show ewkt rep -->
			<xsl:choose>
			  <xsl:when test="contains($fndef, 'geometry ')">
	SELECT ST_AsEWKT(<xsl:value-of select="$fnname" />(<xsl:value-of select="$fnfakeparams" />))
			  </xsl:when>
			  <xsl:otherwise>
	SELECT <xsl:value-of select="$fnname" />(<xsl:value-of select="$fnfakeparams" />)
			  </xsl:otherwise>
			</xsl:choose>
			FROM (<xsl:value-of select="$from1" />) As foo1 CROSS JOIN (<xsl:value-of select="." />) As foo2
			LIMIT 3;
	COMMIT;
	SELECT '<xsl:value-of select="$fnname" />(<xsl:value-of select="$fnargs" />) <xsl:text> </xsl:text> <xsl:value-of select="@ID" />: End Testing <xsl:value-of select="$geom1type" />, <xsl:value-of select="@GeometryType" />';
		<xsl:text>

		</xsl:text>
			</xsl:for-each>
SELECT '<xsl:value-of select="$fnname" /><xsl:text> </xsl:text><xsl:value-of select="@ID" />(<xsl:value-of select="$fnargs" />): End Testing <xsl:value-of select="@GeometryType" /> against other types';
		</xsl:for-each>
		</xsl:when>
	</xsl:choose>
			</xsl:for-each>
		</xsl:for-each>
	</xsl:template>

	<!--macro to replace func args with dummy var args -->
	<!--macro to replace func args with dummy var args -->
	<xsl:template name="replaceparams">
		<xsl:param name="func" />
		<xsl:for-each select="$func">
			<xsl:for-each select="paramdef">
				<xsl:choose>
					<xsl:when test="contains(parameter, 'matrix') or contains(parameter, 'Matrix')">
						<xsl:value-of select="$var_matrix" />
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
					<xsl:when test="(contains(type,'box') or type = 'geometry' or type = 'geometry ' or contains(type,'geometry set')) and (position() = 1 or count($func/paramdef/type[contains(text(),'geometry') or contains(text(),'box') or contains(text(), 'WKT') or contains(text(), 'bytea')]) = '1')">
						<xsl:text>foo1.the_geom</xsl:text>
					</xsl:when>
					<xsl:when test="contains(type,'box') or type = 'geometry' or type = 'geometry '">
						<xsl:text>foo2.the_geom</xsl:text>
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
