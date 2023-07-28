#ifndef USD_C_H
#define USD_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "liblwgeom.h"
#include "lwgeom_log.h"

typedef enum usd_format {
    USDA = 1,
    USDC = 2
} usd_format;

struct usd_write_context;

struct usd_write_context *usd_write_create();

void usd_write_destroy(struct usd_write_context *ctx);

void usd_write_convert(struct usd_write_context *ctx, LWGEOM *geom);

void usd_write_save(struct usd_write_context *ctx, usd_format format);

void usd_write_data(struct usd_write_context *ctx, size_t *size, void **data);

#ifdef __cplusplus
}
#endif

#endif
