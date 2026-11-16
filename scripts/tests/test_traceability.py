"""Unit tests for scripts/traceability.py, on small made-up repos."""

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import traceability  # noqa: E402

REQUIREMENTS = """# Requirements

## Build order

| Increment | Requirements | Notes |
|---|---|---|
| **1. Skeleton** | F1, S1 | Notes may name F9 |
| 2. Sensors | T1, S2 | |
| Later | D1 | |

## Features

| ID | Requirement | Stage | How we check |
|---|---|---|---|
| F1 | Drive | 2 | Watch |
| S1 | Stop | 2 | Watch |
| S2 | Stop by remote | 2 | Watch |
| T1 | Frame | 2 | Measure |
| D1 | Wi-Fi | 2 | Use |
"""

UNIT_TEST = """#include "check.h"
void stops() {  // S1, A5: a note that names F1 too
}
void detail() {  // nothing traced here
}
void unknown() {  // X9
}
void runUnitTests() {
}
"""

ROBOT_TEST = 'REQUIREMENTS = ["S1", "S2"]\nMOVES = False\n'


def report(status_s1: str) -> dict:
    return {"reports": [{"test": "stops", "requirements": ["S1", "S2"], "status": status_s1,
                         "firmware": {"version": "0.1.0", "commit": "abc1234"},
                         "results": [{"check": "stop", "status": status_s1, "requirements": ["S1"]}]}]}


class TraceTests(unittest.TestCase):
    def make_repo(self, reports: dict[str, dict]) -> Path:
        root = Path(tempfile.mkdtemp())
        self.addCleanup(lambda: __import__("shutil").rmtree(root))
        (root / "docs").mkdir()
        (root / "docs" / "requirements.md").write_text(REQUIREMENTS, encoding="utf-8")
        tests = root / "firmware" / "uno" / "services" / "safety" / "tests"
        tests.mkdir(parents=True)
        (tests / "test_safety.cpp").write_text(UNIT_TEST, encoding="utf-8")
        (root / "tests" / "robot").mkdir(parents=True)
        (root / "tests" / "robot" / "stops.py").write_text(ROBOT_TEST, encoding="utf-8")
        for folder, content in reports.items():
            (root / "docs" / "test-reports" / folder).mkdir(parents=True)
            (root / "docs" / "test-reports" / folder / "report.json").write_text(json.dumps(content), encoding="utf-8")
        return root

    def trace(self, root: Path, increment=None, commits=0) -> dict:
        return traceability.trace(root, increment, since=lambda *_: commits)

    def row(self, result: dict, requirement: str) -> dict:
        return next(r for r in result["requirements"] if r["id"] == requirement)

    def test_build_order_puts_each_id_in_its_increment(self) -> None:
        order = traceability.build_order(REQUIREMENTS)
        self.assertEqual(order, {"F1": 1, "S1": 1, "T1": 2, "S2": 2, "D1": "later"})

    def test_increment_filter(self) -> None:
        result = self.trace(self.make_repo({}), increment=1)
        self.assertEqual([r["id"] for r in result["requirements"]], ["F1", "S1"])

    def test_unit_tags_skip_architecture_sections_and_untagged_tests(self) -> None:
        root = self.make_repo({})
        tags = traceability.unit_test_tags(root)
        self.assertEqual([(t["test"], t["ids"]) for t in tags], [("stops", ["S1", "F1"]), ("unknown", ["X9"])])

    def test_unknown_ids_are_reported(self) -> None:
        result = self.trace(self.make_repo({}))
        self.assertEqual([u["id"] for u in result["unknown_ids"]], ["X9"])

    def test_no_test(self) -> None:
        self.assertEqual(self.row(self.trace(self.make_repo({})), "T1")["flags"], ["no test"])

    def test_no_result(self) -> None:
        self.assertEqual(self.row(self.trace(self.make_repo({})), "S2")["flags"], ["no result"])

    def test_latest_report_wins(self) -> None:
        root = self.make_repo({"2026-01-01-a": report("failed"), "2026-02-01-b": report("passed")})
        row = self.row(self.trace(root), "S1")
        self.assertEqual(row["latest_robot_result"]["status"], "passed")
        self.assertEqual(row["latest_robot_result"]["report"], "docs/test-reports/2026-02-01-b")
        self.assertEqual(row["flags"], [])

    def test_not_passed(self) -> None:
        row = self.row(self.trace(self.make_repo({"2026-01-01-a": report("unanswered")})), "S1")
        self.assertEqual(row["latest_robot_result"]["status"], "incomplete")
        self.assertEqual(row["flags"], ["not passed"])

    def test_requirement_named_by_the_test_but_no_check_uses_the_test_status(self) -> None:
        row = self.row(self.trace(self.make_repo({"2026-01-01-a": report("passed")})), "S2")
        self.assertEqual(row["latest_robot_result"]["status"], "passed")

    def test_possibly_stale(self) -> None:
        row = self.row(self.trace(self.make_repo({"2026-01-01-a": report("passed")}), commits=3), "S1")
        self.assertEqual(row["flags"], ["possibly stale: 3 firmware commits since"])

    def test_build_order_problems(self) -> None:
        root = self.make_repo({})
        text = REQUIREMENTS.replace("| Later | D1 |", "| Later | D1, S1, Q7 |").replace("| T1, S2 |", "| S2 |")
        (root / "docs" / "requirements.md").write_text(text, encoding="utf-8")
        problems = self.trace(root)["build_order_problems"]
        self.assertEqual(problems, ["T1 is in no row of the build order",
                                    "S1 is in more than one row of the build order",
                                    "Q7 is in the build order but isn't a requirement"])

    def test_table(self) -> None:
        table = traceability.as_table(self.trace(self.make_repo({"2026-01-01-a": report("passed")}), increment=1))
        self.assertIn("| S1 | safety (1) | `stops` | Passed, 2026-01-01-a, `abc1234` |  |", table)
        self.assertIn("| F1 | safety (1) | none | none |  |", table)


if __name__ == "__main__":
    unittest.main()
