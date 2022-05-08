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
 * Copyright 2021 Björn Harrtell
 *
 **********************************************************************/

#include "flatgeobuf_c.h"
#include "feature_generated.h"
#include "geometrywriter.h"
#include "geometryreader.h"
#include "packedrtree.h"

using namespace flatbuffers;
using namespace FlatGeobuf;

typedef flatgeobuf_ctx ctx;

uint8_t flatgeobuf_magicbytes[] = { 0x66, 0x67, 0x62, 0x03, 0x66, 0x67, 0x62, 0x01 };
uint8_t FLATGEOBUF_MAGICBYTES_SIZE = sizeof(flatgeobuf_magicbytes);

struct FeatureItem : FlatGeobuf::Item {
	uoffset_t size;
	uint64_t offset;
};

int flatgeobuf_encode_header(ctx *ctx)
{
	FlatBufferBuilder fbb;
	fbb.TrackMinAlign(8);

	// inspect first geometry
	if (ctx->lwgeom != NULL) {
		if (lwgeom_has_srid(ctx->lwgeom))
			ctx->srid = ctx->lwgeom->srid;
		ctx->has_z = lwgeom_has_z(ctx->lwgeom);
		ctx->has_m = lwgeom_has_m(ctx->lwgeom);
		ctx->lwgeom_type = ctx->lwgeom->type;
		ctx->geometry_type = (uint8_t) GeometryWriter::get_geometrytype(ctx->lwgeom);
	} else {
		LWDEBUG(2, "ctx->lwgeom is null");
		ctx->geometry_type = 0;
	}

	LWDEBUGF(2, "ctx->geometry_type %d", ctx->geometry_type);

	std::vector<flatbuffers::Offset<FlatGeobuf::Column>> columns;
	std::vector<flatbuffers::Offset<FlatGeobuf::Column>> *pColumns = nullptr;

	if (ctx->columns_size > 0) {
		for (uint16_t i = 0; i < ctx->columns_size; i++) {
			auto c = ctx->columns[i];
			columns.push_back(CreateColumnDirect(fbb, c->name, (ColumnType) c->type));
		}
	}
	if (columns.size() > 0)
		pColumns = &columns;

	flatbuffers::Offset<Crs> crs = 0;
	if (ctx->srid > 0)
		crs = CreateCrsDirect(fbb, nullptr, ctx->srid);

	std::vector<double> envelope;
	std::vector<double> *pEnvelope = nullptr;
	if (ctx->has_extent) {
		envelope.push_back(ctx->xmin);
		envelope.push_back(ctx->ymin);
		envelope.push_back(ctx->xmax);
		envelope.push_back(ctx->ymax);
	}
	if (envelope.size() > 0)
		pEnvelope = &envelope;

	const auto header = CreateHeaderDirect(
		fbb, ctx->name, pEnvelope, (GeometryType) ctx->geometry_type, ctx->has_z, ctx->has_m, ctx->has_t, ctx->has_tm, pColumns, ctx->features_count, ctx->index_node_size, crs);
	fbb.FinishSizePrefixed(header);
	const auto buffer = fbb.GetBufferPointer();
	const auto size = fbb.GetSize();

	LWDEBUGF(2, "header size %d (with size prefix)", size);

	Verifier verifier(buffer, size - sizeof(uoffset_t));
	if (VerifySizePrefixedHeaderBuffer(verifier)) {
		lwerror("buffer did not pass verification");
		return -1;
	}

	ctx->buf = (uint8_t *) lwrealloc(ctx->buf, ctx->offset + size);
	LWDEBUGF(2, "copying to ctx->buf at offset %ld", ctx->offset);
	memcpy(ctx->buf + ctx->offset, buffer, size);

	ctx->offset += size;

	return 0;
}

