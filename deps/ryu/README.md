# Ryū

Ryu generates the shortest decimal representation of a floating point number that maintains round-trip safety. That is, a correct parser can recover the exact original number. For example, consider the binary 64-bit floating point number 00111110100110011001100110011010. The stored value is exactly 0.300000011920928955078125. However, this floating point number is also the closest number to the decimal number 0.3, so that is what Ryu outputs.

This problem of generating the shortest possible representation was originally posed by White and Steele [1], for which they described an algorithm called "Dragon". It was subsequently improved upon with algorithms that also had dragon-themed names. I followed in the same vein using the japanese word for dragon, Ryu. In general, all these algorithms should produce identical output given identical input, and this is checked when running the benchmark program.

## Ryū Printf

Since Ryu generates the shortest decimal representation, it is not immediately suitable for use in languages that have printf-like facilities. In most implementations, printf provides three floating-point specific formatters, %f, %e, and %g:

The %f format prints the full decimal part of the given floating point number, and then appends as many digits of the fractional part as specified using the precision parameter.

The %e format prints the decimal number in scientific notation with as many digits after the initial digit as specified using the precision parameter.

The %g format prints either %f or %e format, whichever is shorter.

Ryu Printf implements %f and %e formatting in a way that should be drop-in compatible with most implementations of printf, although it currently does not implement any formatting flags other than precision.

## Implementation

The C implementation of Ryu comes from the ryu/ directory in the git repostitory (https://github.com/ulfjack/ryu).

We've only copied the necessary files, that is `d2s.c` and its headers. It is integrated in the project by generating a static library, libryu.la.


### Main considerations about the library

The library has one single function to connect to liblwgeom, lwprint_double, which matches the behaviour as closely as possible to the previous output, based on printf. We only use 2 functions from ryu:

  - d2sfixed_buffered_n: Prints a double with decimal notation.
  - d2sexp_buffered_n: Prints a double with exponential notation.

Both functions have been heavily modified to support a new precision parameter that limits the amount of digits that are included after the floating point. This precision parameter affects both the scientific and fixed notation and respects proper rounding (round to nearest, ties to even).

The output has also been changed to match the old behaviour:
- Uses "e+1" instead of E1.
- Uses "e-1" instead of E-1.
- Never outputs negative 0. It always returns "0".

### Dependency changelog

  - 2019-01-10 - [Ryu] Library extraction from https://github.com/ulfjack/ryu/tree/master/ryu. Added changes to remove trailing zeros from the output and minor changes to please ubsan.
  - 2020-07-20 - [Ryu] Switch from d2fixed/d2exp to d2sfixed/d2sexp, that is, using the shortest notation
