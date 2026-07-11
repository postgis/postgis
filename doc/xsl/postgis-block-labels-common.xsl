<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

<!--
  Shared labels for rendered DocBook code/output blocks and the HTML copy
  button. Keep this small so HTML and dblatex PDF customizations use the
  same strings without sharing output-specific templates.
-->
<xsl:template name="postgis-localized-block-label">
  <xsl:param name="kind"/>
  <xsl:variable name="label" select="document('xsl-config.xml')//block_labels/label[@role=$kind]/para"/>

  <xsl:choose>
    <xsl:when test="string($label) != ''">
      <xsl:value-of select="$label"/>
    </xsl:when>
    <xsl:when test="$kind = 'output'">Output</xsl:when>
    <xsl:when test="$kind = 'copy'">Copy</xsl:when>
    <xsl:when test="$kind = 'copied'">Copied</xsl:when>
    <xsl:when test="$kind = 'copy-failed'">Failed</xsl:when>
    <xsl:when test="$kind = 'figure'">Figure</xsl:when>
    <xsl:otherwise>Code</xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>
