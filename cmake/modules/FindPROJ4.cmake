###############################################################################
# CMake module to search for PROJ.4 library
#
# On success, the macro sets the following variables:
# PROJ4_FOUND       = if the library found
# PROJ4_LIBRARY     = full path to the library
# PROJ4_INCLUDE_DIR = where to find the library headers 
# also defined, but not for general use are
# PROJ4_LIBRARY, where to find the PROJ.4 library.
#
# Copyright (c) 2009 Mateusz Loskot <mateusz@loskot.net>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
###############################################################################

# Try to use OSGeo4W installation
if(WIN32)
    set(_OSGEO4W_HOME "C:/OSGeo4W") 

    if($ENV{OSGEO4W_HOME})
        set(_OSGEO4W_HOME "$ENV{OSGEO4W_HOME}") 
    endif()
endif(WIN32)

find_path(PROJ4_INCLUDE_DIR proj_api.h
    PATHS ${_OSGEO4W_HOME}/include
    DOC "Path to PROJ.4 library include directory")

set(PROJ4_NAMES ${PROJ4_NAMES} proj proj_i)
find_library(PROJ4_LIBRARY
    NAMES ${PROJ4_NAMES}
    PATHS ${_OSGEO4W_HOME}/lib
    DOC "Path to PROJ.4 library file")

# Handle the QUIETLY and REQUIRED arguments and set SPATIALINDEX_FOUND to TRUE
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PROJ4 DEFAULT_MSG PROJ4_LIBRARY PROJ4_INCLUDE_DIR)

if(PROJ4_FOUND)
  set(PROJ4_LIBRARIES ${PROJ4_LIBRARY})
endif()
