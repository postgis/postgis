"""PostGIS compatibility data updater, validator, and payload builder."""

from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[3]
DATA_DIRECTORY = REPOSITORY_ROOT / "doc" / "development" / "compatibility" / "data"
DEFAULT_MATRIX = DATA_DIRECTORY / "matrix.json"
DEFAULT_CACHE = DATA_DIRECTORY / "cache.json"
