#ifndef _LWPGIS_DEFINES
#define _LWPGIS_DEFINES

#define CREATEFUNCTION CREATE OR REPLACE FUNCTION

#if USE_VERSION > 72
# define _IMMUTABLE_STRICT IMMUTABLE STRICT
# define _IMMUTABLE IMMUTABLE
# define _STABLE_STRICT STABLE STRICT
# define _STABLE STABLE
# define _VOLATILE_STRICT VOLATILE STRICT
# define _VOLATILE VOLATILE
# define _STRICT STRICT
# define _SECURITY_DEFINER SECURITY DEFINER
#else 
# define _IMMUTABLE_STRICT  with(iscachable,isstrict)
# define _IMMUTABLE with(iscachable)
# define _STABLE_STRICT with(isstrict)
# define _STABLE 
# define _VOLATILE_STRICT with(isstrict)
# define _VOLATILE 
# define _STRICT with(isstrict)
# define _SECURITY_DEFINER 
#endif 

#if USE_VERSION >= 73
# define HAS_SCHEMAS 1
#endif

#endif /* _LWPGIS_DEFINES */
