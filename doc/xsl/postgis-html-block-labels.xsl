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
<xsl:param name="postgis.block.label.language" select="'en'"/>

<xsl:template name="postgis-localized-block-label">
  <xsl:param name="kind"/>
  <xsl:variable name="lang"
                select="translate($postgis.block.label.language,
                                  '-ABCDEFGHIJKLMNOPQRSTUVWXYZ',
                                  '_abcdefghijklmnopqrstuvwxyz')"/>

  <xsl:choose>
    <xsl:when test="$kind = 'output'">
      <xsl:choose>
        <xsl:when test="$lang = 'be'">Вывад</xsl:when>
        <xsl:when test="$lang = 'br' or $lang = 'pt_br'">Saída</xsl:when>
        <xsl:when test="$lang = 'da'">Uddata</xsl:when>
        <xsl:when test="$lang = 'de'">Ausgabe</xsl:when>
        <xsl:when test="$lang = 'es'">Salida</xsl:when>
        <xsl:when test="$lang = 'fr'">Sortie</xsl:when>
        <xsl:when test="$lang = 'it' or $lang = 'it_it'">Risultato</xsl:when>
        <xsl:when test="$lang = 'ja'">出力</xsl:when>
        <xsl:when test="$lang = 'ka'">შედეგი</xsl:when>
        <xsl:when test="$lang = 'ko' or $lang = 'ko_kr'">출력</xsl:when>
        <xsl:when test="$lang = 'pl'">Wynik</xsl:when>
        <xsl:when test="$lang = 'ro'">Ieșire</xsl:when>
        <xsl:when test="$lang = 'ru'">Вывод</xsl:when>
        <xsl:when test="$lang = 'sv'">Utdata</xsl:when>
        <xsl:when test="$lang = 'uk'">Вивід</xsl:when>
        <xsl:when test="$lang = 'zh' or $lang = 'zh_hans'">输出</xsl:when>
        <xsl:otherwise>Output</xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="$lang = 'be'">Код</xsl:when>
        <xsl:when test="$lang = 'br' or $lang = 'pt_br'">Código</xsl:when>
        <xsl:when test="$lang = 'da'">Kode</xsl:when>
        <xsl:when test="$lang = 'de'">Code</xsl:when>
        <xsl:when test="$lang = 'es'">Código</xsl:when>
        <xsl:when test="$lang = 'fr'">Code</xsl:when>
        <xsl:when test="$lang = 'it' or $lang = 'it_it'">Codice</xsl:when>
        <xsl:when test="$lang = 'ja'">コード</xsl:when>
        <xsl:when test="$lang = 'ka'">კოდი</xsl:when>
        <xsl:when test="$lang = 'ko' or $lang = 'ko_kr'">코드</xsl:when>
        <xsl:when test="$lang = 'pl'">Kod</xsl:when>
        <xsl:when test="$lang = 'ro'">Cod</xsl:when>
        <xsl:when test="$lang = 'ru'">Код</xsl:when>
        <xsl:when test="$lang = 'sv'">Kod</xsl:when>
        <xsl:when test="$lang = 'uk'">Код</xsl:when>
        <xsl:when test="$lang = 'zh' or $lang = 'zh_hans'">代码</xsl:when>
        <xsl:otherwise>Code</xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="d:programlisting">
  <xsl:param name="suppress-numbers" select="'0'"/>
  <xsl:variable name="label.id" select="concat('postgis-block-label-', generate-id())"/>

  <div class="postgis-example-block postgis-example-code" role="group" data-postgis-block="code">
    <xsl:attribute name="aria-labelledby">
      <xsl:value-of select="$label.id"/>
    </xsl:attribute>
    <div class="postgis-example-label">
      <xsl:attribute name="id">
        <xsl:value-of select="$label.id"/>
      </xsl:attribute>
      <xsl:call-template name="postgis-localized-block-label">
        <xsl:with-param name="kind" select="'code'"/>
      </xsl:call-template>
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
    <div class="postgis-example-label">
      <xsl:attribute name="id">
        <xsl:value-of select="$label.id"/>
      </xsl:attribute>
      <xsl:call-template name="postgis-localized-block-label">
        <xsl:with-param name="kind" select="'output'"/>
      </xsl:call-template>
    </div>
    <xsl:apply-imports/>
  </div>
</xsl:template>

</xsl:stylesheet>
