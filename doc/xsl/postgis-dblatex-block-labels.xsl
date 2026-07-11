<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:d="http://docbook.org/ns/docbook"
                exclude-result-prefixes="d"
                version="1.0">

<!--
  dblatex maps DocBook programlisting and screen to the same listings
  environment. Add a real text label before each block so PDF and copied text
  distinguish input/code from output without relying on color or background.
-->
<xsl:include href="postgis-block-labels-common.xsl"/>

<xsl:template name="postgis-dblatex-block-label">
  <xsl:param name="kind"/>
  <xsl:text>&#10;\par\smallskip\noindent{\sffamily\small\bfseries </xsl:text>
  <xsl:call-template name="postgis-localized-block-label">
    <xsl:with-param name="kind" select="$kind"/>
  </xsl:call-template>
  <xsl:text>}\par&#10;</xsl:text>
</xsl:template>

<!-- WKT is safe as typewriter text, but commas need explicit break points. -->
<xsl:template name="postgis-dblatex-breakable-geometry">
  <xsl:param name="text"/>
  <xsl:choose>
    <xsl:when test="contains($text, ',')">
      <xsl:value-of select="substring-before($text, ',')"/>
      <xsl:text>,\allowbreak{}</xsl:text>
      <xsl:call-template name="postgis-dblatex-breakable-geometry">
        <xsl:with-param name="text" select="substring-after($text, ',')"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$text"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="d:programlisting|programlisting">
  <xsl:call-template name="postgis-dblatex-block-label">
    <xsl:with-param name="kind" select="'code'"/>
  </xsl:call-template>
  <xsl:apply-imports/>
</xsl:template>

<xsl:template match="d:screen|screen">
	<xsl:variable name="role.tokens" select="concat(' ', normalize-space(@role), ' ')"/>
	<xsl:variable name="visual.id" select="(@xml:id | @id)[1]"/>
  <xsl:call-template name="postgis-dblatex-block-label">
    <xsl:with-param name="kind" select="'output'"/>
  </xsl:call-template>
	<xsl:if test="contains($role.tokens, ' visual-primary ') and string($visual.id) != ''">
		<xsl:text>&#10;\begingroup\scriptsize\ttfamily\noindent{</xsl:text>
		<xsl:call-template name="postgis-dblatex-breakable-geometry">
			<xsl:with-param name="text" select="normalize-space(.)"/>
		</xsl:call-template>
		<xsl:text>}\par\endgroup&#10;</xsl:text>
		<xsl:call-template name="postgis-dblatex-block-label">
			<xsl:with-param name="kind" select="'figure'"/>
		</xsl:call-template>
		<xsl:text>&#10;\begin{center}\includegraphics[width=0.96\linewidth,height=0.25\textheight,keepaspectratio]{images/visual-examples/</xsl:text>
		<xsl:value-of select="$visual.id"/>
		<xsl:text>.png}\end{center}&#10;</xsl:text>
	</xsl:if>
	<xsl:if test="not(contains($role.tokens, ' visual-primary ')) or string($visual.id) = ''">
		<xsl:apply-imports/>
	</xsl:if>
</xsl:template>

</xsl:stylesheet>
