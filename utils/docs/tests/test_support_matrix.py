import copy
import unittest

from utils.docs.support_matrix import cli, payload, update


class SupportMatrixTest(unittest.TestCase):
    def test_postgresql_replaces_preview_when_released(self):
        rows = [
            (str(major), f"{major}.0", "Yes", "September 1, 2026", "September 1, 2031")
            for major in (21, 18, 17, 16, 15)
        ]
        rows.append(("14", "14.25", "No", "September 30, 2021", "November 12, 2026"))
        page = "".join("<tr>" + "".join(f"<td>{field}</td>" for field in row) + "</tr>" for row in rows)
        preview = {"version": "22", "lifecycle": "development", "source": "preview"}
        existing = [
            preview,
            {"version": "21", "current_minor": "21beta2", "lifecycle": "development"},
        ]

        versions = update.parse_postgresql(page, existing)

        self.assertEqual(versions[0], preview)
        released = [item for item in versions if item["version"] == "21"]
        self.assertEqual(len(released), 1)
        self.assertEqual(released[0]["current_minor"], "21.0")
        self.assertEqual(released[0]["lifecycle"], "supported")
        self.assertTrue(versions[-1]["eol"])

    def test_new_library_line_demotes_previous_current_line(self):
        page = (
            "<strong>2026-09-01</strong> library-8.6.0.tar.gz "
            "<strong>2026-08-01</strong> library-8.5.1.tar.gz"
        )
        existing = [{"version": "8.5", "lifecycle": "current"}]

        lines = update.release_line_versions(page, "library", existing)

        self.assertEqual([(line["version"], line["lifecycle"]) for line in lines],
                         [("8.6", "current"), ("8.5", "historical")])

    def test_all_released_lines_can_have_feature_limits(self):
        data = {
            "postgis_series": [{
                "minor": "3.7",
                "dependencies": {"geos": {"minimum": "3.10.0", "feature_complete": "3.16.0"}},
            }],
            "dependency_versions": {"geos": [{"version": "3.15"}]},
        }
        self.assertEqual(cli.compatibility_conflicts(data), [])

        unusable = copy.deepcopy(data)
        unusable["dependency_versions"]["geos"] = [{"version": "3.9"}]
        self.assertTrue(cli.compatibility_conflicts(unusable))

    def test_milestone_year_does_not_imply_a_first_release(self):
        series = {
            "lifecycle": "development",
            "planned_release": {"year": 2029, "url": "https://example.org/milestone"},
        }

        date, lifecycle = payload.browser_lifecycle(series, 2028)

        self.assertEqual(date["text"], "Planned for 2029")
        self.assertNotIn("First release", date["title"])
        self.assertEqual(lifecycle["text"], "development")
