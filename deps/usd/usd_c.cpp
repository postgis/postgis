/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright 2023 J CUBE Inc and Jadason Technology Ltd
 *
 **********************************************************************/

#include "usd_c.h"
#include "reader.h"
#include "writer.h"

#include <fstream>
#include <random>
#include <sstream>
#include <vector>

static std::string
create_unique_temporary_path(const char *name)
{
	static std::mt19937 mt(time(nullptr));
#ifdef _WIN32
	char tmp_dir[128] = {'\0'};
	GetTempPathA(tmp_dir, 127);
#else
	const char *tmp_dir = P_tmpdir;
#endif
	std::ostringstream oss;
	oss << tmp_dir << '/' << mt() << '-' << name;
	std::string oss_string = oss.str();

	return oss_string;
}

struct usd_write_context {
	usd_write_context(usd_format format, const std::string &root_name, const std::string &geom_name)
	    : m_format(format), m_writer(root_name, geom_name), m_size(0)
	{}

	usd_format m_format;

	USD::Writer m_writer;

	size_t m_size;
	std::vector<char> m_data;
};

usd_write_context *
usd_write_create(usd_format format, const char *root_name, const char *geom_name)
{
	usd_write_context *ctx = new usd_write_context(format, root_name, geom_name);
	return ctx;
}

void
usd_write_destroy(usd_write_context *ctx)
{
	delete ctx;
}

void
usd_write_convert(usd_write_context *ctx, LWGEOM *geom)
{
	ctx->m_writer.Write(geom);
}

void
usd_write_save(struct usd_write_context *ctx)
{
	try
	{
		if (ctx->m_format == USDA)
		{
			std::string stage_string;
			if (ctx->m_writer.m_stage->ExportToString(&stage_string))
			{
				ctx->m_size = stage_string.size();
				ctx->m_data.resize(ctx->m_size + 1, '\0');
				strncpy(ctx->m_data.data(), stage_string.c_str(), stage_string.size());
			}
			else
			{
				lwerror("usd: failed to save as usda");
			}
		}
		else if (ctx->m_format == USDC)
		{
			/* create temporary USDC file and export */
			const std::string &usdc_file_path = create_unique_temporary_path("postgis_usd.usdc");
			if (ctx->m_writer.m_stage->Export(usdc_file_path))
			{
				/* read back the USDC file */
				FILE *file = fopen(usdc_file_path.c_str(), "rb");
				fseek(file, 0, SEEK_END);
				ctx->m_size = ftell(file);
				ctx->m_data.resize(ctx->m_size);
				fseek(file, 0, SEEK_SET);
				fread(ctx->m_data.data(), ctx->m_size, 1, file);
				fclose(file);

				/* remove the temporary USDC file */
				remove(usdc_file_path.c_str());
			}
			else
			{
				lwerror("usd: failed to save as usdc");
			}
		}
		else
		{
			lwerror("usd: unknown usd format to save");
		}
	}
	catch (const std::exception &e)
	{
		lwerror("usd: caught exception %s", e.what());
	}
	catch (...)
	{
		lwerror("usd: caught unknown exception");
	}
}

void
usd_write_data(struct usd_write_context *ctx, size_t *size, void **data)
{
	*size = ctx->m_size;
	if (data)
	{
		*data = ctx->m_data.data();
	}
}

struct usd_read_context {
	USD::Reader m_reader;
};

struct usd_read_context *
usd_read_create()
{
	usd_read_context *ctx = new usd_read_context;
	return ctx;
}

void
usd_read_convert(usd_read_context *ctx, const char *data, size_t size, usd_format format)
{
	try
	{
		if (format == USDA)
		{
			ctx->m_reader.ReadUSDA(std::string(data, size));
		}
	}
	catch (const std::exception &e)
	{
		lwerror("usd: caught c++ exception %s", e.what());
	}
	catch (...)
	{
		lwerror("usd: caught unknown exception");
	}
}

struct usd_read_iterator {
	usd_read_iterator(usd_read_context *ctx) : m_ctx(ctx), m_index(0) {}
	usd_read_context *m_ctx;

	size_t m_index;
};

struct usd_read_iterator *
usd_read_iterator_create(usd_read_context *ctx)
{
	usd_read_iterator *itr = new usd_read_iterator(ctx);
	return itr;
}

LWGEOM *
usd_read_iterator_next(struct usd_read_iterator *itr)
{
	/* return false if context has being iterated */
	if (itr->m_index >= itr->m_ctx->m_reader.m_geoms.size())
	{
		return nullptr;
	}

	/* copy data */
	LWGEOM *geom = itr->m_ctx->m_reader.m_geoms[itr->m_index];

	/* increase index */
	itr->m_index += 1;

	return geom;
}

void
usd_read_iterator_destroy(struct usd_read_iterator *itr)
{
	delete itr;
}

void
usd_read_destroy(struct usd_read_context *ctx)
{
	delete ctx;
}
