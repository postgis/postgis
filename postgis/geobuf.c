#include "geobuf.h"

void tupdesc_analyze(char ***keys, uint32_t **properties, char *geom_name);
int get_geom_index(char *geom_name);
LWGEOM *get_lwgeom(int row, char *geom_name);

Data__Feature *encode_feature(int row, char *geom_name, uint32_t *properties);
void encode_properties(int row, Data__Feature *feature, uint32_t *properties, char *geom_name);

Data__Geometry *encode_geometry(LWGEOM *lwgeom);
Data__Geometry *encode_point(LWPOINT *lwgeom);
Data__Geometry *encode_line(LWLINE *lwline);
Data__Geometry *encode_poly(LWPOLY* lwpoly);
Data__Geometry *encode_mpoint(LWMPOINT *lwmgeom);
Data__Geometry *encode_mline(LWMLINE *lwmline);
Data__Geometry *encode_mpoly(LWMPOLY* lwmpoly);
Data__Geometry *encode_collection(LWCOLLECTION* lwcollection);

int64_t *encode_coords(POINTARRAY *pa, int64_t *coords, int len, int offset);

void tupdesc_analyze(char ***keys, uint32_t **properties, char *geom_name) {
    int i, c = 0;
    int natts = SPI_tuptable->tupdesc->natts;
    *keys = malloc(sizeof (char *) * (natts - 1));
    *properties = malloc(sizeof (uint32_t) * (natts - 1) * 2);
    for (i = 0; i < natts; i++) {
        char* key = SPI_tuptable->tupdesc->attrs[i]->attname.data;
        if (strcmp(key, geom_name) == 0) continue;
        (*keys)[c] = key;
        (*properties)[c * 2] = c;
        (*properties)[c * 2 + 1] = c;
        c++;
    }
}

void encode_properties(int row, Data__Feature *feature, uint32_t *properties, char *geom_name) {
    int i, c;
    Data__Value **values;
    int natts = SPI_tuptable->tupdesc->natts;
    values = malloc (sizeof (Data__Value *) * (natts - 1));
    c = 0;
    for (i = 0; i < natts; i++) {
        Data__Value *value;
        char *string_value;
        char *key;
        key = SPI_tuptable->tupdesc->attrs[i]->attname.data;
        if (strcmp(key, geom_name) == 0) continue;
        // bool isnull;
        // Datum datum;
        // datum = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, i + 1, &isnull);
        // TODO: Process datum depending on meta
        string_value = SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, i + 1);
        value = malloc (sizeof (Data__Value));
        data__value__init(value);
        value->value_type_case = DATA__VALUE__VALUE_TYPE_STRING_VALUE;
        value->string_value = string_value;
        values[c++] = value;
    }

    feature->n_values = natts - 1;
    feature->values = values;
    feature->n_properties = (natts - 1) * 2;
    feature->properties = properties;
}

int get_geom_index(char *geom_name) {
    for (int i = 0; i < SPI_tuptable->tupdesc->natts; i++) {
        char *key = SPI_tuptable->tupdesc->attrs[i]->attname.data;
        if (strcmp(key, geom_name) == 0) return i + 1;
    }
    return -1;
}

LWGEOM *get_lwgeom(int row, char *geom_name) {
    Datum datum;
    GSERIALIZED *geom;
    bool isnull;
    LWGEOM* lwgeom;
    datum = SPI_getbinval(SPI_tuptable->vals[row], SPI_tuptable->tupdesc, get_geom_index(geom_name), &isnull);
    geom = (GSERIALIZED *) PG_DETOAST_DATUM(datum);
    lwgeom = lwgeom_from_gserialized(geom);
    return lwgeom;
}

Data__Geometry *encode_point(LWPOINT *lwpoint) {
    int i, npoints;
    Data__Geometry *geometry;
    int64_t *coord;
    POINTARRAY *pa;

    geometry = malloc (sizeof (Data__Geometry));
    data__geometry__init(geometry);
    geometry->type = DATA__GEOMETRY__TYPE__POINT;

    pa = lwpoint->point;
    npoints = pa->npoints;

    if (npoints == 0) return geometry;

    coord = malloc(sizeof (int64_t) * npoints * 2);
    for (i = 0; i < npoints; i++) {
        const POINT2D *pt;
        pt = getPoint2d_cp(pa, i);
        coord[i * 2] = pt->x * 10e5;
        coord[i * 2 + 1] = pt->y * 10e5;
    }

    geometry->n_coords = npoints * 2;
    geometry->coords = coord;

    return geometry;
}

