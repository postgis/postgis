<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version='2.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
	<xsl:output method="text" />
	<xsl:template match='/chapter'>
		<xsl:variable name="ap"><xsl:text>'</xsl:text></xsl:variable>
		<xsl:for-each select='sect1/refentry'>
		  <xsl:variable name='plaincomment'>
		  	<xsl:value-of select="refnamediv/refpurpose" />
		  </xsl:variable>
		  
			<xsl:variable name='comment'>
				<xsl:call-template name="globalReplace">
					<xsl:with-param name="outputString" select="$plaincomment"/>
					<xsl:with-param name="target" select="$ap"/>
					<xsl:with-param name="replacement" select="''"/>
				</xsl:call-template>
			</xsl:variable>

		<xsl:for-each select="refsynopsisdiv/funcsynopsis/funcprototype">
COMMENT ON <xsl:choose><xsl:when test="contains(paramdef/type,'geometry set')">AGGREGATE</xsl:when><xsl:otherwise>FUNCTION</xsl:otherwise></xsl:choose><xsl:text> </xsl:text> <xsl:value-of select="funcdef/function" />(<xsl:for-each select="paramdef"><xsl:choose><xsl:when test="count(parameter) &gt; 0"> 
<xsl:choose><xsl:when test="contains(type,'geometry set')">geometry</xsl:when><xsl:otherwise><xsl:value-of select="type" /></xsl:otherwise></xsl:choose><xsl:if test="position()&lt;last()"><xsl:text>, </xsl:text></xsl:if></xsl:when>
</xsl:choose></xsl:for-each>) IS '<xsl:call-template name="listparams"><xsl:with-param name="func" select="." /></xsl:call-template> <xsl:value-of select='$comment' />';
			</xsl:for-each>
		</xsl:for-each>
	</xsl:template>
	
<xsl:template name="globalReplace">
  <xsl:param name="outputString"/>
  <xsl:param name="target"/>
  <xsl:param name="replacement"/>
  <xsl:choose>
    <xsl:when test="contains($outputString,$target)">
      <xsl:value-of select=
        "concat(substring-before($outputString,$target),
               $replacement)"/>
      <xsl:call-template name="globalReplace">
        <xsl:with-param name="outputString" 
             select="substring-after($outputString,$target)"/>
        <xsl:with-param name="target" select="$target"/>
        <xsl:with-param name="replacement" 
             select="$replacement"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$outputString"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="listparams">
	<xsl:param name="func" />
	<xsl:for-each select="$func">
		<xsl:if test="count(paramdef/parameter) &gt; 0">args: </xsl:if>
		<xsl:for-each select="paramdef">
			<xsl:choose>
				<xsl:when test="count(parameter) &gt; 0"> 
					<xsl:value-of select="parameter" />
				</xsl:when>
			</xsl:choose>
			<xsl:if test="position()&lt;last()"><xsl:text>, </xsl:text></xsl:if>
		</xsl:for-each>
		<xsl:if test="count(paramdef/parameter) &gt; 0"> - </xsl:if>
	</xsl:for-each>	
</xsl:template>


</xsl:stylesheet>

