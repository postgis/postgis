#!/usr/bin/env perl

$ENV{"LC_ALL"} = "C";

use warnings;
use strict;

my $top_srcdir = ".";
my $rev_file = $top_srcdir.'/postgis_revision.h';

my $target = 'local';
$target = $ARGV[0] if $ARGV[0];

# Read the revision number
my $rev = &read_rev($target);

# Write it
&write_defn($rev);


sub read_rev {

  my $target = shift;

  #print STDERR "Target: $target\n";

	return &read_rev_git();
}

sub read_rev_git {

  # TODO: test on old systems, I think I saw some `which`
  #       implementations returning "nothing found" or something
  #       like that, making the later if ( ! $git_exe ) always false
  #
  my $git_exe = `which git`;
  if ( ! $git_exe ) {
    print STDERR "Can't determine revision: no git executable found\n";
    return 0;
  }
  chop($git_exe);

  my $cmd = "\"${git_exe}\" describe --always";
  #print STDERR "cmd: ${cmd}\n";
  my $rev  = `$cmd`;

  if ( ! $rev ) {
    print STDERR "Can't determine revision from git log\n";
    $rev = 0;
  } else {
    chop($rev);
  }

  return $rev;
}

sub write_defn {
  my $rev = shift;
  my $oldrev = 0;

  # Do not override the file if new detected
  # revision isn't zero nor different from the existing one
  if ( -f $rev_file ) {
    open(IN, "<$rev_file");
    my $oldrevline = <IN>;
    if ( $oldrevline =~ /POSTGIS_REVISION (.*)/ ) {
      $oldrev = $1;
    }
    close(IN);
    if ( $rev eq "0" or $rev eq $oldrev ) {
      print STDERR "Not updating existing rev file at $oldrev\n";
      return;
    }
  }

  my $string = "#define POSTGIS_REVISION $rev\n";
  open(OUT,">$rev_file");
  print OUT $string;
  close(OUT);
  print STDERR "Wrote rev file at $rev\n";
}