Data__Geometry *encode_mpoint(LWMPOINT *lwmpoint) {
    int i, n_lengths;
    POINTARRAY *pa;
    Data__Geometry *geometry;
    int64_t *coords = NULL;

    geometry = malloc (sizeof (Data__Geometry));
    data__geometry__init(geometry);
    geometry->type = DATA__GEOMETRY__TYPE__MULTIPOINT;

    n_lengths = lwmpoint->ngeoms;

    if (n_lengths == 0) return geometry;
    
    pa = ptarray_construct_empty(0, 0, n_lengths);

    for (i = 0; i < n_lengths; i++) {
        POINT4D pt;
        getPoint4d_p(lwmpoint->geoms[i]->point, 0, &pt);
        ptarray_append_point(pa, &pt, 0);
    }

    geometry->n_coords = n_lengths * 2;
    geometry->coords = encode_coords(pa, coords, n_lengths, 0);

    return geometry;
}

Data__Geometry *encode_line(LWLINE *lwline) {
    POINTARRAY *pa;
    Data__Geometry *geometry;

    geometry = malloc (sizeof (Data__Geometry));
    data__geometry__init(geometry);
    geometry->type = DATA__GEOMETRY__TYPE__LINESTRING;

    pa = lwline->points;

    if (pa->npoints == 0) return geometry;

    geometry->n_coords = pa->npoints * 2;
    geometry->coords = encode_coords(pa, NULL, pa->npoints, 0);

    return geometry;
}

Data__Geometry *encode_mline(LWMLINE *lwmline) {
    int i, cc, n_lengths;
    POINTARRAY *pa;
    Data__Geometry *geometry;
    int offset;
    uint32_t *lengths;
    int64_t *coords = NULL;

    geometry = malloc (sizeof (Data__Geometry));
    data__geometry__init(geometry);
    geometry->type = DATA__GEOMETRY__TYPE__MULTILINESTRING;

    n_lengths = lwmline->ngeoms;

    if (n_lengths == 0) return geometry;
    
    lengths = malloc (sizeof (uint32_t) * n_lengths);
    
    cc = 0;
    offset = 0;
    for (i = 0; i < n_lengths; i++) {
        pa = lwmline->geoms[i]->points;
        coords = encode_coords(pa, coords, pa->npoints, offset);
        offset += pa->npoints * 2;
        lengths[i] = pa->npoints;
        cc += offset;
    }

    if (n_lengths > 1) {
        geometry->n_lengths = n_lengths;
        geometry->lengths = lengths;
    }
    geometry->n_coords = cc;
    geometry->coords = coords;

    return geometry;
}

Data__Geometry *encode_poly(LWPOLY *lwpoly) {
    int i, cc, n_lengths;
    POINTARRAY *pa;
    Data__Geometry *geometry;
    int offset;
    uint32_t *lengths;
    int64_t *coords = NULL;

    geometry = malloc (sizeof (Data__Geometry));
    data__geometry__init(geometry);
    geometry->type = DATA__GEOMETRY__TYPE__POLYGON;

    n_lengths = lwpoly->nrings;

    if (n_lengths == 0) return geometry;
    
    lengths = malloc (sizeof (uint32_t) * n_lengths);
    
    cc = 0;
    offset = 0;
    for (i = 0; i < n_lengths; i++) {
        pa = lwpoly->rings[i];
        coords = encode_coords(pa, coords, pa->npoints - 1, offset);
        offset += (pa->npoints - 1) * 2;
        lengths[i] = pa->npoints - 1;
        cc += offset;
    }

    if (n_lengths > 1) {
        geometry->n_lengths = n_lengths;
        geometry->lengths = lengths;
    }
    geometry->n_coords = cc;
    geometry->coords = coords;

    return geometry;
}

Data__Geometry *encode_mpoly(LWMPOLY* lwmpoly) {
    int i, j, c, cc, n_lengths, n_rings;
    POINTARRAY *pa;
    Data__Geometry *geometry;
    int offset;
    uint32_t *lengths;
    int64_t *coords = NULL;

    geometry = malloc (sizeof (Data__Geometry));
    data__geometry__init(geometry);
    geometry->type = DATA__GEOMETRY__TYPE__MULTIPOLYGON;

    n_lengths = lwmpoly->ngeoms;

    if (n_lengths == 0) return geometry;
    
    // TODO: need to defer allocation
    lengths = malloc (sizeof (uint32_t) * 100);
    
    c = 0;
    cc = 0;
    offset = 0;
    lengths[c++] = n_lengths;
    for (i = 0; i < n_lengths; i++) {
        n_rings = lwmpoly->geoms[i]->nrings;
        lengths[c++] = n_rings;
        for (j = 0; j < n_rings; j++) {
            pa = lwmpoly->geoms[i]->rings[j];
            coords = encode_coords(pa, coords, pa->npoints - 1, offset);
            offset += (pa->npoints - 1) * 2;
            lengths[c++] = pa->npoints - 1;
            cc += offset;
        }
    }

    if (c > 1) {
        geometry->n_lengths = c;
        geometry->lengths = lengths;
    }
    geometry->n_coords = cc;
    geometry->coords = coords;

    return geometry;
}

