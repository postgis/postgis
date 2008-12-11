<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!-- ********************************************************************
 * $Id: 0.1 postgis_gardentest.sql.xsl 3377 2008-12-11 15:56:18Z robe $
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
<!-- For each function prototype generate a test sql statement
	Test functions that take no arguments  -->
			<xsl:for-each select="refsynopsisdiv/funcsynopsis/funcprototype">
<xsl:if test="count(paramdef/parameter) = 0">SELECT  <xsl:value-of select="funcdef/function" />();
</xsl:if>
			</xsl:for-each>
		</xsl:for-each>
	</xsl:template>
</xsl:stylesheet>
