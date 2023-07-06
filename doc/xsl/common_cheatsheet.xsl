<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

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


</xsl:stylesheet>
