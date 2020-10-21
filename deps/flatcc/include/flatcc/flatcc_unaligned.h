#ifndef FLATCC_UNLIGNED_H
#define FLATCC_UNLIGNED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "flatcc/portable/punaligned.h"

#define FLATCC_ALLOW_UNALIGNED_ACCESS PORTABLE_UNALIGNED_ACCESS

#ifdef __cplusplus
}
#endif

#endif /* FLATCC_UNLIGNED_H */
