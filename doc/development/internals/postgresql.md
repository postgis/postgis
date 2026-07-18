---
title: "PostgreSQL Internals for PostGIS Developers"
date: 2026-06-26
weight: 20
geekdocHidden: false
---

This page collects implementation notes for C code that crosses the PostgreSQL
extension boundary. Use [Memory management](memory.md) for allocator ownership
rules.

## PostgreSQL Function Boundary

PostgreSQL C functions use `PG_FUNCTION_INFO_V1(function_name)` before the
function definition so the backend can find the symbol in the shared library.
The function itself receives `PG_FUNCTION_ARGS`; use `PG_GETARG_*` macros to
read typed SQL arguments and `PG_RETURN_*` macros to return values.

Example:

```c
PG_FUNCTION_INFO_V1(multiply);
Datum
multiply(PG_FUNCTION_ARGS)
{
	int count = PG_GETARG_INT32(0);
	double factor = PG_GETARG_DOUBLE(1);

	PG_RETURN_DOUBLE(factor * count);
}
```

## Varlena and Detoasting

Variable-length PostgreSQL values are varlena objects. Their payload is
preceded by a metadata header, and large values may be stored out-of-line by
TOAST. Use `PG_DETOAST_DATUM(PG_GETARG_DATUM(n))` before directly accessing a
PostGIS geometry or geography argument. The returned pointer may be either a
copy or a pointer into PostgreSQL storage, so release it with
`PG_FREE_IF_COPY(pointer, argument_number)` before returning.

For PostgreSQL `text`, allocate space for the varlena header and payload, set
the total size with `SET_VARSIZE`, and copy bytes into `VARDATA`.

## LWGEOM, POINTARRAY, and Serialized Forms

`LWGEOM` is the in-memory geometry structure used by PostGIS algorithms. Each
geometry has a type, flags, optional bounding box, and type-specific payload.
`LWPOINT`, `LWLINE`, `TRIANGLE`, and `LWCIRCSTRING` reference a `POINTARRAY`.
Collections reference arrays of child `LWGEOM` objects.

`POINTARRAY` coordinate storage may be owned by the `POINTARRAY` or may be a
read-only reference into a serialized database object. Use the provided
constructors and free with `ptarray_free()` or `lwgeom_free()` so read-only
coordinate storage is handled correctly.

`GSERIALIZED` is the PostgreSQL varlena representation used by current geometry
storage. Convert between serialized database values and algorithm structures
with:

```c
LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
GSERIALIZED *result = geometry_serialize(lwgeom);
```

Free `LWGEOM` objects with `lwgeom_free()` when ownership is complete. Use
`lwgeom_release()` only when you need to release the wrapper structure while
leaving child geometries or point arrays owned elsewhere.

## Typical SQL Function Flow

A PostgreSQL-facing geometry function usually follows this order:

1. Detoast input varlena arguments.
2. Convert serialized inputs to `LWGEOM`.
3. Validate invariants such as matching SRIDs.
4. Run the algorithm.
5. Serialize the result.
6. Free owned `LWGEOM` values.
7. Call `PG_FREE_IF_COPY` on detoasted inputs.
8. Return the serialized result.

Keep this order explicit in code review. It prevents leaks in utility contexts,
keeps large SQL calls from accumulating unnecessary memory, and makes ownership
clear to future maintainers.
