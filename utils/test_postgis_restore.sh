#!/bin/sh

set -eu

if test "$#" -ne 1; then
  echo "usage: $0 POSTGIS_RESTORE" >&2
  exit 2
fi

restore=$1
tmpdir=$(mktemp -d "${TMPDIR:-.}/postgis_restore.XXXXXX")
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM
mkdir "$tmpdir/bin"

cat > "$tmpdir/bin/pg_dump" <<'EOF'
#!/bin/sh
echo 'pg_dump (PostgreSQL) test'
EOF

cat > "$tmpdir/bin/pg_restore" <<'EOF'
#!/bin/sh
if test "${1-}" = '--version'; then
  echo 'pg_restore (PostgreSQL) test'
  exit 0
fi

for arg in "$@"; do
  if test "$arg" = '-l'; then
    exit 0
  fi
done

echo 'CREATE TABLE restore_failure_test (id integer);'
exit 1
EOF
chmod +x "$tmpdir/bin/pg_dump" "$tmpdir/bin/pg_restore"
touch "$tmpdir/dump"

set +e
sed \
  -e 's/@SRID_MAXIMUM@/999999/g' \
  -e 's/@SRID_USER_MAXIMUM@/999999/g' \
  "$restore" | PATH="$tmpdir/bin:$PATH" perl - "$tmpdir/dump" > "$tmpdir/out" 2> "$tmpdir/err"
status=$?
set -e

if test "$status" -eq 0; then
  echo 'postgis_restore succeeded after pg_restore failed' >&2
  exit 1
fi

grep -F 'pg_restore returned an error' "$tmpdir/err" >/dev/null
if grep -F 'DROP TABLE _pgis_restore_spatial_ref_sys;' "$tmpdir/out" >/dev/null; then
  echo 'postgis_restore emitted trailing SQL after pg_restore failed' >&2
  exit 1
fi
