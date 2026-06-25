# PostGIS Website Maintenance

The public website source lives in the OSGeo Gitea repository:

```sh
git clone https://gitea.osgeo.org/postgis/postgis.net.git
cd postgis.net
```

The site is a Hugo site using a customized `hugo-geekdoc` theme. Keep local
changes in the site content, layouts, shortcodes, static assets, or config; do
not edit the vendored theme for PostGIS-specific behavior.

Useful commands:

```sh
hugo server
hugo
make check
```

`hugo server` renders a local preview. Pages with `draft: true` will not appear
in normal builds, so set `draft: false` before expecting a page to show up.
Running `hugo` writes the generated site into `public/`.

`make check` currently runs `utils/check_releases_md5.sh`. That script reads the
published release list from `config.toml`, downloads each non-development source
tarball from `https://download.osgeo.org/postgis/source`, downloads the matching
`.md5` file from `https://postgis.net/stuff`, and verifies the checksum. It
uses `tomlq`, `curl`, and `md5sum`.

## Content Areas

Website source content that overlaps with repository-maintained developer docs:

| Website source | Repository-maintained destination |
|----------------|-----------------------------------|
| `content/development/source_code.md` | [Contributing workflow](../workflow/) and [Pull request and maintainer workflow](../maintenance/) |
| `content/development/getting_involved.md` | [Contributing workflow](../workflow/) |
| `content/development/bug_reporting.md` | [Contributing workflow](../workflow/) |
| `content/development/developer_docs.md` | [Documentation workflow](../manual/) and this `doc/development/` index |
| `content/development/versions_eol.md` | [Release process](../release-process/) and [Release and upgrade rules](../release/) |
| `content/development/rfcs/*.md` | [Governance notes](../governance/) and historical RFC pages |
| `content/community/mailinglists.md` and `content/community/chat.md` | [Contributing workflow](../workflow/) |
| `content/community/conduct.md` | [Governance notes](../governance/) |
| `content/documentation/manual.md` | [Documentation workflow](../manual/) |
| `content/documentation/training.md` | Public website; only the workshop repository is an umbrella project |

The website declares its content license as CC BY-SA 4.0 in `config.toml`.

## Release Updates

When publishing a PostGIS release, update the website after the source and
documentation artefacts exist:

1. Update `config.toml` under `[params.postgis]` and
   `[params.postgis.releases]` so the current release, development release, EOL
   markers, source links, documentation links, and release-note links match the
   freshly published artefacts.
2. Add a release news item under `content/news/<year>/`. The historical naming
   convention starts release filenames with `mm-dd`; Hugo can derive permalinks
   from slug/title/date, but the convention is still used for readability.
3. Run `make check` to verify release tarball checksums.
4. Push the website change to `https://gitea.osgeo.org/postgis/postgis.net` and
   wait for the website deployment.

See [Release process](../release-process/) for the broader release checklist.
