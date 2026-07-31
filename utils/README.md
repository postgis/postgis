test_estimation.pl
	test selectivity estimator for the && (overlap) operator

postgis_restore.pl
	restore a spatial database dump to a (possibly updated)
	new spatial database.

create_upgrade.pl
	Create a postgis procedures upgrade script. Will warn
	if operations is not cleanly possible due to nature
	of changes in postgis procedures.

profile_intersects.pl
	compares distance()=0 and intersects() timings.

ci-status.py
	Query repository-owned CI inventory from utils/ci-status.json and
	print a compact status report.  It queries GitHub Actions, GitLab,
	Woodpecker, and Jenkins JSON APIs directly.  The default command checks
	the configured non-EOL branches:

		python3 utils/ci-status.py

	Use `--branch stable-3.6` to inspect one branch, `--format json` for
	machine-readable output, `--format html` for static dashboard output,
	`--verbose` when a passing-check matrix is useful, and `--include-eol` to
	include configured EOL branches.  The default colored report summarizes
	passing, failing, running, and unknown checks in a compact branch table,
	but only expands non-OK details.  Use `--no-color` for plain logs.  HTTP
	provider checks run concurrently by default through asyncio.
	GitHub API access works without a token for light local use; frequent cron
	jobs may set `GITHUB_TOKEN` or `GH_TOKEN` for higher rate limits.  If the
	GitHub API is unavailable or rate-limited, GitHub workflow checks fall
	back to the public SVG badge status for a fresher unauthenticated answer.
	Problem-check detail headings include the provider link, using terminal
	hyperlinks when available and plain URLs otherwise.

	Generate the static dashboard for publishing with:

		python3 utils/ci-status.py --format html --output-dir /var/www/postgis/ci

	For no-404 live publishing, make the live path a symbolic link to a release
	directory and switch it with:

		python3 utils/ci-status.py --format html --atomic-switch --output-dir /var/www/postgis/ci

	The script writes index.html and status.json atomically, so a publishing
	job can run periodically without leaving a half-written page behind.
	`--atomic-switch` writes a complete sibling directory first and then
	atomically replaces the output symlink, so the web server never observes the
	live CI path as missing during publication.  A red CI status is valid page
	content and does not make HTML generation fail.

	Add new CI by adding a check object to utils/ci-status.json.  Retire old
	CI by changing the check provider to `disabled`, setting `required` to
	false, and recording the reason in `message`.  Mark unsupported release
	branches with `"eol": true` so they stay out of the default report.

support-matrix.py
	Update, validate, and build repository-owned compatibility data for the
	public browser:

		python3 utils/support-matrix.py update
		python3 utils/support-matrix.py check
		python3 utils/support-matrix.py build compatibility.json

	`update` refreshes release, lifecycle, packaging, vendored-source, and
	compile-time feature-gate metadata in the generated fallback cache.  If an
	upstream format changes, it keeps the cached data and records a warning for
	the browser.  Reviewable project statements live in
	`doc/development/compatibility/data/matrix.json`; the compact `cache.json`
	is tracked but excluded from normal diffs.  The public `compatibility.json`
	browser payload is a disposable build artifact.  `check` validates both the
	source data and the fully generated browser model before anything is
	published.
