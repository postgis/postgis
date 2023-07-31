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
#include "writer.h"

#include <fstream>

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

		char tmp_name[128] = {'\0'};
		snprintf(tmp_name, 127, "postgis_usd_%p.usdc", boost::get_pointer(ctx->m_writer.m_stage));

		char file_path[256] = {'\0'};
		snprintf(file_path, 256, "%s/%s", tmp_path, tmp_name);

		if (ctx->m_writer.m_stage->Export(tmp_path))
		{
			std::ifstream tmp_file(tmp_path, std::ios::binary);
			tmp_file.seekg(0, std::ios::end);
			ctx->m_size = tmp_file.tellg();
			tmp_file.seekg(0, std::ios::beg);
			ctx->m_data.reset(new char[ctx->m_size]);
			tmp_file.read(ctx->m_data.get(), ctx->m_size);
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

void
usd_write_data(struct usd_write_context *ctx, size_t *size, void **data)
{
	*size = ctx->m_size;
	if (data)
	{
		*data = ctx->m_data.get();
	}
}
