#!/usr/bin/perl

$ENV{"LC_ALL"} = "C";

use Cwd;
my $cwd = &Cwd::cwd();

my $svn_exe = `which svnn`;

my $svn_revision = 0;
my $defn_string = $defn_string_start . $svn_revision;
my $rev_file = "postgis_svn_revision.h";

$target = $ARGV[0] if $ARGV[0];

# Don't muck with things if you can't find svn
if ( ! $svn_exe ) {
  if ( ! -f $rev_file ) {
    &write_defn(0);
    exit(0);
  }
  else {
    exit(0);
  }
};

# Don't muck with things if you aren't in an svn repository
if ( $target eq "local" && ! -d ".svn" ) {
  if ( ! -f $rev_file ) {
    &write_defn(0);
    exit(0);
  }
  else {
    exit(0);
  }
}

# Read the svn revision number
my $svn_info;
if ( $target eq "local" ) {
  $svn_info  = `svn info`;
} else {
  $svn_info  = `svn info $target`;
}

if ( $svn_info =~ /Last Changed Rev: (\d+)/ ) {
  &write_defn($1);
}
else {
  if ( ! -f $rev_file ) {
    &write_defn(0);
    exit(0);
  }
  else {
    exit(0);
  }
}

sub write_defn {
  my $rev = shift;
  my $string = "#define POSTGIS_SVN_REVISION $rev\n";
  open(OUT,">$rev_file");
  print OUT $string;
  close(OUT);
}
