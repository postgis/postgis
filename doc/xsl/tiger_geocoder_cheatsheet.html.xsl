<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!-- ********************************************************************
     $Id: topology_cheatsheet.html.xsl 6130 2010-10-26 14:47:57Z robe $
     ********************************************************************
	 Copyright 2011, Regina Obe
     License: BSD
	 Purpose: This is an xsl transform that generates PostgreSQL COMMENT ON FUNCTION ddl
	 statements from postgis xml doc reference
     ******************************************************************** -->
	<xsl:output method="text" />
	<xsl:variable name='postgis_version'>2.0</xsl:variable>
	<xsl:variable name='new_tag'>Availability: <xsl:value-of select="$postgis_version" /></xsl:variable>
<xsl:template match="/">
	<xsl:text><![CDATA[<html><head><title>PostGIS ]]></xsl:text> <xsl:value-of select="$postgis_version" /> Tiger Geocoder Cheat Sheet <xsl:text><![CDATA[</title>
	<style type="text/css">
<!--
body {
	font-family: Arial, sans-serif;
	font-size: 8.5pt;
}

.comment {font-size:x-small;color:green;font-family:"courier new"}

.example_heading {
	border-bottom: 1px solid #000;
	margin: 10px 15px 10px 85px;
	background-color: #3e332a;
	color: #fff;
}

#content_functions {
	float: left;
	width:100%;
}

#content_examples {
	float: left;
	width: 100%;
}

.section {
	border: 1px solid #000;
	margin: 4px;
	width: 100%;
}

.section th {
	border: 1px solid #000;
	color: #fff;
	background-color: #3b332a;
	font-size: 9.5pt;
	
}
.section td {
	font-family: Arial, sans-serif;
	font-size: 8.5pt;
	vertical-align: top;
	border: 0;
}

.section td.func {font-weight: 600}

.evenrow {
	background-color: #eee;
}

.oddrow {
	background-color: #fff;
}

h1 {
	margin: 0px;
	padding: 0px;
	font-size: 14pt;
}

-->
</style>
	</head><body><h1 style='text-align:center'>PostGIS ]]></xsl:text> <xsl:value-of select="$postgis_version" /><xsl:text><![CDATA[ Tiger Gecoder Cheatsheet</h1>]]></xsl:text>
		<xsl:text><![CDATA[<div id="content_functions"><span class='comment'>New in this release <sup>1</sup><br />Requires GEOS 3.3 or higher<sup>3.3</sup></span>]]></xsl:text>
			<xsl:apply-templates select="/book/chapter[@id='Extras']" name="function_list" />
			<xsl:text><![CDATA[</div>]]></xsl:text>
			<xsl:text><![CDATA[<div id="content_examples">]]></xsl:text>
			<!-- examples go here -->
			<xsl:apply-templates select="/book/chapter[@id='Extras']/sect1[count(//refentry//refsection//programlisting) &gt; 0]"  />
			<xsl:text><![CDATA[</div>]]></xsl:text>
			<xsl:text><![CDATA[</body></html>]]></xsl:text>
