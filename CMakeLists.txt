################################################################################
# CMakeLists.txt - root CMake configuration file, part PostGIS project
#
# Copyright (C) 2012 Mateusz Loskot <mateusz@loskot.net>
################################################################################
cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)
project(PostGIS)

################################################################################
# User options to control PostGIS features

option(POSTGIS_WITH_RASTER
    "Set ON|OFF (default=OFF) to control building raster extension" OFF)

option(POSTGIS_WITH_TOPOLOGY
    "Set ON|OFF (default=OFF) to control building topology extension" OFF)
    
option(POSTGIS_ENABLE_STATS
    "Set ON|OFF (default=ON) to control use ANALYZE statistics" ON)
set(POSTGIS_USE_STATS 1)
if(NOT POSTGIS_ENABLE_STATS)
    set(POSTGIS_USE_STATS 0)
endif()

option(POSTGIS_ENABLE_AUTOCACHE_BBOX
    "Set ON|OFF (default=ON) to control BBOX caching within geometries" ON)
set(POSTGIS_AUTOCACHE_BBOX 1)
if(NOT POSTGIS_ENABLE_AUTOCACHE_BBOX)
    set(POSTGIS_AUTOCACHE_BBOX 0)
endif()

option(POSTGIS_ENABLE_PROFILE
  "Set ON|OFF (default=OFF) to control use GEOS profiling" OFF)
set(POSTGIS_PROFILE 0)
if(POSTGIS_ENABLE_PROFILE)
    set(POSTGIS_PROFILE 1)
endif()

option(POSTGIS_ENABLE_DEBUG
  "Set ON|OFF (default=OFF) to control verbose debugging messages" OFF)
set(POSTGIS_DEBUG_LEVEL 0)
if(POSTGIS_ENABLE_DEBUG)
    set(POSTGIS_DEBUG_LEVEL 4)
endif()

