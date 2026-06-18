#!/usr/bin/env perl

#
# PostGIS - Spatial Types for PostgreSQL
# http://postgis.net
#
# Copyright (C) 2026 Darafei Praliaskouski <me@komzpa.net>
#
# This is free software; you can redistribute and/or modify it under
# the terms of the GNU General Public Licence. See the COPYING file.
#

use warnings;
use strict;

use File::Temp qw(tempdir);

my $restore_script = shift @ARGV
  or die "Usage: $0 <postgis_restore.pl>\n";

sub write_file {
  my ($path, $contents) = @_;
  open(my $fh, '>', $path) || die "Could not write $path: $!\n";
  print {$fh} $contents;
  close($fh) || die "Could not close $path: $!\n";
}

sub install_fake_programs {
  my ($bindir) = @_;

  write_file("$bindir/pg_dump", <<'SH');
#!/bin/sh
echo "pg_dump (fake)"
SH
  chmod 0755, "$bindir/pg_dump";

  write_file("$bindir/pg_restore", <<'SH');
#!/bin/sh
for arg in "$@"; do
  if test "$arg" = "--version"; then
    echo "pg_restore (fake)"
    exit 0
  fi
  if test "$arg" = "-l"; then
    cat "$POSTGIS_RESTORE_TEST_TOC"
    exit 0
  fi
done
cat "$POSTGIS_RESTORE_TEST_SQL"
SH
  chmod 0755, "$bindir/pg_restore";
}

sub run_restore {
  my ($toc, $sql) = @_;
  my $dir = tempdir(CLEANUP => 1);
  my $bindir = "$dir/bin";
  mkdir $bindir || die "Could not create $bindir: $!\n";
  install_fake_programs($bindir);

  my $dump = "$dir/test.dump";
  my $toc_file = "$dir/toc.txt";
  my $sql_file = "$dir/restore.sql";
  write_file($dump, "fake dump\n");
  write_file($toc_file, $toc);
  write_file($sql_file, $sql);

  local $ENV{PATH} = "$bindir:$ENV{PATH}";
  local $ENV{POSTGIS_RESTORE_TEST_TOC} = $toc_file;
  local $ENV{POSTGIS_RESTORE_TEST_SQL} = $sql_file;

  open(my $out, '-|', $^X, $restore_script, $dump)
    || die "Could not run $restore_script: $!\n";
  my $stdout = do { local $/; <$out> };
  close($out) || die "$restore_script failed\n";

  open(my $manifest, '<', "$dump.lst")
    || die "Could not read generated manifest: $!\n";
  my $manifest_text = do { local $/; <$manifest> };
  close($manifest) || die "Could not close generated manifest: $!\n";

  return ($stdout, $manifest_text);
}

sub assert_contains {
  my ($text, $needle, $label) = @_;
  die "not ok - $label\nMissing: $needle\nIn:\n$text\n"
    if index($text, $needle) < 0;
}

sub assert_not_contains {
  my ($text, $needle, $label) = @_;
  die "not ok - $label\nUnexpected: $needle\nIn:\n$text\n"
    if index($text, $needle) >= 0;
}

my ($stdout, $manifest) = run_restore(<<'TOC', <<'SQL');
1; 0 0 TABLE public topology postgres
2; 0 0 SEQUENCE public topology_id_seq postgres
3; 0 0 DEFAULT public topology id postgres
4; 0 0 COMMENT public TABLE topology postgres
5; 0 0 TABLE DATA public topology postgres
6; 0 0 TABLE DATA public spatial_ref_sys postgres
7; 0 0 TABLE topology layer postgres
8; 0 0 ACL topology TABLE layer postgres
9; 0 0 ACL - SCHEMA topology postgres
TOC
SET search_path = public, pg_catalog;
SQL

