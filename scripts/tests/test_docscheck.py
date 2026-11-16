"""Unit tests for scripts/docscheck.py, on a small made-up repo."""

import shutil
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import docscheck  # noqa: E402

REQUIREMENTS = """# Requirements

## Build order

| Increment | Requirements | Notes |
|---|---|---|
| 1. Skeleton | S1 | |

## Safety

| ID | Requirement |
|---|---|
| S1 | Stop |
"""


class DocsCheckTests(unittest.TestCase):
    def setUp(self) -> None:
        self.root = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.root)
        self.write("docs/requirements.md", REQUIREMENTS)
        for folder in ("decisions", "notebook", "research", "novelties", "test-reports"):
            self.write(f"docs/{folder}/README.md", "# Index\n")

    def write(self, name: str, text: str) -> Path:
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
        return path

    def problems(self, check: str) -> list[str]:
        result = docscheck.check(self.root)
        return [f"{p['where']}: {p['detail']}" for p in result["problems"] if p["check"] == check]

    def test_slug_follows_github(self) -> None:
        self.assertEqual(docscheck.slug("Update, 2026-11-16: parked"), "update-2026-11-16-parked")
        self.assertEqual(docscheck.slug("A5. Health: monitors"), "a5-health-monitors")
        self.assertEqual(docscheck.slug("`dev.py` and [tools](x.md)"), "devpy-and-tools")

    def test_links_and_anchors(self) -> None:
        self.write("docs/a.md", "# Title\n\n## Build order\n\n[ok](b.md) [ok](#build-order) [web](https://x.y)\n"
                                "[bad](missing.md) [bad anchor](b.md#nowhere)\n```\n[in code](nowhere.md)\n```\n")
        self.write("docs/b.md", "# B\n")
        self.assertEqual(self.problems("link"), ["docs/a.md:6: missing.md doesn't exist"])
        self.assertEqual(self.problems("anchor"), ["docs/a.md:6: b.md#nowhere: no heading #nowhere"])

    def test_requirement_ids(self) -> None:
        self.write("docs/a.md", "S1 and S9 matter. The ESP32-S3, pin `D2` and report R3 don't.\n")
        self.write("docs/hardware/pins.md", "D2 is a pin.\n")
        self.assertEqual(self.problems("requirement"), ["docs/a.md:1: S9 isn't in docs/requirements.md"])

    def test_indexes(self) -> None:
        self.write("docs/notebook/README.md", "[Listed](2026-01-01-listed.md) [Gone](2026-01-02-gone.md)\n")
        self.write("docs/notebook/2026-01-01-listed.md", "# Listed\n")
        self.write("docs/notebook/2026-01-03-new.md", "# New\n")
        self.write("docs/test-reports/2026-01-01-increment-1/README.md", "# Report\n")
        self.assertEqual(self.problems("index"), ["docs/notebook/README.md: 2026-01-03-new.md isn't listed",
                                                  "docs/test-reports/README.md: 2026-01-01-increment-1 isn't listed"])
        self.assertEqual(self.problems("link"), ["docs/notebook/README.md:1: 2026-01-02-gone.md doesn't exist"])

    def test_decision_numbers(self) -> None:
        for name in ("0001-a.md", "0002-b.md", "0002-c.md", "0004-d.md"):
            self.write(f"docs/decisions/{name}", "# D\n")
        self.write("docs/decisions/README.md", "".join(f"[x]({n}) " for n in
                                                      ("0001-a.md", "0002-b.md", "0002-c.md", "0004-d.md")))
        self.assertEqual(self.problems("decision"), ["docs/decisions: 0002 is used by 0002-b.md, 0002-c.md",
                                                     "docs/decisions: 0003 is missing"])

    def test_units(self) -> None:
        self.write("firmware/uno/services/safety/README.md", "# Safety\n")
        self.write("firmware/uno/services/safety/library.properties", "name=safety\n")
        self.write("firmware/uno/devices/led/README.md", "# LED\n")
        self.assertEqual(self.problems("unit"), ["firmware/uno/devices/led: no library.properties"])

    def test_clean_repo_is_ok(self) -> None:
        self.assertTrue(docscheck.check(self.root)["ok"])


if __name__ == "__main__":
    unittest.main()
