<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:d="http://docbook.org/ns/docbook"
                xmlns="http://www.w3.org/1999/xhtml"
                exclude-result-prefixes="d"
                version="1.0">

<!--
  DocBook emits both programlisting and screen as bare <pre> elements.
  Keep the original <pre> output, but add real visible labels around the
  generated block so code/input and output remain distinguishable without
  relying on color or CSS-generated content.
-->
<xsl:include href="postgis-block-labels-common.xsl"/>

<!--
  DocBook XSL serializes external <script> links as self-closing XHTML.
  The published manual is commonly served as text/html, where a self-closing
  script tag swallows the body in browsers. Emit an explicit closing tag.
-->
<xsl:template name="make.script.link">
  <xsl:param name="script.filename" select="''"/>

  <xsl:variable name="src">
    <xsl:call-template name="relative.path.link">
      <xsl:with-param name="target.pathname" select="$script.filename"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:if test="string-length($script.filename) != 0">
    <script>
      <xsl:attribute name="src">
        <xsl:value-of select="$src"/>
      </xsl:attribute>
      <xsl:attribute name="type">
        <xsl:value-of select="$html.script.type"/>
      </xsl:attribute>
      <xsl:call-template name="other.script.attributes">
        <xsl:with-param name="script.filename" select="$script.filename"/>
      </xsl:call-template>
      <xsl:text> </xsl:text>
    </script>
  </xsl:if>
</xsl:template>

<xsl:template match="d:programlisting">
  <xsl:param name="suppress-numbers" select="'0'"/>
  <xsl:variable name="label.id" select="concat('postgis-block-label-', generate-id())"/>
  <xsl:variable name="copy.label">
    <xsl:call-template name="postgis-localized-block-label">
      <xsl:with-param name="kind" select="'copy'"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="copied.label">
    <xsl:call-template name="postgis-localized-block-label">
      <xsl:with-param name="kind" select="'copied'"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="copy.failed.label">
    <xsl:call-template name="postgis-localized-block-label">
      <xsl:with-param name="kind" select="'copy-failed'"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="has.legacy.output"
                select="contains(., '---+')
                        or contains(., '&#10;--------')
                        or contains(., '&#10;    --------')
                        or contains(., ' row)')
                        or contains(., ' rows)')
                        or contains(., 'ERROR:')
                        or contains(., 'NOTICE:')
                        or contains(., 'WARNING:')"/>

  <div class="postgis-example-block postgis-example-code" role="group" data-postgis-block="code">
    <xsl:attribute name="aria-labelledby">
      <xsl:value-of select="$label.id"/>
    </xsl:attribute>
    <xsl:if test="$has.legacy.output">
      <xsl:attribute name="data-postgis-copyable">false</xsl:attribute>
    </xsl:if>
    <div class="postgis-example-header">
      <div class="postgis-example-label">
        <xsl:attribute name="id">
          <xsl:value-of select="$label.id"/>
        </xsl:attribute>
        <xsl:call-template name="postgis-localized-block-label">
          <xsl:with-param name="kind" select="'code'"/>
        </xsl:call-template>
      </div>
      <xsl:if test="not($has.legacy.output)">
        <button class="postgis-copy-button" type="button" data-postgis-copy="code">
          <xsl:attribute name="aria-label">
            <xsl:value-of select="$copy.label"/>
          </xsl:attribute>
          <xsl:attribute name="title">
            <xsl:value-of select="$copy.label"/>
          </xsl:attribute>
          <xsl:attribute name="data-copy-label">
            <xsl:value-of select="$copy.label"/>
          </xsl:attribute>
          <xsl:attribute name="data-copied-label">
            <xsl:value-of select="$copied.label"/>
          </xsl:attribute>
          <xsl:attribute name="data-copy-failed-label">
            <xsl:value-of select="$copy.failed.label"/>
          </xsl:attribute>
          <xsl:value-of select="$copy.label"/>
        </button>
      </xsl:if>
    </div>
    <xsl:apply-imports/>
  </div>
</xsl:template>

<xsl:template match="d:screen">
  <xsl:param name="suppress-numbers" select="'0'"/>
  <xsl:variable name="label.id" select="concat('postgis-block-label-', generate-id())"/>

  <div class="postgis-example-block postgis-example-output" role="group" data-postgis-block="output">
    <xsl:attribute name="aria-labelledby">
      <xsl:value-of select="$label.id"/>
    </xsl:attribute>
    <div class="postgis-example-header">
      <div class="postgis-example-label">
        <xsl:attribute name="id">
          <xsl:value-of select="$label.id"/>
        </xsl:attribute>
        <xsl:call-template name="postgis-localized-block-label">
          <xsl:with-param name="kind" select="'output'"/>
        </xsl:call-template>
      </div>
    </div>
    <xsl:apply-imports/>
  </div>
</xsl:template>

</xsl:stylesheet>
