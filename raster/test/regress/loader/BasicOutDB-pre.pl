use File::Basename;
use Cwd 'abs_path';

my $TARGETFILE = $TEST . '.tif';

my $FILERASTER = abs_path(dirname($TEST)) . "/testraster.tif";

if ( ! -e $TARGETFILE ) {
	link ("$FILERASTER", "$TARGETFILE") ||
		die("Cannot link $FILERASTER to $TARGETFILE: $!");
}

my $OPTSFNAME = $TEST . '.opts';
open(OPTS, '>', $OPTSFNAME) || die ('Cannot open ' . $OPTSFNAME. ": $1");
print OPTS "-F -C -R \"$FILERASTER\"\n";
close(OPTS);
