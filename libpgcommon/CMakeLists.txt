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
