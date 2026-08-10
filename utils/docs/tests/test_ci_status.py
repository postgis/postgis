import json
import io
import pathlib
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

from ci_status import report as CI_STATUS


CONFIG_PATH = pathlib.Path(CI_STATUS.__file__).with_name("config.json")


def check(name, status, *, required=True, url=None):
    result = {
        "branch": "stable-synthetic",
        "branch_label": "Synthetic",
        "check": name,
        "provider": "synthetic",
        "required": required,
        "status": status,
    }
    if url:
        result["url"] = url
    return result


class CIStatusTest(unittest.TestCase):
    def test_current_success_cache_skips_provider(self):
        config = {
            "branches": [{"name": "master", "label": "master"}],
            "checks": [{
                "name": "Synthetic CI",
                "provider": "synthetic",
                "required": True,
            }],
        }
        cached_result = {
            "branch": "master",
            "branch_label": "master",
            "check": "Synthetic CI",
            "provider": "synthetic",
            "required": True,
            "status": CI_STATUS.SUCCESS,
            "revision": "a" * 40,
            "message": "build 7",
        }
        cache = CI_STATUS.index_status_cache({
            "generated_at": "2026-07-25T12:00:00+00:00",
            "branches": [{
                "name": "master",
                "checks": [cached_result],
            }],
        })
        provider = mock.Mock()

        with (
            mock.patch.dict(CI_STATUS.PROVIDERS, {"synthetic": provider}),
            mock.patch.object(
                CI_STATUS,
                "resolve_cache_heads",
                return_value={"master": "a" * 40},
            ),
        ):
            data = CI_STATUS.collect_status(config, cache=cache)

        provider.assert_not_called()
        result = data["branches"][0]["checks"][0]
        self.assertEqual(CI_STATUS.SUCCESS, result["status"])
        self.assertTrue(result["cached"])
        self.assertEqual("2026-07-25T12:00:00+00:00", result["cached_at"])
        self.assertEqual("build 7 (cached; unchanged revision)", result["message"])

    def test_cache_does_not_hide_new_revision_result(self):
        config = {
            "branches": [{"name": "master", "label": "master"}],
            "checks": [{
                "name": "Synthetic CI",
                "provider": "synthetic",
                "required": True,
            }],
        }
        cache = CI_STATUS.index_status_cache({
            "generated_at": "2026-07-25T12:00:00+00:00",
            "branches": [{
                "name": "master",
                "checks": [{
                    "branch": "master",
                    "branch_label": "master",
                    "check": "Synthetic CI",
                    "provider": "synthetic",
                    "required": True,
                    "status": CI_STATUS.SUCCESS,
                    "revision": "a" * 40,
                }],
            }],
        })
        live_result = check("Synthetic CI", CI_STATUS.FAILURE)
        live_result.update({"branch": "master", "branch_label": "master"})
        provider = mock.Mock(return_value=live_result)

        with (
            mock.patch.dict(CI_STATUS.PROVIDERS, {"synthetic": provider}),
            mock.patch.object(
                CI_STATUS,
                "resolve_cache_heads",
                return_value={"master": "b" * 40},
            ),
        ):
            data = CI_STATUS.collect_status(config, cache=cache)

        provider.assert_called_once()
        self.assertEqual(CI_STATUS.FAILURE, data["branches"][0]["checks"][0]["status"])

    def test_cache_rejects_non_success_revisionless_and_changed_provider_results(self):
        config = {
            "branches": [{"name": "master", "label": "master"}],
            "checks": [{
                "name": "Synthetic CI",
                "provider": "synthetic",
                "required": True,
            }],
        }
        branch = config["branches"][0]
        check_config = config["checks"][0]
        base = {
            "branch": "master",
            "branch_label": "master",
            "check": "Synthetic CI",
            "provider": "synthetic",
            "required": True,
            "status": CI_STATUS.SUCCESS,
            "revision": "a" * 40,
        }
        rejected = [
            {**base, "status": CI_STATUS.FAILURE},
            {**base, "status": CI_STATUS.IN_PROGRESS},
            {**base, "status": CI_STATUS.UNKNOWN},
            {key: value for key, value in base.items() if key != "revision"},
            {**base, "branch": "stable-3.6"},
            {**base, "check": "Other CI"},
            {**base, "provider": "other"},
            {**base, "required": False},
        ]

        for cached in rejected:
            with self.subTest(cached=cached):
                cache = CI_STATUS.index_status_cache({
                    "branches": [{"name": "master", "checks": [cached]}],
                })
                self.assertIsNone(
                    CI_STATUS.cached_success_result(
                        branch,
                        check_config,
                        cache,
                        {"master": "a" * 40},
                    )
                )

    def test_cache_uses_fresh_remote_head_not_stale_local_ref(self):
        config = {
            "branches": [{"name": "master", "label": "master"}],
            "checks": [{
                "name": "Synthetic CI",
                "provider": "synthetic",
                "required": True,
            }],
        }
        cache = CI_STATUS.index_status_cache({
            "branches": [{
                "name": "master",
                "checks": [{
                    "branch": "master",
                    "branch_label": "master",
                    "check": "Synthetic CI",
                    "provider": "synthetic",
                    "required": True,
                    "status": CI_STATUS.SUCCESS,
                    "revision": "a" * 40,
                }],
            }],
        })
        live_result = check("Synthetic CI", CI_STATUS.FAILURE)
        live_result.update({"branch": "master", "branch_label": "master"})
        provider = mock.Mock(return_value=live_result)

        with (
            mock.patch.dict(CI_STATUS.PROVIDERS, {"synthetic": provider}),
            mock.patch.object(CI_STATUS, "git_commit_distance", return_value=0),
            mock.patch.object(
                CI_STATUS,
                "resolve_cache_heads",
                return_value={"master": "b" * 40},
            ),
        ):
            data = CI_STATUS.collect_status(config, cache=cache)

        provider.assert_called_once()
        self.assertEqual(CI_STATUS.FAILURE, data["branches"][0]["checks"][0]["status"])

    def test_cache_head_lookup_queries_remote_once_for_all_branches(self):
        config = {"cache_head_remote": "https://example.test/postgis.git"}
        branches = [
            {"name": "master", "label": "master"},
            {"name": "stable-3.6", "label": "3.6"},
        ]
        work = [(branch, {"name": "Synthetic CI"}) for branch in branches]
        cache = CI_STATUS.index_status_cache({
            "branches": [
                {
                    "name": branch["name"],
                    "checks": [{
                        "check": "Synthetic CI",
                        "status": CI_STATUS.SUCCESS,
                    }],
                }
                for branch in branches
            ],
        })
        completed = subprocess.CompletedProcess(
            args=[],
            returncode=0,
            stdout=(
                f"{'a' * 40}\trefs/heads/master\n"
                f"{'b' * 40}\trefs/heads/stable-3.6\n"
            ),
            stderr="",
        )

        with mock.patch.object(CI_STATUS.subprocess, "run", return_value=completed) as run:
            heads = CI_STATUS.resolve_cache_heads(config, work, cache, timeout=7)

        self.assertEqual({"master": "a" * 40, "stable-3.6": "b" * 40}, heads)
        run.assert_called_once_with(
            [
                "git",
                "ls-remote",
                "--exit-code",
                "--heads",
                "--",
                "https://example.test/postgis.git",
                "refs/heads/master",
                "refs/heads/stable-3.6",
            ],
            check=True,
            capture_output=True,
            text=True,
            timeout=7,
        )

    def test_cache_head_lookup_rejects_option_like_remote(self):
        config = {"cache_head_remote": "--upload-pack=sh"}
        work = [({"name": "master", "label": "master"}, {"name": "Synthetic CI"})]
        cache = CI_STATUS.index_status_cache({
            "branches": [{
                "name": "master",
                "checks": [{
                    "check": "Synthetic CI",
                    "status": CI_STATUS.SUCCESS,
                }],
            }],
        })

        with self.assertRaises(CI_STATUS.ConfigError):
            CI_STATUS.resolve_cache_heads(config, work, cache, timeout=7)

    def test_branch_table_counts_all_stale_statuses_as_unknown(self):
        branch = {
            "name": "stable-synthetic",
            "label": "Synthetic",
            "status": CI_STATUS.UNKNOWN,
            "checks": [
                check("Required / Unknown", CI_STATUS.UNKNOWN),
                check("Required / Stale", CI_STATUS.STALE),
                check("Required / Passed", CI_STATUS.STALE_PASSED),
                check("Required / Failed", CI_STATUS.STALE_FAILED),
            ],
        }

        with mock.patch("sys.stdout", new_callable=io.StringIO) as stdout:
            CI_STATUS.print_branch_table([branch], use_color=False)

        self.assertIn("  4  ", stdout.getvalue())

    def test_missing_optional_status_cache_starts_empty(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            missing = pathlib.Path(tmpdir) / "status.json"
            self.assertIsNone(CI_STATUS.load_status_cache(missing))

    def test_woodpecker_covers_supported_release_branches(self):
        config = json.loads(CONFIG_PATH.read_text(encoding="utf-8"))
        woodpecker = next(check for check in config["checks"] if check["name"] == "Woodpecker")

        self.assertEqual(
            [
                "master",
                "stable-3.6",
                "stable-3.5",
                "stable-3.4",
                "stable-3.3",
                "stable-3.2",
            ],
            woodpecker["branches"],
        )

    def test_retired_badges_record_current_non_gating_reason(self):
        config = json.loads(CONFIG_PATH.read_text(encoding="utf-8"))
        serialized = json.dumps(config)

        self.assertNotIn("bessie32", serialized.lower())

        checks = {check["name"]: check for check in config["checks"]}
        self.assertIn("api.cirrus-ci.com", checks["Cirrus CI"]["message"])
        self.assertIn("fails TLS", checks["Cirrus CI"]["message"])
        self.assertIn("ci_quota_exceeded", checks["GitLab mirror"]["message"])
        self.assertIn("badge endpoint still resolves", checks["GitLab mirror"]["message"])
        self.assertIn("stale failed mirror results", checks["INAF GitLab mirror"]["message"])

    def test_apply_staleness_labels_passed_and_failed_results(self):
        config = {
            "stale_after_hours": 168,
            "branches": [{"name": "stable-synthetic", "label": "Synthetic"}],
        }
        stale_check = {"name": "Synthetic CI"}
        base = {
            "branch": "stable-synthetic",
            "branch_label": "Synthetic",
            "check": "Synthetic CI",
            "provider": "synthetic",
            "required": True,
            "revision": "0" * 40,
        }

        with mock.patch.object(CI_STATUS, "result_revision_distance", return_value=(3, "stable-synthetic")):
            failed = CI_STATUS.apply_staleness({
                **base,
                "status": CI_STATUS.FAILURE,
                "message": "build 1",
            }, config, stale_check)
            passed = CI_STATUS.apply_staleness({
                **base,
                "status": CI_STATUS.SUCCESS,
                "completed_at": "2026-07-01T00:00:00Z",
                "message": "build 2",
            }, config, stale_check)

        self.assertEqual(CI_STATUS.STALE_FAILED, failed["status"])
        self.assertEqual(CI_STATUS.FAILURE, failed["stale_base_status"])
        self.assertEqual("Stale fail", failed["status_label"])
        self.assertIn("3 commits behind stable-synthetic", failed["message"])

        self.assertEqual(CI_STATUS.STALE_PASSED, passed["status"])
        self.assertEqual(CI_STATUS.SUCCESS, passed["stale_base_status"])
        self.assertEqual("Stale passed", passed["status_label"])

    def test_jenkins_matrix_failure_names_failing_axis(self):
        check_config = {
            "name": "Jenkins / Winnie",
            "provider": "jenkins",
            "required": True,
            "job_url": "https://ci.example.test/job/PostGIS_trunk/",
        }
        branch = {
            "name": "master",
            "label": "master",
            "version_or_trunk": "trunk",
        }
        current = {
            "number": 5284,
            "result": "FAILURE",
            "url": "https://ci.example.test/job/PostGIS_trunk/5284/",
            "timestamp": 1784791530000,
            "actions": [
                {"lastBuiltRevision": {"SHA1": "a" * 40}},
            ],
        }
        matrix = [
            {
                "name": "PG_VER=15,OS_BUILD=64",
                "lastBuild": {
                    "number": 5279,
                    "result": "SUCCESS",
                    "url": "https://ci.example.test/job/PostGIS_trunk/PG_VER=15/5279/",
                },
            },
            {
                "name": "PG_VER=19,OS_BUILD=64",
                "lastBuild": {
                    "number": 5284,
                    "result": "FAILURE",
                    "url": "https://ci.example.test/job/PostGIS_trunk/PG_VER=19/5284/",
                },
            },
        ]

        with (
            mock.patch.object(CI_STATUS, "jenkins_queued_check", return_value=None),
            mock.patch.object(CI_STATUS, "jenkins_builds", return_value=[current]),
            mock.patch.object(CI_STATUS, "jenkins_matrix_configurations", return_value=matrix),
        ):
            result = CI_STATUS.jenkins_check(check_config, branch, timeout=5)

        self.assertEqual(CI_STATUS.FAILURE, result["status"])
        self.assertEqual("build 5284; failed: PG19", result["message"])
        self.assertEqual("https://ci.example.test/job/PostGIS_trunk/PG_VER=19/5284/", result["url"])
        self.assertEqual("a" * 40, result["revision"])

    def test_jenkins_single_configuration_matrix_keeps_parent_message(self):
        check_config = {
            "name": "Jenkins / Make Dist",
            "provider": "jenkins",
            "required": True,
            "job_url": "https://ci.example.test/job/PostGIS_Make_Dist/",
        }
        branch = {
            "name": "master",
            "label": "master",
            "version_or_trunk": "trunk",
        }
        current = {
            "number": 7808,
            "building": True,
            "result": None,
            "url": "https://ci.example.test/job/PostGIS_Make_Dist/7808/",
        }
        matrix = [
            {
                "name": "label=debbie",
                "lastBuild": {
                    "number": 7808,
                    "building": True,
                    "result": None,
                    "url": "https://ci.example.test/job/PostGIS_Make_Dist/label=debbie/7808/",
                },
            },
        ]

        with (
            mock.patch.object(CI_STATUS, "jenkins_queued_check", return_value=None),
            mock.patch.object(CI_STATUS, "jenkins_builds", return_value=[current]),
            mock.patch.object(CI_STATUS, "jenkins_matrix_configurations", return_value=matrix),
        ):
            result = CI_STATUS.jenkins_check(check_config, branch, timeout=5)

        self.assertEqual(CI_STATUS.IN_PROGRESS, result["status"])
        self.assertEqual("build 7808", result["message"])
        self.assertEqual("https://ci.example.test/job/PostGIS_Make_Dist/7808/", result["url"])

    def test_jenkins_matrix_ignores_another_parent_build(self):
        check_config = {
            "name": "Jenkins / Debbie main",
            "provider": "jenkins",
            "required": True,
            "job_url": "https://ci.example.test/job/PostGIS_trunk/",
        }
        branch = {
            "name": "master",
            "label": "master",
            "version_or_trunk": "trunk",
        }
        current = {
            "number": 5212,
            "building": True,
            "result": None,
            "url": "https://ci.example.test/job/PostGIS_trunk/5212/",
        }
        matrix = [
            {
                "name": "PG_VER=14,OS_BUILD=64",
                "lastBuild": {
                    "number": 5208,
                    "result": "FAILURE",
                    "url": "https://ci.example.test/job/PostGIS_trunk/PG_VER=14/5208/",
                },
            },
            {
                "name": "PG_VER=18,OS_BUILD=64",
                "lastBuild": {
                    "number": 5212,
                    "building": True,
                    "result": None,
                    "url": "https://ci.example.test/job/PostGIS_trunk/PG_VER=18/5212/",
                },
            },
        ]

        with (
            mock.patch.object(CI_STATUS, "jenkins_queued_check", return_value=None),
            mock.patch.object(CI_STATUS, "jenkins_builds", return_value=[current]),
            mock.patch.object(CI_STATUS, "jenkins_matrix_configurations", return_value=matrix),
        ):
            result = CI_STATUS.jenkins_check(check_config, branch, timeout=5)

        self.assertEqual(CI_STATUS.IN_PROGRESS, result["status"])
        self.assertEqual("build 5212; running: PG18", result["message"])
        self.assertEqual(
            "https://ci.example.test/job/PostGIS_trunk/PG_VER=18/5212/",
            result["url"],
        )

    def test_jenkins_queue_prefers_current_branch_revision(self):
        check_config = {
            "name": "Jenkins / Berrie",
            "provider": "jenkins",
            "required": True,
            "job_url": "https://ci.example.test/job/PostGIS_Worker_Run/label=berrie/",
            "branch_parameter": "reference",
        }
        branch = {
            "name": "master",
            "label": "master",
        }
        old_revision = "7" * 40
        current_revision = "4" * 40
        queued = [
            {
                "id": 108699,
                "task": {"url": "https://ci.example.test/job/PostGIS_Worker_Run/"},
                "why": "Build #8,030 is already in progress",
                "inQueueSince": 1784820000000,
                "actions": [{"parameters": [
                    {"name": "reference", "value": "refs/heads/master"},
                    {"name": "after", "value": old_revision},
                ]}],
            },
            {
                "id": 108746,
                "task": {"url": "https://ci.example.test/job/PostGIS_Worker_Run/"},
                "why": "Build #8,030 is already in progress",
                "inQueueSince": 1784821000000,
                "actions": [{"parameters": [
                    {"name": "reference", "value": "refs/heads/master"},
                    {"name": "after", "value": current_revision},
                ]}],
            },
        ]

        def fake_distance(revision, ref):
            return 0 if revision == current_revision else 26

        with (
            mock.patch.object(CI_STATUS, "jenkins_queue_items", return_value=queued),
            mock.patch.object(CI_STATUS, "git_commit_distance", side_effect=fake_distance),
        ):
            result = CI_STATUS.jenkins_queued_check(
                check_config,
                branch,
                "https://ci.example.test/job/PostGIS_Worker_Run/label=berrie/",
                timeout=5,
            )

        self.assertEqual(CI_STATUS.IN_PROGRESS, result["status"])
        self.assertEqual(current_revision, result["revision"])
        self.assertEqual("queued item 108746: Build #8,030 is already in progress", result["message"])

    def test_jenkins_old_queue_item_is_labeled_stale(self):
        queued = {
            "id": 108746,
            "inQueueSince": 1784821000000,
        }

        now = CI_STATUS.dt.datetime.fromtimestamp(1784840000, CI_STATUS.dt.timezone.utc)
        with mock.patch.object(CI_STATUS, "utc_now", return_value=now):
            self.assertEqual("Queued stale", CI_STATUS.jenkins_queued_status_label(queued))

    def test_woodpecker_failure_names_single_failed_workflow(self):
        check_config = {
            "name": "Woodpecker",
            "provider": "woodpecker",
            "required": True,
            "api_url": "https://woodie.example.test/api/repos/30/pipelines",
            "web_url": "https://woodie.example.test/repos/30",
        }
        branch = {"name": "master", "label": "master"}
        pipeline = {
            "number": 5430,
            "event": "push",
            "branch": "master",
            "ref": "refs/heads/master",
            "status": "killed",
            "commit": "a" * 40,
            "message": "opaque commit message",
        }
        pipeline_detail = {
            **pipeline,
            "workflows": [
                {"pid": 1, "id": 24450, "name": "regress", "state": "success"},
                {"pid": 18, "id": 24467, "name": "regress", "state": "killed"},
                {"pid": 27, "id": 24476, "name": "tools", "state": "success"},
            ],
        }

        with mock.patch.object(CI_STATUS, "http_json", side_effect=([pipeline], pipeline_detail)) as http_json:
            result = CI_STATUS.woodpecker_check(check_config, branch, timeout=5)

        self.assertEqual(CI_STATUS.UNKNOWN, result["status"])
        self.assertEqual("unknown: regress/18", result["message"])
        self.assertEqual("https://woodie.example.test/repos/30/pipeline/5430/18", result["url"])
        self.assertEqual("a" * 40, result["revision"])
        self.assertEqual(
            "https://woodie.example.test/api/repos/30/pipelines/5430",
            http_json.call_args_list[1].args[0],
        )

    def test_woodpecker_failure_names_single_failed_workflow_child_step(self):
        check_config = {
            "name": "Woodpecker",
            "provider": "woodpecker",
            "required": True,
            "api_url": "https://woodie.example.test/api/repos/30/pipelines",
            "web_url": "https://woodie.example.test/repos/30",
        }
        branch = {"name": "stable-3.6", "label": "3.6"}
        pipeline = {
            "number": 5431,
            "event": "push",
            "branch": "stable-3.6",
            "ref": "refs/heads/stable-3.6",
            "status": "failure",
            "commit": "b" * 40,
            "message": "opaque commit message",
        }
        pipeline_detail = {
            **pipeline,
            "workflows": [
                {"pid": 9, "id": 24470, "name": "regress", "state": "success"},
                {
                    "pid": 29,
                    "id": 24477,
                    "name": "regress",
                    "state": "failure",
                    "children": [
                        {"pid": 100, "name": "test-upgrades", "state": "failure"},
                    ],
                },
            ],
        }

        with mock.patch.object(CI_STATUS, "http_json", side_effect=([pipeline], pipeline_detail)) as http_json:
            result = CI_STATUS.woodpecker_check(check_config, branch, timeout=5)

        self.assertEqual(CI_STATUS.FAILURE, result["status"])
        self.assertEqual("failed: regress/29 (test-upgrades)", result["message"])
        self.assertEqual("https://woodie.example.test/repos/30/pipeline/5431/29", result["url"])
        self.assertEqual("b" * 40, result["revision"])
        self.assertEqual(
            "https://woodie.example.test/api/repos/30/pipelines/5431",
            http_json.call_args_list[1].args[0],
        )

    def test_woodpecker_error_without_workflows_shows_error_message(self):
        check_config = {
            "name": "Woodpecker",
            "provider": "woodpecker",
            "required": True,
            "api_url": "https://woodie.example.test/api/repos/30/pipelines",
            "web_url": "https://woodie.example.test/repos/30",
        }
        branch = {"name": "master", "label": "master"}
        pipeline = {
            "number": 5733,
            "event": "push",
            "branch": "master",
            "ref": "refs/heads/master",
            "status": "error",
            "commit": "c" * 40,
            "message": "opaque commit message",
            "errors": [{"message": "step 'html-ja' depends on unknown step 'html-de'"}],
        }

        with mock.patch.object(CI_STATUS, "http_json", return_value=[pipeline]) as http_json:
            result = CI_STATUS.woodpecker_check(check_config, branch, timeout=5)

        self.assertEqual(CI_STATUS.FAILURE, result["status"])
        self.assertEqual("Config error", result["status_label"])
        self.assertEqual("step 'html-ja' depends on unknown step 'html-de'", result["message"])
        http_json.assert_called_once()

    def test_woodpecker_killed_zero_exit_steps_are_agent_loss(self):
        check_config = {
            "name": "Woodpecker",
            "provider": "woodpecker",
            "required": True,
            "api_url": "https://woodie.example.test/api/repos/30/pipelines",
            "web_url": "https://woodie.example.test/repos/30",
        }
        branch = {"name": "stable-3.6", "label": "3.6"}
        pipeline = {
            "number": 5696,
            "event": "pull_request",
            "branch": "stable-3.6",
            "ref": "refs/heads/stable-3.6",
            "status": "failure",
            "commit": "d" * 40,
            "message": "opaque commit message",
        }
        pipeline_detail = {
            **pipeline,
            "workflows": [
                {
                    "pid": 1,
                    "name": "docs",
                    "state": "failure",
                    "children": [
                        {"pid": 4, "name": "clone", "state": "killed", "exit_code": 0},
                        {"pid": 5, "name": "prepare", "state": "killed", "exit_code": 0},
                        {"pid": 6, "name": "check-xml", "state": "killed", "exit_code": 0},
                    ],
                },
            ],
        }

        with mock.patch.object(CI_STATUS, "http_json", side_effect=([pipeline], pipeline_detail)):
            result = CI_STATUS.woodpecker_check(
                {**check_config, "event": "pull_request"},
                branch,
                timeout=5,
            )

        self.assertEqual(CI_STATUS.UNKNOWN, result["status"])
        self.assertEqual("Agent lost", result["status_label"])
        self.assertEqual("agent lost: 3 steps stopped at exit 0 (clone, prepare, check-xml)", result["message"])

    def test_woodpecker_killed_pipeline_with_deadline_exceeded_is_agent_loss(self):
        check_config = {
            "name": "Woodpecker",
            "provider": "woodpecker",
            "required": True,
            "api_url": "https://woodie.example.test/api/repos/30/pipelines",
            "web_url": "https://woodie.example.test/repos/30",
        }
        branch = {"name": "stable-3.6", "label": "3.6"}
        pipeline = {
            "number": 6852,
            "event": "push",
            "branch": "stable-3.6",
            "ref": "refs/heads/stable-3.6",
            "status": "killed",
            "commit": "e" * 40,
            "workflows": [
                {
                    "pid": 1,
                    "name": "regress",
                    "state": "killed",
                    "children": [{
                        "pid": 4,
                        "name": "test-upgrades",
                        "state": "failure",
                        "exit_code": 0,
                        "error": "Post docker.sock/wait: context deadline exceeded",
                    }],
                },
            ],
        }

        with mock.patch.object(CI_STATUS, "http_json", return_value=[pipeline]):
            result = CI_STATUS.woodpecker_check(check_config, branch, timeout=5)

        self.assertEqual(CI_STATUS.UNKNOWN, result["status"])
        self.assertEqual("Agent lost", result["status_label"])

    def test_woodpecker_killed_pipeline_with_nonzero_failure_is_failure(self):
        check_config = {
            "name": "Woodpecker",
            "provider": "woodpecker",
            "required": True,
            "api_url": "https://woodie.example.test/api/repos/30/pipelines",
            "web_url": "https://woodie.example.test/repos/30",
        }
        branch = {"name": "master", "label": "master"}
        pipeline = {
            "number": 6874,
            "event": "push",
            "branch": "master",
            "ref": "refs/heads/master",
            "status": "killed",
            "commit": "f" * 40,
            "workflows": [
                {
                    "pid": 1,
                    "name": "tools",
                    "state": "failure",
                    "children": [{"pid": 4, "name": "build", "state": "failure", "exit_code": 2}],
                },
            ],
        }

        with mock.patch.object(CI_STATUS, "http_json", return_value=[pipeline]):
            result = CI_STATUS.woodpecker_check(check_config, branch, timeout=5)

        self.assertEqual(CI_STATUS.FAILURE, result["status"])
        self.assertEqual("failed: tools (build)", result["message"])

    def test_woodpecker_running_workflows_are_summarized(self):
        check_config = {
            "name": "Woodpecker",
            "provider": "woodpecker",
            "required": True,
            "api_url": "https://woodie.example.test/api/repos/30/pipelines",
            "web_url": "https://woodie.example.test/repos/30",
        }
        branch = {"name": "stable-3.6", "label": "3.6"}
        pipeline = {
            "number": 5434,
            "event": "pull_request",
            "branch": "stable-3.6",
            "ref": "refs/heads/stable-3.6",
            "status": "running",
            "commit": "b" * 40,
            "workflows": [
                {"pid": 1, "id": 24484, "name": "docs", "state": "success"},
                {"pid": 2, "id": 24485, "name": "regress", "state": "running"},
                {"pid": 3, "id": 24486, "name": "tools", "state": "success"},
            ],
        }

        with mock.patch.object(CI_STATUS, "http_json", return_value=[pipeline]):
            result = CI_STATUS.woodpecker_check(
                {**check_config, "event": "pull_request"},
                branch,
                timeout=5,
            )

        self.assertEqual(CI_STATUS.IN_PROGRESS, result["status"])
        self.assertEqual("running: regress", result["message"])
        self.assertEqual("https://woodie.example.test/repos/30/pipeline/5434/2", result["url"])

    def test_woodpecker_uses_newest_pipeline_when_api_order_is_unstable(self):
        check_config = {
            "name": "Woodpecker",
            "provider": "woodpecker",
            "required": True,
            "api_url": "https://woodie.example.test/api/repos/30/pipelines",
            "web_url": "https://woodie.example.test/repos/30",
        }
        branch = {"name": "stable-3.4", "label": "3.4"}
        older_failed = {
            "number": 5415,
            "event": "push",
            "branch": "stable-3.4",
            "ref": "refs/heads/stable-3.4",
            "status": "failure",
            "commit": "5" * 40,
            "created": 1784787793,
            "started": 1784787796,
            "finished": 1784794269,
            "message": "older failed pipeline",
        }
        newer_success = {
            "number": 5425,
            "event": "push",
            "branch": "stable-3.4",
            "ref": "refs/heads/stable-3.4",
            "status": "success",
            "commit": "5" * 40,
            "created": 1784806066,
            "started": 1784806067,
            "finished": 1784812852,
            "message": "newer successful pipeline",
        }

        with mock.patch.object(CI_STATUS, "http_json", side_effect=([older_failed, newer_success], newer_success)):
            result = CI_STATUS.woodpecker_check(check_config, branch, timeout=5)

        self.assertEqual(CI_STATUS.SUCCESS, result["status"])
        self.assertEqual("https://woodie.example.test/repos/30/pipeline/5425", result["url"])
        self.assertEqual("newer successful pipeline", result["message"])
        self.assertEqual(CI_STATUS.FAILURE, result["previous_completed_status"])
        self.assertEqual("https://woodie.example.test/repos/30/pipeline/5415", result["previous_completed_url"])

    def test_stale_summary_distinguishes_passed_and_failed(self):
        branch = {
            "name": "stable-synthetic",
            "label": "Synthetic",
            "status": CI_STATUS.UNKNOWN,
            "failures": 0,
            "checks": [
                check("Required / Passed", CI_STATUS.STALE_PASSED),
                check("Required / Failed", CI_STATUS.STALE_FAILED),
                check("Required / Unknown", CI_STATUS.UNKNOWN),
            ],
        }
        branch["checks"][0]["stale_base_status"] = CI_STATUS.SUCCESS
        branch["checks"][0]["status_label"] = "Stale passed"
        branch["checks"][1]["stale_base_status"] = CI_STATUS.FAILURE
        branch["checks"][1]["status_label"] = "Stale fail"

        self.assertEqual(
            "no known failures; 1 unknown, 1 stale-passed, 1 stale-fail",
            CI_STATUS.summary_text(branch),
        )


if __name__ == "__main__":
    unittest.main()
