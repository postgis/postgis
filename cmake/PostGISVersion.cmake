################################################################################
# version.cmake - part of CMake configuration of PostGIS
#
# Copyright (C) 2012 Mateusz Loskot <mateusz@loskot.net>
################################################################################
# Macros in this module:
#   
#   postgis_version - defines version information for PostGIS build
#
################################################################################
# Defines version information for PostGIS
#
# postgis_version(MAJOR major_version MINOR minor_version PATCH patch_level)
#
#    MAJOR.MINOR version is used to set SOVERSION
#
macro(postgis_version)
  parse_arguments(THIS_VERSION "MAJOR;MINOR;PATCH;"
    ""
    ${ARGN})

  # Set version components
  set(${PROJECT_NAME}_VERSION_MAJOR ${THIS_VERSION_MAJOR})
  set(${PROJECT_NAME}_VERSION_MINOR ${THIS_VERSION_MINOR})
  set(${PROJECT_NAME}_VERSION_PATCH ${THIS_VERSION_PATCH})

  # Set VERSION string
  set(${PROJECT_NAME}_VERSION
    "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_PATCH}")

  # Set SOVERSION based on major and minor
  set(${PROJECT_NAME}_SOVERSION
    "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}")

  # Set ABI version string used to name binary output and, by PostGIS loader, to find binaries.
  # On Windows, ABI version is specified using binary file name suffix.
  # On Unix, suffix ix empty and SOVERSION is used instead.
  if (UNIX)
    set(${PROJECT_NAME}_ABI_VERSION ${${PROJECT_NAME}_SOVERSION})
  elseif(WIN32)
    set(${PROJECT_NAME}_ABI_VERSION
      "${${PROJECT_NAME}_VERSION_MAJOR}_${${PROJECT_NAME}_VERSION_MINOR}")
  else()
    message(FATAL_ERROR "Ambiguous target platform with unknown ABI version scheme. Giving up.")
  endif()

  message(STATUS "")

  postgis_report_value(${PROJECT_NAME}_VERSION)
  postgis_report_value(${PROJECT_NAME}_ABI_VERSION)

  add_definitions(-Dpostgis_ABI_VERSION="${${PROJECT_NAME}_ABI_VERSION}")

endmacro()
