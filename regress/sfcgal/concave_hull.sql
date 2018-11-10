---
--- SFCGAL backend tests based on GEOS/JTS implemented functions
---
---

SET postgis.backend = 'sfcgal';

\cd :regdir
\i core/concave_hull.sql
