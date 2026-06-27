
/* Opaque type to use in standardizer cache API */
typedef void *StdCache;

StdCache GetStdCache(FunctionCallInfo fcinfo);
bool IsInStdCache(StdCache STDCache, char *lextab, char *gaztab, char *rultab);
void AddToStdCache(StdCache cache, char *lextab, char *gaztab, char *rultab);
STANDARDIZER *GetStdFromStdCache(StdCache STDCache,  char *lextab, char *gaztab, char *rultab);

/*
 * This is the only interface external code should be calling
 * it will get the standardizer out of the cache, or
 * it will create a new one and save it in the cache
*/
STANDARDIZER *GetStdUsingFCInfo(FunctionCallInfo fcinfo, char *lextab, char *gaztab, char *rultab);

/*
 * Resolve a user-supplied table name through the catalog and return a
 * schema-qualified, properly-quoted identifier safe for embedding in SQL.
 * Returns NULL if the relation does not exist.
 */
char *resolve_and_quote_tabname(const char *tabname);
