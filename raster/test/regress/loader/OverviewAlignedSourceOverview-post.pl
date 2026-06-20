use File::Basename qw(dirname);

my $dir = dirname($TEST);
unlink "$dir/OverviewAlignedSourceOverviewA.tif";
unlink "$dir/OverviewAlignedSourceOverviewB.tif";
