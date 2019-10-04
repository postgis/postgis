
/**
** To pass the internal state of our collection between the
** transfn and finalfn we need to wrap it into a custom type first,
** the CollectionBuildState type in our case.  The extra "data" member
** can optionally be used to pass additional constant
** arguments to a finalizer function.
*/
#define CollectionBuildStateDataSize 2
typedef struct CollectionBuildState
{
	List *geoms;  /* collected geometries */
	Datum data[CollectionBuildStateDataSize];
	Oid geomOid;
} CollectionBuildState;
