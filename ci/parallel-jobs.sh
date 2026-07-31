#!/bin/sh

# Helper used by PostGIS CI jobs to choose a safe level of parallelism without
# overcommitting memory.
# It was introduced to avoid CI runs being killed by the Linux OOM killer with
# code 137 on small shared agents (for example, geocint/smallcat/nucat runner
# nodes) when too many tests run simultaneously.

postgis_ci_positive_integer()
{
	case "$1" in
		''|*[!0-9]*|0) return 1 ;;
		*) return 0 ;;
	esac
}

postgis_ci_cpu_count()
{
	# CPU limit from host; only a fallback to 1 when detection fails.
	cpus=$(getconf _NPROCESSORS_ONLN 2>/dev/null || nproc 2>/dev/null || echo 1)
	if ! postgis_ci_positive_integer "${cpus}"; then
		cpus=1
	fi
	echo "${cpus}"
}

postgis_ci_cgroup_memory_kb()
{
	# Read /proc/self/cgroup and keep the last path component for the active
	# cgroup membership. For unified cgroup v2 containers this is usually the
	# only relevant hierarchy.
	cgroup_path=
	while IFS=: read -r _hierarchy _controllers _path; do
		cgroup_path=${_path}
	done < /proc/self/cgroup

	test -n "${cgroup_path}" || return 1

	if test "${POSTGIS_CI_TEST_CGROUP_ROOT:-}" != ""; then
		cgroup_root=${POSTGIS_CI_TEST_CGROUP_ROOT}
	else
		cgroup_root=/sys/fs/cgroup
	fi

	# memory.max is cgroup v2 memory limit in bytes. Value "max" means no
	# explicit cgroup limit was configured, so we can not compute bytes-to-KiB
	# here and must fall back to other sources.
	memory_max="${cgroup_root}${cgroup_path}/memory.max"
	test -r "${memory_max}" || return 1

	memory_bytes=$(cat "${memory_max}")
	case "${memory_bytes}" in
		''|max|*[!0-9]*) return 1 ;;
	esac

	echo $((memory_bytes / 1024))
}

postgis_ci_available_memory_kb()
{
	# POSTGIS_CI_MEM_AVAILABLE_KB: manual override in KiB for deterministic CI
	# sizing. Set this on hosts where memory reporting is noisy or unstable.
	if postgis_ci_positive_integer "${POSTGIS_CI_MEM_AVAILABLE_KB:-}"; then
		echo "${POSTGIS_CI_MEM_AVAILABLE_KB}"
		return 0
	fi

	# Usual path: compare cgroup limit and MemAvailable, then use the smaller
	# value as the effective budget for CI jobs.
	cgroup_kb=$(postgis_ci_cgroup_memory_kb 2>/dev/null || true)
	memavailable_kb=$(awk '/^MemAvailable:/ { print $2; exit }' /proc/meminfo 2>/dev/null || true)

	if postgis_ci_positive_integer "${cgroup_kb}" && postgis_ci_positive_integer "${memavailable_kb}"; then
		if test "${cgroup_kb}" -lt "${memavailable_kb}"; then
			echo "${cgroup_kb}"
		else
			echo "${memavailable_kb}"
		fi
		return 0
	fi

	# Fallback when only cgroup memory data is available (container-enforced cap).
	if postgis_ci_positive_integer "${cgroup_kb}"; then
		echo "${cgroup_kb}"
		return 0
	fi

	# Fallback when cgroup data is missing but host MemAvailable is parseable.
	if postgis_ci_positive_integer "${memavailable_kb}"; then
		echo "${memavailable_kb}"
		return 0
	fi

	return 1
}

postgis_ci_parallel_jobs()
{
	cpus=$(postgis_ci_cpu_count)

	# POSTGIS_CI_MAX_JOBS: explicit job cap from CI; defaults to CPU count.
	# Set this to lower-than-CPU when runners are noisy/shared.
	if postgis_ci_positive_integer "${POSTGIS_CI_MAX_JOBS:-}"; then
		max_jobs=${POSTGIS_CI_MAX_JOBS}
	else
		max_jobs=${cpus}
	fi
	if test "${max_jobs}" -gt "${cpus}"; then
		max_jobs=${cpus}
	fi

	# POSTGIS_CI_JOB_MEMORY_MB: estimated memory per job in MiB, default 1024.
	# Adjust upward/downward when workloads have heavier/lighter memory profiles.
	if postgis_ci_positive_integer "${POSTGIS_CI_JOB_MEMORY_MB:-}"; then
		job_memory_mb=${POSTGIS_CI_JOB_MEMORY_MB}
	else
		job_memory_mb=1024
	fi

	if postgis_ci_positive_integer "${POSTGIS_CI_MEMORY_RESERVE_MB:-}"; then
		reserve_mb=${POSTGIS_CI_MEMORY_RESERVE_MB}
	else
		reserve_mb=512
	fi

	available_kb=$(postgis_ci_available_memory_kb 2>/dev/null || true)
	if ! postgis_ci_positive_integer "${available_kb}"; then
		echo "${max_jobs}"
		return 0
	fi

	available_mb=$((available_kb / 1024))
	if test "${available_mb}" -le "${reserve_mb}"; then
		echo 1
		return 0
	fi

	jobs=$(((available_mb - reserve_mb) / job_memory_mb))
	if test "${jobs}" -lt 1; then
		jobs=1
	fi
	if test "${jobs}" -gt "${max_jobs}"; then
		jobs=${max_jobs}
	fi

	echo "${jobs}"
}

if test "${0##*/}" = "parallel-jobs.sh"; then
	postgis_ci_parallel_jobs
fi
