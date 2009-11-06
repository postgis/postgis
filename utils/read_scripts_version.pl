#!/usr/bin/perl

$url = "http://svn.osgeo.org/postgis/";

if ( $ARGV[1] ) 
{
  $url .= "branches/" . $ARGV[1] . "/postgis/";
  print "Reading scripts version from branch $ARGV[1] ...\n";
}
else
{
  $url .= "trunk/postgis/";
  print "Reading scripts version from trunk ...\n";
}

@files = ( 
  "postgis.sql.in.c",
  "geography.sql.in.c",
  "sqlmm.sql.in.c",
  "long_xact.sql.in.c" 
  );

$rev = 0;

foreach $f (@files)
{
  $uf = $url . $f;
  $s = `svn info $uf`;
  ($r) = ($s =~ /Last Changed Rev: (\d+)/);
  print $uf," (Revision $r)\n";
  $rev = $r if $r > $rev; 
}

print "\nScripts revision: $rev\n\n";