</xsl:template>
			
        
    <xsl:template match="chapter" name="function_list">
		<xsl:for-each select="sect1">
			<!--Beginning of section -->
			<xsl:text><![CDATA[<table class="section"><tr><th colspan="2">]]></xsl:text>
				<xsl:value-of select="title" />
				<!-- end of section header beginning of function list -->
				<xsl:text><![CDATA[</th></tr>]]></xsl:text>
			<xsl:for-each select="refentry">
				<!-- add row for each function and alternate colors of rows -->
		 		<![CDATA[<tr]]> class="<xsl:choose><xsl:when test="position() mod 2 = 0">evenrow</xsl:when><xsl:otherwise>oddrow</xsl:otherwise></xsl:choose>" <![CDATA[><td class='func'>]]><xsl:value-of select="refnamediv/refname" /><xsl:if test="contains(.,$new_tag)"><![CDATA[<sup>1</sup> ]]></xsl:if> <xsl:if test="contains(.,'GEOS 3.3')"><![CDATA[<sup>1</sup> ]]></xsl:if><![CDATA[</td>]]>
		 		<![CDATA[<td>]]><xsl:value-of select="refnamediv/refpurpose" /><![CDATA[</td></tr>]]>
		 	</xsl:for-each>
		 	<!--close section -->
		 	<![CDATA[</table>]]>
		</xsl:for-each>
	</xsl:template>
	
	 <xsl:template match="sect1[count(//refentry//refsection//programlisting) &gt; 0]">
			<!--Beginning of section -->
			<xsl:variable name="lt"><xsl:text><![CDATA[<]]></xsl:text></xsl:variable>
			<xsl:if test="contains(., 'Example')">
			<xsl:text><![CDATA[<table><tr><th colspan="2" class="example_heading">]]></xsl:text>
			<xsl:value-of select="title" /> Examples

				<!--only pull the first example seciton of each function -->
			<xsl:for-each select="refentry//refsection[contains(title,'Example')][1]/programlisting[1]">
				<!-- end of section header beginning of function list -->
				<xsl:text><![CDATA[</th></tr>]]></xsl:text>
				 <xsl:variable name='plainlisting'>
					<xsl:call-template name="globalReplace">
						<xsl:with-param name="outputString" select="."/>
						<xsl:with-param name="target" select="$lt"/>
						<xsl:with-param name="replacement" select="'&amp;lt;'"/>
					</xsl:call-template>
				</xsl:variable>
				
				<xsl:variable name='listing'>
					<xsl:call-template name="break">
						<xsl:with-param name="text" select="$plainlisting" />
					</xsl:call-template>
				</xsl:variable>
				
				<!-- add row for each function and alternate colors of rows -->
		 		<![CDATA[<tr]]> class="<xsl:choose><xsl:when test="position() mod 2 = 0">evenrow</xsl:when><xsl:otherwise>oddrow</xsl:otherwise></xsl:choose>"<![CDATA[>]]>
		 		<![CDATA[<td><b>]]><xsl:value-of select="ancestor::refentry/refnamediv/refname" /><![CDATA[</b><br /><code>]]><xsl:value-of select="$listing"  disable-output-escaping="no"/><![CDATA[</code></td></tr>]]>
		 		
		 	</xsl:for-each>
		 	<!--close section -->
		 	<![CDATA[</table>]]>
		 	</xsl:if>
		
	</xsl:template>
	
<!--General replace macro hack to make up for the fact xsl 1.0 does not have a built in one.  
	Not needed for xsl 2.0 lifted from http://www.xml.com/pub/a/2002/06/05/transforming.html -->
	<xsl:template name="globalReplace">
	  <xsl:param name="outputString"/>
	  <xsl:param name="target"/>
	  <xsl:param name="replacement"/>
	  <xsl:choose>
		<xsl:when test="contains($outputString,$target)">
		  <xsl:value-of select=
			"concat(substring-before($outputString,$target),
				   $replacement)"/>
		  <xsl:call-template name="globalReplace">
			<xsl:with-param name="outputString" 
				 select="substring-after($outputString,$target)"/>
			<xsl:with-param name="target" select="$target"/>
			<xsl:with-param name="replacement" 
				 select="$replacement"/>
		  </xsl:call-template>
		</xsl:when>
		<xsl:otherwise>
		  <xsl:value-of select="$outputString"/>
		</xsl:otherwise>
	  </xsl:choose>
	</xsl:template>
	
<xsl:template name="break">
  <xsl:param name="text" select="."/>
  <xsl:choose>
    <xsl:when test="contains($text, '&#xa;')">
      <xsl:value-of select="substring-before($text, '&#xa;')"/>
      <![CDATA[<br/>]]>
      <xsl:call-template name="break">
        <xsl:with-param 
          name="text" 
          select="substring-after($text, '&#xa;')"
        />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$text"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>
