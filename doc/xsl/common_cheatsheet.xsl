<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>

<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:db="http://docbook.org/ns/docbook"
	exclude-result-prefixes="db"
>

<xsl:variable name='new_tag' select='"availability"' />
<xsl:variable name='geos_tag' select='"geos_requirement"' />
<xsl:variable name='sfcgal_tag' select='"sfcgal_requirement"' />
<xsl:variable name='enhanced_tag' select='"enhanced"' />
<xsl:variable name='sqlmm_conformance_tag' select='"sqlmm"' />
<xsl:variable name='Z_conformance_tag' select='"3d"' />
<xsl:variable name='geography_tag' select='"geography_support"' />
<xsl:variable name='curve_conformance_tag' select='"curve"' />
<xsl:variable name='include_examples'>false</xsl:variable>
<xsl:variable name='output_purpose'>true</xsl:variable>
<xsl:variable name='linkstub'>https://postgis.net/docs/manual-<xsl:value-of select="$postgis_version" />/<xsl:value-of select="$postgis_language" /></xsl:variable>
<xsl:variable name="cheatsheets_config" select="document('xsl-config.xml')//cheatsheets" />


<!--macro to pull out function parameter names so we can provide a pretty arg list prefix for each function -->
<xsl:template name="list_in_params">
	<xsl:param name="func" />
	<xsl:for-each select="$func">
		<xsl:if test="count(db:paramdef/db:parameter)  &gt; 0"> </xsl:if>
		<xsl:for-each select="db:paramdef">
			<xsl:choose>
				<xsl:when test="not( contains(db:parameter, 'OUT') )">
					<xsl:value-of select="db:parameter" />
					<xsl:if test="position()&lt;last()"><xsl:text>, </xsl:text></xsl:if>
				</xsl:when>
			</xsl:choose>
		</xsl:for-each>
	</xsl:for-each>
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

<xsl:template match="db:chapter" name="function_list_html">

	<div id="content_functions_left">

		<xsl:variable name="col_func_count"><xsl:value-of select="count(descendant::*//db:funcprototype) div 1.65" /></xsl:variable>

    <!--count(preceding-sibling::*/db:refentry/db:refsynopsisdiv/db:funcsynopsis/db:funcprototype)-->
		<xsl:for-each select="db:section[count(//db:funcprototype) &gt; 0 ]">

			 <xsl:apply-templates select="." />

		</xsl:for-each>
	</div>

</xsl:template>

<xsl:template match="db:section">
	<xsl:variable name="col_cur"><xsl:value-of select="count(current()//db:funcprototype) + count(preceding-sibling::*//db:funcprototype)"/></xsl:variable>

<!--
	<xsl:if test="$col_cur &gt;$col_func_count and count(preceding-sibling::*//db:funcprototype) &lt; $col_func_count ">
			</div><div id="content_functions_right">
	</xsl:if>
