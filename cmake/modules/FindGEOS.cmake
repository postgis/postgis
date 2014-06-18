###############################################################################
# CMake module to search for GEOS library
#
# On success, the macro sets the following variables:
# GEOS_FOUND = if the library found
# GEOS_LIBRARY = full path to the library
# GEOS_INCLUDE_DIR = where to find the library headers
#
# Copyright (c) 2009 Mateusz Loskot <mateusz@loskot.net>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
###############################################################################

IF(WIN32)

    # Try to use OSGeo4W installation
    if(WIN32)
        set(_OSGEO4W_HOME "C:/OSGeo4W") 

        if($ENV{OSGEO4W_HOME})
            set(_OSGEO4W_HOME "$ENV{OSGEO4W_HOME}") 
        endif()
    endif(WIN32)

  if(MINGW)
    FIND_PATH(GEOS_INCLUDE_DIR geos_c.h /usr/local/include /usr/include c:/msys/local/include)
    FIND_LIBRARY(GEOS_LIBRARY NAMES geos_c PATHS /usr/local/lib /usr/lib c:/msys/local/lib)
  ENDif(MINGW)

  if(MSVC)
    SET(GEOS_INCLUDE_DIR "$ENV{LIB_DIR}/include" CACHE STRING INTERNAL)
    
    FIND_PATH(GEOS_INCLUDE_DIR geos_c.h
        PATHS ${_OSGEO4W_HOME}/include
        DOC "Path to GEOS library include directory")

    FIND_LIBRARY(GEOS_LIBRARY NAMES geos geos_c_i
      PATHS
      ${_OSGEO4W_HOME}/lib
      "$ENV{LIB_DIR}/lib"
      #mingw
      c:/msys/local/lib
      NO_DEFAULT_PATH
      DOC "Path to GEOS library file")
  endif(MSVC)
  