int flatgeobuf_encode_feature(ctx *ctx)
{
	FlatBufferBuilder fbb;
	Offset<Geometry> geometry = 0;
	Offset<Vector<uint8_t>> properties = 0;

	fbb.TrackMinAlign(8);

	if (ctx->lwgeom != NULL && !lwgeom_is_empty(ctx->lwgeom)) {
		LWDEBUGG(3, ctx->lwgeom, "GeometryWriter input LWGEOM");
		if (ctx->lwgeom_type != ctx->lwgeom->type) {
			lwerror("mixed geometry type is not supported");
			return -1;
		}
		GeometryWriter writer(fbb, ctx->lwgeom, (GeometryType) ctx->geometry_type, ctx->has_z, ctx->has_m);
		geometry = writer.write(0);
	}
	if (ctx->properties_len > 0)
		properties = fbb.CreateVector<uint8_t>(ctx->properties, ctx->properties_len);
	FeatureBuilder builder(fbb);
	builder.add_geometry(geometry);
	builder.add_properties(properties);
	auto feature = builder.Finish();
	fbb.FinishSizePrefixed(feature);
	const auto buffer = fbb.GetBufferPointer();
	const auto size = fbb.GetSize();

	LWDEBUGF(3, "encode_feature size %ld", size);

	Verifier verifier(buffer, size - sizeof(uoffset_t));
	if (VerifySizePrefixedFeatureBuffer(verifier)) {
		lwerror("buffer did not pass verification");
		return -1;
	}

	LWDEBUGF(3, "reallocating ctx->buf to size %ld", ctx->offset + size);
	ctx->buf = (uint8_t * ) lwrealloc(ctx->buf, ctx->offset + size);
	LWDEBUGF(3, "copying feature to ctx->buf at offset %ld", ctx->offset);
	memcpy(ctx->buf + ctx->offset, buffer, size);

	if (ctx->create_index) {
		auto item = (flatgeobuf_item *) lwalloc(sizeof(flatgeobuf_item));
		memset(item, 0, sizeof(flatgeobuf_item));
		if (ctx->lwgeom != NULL && !lwgeom_is_empty(ctx->lwgeom)) {
			auto gbox = lwgeom_get_bbox(ctx->lwgeom);
			item->xmin = gbox->xmin;
			item->xmax = gbox->xmax;
			item->ymin = gbox->ymin;
			item->ymax = gbox->ymax;
		}
		item->offset = ctx->offset;
		item->size = size;
		ctx->items[ctx->features_count] = item;
	}
	ctx->offset += size;
	ctx->features_count++;

	return 0;
}

void flatgeobuf_create_index(ctx *ctx)
{
	// convert to structure expected by packedrtree
	std::vector<std::shared_ptr<Item>> items;
	for (uint64_t i = 0; i < ctx->features_count; i++) {
		const auto item = std::make_shared<FeatureItem>();
		item->nodeItem = {
			ctx->items[i]->xmin, ctx->items[i]->ymin, ctx->items[i]->xmax, ctx->items[i]->ymax
		};
		item->offset = ctx->items[i]->offset;
		item->size = ctx->items[i]->size;
		items.push_back(item);
	}
	// sort items
	hilbertSort(items);
	// calc extent
	auto extent = calcExtent(items);
	ctx->has_extent = true;
	ctx->xmin = extent.minX;
	ctx->ymin = extent.minY;
	ctx->xmax = extent.maxX;
	ctx->ymax = extent.maxY;
	// allocate new buffer and write magicbytes
	auto oldbuf = ctx->buf;
	auto oldoffset = ctx->offset;
	ctx->buf = (uint8_t *) lwalloc(sizeof(signed int) + FLATGEOBUF_MAGICBYTES_SIZE);
	memcpy(ctx->buf + sizeof(signed int), flatgeobuf_magicbytes, FLATGEOBUF_MAGICBYTES_SIZE);
	ctx->offset = sizeof(signed int) + FLATGEOBUF_MAGICBYTES_SIZE;
	// write new header
	flatgeobuf_encode_header(ctx);
	// calculate new offsets
	uint64_t featureOffset = 0;
	for (auto item : items) {
		auto featureItem = std::static_pointer_cast<FeatureItem>(item);
		featureItem->nodeItem.offset = featureOffset;
		featureOffset += featureItem->size;
	}
	// create and write index
	PackedRTree tree(items, extent, ctx->index_node_size);
	const auto writeData = [&ctx] (const void *data, const size_t size) {
		ctx->buf = (uint8_t *) lwrealloc(ctx->buf, ctx->offset + size);
		memcpy(ctx->buf + ctx->offset, data, size);
		ctx->offset += size;
	};
	tree.streamWrite(writeData);
	// read items and write in sorted order
	for (auto item : items) {
		auto featureItem = std::static_pointer_cast<FeatureItem>(item);
		ctx->buf = (uint8_t *) lwrealloc(ctx->buf, ctx->offset + featureItem->size);
		LWDEBUGF(2, "copy from offset %ld", featureItem->offset);
		memcpy(ctx->buf + ctx->offset, oldbuf + featureItem->offset, featureItem->size);
		ctx->offset += featureItem->size;
	}
	lwfree(oldbuf);
}

