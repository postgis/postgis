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
	<xsl:variable name='enhanced_tag'>Enhanced: <xsl:value-of select="$postgis_version" /></xsl:variable>
	<xsl:variable name='include_examples'>false</xsl:variable>
	<xsl:variable name='output_purpose'>false</xsl:variable>
<xsl:template match="/">
	<xsl:text><![CDATA[<html><head><title>PostGIS Raster Cheat Sheet</title>
	<style type="text/css">
<!--
body {
	font-family: Arial, sans-serif;
	font-size: 8.5pt;
}

.comment {font-size:x-small;color:green;font-family:"courier new"}
.notes {font-size:x-small;color:#dd1111;font-weight:500;font-family:verdana}
#example_heading {
	border-bottom: 1px solid #000;
	margin: 10px 15px 10px 85px;
	color: #4a124a;font-size: 7.5pt;
}

#content_functions {
	width:100%;
	float: left
}

#content_examples {
	float: left;
	width: 100%;
	page-break-before: auto
}

.section {
	border: 1px solid #000;
	margin: 4px;
	]]></xsl:text>
	<xsl:choose><xsl:when test="$output_purpose = 'false'"><![CDATA[width: 45%;]]></xsl:when><xsl:otherwise><![CDATA[width: 100%;]]></xsl:otherwise></xsl:choose>
<xsl:text><![CDATA[		float: left;
}

.example {
	border: 1px solid #000;
	margin: 4px;
	width: 50%;
	float:left;
}

.example b {font-size: 7.5pt}
.example th {
	border: 1px solid #000;
	color: #000;
	background-color: #ddd;
	font-size: 8.0pt;
}

.section th {
	border: 1px solid #000;
	color: #fff;
	background-color: #4a124a;
	font-size: 9.5pt;
	
}
.section td {
	font-family: Arial, sans-serif;
	font-size: 8.5pt;
	vertical-align: top;
	border: 0;
}

.func {font-weight: 600}
.func_args {font-size: 8pt;font-family:courier}

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
code {font-size: 8pt}
-->
</style>
	</head><body><h1 style='text-align:center'>PostGIS ]]></xsl:text> <xsl:value-of select="$postgis_version" /><xsl:text><![CDATA[ Raster Cheatsheet</h1>]]></xsl:text>
		<xsl:text><![CDATA[<span class='notes'>New in this release <sup>1</sup> Enhanced in this release <sup>2</sup> Requires GEOS 3.3 or higher<sup>g3.3</sup>    &nbsp;2.5/3D support<sup>3d</sup>&nbsp;SQL-MM<sup>mm</sup> &nbsp;Supports geography <sup>G</sup></span> <div id="content_functions">]]></xsl:text>
			<xsl:apply-templates select="/book/chapter[@id='RT_reference']" name="function_list" />
			<xsl:text><![CDATA[</div>]]></xsl:text>
			<xsl:text><![CDATA[<div id="content_examples">]]></xsl:text>
			<!-- examples go here -->
			<xsl:apply-templates select="/book/chapter[@id='RT_reference']/sect1[count(//refentry//refsection//programlisting) &gt; 0]"  />
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
		 		<![CDATA[<tr]]> class="<xsl:choose><xsl:when test="position() mod 2 = 0">evenrow</xsl:when><xsl:otherwise>oddrow</xsl:otherwise></xsl:choose>" <![CDATA[><td colspan='2'><span class='func'>]]><xsl:value-of select="refnamediv/refname" /><![CDATA[</span>]]><xsl:if test="contains(.,$new_tag)"><![CDATA[<sup>1</sup> ]]></xsl:if> 
		 		<!-- enhanced tag -->
		 		<xsl:if test="contains(.,$enhanced_tag)"><![CDATA[<sup>2</sup> ]]></xsl:if>
		 		<xsl:if test="contains(.,'implements the SQL/MM')"><![CDATA[<sup>mm</sup> ]]></xsl:if>
		 		<xsl:if test="contains(refsynopsisdiv/funcsynopsis,'geography') or contains(refsynopsisdiv/funcsynopsis/funcprototype/funcdef,'geography')"><![CDATA[<sup>G</sup>  ]]></xsl:if>
		 		<xsl:if test="contains(.,'GEOS &gt;= 3.3')"><![CDATA[<sup>g3.3</sup> ]]></xsl:if>
		 		<xsl:if test="contains(.,'This function supports 3d')"><![CDATA[<sup>3D</sup> ]]></xsl:if>
		 		<!-- if only one proto just dispaly it on first line -->
		 		<xsl:if test="count(refsynopsisdiv/funcsynopsis/funcprototype) = 1">
		 			(<xsl:call-template name="list_in_params"><xsl:with-param name="func" select="refsynopsisdiv/funcsynopsis/funcprototype" /></xsl:call-template>)
		 		</xsl:if>
		 		
		 		<![CDATA[&nbsp;&nbsp;]]>
		 		<xsl:if test="$output_purpose = 'true'"><xsl:value-of select="refnamediv/refpurpose" /></xsl:if>
		 		<!-- output different proto arg combos -->
		 		<xsl:if test="count(refsynopsisdiv/funcsynopsis/funcprototype) &gt; 1"><![CDATA[<span class='func_args'><ol>]]><xsl:for-each select="refsynopsisdiv/funcsynopsis/funcprototype"><![CDATA[<li>]]><xsl:call-template name="list_in_params"><xsl:with-param name="func" select="." /></xsl:call-template><![CDATA[</li>]]></xsl:for-each>
		 		<![CDATA[</ol></span>]]></xsl:if>
		 		<![CDATA[</td></tr>]]>
		 	</xsl:for-each>
		 	<!--close section -->
		 	<![CDATA[</table>]]>
		</xsl:for-each>
	</xsl:template>
	
	 <xsl:template match="sect1[//refentry//refsection[contains(title,'Example')]]">
	 		<!-- less than needed for converting html tags in listings so they are printable -->
	 		<xsl:variable name="lt"><xsl:text><![CDATA[<]]></xsl:text></xsl:variable>
	 		<!-- only print section header if it has examples - not sure why this is necessary -->
	 		<xsl:if test="refentry/refsection/programlisting">
			<!--Beginning of section -->
				<xsl:text><![CDATA[<table class='example'><tr><th colspan="2" class="example_heading">]]></xsl:text>
				<xsl:value-of select="title" /> Examples
				<!-- end of section header beginning of function list -->
				<xsl:text><![CDATA[</th></tr>]]></xsl:text>
				<!--only pull the first example section of each function -->
			<xsl:for-each select="refentry//refsection[contains(title,'Example')][1]/programlisting[1]">
				
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
		 	<![CDATA[</table>]]>
		 	</xsl:if>
		 	<!--close section -->
		 
		
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

<!--macro to pull out function parameter names so we can provide a pretty arg list prefix for each function -->
<xsl:template name="list_in_params">
	<xsl:param name="func" />
	<xsl:for-each select="$func">
		<xsl:if test="count(paramdef/parameter)  &gt; 0"> </xsl:if>
		<xsl:for-each select="paramdef">
			<xsl:choose>
				<xsl:when test="not( contains(parameter, 'OUT') )"> 
					<xsl:value-of select="parameter" />
					<xsl:if test="position()&lt;last()"><xsl:text>, </xsl:text></xsl:if>
				</xsl:when>
			</xsl:choose>
			
		</xsl:for-each>
	</xsl:for-each>	
</xsl:template>

</xsl:stylesheet>