################################################################################
# Path to additional CMake modules
set(CMAKE_MODULE_PATH ${PostGIS_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH ${PostGIS_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

include(PostGISUtilities)

message(STATUS)
colormsg(_HIBLUE_ "Configuring PostGIS:")
message(STATUS)

################################################################################
# Dependency (required): PostgreSQL
#OLD: find_program(PG_CONFIG pg_config)
#if(NOT PG_CONFIG)
#    message(FATAL ERROR " Please check your PostgreSQL installation    ") 
#endif(NOT PG_CONFIG)
find_package(PostgreSQL) # TODO: req version?
if(NOT PG_CONFIG)
    message(FATAL ERROR " Please check your PostgreSQL installation")
endif(NOT PG_CONFIG)

exec_program(${PG_CONFIG} ARGS --version OUTPUT_VARIABLE _PGSQL_VERSION_OUTPUT)

string(REGEX REPLACE 
    "^PostgreSQL[\t ]+([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1.\\2.\\3"
    PGSQL_VERSION "${_PGSQL_VERSION_OUTPUT}")
unset(_PGSQL_VERSION_OUTPUT)

if(PGSQL_VERSION)
    string(REGEX REPLACE
        "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1"
        PGSQL_VERSION_MAJOR "${PGSQL_VERSION}")
    string(REGEX REPLACE
        "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2"
        PGSQL_VERSION_MINOR "${PGSQL_VERSION}")

    set(POSTGIS_PGSQL_VERSION ${PGSQL_VERSION_MAJOR}${PGSQL_VERSION_MINOR})
endif()

exec_program(${PG_CONFIG} ARGS --includedir
    OUTPUT_VARIABLE PGSQL_INCLUDEDIR)
exec_program(${PG_CONFIG} ARGS --includedir-server
    OUTPUT_VARIABLE PGSQL_INCLUDEDIR_SERVER)

include_directories(${PGSQL_INCLUDEDIR})
include_directories(${PGSQL_INCLUDEDIR_SERVER})
if(WIN32)
    include_directories(${PGSQL_INCLUDEDIR_SERVER}/port/win32)
    if(MSVC)    
        include_directories(${PGSQL_INCLUDEDIR_SERVER}/port/win32_msvc)
    endif()
endif()

exec_program(${PG_CONFIG} ARGS --pkglibdir OUTPUT_VARIABLE PGSQL_PKGLIBDIR)
exec_program(${PG_CONFIG} ARGS --sharedir OUTPUT_VARIABLE PGSQL_SHAREDIR)

################################################################################
# Dependency (required): PROJ.4
find_package(PROJ4)
if(NOT PROJ4_FOUND)
    message(FATAL ERROR " Please check your PROJ.4 installation") 
endif()

set(POSTGIS_PROJ_VERSION 46) # TODO
include_directories(${PROJ4_INCLUDE_DIR})

################################################################################
# Dependency (required): GEOS
find_package(GEOS)
if(NOT GEOS_FOUND)
    message(FATAL ERROR " Please check your GEOS installation") 
endif()

set(POSTGIS_GEOS_VERSION 32) # TODO
include_directories(${GEOS_INCLUDE_DIR})

if(POSTGIS_WITH_TOPOLOGY)
    #TODO: GEOS version >= 3.3.2.
    postgis_report_value(POSTGIS_WITH_TOPOLOGY)
endif()

################################################################################
# Dependency (required): GEOS
#find_package(JSONC)
#if(NOT JSONC_FOUND)
#    message(FATAL ERROR " Please check your json-c installation") 
#endif()

################################################################################
# Dependency (optional): GDAL
if(POSTGIS_WITH_RASTER)
    find_package(GDAL)
endif()

################################################################################
# Generate config header
configure_file("postgis_config.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/postgis_config.h")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

################################################################################
# Version and build date information

# Extract version details from Version.config
if(EXISTS "${PostGIS_SOURCE_DIR}/Version.config")
  file(READ "${PostGIS_SOURCE_DIR}/Version.config" _POSTGIS_VERSION_CONFIG)
  string(STRIP ${_POSTGIS_VERSION_CONFIG} _POSTGIS_VERSION_CONFIG)

  string(REGEX REPLACE ".*POSTGIS_MAJOR_VERSION=([0-9]+)[\r\n\t\ ].*" "\\1"
    POSTGIS_MAJOR_VERSION ${_POSTGIS_VERSION_CONFIG})
  string(REGEX REPLACE ".*POSTGIS_MINOR_VERSION=([0-9]+)[\r\n\t\ ].*" "\\1"
    POSTGIS_MINOR_VERSION "${_POSTGIS_VERSION_CONFIG}")
  string(REGEX REPLACE ".*POSTGIS_MICRO_VERSION=([0-9]+[a-zA-Z]+)[\r\n\t ]?.*" "\\1"
    POSTGIS_MICRO_VERSION "${_POSTGIS_VERSION_CONFIG}")
  
  set(POSTGIS_VERSION "${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION} USE_GEOS=${POSTGIS_USE_GEOS} USE_PROJ=${POSTGIS_USE_PROJ} USE_STATS=${POSTGIS_USE_STATS}")
  set(POSTGIS_LIB_VERSION "${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.${POSTGIS_MICRO_VERSION}")
  set(POSTGIS_SCRIPTS_VERSION "${POSTGIS_LIB_VERSION}")

  unset(_VERSION_CONFIG_CONTENTS)
endif()

# Determine build date and time
if (WIN32)
    exec_program(date ARGS /T OUTPUT_VARIABLE _BUILD_DATE)
    exec_program(time ARGS /T OUTPUT_VARIABLE _BUILD_TIME)
    set(POSTGIS_BUILD_DATE "${_BUILD_DATE} ${_BUILD_TIME}")
    unset(_BUILD_DATE)
    unset(_BUILD_TIME)
elseif(UNIX)
    exec_program(date ARGS -u "+%Y-%m-%d %H:%M:%S"
        OUTPUT_VARIABLE POSTGIS_BUILD_DATE)
endif()

################################################################################
# Predefined constants
set(SRID_MAX 999999)
set(SRID_USR_MAX 998999)

################################################################################
# Configure all subdirectories with any build targets
add_subdirectory(liblwgeom)
add_subdirectory(libpgcommon)

get_property(LIBLWGEOM_INCLUDE_DIRS GLOBAL PROPERTY LIBLWGEOM_INCLUDE_DIRS)
include_directories(${LIBLWGEOM_INCLUDE_DIRS})

################################################################################
# Install
# TODO
################################################################################
# Configuration summary
message(STATUS)
postgis_report_value(POSTGIS_VERSION)
postgis_report_value(POSTGIS_LIB_VERSION)
postgis_report_value(POSTGIS_SCRIPTS_VERSION)
postgis_report_value(POSTGIS_WITH_TOPOLOGY)
postgis_report_value(POSTGIS_WITH_RASTER)
postgis_report_value(POSTGIS_PGSQL_VERSION)
postgis_report_value(POSTGIS_WITH_TOPOLOGY)
