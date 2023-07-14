<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

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

<xsl:template name="escapesinglequotes">
	<xsl:param name="arg1"/>
	<xsl:variable name="apostrophe">'</xsl:variable>
	<xsl:choose>
		<!-- this string has at least on single quote -->
		<xsl:when test="contains($arg1, $apostrophe)">
			<xsl:if test="string-length(normalize-space(substring-before($arg1, $apostrophe))) > 0"><xsl:value-of select="substring-before($arg1, $apostrophe)" disable-output-escaping="yes"/>''</xsl:if>
			<xsl:call-template name="escapesinglequotes">
				<xsl:with-param name="arg1">
					<xsl:value-of select="substring-after($arg1, $apostrophe)" disable-output-escaping="yes"/>
				</xsl:with-param>
			</xsl:call-template>
		</xsl:when>
		<!-- no quotes found in string, just print it -->
		<xsl:when test="string-length(normalize-space($arg1)) > 0"><xsl:value-of select="normalize-space($arg1)"/></xsl:when>
	</xsl:choose>
</xsl:template>

</xsl:stylesheet>
