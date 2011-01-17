<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!-- ********************************************************************
	 $Id$
	 ********************************************************************
	 Copyright 2010, Regina Obe
	 License: BSD
	 Purpose: This is an xsl transform that generates index listing of aggregate functions and mm /sql compliant functions xml section from reference_new.xml to then
	 be processed by doc book
	 ******************************************************************** -->
	<xsl:output method="xml" indent="yes" encoding="utf-8" />

	<!-- We deal only with the reference chapter -->
	<xsl:template match="/">
		<xsl:apply-templates select="/book/chapter[@id='reference' or @id='RT_references']" />
	</xsl:template>

	<xsl:template match="//chapter">
	<chapter>
		<title>PostGIS Special Functions Index</title>
		<sect1 id="PostGIS_Aggregate_Functions">
			<title>PostGIS Aggregate Functions</title>
			<para>The functions given below are spatial aggregate functions provided with PostGIS that can be used just like any other sql aggregate function such as sum, average.</para>
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

			<!-- For each function prototype if it takes a geometry set then catalog it as an aggregate function  -->
				<xsl:for-each select="refsynopsisdiv/funcsynopsis/funcprototype">
					<xsl:choose>
						<xsl:when test="contains(paramdef/type,' set') or contains(paramdef/type,'geography set') or contains(paramdef/type,'raster set')">
							 <listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refid" /></link> - <xsl:value-of select="$comment" /></simpara></listitem>
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
				and for large geometries or geometry pairs that cover more than one UTM zone. Basic tranform - (favoring UTM, Lambert Azimuthal (North/South), and falling back on mercator in worst case scenario)</para></note>
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
				The box family of types consists of <link linkend="box2d">box2d</link>, <link linkend="box3d">box3d</link>, <link linkend="box3d_extent">box3d_extent</link> </para>
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
			<para>The functions given below are PostGIS functions that can use CIRCULARSTRING, CURVEDPOLYGON, and other curved geometry types</para>
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
			<title>PostGIS Function Support Matrix</title>

			<para>Below is an alphabetical listing of spatial specific functions in PostGIS and the kinds of spatial
				types they work with or OGC/SQL compliance they try to conform to.</para>
			<para><itemizedlist>
				<listitem><simpara>A <xsl:value-of select="$matrix_checkmark" disable-output-escaping="yes"/> means the function works with the type or subtype natively.</simpara></listitem>
				<listitem><simpara>A <xsl:value-of select="$matrix_transform" disable-output-escaping="yes"/> means it works but with a transform cast built-in using cast to geometry, transform to a "best srid" spatial ref and then cast back. Results may not be as expected for large areas or areas at poles 
						and may accumulate floating point junk.</simpara></listitem>
				<listitem><simpara>A <xsl:value-of select="$matrix_autocast" disable-output-escaping="yes"/> means the function works with the type because of a auto-cast to another such as to box3d rather than direct type support.</simpara></listitem>
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
						<!-- Exclude PostGIS types ,management functions, long transaction support, or exceptional functions from consideration  -->
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
										<entry><xsl:value-of select="$matrix_checkmark" disable-output-escaping="yes"/></entry>
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
										<entry><xsl:value-of select="$matrix_checkmark" disable-output-escaping="yes"/></entry>
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
										<entry><xsl:value-of select="$matrix_checkmark" disable-output-escaping="yes"/></entry>
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
										<entry><xsl:value-of select="$matrix_checkmark" disable-output-escaping="yes"/></entry>
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
										<entry><xsl:value-of select="$matrix_checkmark" disable-output-escaping="yes"/></entry>
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
			<sect2 id="NewFunctions_2_0">
				<title>PostGIS Functions new, behavior changed, or enhanced in 2.0</title>
				<para>The functions given below are PostGIS functions that were added, enhanced, or have breaking changes in 2.0 releases.</para>
				<para>New geometry types: TIN and Polyhedral surfaces was introduced in 2.0</para>
				<note><para>Greatly improved support for Topology.  Please refer to <xref linkend="Topology" /> for more details.</para></note>
				<note><para>In PostGIS 2.0, raster type and raster functionality has been integrated.  There are way too many new raster functions to list here and all are new so 
					please refer to <xref linkend="RT_reference" /> for more details of the raster functions available.</para></note>
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
										<xsl:when test="contains(.,'Availability: 2.0')">
											<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="." /><xsl:text> </xsl:text> <xsl:value-of select="$comment" /></simpara></listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
				
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
								<xsl:for-each select="para">
									<xsl:choose>
										<xsl:when test="contains(.,'Enhanced: 2.0')">
											<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="." /></simpara></listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
				
				<para>The functions given below are PostGIS functions that have changed behavior in PostGIS 2.0.</para>
				<note><para>Most deprecated functions have been removed.  These are functions that haven't been documented since 1.2
					or some internal functions that were never documented.  If you are using a function that you don't see documented,
					it's probably deprecated, about to be deprecated, or internal and should be avoided.</para></note>
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
								<xsl:for-each select="para">
									<xsl:choose>
										<xsl:when test="contains(.,'Changed: 2.0')">
											<listitem><simpara><link linkend="{$refid}"><xsl:value-of select="$refname" /></link> - <xsl:value-of select="." /></simpara></listitem>
										</xsl:when>
									</xsl:choose>
								</xsl:for-each>
							</xsl:for-each>
					</xsl:for-each>
				</itemizedlist>
			</sect2>
			<sect2 id="NewFunctions_1_5">
				<title>PostGIS Functions new, behavior changed, or enhanced in 1.5</title>
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
			</sect2>
			<sect2 id="NewFunctions_1_4">
				<title>PostGIS Functions new, behavior changed, or enhanced in 1.4</title>
				<para>The functions given below are PostGIS functions that were introduced or enhanced in the 1.4 release.</para>
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
			</sect2>
			<sect2 id="NewFunctions_1_3">
				<title>PostGIS Functions new in 1.3</title>
				<para>The functions given below are PostGIS functions that were introduced in the 1.3 release.</para>
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
