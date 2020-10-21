#ifndef PSTDBOOL_H
#define PSTDBOOL_H

#if !defined(__cplusplus) && !__bool_true_false_are_defined && !defined(bool) && !defined(__STDBOOL_H)

#ifdef HAVE_STDBOOL_H

#include <stdbool.h>

#elif (defined(__STDC__) && __STDC__ && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
/* C99 or newer */

#define bool _Bool
#define true 1
#define false 0
#define __bool_true_false_are_defined 1

#elif defined(__GNUC__) && !defined(__STRICT_ANSI__)

#define bool bool
#define true true
#define false false
#define __bool_true_false_are_defined 1

#else

typedef unsigned char _Portable_bool;
#define bool _Portable_bool
#define true 1
#define false 0
#define __bool_true_false_are_defined 1

#endif

#endif

#endif /* PSTDBOOL_H */
