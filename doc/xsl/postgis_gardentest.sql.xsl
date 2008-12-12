<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"  xmlns:pgis="pgis schema local to this file">
<!-- ********************************************************************
 * $Id$
 ********************************************************************
	 Copyright 2008, Regina Obe
     License: BSD
	 Purpose: This is an xsl transform that generates an sql test script from xml docs to test all the functions we have documented
	 		using a garden variety of geometries.  Its intent is to flag major crashes.
     ******************************************************************** -->
	<xsl:output method="text" />
	<pgis:gardens>
		<pgis:gset ID='PointSet' GeometryType='POINT'>(SELECT ST_SetSRID(ST_Point(i,j),4326) As the_geom 
		FROM generate_series(-10,50,10) As i 
			CROSS JOIN generate_series(40,70, 10) j)</pgis:gset>
		<pgis:gset ID='LineSet' GeometryType='LINESTRING'>(SELECT ST_MakeLine(ST_SetSRID(ST_Point(i,j),4326),ST_SetSRID(ST_Point(j,i),4326))  As the_geom 
		FROM generate_series(-10,50,10) As i 
			CROSS JOIN generate_series(40,70, 10) As j
			)</pgis:gset>
		<pgis:gset ID='PolySet' GeometryType='POLYGON'>(SELECT ST_Buffer(ST_SetSRID(ST_Point(i,j),4326), j)  As the_geom 
		FROM generate_series(-10,50,10) As i 
			CROSS JOIN generate_series(40,70, 10) As j)</pgis:gset>
		<pgis:gset ID='PointMSet' GeometryType='POINTM'>(SELECT ST_SetSRID(ST_MakePointM(i,j,m),4326) As the_geom 
		FROM generate_series(-10,50,10) As i 
			CROSS JOIN generate_series(50,70, 20) AS j
			CROSS JOIN generate_series(1,2) As m)</pgis:gset>
		<pgis:gset ID='LineMSet' GeometryType='LINESTRINGM'>(SELECT ST_MakeLine(ST_SetSRID(ST_MakePointM(i,j,m),4326),ST_SetSRID(ST_MakePointM(j,i,m),4326))  As the_geom 
		FROM generate_series(-10,50,10) As i 
			CROSS JOIN generate_series(50,70, 20) As j
			CROSS JOIN generate_series(1,2) As m
			)</pgis:gset>
	</pgis:gardens>

	<xsl:template match='/chapter'>
<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
		<xsl:for-each select='sect1/refentry'>
		<xsl:sort select="@id"/>
<!-- For each function prototype generate a test sql statement
	Test functions that take no arguments  -->
			<xsl:for-each select="refsynopsisdiv/funcsynopsis/funcprototype">
<xsl:if test="count(paramdef/parameter) = 0">SELECT  'Starting <xsl:value-of select="funcdef/function" />()';BEGIN; 
SELECT  <xsl:value-of select="funcdef/function" />();
COMMIT;
SELECT  'Ending <xsl:value-of select="funcdef/function" />()';
</xsl:if>
<!--Start Test aggregate and unary functions 
 DONE: Make this section less verbose -->
<!--Garden Aggregator/Unary function with input gsets test -->
<xsl:if test="contains(paramdef/type,'geometry set') or (count(paramdef/parameter) = 1 and contains(paramdef/type, 'geometry'))">
	<xsl:variable name='fnname'><xsl:value-of select="funcdef/function"/></xsl:variable>
	<xsl:variable name='fndef'><xsl:value-of select="funcdef"/></xsl:variable>
	<xsl:for-each select="document('')//pgis:gardens/pgis:gset">
SELECT '<xsl:value-of select="$fnname" /><xsl:text> </xsl:text><xsl:value-of select="@ID" />: Start Testing Multi/<xsl:value-of select="@GeometryType" />'; 
BEGIN; <!-- If output is geometry show ewkt rep -->
		<xsl:choose>
		  <xsl:when test="contains($fndef, 'geometry ')">
SELECT ST_AsEWKT(<xsl:value-of select="$fnname" />(the_geom)),
	ST_AsEWKT(<xsl:value-of select="$fnname" />(ST_Multi(the_geom)))
		  </xsl:when>
		  <xsl:otherwise>
SELECT <xsl:value-of select="$fnname" />(the_geom),
			<xsl:value-of select="$fnname" />(ST_Multi(the_geom))
		  </xsl:otherwise>
		</xsl:choose>
		FROM (<xsl:value-of select="." />) As foo;  
COMMIT;
SELECT '<xsl:value-of select="$fnname" /><xsl:text> </xsl:text> <xsl:value-of select="@ID" />: End Testing Multi/<xsl:value-of select="@GeometryType" />';
	<xsl:text>
	
	</xsl:text>
	</xsl:for-each>
</xsl:if>

<!--Garden Relationship/geom2 output function tests  -->
<xsl:if test="(count(paramdef/parameter) = 2 and contains(paramdef[1]/type, 'geometry ')  and contains(paramdef[2]/type, 'geometry '))">
	<xsl:variable name='fnname'><xsl:value-of select="funcdef/function"/></xsl:variable>
	<xsl:variable name='fndef'><xsl:value-of select="funcdef"/></xsl:variable>
	<xsl:for-each select="document('')//pgis:gardens/pgis:gset">
SELECT '<xsl:value-of select="$fnname" /><xsl:text> </xsl:text><xsl:value-of select="@ID" />: Start Testing Multi/<xsl:value-of select="@GeometryType" />'; 
BEGIN; <!-- If output is geometry show ewkt rep -->
		<xsl:choose>
		  <xsl:when test="contains($fndef, 'geometry ')">
SELECT ST_AsEWKT(<xsl:value-of select="$fnname" />(foo1.the_geom, foo2.the_geom)),
	ST_AsEWKT(<xsl:value-of select="$fnname" />(ST_Multi(foo1.the_geom), ST_Multi(foo2.the_geom)))
		  </xsl:when>
		  <xsl:otherwise>
SELECT <xsl:value-of select="$fnname" />(foo1.the_geom, foo2.the_geom),
			<xsl:value-of select="$fnname" />(ST_Multi(foo1.the_geom), ST_Multi(foo2.the_geom))
		  </xsl:otherwise>
		</xsl:choose>
		FROM (<xsl:value-of select="." />) As foo1 CROSS JOIN (<xsl:value-of select="." />) As foo2
		LIMIT 5;  
COMMIT;
SELECT '<xsl:value-of select="$fnname" /><xsl:text> </xsl:text> <xsl:value-of select="@ID" />: End Testing Multi/<xsl:value-of select="@GeometryType" />';
	<xsl:text>
	
	</xsl:text>
	</xsl:for-each>
</xsl:if>
			</xsl:for-each>
		</xsl:for-each>
	</xsl:template>
</xsl:stylesheet>
