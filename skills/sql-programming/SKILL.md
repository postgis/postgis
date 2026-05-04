---
name: sql-programming
description: General SQL programming, formatting, debugging, and query-shaping tips. Use for SQL work that is not specifically about PostGIS spatial data.
---

## Documentation

 - Make sure every create statement or CTE has a descriptive comment `--` in front of it.
 - Write enough comments so you can deduce what was a requirement in the future and not walk in circles.
 - Every feature needs to have comprehensive up-to-date documentation near it.

## Style

 - SQL is lowercase unless instructed otherwise.
 - Values in databases and layers should be absolute as much as possible: store "birthday" or "construction date" instead of "age".
 - Do not mix tabs and spaces in code.
 - Add empty lines between logical blocks.
 - Format the code nicely and consistently.

## Indexing

 - Consider BRIN indexes for very large naturally ordered tables that will be used for ad-hoc range queries.
 - If you have a cache table that has a primary key, it can make sense to add frequently read values into `including` on the same index for faster lookup.

## Debugging

 - Make sure error messages towards developers are better than just "500 Internal Server Error".
 - Don't stub stuff out with insane fallbacks like `lat = 0` and `lon = 0`; instead make the rest of the code work around data absence and inform the user.
 - SQL files should be idempotent: drop table if exists + create table as; add comments to make queries faster to grasp.
 - Create both "up" and "down/rollback" migrations when the project expects reversible migrations.
 - Don't run one SQL file from another SQL file - this quickly becomes a mess with relative file paths.

## SQL Gotchas

 - `sum(case when A then 1 else 0 end)` is just `count() filter (where A)`.
 - `row_number() ... = 1` can likely be redone as `order by + limit 1` (possibly with `distinct on` or `lateral`).
 - `exists(select 1 from ...)` is just `exists(select from ...)`.
 - `tags ->> 'key' = 'value'` is just `tags @> '{"key": "value"}'` - works faster with indexes.
 - You can't create an ordered table and then rely on it to be ordered on scan without `order by`.