Data__Geometry *encode_collection(LWCOLLECTION* lwcollection) {
    int i, n_lengths;
    Data__Geometry *geometry;
    Data__Geometry **geometries;

    geometry = malloc (sizeof (Data__Geometry));
    data__geometry__init(geometry);
    geometry->type = DATA__GEOMETRY__TYPE__GEOMETRYCOLLECTION;

    n_lengths = lwcollection->ngeoms;

    if (n_lengths == 0) return geometry;
    
    geometries = malloc (sizeof (Data__Geometry *) * n_lengths);
    for (i = 0; i < n_lengths; i++) {
        LWGEOM *lwgeom = lwcollection->geoms[i];
        Data__Geometry *geom = encode_geometry(lwgeom);
        geometries[i] = geom;
    }

    geometry->n_geometries = n_lengths;
    geometry->geometries = geometries;

    return geometry;
}

int64_t *encode_coords(POINTARRAY *pa, int64_t *coords, int len, int offset) {
    int i, c;
    int64_t *dimsum;

    dimsum = calloc(2, sizeof (int64_t));

    if (offset == 0) {
        coords = malloc(sizeof (int64_t) * len * 2);
    } else {
        // TODO: should not be offset * 20 but crash otherwise?
        coords = realloc(coords, sizeof (int64_t) * ((len * 2) + offset*20));
    }

    c = offset;
    for (i = 0; i < len; i++) {
        const POINT2D *pt;
        pt = getPoint2d_cp(pa, i);
        dimsum[0] += coords[c++] = pt->x * 10e5 - dimsum[0];
        dimsum[1] += coords[c++] = pt->y * 10e5 - dimsum[1];
    }
    return coords;
}

Data__Geometry *encode_geometry(LWGEOM *lwgeom) {
    int type = lwgeom->type;
    switch (type)
	{
	case POINTTYPE:
		return encode_point((LWPOINT*)lwgeom);
	case LINETYPE:
		return encode_line((LWLINE*)lwgeom);
	case POLYGONTYPE:
		return encode_poly((LWPOLY*)lwgeom);
	case MULTIPOINTTYPE:
		return encode_mpoint((LWMPOINT*)lwgeom);
	case MULTILINETYPE:
		return encode_mline((LWMLINE*)lwgeom);
	case MULTIPOLYGONTYPE:
		return encode_mpoly((LWMPOLY*)lwgeom);
	case COLLECTIONTYPE:
		return encode_collection((LWCOLLECTION*)lwgeom);
	default:
		lwerror("encode_geometry: '%s' geometry type not supported",
		        lwtype_name(type));
	}
    return NULL;
}

Data__Feature *encode_feature(int row, char *geom_name, uint32_t *properties) {
    Data__Feature *feature;
    LWGEOM *lwgeom;
    feature = malloc (sizeof (Data__Feature));
    data__feature__init(feature);
    lwgeom = get_lwgeom(row, geom_name);
    feature->geometry = encode_geometry(lwgeom);
    if (properties != NULL) {
        encode_properties(row, feature, properties, geom_name);
    }
    return feature;
}

void *encode_to_geobuf(size_t *len, char *geom_name) {
    int i, count;
    void *buf;
    char **keys;
    uint32_t *properties = NULL;
    Data data = DATA__INIT;
    Data__FeatureCollection feature_collection = DATA__FEATURE_COLLECTION__INIT;
    
    count = SPI_processed;

    if (SPI_tuptable->tupdesc->natts > 1) {
        tupdesc_analyze(&keys, &properties, geom_name);
        data.n_keys = SPI_tuptable->tupdesc->natts;
        data.keys = keys;
    }
    data.data_type_case = DATA__DATA_TYPE_FEATURE_COLLECTION;
    data.feature_collection = &feature_collection;
    feature_collection.n_features = count;
    feature_collection.features = malloc (sizeof (Data__Feature *) * count);
    for (i = 0; i < count; i++) {
        feature_collection.features[i] = encode_feature(i, geom_name, properties);
    }

    *len = data__get_packed_size(&data);
    buf = malloc(*len);
    data__pack(&data, buf);
    return buf;
}