<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:variable name='new_tag'>Availability: <xsl:value-of select="$postgis_version" /></xsl:variable>
<xsl:variable name='enhanced_tag'>Enhanced: <xsl:value-of select="$postgis_version" /></xsl:variable>
<xsl:variable name='include_examples'>false</xsl:variable>
<xsl:variable name='output_purpose'>true</xsl:variable>
<xsl:variable name='linkstub'>https://postgis.net/docs/manual-<xsl:value-of select="$postgis_version" />/<xsl:value-of select="$postgis_language" /></xsl:variable>
<xsl:variable name="cheatsheets_config" select="document('xsl-config.xml')//cheatsheets" />


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

<xsl:template match="chapter" name="function_list_html">

	<div id="content_functions_left">

		<xsl:variable name="col_func_count"><xsl:value-of select="count(descendant::*//funcprototype) div 1.65" /></xsl:variable>

    <!--count(preceding-sibling::*/refentry/refsynopsisdiv/funcsynopsis/funcprototype)-->
		<xsl:for-each select="sect1[count(//funcprototype) &gt; 0 and not( contains(@id,'sfcgal') )]">

			<xsl:variable name="col_cur"><xsl:value-of select="count(current()//funcprototype) + count(preceding-sibling::*//funcprototype)"/></xsl:variable>

<!--
			<xsl:if test="$col_cur &gt;$col_func_count and count(preceding-sibling::*//funcprototype) &lt; $col_func_count ">
					</div><div id="content_functions_right">
			</xsl:if>
-->

			<!--Beginning of section -->
			<table class="section"><tr><th colspan="2"><xsl:value-of select="title" />
				<!-- end of section header beginning of function list -->
				</th></tr>
			<xsl:for-each select="current()//refentry">
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
								<xsl:value-of select="concat(concat($linkstub, @id), '.html')" />
							</xsl:attribute>
							<xsl:value-of select="refnamediv/refname" />
						</a>
					</span>
				<xsl:if test="contains(.,$new_tag)"><sup>1</sup> </xsl:if>
		 		<!-- enhanced tag -->
		 		<xsl:if test="contains(.,$enhanced_tag)"><sup>2</sup> </xsl:if>
		 		<xsl:if test="contains(.,'implements the SQL/MM')"><sup>mm</sup> </xsl:if>
		 		<xsl:if test="contains(refsynopsisdiv/funcsynopsis,'geography') or contains(refsynopsisdiv/funcsynopsis/funcprototype/funcdef,'geography')"><sup>G</sup>  </xsl:if>
		 		<xsl:if test="contains(.,'GEOS &gt;= 3.9')"><sup>g3.9</sup> </xsl:if>
		 		<xsl:if test="contains(.,'This function supports 3d')"><sup>3d</sup> </xsl:if>
		 		<!-- if only one proto just dispaly it on first line -->
		 		<xsl:if test="count(refsynopsisdiv/funcsynopsis/funcprototype) = 1">
		 			(<xsl:call-template name="list_in_params"><xsl:with-param name="func" select="refsynopsisdiv/funcsynopsis/funcprototype" /></xsl:call-template>)
		 		</xsl:if>

		 		&nbsp;&nbsp;
		 		<xsl:if test="$output_purpose = 'true'"><xsl:value-of select="refnamediv/refpurpose" /></xsl:if>
		 		<!-- output different proto arg combos -->
		 		<xsl:if test="count(refsynopsisdiv/funcsynopsis/funcprototype) &gt; 1"><span class='func_args'><ol><xsl:for-each select="refsynopsisdiv/funcsynopsis/funcprototype"><li><xsl:call-template name="list_in_params"><xsl:with-param name="func" select="." /></xsl:call-template><xsl:if test=".//paramdef[contains(type,' set')] or .//paramdef[contains(type,'geography set')] or
						.//paramdef[contains(type,'raster set')]"><sup> agg</sup> </xsl:if><xsl:if test=".//paramdef[contains(type,'winset')]"> <sup>W</sup> </xsl:if></li></xsl:for-each>
		 		</ol></span></xsl:if>
		 		</td></tr>
		 		</xsl:for-each>
		 		</table><br />
		 		<!--close section -->
		 	</xsl:for-each>
		</div>

</xsl:template>


</xsl:stylesheet>
