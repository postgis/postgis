set_property(GLOBAL PROPERTY LIBPGCOMMON_INCLUDE_DIRS
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(HEADERS
  lwgeom_pg.h
	lwgeom_transform.h
  gserialized_gist.h
  pgsql_compat.h)
    
add_library(libpgcommon
  OBJECT
  gserialized_gist.c
  lwgeom_transform.c
  lwgeom_pg.c
  ${HEADERS})

add_dependencies(libpgcommon liblwgeom)
