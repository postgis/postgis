<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!-- ********************************************************************
	 ********************************************************************
	 Copyright 2010, Regina Obe
	 License: BSD
	 Purpose: This is an xsl transform that generates index listing of aggregate functions and mm /sql compliant functions xml section from reference_new.xml to then
	 be processed by doc book
	 ******************************************************************** -->
	<xsl:output method="xml" indent="yes" encoding="utf-8" />

	<!-- We deal only with the reference chapter -->
	<xsl:template match="/">
		<xsl:apply-templates select="/book/chapter[@id='reference']" />
	</xsl:template>

	<xsl:template match="//chapter">
	<chapter id="PostGIS_Special_Functions_Index">
		<title>PostGIS Special Functions Index</title>
		<sect1 id="PostGIS_Aggregate_Functions">
			<title>PostGIS Aggregate Functions</title>
			<para>The functions given below are spatial aggregate functions provided with PostGIS that can be used just like any other sql aggregate function such as sum, average.</para>
			<itemizedlist>
			<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
			<xsl:for-each select='//refentry'>
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

			<!-- For each function prototype if it takes a geometry set then catalog it as an aggregate function  -->
				<xsl:for-each select="refsynopsisdiv/funcsynopsis/funcprototype">
					<xsl:choose>
						<xsl:when test="contains(paramdef/type,' set') or contains(paramdef/type,'geography set') or contains(paramdef/type,'raster set')">
							 <listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="$comment" /></simpara></listitem>
						</xsl:when>
					</xsl:choose>
				</xsl:for-each>
			</xsl:for-each>
			</itemizedlist>
		</sect1>

		<sect1 id="PostGIS_Window_Functions">
			<title>PostGIS Window Functions</title>
			<para>The functions given below are spatial window functions provided with PostGIS that can be used just like any other sql window function such as row_numer(), lead(), lag(). All these require an SQL OVER() clause.</para>
            <xsl:if test="//funcprototype[contains(paramdef/type,' winset')]">
			<itemizedlist>
			<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
			<xsl:for-each select='//refentry'>
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

			<!-- For each function prototype if it takes a geometry set then catalog it as an aggregate function  -->
				<xsl:for-each select="refsynopsisdiv/funcsynopsis/funcprototype">
					<xsl:choose>
						<xsl:when test="contains(paramdef/type,' winset')">
							 <listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="$comment" /></simpara></listitem>
						</xsl:when>
					</xsl:choose>
				</xsl:for-each>
			</xsl:for-each>
			</itemizedlist>
            </xsl:if>
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
				<xsl:for-each select='//refentry'>
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
										<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refid" /></link> - <xsl:value-of select="$comment" /> <xsl:value-of select="." /></simpara></listitem>
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
				and for large geometries or geometry pairs that cover more than one UTM zone. Basic transform - (favoring UTM, Lambert Azimuthal (North/South), and falling back on mercator in worst case scenario)</para></note>
				<itemizedlist>
			<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
				<xsl:for-each select='//refentry'>
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
							<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="$comment" /></simpara></listitem>
						</xsl:when>
					</xsl:choose>
				</xsl:for-each>
				</itemizedlist>
		</sect1>

		<sect1 id="PostGIS_RasterFunctions">
			<title>PostGIS Raster Support Functions</title>
			<para>The functions and operators given below are PostGIS functions/operators that take as input or return as output a <xref linkend="raster" /> data type object. Listed
			in alphabetical order.</para>
				<itemizedlist>
			<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
				<xsl:for-each select='//refentry'>
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
						<xsl:when test="contains(refsynopsisdiv/funcsynopsis,'raster') or contains(refsynopsisdiv/funcsynopsis/funcprototype/funcdef,'raster')">
							<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="$comment" /></simpara></listitem>
						</xsl:when>
					</xsl:choose>
				</xsl:for-each>
				</itemizedlist>
		</sect1>


		<sect1 id="PostGIS_Geometry_DumpFunctions">
			<title>PostGIS Geometry / Geography / Raster Dump Functions</title>
			<para>The functions given below are PostGIS functions that take as input or return as output a set of or single <link linkend="geometry_dump">geometry_dump</link> or  <link linkend="geomval">geomval</link> data type object.</para>
				<itemizedlist>
			<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
				<xsl:for-each select='//refentry'>
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
						<xsl:when test="contains(refsynopsisdiv/funcsynopsis,'geometry_dump') or contains(refsynopsisdiv/funcsynopsis/funcprototype/funcdef,'geometry_dump') or contains(refsynopsisdiv/funcsynopsis/funcprototype/funcdef,'geomval')">
							<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="$comment" /></simpara></listitem>
						</xsl:when>
					</xsl:choose>
				</xsl:for-each>
				</itemizedlist>
		</sect1>

		<sect1 id="PostGIS_BoxFunctions">
			<title>PostGIS Box Functions</title>
			<para>The functions given below are PostGIS functions that take as input or return as output the box* family of PostGIS spatial types.
				The box family of types consists of <link linkend="box2d_type">box2d</link>, and <link linkend="box3d_type">box3d</link></para>
				<itemizedlist>
			<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
				<xsl:for-each select='//refentry'>
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
							<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="$comment" /></simpara></listitem>
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
				<xsl:for-each select='//refentry'>
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
										<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refid" /></link> - <xsl:value-of select="$comment" /></simpara></listitem>
									</xsl:when>
								</xsl:choose>
							</xsl:for-each>
						</xsl:for-each>
				</xsl:for-each>
				</itemizedlist>
		</sect1>

		<sect1 id="PostGIS_Curved_GeometryFunctions">
			<title>PostGIS Curved Geometry Support Functions</title>
			<para>The functions given below are PostGIS functions that can use CIRCULARSTRING, CURVEPOLYGON, and other curved geometry types</para>
				<itemizedlist>
			<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
				<xsl:for-each select='//refentry'>
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

			<!-- For each section if there is note that it implements Circular String catalog it -->
						<xsl:for-each select="refsection">
							<xsl:for-each select="para">
								<xsl:choose>
									<xsl:when test="contains(.,'supports Circular Strings')">
										<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="$comment" /></simpara></listitem>
									</xsl:when>
								</xsl:choose>
							</xsl:for-each>
						</xsl:for-each>
				</xsl:for-each>
				</itemizedlist>
		</sect1>

		<sect1 id="PostGIS_PS_GeometryFunctions">
			<title>PostGIS Polyhedral Surface Support Functions</title>
			<para>The functions given below are PostGIS functions that can use POLYHEDRALSURFACE, POLYHEDRALSURFACEM geometries</para>
				<itemizedlist>
			<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
				<xsl:for-each select='//refentry'>
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

			<!-- For each section if there is note that it supports Polyhedral surfaces catalog it -->
						<xsl:for-each select="refsection">
							<xsl:for-each select="para">
								<xsl:choose>
									<xsl:when test="contains(.,'supports Polyhedral')">
										<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="$comment" /></simpara></listitem>
									</xsl:when>
								</xsl:choose>
							</xsl:for-each>
						</xsl:for-each>
				</xsl:for-each>
				</itemizedlist>
		</sect1>

		<sect1 id="PostGIS_TypeFunctionMatrix">
			<xsl:variable name='matrix_checkmark'><![CDATA[<inlinemediaobject><imageobject><imagedata fileref='images/matrix_checkmark.png' /></imageobject></inlinemediaobject>]]></xsl:variable>
			<xsl:variable name='matrix_transform'><![CDATA[<inlinemediaobject><imageobject><imagedata fileref='images/matrix_transform.png' /></imageobject></inlinemediaobject>]]></xsl:variable>
			<xsl:variable name='matrix_autocast'><![CDATA[<inlinemediaobject><imageobject><imagedata fileref='images/matrix_autocast.png' /></imageobject></inlinemediaobject>]]></xsl:variable>
			<xsl:variable name='matrix_sfcgal_required'><![CDATA[<inlinemediaobject><imageobject><imagedata fileref='images/matrix_sfcgal_required.png' /></imageobject></inlinemediaobject>]]></xsl:variable>
			<xsl:variable name='matrix_sfcgal_enhanced'><![CDATA[<inlinemediaobject><imageobject><imagedata fileref='images/matrix_sfcgal_enhanced.png' /></imageobject></inlinemediaobject>]]></xsl:variable>

			<title>PostGIS Function Support Matrix</title>

			<para>Below is an alphabetical listing of spatial specific functions in PostGIS and the kinds of spatial
				types they work with or OGC/SQL compliance they try to conform to.</para>
			<para><itemizedlist>
				<listitem><simpara>A <xsl:value-of select="$matrix_checkmark" disable-output-escaping="yes"/> means the function works with the type or subtype natively.</simpara></listitem>
				<listitem><simpara>A <xsl:value-of select="$matrix_transform" disable-output-escaping="yes"/> means it works but with a transform cast built-in using cast to geometry, transform to a "best srid" spatial ref and then cast back. Results may not be as expected for large areas or areas at poles
						and may accumulate floating point junk.</simpara></listitem>
				<listitem><simpara>A <xsl:value-of select="$matrix_autocast" disable-output-escaping="yes"/> means the function works with the type because of a auto-cast to another such as to box3d rather than direct type support.</simpara></listitem>
				<listitem><simpara>A <xsl:value-of select="$matrix_sfcgal_required" disable-output-escaping="yes"/> means the function only available if PostGIS compiled with SFCGAL support.</simpara></listitem>
				<listitem><simpara>A <xsl:value-of select="$matrix_sfcgal_enhanced" disable-output-escaping="yes"/> means the function support is provided by SFCGAL if PostGIS compiled with SFCGAL support, otherwise GEOS/built-in support.</simpara></listitem>
				<listitem><simpara>geom - Basic 2D geometry support (x,y).</simpara></listitem>
				<listitem><simpara>geog - Basic 2D geography support (x,y).</simpara></listitem>
				<listitem><simpara>2.5D - basic 2D geometries in 3 D/4D space (has Z or M coord).</simpara></listitem>
				<listitem><simpara>PS - Polyhedral surfaces</simpara></listitem>
				<listitem><simpara>T - Triangles and Triangulated Irregular Network surfaces (TIN)</simpara></listitem>
				</itemizedlist>
			</para>

			<para>
				<informaltable frame='all'>
					<tgroup cols='8' align='left' colsep='1' rowsep='1'>
						<colspec colname='function' align='left'/>
						<colspec colname='geometry' align='center'/>
						<colspec colname='geography' align='center'/>
						<colspec colname='25D' align='center'/>
						<colspec colname='Curves' align='center'/>
						<colspec colname='SQLMM' align='center' />
						<colspec colname='PS' align='center' />
						<colspec colname='T' align='center' />
						<thead>
						  <row>
							<entry>Function</entry>
							<entry>geom</entry>
							<entry>geog</entry>
							<entry>2.5D</entry>
							<entry>Curves</entry>
							<entry>SQL MM</entry>
							<entry>PS</entry>
							<entry>T</entry>
						  </row>
						</thead>
						<tbody>
						<!-- Exclude PostGIS types, management functions, long transaction support, or exceptional functions from consideration  -->
						<!-- leaving out operators in an effor to try to fit on one page -->
						<xsl:for-each select="sect1[not(@id='PostGIS_Types' or @id='Management_Functions' or @id='Long_Transactions_Support' or @id='Exceptional_Functions')]/refentry">
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
										<entry><xsl:choose><xsl:when test="contains(.,'needs SFCGAL')"><xsl:value-of select="$matrix_sfcgal_required" disable-output-escaping="yes"/></xsl:when><xsl:otherwise><xsl:value-of select="$matrix_checkmark" disable-output-escaping="yes"/></xsl:otherwise></xsl:choose></entry>
									</xsl:when>
									<!-- support via autocast -->
									<xsl:when test="contains(refsynopsisdiv/funcsynopsis,'box') or contains(refsynopsisdiv/funcsynopsis/funcprototype/funcdef,'box')">
										<entry><xsl:value-of select="$matrix_autocast" disable-output-escaping="yes"/></entry>
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
										<entry><xsl:value-of select="$matrix_transform" disable-output-escaping="yes"/></entry>
									</xsl:when>
									<!-- direct support -->
									<xsl:when test="contains(refsynopsisdiv/funcsynopsis,'geography') or contains(refsynopsisdiv/funcsynopsis/funcprototype/funcdef,'geography')">
										<entry><xsl:value-of select="$matrix_checkmark" disable-output-escaping="yes"/></entry>
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
										<!-- if 3d denote if it needs sfcgal -->
										<entry><xsl:choose><xsl:when test="contains(.,'needs SFCGAL')"><xsl:value-of select="$matrix_sfcgal_required" disable-output-escaping="yes"/></xsl:when>
										<xsl:when test="contains(.,'provided by SFCGAL')"><xsl:value-of select="$matrix_sfcgal_enhanced" disable-output-escaping="yes"/></xsl:when>
										<xsl:otherwise><xsl:value-of select="$matrix_checkmark" disable-output-escaping="yes"/></xsl:otherwise></xsl:choose></entry>
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
										<entry><xsl:value-of select="$matrix_checkmark" disable-output-escaping="yes"/></entry>
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
										<entry><xsl:choose><xsl:when test="contains(.,'needs SFCGAL')"><xsl:value-of select="$matrix_sfcgal_required" disable-output-escaping="yes"/></xsl:when>
										<xsl:when test="contains(.,'provided by SFCGAL')"><xsl:value-of select="$matrix_sfcgal_enhanced" disable-output-escaping="yes"/></xsl:when>
										<xsl:otherwise><xsl:value-of select="$matrix_checkmark" disable-output-escaping="yes"/></xsl:otherwise></xsl:choose></entry>
									</xsl:when>
									<!-- no support -->
									<xsl:otherwise>
										<entry></entry>
									</xsl:otherwise>
								</xsl:choose>
							<!-- Polyhedral surface support -->
								<xsl:choose>
									<!-- supports -->
									<xsl:when test="contains(.,'Polyhedral')">
										<entry><xsl:choose><xsl:when test="contains(.,'needs SFCGAL')"><xsl:value-of select="$matrix_sfcgal_required" disable-output-escaping="yes"/></xsl:when>
										<xsl:when test="contains(.,'provided by SFCGAL')"><xsl:value-of select="$matrix_sfcgal_enhanced" disable-output-escaping="yes"/></xsl:when>
										<xsl:otherwise><xsl:value-of select="$matrix_checkmark" disable-output-escaping="yes"/></xsl:otherwise></xsl:choose></entry>
									</xsl:when>
									<!-- no support -->
									<xsl:otherwise>
										<entry></entry>
									</xsl:otherwise>
								</xsl:choose>
							<!-- Triangle and TIN surface support -->
								<xsl:choose>
									<!-- supports -->
									<xsl:when test="contains(.,'Triang')">
										<entry><xsl:choose><xsl:when test="contains(.,'needs SFCGAL')"><xsl:value-of select="$matrix_sfcgal_required" disable-output-escaping="yes"/></xsl:when>
										<xsl:when test="contains(.,'provided by SFCGAL')"><xsl:value-of select="$matrix_sfcgal_enhanced" disable-output-escaping="yes"/></xsl:when>
										<xsl:otherwise><xsl:value-of select="$matrix_checkmark" disable-output-escaping="yes"/></xsl:otherwise></xsl:choose></entry>
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
		</informaltable>
		</para>
	   </sect1>

	   <sect1 id="NewFunctions">
			<title>New, Enhanced or changed PostGIS Functions</title>

            <sect2 id="NewFunctions_2_4">
				<title>PostGIS Functions new or enhanced in 2.4</title>
				<para>The functions given below are PostGIS functions that were added or enhanced.</para>
                <xsl:if test="//para[contains(text(),'Availability: 2.4')]">
				<para>Functions new in PostGIS 2.4</para>
				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
					<xsl:for-each select='//refentry'>
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
								<xsl:for-each select="para | */para">
									<xsl:choose>
										<xsl:when test="contains(.,'Availability: 2.4')">
											<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="." /><xsl:text> </xsl:text> <xsl:value-of select="$comment" /></simpara></listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
                </xsl:if>

				<xsl:if test="//para[contains(text(),'Enhanced: 2.4')]">
				<para>Functions enhanced in PostGIS 2.4</para>
				<para>All aggregates now marked as parallel safe which should allow them to be used in plans that can employ parallelism.</para>
				<para>PostGIS 2.4.1 postgis_tiger_geocoder set to load Tiger 2017 data. Can optionally load zip code 5-digit tabulation (zcta) as part of the <xref linkend="Loader_Generate_Nation_Script" />.</para>
				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
					<xsl:for-each select='//refentry'>
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
								<xsl:for-each select="para | */para">
									<xsl:choose>
										<xsl:when test="contains(.,'Enhanced: 2.4')">
											<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="." /><xsl:text> </xsl:text> <xsl:value-of select="$comment" /></simpara></listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
                </xsl:if>


				<para>Functions changed in PostGIS 2.4</para>
				<para>All PostGIS aggregates now marked as parallel safe.
				This will force a drop and recreate of aggregates during upgrade which may fail if any user views or sql functions rely on PostGIS aggregates.</para>
				<xsl:if test="//para[contains(text(),'Changed: 2.4')]">
				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
					<xsl:for-each select='//refentry'>
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
								<xsl:for-each select="para | */para">
									<xsl:choose>
										<xsl:when test="contains(.,'Changed: 2.4')">
											<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="." /><xsl:text> </xsl:text> <xsl:value-of select="$comment" /></simpara></listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
                </xsl:if>

			</sect2>

			<sect2 id="NewFunctions_2_3">
				<title>PostGIS Functions new or enhanced in 2.3</title>
				<para>The functions given below are PostGIS functions that were added or enhanced.</para>
                <note><para>PostGIS 2.3.0: PostgreSQL 9.6+ support for parallel queries.</para></note>
                <note><para>PostGIS 2.3.0: PostGIS extension, all functions schema qualified to reduce issues in database restore.</para></note>
                <note><para>PostGIS 2.3.0: PostgreSQL 9.4+ support for BRIN indexes. Refer to <xref linkend="brin_indexes" />.</para></note>
                <note><para>PostGIS 2.3.0: Tiger Geocoder upgraded to work with TIGER 2016 data.</para></note>
                <xsl:if test="//para[contains(text(),'Availability: 2.3')]">
				<para>Functions new in PostGIS 2.3</para>
				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
					<xsl:for-each select='//refentry'>
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
								<xsl:for-each select="para | */para">
									<xsl:choose>
										<xsl:when test="contains(.,'Availability: 2.3')">
											<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="." /><xsl:text> </xsl:text> <xsl:value-of select="$comment" /></simpara></listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
                </xsl:if>

                <xsl:if test="//para[contains(text(),'Enhanced: 2.3')]">
				<para>The functions given below are PostGIS functions that are enhanced in PostGIS 2.3.</para>
				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry   -->
					<xsl:for-each select='//refentry'>
						<xsl:sort select="@id"/>
						<xsl:variable name="refid">
							<xsl:value-of select="@id" />
						</xsl:variable>

						<xsl:variable name="refname">
							<xsl:value-of select="refnamediv/refname" />
						</xsl:variable>
				<!-- For each section if there is note about enhanced in this version -->
							<xsl:for-each select="refsection">
								<xsl:for-each select="para | */para">
									<xsl:choose>
										<xsl:when test="contains(.,'Enhanced: 2.3')">
											<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="." /></simpara></listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
                </xsl:if>

			</sect2>

			<sect2 id="NewFunctions_2_2">
				<title>PostGIS Functions new or enhanced in 2.2</title>
				<para>The functions given below are PostGIS functions that were added or enhanced.</para>

				<note><para>postgis_sfcgal now can be installed as an extension using CREATE EXTENSION postgis_sfcgal;</para></note>
				<note><para>PostGIS 2.2.0: Tiger Geocoder upgraded to work with TIGER 2015 data.</para></note>
				<note><para>address_standardizer, address_standardizer_data_us extensions for standardizing address data refer to <xref linkend="Address_Standardizer" /> for details.</para></note>
				<note><para>Many functions in topology rewritten as C functions for increased performance.</para></note>
				<xsl:if test="//para[contains(text(),'Availability: 2.2')]">
				<para>Functions new in PostGIS 2.2</para>
				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
					<xsl:for-each select='//refentry'>
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
								<xsl:for-each select="para | */para">
									<xsl:choose>
										<xsl:when test="contains(.,'Availability: 2.2')">
											<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="." /><xsl:text> </xsl:text> <xsl:value-of select="$comment" /></simpara></listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
				</xsl:if>

				 <xsl:if test="//para[contains(text(),'Enhanced: 2.2')]">
				<para>The functions given below are PostGIS functions that are enhanced in PostGIS 2.2.</para>
				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry   -->
					<xsl:for-each select='//refentry'>
						<xsl:sort select="@id"/>
						<xsl:variable name="refid">
							<xsl:value-of select="@id" />
						</xsl:variable>

						<xsl:variable name="refname">
							<xsl:value-of select="refnamediv/refname" />
						</xsl:variable>
				<!-- For each section if there is note about enhanced in this version -->
							<xsl:for-each select="refsection">
								<xsl:for-each select="para | */para">
									<xsl:choose>
										<xsl:when test="contains(.,'Enhanced: 2.2')">
											<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="." /></simpara></listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
				</xsl:if>
			</sect2>

			<xsl:if test="//para[contains(text(),'Changed: 2.2')]">
		 	<sect2 id="ChangedFunctions_2_2"><title>PostGIS functions breaking changes in 2.2</title>
				<para>The functions given below are PostGIS functions that have possibly breaking changes in PostGIS 2.2.  If you use any of these, you may need to check your existing code.</para>
				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry   -->
					<xsl:for-each select='//refentry'>
						<xsl:sort select="@id"/>
						<xsl:variable name="refid">
							<xsl:value-of select="@id" />
						</xsl:variable>

						<xsl:variable name="refname">
							<xsl:value-of select="refnamediv/refname" />
						</xsl:variable>
				<!-- For each section if there is note about enhanced in this version -->
							<xsl:for-each select="refsection">
								<xsl:for-each select="para | */para">
									<xsl:choose>
										<xsl:when test="contains(.,'Changed: 2.2')">
											<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="." /></simpara></listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
			</sect2>
			</xsl:if>


			<sect2 id="NewFunctions_2_1">
				<title>PostGIS Functions new or enhanced in 2.1</title>
				<para>The functions given below are PostGIS functions that were added or enhanced.</para>

				<note><para>More Topology performance Improvements.  Please refer to <xref linkend="Topology" /> for more details.</para></note>
				<note><para>Bug fixes (particularly with handling of out-of-band rasters), many new functions (often shortening code you have to write to accomplish a common task) and massive speed improvements to raster functionality. Refer to <xref linkend="RT_reference" /> for more details. </para></note>
				<note><para>PostGIS 2.1.0: Tiger Geocoder upgraded to work with TIGER 2012 census data. <varname>geocode_settings</varname> added for debugging and tweaking rating preferences, loader made less greedy, now only downloads tables to be loaded. PostGIS 2.1.1: Tiger Geocoder upgraded to work with TIGER 2013 data.
					Please refer to <xref linkend="Tiger_Geocoder" /> for more details.</para></note>

				<xsl:if test="//para[contains(text(),'Availability: 2.1')]">
				<para>Functions new in PostGIS 2.1</para>
				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
					<xsl:for-each select='//refentry'>
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
								<xsl:for-each select="para | */para">
									<xsl:choose>
										<xsl:when test="contains(.,'Availability: 2.1')">
											<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="." /><xsl:text> </xsl:text> <xsl:value-of select="$comment" /></simpara></listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
				</xsl:if>

				<xsl:if test="//para[contains(text(),'Enhanced: 2.1')]">
				<para>The functions given below are PostGIS functions that are enhanced in PostGIS 2.1.</para>
				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry   -->
					<xsl:for-each select='//refentry'>
						<xsl:sort select="@id"/>
						<xsl:variable name="refid">
							<xsl:value-of select="@id" />
						</xsl:variable>

						<xsl:variable name="refname">
							<xsl:value-of select="refnamediv/refname" />
						</xsl:variable>
				<!-- For each section if there is note about enhanced in this version -->
							<xsl:for-each select="refsection">
								<xsl:for-each select="para | */para">
									<xsl:choose>
										<xsl:when test="contains(.,'Enhanced: 2.1')">
											<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="." /></simpara></listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
				</xsl:if>

			</sect2>

			<xsl:if test="//para[contains(text(),'Changed: 2.1')]">
			<sect2 id="ChangedFunctions_2_1"><title>PostGIS functions breaking changes in 2.1</title>
				<para>The functions given below are PostGIS functions that have possibly breaking changes in PostGIS 2.1.  If you use any of these, you may need to check your existing code.</para>
				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry   -->
					<xsl:for-each select='//refentry'>
						<xsl:sort select="@id"/>
						<xsl:variable name="refid">
							<xsl:value-of select="@id" />
						</xsl:variable>

						<xsl:variable name="refname">
							<xsl:value-of select="refnamediv/refname" />
						</xsl:variable>
				<!-- For each section if there is note about enhanced in this version -->
							<xsl:for-each select="refsection">
								<xsl:for-each select="para | */para">
									<xsl:choose>
										<xsl:when test="contains(.,'Changed: 2.1')">
											<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="." /></simpara></listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
			</sect2>
			</xsl:if>


			<sect2 id="NewFunctions_2_0">
				<title>PostGIS Functions new, behavior changed, or enhanced in 2.0</title>
				<para>The functions given below are PostGIS functions that were added, enhanced, or have <xref linkend="NewFunctions_2_0_Changed" /> breaking changes in 2.0 releases.</para>
				<para>New geometry types: TIN and Polyhedral surfaces was introduced in 2.0</para>
				<note><para>Greatly improved support for Topology.  Please refer to <xref linkend="Topology" /> for more details.</para></note>
				<note><para>In PostGIS 2.0, raster type and raster functionality has been integrated.  There are way too many new raster functions to list here and all are new so
					please refer to <xref linkend="RT_reference" /> for more details of the raster functions available. Earlier pre-2.0 versions had raster_columns/raster_overviews as real tables. These were changed to views before release.  Functions such as <varname>ST_AddRasterColumn</varname> were removed and replaced with <xref linkend="RT_AddRasterConstraints"/>, <xref linkend="RT_DropRasterConstraints"/> as a result some apps that created raster tables may need changing.</para></note>
				<note><para>Tiger Geocoder upgraded to work with TIGER 2010 census data and now included in the core PostGIS documentation.  A reverse geocoder function was also added.
					Please refer to <xref linkend="Tiger_Geocoder" /> for more details.</para></note>

				<xsl:if test="//para[contains(text(),'Availability: 2.0')]">
				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
					<xsl:for-each select='//refentry'>
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
								<xsl:for-each select="para | */para">
									<xsl:choose>
										<xsl:when test="contains(.,'Availability: 2.0')">
											<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="." /><xsl:text> </xsl:text> <xsl:value-of select="$comment" /></simpara></listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
				</xsl:if>

				<xsl:if test="//para[contains(text(),'Enhanced: 2.0')]">
				<para>The functions given below are PostGIS functions that are enhanced in PostGIS 2.0.</para>
				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry   -->
					<xsl:for-each select='//refentry'>
						<xsl:sort select="@id"/>
						<xsl:variable name="refid">
							<xsl:value-of select="@id" />
						</xsl:variable>

						<xsl:variable name="refname">
							<xsl:value-of select="refnamediv/refname" />
						</xsl:variable>
				<!-- For each section if there is note about enhanced in this version -->
							<xsl:for-each select="refsection">
								<xsl:for-each select="para | */para">
									<xsl:choose>
										<xsl:when test="contains(.,'Enhanced: 2.0')">
											<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="." /></simpara></listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
				</xsl:if>
			</sect2>

			<sect2 id="NewFunctions_2_0_Changed">
			       <title>PostGIS Functions changed behavior in 2.0</title>
			       <para>The functions given below are PostGIS functions that have changed behavior in PostGIS 2.0 and may require application changes.</para>
                    <note><para>Most deprecated functions have been removed.  These are functions that haven't been documented since 1.2
                        or some internal functions that were never documented.  If you are using a function that you don't see documented,
                        it's probably deprecated, about to be deprecated, or internal and should be avoided.  If you have applications or tools
                        that rely on deprecated functions, please refer to <xref linkend="legacy_faq" /> for more details.</para></note>
                    <note><para>Bounding boxes of geometries have been changed from float4 to double precision (float8).  This has an impact
                    	on answers you get using bounding box operators and casting of bounding boxes to geometries. E.g ST_SetSRID(abbox) will
                    	often return a different more accurate answer in PostGIS 2.0+ than it did in prior versions which may very well slightly
                    	change answers to view port queries.</para></note>
                    <note><para>The arguments hasnodata was replaced with exclude_nodata_value which has the same meaning as the older hasnodata but clearer in purpose.</para></note>

					<xsl:if test="//para[contains(text(),'Changed: 2.0')]">
                    <itemizedlist>
                    <!-- Pull out the purpose section for each ref entry   -->
                        <xsl:for-each select='//refentry'>
                            <xsl:sort select="@id"/>
                            <xsl:variable name="refid">
                                <xsl:value-of select="@id" />
                            </xsl:variable>

                            <xsl:variable name="refname">
                                <xsl:value-of select="refnamediv/refname" />
                            </xsl:variable>
                    <!-- For each section if there is note about enhanced in this version -->
                                <xsl:for-each select="refsection">
                                    <xsl:for-each select="para | */para">
                                        <xsl:choose>
                                            <xsl:when test="contains(.,'Changed: 2.0')">
                                                <listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="." /></simpara></listitem>
                                            </xsl:when>
                                        </xsl:choose>
                                    </xsl:for-each>
                                </xsl:for-each>
                        </xsl:for-each>
                    </itemizedlist>
					</xsl:if>
             </sect2>

			<sect2 id="NewFunctions_1_5">
				<title>PostGIS Functions new, behavior changed, or enhanced in 1.5</title>
				<xsl:if test="//para[contains(text(),'Availability: 1.5')]">
				<para>The functions given below are PostGIS functions that were introduced or enhanced in this minor release.</para>
				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
					<xsl:for-each select='//refentry'>
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


				<!-- For each section if there is note about availability in this version -->
							<xsl:for-each select="refsection">
								<xsl:for-each select="para">
									<xsl:choose>
										<xsl:when test="contains(.,'Availability: 1.5')">
											<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="." /><xsl:text> </xsl:text> <xsl:value-of select="$comment" /></simpara></listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
				</xsl:if>
			</sect2>
			<sect2 id="NewFunctions_1_4">
				<title>PostGIS Functions new, behavior changed, or enhanced in 1.4</title>
				<para>The functions given below are PostGIS functions that were introduced or enhanced in the 1.4 release.</para>
				<xsl:if test="//para[contains(text(),'Availability: 1.4')]">
				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
					<xsl:for-each select='//refentry'>
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
											<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refid" /></link> - <xsl:value-of select="$comment" /> <xsl:text> </xsl:text><xsl:value-of select="." /></simpara></listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
				</xsl:if>
			</sect2>
			<sect2 id="NewFunctions_1_3">
				<title>PostGIS Functions new in 1.3</title>
				<para>The functions given below are PostGIS functions that were introduced in the 1.3 release.</para>
				<xsl:if test="//para[contains(text(),'Availability: 1.3')]">
				<itemizedlist>
				<!-- Pull out the purpose section for each ref entry and strip whitespace and put in a variable to be tagged unto each function comment  -->
				<xsl:for-each select='//refentry'>
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
										<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refid" /></link> - <xsl:value-of select="$comment" /> <xsl:text> </xsl:text><xsl:value-of select="." /></simpara></listitem>
									</xsl:when>
								</xsl:choose>
							</xsl:for-each>
						</xsl:for-each>
				</xsl:for-each>
				</itemizedlist>
				</xsl:if>
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
