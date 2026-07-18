---
title: "Memory Management"
date: 2026-06-26
weight: 10
geekdocHidden: false
---

Use the allocator that matches the layer you are editing:

| Area | Allocators |
|------|------------|
| `liblwgeom/` | `lwalloc`, `lwfree`, `lwrealloc` |
| `postgis/` PostgreSQL-facing code | `palloc`, `pfree`, `repalloc` |
| `raster/rt_core/` | `rtalloc`, `rtdealloc`, `rtrealloc` |
| `raster/rt_pg/` PostgreSQL-facing code | `palloc`, `pfree`, `repalloc` |

PostgreSQL runs SQL functions inside memory contexts and releases the context
when the call completes. Do not rely on that as a substitute for ownership
discipline. PostGIS functions can process large geometries and rasters inside a
single call, and `liblwgeom` code is also used by command-line utilities where
PostgreSQL memory contexts are not present.
