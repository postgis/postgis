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
<xsl:param name="postgis.visual.manifest" select="''"/>

<xsl:template name="postgis-dblatex-block-label">
  <xsl:param name="kind"/>
  <xsl:text>&#10;\postgisblocklabel{</xsl:text>
  <xsl:call-template name="postgis-localized-block-label">
    <xsl:with-param name="kind" select="$kind"/>
  </xsl:call-template>
  <xsl:text>}&#10;</xsl:text>
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
  <xsl:variable name="refentry.id"
                select="string((ancestor::d:refentry | ancestor::refentry)[1]/@xml:id)"/>
  <xsl:variable name="screen.ordinal">
    <xsl:choose>
      <xsl:when test="$refentry.id != ''">
        <xsl:value-of select="count(preceding::d:screen[(ancestor::d:refentry | ancestor::refentry)[1]/@xml:id = $refentry.id] | preceding::screen[(ancestor::d:refentry | ancestor::refentry)[1]/@xml:id = $refentry.id]) + 1"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="count(preceding::d:screen[not(ancestor::d:refentry or ancestor::refentry)] | preceding::screen[not(ancestor::d:refentry or ancestor::refentry)]) + 1"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="manifest.visual"
                select="document($postgis.visual.manifest)/visual-examples/visual[@refentry = $refentry.id and @screen = string($screen.ordinal)]"/>
	<xsl:variable name="visual.id">
    <xsl:choose>
      <xsl:when test="contains($role.tokens, ' visual-primary ') and (@xml:id or @id)">
        <xsl:value-of select="(@xml:id | @id)[1]"/>
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="$manifest.visual/@id"/></xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
	<xsl:variable name="visual.preferred"
	              select="not(contains($role.tokens, ' text-primary '))
	                      and ($manifest.visual/@preferred = 'true'
	                           or (not($manifest.visual) and contains($role.tokens, ' visual-primary ')))"/>
	<!--
	  HTML can keep a large WKT output collapsed next to its interactive figure.
	  In the non-interactive PDF the preferred figure carries the same geometry,
	  while typesetting very large WKT collections can exceed TeX dimensions.
	  Authors can retain text in PDF with role="text-primary".
	-->
	<xsl:if test="not($visual.preferred)">
	  <xsl:call-template name="postgis-dblatex-block-label">
	    <xsl:with-param name="kind" select="'output'"/>
	  </xsl:call-template>
		<xsl:choose>
		  <xsl:when test="contains(., '┌') or contains(., '┐') or contains(., '└') or contains(., '┘') or contains(., '─') or contains(., '│')">
		    <xsl:text>&#10;\begingroup\lstset{basicstyle=\ttfamily\scriptsize}&#10;\begin{lstlisting}[language=text]&#10;</xsl:text>
		    <xsl:value-of select="translate(., '┌┐└┘├┤┬┴┼─│', '+++++++++-|')"/>
		    <xsl:text>&#10;\end{lstlisting}&#10;\endgroup&#10;</xsl:text>
		  </xsl:when>
		  <xsl:when test="(contains(@xml:id, 'st-buffer-example-') or contains(@id, 'st-buffer-example-')) and contains(., ',')">
		    <xsl:text>&#10;\begingroup\lstset{basicstyle=\ttfamily\scriptsize}&#10;</xsl:text>
		    <xsl:text>\noindent\ttfamily\scriptsize{}</xsl:text>
		    <xsl:call-template name="postgis-dblatex-breakable-geometry">
		      <xsl:with-param name="text" select="."/>
		    </xsl:call-template>
		    <xsl:text>\par&#10;</xsl:text>
		    <xsl:text>&#10;\endgroup&#10;</xsl:text>
		  </xsl:when>
		  <xsl:when test="contains($role.tokens, ' text-primary ') or string-length(normalize-space(.)) &gt; 600">
		    <xsl:text>&#10;\begingroup\lstset{basicstyle=\ttfamily\scriptsize}&#10;</xsl:text>
		    <xsl:apply-imports/>
		    <xsl:text>&#10;\endgroup&#10;</xsl:text>
		  </xsl:when>
		  <xsl:otherwise>
		    <xsl:apply-imports/>
		  </xsl:otherwise>
		</xsl:choose>
	</xsl:if>
	<xsl:if test="string($visual.id) != ''">
		<!-- Keep the visible Figure label attached to the generated image. -->
		<xsl:text>&#10;\par\noindent\begin{minipage}{\linewidth}&#10;</xsl:text>
		<xsl:call-template name="postgis-dblatex-block-label">
			<xsl:with-param name="kind" select="'figure'"/>
		</xsl:call-template>
		<xsl:text>&#10;\begin{center}\includegraphics[width=\linewidth,height=</xsl:text>
		<xsl:choose>
			<xsl:when test="$manifest.visual/@kind = 'image-output' and count($manifest.visual/layer) &gt; 4">0.70</xsl:when>
			<xsl:when test="$manifest.visual/@kind = 'image-output' and count($manifest.visual/layer) &gt; 1">0.58</xsl:when>
			<xsl:when test="$manifest.visual/@kind = 'image-output'">0.48</xsl:when>
			<xsl:when test="count($manifest.visual/layer[not(@frame = preceding-sibling::layer/@frame)]) &gt; 2">0.55</xsl:when>
			<xsl:otherwise>0.38</xsl:otherwise>
		</xsl:choose>
		<xsl:text>\textheight,keepaspectratio]{images/visual-examples/</xsl:text>
		<xsl:value-of select="$visual.id"/>
		<xsl:text>.png}\end{center}&#10;\end{minipage}\par&#10;</xsl:text>
	</xsl:if>
</xsl:template>

</xsl:stylesheet>
