#!/usr/bin/env python3
"""Update, validate, or export PostGIS compatibility data."""

import sys
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from utils.docs.support_matrix.cli import main  # noqa: E402


if __name__ == "__main__":
    raise SystemExit(main())
