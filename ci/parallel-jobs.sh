#!/bin/sh

postgis_ci_positive_integer()
{
	case "$1" in
		''|*[!0-9]*|0) return 1 ;;
		*) return 0 ;;
	esac
}

postgis_ci_cpu_count()
{
	cpus=$(getconf _NPROCESSORS_ONLN 2>/dev/null || nproc 2>/dev/null || echo 1)
	if ! postgis_ci_positive_integer "${cpus}"; then
		cpus=1
	fi
	echo "${cpus}"
}

postgis_ci_cgroup_memory_kb()
{
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
	if postgis_ci_positive_integer "${POSTGIS_CI_MEM_AVAILABLE_KB:-}"; then
		echo "${POSTGIS_CI_MEM_AVAILABLE_KB}"
		return 0
	fi

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

	if postgis_ci_positive_integer "${cgroup_kb}"; then
		echo "${cgroup_kb}"
		return 0
	fi

	if postgis_ci_positive_integer "${memavailable_kb}"; then
		echo "${memavailable_kb}"
		return 0
	fi

	return 1
}

postgis_ci_parallel_jobs()
{
	cpus=$(postgis_ci_cpu_count)

	if postgis_ci_positive_integer "${POSTGIS_CI_MAX_JOBS:-}"; then
		max_jobs=${POSTGIS_CI_MAX_JOBS}
	else
		max_jobs=${cpus}
	fi
	if test "${max_jobs}" -gt "${cpus}"; then
		max_jobs=${cpus}
	fi

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
