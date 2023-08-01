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

#include <boost/smart_ptr/shared_array.hpp>

struct usd_write_context {
	usd_write_context() : m_size(0) {}

	USD::Writer m_writer;

	size_t m_size;
	boost::shared_array<char> m_data;
};

usd_write_context *
usd_write_create()
{
	usd_write_context *ctx = new usd_write_context;
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
usd_write_save(struct usd_write_context *ctx, usd_format format)
{
	try
	{
		if (format == USDA)
		{
			std::string stage_string;
			if (ctx->m_writer.m_stage->ExportToString(&stage_string))
			{
				ctx->m_size = stage_string.size() + 1;
				ctx->m_data.reset(new char[ctx->m_size]);
				memset(ctx->m_data.get(), '\0', ctx->m_size);
				strncpy(ctx->m_data.get(), stage_string.c_str(), stage_string.size());
			}
			else
			{
				lwerror("usd: failed to save as usda");
			}
		}
		else if (format == USDC)
		{
			/* create unique path for temporary USDC file */
#ifdef _WIN32
			char tmp_path[128] = {'\0'};
			GetTempPathA(tmp_path, 127);
#else
			const char *tmp_path = P_tmpdir;
#endif

			static std::mt19937 mt(time(nullptr));
			uint32_t n = mt();

			char tmp_name[128] = {'\0'};
			snprintf(tmp_name, 127, "postgis_usd_%u.usdc", n);

			char file_path[256] = {'\0'};
			snprintf(file_path, 255, "%s/%s", tmp_path, tmp_name);

			if (ctx->m_writer.m_stage->Export(file_path))
			{
				FILE *file = fopen(file_path, "rb");
				fseek(file, 0, SEEK_END);
				ctx->m_size = ftell(file);
				ctx->m_data.reset(new char[ctx->m_size]);
				fseek(file, 0, SEEK_SET);
				fread(ctx->m_data.get(), ctx->m_size, 1, file);
				fclose(file);

				/* remove the temporary USDC file */
				remove(file_path);
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
		*data = ctx->m_data.get();
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
