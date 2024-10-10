<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:db="http://docbook.org/ns/docbook"
>
<!-- ********************************************************************
     ********************************************************************
	 Copyright 2008, Regina Obe
   License: BSD-3-Clause
	 Purpose: This is an xsl transform that generates PostgreSQL COMMENT ON FUNCTION ddl
	 statements from postgis xml doc reference
     ******************************************************************** -->

	<xsl:include href="common_utils.xsl" />
	<xsl:include href="common_comments.xsl" />

	<xsl:output method="text" />

	<!-- We deal only with the reference chapter -->
        <xsl:template match="/">
                <xsl:apply-templates select="/db:book/db:chapter[@xml:id='Extras']" />
        </xsl:template>

        <xsl:template match="db:chapter">
		<xsl:variable name="ap"><xsl:text>'</xsl:text></xsl:variable>
<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
		<xsl:for-each select="db:section[@xml:id='Tiger_Geocoder']/db:refentry">
		  <xsl:variable name='plaincomment'>
		  	<xsl:value-of select="normalize-space(translate(translate(db:refnamediv/db:refpurpose,'&#x0d;&#x0a;', ' '), '&#09;', ' '))"/>
		  </xsl:variable>
<!-- Replace apostrophes with 2 apostrophes needed for escaping in SQL -->
		<xsl:variable name='comment'>
			<xsl:call-template name="globalReplace">
				<xsl:with-param name="outputString" select="$plaincomment"/>
				<xsl:with-param name="target" select="$ap"/>
				<xsl:with-param name="replacement" select="''"/>
			</xsl:call-template>
		</xsl:variable>
<!-- For each function prototype generate the DDL comment statement
	If its input is a geometry set - we know it is an aggregate function rather than a regular function
	Do not output OUT params since they define output rather than act as input and do not put a comma after argument just before an OUT parameter -->
		<xsl:for-each select="db:refsynopsisdiv/db:funcsynopsis/db:funcprototype">
COMMENT ON <xsl:choose><xsl:when test="contains(db:paramdef/db:type,'geometry set')">AGGREGATE</xsl:when><xsl:otherwise>FUNCTION</xsl:otherwise></xsl:choose><xsl:text> </xsl:text> <xsl:value-of select="db:funcdef/db:function" />(<xsl:for-each select="db:paramdef"><xsl:choose><xsl:when test="count(db:parameter) &gt; 0">
<xsl:choose><xsl:when test="contains(db:parameter,'OUT')"></xsl:when><xsl:when test="contains(db:type,'geometry set')">geometry</xsl:when><xsl:otherwise><xsl:value-of select="db:type" /></xsl:otherwise></xsl:choose><xsl:if test="position()&lt;last() and not(contains(db:parameter,'OUT')) and not(contains(following-sibling::db:paramdef[1],'OUT'))"><xsl:text>, </xsl:text></xsl:if></xsl:when>
</xsl:choose></xsl:for-each>) IS '<xsl:call-template name="listparams"><xsl:with-param name="func" select="." /></xsl:call-template> <xsl:value-of select='$comment' />';
			</xsl:for-each>
		</xsl:for-each>
	</xsl:template>

</xsl:stylesheet>
