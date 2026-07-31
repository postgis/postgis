# Compatibility data

`matrix.json` contains the reviewable project compatibility statements.
`cache.json` is the compact, generated last-known-good snapshot used when an
upstream source changes format or is unavailable. It stays tracked so a clean
checkout can still build the page, but Git treats it as generated and omits its
contents from ordinary diffs. `utils/support-matrix.py update` rewrites only
that cache and preserves a warning in the generated payload when it uses
fallback data.
Keep scalar arrays in `matrix.json` on one line so version-set changes stay
compact and reviewable.

`compatibility.json` is the generated presentation-ready payload consumed by the
website-owned compatibility page. It is intentionally not tracked here; build it
with:

```sh
python3 utils/support-matrix.py build compatibility.json
```
