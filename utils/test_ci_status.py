import importlib.util
import json
import pathlib
import unittest
from unittest import mock


MODULE_PATH = pathlib.Path(__file__).with_name("ci-status.py")
SPEC = importlib.util.spec_from_file_location("ci_status", MODULE_PATH)
CI_STATUS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CI_STATUS)


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


class RequiredFailureHtmlTest(unittest.TestCase):
    def test_woodpecker_covers_supported_release_branches(self):
        config = json.loads(MODULE_PATH.with_suffix(".json").read_text(encoding="utf-8"))
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

        self.assertEqual(CI_STATUS.FAILURE, result["status"])
        self.assertEqual("failed: regress/18", result["message"])
        self.assertEqual("https://woodie.example.test/repos/30/pipeline/5430/18", result["url"])
        self.assertEqual("a" * 40, result["revision"])
        self.assertEqual(
            "https://woodie.example.test/api/repos/30/pipelines/5430",
            http_json.call_args_list[1].args[0],
        )

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

        rendered = CI_STATUS.render_html({
            "generated_at": "2026-07-19T00:00:00+00:00",
            "branches": [branch],
        })

        self.assertIn("1 stale-passed", rendered)
        self.assertIn("1 stale-fail", rendered)
        self.assertIn("Stale passed", rendered)
        self.assertIn("Stale fail", rendered)
        self.assertIn("status-stale-passed stale-success", rendered)
        self.assertIn("status-stale-fail stale-failure", rendered)

    def test_failure_summary_names_only_required_failed_checks(self):
        branches = [
            {
                "name": "stable-synthetic",
                "label": "Synthetic",
                "status": CI_STATUS.FAILURE,
                "failures": 2,
                "checks": [
                    check("Jenkins / Alpha", CI_STATUS.FAILURE, url="https://ci.example.test/a"),
                    check("Woodie / Beta", CI_STATUS.FAILURE, url="https://ci.example.test/b"),
                    check("Optional / Gamma", CI_STATUS.FAILURE, required=False),
                    check("Required / Waiting", CI_STATUS.UNKNOWN),
                    check("Required / Passing", CI_STATUS.SUCCESS),
                ],
            },
            {
                "name": "stable-disabled",
                "label": "Disabled",
                "status": CI_STATUS.NOT_APPLICABLE,
                "failures": 0,
                "checks": [check("Retired / Delta", CI_STATUS.DISABLED)],
            },
        ]

        summary = CI_STATUS.html_required_failures(branches)

        self.assertIn("<strong>Required:</strong>", summary)
        self.assertIn("Jenkins / Alpha", summary)
        self.assertIn("https://ci.example.test/a", summary)
        self.assertIn("Woodie / Beta", summary)
        self.assertNotIn("Optional / Gamma", summary)
        self.assertNotIn("Required / Waiting", summary)
        self.assertNotIn("Retired / Delta", summary)

    def test_page_explains_rollup_scope(self):
        branch = {
            "name": "stable-synthetic",
            "label": "Synthetic",
            "status": CI_STATUS.FAILURE,
            "failures": 1,
            "checks": [
                check("Jenkins / Alpha", CI_STATUS.FAILURE),
                check("Required / Passing", CI_STATUS.SUCCESS),
                check("Required / Waiting", CI_STATUS.IN_PROGRESS),
            ],
        }

        rendered = CI_STATUS.render_html({
            "generated_at": "2026-07-19T00:00:00+00:00",
            "branches": [branch],
        })

        self.assertIn("1 OK; 1 failure; 1 running", rendered)
        self.assertIn("<strong>Required:</strong>", rendered)
        banner_start = rendered.index('<section class="status-banner')
        banner_end = rendered.index("</section>", banner_start)
        failure_start = rendered.index("aria-label='Required CI failures'", banner_start)
        self.assertLess(failure_start, banner_end)
        banner = rendered[banner_start:banner_end]
        self.assertNotIn("Failing means", banner)
        self.assertNotIn("<ul>", banner)

    def test_failure_summary_keeps_multiple_branches_inline(self):
        branches = [
            {
                "name": f"stable-synthetic-{index}",
                "label": f"Synthetic {index}",
                "status": CI_STATUS.FAILURE,
                "failures": 1,
                "checks": [check(f"Required / Failed {index}", CI_STATUS.FAILURE)],
            }
            for index in (1, 2)
        ]

        summary = CI_STATUS.html_required_failures(branches)

        self.assertIn("Synthetic 1", summary)
        self.assertIn("· <strong>Synthetic 2</strong>", summary)

    def test_failure_block_is_absent_without_required_failures(self):
        branch = {
            "name": "stable-synthetic",
            "label": "Synthetic",
            "status": CI_STATUS.UNKNOWN,
            "failures": 0,
            "checks": [check("Required / Waiting", CI_STATUS.UNKNOWN)],
        }

        self.assertEqual("", CI_STATUS.html_required_failures([branch]))
        rendered = CI_STATUS.render_html({
            "generated_at": "2026-07-19T00:00:00+00:00",
            "branches": [branch],
        })
        self.assertNotIn("aria-label='Required CI failures'", rendered)


if __name__ == "__main__":
    unittest.main()
