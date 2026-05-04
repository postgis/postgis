---
name: sql-style
description: General SQL style, documentation, formatting, and maintenance hygiene. Use for SQL work that needs project-style guidance rather than PostGIS-specific spatial guidance.
---

## Documentation

 - Make sure every create statement or CTE has a descriptive comment `--` in front of it.
 - Write enough comments so you can deduce what was a requirement in the future and not walk in circles.
 - Every feature needs to have comprehensive up-to-date documentation near it.

## Style

 - SQL is lowercase unless instructed otherwise.
 - Do not mix tabs and spaces in code.
 - Add empty lines between logical blocks.
 - Format the code nicely and consistently.

## Debugging

 - Make sure error messages towards developers are better than just "500 Internal Server Error".
 - SQL files should be idempotent: drop table if exists + create table as; add comments to make queries faster to grasp.
 - Create both "up" and "down/rollback" migrations when the project expects reversible migrations.
 - Don't run one SQL file from another SQL file - this quickly becomes a mess with relative file paths.
