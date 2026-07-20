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
<xsl:param name="postgis.visual.manifest" select="''"/>
<xsl:param name="postgis.visual.version" select="''"/>

<!-- Keep a semantic hook around literal syntax collections.  The upstream
     DocBook itemized-list template uses role internally and does not retain it
     in the generated HTML class list. -->
<xsl:template match="d:itemizedlist[@role = 'postgis-syntax-list'] | d:itemizedlist[@role = 'postgis-syntax-comparison']">
  <div>
    <xsl:attribute name="class">
      <xsl:text>postgis-syntax-list</xsl:text>
      <xsl:if test="@role = 'postgis-syntax-comparison'">
        <xsl:text> postgis-syntax-comparison</xsl:text>
      </xsl:if>
    </xsl:attribute>
    <xsl:apply-imports/>
  </div>
</xsl:template>

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
                        or starts-with($code.text.upper, 'SET ')
                        or starts-with($code.text.upper, 'CREATE ')
                        or starts-with($code.text.upper, 'ALTER ')
                        or starts-with($code.text.upper, 'DROP ')
                        or starts-with($code.text.upper, 'EXPLAIN ')
                        or starts-with($code.text.upper, 'ANALYZE ')
                        or starts-with($code.text.upper, 'VACUUM ')
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
  <xsl:variable name="next.screen" select="following-sibling::*[1][self::d:screen]"/>
  <xsl:variable name="next.screen.role.tokens"
                select="concat(' ', normalize-space($next.screen/@role), ' ')"/>
  <xsl:variable name="next.refentry.id" select="string(ancestor::d:refentry[1]/@xml:id)"/>
  <xsl:variable name="next.screen.ordinal">
    <xsl:choose>
      <xsl:when test="$next.refentry.id != ''">
        <xsl:value-of select="count($next.screen/preceding::d:screen[ancestor::d:refentry[1]/@xml:id = $next.refentry.id]) + 1"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="count($next.screen/preceding::d:screen[not(ancestor::d:refentry)]) + 1"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="next.manifest.visual"
                select="document($postgis.visual.manifest)/visual-examples/visual[@refentry = $next.refentry.id and @screen = string($next.screen.ordinal)]"/>
  <xsl:variable name="visual.id">
    <xsl:choose>
      <xsl:when test="$next.screen and contains($next.screen.role.tokens, ' visual-primary ') and $next.screen/@xml:id">
        <xsl:value-of select="$next.screen/@xml:id"/>
      </xsl:when>
      <xsl:when test="$next.screen"><xsl:value-of select="$next.manifest.visual/@id"/></xsl:when>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="rendered.block">
    <xsl:apply-imports/>
  </xsl:variable>

  <div role="group" data-postgis-block="code">
    <xsl:if test="string($visual.id) != ''">
      <xsl:attribute name="data-postgis-visual-id"><xsl:value-of select="$visual.id"/></xsl:attribute>
    </xsl:if>
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
  <xsl:variable name="role.tokens" select="concat(' ', normalize-space(@role), ' ')"/>
  <xsl:variable name="refentry.id" select="string(ancestor::d:refentry[1]/@xml:id)"/>
  <xsl:variable name="screen.ordinal">
    <xsl:choose>
      <xsl:when test="$refentry.id != ''">
        <xsl:value-of select="count(preceding::d:screen[ancestor::d:refentry[1]/@xml:id = $refentry.id]) + 1"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="count(preceding::d:screen[not(ancestor::d:refentry)]) + 1"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="manifest.visual"
                select="document($postgis.visual.manifest)/visual-examples/visual[@refentry = $refentry.id and @screen = string($screen.ordinal)]"/>
  <xsl:variable name="native.output" select="$manifest.visual/native-output[@format = 'hexewkb']"/>
  <xsl:variable name="show.text.label">
    <xsl:call-template name="postgis-localized-block-label">
      <xsl:with-param name="kind" select="'show-text'"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="hide.text.label">
    <xsl:call-template name="postgis-localized-block-label">
      <xsl:with-param name="kind" select="'hide-text'"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="show.output.text.label">
    <xsl:call-template name="postgis-localized-block-label">
      <xsl:with-param name="kind" select="'show-output-text'"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="hide.output.text.label">
    <xsl:call-template name="postgis-localized-block-label">
      <xsl:with-param name="kind" select="'hide-output-text'"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="show.hexewkb.label">
    <xsl:call-template name="postgis-localized-block-label">
      <xsl:with-param name="kind" select="'show-hexewkb'"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="show.native.hexewkb.label">
    <xsl:call-template name="postgis-localized-block-label">
      <xsl:with-param name="kind" select="'show-native-hexewkb'"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="show.ewkt.label">
    <xsl:call-template name="postgis-localized-block-label">
      <xsl:with-param name="kind" select="'show-ewkt'"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="show.readable.ewkt.label">
    <xsl:call-template name="postgis-localized-block-label">
      <xsl:with-param name="kind" select="'show-readable-ewkt'"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="visual.id">
    <xsl:choose>
      <xsl:when test="contains($role.tokens, ' visual-primary ') and @xml:id">
        <xsl:value-of select="@xml:id"/>
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="$manifest.visual/@id"/></xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="visual.preferred"
                select="not(contains($role.tokens, ' text-primary '))
                        and ($manifest.visual/@preferred = 'true'
                             or (not($manifest.visual) and contains($role.tokens, ' visual-primary ')))"/>

  <div role="group" data-postgis-block="output">
    <xsl:attribute name="class">
      <xsl:text>postgis-example-block postgis-example-output</xsl:text>
      <xsl:if test="contains($role.tokens, ' psql-expanded ')">
        <xsl:text> postgis-example-output-expanded</xsl:text>
      </xsl:if>
    </xsl:attribute>
    <xsl:if test="contains($role.tokens, ' psql-expanded ')">
      <xsl:attribute name="data-postgis-output-layout">expanded</xsl:attribute>
    </xsl:if>
    <xsl:if test="string($visual.id) != ''">
      <xsl:attribute name="data-postgis-visual-id"><xsl:value-of select="$visual.id"/></xsl:attribute>
    </xsl:if>
    <xsl:if test="$visual.preferred">
      <xsl:attribute name="data-postgis-output-preference">visual</xsl:attribute>
    </xsl:if>
    <xsl:attribute name="data-postgis-show-text-label"><xsl:value-of select="$show.text.label"/></xsl:attribute>
    <xsl:attribute name="data-postgis-hide-text-label"><xsl:value-of select="$hide.text.label"/></xsl:attribute>
    <xsl:attribute name="data-postgis-show-output-text-label"><xsl:value-of select="$show.output.text.label"/></xsl:attribute>
    <xsl:attribute name="data-postgis-hide-output-text-label"><xsl:value-of select="$hide.output.text.label"/></xsl:attribute>
    <xsl:attribute name="data-postgis-show-hexewkb-label"><xsl:value-of select="$show.hexewkb.label"/></xsl:attribute>
    <xsl:attribute name="data-postgis-show-native-hexewkb-label"><xsl:value-of select="$show.native.hexewkb.label"/></xsl:attribute>
    <xsl:attribute name="data-postgis-show-ewkt-label"><xsl:value-of select="$show.ewkt.label"/></xsl:attribute>
    <xsl:attribute name="data-postgis-show-readable-ewkt-label"><xsl:value-of select="$show.readable.ewkt.label"/></xsl:attribute>
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
      <xsl:if test="$native.output">
        <button class="postgis-output-representation-toggle" type="button"
                aria-expanded="false">
          <xsl:attribute name="aria-label"><xsl:value-of select="$show.native.hexewkb.label"/></xsl:attribute>
          <xsl:attribute name="title"><xsl:value-of select="$show.native.hexewkb.label"/></xsl:attribute>
          <xsl:value-of select="$show.hexewkb.label"/>
        </button>
      </xsl:if>
    </div>
    <xsl:apply-imports/>
    <xsl:if test="$native.output">
      <pre class="screen postgis-native-output" hidden="hidden" aria-hidden="true"><xsl:value-of select="$native.output"/></pre>
    </xsl:if>
  </div>
  <xsl:if test="string($visual.id) != ''">
    <div class="postgis-geometry-figure" data-postgis-built-geometry="true">
      <xsl:attribute name="data-postgis-visual-id"><xsl:value-of select="$visual.id"/></xsl:attribute>
      <div class="postgis-example-header">
        <div class="postgis-example-label">
          <xsl:call-template name="postgis-localized-block-label">
            <xsl:with-param name="kind" select="'figure'"/>
          </xsl:call-template>
        </div>
      </div>
      <xsl:variable name="visual.src">
        <xsl:value-of select="concat($img.src.path, 'images/visual-examples/', $visual.id, '.svg')"/>
        <xsl:if test="$postgis.visual.version != ''">
          <xsl:value-of select="concat('?v=', $postgis.visual.version)"/>
        </xsl:if>
      </xsl:variable>
      <object type="image/svg+xml">
        <xsl:if test="$manifest.visual/@width">
          <xsl:attribute name="width"><xsl:value-of select="$manifest.visual/@width"/></xsl:attribute>
        </xsl:if>
        <xsl:if test="$manifest.visual/@height">
          <xsl:attribute name="height"><xsl:value-of select="$manifest.visual/@height"/></xsl:attribute>
        </xsl:if>
        <xsl:attribute name="class">
          <xsl:text>postgis-geometry-figure-body</xsl:text>
          <xsl:if test="$manifest.visual/@kind = 'image-output'">
            <xsl:text> postgis-image-output-figure-body</xsl:text>
          </xsl:if>
        </xsl:attribute>
        <xsl:attribute name="data">
          <xsl:value-of select="$visual.src"/>
        </xsl:attribute>
        <img class="postgis-geometry-figure-fallback">
          <xsl:attribute name="src"><xsl:value-of select="$visual.src"/></xsl:attribute>
          <xsl:attribute name="alt">
            <xsl:text>Geometry figure for </xsl:text><xsl:value-of select="$visual.id"/>
          </xsl:attribute>
        </img>
      </object>
    </div>
  </xsl:if>
</xsl:template>

</xsl:stylesheet>
