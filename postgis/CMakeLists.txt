configure_file(sqldefines.h.in ${CMAKE_CURRENT_BINARY_DIR}/sqldefines.h)
set_property(GLOBAL PROPERTY POSTGIS_INCLUDE_DIRS
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(HEADERS
  geography.h
  geography_measurement_trees.h
  lwgeom_export.h
  lwgeom_functions_analytic.h
  lwgeom_geos.h
  lwgeom_geos_prepared.h
  lwgeom_rtree.h)

add_library(postgis SHARED
  postgis_module.c
  lwgeom_accum.c
	lwgeom_spheroid.c
	lwgeom_ogc.c
	lwgeom_functions_analytic.c
	lwgeom_inout.c
	lwgeom_functions_basic.c
	lwgeom_btree.c
	lwgeom_box.c
	lwgeom_box3d.c
	lwgeom_geos.c
	lwgeom_geos_prepared.c
	lwgeom_geos_clean.c
	lwgeom_geos_relatematch.c
	lwgeom_export.c
	lwgeom_in_gml.c
	lwgeom_in_kml.c
	lwgeom_in_geojson.c
	lwgeom_triggers.c
	lwgeom_dump.c
	lwgeom_functions_lrs.c
	long_xact.c
	lwgeom_sqlmm.c
	lwgeom_rtree.c
	lwgeom_transform.c
	gserialized_typmod.c
	gserialized_gist_2d.c
	gserialized_gist_nd.c
	geography_inout.c
	geography_btree.c
	geography_estimate.c
	geography_measurement.c
	geography_measurement_trees.c
	geometry_estimate.c
  ${HEADERS})

target_link_libraries(postgis
  $<TARGET_OBJECTS:libpgcommon>
  $<TARGET_OBJECTS:liblwgeom>)

add_custom_command(OUTPUT postgis.sql.in
  COMMAND
  "${CMAKE_C_COMPILER}" -E -P -w ${CMAKE_CURRENT_SOURCE_DIR}/postgis.sql.in.c -o postgis.sql.in -I${CMAKE_CURRENT_BINARY_DIR} -I${CMAKE_SOURCE_DIR}/libpgcommon
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/postgis.sql.in.c
  COMMENT "Preprocessing postgis.sql.in file"
  VERBATIM
)
add_custom_target(PreprocessSQL
  ALL
  DEPENDS postgis.sql.in
  COMMENT "Preprocessing SQL files"
  VERBATIM
  SOURCES postgis.sql.in.c)
