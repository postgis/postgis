/*
 * january, 2017, ported to portable library by mikkelfj.
 * Based on dbgtools static assert counter, but with renamed macros.
 */

/*
	 dbgtools - platform independent wrapping of "nice to have" debug functions.

	 version 0.1, october, 2013
	
	 https://github.com/wc-duck/dbgtools

	 Copyright (C) 2013- Fredrik Kihlander

	 This software is provided 'as-is', without any express or implied
	 warranty.  In no event will the authors be held liable for any damages
	 arising from the use of this software.

	 Permission is granted to anyone to use this software for any purpose,
	 including commercial applications, and to alter it and redistribute it
	 freely, subject to the following restrictions:

	 1. The origin of this software must not be misrepresented; you must not
	    claim that you wrote the original software. If you use this software
	    in a product, an acknowledgment in the product documentation would be
	    appreciated but is not required.
	 2. Altered source versions must be plainly marked as such, and must not be
	    misrepresented as being the original software.
	 3. This notice may not be removed or altered from any source distribution.

	 Fredrik Kihlander
*/

/**
 * Auto-generated header implementing a counter that increases by each include of the file.
 * 
 * This header will define the macro __PSTATIC_ASSERT_COUNTER to be increased for each inclusion of the file.
 * 
 * It has been generated with 3 amount of digits resulting in the counter wrapping around after
 * 10000 inclusions.
 * 
 * Usage:
 * 
 * #include "this_header.h"
 * int a = __PSTATIC_ASSERT_COUNTER; // 0
 * #include "this_header.h"
 * int b = __PSTATIC_ASSERT_COUNTER; // 1
 * #include "this_header.h"
 * int c = __PSTATIC_ASSERT_COUNTER; // 2
 * #include "this_header.h"
 * int d = __PSTATIC_ASSERT_COUNTER; // 3
 */

#ifndef __PSTATIC_ASSERT_COUNTER
#  define __PSTATIC_ASSERT_COUNTER_0 0
#  define __PSTATIC_ASSERT_COUNTER_1
#  define __PSTATIC_ASSERT_COUNTER_2
#  define __PSTATIC_ASSERT_COUNTER_3
#  define __PSTATIC_ASSERT_COUNTER_D1_0
#  define __PSTATIC_ASSERT_COUNTER_D2_0
#  define __PSTATIC_ASSERT_COUNTER_D3_0
#endif /* __PSTATIC_ASSERT_COUNTER */

