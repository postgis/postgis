<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:import href="postgis_gardentest.sql.xsl"/>
  <xsl:output method="text" encoding="UTF-8"/>

  <xsl:template name="case">
    <xsl:param name="name"/>
    <xsl:param name="value"/>
    <xsl:value-of select="$name"/><xsl:text>|</xsl:text>
    <xsl:call-template name="escapesinglequotes">
      <xsl:with-param name="arg1" select="$value"/>
    </xsl:call-template>
    <xsl:text>&#10;</xsl:text>
  </xsl:template>

  <xsl:template match="/">
    <xsl:call-template name="case"><xsl:with-param name="name" select="'plain'"/><xsl:with-param name="value" select="'plain'"/></xsl:call-template>
    <xsl:call-template name="case"><xsl:with-param name="name" select="'one'"/><xsl:with-param name="value" select="&quot;O'Brien&quot;"/></xsl:call-template>
    <xsl:call-template name="case"><xsl:with-param name="name" select="'leading'"/><xsl:with-param name="value" select="&quot;'start&quot;"/></xsl:call-template>
    <xsl:call-template name="case"><xsl:with-param name="name" select="'adjacent'"/><xsl:with-param name="value" select="&quot;a''b&quot;"/></xsl:call-template>
    <xsl:call-template name="case"><xsl:with-param name="name" select="'trailing'"/><xsl:with-param name="value" select="&quot;end'&quot;"/></xsl:call-template>
    <xsl:call-template name="case"><xsl:with-param name="name" select="'whitespace'"/><xsl:with-param name="value" select="'  keep  spaces  '"/></xsl:call-template>
  </xsl:template>
</xsl:stylesheet>
