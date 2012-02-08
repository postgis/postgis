#!/usr/bin/perl

$ENV{"LC_ALL"} = "C";

use Cwd;
my $cwd = &Cwd::cwd();
my $svn_exe = `which svn`;
my $rev = 0;

# We have a repo and can read from it
if ( $svn_exe && -d ".svn" ) {
  my $svn_info;
  $svn_info  = `svn info`;

  if ( $svn_info =~ /Last Changed Rev: (\d+)/ ) {
    $rev = $1;
    open(OUT,">$cwd/svnrevision.h");
    print OUT "#define SVNREV $rev\n";
    close(OUT);
  } 
  else {
    die "Unable to find revision in svn info\n";
  }
}
# No repo, but there's a version file in the tarball
elsif ( -f "svnrevision.h" ) {
  my $svn_revision_file = `cat svnrevision.h`;
  if ( $svn_revision_file =~ /SVNREV (\d+)/ ) {
    $rev = $1;
  }
  else {
    die "svnrevision.h has an unexpected format\n";
  }
}
else {
  die "Unable read svnrevision.h or svn repository metadata\n";
}

print $rev;