assert_contains($manifest, 'TABLE public topology', 'keep public table named topology');
assert_contains($manifest, 'SEQUENCE public topology_id_seq', 'keep serial sequence for public topology');
assert_contains($manifest, 'DEFAULT public topology id', 'keep serial default for public topology');
assert_contains($manifest, 'COMMENT public TABLE topology', 'keep comments for public topology');
assert_not_contains($manifest, 'TABLE topology layer', 'skip real topology layer table');
assert_not_contains($manifest, 'ACL topology TABLE layer', 'skip real topology layer ACL');
assert_not_contains($manifest, 'ACL - SCHEMA topology', 'skip real topology schema ACL');

($stdout, $manifest) = run_restore(<<'TOC', <<'SQL');
1; 0 0 TABLE app spatial_ref_sys postgres
2; 0 0 TABLE DATA app spatial_ref_sys postgres
3; 0 0 TABLE DATA postgis spatial_ref_sys postgres
4; 0 0 FUNCTION postgis st_concavehull(geometry, double precision, boolean) postgres
TOC
SET search_path = app, pg_catalog;
COPY spatial_ref_sys (srid, auth_name) FROM stdin;
1000000	app
\.
SET search_path = postgis, pg_catalog;
COPY spatial_ref_sys (srid, auth_name) FROM stdin;
1000000	postgis
\.
SQL

assert_contains($manifest, 'TABLE app spatial_ref_sys', 'keep application spatial_ref_sys table');
assert_contains($manifest, 'TABLE DATA app spatial_ref_sys', 'keep application spatial_ref_sys data');
assert_not_contains($manifest, 'TABLE DATA postgis spatial_ref_sys', 'skip PostGIS spatial_ref_sys data');
assert_not_contains($manifest, 'FUNCTION postgis st_concavehull', 'skip PostGIS function before restore');
assert_contains($stdout, "1000000\tapp", 'do not clamp application spatial_ref_sys COPY');
assert_not_contains($stdout, "1000000\tpostgis", 'clamp PostGIS spatial_ref_sys COPY');

($stdout, $manifest) = run_restore(<<'TOC', <<'SQL');
1; 0 0 TABLE app spatial_ref_sys postgres
2; 0 0 TABLE DATA app spatial_ref_sys postgres
3; 0 0 FUNCTION app st_concavehull(geometry, double precision, boolean) postgres
4; 0 0 TABLE DATA postgis spatial_ref_sys postgres
TOC
SET search_path = app, pg_catalog;
COPY spatial_ref_sys (srid, auth_name) FROM stdin;
1000000	app
\.
SET search_path = postgis, pg_catalog;
COPY spatial_ref_sys (srid, auth_name) FROM stdin;
1000000	postgis
\.
SQL

assert_contains($manifest, 'TABLE app spatial_ref_sys', 'keep application spatial_ref_sys table when app objects match skip list');
assert_contains($manifest, 'TABLE DATA app spatial_ref_sys', 'keep application spatial_ref_sys data when app objects match skip list');
assert_contains($manifest, 'FUNCTION app st_concavehull', 'keep application function named like PostGIS function');
assert_not_contains($manifest, 'TABLE DATA postgis spatial_ref_sys', 'skip extension-owned PostGIS spatial_ref_sys data');
assert_contains($stdout, "1000000\tapp", 'do not clamp application spatial_ref_sys COPY selected by app object');
assert_not_contains($stdout, "1000000\tpostgis", 'clamp extension-owned PostGIS spatial_ref_sys COPY');

($stdout, $manifest) = run_restore(<<'TOC', <<'SQL');
1; 0 0 TABLE DATA postgis spatial_ref_sys postgres
2; 0 0 SHELL TYPE app geometry postgres
3; 0 0 TYPE app geometry postgres
4; 0 0 SHELL TYPE postgis geometry postgres
5; 0 0 TYPE postgis geometry postgres
TOC
SET search_path = app, pg_catalog;
SQL

assert_contains($manifest, 'SHELL TYPE app geometry', 'keep application shell type named geometry');
assert_contains($manifest, 'TYPE app geometry', 'keep application type named geometry');
assert_not_contains($manifest, 'SHELL TYPE postgis geometry', 'skip PostGIS shell type');
assert_not_contains($manifest, 'TYPE postgis geometry', 'skip PostGIS type');

print "ok - postgis_restore schema-aware skip tests\n";
