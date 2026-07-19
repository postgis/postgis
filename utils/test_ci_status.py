import importlib.util
import pathlib
import unittest


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

        self.assertIn("Required failures (2 across 1 branch)", summary)
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

        self.assertIn(
            "Failing means one or more required checks failed on a supported branch. "
            "1 OK; 1 failure; 1 running",
            rendered,
        )
        self.assertIn("Required failures (1 across 1 branch)", rendered)
        banner_start = rendered.index('<section class="status-banner')
        banner_end = rendered.index("</section>", banner_start)
        failure_start = rendered.index("<div class='failure-attribution'", banner_start)
        self.assertLess(failure_start, banner_end)

    def test_failure_summary_pluralizes_branches(self):
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

        self.assertIn("Required failures (2 across 2 branches)", summary)

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
        self.assertNotIn("<div class='failure-attribution'", rendered)


if __name__ == "__main__":
    unittest.main()
