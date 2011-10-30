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
	<xsl:variable name='postgis_version'>2.0.0</xsl:variable>
<xsl:template match="/">
	<xsl:text><![CDATA[<html><head><title>PostGIS Topology Cheat Sheet</title>
	<style type="text/css">
<!--
body {
	font-family: Arial, sans-serif;
	font-size: 8.5pt;
}

#example_heading {
	border-bottom: 1px solid #000;
	margin: 10px 15px 10px 85px;
	color: #00d
}

#contentbox {
	margin-left: 90px;
	margin-right: 15px;
	top: 50px;
}		

#content_functions {
	float: left;
	width:44%;
}

#content_examples {
	float: right;
	width: 53%;
}

#sidebar1 {
		position: absolute;
		left:10px;
		width:75px;
		top:50px;
		text-align: right;
}

.section {
	border: 1px solid #000;
	margin: 4px;
	width: 100%;
}

.section th {
	border: 1px solid #000;
	color: #fff;
	background-color: #b63300;
	font-size: 9.5pt;
	
}
.section td {
	font-family: Arial, sans-serif;
	font-size: 8.5pt;
	vertical-align: top;
	border: 0;
}

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
	</head><body><h1 style='text-align:center'>PostGIS ]]></xsl:text> <xsl:value-of select="$postgis_version" /><xsl:text><![CDATA[ Topology Cheatsheet</h1>]]></xsl:text>
		<xsl:text><![CDATA[<div id="content_functions">]]></xsl:text>
			<xsl:apply-templates select="/book/chapter[@id='Topology']" name="function_list" />
			<xsl:text><![CDATA[</div>]]></xsl:text>
			<xsl:text><![CDATA[<div id="content_examples">]]></xsl:text>
			<!-- examples go here -->
			<xsl:apply-templates select="/book/chapter[@id='Topology']/sect1[count(//refentry//refsection//programlisting) > 0]"  />
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
		 		<![CDATA[<tr]]> class="<xsl:choose><xsl:when test="position() mod 2 = 0">evenrow</xsl:when><xsl:otherwise>oddrow</xsl:otherwise></xsl:choose>" <![CDATA[><td>]]><xsl:value-of select="refnamediv/refname" /><![CDATA[</td>]]>
		 		<![CDATA[<td>]]><xsl:value-of select="refnamediv/refpurpose" /><![CDATA[</td></tr>]]>
		 	</xsl:for-each>
		 	<!--close section -->
		 	<![CDATA[</table>]]>
		</xsl:for-each>
	</xsl:template>
	
	 <xsl:template match="sect1[count(//refentry//refsection//programlisting) > 0]">
			<!--Beginning of section -->
		
			 	<xsl:text><![CDATA[<table><tr><th colspan="2" class="example_heading">]]></xsl:text>
				<xsl:value-of select="title" /> Examples
				<!-- end of section header beginning of function list -->
				<xsl:text><![CDATA[</th></tr>]]></xsl:text>
			<xsl:for-each select="refentry/refsection/programlisting[1]">
				 <xsl:variable name='plainlisting'>
				 	<xsl:value-of select="."/>
				 </xsl:variable>
				 
		<xsl:variable name='listing'>
			<xsl:call-template name="break">
				<xsl:with-param name="text" select="."/>
			</xsl:call-template>
		</xsl:variable>

				<!-- add row for each function and alternate colors of rows -->
		 		<![CDATA[<tr]]> class="<xsl:choose><xsl:when test="position() mod 2 = 0">evenrow</xsl:when><xsl:otherwise>oddrow</xsl:otherwise></xsl:choose>"<![CDATA[>]]>
		 		<![CDATA[<td><code>]]><xsl:value-of select="$listing" /><![CDATA[</code></td></tr>]]>
		 	</xsl:for-each>
		 	<!--close section -->
		 	<![CDATA[</table>]]>
		
	</xsl:template>
	
<!--General replace macro hack to make up for the fact xsl 1.0 does not have a built in one.  
	Not needed for xsl 2.0 lifted from http://www.xml.com/pub/a/2002/06/05/transforming.html -->
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
