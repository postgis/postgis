#include "usd_c.h"
#include "writer.h"

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
			ctx->m_size = stage_string.size();
			ctx->m_data.reset(new char[stage_string.size() + 1]);
			strncpy(ctx->m_data.get(), stage_string.c_str(), stage_string.size());
		}
		else
		{
			lwerror("usd: failed to save as usda");
		}
	}
	else if (format == USDC)
	{
		lwerror("usd: failed to save as usdc");
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
