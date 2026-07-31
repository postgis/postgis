"""Command-line interface for the PostGIS CI inventory."""

import argparse
import json
import sys

from . import DEFAULT_CONFIG
from .report import (
    ConfigError,
    collect_status,
    exit_code_for_terminal,
    load_config,
    load_status_cache,
    print_terminal,
    write_html_output,
)


def positive_int(value):
    try:
        parsed = int(value)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("must be an integer") from exc
    if parsed <= 0:
        raise argparse.ArgumentTypeError("must be greater than 0")
    return parsed


def parse_args(argv):
    parser = argparse.ArgumentParser(description="Report PostGIS CI status")
    parser.add_argument("legacy_mode", nargs="?", choices=("html",), help=argparse.SUPPRESS)
    parser.add_argument("--branch", help="check one branch name or label")
    parser.add_argument("--config", default=str(DEFAULT_CONFIG))
    parser.add_argument(
        "--format",
        choices=("terminal", "json", "html"),
        default="terminal",
        help="output format",
    )
    parser.add_argument("--output-dir", default="ci-status")
    parser.add_argument(
        "--cache",
        help="reuse successful results from this status.json when their revision is still the branch head",
    )
    parser.add_argument(
        "--atomic-switch",
        action="store_true",
        help="write HTML output to a complete staging directory, then atomically switch the output symlink",
    )
    parser.add_argument("--json", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--no-color", action="store_true")
    parser.add_argument("--verbose", action="store_true", help="show all checks, including passing checks")
    parser.add_argument("--include-eol", action="store_true", help="include configured EOL branches")
    parser.add_argument("--timeout", type=positive_int, default=30, help="per-request timeout in seconds")
    args = parser.parse_args(argv)
    if args.legacy_mode and args.format != "terminal":
        parser.error("legacy html mode cannot be combined with --format")
    if args.json and args.format != "terminal":
        parser.error("--json cannot be combined with --format")
    if args.legacy_mode:
        args.format = "html"
    if args.json:
        args.format = "json"
    return args


def main(argv=None):
    args = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        config = load_config(args.config)
        cache = load_status_cache(args.cache)
        data = collect_status(config, args.branch, args.include_eol, args.timeout, cache)
        if args.format == "html":
            write_html_output(data, args.output_dir, atomic_switch=args.atomic_switch)
            return 0
        if args.format == "json":
            print(json.dumps(data, indent=2, sort_keys=True))
        else:
            use_color = not args.no_color and sys.stdout.isatty()
            print_terminal(data, use_color=use_color, verbose=args.verbose)
        return exit_code_for_terminal(data)
    except ConfigError as exc:
        print(f"ci-status: {exc}", file=sys.stderr)
        return 3
