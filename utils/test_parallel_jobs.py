import os
import subprocess
import tempfile
import unittest


class ParallelJobsTest(unittest.TestCase):
    def run_jobs(self, **env):
        run_env = os.environ.copy()
        for name in list(run_env):
            if name.startswith("POSTGIS_CI_"):
                run_env.pop(name)
        run_env.update({name: str(value) for name, value in env.items()})
        output = subprocess.check_output(
            [
                "sh",
                "-c",
                ". ./ci/parallel-jobs.sh; postgis_ci_parallel_jobs",
            ],
            cwd=os.path.dirname(os.path.dirname(__file__)),
            env=run_env,
            text=True,
        )
        return int(output.strip())

    def test_memory_budget_caps_parallelism(self):
        self.assertEqual(
            3,
            self.run_jobs(
                POSTGIS_CI_MEM_AVAILABLE_KB=4 * 1024 * 1024,
                POSTGIS_CI_JOB_MEMORY_MB=1024,
                POSTGIS_CI_MEMORY_RESERVE_MB=512,
                POSTGIS_CI_MAX_JOBS=32,
            ),
        )

    def test_low_memory_keeps_one_job(self):
        self.assertEqual(
            1,
            self.run_jobs(
                POSTGIS_CI_MEM_AVAILABLE_KB=600 * 1024,
                POSTGIS_CI_JOB_MEMORY_MB=1024,
                POSTGIS_CI_MEMORY_RESERVE_MB=512,
                POSTGIS_CI_MAX_JOBS=32,
            ),
        )

    def test_explicit_max_jobs_is_honored(self):
        self.assertEqual(
            4,
            self.run_jobs(
                POSTGIS_CI_MEM_AVAILABLE_KB=64 * 1024 * 1024,
                POSTGIS_CI_JOB_MEMORY_MB=1024,
                POSTGIS_CI_MEMORY_RESERVE_MB=512,
                POSTGIS_CI_MAX_JOBS=4,
            ),
        )

    def test_invalid_overrides_fall_back_to_a_positive_job_count(self):
        self.assertGreaterEqual(
            self.run_jobs(
                POSTGIS_CI_MEM_AVAILABLE_KB="bogus",
                POSTGIS_CI_JOB_MEMORY_MB="bogus",
                POSTGIS_CI_MEMORY_RESERVE_MB="bogus",
                POSTGIS_CI_MAX_JOBS="bogus",
            ),
            1,
        )

    def test_cgroup_v2_memory_limit_is_used(self):
        with tempfile.TemporaryDirectory() as cgroup_root:
            with open("/proc/self/cgroup", encoding="utf-8") as f:
                cgroup_path = f.read().strip().split(":")[-1].lstrip("/")
            memory_dir = os.path.join(cgroup_root, cgroup_path)
            os.makedirs(memory_dir, exist_ok=True)
            with open(os.path.join(memory_dir, "memory.max"), "w", encoding="utf-8") as f:
                f.write(str(4 * 1024 * 1024 * 1024))
            self.assertEqual(
                3,
                self.run_jobs(
                    POSTGIS_CI_TEST_CGROUP_ROOT=cgroup_root,
                    POSTGIS_CI_JOB_MEMORY_MB=1024,
                    POSTGIS_CI_MEMORY_RESERVE_MB=512,
                    POSTGIS_CI_MAX_JOBS=32,
                ),
            )


if __name__ == "__main__":
    unittest.main()
