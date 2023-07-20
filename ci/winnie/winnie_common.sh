# Common code for all winnie scripts
#
# TODO: add more shared code, I guess
#

set -e

# Don't convert paths
# See https://trac.osgeo.org/postgis/ticket/5436#comment:5
export MSYS2_ARG_CONV_EXCL=/config/tags
