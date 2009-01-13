/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2008 Paul Ramsey
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 * 
 **********************************************************************/

#include <stdio.h>
#include <string.h>
#include "CUnit/Basic.h"

#include "cu_algorithm.h"


/*
** Set up liblwgeom to run in stand-alone mode using the 
** usual system memory handling functions.
*/
void lwgeom_init_allocators(void) {
        /* liblwgeom callback - install default handlers */
        lwgeom_install_default_allocators();
}

/*
** The main() function for setting up and running the tests.
** Returns a CUE_SUCCESS on successful running, another
** CUnit error code on failure.
*/
int main()
{

	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	/* Add the PostGIS suite to the registry */
	if (NULL == register_cg_suite())
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();

	return CU_get_error();

}
