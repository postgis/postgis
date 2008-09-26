<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!-- ********************************************************************
     $Id: 0.1 postgis_mm.xml.xsl 2008-09-26 $
     ********************************************************************
	 Copyright 2008, Regina Obe
     License: BSD
	 Purpose: This is an xsl transform that generates index listing of aggregate functions and mm /sql compliant functions xml section from reference_new.xml to then
	 be processed by doc book
     ******************************************************************** -->
	<xsl:output method="xml" indent="yes" />
	<xsl:template match='/chapter'>
	
	  <title>PostGIS Aggregates Index</title>
	  <para>The functions given below are spatial aggregate functions provided with PostGIS that can
	  be used just like any other sql aggregate function such as sum, average.</para>
	  <chapter>
	  <sect1><title>PostGIS Aggregate Functions</title>
	  	
		<xsl:variable name="ap"><xsl:text>'</xsl:text></xsl:variable>
<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
		<xsl:for-each select='sect1/refentry'>
		  <xsl:variable name='comment'>
		  	<xsl:value-of select="normalize-space(translate(translate(refnamediv/refpurpose,'&#x0d;&#x0a;', ' '), '&#09;', ' '))"/>
		  </xsl:variable>

<!-- For each function prototype generate the DDL comment statement
	If its input is a geometry set - we know it is an aggregate function rather than a regular function -->
			<xsl:for-each select="refsynopsisdiv/funcsynopsis/funcprototype">
				<xsl:choose>
					<xsl:when test="contains(paramdef/type,'geometry set')">
						<p><xref linkend="{funcdef/function}" />(geometry <xsl:call-template name="listparams"><xsl:with-param name="func" select="." /></xsl:call-template>) - <xsl:value-of select='$comment' /></p>
					</xsl:when>
				</xsl:choose>
			</xsl:for-each>
		</xsl:for-each>
		</sect1>
		
		<sect1><title>PostGIS MM Compliant Functions</title>
		<para>The functions given below are PostGIS functions that conform to the SQL/MM 3 standard</para>
<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
		<xsl:for-each select='sect1/refentry'>
		  <xsl:variable name='comment'>
		  	<xsl:value-of select="normalize-space(translate(translate(refnamediv/refpurpose,'&#x0d;&#x0a;', ' '), '&#09;', ' '))"/>
		  </xsl:variable>
		  <xsl:variable name='refid'>
		  	<xsl:value-of select='@id' />
		  </xsl:variable>

<!-- For each function prototype generate the DDL comment statement
	If its input is a geometry set - we know it is an aggregate function rather than a regular function -->
			<xsl:for-each select="refsection">
				<xsl:for-each select="para">
					<xsl:choose>
						<xsl:when test="contains(.,'implements the SQL/MM')">
							<p><xref linkend="{$refid}" /> - <xsl:value-of select='$comment' /></p>
						</xsl:when>
					</xsl:choose>
				</xsl:for-each>
			</xsl:for-each>
		</xsl:for-each>
		</sect1>
		</chapter>
	</xsl:template>

	<!--macro to pull out function parameter names so we can provide a pretty arg list prefix for each function -->
	<xsl:template name="listparams">
		<xsl:param name="func" />
		<xsl:for-each select="$func">
			<xsl:if test="count(paramdef/parameter) &gt; 0">args: </xsl:if>
			<xsl:for-each select="paramdef">
				<xsl:choose>
					<xsl:when test="count(parameter) &gt; 0"> 
						<xsl:value-of select="parameter" />
					</xsl:when>
				</xsl:choose>
				<xsl:if test="position()&lt;last()"><xsl:text>, </xsl:text></xsl:if>
			</xsl:for-each>
			<xsl:if test="count(paramdef/parameter) &gt; 0"> - </xsl:if>
		</xsl:for-each>	
	</xsl:template>

</xsl:stylesheet>
