#!/usr/bin/env bash
set -e

curl -sSfL https://github.com/mapbox/logbt/archive/v2.0.3.tar.gz | sudo tar --gunzip --extract --strip-components=1 --exclude="*md" --exclude="test*" --directory=/usr/local/
curl -sSfL https://raw.githubusercontent.com/mapbox/logbt/30c554dd37b6c96c23fc424f75910fc6d6696f00/bin/logbt | sudo tee /usr/local/bin/logbt > /dev/null
sudo logbt --setp

WARNINGS="-Werror -Wall -Wextra -Wformat -Werror=format-security"
WARNINGS_DISABLED="-Wno-unused-parameter -Wno-implicit-fallthrough -Wno-unknown-warning-option -Wno-cast-function-type"

CFLAGS_STD="-g -O2 -mtune=generic -fno-omit-frame-pointer ${WARNINGS} ${WARNINGS_DISABLED}"
LDFLAGS_STD="-Wl,-Bsymbolic-functions -Wl,-z,relro"

CFLAGS_COV="-g -O0 --coverage"
LDFLAGS_COV="--coverage"

/usr/local/pgsql/bin/pg_ctl -c -l /tmp/logfile start
./autogen.sh

./configure CFLAGS="${CFLAGS_STD}" LDFLAGS="${LDFLAGS_STD}"
logbt -- make -j check "RUNTESTFLAGS='--dumprestore --verbose'"

./configure CFLAGS="${CFLAGS_COV}" LDFLAGS="${LDFLAGS_COV}"
make -j check
