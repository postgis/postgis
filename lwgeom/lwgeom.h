typedef struct LWGEOM_T LWGEOM;

// Serialization / deserialization
extern LWGEOM *lwgeom_deserialize(char *serializedform);
extern char *lwgeom_serialize(LWGEOM *lwgeom);
extern char *lwgeom_serialize_size(LWGEOM *lwgeom);
extern void lwgeom_serialize_buf(LWGEOM *lwgeom, char *buf);

extern void lwgeom_reverse(LWGEOM *lwgeom);
extern void lwgeom_forceRHR(LWGEOM *lwgeom);
