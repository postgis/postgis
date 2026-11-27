#!/usr/bin/env python3
"""Point privately installed extension scripts at their private libraries."""

import pathlib
import sys

for script in pathlib.Path(sys.argv[1]).iterdir():
    if script.suffix in (".control", ".sql"):
        script.write_text(script.read_text().replace("$libdir", sys.argv[2]))
