## 2.5.1.2+carto-1

**Release date**: 2019-XX-XX

Changes:
- carto-package.json: Declare internal package compatibility.
- Merged community `svn-2.5` as of 2019-02-12.
- Backport wagyu introduction ([5508a4f](https://github.com/postgis/postgis/commit/5508a4f89c20686a19f233ef0a04b796d8a2cbaa)).

## 2.5.1.1+carto-1

**Release date**: 2019-01-31

Changes:
- Start from `svn-2.5` ([427ed71](https://github.com/postgis/postgis/commit/427ed71c10683892d4f6b3f0898da9b5745562b4)).
- Backport .travis from trunk (Sanitizers, multiple builds).
- Backport [24efadd](https://github.com/postgis/postgis/commit/24efadd48d94ae7d6ce2aaab66ca940ab97a0a14): GCC warnings.
- Backport clang-format.
- Ported (2.4.1.1+carto-1): make_cartodb_dist.sh.
- Ported (2.4.1.2+carto-1): Strip decorators from 'sql' functions so inlining works better.
- Ported (2.4.1.2+carto-1 / 2.4.1.4+carto-1): Add explicit costs to expensive and other functions.
- Ported (trunk [e5c92b18f](https://github.com/postgis/postgis/commit/e5c92b18ffad323b3996fd68f0b23f80dc5bca28)): ST_AsMVTGeom: Drop geometries smaller than the resolution.
- Add carto-package.json.
- Merged community `svn-2.5` as of 2019-01-25 ([cb703716d](https://github.com/postgis/postgis/commit/fa3163d575b99abe430133909d2cd755c904e9c3)).
- Backport [789707ed2](https://github.com/postgis/postgis/commit/789707ed2c2e67c728cdc088de427f409379944b): ST_AsMVTGeom: Do resolution check before deserializing.
- Merged community `svn-2.5` as of 2019-01-31.
