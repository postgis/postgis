use File::Basename qw(dirname);

my $dir = dirname($TEST);
my $asc = "$dir/SourceOverview.asc";
my $tif = "$dir/SourceOverview-generated.tif";

# The source raster values are deliberately sequential so the averaged GDAL
# overview has different corner pixels than a nearest-neighbor overview generated
# by raster2pgsql from the full-resolution band.
open(my $fh, '>', $asc) or die "Could not create $asc: $!";
print $fh "ncols 8\n";
print $fh "nrows 8\n";
print $fh "xllcorner 0\n";
print $fh "yllcorner 0\n";
print $fh "cellsize 1\n";
print $fh "NODATA_value -9999\n";
for my $y (0..7) {
	print $fh join(' ', map { $y * 8 + $_ } 1..8), "\n";
}
close($fh);

system(
	'gdal_translate', '-q', '-of', 'GTiff', '-ot', 'Byte',
	'-a_ullr', '0', '0', '8', '-8',
	$asc, $tif
) == 0 or die "Could not create $tif";

system('gdaladdo', '-q', '-r', 'average', $tif, '2') == 0
	or die "Could not create overview for $tif";

unlink $asc;
