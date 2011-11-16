<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!-- ********************************************************************
     $Id: topology_cheatsheet.html.xsl 6130 2010-10-26 14:47:57Z robe $
     ********************************************************************
	 Copyright 2011, Regina Obe
     License: BSD
	 Purpose: This is an xsl transform that generates PostgreSQL COMMENT ON FUNCTION ddl
	 statements from postgis xml doc reference
     ******************************************************************** -->
	<xsl:output method="text" />
	<xsl:variable name='postgis_version'>2.0</xsl:variable>
	<xsl:variable name='new_tag'>Availability: <xsl:value-of select="$postgis_version" /></xsl:variable>
	<xsl:variable name='enhanced_tag'>Enhanced: <xsl:value-of select="$postgis_version" /></xsl:variable>

<xsl:template match="/">
	<xsl:text><![CDATA[<html><head><title>Post GIS PostGIS Playing Cards</title>
	<style type="text/css">
<!--
body {
	font-family: Arial, sans-serif;
	font-size: 8.5pt;
}

.comment {font-size:x-small;color:green;font-family:"courier new"}
.notes {font-size:x-small;color:#dd1111;font-weight:500;font-family:verdana}

	]]></xsl:text>

<xsl:text><![CDATA[	
.func {position:relative;left:10px;top:20px;font-weight: 600;font-size:10pt;text-align:center; padding: 1px}
.func_descrip {font-size: 8pt;text-align:left; padding:10px 5px 15px 20px;}
#divoutter {width:850px }
.card_front {
	background-color: #eee;
	width:200px; height: 250px;
	float:left;border-bottom:thin dotted #ff0000;border-top:thin dotted #ff0000;
}

.card_back {
	background-color: #fff;
	width:200px; height: 250px;
	float:left; border-top:thin dotted #ff0000;border-bottom:thin dotted #ff0000;border-right:thin dotted #ff0000;
}

h1 {
	margin: 0px;
	padding: 0px;
	font-size: 14pt;
}

-->
</style>
	</head><body><h1 style='text-align:center'>Post GIS ]]></xsl:text> <xsl:text><![CDATA[ Day 2011 Commemorative Playing Cards</h1>
			<a href="http://www.postgis.org"><img src='images/ccbysa.png' /></a><a href="http://www.postgis.org">http://www.postgis.org</a>
			<p>Celebrate this Post GIS day with these versatile Post GIS day commemorative playing cards. The number of games and fun-filled hours you
			can have with these cards is priceless. Here is a small listing of the infinite number of games you can play with Post GIS cards:</p>
			<ul><li><b>Name that thing</b> In this game you have the descriptions face up and have the opponent guess the name of the function, type, or operator.</li>
				<li><b>What does it do?</b> In this game you have the name of the thing face up and have the opponent describe what the thing does or is for </li> 
				<li><b>Post GIS war game</b> This game requires no knowledge of PostGIS what-so-ever. In this game, you play with the descriptions face up. Even your kids will like this game, and may even get tricked into learning how to use PostGIS.
					There are two joker cards -- the What Is Post GIS and What does Post GIS.  Any player that is dealt either of these cards wins - period.  For other cards the order of precendence is: 
						<sup>1</sup> - Is super and beats anything else except another <sup>1</sup> or joker card. In event of multiple <sup>1</sup>, the one that happens alphabetically first trumps the others.  Symbols always trump letters. <br />
						<sup>2</sup> - Second favorite, alphabetical rules apply (is beaten by a joker, <sup>1</sup>) <br />
						<sup>mm</sup> - third highest ranking <br />
						All other cards precedence by alphabetical order.</li>
				<li><b>Post GIS in a language I don't understand</b> To celebrate the ubiquity of PostGIS, you can create Post GIS playing cards in a language
					you don't understand.  Here is what you do.  Go to <a href="http://translate.google.com">http://translate.google.com</a> and paste in the URL to this page in the first text box (make sure it is set to English),
					in the <b>To:</b> drop down, pick a language you do not know, but preferably you have friends that speak that can laugh at your grammar and pronounciation. In no time you'll be able to impress your friends living far away with your command of their language.
				</li>	
				<li><b>Post GIS in a language I do understand</b> Similar to the I don't understand game, except you pick a non-english language that you do understand. Enjoy many moments of laughter reading machine generated translations that are sorta accurate but often hysterical.
					</li>
				</ul>
			<div id="divoutter"><div class='card_front'><div class='func'>WHAT IS POST GIS?</div></div><div class='card_back'><div class='func'>POSTGIS<br /><img src='images/PostGIS_logo.png' style='width:150px;height:150px' /></div></div>
			<div class='card_front'><div class='func'>WHAT DOES POST GIS?</div></div><div class='card_back'><div class='func'>POSTGIS<br /><img src='images/PostGIS_logo.png' style='width:150px;height:150px'/></div></div>]]></xsl:text>
			<xsl:apply-templates select="/book/chapter//refentry" />
			<xsl:text><![CDATA[</div></body></html>]]></xsl:text>
</xsl:template>
			
        
<xsl:template match="refentry" >
	<xsl:variable name="lt"><xsl:text><![CDATA[<]]></xsl:text></xsl:variable>
	<xsl:variable name="gt"><xsl:text><![CDATA[>]]></xsl:text></xsl:variable>
	 <xsl:variable name='plaindescr'>
		<xsl:call-template name="globalReplace">
			<xsl:with-param name="outputString" select="refnamediv/refpurpose"/>
			<xsl:with-param name="target" select="$lt"/>
			<xsl:with-param name="replacement" select="''"/>
		</xsl:call-template>
	</xsl:variable>
	<xsl:variable name='plaindescr2'>
		<xsl:call-template name="globalReplace">
			<xsl:with-param name="outputString" select="$plaindescr"/>
			<xsl:with-param name="target" select="$gt"/>
			<xsl:with-param name="replacement" select="''"/>
		</xsl:call-template>
	</xsl:variable>
	<!-- add row for each function and alternate colors of rows -->
	<![CDATA[<div class="card_front"><div class='func'>]]><xsl:value-of select="refnamediv/refname" /><xsl:if test="contains(.,$new_tag)"><![CDATA[<sup>1</sup> ]]></xsl:if> 
	<!-- enhanced tag -->
	<xsl:if test="contains(.,$enhanced_tag)"><![CDATA[<sup>2</sup> ]]></xsl:if>
	<xsl:if test="contains(.,'implements the SQL/MM')"><![CDATA[<sup>mm</sup> ]]></xsl:if>
	<xsl:if test="contains(refsynopsisdiv/funcsynopsis,'geography') or contains(refsynopsisdiv/funcsynopsis/funcprototype/funcdef,'geography')"><![CDATA[<sup>G</sup>  ]]></xsl:if>
	<xsl:if test="contains(.,'GEOS &gt;= 3.3')"><![CDATA[<sup>g3.3</sup> ]]></xsl:if>
	<xsl:if test="contains(.,'This function supports 3d')"><![CDATA[<sup>3d</sup> ]]></xsl:if>
	
	<![CDATA[</div></div><div class='card_back'><div class='func_descrip'>]]><xsl:value-of select="$plaindescr2" /><![CDATA[</div></div>]]>
</xsl:template>
	
<!--General replace macro hack to make up for the fact xsl 1.0 does not have a built in one.  
	Not needed for xsl 2.0 lifted from http://www.xml.com/pub/a/2002/06/05/transforming.html -->
	<xsl:template name="globalReplace">
	  <xsl:param name="outputString"/>
	  <xsl:param name="target"/>
	  <xsl:param name="replacement"/>
	  <xsl:choose>
		<xsl:when test="contains($outputString,$target)">
		  <xsl:value-of select=
			"concat(substring-before($outputString,$target),
				   $replacement)"/>
		  <xsl:call-template name="globalReplace">
			<xsl:with-param name="outputString" 
				 select="substring-after($outputString,$target)"/>
			<xsl:with-param name="target" select="$target"/>
			<xsl:with-param name="replacement" 
				 select="$replacement"/>
		  </xsl:call-template>
		</xsl:when>
		<xsl:otherwise>
		  <xsl:value-of select="$outputString"/>
		</xsl:otherwise>
	  </xsl:choose>
	</xsl:template>
	
<xsl:template name="break">
  <xsl:param name="text" select="."/>
  <xsl:choose>
    <xsl:when test="contains($text, '&#xa;')">
      <xsl:value-of select="substring-before($text, '&#xa;')"/>
      <![CDATA[<br/>]]>
      <xsl:call-template name="break">
        <xsl:with-param 
          name="text" 
          select="substring-after($text, '&#xa;')"
        />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$text"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


</xsl:stylesheet>