-->

	<!--Beginning of section -->
	<table class="section"><tr><th colspan="2"><xsl:value-of select="db:title" />
		<!-- end of section header beginning of function list -->
		</th></tr>
	<xsl:for-each select="current()//db:refentry">

		<xsl:variable name="ref" select="." />

		<!-- add row for each function and alternate colors of rows -->
		<!-- , hyperlink to online manual -->
		<tr>
			<xsl:attribute name="class">
				<xsl:choose>
					<xsl:when test="position() mod 2 = 0">evenrow</xsl:when>
					<xsl:otherwise>oddrow</xsl:otherwise>
				</xsl:choose>
			</xsl:attribute>

		<td colspan='2'>
			<span class='func'>
				<a target="_blank">
					<xsl:attribute name="href">
						<xsl:value-of select="concat(concat($linkstub, @xml:id), '.html')" />
					</xsl:attribute>
					<xsl:value-of select="db:refnamediv/db:refname" />
				</a>
			</span>
		<xsl:if test="$ref//db:para[@role=$new_tag and starts-with(@conformance, $postgis_version)]">
			&nbsp;<sup>1</sup>
		</xsl:if>
		<xsl:if test="$ref//db:para[@role=$enhanced_tag and starts-with(@conformance, $postgis_version)]">
			&nbsp;<sup>2</sup>
		</xsl:if>
		<xsl:if test="$ref/descendant::*[@conformance=$sqlmm_conformance_tag]">
			&nbsp;<sup>mm</sup>
		</xsl:if>
		<xsl:if test="contains(db:refsynopsisdiv/db:funcsynopsis,'geography') or contains(db:refsynopsisdiv/db:funcsynopsis/db:funcprototype/funcdef,'geography')">
			&nbsp;<sup>G</sup>
		</xsl:if>
		<xsl:if test="$ref//db:para[@role=$sfcgal_tag and starts-with(./@conformance, '1.5')]">
			&nbsp;<sup>cg1.5</sup>
		</xsl:if>
		<xsl:if test="$ref//db:para[@role=$geos_tag and starts-with(./@conformance, '3.9')]">
			&nbsp;<sup>g3.9</sup>
		</xsl:if>
		<xsl:if test="$ref//db:para[@role=$geos_tag and starts-with(./@conformance, '3.10')]">
			&nbsp;<sup>g3.10</sup>
		</xsl:if>
		<xsl:if test="$ref//db:para[@role=$geos_tag and starts-with(./@conformance, '3.11')]">
			&nbsp;<sup>g3.11</sup>
		</xsl:if>
		<xsl:if test="$ref//db:para[@role=$geos_tag and starts-with(./@conformance, '3.12')]">
			&nbsp;<sup>g3.12</sup>
		</xsl:if>
		<xsl:if test="$ref/descendant::*[@conformance=$Z_conformance_tag]">
			&nbsp;<sup>3d</sup>
		</xsl:if>

		<!-- if only one proto just dispaly it on first line -->
		<xsl:if test="count(db:refsynopsisdiv/db:funcsynopsis/db:funcprototype) = 1">
			(<xsl:call-template name="list_in_params"><xsl:with-param name="func" select="db:refsynopsisdiv/db:funcsynopsis/db:funcprototype" /></xsl:call-template>)
		</xsl:if>

		&nbsp;&nbsp;
		<xsl:if test="$output_purpose = 'true'"><xsl:value-of select="db:refnamediv/db:refpurpose" /></xsl:if>
		<!-- output different proto arg combos -->
		<xsl:if test="count(db:refsynopsisdiv/db:funcsynopsis/db:funcprototype) &gt; 1">
			<span class='func_args'><ol>
			<xsl:for-each select="db:refsynopsisdiv/db:funcsynopsis/db:funcprototype">
				<li><xsl:call-template name="list_in_params"><xsl:with-param name="func" select="." /></xsl:call-template>
				<xsl:if test=".//db:paramdef[contains(db:type,' set')]"><sup> agg</sup></xsl:if>
				<xsl:if test=".//db:paramdef[contains(db:type,' winset')]"> <sup>W</sup> </xsl:if></li>
			</xsl:for-each>
			</ol></span>
		</xsl:if>
		</td></tr>
		</xsl:for-each>
		</table><br />
		<!--close section -->
</xsl:template>

<xsl:template name="examples_list">

	<!--Beginning of section -->
	<xsl:variable name="lt"><xsl:text><![CDATA[<]]></xsl:text></xsl:variable>

	<!-- TODO: do not rely on english "Example" -->
	<xsl:if test="contains(., 'Example')">
	<table>
		<tr>
			<th colspan="2" class="example_heading">
				<!-- TODO: make Examples translatable -->
				<xsl:value-of select="db:title" /> Examples
			</th>
		</tr>

		<!--only pull the first example section of each function -->
		<xsl:for-each select="db:refentry//db:refsection[contains(db:title,'Example')][1]/db:programlisting[1]">
			<!-- add row for each function and alternate colors of rows -->
			<tr>
				<xsl:attribute name="class">
					<xsl:choose>
						<xsl:when test="position() mod 2 = 0">evenrow</xsl:when>
						<xsl:otherwise>oddrow</xsl:otherwise>
					</xsl:choose>
				</xsl:attribute>
				<td>
					<b>
						<xsl:value-of select="ancestor::db:refentry/db:refnamediv/db:refname" />
					</b>
					<br />
					<pre>
						<xsl:value-of select="."  disable-output-escaping="no"/>
					</pre>
				</td>
			</tr>

			</xsl:for-each>
	<!--close section -->
	</table>
	</xsl:if>

</xsl:template>

</xsl:stylesheet>
