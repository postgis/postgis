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

<xsl:template match="d:programlisting|programlisting">
  <xsl:call-template name="postgis-dblatex-block-label">
    <xsl:with-param name="kind" select="'code'"/>
  </xsl:call-template>
  <xsl:apply-imports/>
</xsl:template>

<xsl:template match="d:screen|screen">
  <xsl:call-template name="postgis-dblatex-block-label">
    <xsl:with-param name="kind" select="'output'"/>
  </xsl:call-template>
  <xsl:apply-imports/>
</xsl:template>

</xsl:stylesheet>
