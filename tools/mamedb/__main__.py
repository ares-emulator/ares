"""Command-line entry point for generated MAME metadata databases."""

from __future__ import annotations

import argparse
import contextlib
import os
import stat
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import List, Optional

from . import bml, mame, vsnes
from .config import DRIVERS, SOFTWARE_LISTS


DEFAULT_ARCADE_OUTPUT = (
    Path(__file__).resolve().parents[2] / "mia" / "Database" / "Arcade.bml"
)

DEFAULT_VSNES_OUTPUT = (
    Path(__file__).resolve().parents[2] / "mia" / "Database" / "VsSystem.bml"
)

def write_atomically(destination: Path, contents: str) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    try:
        existing_mode = stat.S_IMODE(destination.stat().st_mode)
    except FileNotFoundError:
        existing_mode = None
    descriptor, temporary_name = tempfile.mkstemp(
        prefix=f".{destination.name}.", dir=destination.parent
    )
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="") as output:
            _ = output.write(contents)
            output.flush()
            os.fsync(output.fileno())
        if existing_mode is not None:
            os.chmod(temporary_name, existing_mode)
        os.replace(temporary_name, destination)
    except BaseException:
        with contextlib.suppress(FileNotFoundError):
            os.unlink(temporary_name)
        raise


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subcommands = parser.add_subparsers(dest="database", required=True)

    arcade_parser = subcommands.add_parser(
        "arcade", help="generate the Arcade.bml database"
    )
    arcade_parser.add_argument(
        "mame_xml", type=Path, help="MAME -listxml output XML file"
    )
    arcade_parser.add_argument(
        "hash_path", type=Path, help="MAME hash directory containing software list XMLs"
    )
    arcade_parser.add_argument(
        "--output", type=Path, default=DEFAULT_ARCADE_OUTPUT,
        help="output BML path (default: repository mia/Database/Arcade.bml)",
    )

    vsnes_parser = subcommands.add_parser(
        "vsnes", help="generate the Nintendo Vs. System database"
    )
    vsnes_parser.add_argument(
        "mame",
        nargs="?",
        help="MAME executable or directory (default: find MAME on PATH)",
    )
    vsnes_parser.add_argument(
        "--jobs",
        type=int,
        default=4,
        help="maximum parallel MAME -listxml queries (default: 4)",
    )
    vsnes_parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_VSNES_OUTPUT,
        help="output BML path (default: repository mia/Database/VsSystem.bml)",
    )
    return parser


def main(argv: Optional[List[str]] = None) -> int:
    parser = _parser()
    args = parser.parse_args(argv)

    try:
        if args.database == "arcade":
            result = bml.generate(
                mame_xml=args.mame_xml,
                hash_path=args.hash_path,
                drivers=DRIVERS,
                software_lists=SOFTWARE_LISTS,
            )
            write_atomically(args.output, result)

        elif args.database == "vsnes":
            jobs = args.jobs
            if jobs < 1:
                parser.error("--jobs must be at least 1")
            executable = mame.resolve_executable(args.mame)
            root = mame.load_source(
                executable, vsnes.SOURCE, vsnes.EXPECTED_MACHINES, jobs
            )
            write_atomically(args.output, vsnes.process_root(root))

    except (
        mame.MameError, vsnes.MetadataError, ET.ParseError, OSError, ValueError
    ) as error:
        parser.exit(1, f"error: {error}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
