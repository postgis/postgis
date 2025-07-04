<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:db="http://docbook.org/ns/docbook"
	exclude-result-prefixes="db"
>
<!-- ********************************************************************
	 ********************************************************************
	 Copyright 2010-2022, Regina Obe
	 License: BSD-3-Clause
   Purpose: This is an xsl transform that generates file list_window_functions.xml.xsl which
   includes index listing of window functions.
   It uses xml reference sections from reference.xml to then be processed by docbook
	 ******************************************************************** -->
	<xsl:output method="xml" indent="yes" encoding="utf-8" />

	<!-- We deal only with the reference chapters -->
	<xsl:template match="/">
		<xsl:apply-templates select="/db:book/db:chapter[contains(@xml:id, 'reference')]" />
	</xsl:template>

	<xsl:template match="/">
            <xsl:if test="//db:funcprototype[contains(db:paramdef/db:type,' winset')]">
			<itemizedlist>
			<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
			<xsl:for-each select='//db:refentry'>
				<xsl:sort select="db:refnamediv/db:refname"/>
				<xsl:variable name='comment'>
					<xsl:value-of select="normalize-space(translate(translate(db:refnamediv/db:refpurpose,'&#x0d;&#x0a;', ' '), '&#09;', ' '))"/>
				</xsl:variable>
				<xsl:variable name="refid">
					<xsl:value-of select="@xml:id" />
				</xsl:variable>
				<xsl:variable name="refname">
					<xsl:value-of select="db:refnamediv/db:refname" />
				</xsl:variable>

			<!-- For each function prototype if it takes a geometry set then catalog it as an aggregate function  -->
				<xsl:for-each select="db:refsynopsisdiv/db:funcsynopsis/db:funcprototype">
					<xsl:choose>
						<xsl:when test="contains(db:paramdef/db:type,' winset')">
							 <listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="$comment" /></simpara></listitem>
						</xsl:when>
					</xsl:choose>
				</xsl:for-each>
			</xsl:for-each>
			</itemizedlist>
            </xsl:if>
	</xsl:template>

</xsl:stylesheet>
