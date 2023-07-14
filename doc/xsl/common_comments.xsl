<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:db="http://docbook.org/ns/docbook"
	exclude-result-prefixes="db"
>

<!--macro to pull out function parameter names so we can provide a pretty arg list prefix for each function.  -->
<xsl:template name="listparams">
	<xsl:param name="func" />
	<xsl:for-each select="$func">
		<xsl:if test="count(db:paramdef/db:parameter) &gt; 0">args: </xsl:if>
		<xsl:for-each select="db:paramdef">
			<xsl:choose>
				<xsl:when test="count(db:parameter) &gt; 0">
					<xsl:call-template name="escapesinglequotes">
						<xsl:with-param name="arg1" select="db:parameter"/>
					</xsl:call-template>
				</xsl:when>
			</xsl:choose>
			<xsl:if test="position()&lt;last()"><xsl:text>, </xsl:text></xsl:if>
		</xsl:for-each>
		<xsl:if test="count(db:paramdef/db:parameter) &gt; 0"> - </xsl:if>
	</xsl:for-each>
</xsl:template>

</xsl:stylesheet>
