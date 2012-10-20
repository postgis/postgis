<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!-- ********************************************************************
	 $Id$
	 ********************************************************************
	 Copyright 2010, Regina Obe
	 License: BSD
	 Purpose: This is an xsl transform that concatenates 
	      all the sections defined in postgis.xml
	 To use: xsltproc -o postgis_full.xml postgis_reference.xml.xsl postgis.xml
	 ******************************************************************** -->
	<xsl:output method="xml" indent="yes" encoding="utf-8" />
	<xsl:template match="*">
	<xsl:copy>
	<xsl:copy-of select="@*"/>
	<xsl:apply-templates/>
	</xsl:copy>
	</xsl:template>
</xsl:stylesheet>
