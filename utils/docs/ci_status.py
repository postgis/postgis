#!/usr/bin/env python3
"""Report the repository-owned PostGIS CI inventory."""

import sys
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from utils.docs.ci_status.cli import main  # noqa: E402


if __name__ == "__main__":
    raise SystemExit(main())
