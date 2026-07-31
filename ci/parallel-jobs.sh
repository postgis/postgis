#!/bin/sh

# Helper used by PostGIS CI jobs to choose a safe level of parallelism for
# memory-bound build steps.
#
# Woodpecker pipelines were failing with "received oom kill" on small shared
# agents such as 145045 (capacity 4) and 145144 (capacity 2). An unbounded
# make -j sizes itself to the host CPU count, while a CI container may have only
# a fraction of the host memory.
#
# The selected job count is bounded by CPU count and by an assumed per-job
# memory cost. Available memory is treated as the smaller of the cgroup memory
# limit and /proc/meminfo MemAvailable, when both are available.

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
	# POSTGIS_CI_MEM_AVAILABLE_KB: manual override for available memory in KiB.
	# Default: auto-detect the smaller of the cgroup memory limit and
	# MemAvailable. Set it when CI memory reporting is missing, noisy, or known
	# to disagree with the worker capacity assigned to the job.
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

	# POSTGIS_CI_MAX_JOBS: explicit job cap from CI. Default: detected CPU
	# count. Set it to keep a shared worker below its CPU-based maximum even
	# when memory would otherwise allow more jobs.
	if postgis_ci_positive_integer "${POSTGIS_CI_MAX_JOBS:-}"; then
		max_jobs=${POSTGIS_CI_MAX_JOBS}
	else
		max_jobs=${cpus}
	fi
	if test "${max_jobs}" -gt "${cpus}"; then
		max_jobs=${cpus}
	fi

	# POSTGIS_CI_JOB_MEMORY_MB: estimated memory cost per parallel job in MiB.
	# Default: 1024. Set it when a job family is measured to use materially more
	# or less memory than the default docs build assumption.
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