#if !defined( __PSTATIC_ASSERT_COUNTER_D0_0 )
#  define __PSTATIC_ASSERT_COUNTER_D0_0
#  undef  __PSTATIC_ASSERT_COUNTER_0
#  define __PSTATIC_ASSERT_COUNTER_0 0
#elif !defined( __PSTATIC_ASSERT_COUNTER_D0_1 )
#  define __PSTATIC_ASSERT_COUNTER_D0_1
#  undef  __PSTATIC_ASSERT_COUNTER_0
#  define __PSTATIC_ASSERT_COUNTER_0 1
#elif !defined( __PSTATIC_ASSERT_COUNTER_D0_2 )
#  define __PSTATIC_ASSERT_COUNTER_D0_2
#  undef  __PSTATIC_ASSERT_COUNTER_0
#  define __PSTATIC_ASSERT_COUNTER_0 2
#elif !defined( __PSTATIC_ASSERT_COUNTER_D0_3 )
#  define __PSTATIC_ASSERT_COUNTER_D0_3
#  undef  __PSTATIC_ASSERT_COUNTER_0
#  define __PSTATIC_ASSERT_COUNTER_0 3
#elif !defined( __PSTATIC_ASSERT_COUNTER_D0_4 )
#  define __PSTATIC_ASSERT_COUNTER_D0_4
#  undef  __PSTATIC_ASSERT_COUNTER_0
#  define __PSTATIC_ASSERT_COUNTER_0 4
#elif !defined( __PSTATIC_ASSERT_COUNTER_D0_5 )
#  define __PSTATIC_ASSERT_COUNTER_D0_5
#  undef  __PSTATIC_ASSERT_COUNTER_0
#  define __PSTATIC_ASSERT_COUNTER_0 5
#elif !defined( __PSTATIC_ASSERT_COUNTER_D0_6 )
#  define __PSTATIC_ASSERT_COUNTER_D0_6
#  undef  __PSTATIC_ASSERT_COUNTER_0
#  define __PSTATIC_ASSERT_COUNTER_0 6
#elif !defined( __PSTATIC_ASSERT_COUNTER_D0_7 )
#  define __PSTATIC_ASSERT_COUNTER_D0_7
#  undef  __PSTATIC_ASSERT_COUNTER_0
#  define __PSTATIC_ASSERT_COUNTER_0 7
#elif !defined( __PSTATIC_ASSERT_COUNTER_D0_8 )
#  define __PSTATIC_ASSERT_COUNTER_D0_8
#  undef  __PSTATIC_ASSERT_COUNTER_0
#  define __PSTATIC_ASSERT_COUNTER_0 8
#elif !defined( __PSTATIC_ASSERT_COUNTER_D0_9 )
#  define __PSTATIC_ASSERT_COUNTER_D0_9
#  undef  __PSTATIC_ASSERT_COUNTER_0
#  define __PSTATIC_ASSERT_COUNTER_0 9
#else
#  undef __PSTATIC_ASSERT_COUNTER_D0_1
#  undef __PSTATIC_ASSERT_COUNTER_D0_2
#  undef __PSTATIC_ASSERT_COUNTER_D0_3
#  undef __PSTATIC_ASSERT_COUNTER_D0_4
#  undef __PSTATIC_ASSERT_COUNTER_D0_5
#  undef __PSTATIC_ASSERT_COUNTER_D0_6
#  undef __PSTATIC_ASSERT_COUNTER_D0_7
#  undef __PSTATIC_ASSERT_COUNTER_D0_8
#  undef __PSTATIC_ASSERT_COUNTER_D0_9
#  undef  __PSTATIC_ASSERT_COUNTER_0
#  define __PSTATIC_ASSERT_COUNTER_0 0
#  if !defined( __PSTATIC_ASSERT_COUNTER_D1_0 )
#    define __PSTATIC_ASSERT_COUNTER_D1_0
#    undef  __PSTATIC_ASSERT_COUNTER_1
#    define __PSTATIC_ASSERT_COUNTER_1 0
#  elif !defined( __PSTATIC_ASSERT_COUNTER_D1_1 )
#    define __PSTATIC_ASSERT_COUNTER_D1_1
#    undef  __PSTATIC_ASSERT_COUNTER_1
#    define __PSTATIC_ASSERT_COUNTER_1 1
#  elif !defined( __PSTATIC_ASSERT_COUNTER_D1_2 )
#    define __PSTATIC_ASSERT_COUNTER_D1_2
#    undef  __PSTATIC_ASSERT_COUNTER_1
#    define __PSTATIC_ASSERT_COUNTER_1 2
#  elif !defined( __PSTATIC_ASSERT_COUNTER_D1_3 )
#    define __PSTATIC_ASSERT_COUNTER_D1_3
#    undef  __PSTATIC_ASSERT_COUNTER_1
#    define __PSTATIC_ASSERT_COUNTER_1 3
#  elif !defined( __PSTATIC_ASSERT_COUNTER_D1_4 )
#    define __PSTATIC_ASSERT_COUNTER_D1_4
#    undef  __PSTATIC_ASSERT_COUNTER_1
#    define __PSTATIC_ASSERT_COUNTER_1 4
#  elif !defined( __PSTATIC_ASSERT_COUNTER_D1_5 )
#    define __PSTATIC_ASSERT_COUNTER_D1_5
#    undef  __PSTATIC_ASSERT_COUNTER_1
#    define __PSTATIC_ASSERT_COUNTER_1 5
#  elif !defined( __PSTATIC_ASSERT_COUNTER_D1_6 )
#    define __PSTATIC_ASSERT_COUNTER_D1_6
#    undef  __PSTATIC_ASSERT_COUNTER_1
#    define __PSTATIC_ASSERT_COUNTER_1 6
#  elif !defined( __PSTATIC_ASSERT_COUNTER_D1_7 )
#    define __PSTATIC_ASSERT_COUNTER_D1_7
#    undef  __PSTATIC_ASSERT_COUNTER_1
#    define __PSTATIC_ASSERT_COUNTER_1 7
#  elif !defined( __PSTATIC_ASSERT_COUNTER_D1_8 )
#    define __PSTATIC_ASSERT_COUNTER_D1_8
#    undef  __PSTATIC_ASSERT_COUNTER_1
#    define __PSTATIC_ASSERT_COUNTER_1 8
#  elif !defined( __PSTATIC_ASSERT_COUNTER_D1_9 )
#    define __PSTATIC_ASSERT_COUNTER_D1_9
#    undef  __PSTATIC_ASSERT_COUNTER_1
#    define __PSTATIC_ASSERT_COUNTER_1 9
#  else
#    undef __PSTATIC_ASSERT_COUNTER_D1_1
#    undef __PSTATIC_ASSERT_COUNTER_D1_2
#    undef __PSTATIC_ASSERT_COUNTER_D1_3
#    undef __PSTATIC_ASSERT_COUNTER_D1_4
#    undef __PSTATIC_ASSERT_COUNTER_D1_5
#    undef __PSTATIC_ASSERT_COUNTER_D1_6
#    undef __PSTATIC_ASSERT_COUNTER_D1_7
#    undef __PSTATIC_ASSERT_COUNTER_D1_8
#    undef __PSTATIC_ASSERT_COUNTER_D1_9
#    undef  __PSTATIC_ASSERT_COUNTER_1
#    define __PSTATIC_ASSERT_COUNTER_1 0
#    if !defined( __PSTATIC_ASSERT_COUNTER_D2_0 )
#      define __PSTATIC_ASSERT_COUNTER_D2_0
#      undef  __PSTATIC_ASSERT_COUNTER_2
#      define __PSTATIC_ASSERT_COUNTER_2 0
#    elif !defined( __PSTATIC_ASSERT_COUNTER_D2_1 )
#      define __PSTATIC_ASSERT_COUNTER_D2_1
#      undef  __PSTATIC_ASSERT_COUNTER_2
#      define __PSTATIC_ASSERT_COUNTER_2 1
#    elif !defined( __PSTATIC_ASSERT_COUNTER_D2_2 )
#      define __PSTATIC_ASSERT_COUNTER_D2_2
#      undef  __PSTATIC_ASSERT_COUNTER_2
#      define __PSTATIC_ASSERT_COUNTER_2 2
#    elif !defined( __PSTATIC_ASSERT_COUNTER_D2_3 )
#      define __PSTATIC_ASSERT_COUNTER_D2_3
#      undef  __PSTATIC_ASSERT_COUNTER_2
#      define __PSTATIC_ASSERT_COUNTER_2 3
#    elif !defined( __PSTATIC_ASSERT_COUNTER_D2_4 )
#      define __PSTATIC_ASSERT_COUNTER_D2_4
#      undef  __PSTATIC_ASSERT_COUNTER_2
#      define __PSTATIC_ASSERT_COUNTER_2 4
#    elif !defined( __PSTATIC_ASSERT_COUNTER_D2_5 )
#      define __PSTATIC_ASSERT_COUNTER_D2_5
#      undef  __PSTATIC_ASSERT_COUNTER_2
#      define __PSTATIC_ASSERT_COUNTER_2 5
#    elif !defined( __PSTATIC_ASSERT_COUNTER_D2_6 )
#      define __PSTATIC_ASSERT_COUNTER_D2_6
#      undef  __PSTATIC_ASSERT_COUNTER_2
#      define __PSTATIC_ASSERT_COUNTER_2 6
#    elif !defined( __PSTATIC_ASSERT_COUNTER_D2_7 )
#      define __PSTATIC_ASSERT_COUNTER_D2_7
#      undef  __PSTATIC_ASSERT_COUNTER_2
#      define __PSTATIC_ASSERT_COUNTER_2 7
#    elif !defined( __PSTATIC_ASSERT_COUNTER_D2_8 )
#      define __PSTATIC_ASSERT_COUNTER_D2_8
#      undef  __PSTATIC_ASSERT_COUNTER_2
#      define __PSTATIC_ASSERT_COUNTER_2 8
#    elif !defined( __PSTATIC_ASSERT_COUNTER_D2_9 )
#      define __PSTATIC_ASSERT_COUNTER_D2_9
#      undef  __PSTATIC_ASSERT_COUNTER_2
#      define __PSTATIC_ASSERT_COUNTER_2 9
#    else
#      undef __PSTATIC_ASSERT_COUNTER_D2_1
#      undef __PSTATIC_ASSERT_COUNTER_D2_2
#      undef __PSTATIC_ASSERT_COUNTER_D2_3
#      undef __PSTATIC_ASSERT_COUNTER_D2_4
#      undef __PSTATIC_ASSERT_COUNTER_D2_5
#      undef __PSTATIC_ASSERT_COUNTER_D2_6
#      undef __PSTATIC_ASSERT_COUNTER_D2_7
#      undef __PSTATIC_ASSERT_COUNTER_D2_8
#      undef __PSTATIC_ASSERT_COUNTER_D2_9
#      undef  __PSTATIC_ASSERT_COUNTER_2
#      define __PSTATIC_ASSERT_COUNTER_2 0
#      if !defined( __PSTATIC_ASSERT_COUNTER_D3_0 )
#        define __PSTATIC_ASSERT_COUNTER_D3_0
#        undef  __PSTATIC_ASSERT_COUNTER_3
#        define __PSTATIC_ASSERT_COUNTER_3 0
#      elif !defined( __PSTATIC_ASSERT_COUNTER_D3_1 )
#        define __PSTATIC_ASSERT_COUNTER_D3_1
#        undef  __PSTATIC_ASSERT_COUNTER_3
#        define __PSTATIC_ASSERT_COUNTER_3 1
#      elif !defined( __PSTATIC_ASSERT_COUNTER_D3_2 )
#        define __PSTATIC_ASSERT_COUNTER_D3_2
#        undef  __PSTATIC_ASSERT_COUNTER_3
#        define __PSTATIC_ASSERT_COUNTER_3 2
#      elif !defined( __PSTATIC_ASSERT_COUNTER_D3_3 )
#        define __PSTATIC_ASSERT_COUNTER_D3_3
#        undef  __PSTATIC_ASSERT_COUNTER_3
#        define __PSTATIC_ASSERT_COUNTER_3 3
#      elif !defined( __PSTATIC_ASSERT_COUNTER_D3_4 )
#        define __PSTATIC_ASSERT_COUNTER_D3_4
#        undef  __PSTATIC_ASSERT_COUNTER_3
#        define __PSTATIC_ASSERT_COUNTER_3 4
#      elif !defined( __PSTATIC_ASSERT_COUNTER_D3_5 )
#        define __PSTATIC_ASSERT_COUNTER_D3_5
#        undef  __PSTATIC_ASSERT_COUNTER_3
#        define __PSTATIC_ASSERT_COUNTER_3 5
#      elif !defined( __PSTATIC_ASSERT_COUNTER_D3_6 )
#        define __PSTATIC_ASSERT_COUNTER_D3_6
#        undef  __PSTATIC_ASSERT_COUNTER_3
#        define __PSTATIC_ASSERT_COUNTER_3 6
#      elif !defined( __PSTATIC_ASSERT_COUNTER_D3_7 )
#        define __PSTATIC_ASSERT_COUNTER_D3_7
#        undef  __PSTATIC_ASSERT_COUNTER_3
#        define __PSTATIC_ASSERT_COUNTER_3 7
#      elif !defined( __PSTATIC_ASSERT_COUNTER_D3_8 )
#        define __PSTATIC_ASSERT_COUNTER_D3_8
#        undef  __PSTATIC_ASSERT_COUNTER_3
#        define __PSTATIC_ASSERT_COUNTER_3 8
#      elif !defined( __PSTATIC_ASSERT_COUNTER_D3_9 )
#        define __PSTATIC_ASSERT_COUNTER_D3_9
#        undef  __PSTATIC_ASSERT_COUNTER_3
#        define __PSTATIC_ASSERT_COUNTER_3 9
#      else
#        undef __PSTATIC_ASSERT_COUNTER_D3_1
#        undef __PSTATIC_ASSERT_COUNTER_D3_2
#        undef __PSTATIC_ASSERT_COUNTER_D3_3
#        undef __PSTATIC_ASSERT_COUNTER_D3_4
#        undef __PSTATIC_ASSERT_COUNTER_D3_5
#        undef __PSTATIC_ASSERT_COUNTER_D3_6
#        undef __PSTATIC_ASSERT_COUNTER_D3_7
#        undef __PSTATIC_ASSERT_COUNTER_D3_8
#        undef __PSTATIC_ASSERT_COUNTER_D3_9
#        undef  __PSTATIC_ASSERT_COUNTER_3
#        define __PSTATIC_ASSERT_COUNTER_3 0
#      endif
#    endif
#  endif
#endif

#define __PSTATIC_ASSERT_COUNTER_JOIN_DIGITS_MACRO_(digit0,digit1,digit2,digit3) digit0##digit1##digit2##digit3
#define __PSTATIC_ASSERT_COUNTER_JOIN_DIGITS_MACRO(digit0,digit1,digit2,digit3) __PSTATIC_ASSERT_COUNTER_JOIN_DIGITS_MACRO_(digit0,digit1,digit2,digit3)
#undef  __PSTATIC_ASSERT_COUNTER
#define __PSTATIC_ASSERT_COUNTER __PSTATIC_ASSERT_COUNTER_JOIN_DIGITS_MACRO(__PSTATIC_ASSERT_COUNTER_3,__PSTATIC_ASSERT_COUNTER_2,__PSTATIC_ASSERT_COUNTER_1,__PSTATIC_ASSERT_COUNTER_0)
