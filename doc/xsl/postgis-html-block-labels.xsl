<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:d="http://docbook.org/ns/docbook"
                xmlns:exsl="http://exslt.org/common"
                xmlns:h="http://www.w3.org/1999/xhtml"
                xmlns="http://www.w3.org/1999/xhtml"
                exclude-result-prefixes="d exsl h"
                version="1.0">

<!--
  DocBook emits both programlisting and screen as bare <pre> elements.
  Keep the original <pre> output, but add real visible labels around the
  generated block so code/input and output remain distinguishable without
  relying on color or CSS-generated content.
-->
<xsl:include href="postgis-block-labels-common.xsl"/>

<xsl:template match="@*|node()" mode="postgis-code-block-html">
  <xsl:param name="language"/>
  <xsl:copy>
    <xsl:apply-templates select="@*|node()" mode="postgis-code-block-html">
      <xsl:with-param name="language" select="$language"/>
    </xsl:apply-templates>
  </xsl:copy>
</xsl:template>


<xsl:template match="pre[contains(concat(' ', @class, ' '), ' programlisting ')] | h:pre[contains(concat(' ', @class, ' '), ' programlisting ')]" mode="postgis-code-block-html">
  <xsl:param name="language"/>
  <xsl:copy>
    <xsl:copy-of select="@*[name() != 'class' and name() != 'data-postgis-language']"/>
    <xsl:attribute name="class">
      <xsl:value-of select="normalize-space(@class)"/>
      <xsl:if test="$language != ''">
        <xsl:text> language-</xsl:text>
        <xsl:value-of select="$language"/>
      </xsl:if>
    </xsl:attribute>
    <xsl:if test="$language != ''">
      <xsl:attribute name="data-postgis-language">
        <xsl:value-of select="$language"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:apply-templates select="node()" mode="postgis-code-block-html">
      <xsl:with-param name="language" select="$language"/>
    </xsl:apply-templates>
  </xsl:copy>
</xsl:template>

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
  <xsl:variable name="code.text" select="normalize-space(.)"/>
  <xsl:variable name="code.text.upper"
                select="translate($code.text, 'abcdefghijklmnopqrstuvwxyz', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/>
  <xsl:variable name="looks.like.sql"
                select="starts-with($code.text.upper, 'SELECT ')
                        or starts-with($code.text.upper, 'WITH ')
                        or starts-with($code.text.upper, 'VALUES ')
                        or starts-with($code.text.upper, 'INSERT ')
                        or starts-with($code.text.upper, 'UPDATE ')
                        or starts-with($code.text.upper, 'DELETE ')
                        or starts-with($code.text.upper, 'CREATE ')
                        or starts-with($code.text.upper, 'ALTER ')
                        or starts-with($code.text.upper, 'DROP ')
                        or starts-with($code.text.upper, 'EXPLAIN ')
                        or starts-with($code.text.upper, 'DO ')
                        or starts-with($code.text.upper, 'BEGIN;')
                        or starts-with($code.text.upper, 'COMMIT;')"/>
  <xsl:variable name="declared.language"
                select="translate(normalize-space(@language), 'ABCDEFGHIJKLMNOPQRSTUVWXYZ ', 'abcdefghijklmnopqrstuvwxyz-')"/>
  <xsl:variable name="effective.language">
    <xsl:choose>
      <xsl:when test="$declared.language != ''">
        <xsl:value-of select="$declared.language"/>
      </xsl:when>
      <xsl:when test="$looks.like.sql">sql</xsl:when>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="rendered.block">
    <xsl:apply-imports/>
  </xsl:variable>

  <div role="group" data-postgis-block="code">
    <xsl:attribute name="class">
      <xsl:text>postgis-example-block postgis-example-code</xsl:text>
      <xsl:if test="$effective.language != ''">
        <xsl:text> postgis-example-language-</xsl:text>
        <xsl:value-of select="$effective.language"/>
      </xsl:if>
    </xsl:attribute>
    <xsl:if test="$effective.language != ''">
      <xsl:attribute name="data-postgis-language">
        <xsl:value-of select="$effective.language"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:attribute name="aria-labelledby">
      <xsl:value-of select="$label.id"/>
    </xsl:attribute>
    <div class="postgis-example-header">
      <div class="postgis-example-label">
        <xsl:attribute name="id">
          <xsl:value-of select="$label.id"/>
        </xsl:attribute>
        <xsl:call-template name="postgis-localized-block-label">
          <xsl:with-param name="kind" select="'code'"/>
        </xsl:call-template>
      </div>
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
    </div>
    <xsl:apply-templates select="exsl:node-set($rendered.block)/node()" mode="postgis-code-block-html">
      <xsl:with-param name="language" select="$effective.language"/>
    </xsl:apply-templates>
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
