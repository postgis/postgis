/******************************************************************************
 *
 * Project:  PostGIS
 * Purpose:  GSERIALIZED seed corpus builder
 *
 ******************************************************************************
 * Copyright (C) 2026 Darafei Praliaskouski <me@komzpa.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 ****************************************************************************/

#include <errno.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#endif

#include <fstream>
#include <string>

extern "C" {
#include "geos_stub.h"
#include "proj_stub.h"
}

#include "liblwgeom_fuzzer.hpp"

extern "C" GSERIALIZED *gserialized1_from_lwgeom(LWGEOM *geom, size_t *size);

static int
mkdir_if_needed(const std::string &path)
{
	int result;

#ifdef _WIN32
	result = _mkdir(path.c_str());
#else
	result = mkdir(path.c_str(), 0777);
#endif

	if (result == 0 || errno == EEXIST)
		return 1;
	perror(path.c_str());
	return 0;
}

static int
write_file(const std::string &path, const void *data, size_t len)
{
	FILE *file = fopen(path.c_str(), "wb");
	if (file == NULL)
	{
		perror(path.c_str());
		return 0;
	}
	if (len != 0 && fwrite(data, 1, len, file) != len)
	{
		perror(path.c_str());
		fclose(file);
		return 0;
	}
	if (fclose(file) != 0)
	{
		perror(path.c_str());
		return 0;
	}
	return 1;
}

static int
write_seed(const std::string &base, const std::string &name, const std::string &wkt)
{
	LWGEOM *geom = lwgeom_from_wkt(wkt.c_str(), LW_PARSER_CHECK_NONE);
	if (geom == NULL)
		return 0;

	lwvarlena_t *wkb = lwgeom_to_wkb_varlena(geom, WKB_NDR | WKB_EXTENDED);
	size_t gserialized1_size = 0;
	size_t gserialized2_size = 0;
	GSERIALIZED *gserialized1 = gserialized1_from_lwgeom(geom, &gserialized1_size);
	GSERIALIZED *gserialized2 = gserialized_from_lwgeom(geom, &gserialized2_size);

	int ok = 1;
	if (wkb != NULL)
	{
		ok &= write_file(base + "/gserialized_from_lwgeom_fuzzer_seed_corpus/" + name,
				 wkb->data,
				 LWSIZE_GET(wkb->size) - LWVARHDRSZ);
	}
	if (gserialized1 != NULL)
	{
		ok &= write_file(base + "/gserialized_from_bytea_fuzzer_seed_corpus/" + name + "_v1",
				 gserialized1,
				 gserialized1_size);
	}
	if (gserialized2 != NULL)
	{
		ok &= write_file(base + "/gserialized_from_bytea_fuzzer_seed_corpus/" + name + "_v2",
				 gserialized2,
				 gserialized2_size);
	}

	lwfree(wkb);
	lwfree(gserialized1);
	lwfree(gserialized2);
	lwgeom_free(geom);
	return ok;
}

int
main(int argc, char **argv)
{
	if (argc != 3)
	{
		fprintf(stderr, "usage: %s OUT seed-list\n", argv[0]);
		return 1;
	}

	const std::string out = argv[1];
	if (!mkdir_if_needed(out) || !mkdir_if_needed(out + "/gserialized_from_bytea_fuzzer_seed_corpus") ||
	    !mkdir_if_needed(out + "/gserialized_from_lwgeom_fuzzer_seed_corpus"))
	{
		return 1;
	}

	postgis_lwgeom_fuzzer_initialize();

	std::ifstream input(argv[2]);
	if (!input)
	{
		perror(argv[2]);
		return 1;
	}

	std::string line;
	int ok = 1;
	while (std::getline(input, line))
	{
		if (line.empty() || line[0] == '#')
			continue;

		const std::string::size_type separator = line.find('|');
		if (separator == std::string::npos)
		{
			fprintf(stderr, "invalid seed line: %s\n", line.c_str());
			ok = 0;
			continue;
		}

		const std::string name = line.substr(0, separator);
		const std::string wkt = line.substr(separator + 1);
		if (POSTGIS_LWGEOM_FUZZER_SETJMP())
		{
			postgis_lwgeom_fuzzer_cleanup_allocations();
			ok = 0;
			continue;
		}
		ok &= write_seed(out, name, wkt);
		postgis_lwgeom_fuzzer_cleanup_allocations();
	}

	return ok ? 0 : 1;
}
