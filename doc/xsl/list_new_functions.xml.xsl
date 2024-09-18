<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:db="http://docbook.org/ns/docbook"
	exclude-result-prefixes="db"
>
<!-- ********************************************************************
	 ********************************************************************
	 Copyright 2010-2022, Regina Obe
	 License: BSD-3-Clause
	 Purpose: This is an xsl transform that generates file list_new_functions.xml which
	 includes what is new in each release
	 It uses xml reference sections from reference.xml to then be processed by docbook
	 ******************************************************************** -->
	<xsl:output method="xml" indent="yes" encoding="utf-8" />

	<!-- We deal only with the reference chapters -->
	<xsl:template match="/">
		<xsl:apply-templates select="/db:book/db:chapter[contains(@xml:id, 'reference')]" />
	</xsl:template>

	<xsl:template match="/">

		<xsl:variable name="chap" select="." />

		<!-- for each postgis-version { -->
		<xsl:for-each select="document('xsl-config.xml')//list_new_functions/postgis_versions/*">


			<xsl:variable name='ver'>
				<xsl:value-of select="." />
			</xsl:variable>

			<xsl:variable name='ver_id'>
				<xsl:value-of select="translate($ver,'.','_')" />
			</xsl:variable>

			<section>

				<xsl:variable name="header" select="document('xsl-config.xml')//list_new_functions/per_version_header" />

				<title>
				<xsl:value-of select="substring-before($header/title, '%')" />
				<xsl:value-of select="$ver" />
				<xsl:value-of select="substring-after($header/title, '%')" />
				</title>

				<para>
				<xsl:value-of select="$header/para" />
				</para>

				<!-- for each supported-tag { -->
				<xsl:for-each select="document('xsl-config.xml')//list_new_functions/supported_tags/*">

				<xsl:variable name='tag_node' select="." />
				<xsl:variable name='tag_para' select="string($tag_node/para)" />
				<xsl:variable name='tag_role' select="$tag_node/@role" />

				<!-- { -->
				<xsl:if test="$chap//db:para[@role=$tag_role and starts-with(./@conformance, $ver)]">

				<para>
				<xsl:value-of select="substring-before($tag_para, '%')" />
				<xsl:value-of select="$ver" />
				<xsl:value-of select="substring-after($tag_para, '%')" />
				</para>

				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry and strip whitespace and put in
						 a variable to be tagged unto each function comment	-->
					<xsl:for-each select="$chap//db:refentry">

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

						<!-- For each section if there is note about availability in this version -->
						<xsl:for-each select="db:refsection">
							<xsl:for-each select=".//db:para[@role=$tag_role and starts-with(./@conformance, $ver)]">
								<listitem>
									<simpara>
										<link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="." /><xsl:text> </xsl:text> <xsl:value-of select="$comment" />
									</simpara>
								</listitem>
							</xsl:for-each>
						</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
				</xsl:if>
				<!-- } -->

				</xsl:for-each>
				<!-- each supporte-tag } -->

			</section>

		</xsl:for-each>
		<!-- each postgis-version } -->

	</xsl:template>

</xsl:stylesheet>