int flatgeobuf_decode_feature(ctx *ctx)
{
	LWDEBUGF(2, "reading size prefix at %ld", ctx->offset);
	auto size = flatbuffers::GetPrefixedSize(ctx->buf + ctx->offset);
	LWDEBUGF(2, "size is %ld (without size prefix)", size);

	Verifier verifier(ctx->buf + ctx->offset, size);
	if (VerifySizePrefixedFeatureBuffer(verifier)) {
		lwerror("buffer did not pass verification");
		return -1;
	}

	ctx->offset += sizeof(uoffset_t);

	auto feature = GetFeature(ctx->buf + ctx->offset);
	ctx->offset += size;

	const auto geometry = feature->geometry();
	if (geometry != nullptr) {
		LWDEBUGF(3, "Constructing GeometryReader with geometry_type %d has_z %d haz_m %d", ctx->geometry_type, ctx->has_z, ctx->has_m);
		GeometryReader reader(geometry, (GeometryType) ctx->geometry_type, ctx->has_z, ctx->has_m);
		ctx->lwgeom = reader.read();
		if (ctx->srid > 0)
			lwgeom_set_srid(ctx->lwgeom, ctx->srid);
		LWDEBUGG(3, ctx->lwgeom, "GeometryReader output LWGEOM");
	} else {
		ctx->lwgeom = NULL;
	}
	if (feature->properties() != nullptr && feature->properties()->size() != 0) {
		ctx->properties = (uint8_t *) feature->properties()->data();
		ctx->properties_len = feature->properties()->size();
	} else {
		ctx->properties_len = 0;
	}

	return 0;
}

int flatgeobuf_decode_header(ctx *ctx)
{
	LWDEBUGF(2, "reading size prefix at %ld", ctx->offset);
	auto size = flatbuffers::GetPrefixedSize(ctx->buf + ctx->offset);
	LWDEBUGF(2, "size is %ld (without size prefix)", size);

	Verifier verifier(ctx->buf + ctx->offset, size);
	if (VerifySizePrefixedHeaderBuffer(verifier)) {
		lwerror("buffer did not pass verification");
		return -1;
	}

	ctx->offset += sizeof(uoffset_t);

	LWDEBUGF(2, "reading header at %ld with size %ld", ctx->offset, size);
	auto header = GetHeader(ctx->buf + ctx->offset);
	ctx->offset += size;

	ctx->geometry_type = (uint8_t) header->geometry_type();
	ctx->features_count = header->features_count();
	ctx->has_z = header->has_z();
	ctx->has_m = header->has_m();
	ctx->has_t = header->has_t();
	ctx->has_tm = header->has_tm();
	ctx->index_node_size = header->index_node_size();
	auto crs = header->crs();
	if (crs != nullptr)
		ctx->srid = crs->code();
	auto columns = header->columns();
	if (columns != nullptr) {
		auto size = columns->size();
		ctx->columns = (flatgeobuf_column **) lwalloc(sizeof(flatgeobuf_column *) * size);
		ctx->columns_size = size;
		for (uint32_t i = 0; i < size; i++) {
			auto column = columns->Get(i);
			ctx->columns[i] = (flatgeobuf_column *) lwalloc(sizeof(flatgeobuf_column));
			memset(ctx->columns[i], 0, sizeof(flatgeobuf_column));
			ctx->columns[i]->name = column->name()->c_str();
			ctx->columns[i]->type = (uint8_t) column->type();
		}
	}

	LWDEBUGF(2, "ctx->geometry_type: %d", ctx->geometry_type);
	LWDEBUGF(2, "ctx->columns_len: %d", ctx->columns_size);

	if (ctx->index_node_size > 0 && ctx->features_count > 0) {
		auto treeSize = PackedRTree::size(ctx->features_count, ctx->index_node_size);
		LWDEBUGF(2, "Adding tree size %ld to offset", treeSize);
		ctx->offset += treeSize;
	}

	return 0;
}