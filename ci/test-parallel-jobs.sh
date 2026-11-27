#!/bin/sh

set -eu
cd "$(dirname "$0")/.."

check_jobs()
{
	expected=$1
	shift
	actual=$(env "$@" sh ci/parallel-jobs.sh)
	if test "${actual}" != "${expected}"; then
		echo "parallel-jobs.sh returned ${actual}, expected ${expected} ($*)" >&2
		exit 1
	fi
}

# Force enough memory for any CPU, then check the explicit job cap.
check_jobs 1 POSTGIS_CI_MAX_JOBS=1 POSTGIS_CI_MEM_AVAILABLE_KB=8388608 \
	POSTGIS_CI_JOB_MEMORY_MB=1024 POSTGIS_CI_MEMORY_RESERVE_MB=512
# Even a budget below the reserve must produce a valid make -j argument.
check_jobs 1 POSTGIS_CI_MAX_JOBS=8 POSTGIS_CI_MEM_AVAILABLE_KB=262144 \
	POSTGIS_CI_JOB_MEMORY_MB=1024 POSTGIS_CI_MEMORY_RESERVE_MB=512
