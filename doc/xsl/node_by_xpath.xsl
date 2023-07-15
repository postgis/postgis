<?xml version="1.0"?>

<!--
Example usage (replace the single dash with two dashes):

	xsltproc \
		-stringparam xpath "/config/tags/para[@role='tag_P_support']"
		xsl-config.xml
-->

<xsl:stylesheet
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
	xmlns:dyn="http://exslt.org/dynamic" extension-element-prefixes="dyn"
>
  <xsl:output method="xml" omit-xml-declaration="yes" indent="no" />

  <xsl:template match="/">
    <xsl:for-each select="dyn:evaluate($xpath)">
      <xsl:copy-of select="." />
    </xsl:for-each>
  </xsl:template>
</xsl:stylesheet>

