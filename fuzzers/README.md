# PostGIS Fuzzers

This directory contains fuzzer entry points and helper scripts for the
[Google OSS-Fuzz project](https://github.com/google/oss-fuzz/).

## OSS-Fuzz Build Entrypoint

The main OSS-Fuzz build orchestration lives in:

- `./fuzzers/build_oss_fuzz.sh`

The wrapper in
[`oss-fuzz/projects/postgis`](https://github.com/google/oss-fuzz/tree/master/projects/postgis)
should call this script.

## OSS-Fuzz Image Dependencies

The package list for the OSS-Fuzz builder image lives in:

- `./fuzzers/install_oss_fuzz_build_deps.sh`

The `oss-fuzz` project `Dockerfile` should call this script during image build.

## Issues

- [PostGIS OSS-Fuzz issue list](https://issues.oss-fuzz.com/issues?q=postgis)

## Local Workflow

### Simulate Dummy Fuzzer Build

```bash
make dummyfuzzers
```

Artifacts are created in `/tmp`:

- `/tmp/*_fuzzer`
- `/tmp/*_fuzzer_seed_corpus.zip`
- Runtime shared libraries in `/tmp/lib` (when available)

Run one fuzzer locally:

```bash
/tmp/wkt_import_fuzzer a_file_name
```

### Run OSS-Fuzz Locally

```bash
git clone --depth=1 https://github.com/google/oss-fuzz.git
cd oss-fuzz
python infra/helper.py build_image postgis
python infra/helper.py build_fuzzers --sanitizer address postgis
python infra/helper.py run_fuzzer postgis wkt_import_fuzzer
```

## Handling Reported OSS-Fuzz Issues

1. Leave a comment in the bug entry to indicate that you are working on it.
2. Implement the fix.
3. Commit with a message including `Credit to OSS-Fuzz` and a link to the OSS-Fuzz ticket.
4. Add a link to the implementing changeset/commit in the OSS-Fuzz ticket.
5. Verify OSS-Fuzz closes the bug (usually within a day or two).
