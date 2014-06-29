SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ] ; do SOURCE="$(readlink "$SOURCE")"; done
DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

cp loader/testraster.tif loader/BasicOutDB.tif

echo "-F -C -R \"$DIR/loader/testraster.tif\"" > $DIR/BasicOutDB.opts
