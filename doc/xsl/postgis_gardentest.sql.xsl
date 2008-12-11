<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!-- ********************************************************************
 * $Id$
 ********************************************************************
	 Copyright 2008, Regina Obe
     License: BSD
	 Purpose: This is an xsl transform that generates an sql test script from xml docs to test all the functions we have documented
	 		using a garden variety of geometries.  Its intent is to flag major crashes.
     ******************************************************************** -->
	<xsl:output method="text" />
	<xsl:template match='/chapter'>
<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
		<xsl:for-each select='sect1/refentry'>
		<xsl:sort select="@id"/>
<!-- For each function prototype generate a test sql statement
	Test functions that take no arguments  -->
			<xsl:for-each select="refsynopsisdiv/funcsynopsis/funcprototype">
<xsl:if test="count(paramdef/parameter) = 0">SELECT  <xsl:value-of select="funcdef/function" />();
</xsl:if>
<!--Start Test aggregate and unary functions 
 TODO: Make this section less verbose -->
<!--Point Aggregate/geom accessor test -->
<xsl:if test="contains(paramdef/type,'geometry set') or (count(paramdef/parameter) = 1 and contains(paramdef/type, 'geometry'))">
<!-- If output is geometry show ewkt rep -->
	<xsl:choose>
	  <xsl:when test="contains(funcdef, 'geometry ')">
		SELECT ST_AsEWKT(<xsl:value-of select="funcdef/function" />(the_geom)),
	ST_AsEWKT(<xsl:value-of select="funcdef/function" />(ST_Multi(the_geom)))
	  </xsl:when>
	  <xsl:otherwise>
		SELECT <xsl:value-of select="funcdef/function" />(the_geom),
		<xsl:value-of select="funcdef/function" />(ST_Multi(the_geom))
	  </xsl:otherwise>
	</xsl:choose>
	FROM (SELECT ST_SetSRID(ST_Point(i,j),4326) As the_geom 
		FROM generate_series(-60,50,5) As i 
			CROSS JOIN generate_series(40,70, 5) j) As foo;  
</xsl:if>
<!--Multi/Line Aggregate/accessor test -->
<xsl:if test="contains(paramdef/type,'geometry set') or (count(paramdef/parameter) = 1 and contains(paramdef/type, 'geometry'))">
	<xsl:choose>
	  <xsl:when test="contains(funcdef, 'geometry ')">
SELECT ST_AsEWKT(<xsl:value-of select="funcdef/function" />(the_geom)),
	ST_AsEWKT(<xsl:value-of select="funcdef/function" />(ST_Multi(the_geom)))
	  </xsl:when>
	  <xsl:otherwise>
		SELECT <xsl:value-of select="funcdef/function" />(the_geom),
		<xsl:value-of select="funcdef/function" />(ST_Multi(the_geom))
	  </xsl:otherwise>
	</xsl:choose>
	FROM (SELECT ST_MakeLine(ST_SetSRID(ST_Point(i,j),4326),ST_SetSRID(ST_Point(j,i),4326))  As the_geom 
		FROM generate_series(-60,50,5) As i 
			CROSS JOIN generate_series(40,70, 5) j) As foo;  
</xsl:if>
<!--Multi/Polygon Aggregate/accessor test -->
<xsl:if test="contains(paramdef/type,'geometry set') or (count(paramdef/parameter) = 1 and contains(paramdef/type, 'geometry'))">
<!-- If output is geometry show ewkt rep -->
	<xsl:choose>
	  <xsl:when test="contains(funcdef, 'geometry ')">
		SELECT ST_AsEWKT(<xsl:value-of select="funcdef/function" />(the_geom)),
	ST_AsEWKT(<xsl:value-of select="funcdef/function" />(ST_Multi(the_geom)))
	  </xsl:when>
	  <xsl:otherwise>
		SELECT <xsl:value-of select="funcdef/function" />(the_geom),
		<xsl:value-of select="funcdef/function" />(ST_Multi(the_geom))
	  </xsl:otherwise>
	</xsl:choose>
	FROM (SELECT ST_Buffer(ST_SetSRID(ST_Point(i,j),4326), j)  As the_geom 
		FROM generate_series(-60,50,5) As i 
			CROSS JOIN generate_series(40,70, 5) j) As foo;  
</xsl:if>
<!--End Test aggregate and unary functions -->
			</xsl:for-each>
		</xsl:for-each>
	</xsl:template>
</xsl:stylesheet>
