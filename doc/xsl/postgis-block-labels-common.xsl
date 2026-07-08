<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

<!--
  Shared labels for rendered DocBook code/output blocks and the HTML copy
  button. Keep this small so HTML and dblatex PDF customizations use the
  same strings without sharing output-specific templates.
-->
<xsl:param name="postgis.block.label.language" select="'en'"/>

<xsl:template name="postgis-localized-block-label">
  <xsl:param name="kind"/>
  <xsl:variable name="lang"
                select="translate($postgis.block.label.language,
                                  '-ABCDEFGHIJKLMNOPQRSTUVWXYZ',
                                  '_abcdefghijklmnopqrstuvwxyz')"/>

  <xsl:choose>
    <xsl:when test="$kind = 'copy'">
      <xsl:choose>
        <xsl:when test="$lang = 'be'">Капіяваць</xsl:when>
        <xsl:when test="$lang = 'br' or $lang = 'pt_br'">Copiar</xsl:when>
        <xsl:when test="$lang = 'da'">Kopiér</xsl:when>
        <xsl:when test="$lang = 'de'">Kopieren</xsl:when>
        <xsl:when test="$lang = 'es'">Copiar</xsl:when>
        <xsl:when test="$lang = 'fr'">Copier</xsl:when>
        <xsl:when test="$lang = 'it' or $lang = 'it_it'">Copia</xsl:when>
        <xsl:when test="$lang = 'ja'">コピー</xsl:when>
        <xsl:when test="$lang = 'ka'">კოპირება</xsl:when>
        <xsl:when test="$lang = 'ko' or $lang = 'ko_kr'">복사</xsl:when>
        <xsl:when test="$lang = 'pl'">Kopiuj</xsl:when>
        <xsl:when test="$lang = 'ro'">Copiază</xsl:when>
        <xsl:when test="$lang = 'ru'">Копировать</xsl:when>
        <xsl:when test="$lang = 'sv'">Kopiera</xsl:when>
        <xsl:when test="$lang = 'uk'">Копіювати</xsl:when>
        <xsl:when test="$lang = 'zh' or $lang = 'zh_hans'">复制</xsl:when>
        <xsl:otherwise>Copy</xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:when test="$kind = 'copied'">
      <xsl:choose>
        <xsl:when test="$lang = 'be'">Скапіявана</xsl:when>
        <xsl:when test="$lang = 'br' or $lang = 'pt_br'">Copiado</xsl:when>
        <xsl:when test="$lang = 'da'">Kopieret</xsl:when>
        <xsl:when test="$lang = 'de'">Kopiert</xsl:when>
        <xsl:when test="$lang = 'es'">Copiado</xsl:when>
        <xsl:when test="$lang = 'fr'">Copié</xsl:when>
        <xsl:when test="$lang = 'it' or $lang = 'it_it'">Copiato</xsl:when>
        <xsl:when test="$lang = 'ja'">コピー済み</xsl:when>
        <xsl:when test="$lang = 'ka'">კოპირებულია</xsl:when>
        <xsl:when test="$lang = 'ko' or $lang = 'ko_kr'">복사됨</xsl:when>
        <xsl:when test="$lang = 'pl'">Skopiowano</xsl:when>
        <xsl:when test="$lang = 'ro'">Copiat</xsl:when>
        <xsl:when test="$lang = 'ru'">Скопировано</xsl:when>
        <xsl:when test="$lang = 'sv'">Kopierat</xsl:when>
        <xsl:when test="$lang = 'uk'">Скопійовано</xsl:when>
        <xsl:when test="$lang = 'zh' or $lang = 'zh_hans'">已复制</xsl:when>
        <xsl:otherwise>Copied</xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:when test="$kind = 'copy-failed'">
      <xsl:choose>
        <xsl:when test="$lang = 'fr'">Échec</xsl:when>
        <xsl:when test="$lang = 'de'">Fehler</xsl:when>
        <xsl:when test="$lang = 'es'">Error</xsl:when>
        <xsl:when test="$lang = 'ru'">Ошибка</xsl:when>
        <xsl:when test="$lang = 'uk'">Помилка</xsl:when>
        <xsl:when test="$lang = 'zh' or $lang = 'zh_hans'">失败</xsl:when>
        <xsl:when test="$lang = 'ja'">失敗</xsl:when>
        <xsl:when test="$lang = 'ko' or $lang = 'ko_kr'">실패</xsl:when>
        <xsl:otherwise>Failed</xsl:otherwise>
      </xsl:choose>
    </xsl:when>
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

</xsl:stylesheet>
