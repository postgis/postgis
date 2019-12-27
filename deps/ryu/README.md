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

The only .c file that we need is `d2fixed.c`, we copy them all from upstream so it's easier

It is integrated in the project by generating a static library, libryu.la, that is later integrated inside liblwgeom.la.


### Main considerations about the library

The library has one single function to connect to liblwgeom, lwprint_double, which matches the behaviour as closely as possible to the previous output, based on printf. We only use 2 functions from ryu:

  - d2fixed_buffered_n: Prints a double with decimal notation.
  - d2exp_buffered_n: Prints a double with exponential notation.

To speed up the removal of trailing zeros, extra code has been added to ryu itself since it already had the information necessary to do it. It has been proposed (https://github.com/ulfjack/ryu/issues/142), but it isn't included upstream yet. The full changediff is:

```diff
diff --git a/ryu/d2fixed.c b/ryu/d2fixed.c
index b008dab..3bcc22f 100644
--- a/ryu/d2fixed.c
+++ b/ryu/d2fixed.c
@@ -420,7 +420,8 @@ int d2fixed_buffered_n(double d, uint32_t precision, char* result) {
   if (!nonzero) {
     result[index++] = '0';
   }
-  if (precision > 0) {
+  const bool printDecimalPoint = precision > 0;
+  if (printDecimalPoint) {
     result[index++] = '.';
   }
 #ifdef RYU_DEBUG
@@ -532,6 +533,18 @@ int d2fixed_buffered_n(double d, uint32_t precision, char* result) {
     memset(result + index, '0', precision);
     index += precision;
   }
+
+#if RYU_NO_TRAILING_ZEROS
+  if (printDecimalPoint) {
+    while (result[index - 1] == '0') {
+      index--;
+    }
+    if (result[index - 1] == '.') {
+      index--;
+    }
+  }
+#endif
+
   return index;
 }

@@ -771,6 +784,18 @@ int d2exp_buffered_n(double d, uint32_t precision, char* result) {
       }
     }
   }
+
+#if RYU_NO_TRAILING_ZEROS
+  if (printDecimalPoint) {
+    while (result[index - 1] == '0') {
+      index--;
+    }
+    if (result[index - 1] == '.') {
+      index--;
+    }
+  }
+#endif
+
   result[index++] = 'e';
   if (exp < 0) {
     result[index++] = '-';
```

It is hidden behind `RYU_NO_TRAILING_ZEROS` so that macro has to be passed during compilation.

And to avoid ubsan warnings, the following changes have been introduced:
```diff
diff --git a/deps/ryu/d2fixed.c b/deps/ryu/d2fixed.c
index 3bcc22f48..b23ca17c4 100644
--- a/deps/ryu/d2fixed.c
+++ b/deps/ryu/d2fixed.c
@@ -402,10 +402,10 @@ int d2fixed_buffered_n(double d, uint32_t precision, char* result) {
     printf("len=%d\n", len);
 #endif
     for (int32_t i = len - 1; i >= 0; --i) {
-      const uint32_t j = p10bits - e2;
+      const int32_t j = ((int32_t) p10bits) - e2;
       // Temporary: j is usually around 128, and by shifting a bit, we push it to 128 or above, which is
       // a slightly faster code path in mulShift_mod1e9. Instead, we can just increase the multipliers.
-      const uint32_t digits = mulShift_mod1e9(m2 << 8, POW10_SPLIT[POW10_OFFSET[idx] + i], (int32_t) (j + 8));
+      const uint32_t digits = mulShift_mod1e9(m2 << 8, POW10_SPLIT[POW10_OFFSET[idx] + i], j + 8);
       if (nonzero) {
         append_nine_digits(digits, result + index);
         index += 9;
@@ -630,10 +630,10 @@ int d2exp_buffered_n(double d, uint32_t precision, char* result) {
     printf("len=%d\n", len);
 #endif
     for (int32_t i = len - 1; i >= 0; --i) {
-      const uint32_t j = p10bits - e2;
+      const int32_t j = ((int32_t) p10bits) - e2;
       // Temporary: j is usually around 128, and by shifting a bit, we push it to 128 or above, which is
       // a slightly faster code path in mulShift_mod1e9. Instead, we can just increase the multipliers.
-      digits = mulShift_mod1e9(m2 << 8, POW10_SPLIT[POW10_OFFSET[idx] + i], (int32_t) (j + 8));
+      digits = mulShift_mod1e9(m2 << 8, POW10_SPLIT[POW10_OFFSET[idx] + i], j + 8);
       if (printedDigits != 0) {
         if (printedDigits + 9 > precision) {
           availableDigits = 9;
```


### Dependency changelog

  - 2019-12-20 - [Ryu] Library extraction from https://github.com/ulfjack/ryu/tree/master/ryu: Extracts *.h and *.c files only, no subfolders. Added changes to remove trailing zeros from the output and minor changes to please ubsan.
