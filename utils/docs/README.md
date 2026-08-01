# Documentation utilities

Run these tools from the PostGIS repository root. User-facing commands and
standalone scripts stay at this level, implementation modules live in
subpackages, and unit tests live under `tests/`.

## Generated inventories

### CI status

`ci_status/` owns the checked-in CI inventory and the code that queries GitHub
Actions, GitLab, Woodpecker, and Jenkins:

```sh
python3 utils/docs/ci_status.py
python3 utils/docs/ci_status.py --branch stable-3.6
python3 utils/docs/ci_status.py --format json
python3 utils/docs/ci_status.py --format json --output status.json
```

* `ci_status/config.json` is the reviewable inventory of branches and checks.
* `ci_status/report.py` contains provider collection, status reduction, and
  terminal and JSON report generation.
* `ci_status/cli.py` owns command-line parsing and process exit codes.
* `tests/test_ci_status.py` covers provider responses, cache safety, status
  summaries, and JSON publication.

Add a CI service or branch in `config.json`. Retire a service by changing its
provider to `disabled`, setting `required` to false, and recording the reason in
`message`; mark unsupported release branches with `"eol": true`. See the
[CI inventory guide](../../doc/development/testing/ci.md) for the maintenance
rules.

### Compatibility support matrix

`support_matrix/` refreshes, validates, and exports the repository-owned
compatibility model:

```sh
python3 utils/docs/support_matrix.py update
python3 utils/docs/support_matrix.py check
python3 utils/docs/support_matrix.py build compatibility.json
```

* `support_matrix/update.py` refreshes source-derived metadata and preserves
  last-known-good cache sections when an upstream source fails.
* `support_matrix/cli.py` resolves and validates compatibility statements.
* `support_matrix/payload.py` builds and validates the presentation-ready
  browser payload.

The reviewable data and fallback cache live under
`doc/development/compatibility/data/`; `compatibility.json` is disposable and
must not be committed. See the [support matrix maintenance guide](../../doc/development/compatibility/)
for the data contract and website handoff.

## DocBook and manual checks

* `docbook_qa.py` and `xml_tree.py` validate source and generated DocBook/HTML.
* `postgis_exampletest.py` classifies and runs examples from the manual.
* `check_localized_cheatsheets.sh` checks translated cheat sheets.
* `fix_xml_entities.sh` normalizes malformed entity whitespace in translations.
* `check_contributor_credits.py` checks Git and `NEWS` contributors against the
  manual credits.
* `check_news.sh` validates release ordering, duplicate headings, and changed
  release notes.

The `tests/test_*.py` files exercise these tools' parsers and failure paths.
Build and CI callers use the same paths; when moving a tool, update those
callers in the same change.
