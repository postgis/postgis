<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!-- ********************************************************************
	 ********************************************************************
	 Copyright 2010-2022, Regina Obe
	 License: BSD
	 Purpose: This is an xsl transform that generates file list_new_functions.xml which
	 includes what is new in each release
	 It uses xml reference sections from reference.xml to then be processed by docbook
	 ******************************************************************** -->
	<xsl:output method="xml" indent="yes" encoding="utf-8" />

	<!-- TODO: extract this from inspection of Availability/Changed/Enhanced comments -->
	<xsl:variable name="postgis-versions">
		<v>3.4</v>
		<v>3.3</v>
		<v>3.2</v>
		<v>3.1</v>
		<v>3.0</v>
		<v>2.5</v>
		<v>2.4</v>
		<v>2.3</v>
		<v>2.2</v>
		<v>2.1</v>
		<v>2.0</v>
		<v>1.5</v>
		<v>1.4</v>
		<v>1.3</v>
	</xsl:variable>

	<!-- TODO: read from a localized .xml, togheter with paragraphs to
			be translated, see #5414 -->
	<xsl:variable name="supported-tags">
		<v>Availability</v>
		<v>Enhanced</v>
		<v>Changed</v>
	</xsl:variable>


	<!-- We deal only with the reference chapter -->
	<xsl:template match="/">
		<xsl:apply-templates select="/book/chapter[@id='reference']" />
	</xsl:template>

	<xsl:template match="//chapter">

		<xsl:variable name="chap" select="." />

		<!-- for each postgis-version { -->
		<xsl:for-each select="document('')/*/xsl:variable[@name='postgis-versions']/*">

			<xsl:variable name='ver'>
				<xsl:value-of select="." />
			</xsl:variable>

			<xsl:variable name='ver_id'>
				<xsl:value-of select="translate($ver,'.','_')" />
			</xsl:variable>

			<sect2 id="NewFunctions_{$ver_id}">

				<!-- TODO: make title and parameter translatable -->
				<title>PostGIS Functions new or enhanced in <xsl:value-of select="$ver" /></title>
				<para>The functions given below are PostGIS functions that were added or enhanced.</para>

				<!-- for each supported-tag { -->
				<xsl:for-each select="document('')/*/xsl:variable[@name='supported-tags']/*">

				<xsl:variable name='tag'>
					<xsl:value-of select="." />
				</xsl:variable>

				<xsl:variable name='tag_verb'>
					<xsl:choose>
						<xsl:when test="$tag = 'Availability'">new</xsl:when>
						<xsl:when test="$tag = 'Enhanced'">enhanced</xsl:when>
						<xsl:when test="$tag = 'Changed'">changed</xsl:when>
					</xsl:choose>
				</xsl:variable>


				<!-- { -->
				<xsl:if test="$chap//para[contains(., concat(concat($tag, ': '), $ver))]">
				<!-- TODO: make next parameter translatable -->
				<para>Functions <xsl:value-of select="$tag_verb" /> in PostGIS <xsl:value-of select="$ver" /></para>
				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry and strip whitespace and put in
						 a variable to be tagged unto each function comment	-->
					<xsl:for-each select="$chap//refentry">
						<xsl:sort select="refnamediv/refname"/>
						<xsl:variable name='comment'>
							<xsl:value-of select="normalize-space(translate(translate(refnamediv/refpurpose,'&#x0d;&#x0a;', ' '), '&#09;', ' '))"/>
						</xsl:variable>
						<xsl:variable name="refid">
							<xsl:value-of select="@id" />
						</xsl:variable>

						<xsl:variable name="refname">
							<xsl:value-of select="refnamediv/refname" />
						</xsl:variable>

						<!-- For each section if there is note about availability in this version -->
						<xsl:for-each select="refsection">
							<xsl:for-each select=".//para">
								<xsl:choose>
									<xsl:when test="contains(., concat(concat($tag, ': '), $ver))">
										<listitem>
											<simpara>
												<link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="." /><xsl:text> </xsl:text> <xsl:value-of select="$comment" />
											</simpara>
										</listitem>
									</xsl:when>
								</xsl:choose>
							</xsl:for-each>
						</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
				</xsl:if>
				<!-- } -->

				</xsl:for-each>
				<!-- each supporte-tag } -->

			</sect2>

		</xsl:for-each>
		<!-- each postgis-version } -->

	</xsl:template>

</xsl:stylesheet>
