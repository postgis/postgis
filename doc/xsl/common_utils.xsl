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

</xsl:stylesheet>
