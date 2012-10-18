<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!-- ********************************************************************
	 $Id: postgis_aggs_mm.xml.xsl 10286 2012-09-14 11:29:38Z robe $
	 ********************************************************************
	 Copyright 2010, Regina Obe
	 License: BSD
	 Purpose: This is an xsl transform that generates index listing of aggregate functions and mm /sql compliant functions xml section from reference_new.xml to then
	 be processed by doc book
	 ******************************************************************** -->
	<xsl:output method="xml" indent="yes" encoding="utf-8" />
	<xsl:template match="*">
	<xsl:copy>
	<xsl:copy-of select="@*"/>
	<xsl:apply-templates/>
	</xsl:copy>
	</xsl:template>
</xsl:stylesheet>
