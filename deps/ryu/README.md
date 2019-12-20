# Ryū

Ryu generates the shortest decimal representation of a floating point number that maintains round-trip safety. That is, a correct parser can recover the exact original number. For example, consider the binary 64-bit floating point number 00111110100110011001100110011010. The stored value is exactly 0.300000011920928955078125. However, this floating point number is also the closest number to the decimal number 0.3, so that is what Ryu outputs.

This problem of generating the shortest possible representation was originally posed by White and Steele [1], for which they described an algorithm called "Dragon". It was subsequently improved upon with algorithms that also had dragon-themed names. I followed in the same vein using the japanese word for dragon, Ryu. In general, all these algorithms should produce identical output given identical input, and this is checked when running the benchmark program.

# Ryū Printf

Since Ryu generates the shortest decimal representation, it is not immediately suitable for use in languages that have printf-like facilities. In most implementations, printf provides three floating-point specific formatters, %f, %e, and %g:

The %f format prints the full decimal part of the given floating point number, and then appends as many digits of the fractional part as specified using the precision parameter.

The %e format prints the decimal number in scientific notation with as many digits after the initial digit as specified using the precision parameter.

The %g format prints either %f or %e format, whichever is shorter.

Ryu Printf implements %f and %e formatting in a way that should be drop-in compatible with most implementations of printf, although it currently does not implement any formatting flags other than precision.

# Implementation

The C implementation of Ryu comes from the ryu/ directory in the git repostitory (https://github.com/ulfjack/ryu)

It is integrated in the project by generating a static library, libryu.la, that is later integrated inside liblwgeom.la.



# Main considerations about the library

- We use 2 functions to print doubles:
  - d2fixed_buffered_n: TODO UPDATE THIS WHEN FINISHED
  - d2exp_buffered_n: TODO UPDATE THIS WHEN FINISHED


# Dependency changelog

  - 2019-12-20 - [Ryu] Library extraction from https://github.com/ulfjack/ryu/tree/master/ryu: Extracts *.h and *.c files only, no subfolders.
