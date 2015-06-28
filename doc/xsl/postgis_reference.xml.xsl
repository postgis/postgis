<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!-- ********************************************************************
	 ********************************************************************
	 Copyright 2010, Regina Obe
	 License: BSD
	 Purpose: This is an xsl transform that concatenates 
	      all the sections defined in postgis.xml and strips all tags
	      for suitable import by expert bot 
	 To use: xsltproc -o postgis_full.xml postgis_reference.xml.xsl postgis.xml
	 ******************************************************************** -->
	<xsl:output method="xml" indent="yes" encoding="utf-8" />
	<xsl:template match="*">
	<xsl:copy>
	<xsl:copy-of select="@*"/>
	<xsl:apply-templates/>
	</xsl:copy>
	</xsl:template>
	
	<!-- just grab link and title -->
	 <xsl:template match="ulink">
	 	<xsl:value-of select="@url" /><xsl:text> </xsl:text><xsl:value-of select="." />
	 </xsl:template>

	<!--just grab name of file --> 
	 <xsl:template match="filename">
	 	<xsl:value-of select="." />
	 </xsl:template>
	 
	 <!--strip varname tag and leave just the name --> 
	 <xsl:template match="varname">
	 	<xsl:value-of select="." />
	 </xsl:template>
	 
	 <!--strip xref tag and just leave reference section --> 
	 <xsl:template match="xref">
	 	<xsl:value-of select="@linkend" />
	 </xsl:template>
	 
	 <!--strip comman tag and just leave inner body --> 
	 <xsl:template match="command">
	 	command: <xsl:value-of select="." />
	 </xsl:template>
</xsl:stylesheet>
