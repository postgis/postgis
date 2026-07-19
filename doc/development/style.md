---
title: "Coding Style Guidelines"
date: 2026-06-26
weight: 60
geekdocHidden: false
---

This page covers source formatting, comments, Doxygen comments, and naming
conventions. For broader developer workflow notes, including release policy,
SQL API upgrade rules, and the `Availability:`, `Changed:`, and `Replaces`
comments parsed by the upgrade script generator, see
[PostGIS Development Guide](README.md).

## Preamble

PostGIS was created over many years, by many different people, some in a
hurry. As a result, the existing coding style is all over the map. We
recognize this, and understand it will be the work of many years before
PostGIS has a consistently named internal set of functions, but,
we can dream.

If new functions follow this guideline, if we do a little renovation work
from time to time, we will eventually get there.

## Documentation Voice

Write contributor documentation in plain, direct English. Prefer short
sentences, concrete commands, and links to the maintained page for deeper
context. Explain the purpose of a step before listing exceptions.

Avoid preserving old wiki phrasing when it hides the current source of truth.
When converting older notes, keep the useful idea and replace stale paths,
release numbers, chat networks, or one-off examples with current repository
links.

## Formatting

Most C code should use an ANSI standard formatting with tabs for block
indenting. When not block indenting, use spaces. To convert a file
to the standard format use:

  astyle --style=ansi --indent=tab

Some files use space indenting instead (check .editorconfig for info).
For them, you can use:

  astyle --style=ansi --indent=spaces=2

Do not get too happy with this command. If you want to re-format a file you
are working on:

 a) run astyle on it
 b) commit
 c) do your work
 d) run astyle again
 e) commit

The idea is to avoid combining style-only commits and commits that change
logic, so the logic commits are easier to read.

Before submitting patches that modify C or C++ files, run clang-format only on
the relevant hunks to avoid unrelated churn:

```bash
git clang-format
```

The command formats staged changes by default. Use `git clang-format <commit>`
to reflow a specific range, or pass `--extensions c,cpp,h` when touching files
with non-standard suffixes. Validate that the resulting diff stays focused on
the code you touched.

Macros should be ALL_UPPERCASE.
Enumerations should be ALL_UPPERCASE.

Comments should be written in C style (/* .... */) and not C++ style (//)
When describing a function,  the description should be right above the function and should start with /**
This is so the function description can be picked up by the doxygen autodocumentor.  For example

```c
/**
 * Does something funny
 */
double funny_function(POINT2D *p1, POINT2D *p2, POINT2D *q){
	funny stuff here;
}
```

More advanced commenting

```c
/**
 * Does something funny
 *
 * @param p1       : first point
 * @param p2     : second point
 * @param q   : the quiet point
 *
 * @return a funny double number
 */
double funny_function(POINT2D *p1, POINT2D *p2, POINT2D *q){
	funny stuff here;
}
```

## Naming

For ./liblwgeom:

- Files should be named with an lw prefix.
- Functions should have an lw prefix (lw_get_point).
- Function names should use underscores, not camel case.
- Function names should indicate the geometry type of inputs
  if relevant (lwline_split)

For ./postgis:

- C functions called by the back-end directly (function(PG_FUNCTION_ARGS))
  should be named to match their most likely SQL alias. So
  the SQL ST_Distance(geometry) maps to the C function
  ST_Distance(PG_FUNCTION_ARG)
- C utility functions should be prefixed with pgis_ (lower case)
