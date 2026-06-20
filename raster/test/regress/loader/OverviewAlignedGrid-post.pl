use File::Basename qw(dirname);

my $dir = dirname($TEST);
unlink "$dir/OverviewAlignedGridA.tif";
unlink "$dir/OverviewAlignedGridB.tif";
