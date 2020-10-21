A small library for adding C11 compatibility to older C compilers, but
only a small highly useful subset such as static assertions, inline
functions and alignment.

C++ is not a primary target, but the library has been updated to be more
C++ friendly based on user feedback.

Many compilers already have the required functionality but with slightly
different names and arguments.

In addition, compatibility with the Linux `<endian.h>` system file is
provided, and "punaligned.h" is provided for unaligned memory reads
which in part depends on endian support.

The library also provides fast integer printing and floating point
printing and parsing optionally using the grisu3 algorithm, but can fall
back to strtod and related. The `pgrisu3` folder is header only and
excludes test cases found in the main grisu3 project the files were
extracted from. Base64 conversion is also provided.

Integer conversion is not just an optimization. It is more difficult
than it would appear to portably parse an integer of known size such as
`uint64_t` up to at most n bytes which is needed for safe parsing. At
the same time, the sometimes significant performance gains warrants
custom implementations that might as well be done once and for all.

Files can be included individually, or portable.h may be included to get
all functionality. If the compiler is C11 compliant, portable.h will not
include anything, except: it will provide a patch for static assertions
which clang does not fully support in all versions even with C11 flagged.

The grisu3 header files are the runtime files for the Grisu3 floating
point conversion to/from text C port. Test coverage is provided separately.
This library can be used indirectly via pparsefp.h and pprintfp.h.

The `pstatic_assert.h` file is often needed on C11 systems because the
compiler and standard library  may support `_Static_assert` without
`static_assert`. For compilers without `_Static_assert`, a unique
identifier is needed for each assertion. This is done non-standard with
the `__COUNTER__` macro, but has a fallback to `pstatic_assert_scope.h`
for systems witout the `__COUNTER__` macro. Because of this fallback,
`pstatic_assert.h` needs to be included in every file using
`static_assert` in order to increment a scope counter, otherwise there
is a risk of assert identifier conflicts when `static_assert` happen on
the same line in different files.

The `paligned_alloc.h` file implements the non-standard `aligned_free`
to match the C11 standard `aligned_alloc` call. `aligned_free`  is
normally equivalent to `free`, but not on systems where `aligned_free`
cannot be implemented using a system provived `free` call. Use of
`aligned_free` is thus optional on some systems, but using it increases
general portablity at the cost of pure C11 compatibility.

IMPORTANT NOTE: this library has been used on various platforms and
updated with user feedback but it is impossibly to systematically test
all platforms so please test for specific uses cases and report
any issues upstream.
