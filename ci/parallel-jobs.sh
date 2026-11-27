#!/bin/sh

# Helper used by PostGIS CI jobs to choose a safe level of parallelism for
# memory-bound build steps.
#
# Use the smaller of the CPU and memory budgets to avoid overcommitting
# shared runners, especially when the container sees more resources than
# the job can actually use.

postgis_ci_positive_integer()
{
	case "$1" in
		''|0*|*[!0-9]*) return 1 ;;
	esac
}

postgis_ci_cpu_count()
{
	# nproc accounts for CPU affinity on Linux. BSD runners may lack it.
	cpus=$(
		unset OMP_NUM_THREADS OMP_THREAD_LIMIT
		nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null ||
			sysctl -n hw.ncpu 2>/dev/null) || cpus=1
	if ! postgis_ci_positive_integer "${cpus}"; then
		cpus=1
	fi
	echo "${cpus}"
}

postgis_ci_cgroup_path()
{
	while IFS=: read -r hierarchy controllers path; do
		case "$1:${hierarchy}:,${controllers}," in
			v2:0:,,|cpu:*:*,cpu,*|memory:*:*,memory,*)
				printf '%s\n' "${path}"
				return
				;;
		esac
	done < /proc/self/cgroup
}

postgis_ci_cgroup_limits()
{
	root=$1
	path=$2
	kind=$3
	# Docker can mount the job's own cgroup as the filesystem root.
	while :; do
		dir=${root}${path}
		case "${kind}" in
			cpu-v2)
				test -r "${dir}/cpu.max" &&
					read -r quota period < "${dir}/cpu.max"
				;;
			cpu-v1)
				test -r "${dir}/cpu.cfs_quota_us" &&
					read -r quota < "${dir}/cpu.cfs_quota_us" &&
					read -r period < "${dir}/cpu.cfs_period_us"
				;;
			memory-v2|memory-v1)
				file=memory.max
				if test "${kind}" = memory-v1; then
					file=memory.limit_in_bytes
				fi
				if test -r "${dir}/${file}" && read -r bytes < "${dir}/${file}" &&
					postgis_ci_positive_integer "${bytes}"; then
					echo "$((bytes / 1024))"
				fi
				;;
		esac
		case "${kind}" in
			cpu-*)
				if postgis_ci_positive_integer "${quota:-}" &&
					postgis_ci_positive_integer "${period:-}"; then
					jobs=$((quota / period))
					echo "$((jobs > 0 ? jobs : 1))"
				fi
				unset quota period
				;;
		esac
		test -n "${path}" || break
		path=${path%/*}
	done
}

postgis_ci_cgroup_budget()
{
	test -r /proc/self/cgroup || return 0
	root=${POSTGIS_CI_TEST_CGROUP_ROOT:-/sys/fs/cgroup}
	v2=$(postgis_ci_cgroup_path v2)
	if test -n "${v2}"; then
		postgis_ci_cgroup_limits "${root}" "${v2%/}" "$1-v2"
	fi
	v1=$(postgis_ci_cgroup_path "$1")
	if test -n "${v1}"; then
		controller=$1
		if test "$1" = cpu && test -d "${root}/cpu,cpuacct"; then
			controller=cpu,cpuacct
		fi
		postgis_ci_cgroup_limits "${root}/${controller}" "${v1%/}" "$1-v1"
	fi
}

postgis_ci_available_memory_kb()
{
	if postgis_ci_positive_integer "${POSTGIS_CI_MEM_AVAILABLE_KB:-}"; then
		echo "${POSTGIS_CI_MEM_AVAILABLE_KB}"
		return
	fi

	postgis_ci_cgroup_budget memory
	if test -r /proc/meminfo; then
		awk '/^MemAvailable:/ { print $2; exit }' /proc/meminfo
	else
		# macOS and FreeBSD expose physical memory through sysctl.
		bytes=$(sysctl -n hw.memsize 2>/dev/null || sysctl -n hw.physmem 2>/dev/null)
		if postgis_ci_positive_integer "${bytes}"; then
			echo "$((bytes / 1024))"
		fi
	fi
}

postgis_ci_parallel_jobs()
{
	cpus=$( { postgis_ci_cpu_count; postgis_ci_cgroup_budget cpu; } |
		awk 'NR == 1 || $1 < min { min = $1 } END { print min }')

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

	available_kb=$(postgis_ci_available_memory_kb |
		awk 'NR == 1 || $1 < min { min = $1 } END { print min }')
	if test -z "${available_kb}"; then
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

postgis_ci_parallel_jobs
