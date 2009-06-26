<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!-- ********************************************************************
	 $Id$
	 ********************************************************************
	 Copyright 2008, Regina Obe
	 License: BSD
	 Purpose: This is an xsl transform that generates index listing of aggregate functions and mm /sql compliant functions xml section from reference_new.xml to then
	 be processed by doc book
	 ******************************************************************** -->
	<xsl:output method="xml" indent="yes" encoding="utf-8"/>
	<xsl:template match='/chapter'>
	<chapter>
		<title>PostGIS Special Functions Index</title>
		<sect1 id="PostGIS_Aggregate_Functions">
			<title>PostGIS Aggregate Functions</title>
			<para>The functions given below are spatial aggregate functions provided with PostGIS that can be used just like any other sql aggregate function such as sum, average.</para>
			<itemizedlist>
			<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
			<xsl:for-each select='sect1/refentry'>
				<xsl:sort select="@id"/>
				<xsl:variable name='comment'>
					<xsl:value-of select="normalize-space(translate(translate(refnamediv/refpurpose,'&#x0d;&#x0a;', ' '), '&#09;', ' '))"/>
				</xsl:variable>
				<xsl:variable name="refid">
					<xsl:value-of select="@id" />
				</xsl:variable>

			<!-- For each function prototype if it takes a geometry set then catalog it as an aggregate function  -->
				<xsl:for-each select="refsynopsisdiv/funcsynopsis/funcprototype">
					<xsl:choose>
						<xsl:when test="contains(paramdef/type,'geometry set')">
							 <listitem><link linkend="{$refid}"><xsl:value-of select="$refid" /></link> - <xsl:value-of select="$comment" /></listitem>
						</xsl:when>
					</xsl:choose>
				</xsl:for-each>
			</xsl:for-each>
			</itemizedlist>
		</sect1>

		<sect1 id="PostGIS_SQLMM_Functions">
			<title>PostGIS SQL-MM Compliant Functions</title>
			<para>The functions given below are PostGIS functions that conform to the SQL/MM 3 standard</para>
			<note>
			  <para>SQL-MM defines the default SRID of all geometry constructors as 0.
			  PostGIS uses a default SRID of -1.</para>
			</note>
				<itemizedlist>
			<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
				<xsl:for-each select='sect1/refentry'>
					<xsl:sort select="@id"/>
					<xsl:variable name='comment'>
						<xsl:value-of select="normalize-space(translate(translate(refnamediv/refpurpose,'&#x0d;&#x0a;', ' '), '&#09;', ' '))"/>
					</xsl:variable>
					<xsl:variable name="refid">
						<xsl:value-of select="@id" />
					</xsl:variable>

			<!-- For each section if there is note that it implements SQL/MM catalog it -->
						<xsl:for-each select="refsection">
							<xsl:for-each select="para">
								<xsl:choose>
									<xsl:when test="contains(.,'implements the SQL/MM')">
										<listitem><link linkend="{$refid}"><xsl:value-of select="$refid" /></link> - <xsl:value-of select="$comment" /> <xsl:value-of select="." /></listitem>
									</xsl:when>
								</xsl:choose>
							</xsl:for-each>
						</xsl:for-each>
				</xsl:for-each>
				</itemizedlist>
		</sect1>

		<sect1 id="PostGIS_3D_Functions">
			<title>PostGIS Functions that support 3D</title>
			<para>The functions given below are PostGIS functions that do not throw away the Z-Index.</para>
				<itemizedlist>
			<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
				<xsl:for-each select='sect1/refentry'>
					<xsl:sort select="@id"/>
					<xsl:variable name='comment'>
						<xsl:value-of select="normalize-space(translate(translate(refnamediv/refpurpose,'&#x0d;&#x0a;', ' '), '&#09;', ' '))"/>
					</xsl:variable>
					<xsl:variable name="refid">
						<xsl:value-of select="@id" />
					</xsl:variable>

			<!-- For each section if there is note that it supports 3d catalog it -->
						<xsl:for-each select="refsection">
							<xsl:for-each select="para">
								<xsl:choose>
									<xsl:when test="contains(.,'This function supports 3d')">
										<listitem><link linkend="{$refid}"><xsl:value-of select="$refid" /></link> - <xsl:value-of select="$comment" /></listitem>
									</xsl:when>
								</xsl:choose>
							</xsl:for-each>
						</xsl:for-each>
				</xsl:for-each>
				</itemizedlist>
		</sect1>

		<sect1 id="PostGIS_Curved_GeometryFunctions">
			<title>PostGIS Curved Geometry Support Functions</title>
			<para>The functions given below are PostGIS functions that can use CIRCULARSTRING, CURVEDPOLYGON, and other curved geometry types</para>
				<itemizedlist>
			<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
				<xsl:for-each select='sect1/refentry'>
					<xsl:sort select="@id"/>
					<xsl:variable name='comment'>
						<xsl:value-of select="normalize-space(translate(translate(refnamediv/refpurpose,'&#x0d;&#x0a;', ' '), '&#09;', ' '))"/>
					</xsl:variable>
					<xsl:variable name="refid">
						<xsl:value-of select="@id" />
					</xsl:variable>

			<!-- For each section if there is note that it implements Circular String catalog it -->
						<xsl:for-each select="refsection">
							<xsl:for-each select="para">
								<xsl:choose>
									<xsl:when test="contains(.,'supports Circular Strings')">
										<listitem><link linkend="{$refid}"><xsl:value-of select="$refid" /></link> - <xsl:value-of select="$comment" /></listitem>
									</xsl:when>
								</xsl:choose>
							</xsl:for-each>
						</xsl:for-each>
				</xsl:for-each>
				</itemizedlist>
		</sect1>

		<sect1 id="NewFunctions">
			<title>New PostGIS Functions</title>
			<sect2 id="NewFunctions_1_5">
				<title>PostGIS Functions new in 1.5</title>
				<para>The functions given below are PostGIS functions that were introduced in this major release.</para>
				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
					<xsl:for-each select='sect1/refentry'>
						<xsl:sort select="@id"/>
						<xsl:variable name='comment'>
							<xsl:value-of select="normalize-space(translate(translate(refnamediv/refpurpose,'&#x0d;&#x0a;', ' '), '&#09;', ' '))"/>
						</xsl:variable>
						<xsl:variable name="refid">
							<xsl:value-of select="@id" />
						</xsl:variable>
	
				<!-- For each section if there is note about availability in this version -->
							<xsl:for-each select="refsection">
								<xsl:for-each select="para">
									<xsl:choose>
										<xsl:when test="contains(.,'Availability: 1.5')">
											<listitem><link linkend="{$refid}"><xsl:value-of select="$refid" /></link> - <xsl:value-of select="$comment" /> <xsl:text> </xsl:text><xsl:value-of select="." /></listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
			</sect2>
			<sect2 id="NewFunctions_1_4">
				<title>PostGIS Functions new in 1.4</title>
				<para>The functions given below are PostGIS functions that were introduced in the 1.4 release.</para>
				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
					<xsl:for-each select='sect1/refentry'>
						<xsl:sort select="@id"/>
						<xsl:variable name='comment'>
							<xsl:value-of select="normalize-space(translate(translate(refnamediv/refpurpose,'&#x0d;&#x0a;', ' '), '&#09;', ' '))"/>
						</xsl:variable>
						<xsl:variable name="refid">
							<xsl:value-of select="@id" />
						</xsl:variable>
	
				<!-- For each section if there is note about availability in this version -->
							<xsl:for-each select="refsection">
								<xsl:for-each select="para">
									<xsl:choose>
										<xsl:when test="contains(.,'Availability: 1.4')">
											<listitem><link linkend="{$refid}"><xsl:value-of select="$refid" /></link> - <xsl:value-of select="$comment" /> <xsl:text> </xsl:text><xsl:value-of select="." /></listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
			</sect2>
			<sect2 id="NewFunctions_1_3">
				<title>PostGIS Functions new in 1.3</title>
				<para>The functions given below are PostGIS functions that were introduced in the 1.3 release.</para>
				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
				<xsl:for-each select='sect1/refentry'>
					<xsl:sort select="@id"/>
					<xsl:variable name='comment'>
						<xsl:value-of select="normalize-space(translate(translate(refnamediv/refpurpose,'&#x0d;&#x0a;', ' '), '&#09;', ' '))"/>
					</xsl:variable>
					<xsl:variable name="refid">
						<xsl:value-of select="@id" />
					</xsl:variable>

				<!-- For each section if there is note about availability in this version -->
						<xsl:for-each select="refsection">
							<xsl:for-each select="para">
								<xsl:choose>
									<xsl:when test="contains(.,'Availability: 1.3')">
										<listitem><link linkend="{$refid}"><xsl:value-of select="$refid" /></link> - <xsl:value-of select="$comment" /> <xsl:text> </xsl:text><xsl:value-of select="." /></listitem>
									</xsl:when>
								</xsl:choose>
							</xsl:for-each>
						</xsl:for-each>
				</xsl:for-each>
				</itemizedlist>
			</sect2>
		</sect1>

	</chapter>
	</xsl:template>

	<!--macro to pull out function parameter names so we can provide a pretty arg list prefix for each function -->
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
