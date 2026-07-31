# Compatibility data

`matrix.json` contains the reviewable project compatibility statements.
`cache.json` is the compact, generated last-known-good snapshot used when an
upstream source changes format or is unavailable. It stays tracked so a clean
checkout can still build the page, but Git treats it as generated and omits its
contents from ordinary diffs. `utils/support-matrix.py update` rewrites only
that cache and preserves a warning for the browser when it uses fallback data.
Keep scalar arrays in `matrix.json` on one line so version-set changes stay
compact and reviewable.

`compatibility.json` is the generated browser payload. It is intentionally not
tracked here; the website build creates it with:

```sh
python3 utils/support-matrix.py build compatibility.json
```
