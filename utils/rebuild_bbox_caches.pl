#
# $Id$
#

eval "exec perl -w $0 $@"
	if (0);

use Pg;

$VERBOSE = 0;

sub
usage
{
	local($me) = `basename $0`;
	chop($me);
	print STDERR "$me [-v] <database> [<database>...]\n";
	exit 1;
}

usage unless @ARGV;

for ($i=0; $i<@ARGV; $i++)
{
	if ( $ARGV[$i] eq '-v' )
	{
		$VERBOSE++;
		next;
	}

	local($db) = $ARGV[$i];

	print "Database: $db\n" if $VERBOSE;

	local($conn) = &connect($db);

	rebuild_bbox_cache_db($conn);
	#vacuum_db($conn);
	update_geometry_stats($conn);
}

sub
vacuum_db
{
	local($conn) = shift;
	local($query) = "VACUUM FULL";
	local($res) = $conn->exec($query);
	if ( $res->resultStatus != PGRES_COMMAND_OK )  {
		print STDERR $conn->errorMessage;
		exit(1);
	}
}

sub
update_geometry_stats
{
	local($conn) = shift;
	local($query) = "SELECT update_geometry_stats()";
	local($res) = $conn->exec($query);
	if ( $res->resultStatus != PGRES_TUPLES_OK )  {
		print STDERR $conn->errorMessage;
		exit(1);
	}
}

sub
vacuum_reindex_table
{
	local($conn) = shift;
	local($tblspec) = shift;
	local($query);
	local($res);

	$query = "VACUUM FULL ANALYZE $tblspec";
	print "\to $query\n" if $VERBOSE;

	$res = $conn->exec($query);
	if ( $res->resultStatus != PGRES_COMMAND_OK )  {
		print STDERR $conn->errorMessage;
		exit(1);
	}

	$query = "REINDEX TABLE $tblspec";
	print "\to $query\n" if $VERBOSE;

	$res = $conn->exec($query);
	if ( $res->resultStatus != PGRES_COMMAND_OK )  {
		print STDERR $conn->errorMessage;
		exit(1);
	}
}

sub
rebuild_bbox_cache_db
{
	local($conn) = shift;

	local($gcres) = get_geometry_columns($conn);

	local($lasttblspec) = undef;
	local(@row);
	while (@row = $gcres->fetchrow)
	{
		local($schema) = $row[0];
		local($table) = $row[1];
		local($col) = $row[2];
		local($tblspec);

		if ( defined($schema) ) {
			$tblspec = "\"$schema\".\"$table\"";
		} else {
			$tblspec = "\"$table\"";
		}

		$lasttblspec = $tblspec if ! defined($lasttblspec);
		if ( $tblspec ne $lasttblspec )
		{
			&vacuum_reindex_table($conn, $lasttblspec);
			$lasttblspec = $tblspec;
		}

		rebuild_bbox_cache_col($conn, $tblspec, $col);

	}

	&vacuum_reindex_table($conn, $lasttblspec);
}

sub
rebuild_bbox_cache_col
{
	local($conn) = shift;
	local($tblspec) = shift;
	local($column) = shift;

	print "\to UPDATE $schema.$table.$column" if $VERBOSE;

	local($query) = "UPDATE $tblspec SET \"$column\" = addbbox(dropbbox(\"$column\")) WHERE hasbbox(\"$column\")";
	local($res) = $conn->exec($query);
	if ( $res->resultStatus != PGRES_COMMAND_OK )  {
		print STDERR $conn->errorMessage;
		exit(1);
	}
	print ": ".$res->cmdTuples." updates\n" if $VERBOSE;

}

	

sub
connect
{
	local($db) = shift;

	#connect
	$conn = Pg::connectdb("dbname=$db");
	if ( $conn->status != PGRES_CONNECTION_OK ) {
		print STDERR $conn->errorMessage;
		exit(1);
	}

	return $conn;
}

#
# Check if backend supports schema
#
sub
check_has_schema
{
	local($conn) = shift;

	local($query) = "SELECT version()";
	local($res) = $conn->exec($query);
	if ( $res->resultStatus != PGRES_TUPLES_OK )  {
		print STDERR $conn->errorMessage;
		exit(1);
	}
	local($version) = $res->getvalue(0, 0);
	# PostgreSQL 8.0.0 on i686-pc-linux-gnu, compiled by GCC gcc (GCC) 3.4.3
	$version =~ /PostgreSQL (\d*)\.(\d*).(\d*)/;
	$major = $1;
	$minor = $2;
	return 1 if ( $major gt 7 );
	return 1 if ( $major eq 7 && $minor ge 3 );
	return 0;
	print "Postgresql version : $major $minor\n";
}

#
# Return a relation with 3 fields:
# 	schema, table, column
# Schema will be undef if schema are unsupported.
#
sub
get_geometry_columns
{
	local($conn) = shift;

	if ( check_has_schema($conn) )
	{
		$query = "select n.nspname as schema, c.relname as tbl, a.attname as col from pg_class c, pg_namespace n, pg_attribute a, pg_type t WHERE c.relnamespace = n.oid and a.attrelid = c.oid and a.atttypid = t.oid and t.typname = 'geometry' and c.relkind = 'r'";
	}
	else
	{
		$query = "select NULL as schema, c.relname as tbl, a.attname as col from pg_class c, pg_attribute a, pg_type t WHERE a.attrelid = c.oid and a.atttypid = t.oid and t.typname = 'geometry' and relkind = 'r'";
	}

	$query .= " ORDER BY schema, tbl, col";

	local($res) = $conn->exec($query);
	if ( $res->resultStatus != PGRES_TUPLES_OK )  {
		print STDERR $conn->errorMessage;
		exit(1);
	}

	return $res;
}


##################################################################

# 
# $Log$
# Revision 1.1.2.1  2005/07/29 11:46:16  strk
# Initial implementation of geometries bbox cache recomputation script
#
# Revision 1.8.4.1  2005/04/18 13:28:19  strk
# Fixed to work against LWGEOM installations
#
# Revision 1.8  2004/03/08 17:21:57  strk
# changed error computation code to delta/totrows
#
# Revision 1.7  2004/03/06 18:02:48  strk
# Comma-separated bps values accepted
#
# Revision 1.6  2004/03/05 21:06:04  strk
# Added -vacuum switch
#
# Revision 1.5  2004/03/05 21:03:18  strk
# Made the -bps switch specify the exact level(s) at which to run the test
#
# Revision 1.4  2004/03/05 16:40:30  strk
# rewritten split_extent to be more datatype-conservative
#
# Revision 1.3  2004/03/05 16:01:02  strk
# added -bps switch to set maximun query level. reworked command line parsing
#
# Revision 1.2  2004/03/05 15:29:35  strk
# more verbose output
#
# Revision 1.1  2004/03/05 11:52:24  strk
# initial import
#
#
