/* h9_lwgeom_shim.h — runtime resolution of liblwgeom entry points.
 *
 * WHY THIS EXISTS
 * ---------------
 * postgis_hex9 produces and consumes GSERIALIZED geometries, so it calls a
 * handful of liblwgeom functions (ptarray_construct, lwpoly_construct,
 * gserialized_from_lwgeom, …).  Those symbols live inside the PostGIS module
 * (postgis-3), which on macOS is a Mach-O *bundle* that cannot be linked
 * against, and on Linux is a regular module that nonetheless must already be
 * resident in the backend.  If we carried these as ordinary undefined symbols,
 * the dynamic loader would have to resolve them when *our* module is dlopen'd —
 * which is before any of our code runs — and that fails in any backend where a
 * PostGIS function has not yet been called.  (That is the "call a PostGIS
 * function first / SET shared_preload_libraries" workaround.)
 *
 * HOW IT FIXES IT
 * ---------------
 * We reference every liblwgeom function through a function pointer instead of
 * by name, so our module has ZERO undefined liblwgeom symbols and loads in any
 * order.  The pointers are filled at runtime by h9_lwgeom_resolve(), which must
 * be called once from _PG_init BEFORE any liblwgeom-backed function runs.
 * Resolution uses load_file("$libdir/postgis-3") — PostgreSQL's own loader, so
 * PostGIS's _PG_init runs and wires liblwgeom's lwalloc/lwfree handlers to
 * palloc/pfree (a raw dlopen would NOT do this and ptarray_construct would
 * allocate outside the memory context) — and then dlsym(RTLD_DEFAULT, …) pulls
 * each symbol from the now-global namespace.
 *
 * USAGE
 * -----
 *   #include "liblwgeom.h"          // types must be known first
 *   #include "h9_lwgeom_shim.h"     // typedefs + pointers + resolve()
 *   ... define h9_lwgeom_resolve() body (it uses load_file/dlsym/ereport) ...
 *   ... THEN the redirect #defines (see lwgeom_hex9.cpp) map the natural names
 *       onto the pointers for all subsequent call sites ...
 *
 * The redirect macros are intentionally NOT defined here: they must come after
 * the resolve() body so that body can use the real dlsym names, and after every
 * liblwgeom prototype so those declarations are left untouched.
 */
#pragma once

#include <dlfcn.h>
#include <cstddef>   /* size_t */
#include <cstdint>   /* int32_t, uint32_t */

/* Function-pointer typedefs — signatures mirror liblwgeom.h (3.x).  If a future
 * PostGIS changes one of these signatures, update it here in lockstep. */
typedef int          (*h9lw_fp_getPoint4d_p)(const POINTARRAY *, uint32_t, POINT4D *);
typedef GSERIALIZED *(*h9lw_fp_gserialized_from_lwgeom)(LWGEOM *, size_t *);
typedef int          (*h9lw_fp_lwgeom_calculate_gbox)(const LWGEOM *, GBOX *);
typedef void         (*h9lw_fp_lwgeom_free)(LWGEOM *);
typedef LWGEOM      *(*h9lw_fp_lwgeom_from_gserialized)(const GSERIALIZED *);
typedef void         (*h9lw_fp_lwpoint_free)(LWPOINT *);
typedef LWPOINT     *(*h9lw_fp_lwpoint_make2d)(int32_t, double, double);
typedef LWPOLY      *(*h9lw_fp_lwpoly_construct)(int32_t, GBOX *, uint32_t, POINTARRAY **);
typedef int          (*h9lw_fp_lwpoly_contains_point)(const LWPOLY *, const POINT2D *);
typedef void         (*h9lw_fp_lwpoly_free)(LWPOLY *);
typedef POINTARRAY  *(*h9lw_fp_ptarray_construct)(char, char, uint32_t);
typedef void         (*h9lw_fp_ptarray_set_point4d)(POINTARRAY *, uint32_t, const POINT4D *);

/* The resolved pointers.  C++17 inline variables: one definition, header-safe. */
inline h9lw_fp_getPoint4d_p            h9lw_getPoint4d_p            = nullptr;
inline h9lw_fp_gserialized_from_lwgeom h9lw_gserialized_from_lwgeom = nullptr;
inline h9lw_fp_lwgeom_calculate_gbox   h9lw_lwgeom_calculate_gbox   = nullptr;
inline h9lw_fp_lwgeom_free             h9lw_lwgeom_free             = nullptr;
inline h9lw_fp_lwgeom_from_gserialized h9lw_lwgeom_from_gserialized = nullptr;
inline h9lw_fp_lwpoint_free            h9lw_lwpoint_free            = nullptr;
inline h9lw_fp_lwpoint_make2d          h9lw_lwpoint_make2d          = nullptr;
inline h9lw_fp_lwpoly_construct        h9lw_lwpoly_construct        = nullptr;
inline h9lw_fp_lwpoly_contains_point   h9lw_lwpoly_contains_point   = nullptr;
inline h9lw_fp_lwpoly_free             h9lw_lwpoly_free             = nullptr;
inline h9lw_fp_ptarray_construct       h9lw_ptarray_construct       = nullptr;
inline h9lw_fp_ptarray_set_point4d     h9lw_ptarray_set_point4d     = nullptr;

/* Resolve every pointer above; ereport(ERROR) on any failure.  Call exactly
 * once, first thing in _PG_init.  Defined in lwgeom_hex9.cpp (needs PG's
 * load_file/ereport). */
extern "C" void h9_lwgeom_resolve(void);
