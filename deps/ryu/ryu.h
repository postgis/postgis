// Copyright 2018 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.
#ifndef RYU_H
#define RYU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

/* Print the shortest representation of a double using fixed notation
 * Only works for numbers smaller than 1e+17 (absolute value)
 * Precision limits the amount of digits of the decimal part
 */
int d2sfixed_buffered_n(double f, uint32_t precision, char* result);

/* Print the shortest representation of a double using scientific notation
 * Precision limits the amount of digits of the decimal part
 */
int d2sexp_buffered_n(double f, uint32_t precision, char* result);

#ifdef __cplusplus
}
#endif

#endif // RYU_H
