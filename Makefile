#---------------------------------------------------------------
# Configuration Directives
#---------------------------------------------------------------

#
# We recommend that you install the Proj4 and GEOS libraries
# referenced below to get the most use out of your PostGIS
# database.

#
# Set USE_PROJ to 1 for Proj4 reprojection support (recommended)
#
# Reprojection allows you to transform coordinate systems
# in the database with the Transform() function.
#
# Download from: http://www.remotesensing.org/proj
#
USE_PROJ=1
ifeq (${PROJ_DIR},) 
	PROJ_DIR=/usr/local
endif

#
# Set USE_GEOS to 1 for GEOS spatial predicate and operator
# support (recommended).
# GEOS installation directory defaults to /usr/local,
# set GEOS_DIR environment variable to change it.
#
# GEOS allows you to do exact topological tests, such as
# Intersects() and Touches(), as well as geometry operations,
# such as Buffer(), GeomUnion() and Difference().
#
# Download from: http://geos.refractions.net
#
USE_GEOS=1
ifeq (${GEOS_DIR},) 
	GEOS_DIR=/usr/local
endif

#
# Set USE_STATS to 1 for new GiST statistics collection support
# Note that this support requires additional columns in 
# GEOMETRY_COLUMNS, so see the list archives for info or
# install a fresh database using postgis.sql
#
USE_STATS=1

#
# Root of the PostgreSQL source tree 
# If you are not building from within postgresql 'contrib' directory
# set the PGSQL_SRC either below or in the environment (an absolute path).
#
# PGSQL_SRC=/usr/src/postgresql

#
# Path to library (to be specified in CREATE FUNCTION queries)
# Defaults to $libdir.
# Set LPATH below or in the environment to change it.
#
# LPATH=/usr/src/postgis

#---------------------------------------------------------------
# END OF CONFIGURATION
#---------------------------------------------------------------

export USE_GEOS
export GEOS_DIR
export USE_STATS
export USE_PROJ
export PGSQL_SRC

all: liblwgeom loaderdumper

install: all liblwgeom-install loaderdumper-install

clean: liblwgeom-clean loaderdumper-clean

liblwgeom: 
	$(MAKE) -C lwgeom

liblwgeom-clean:
	$(MAKE) -C lwgeom clean

liblwgeom-install:
	$(MAKE) -C lwgeom install

loaderdumper:
	$(MAKE) -C loader

loaderdumper-clean:
	$(MAKE) -C loader clean

loaderdumper-install:
	$(MAKE) -C loader install
