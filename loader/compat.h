#ifndef _COMPAT_H
#define _COMPAT_H 1

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#if USE_VERSION < 73
unsigned char *PQunescapeBytea(const unsigned char *strtext, size_t *retbuflen);
#endif

#endif // ifndef _COMPAT_H