ELSE(WIN32)

 IF(UNIX) 

    # try to use framework on mac
    if(APPLE)
      SET (GEOS_MAC_PATH /Library/Frameworks/GEOS.framework/unix/bin)
    ENDif(APPLE)

    SET(GEOS_CONFIG_PREFER_PATH "$ENV{GEOS_HOME}/bin" CACHE STRING "preferred path to GEOS (geos-config)")
    FIND_PROGRAM(GEOS_CONFIG geos-config
      ${GEOS_CONFIG_PREFER_PATH}
      ${GEOS_MAC_PATH}
      /usr/local/bin/
      /usr/bin/
      )
    #MESSAGE("DBG GEOS_CONFIG ${GEOS_CONFIG}")

    if(GEOS_CONFIG)
      
      EXEC_PROGRAM(${GEOS_CONFIG}
        ARGS --version
        OUTPUT_VARIABLE GEOS_VERSION)
      STRING(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1" GEOS_VERSION_MAJOR "${GEOS_VERSION}")
      STRING(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2" GEOS_VERSION_MINOR "${GEOS_VERSION}")

      if(GEOS_VERSION_MAJOR LESS 3)
          MESSAGE (FATAL_ERROR "GEOS version is too old (${GEOS_VERSION}). Use 3.0.0 or higher.")
      ENDif(GEOS_VERSION_MAJOR LESS 3)
     
      # set INCLUDE_DIR to prefix+include
      EXEC_PROGRAM(${GEOS_CONFIG}
        ARGS --prefix
        OUTPUT_VARIABLE GEOS_PREFIX)

      FIND_PATH(GEOS_INCLUDE_DIR 
        geos_c.h 
        ${GEOS_PREFIX}/include
        /usr/local/include 
        /usr/include 
        )

      ## extract link dirs for rpath  
      EXEC_PROGRAM(${GEOS_CONFIG}
        ARGS --libs
        OUTPUT_VARIABLE GEOS_CONFIG_LIBS )

      ## split off the link dirs (for rpath)
      ## use regular expression to match wildcard equivalent "-L*<endchar>"
      ## with <endchar> is a space or a semicolon
      STRING(REGEX MATCHALL "[-][L]([^ ;])+" 
        GEOS_LINK_DIRECTORIES_WITH_PREFIX 
        "${GEOS_CONFIG_LIBS}" )
      #MESSAGE("DBG  GEOS_LINK_DIRECTORIES_WITH_PREFIX=${GEOS_LINK_DIRECTORIES_WITH_PREFIX}")

      ## remove prefix -L because we need the pure directory for LINK_DIRECTORIES
      
      if(GEOS_LINK_DIRECTORIES_WITH_PREFIX)
        STRING(REGEX REPLACE "[-][L]" "" GEOS_LINK_DIRECTORIES ${GEOS_LINK_DIRECTORIES_WITH_PREFIX} )
      ENDif(GEOS_LINK_DIRECTORIES_WITH_PREFIX)

      ### XXX - mloskot: geos-config --libs does not return -lgeos_c, so set it manually
      ## split off the name
      ## use regular expression to match wildcard equivalent "-l*<endchar>"
      ## with <endchar> is a space or a semicolon
      #STRING(REGEX MATCHALL "[-][l]([^ ;])+" 
      #  GEOS_LIB_NAME_WITH_PREFIX 
      #  "${GEOS_CONFIG_LIBS}" )
      #MESSAGE("DBG  GEOS_CONFIG_LIBS=${GEOS_CONFIG_LIBS}")
      #MESSAGE("DBG  GEOS_LIB_NAME_WITH_PREFIX=${GEOS_LIB_NAME_WITH_PREFIX}")
      SET(GEOS_LIB_NAME_WITH_PREFIX -lgeos_c CACHE STRING INTERNAL)

      ## remove prefix -l because we need the pure name
      
      if(GEOS_LIB_NAME_WITH_PREFIX)
        STRING(REGEX REPLACE "[-][l]" "" GEOS_LIB_NAME ${GEOS_LIB_NAME_WITH_PREFIX} )
      ENDif(GEOS_LIB_NAME_WITH_PREFIX)
      #MESSAGE("DBG  GEOS_LIB_NAME=${GEOS_LIB_NAME}")

      if(APPLE)
        SET(GEOS_LIBRARY ${GEOS_LINK_DIRECTORIES}/lib${GEOS_LIB_NAME}.dylib CACHE STRING INTERNAL)
      ELSE (APPLE)
       SET(GEOS_LIBRARY ${GEOS_LINK_DIRECTORIES}/lib${GEOS_LIB_NAME}.so CACHE STRING INTERNAL)
      ENDif(APPLE)
      #MESSAGE("DBG  GEOS_LIBRARY=${GEOS_LIBRARY}")
      
    ELSE(GEOS_CONFIG)
      MESSAGE("FindGEOS.cmake: geos-config not found. Please set it manually. GEOS_CONFIG=${GEOS_CONFIG}")
    ENDIF(GEOS_CONFIG)

  ENDIF(UNIX)
ENDIF(WIN32)


if(GEOS_INCLUDE_DIR AND GEOS_LIBRARY)
   SET(GEOS_FOUND TRUE)
ENDif(GEOS_INCLUDE_DIR AND GEOS_LIBRARY)

if(GEOS_FOUND)

   if(NOT GEOS_FIND_QUIETLY)
      MESSAGE(STATUS "Found GEOS: ${GEOS_LIBRARY}")
   ENDif(NOT GEOS_FIND_QUIETLY)

ELSE (GEOS_FOUND)

   MESSAGE(GEOS_INCLUDE_DIR=${GEOS_INCLUDE_DIR})
   MESSAGE(GEOS_LIBRARY=${GEOS_LIBRARY})
   MESSAGE(FATAL_ERROR "Could not find GEOS")

ENDif(GEOS_FOUND)
