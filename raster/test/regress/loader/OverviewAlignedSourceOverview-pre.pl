use File::Basename qw(dirname);

sub make_tif {
	my ($name, $xmin, $xmax) = @_;
	my $dir = dirname($TEST);
	my $asc = "$dir/$name.asc";
	my $tif = "$dir/$name.tif";

	open(my $fh, '>', $asc) or die "Could not create $asc: $!";
	print $fh "ncols 100\n";
	print $fh "nrows 100\n";
	print $fh "xllcorner $xmin\n";
	print $fh "yllcorner -100\n";
	print $fh "cellsize 1\n";
	print $fh "NODATA_value -9999\n";
	for my $y (0..99) {
		print $fh join(' ', map { $y * 100 + $_ } 1..100), "\n";
	}
	close($fh);

	system(
		'gdal_translate', '-q', '-of', 'GTiff', '-ot', 'UInt16',
		'-a_ullr', $xmin, '0', $xmax, '-100',
		$asc, $tif
	) == 0 or die "Could not create $tif";
	system('gdaladdo', '-q', '-r', 'average', $tif, '3') == 0
		or die "Could not create overview for $tif";

	unlink $asc;
}

make_tif('OverviewAlignedSourceOverviewA', 0, 100);
make_tif('OverviewAlignedSourceOverviewB', 100, 200);
