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

	<!-- We deal only with the reference chapter -->
	<xsl:template match="/">
		<xsl:apply-templates select="/book/chapter[@id='reference']" />
	</xsl:template>

	<xsl:template match="chapter">
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

		<sect1 id="PostGIS_GeographyFunctions">
			<title>PostGIS Geography Support Functions</title>
			<para>The functions and operators given below are PostGIS functions/operators that take as input or return as output a <link linkend="PostGIS_Geography">geography</link> data type object.</para>
			<note><para>Functions with a (T) are not native geodetic functions, and use a ST_Transform call to and from geometry to do the operation.  As a result, they may not behave as expected when going over dateline, poles, 
				and for large geometries or geometry pairs that cover more than one UTM zone. Basic tranform - (favoring UTM, Lambert Azimuthal (North/South), and falling back on mercator in worst case scenario)</para></note>
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
					<xsl:variable name="refname">
						<xsl:value-of select="refnamediv/refname" />
					</xsl:variable>

			<!-- If at least one proto function accepts or returns a geography -->
					<xsl:choose>
						<xsl:when test="contains(refsynopsisdiv/funcsynopsis,'geography') or contains(refsynopsisdiv/funcsynopsis/funcprototype/funcdef,'geography')">
							<listitem><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="$comment" /></listitem>
						</xsl:when>
					</xsl:choose>
				</xsl:for-each>
				</itemizedlist>
		</sect1>
		
		<sect1 id="PostGIS_Geometry_DumpFunctions">
			<title>PostGIS Geometry Dump Functions</title>
			<para>The functions given below are PostGIS functions that take as input or return as output a set of or single <link linkend="geometry_dump">geometry_dump</link> data type object.</para>
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
					<xsl:variable name="refname">
						<xsl:value-of select="refnamediv/refname" />
					</xsl:variable>

			<!-- If at least one proto function accepts or returns a geography -->
					<xsl:choose>
						<xsl:when test="contains(refsynopsisdiv/funcsynopsis,'geometry_dump') or contains(refsynopsisdiv/funcsynopsis/funcprototype/funcdef,'geometry_dump')">
							<listitem><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="$comment" /></listitem>
						</xsl:when>
					</xsl:choose>
				</xsl:for-each>
				</itemizedlist>
		</sect1>
		
		<sect1 id="PostGIS_BoxFunctions">
			<title>PostGIS Box Functions</title>
			<para>The functions given below are PostGIS functions that take as input or return as output the box* family of PostGIS spatial types.
				The box family of types consists of <link linkend="box2d">box2d</link>, <link linkend="box3d">box3d</link>, <link linkend="box3d_extent">box3d_extent</link> </para>
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
					<xsl:variable name="refname">
						<xsl:value-of select="refnamediv/refname" />
					</xsl:variable>

			<!-- If at least one proto function accepts or returns a geography -->
					<xsl:choose>
						<xsl:when test="contains(refsynopsisdiv/funcsynopsis,'box') or contains(refsynopsisdiv/funcsynopsis/funcprototype/funcdef,'box')">
							<listitem><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="$comment" /></listitem>
						</xsl:when>
					</xsl:choose>
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

		<sect1 id="PostGIS_TypeFunctionMatrix">
			<title>PostGIS Function Support Matrix</title>

			<para>Below is an alphabetical listing of spatial specific functions in PostGIS and the kinds of spatial
				types they work with or OGC/SQL compliance they try to conform to.</para>
			<para>A + means the function works with the type or subtype natively.  A T means it works but has a transform cast built-in to 
				cast to geometry, transform and then cast back. A * means the function works with it, but only does because of auto-casting
					behavior.</para>
				
			<para>
				<table frame='all'>
					<tgroup cols='6' align='left' colsep='1' rowsep='1'>
						<colspec colname='function' />
						<colspec colname='geometry' align='center'/>
						<colspec colname='geography' align='center'/>
						<colspec colname='3D' align='center'/>
						<colspec colname='Curves' align='center'/>
						<colspec colname='SQLMM' align='center' />
						<thead>
						  <row>
							<entry>Function</entry>
							<entry>geometry</entry>
							<entry>geography</entry>
							<entry>3D (2.5D)</entry>
							<entry>Curves</entry>
							<entry>SQL MM</entry>
						  </row>
						</thead>
						<tbody>
						<!-- Exclude PostGIS types ,management functions, long transaction support, or exceptional functions from consideration  -->
						<!-- leaving out operators in an effor to try to fit on one page -->
						<xsl:for-each select="sect1[not(@id='PostGIS_Types' or @id='Management_Functions' or @id='Long_Transactions_Support' or @id='Exceptional_Functions' or @id='Operators')]/refentry">
							<xsl:sort select="@id"/>
							<xsl:variable name='comment'>
								<xsl:value-of select="normalize-space(translate(translate(refnamediv/refpurpose,'&#x0d;&#x0a;', ' '), '&#09;', ' '))"/>
							</xsl:variable>
							<xsl:variable name="refid">
								<xsl:value-of select="@id" />
							</xsl:variable>
							<xsl:variable name="refname">
								<xsl:value-of select="refnamediv/refname" />
							</xsl:variable>
			
							<row>
								<!-- Display name of function and link to it -->
								<entry><link linkend="{$refid}"><xsl:value-of select="$refname" /></link></entry>
								<!-- If at least one proto function accepts or returns a geometry -->
								<xsl:choose>
									<!-- direct support -->
									<xsl:when test="contains(refsynopsisdiv/funcsynopsis,'geometry') or contains(refsynopsisdiv/funcsynopsis/funcprototype/funcdef,'geometry')">
										<entry>+</entry>
									</xsl:when>
									<!-- support via autocast -->
									<xsl:when test="contains(refsynopsisdiv/funcsynopsis,'box') or contains(refsynopsisdiv/funcsynopsis/funcprototype/funcdef,'box')">
										<entry>*</entry>
									</xsl:when>
									<!-- no support -->
									<xsl:otherwise>
										<entry></entry>
									</xsl:otherwise>
								</xsl:choose>
								<!-- If at least one proto function accepts or returns a geography -->
								<xsl:choose>
									<!-- Support via geometry transform hack -->
									<xsl:when test="(contains(refsynopsisdiv/funcsynopsis,'geography') or contains(refsynopsisdiv/funcsynopsis/funcprototype/funcdef,'geography')) and contains($comment,'(T)')">
										<entry>T</entry>
									</xsl:when>
									<!-- direct support -->
									<xsl:when test="contains(refsynopsisdiv/funcsynopsis,'geography') or contains(refsynopsisdiv/funcsynopsis/funcprototype/funcdef,'geography')">
										<entry>+</entry>
									</xsl:when>
									<!-- no support -->
									<xsl:otherwise>
										<entry></entry>
									</xsl:otherwise>
								</xsl:choose>
								
								<!-- If at least one paragraph contains support 3d -->
								<xsl:choose>
									<!-- supports -->
									<xsl:when test="contains(.,'This function supports 3d')">
										<entry>+</entry>
									</xsl:when>
									<!-- no support -->
									<xsl:otherwise>
										<entry></entry>
									</xsl:otherwise>
								</xsl:choose>
								<!-- Support for Curve -->
								<xsl:choose>
									<!-- supports -->
									<xsl:when test="contains(.,'supports Circular Strings')">
										<entry>+</entry>
									</xsl:when>
									<!-- no support -->
									<xsl:otherwise>
										<entry></entry>
									</xsl:otherwise>
								</xsl:choose>	
								<!-- SQL MM compliance -->
								<xsl:choose>
									<!-- supports -->
									<xsl:when test="contains(.,'implements the SQL/MM')">
										<entry>+</entry>
									</xsl:when>
									<!-- no support -->
									<xsl:otherwise>
										<entry></entry>
									</xsl:otherwise>
								</xsl:choose>
							</row>
						</xsl:for-each>
						</tbody>
					</tgroup>
		</table>	
		</para>
	   </sect1>

		<sect1 id="NewFunctions">
			<title>New PostGIS Functions</title>
			<sect2 id="NewFunctions_1_5">
				<title>PostGIS Functions new in 1.5</title>
				<para>The functions given below are PostGIS functions that were introduced or enhanced in this major release.</para>
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
											<listitem><link linkend="{$refid}"><xsl:value-of select="$refid" /></link> - <xsl:value-of select="." /><xsl:text> </xsl:text> <xsl:value-of select="$comment" /> </listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
			</sect2>
			<sect2 id="NewFunctions_1_4">
				<title>PostGIS Functions new in 1.4</title>
				<para>The functions given below are PostGIS functions that were introduced or enhanced in the 1.4 release.</para>
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
								<xsl:for-each select="para|note">
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
