#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["pyfatfs>=1.1.0,<2", "setuptools>=70,<81"]
# ///
"""Create a raw FAT SD-card image from a directory.

Example:

    uv run tools/create-sc64-image.py assets/ sd.img --size 64M
"""

from __future__ import annotations

import argparse
import shutil
import sys
import warnings
from pathlib import Path

# pyfilesystem2 currently imports pkg_resources during startup. Keep its
# deprecation warning from obscuring the tool's own diagnostics.
warnings.filterwarnings("ignore", message="pkg_resources is deprecated as an API", category=UserWarning)

from pyfatfs.PyFat import PyFat
from pyfatfs.PyFatFS import PyFatFS


FAT_TYPES = {12: PyFat.FAT_TYPE_FAT12, 16: PyFat.FAT_TYPE_FAT16, 32: PyFat.FAT_TYPE_FAT32}
SIZE_SUFFIXES = {
    "k": 1024,
    "kb": 1024,
    "m": 1024**2,
    "mb": 1024**2,
    "g": 1024**3,
    "gb": 1024**3,
}


def parse_size(value: str) -> int:
    text = value.strip().lower()
    for suffix, multiplier in sorted(SIZE_SUFFIXES.items(), key=lambda item: -len(item[0])):
        if text.endswith(suffix):
            number = text[: -len(suffix)].strip()
            break
    else:
        number = text
        multiplier = 1

    try:
        size = int(float(number) * multiplier)
    except ValueError as error:
        raise argparse.ArgumentTypeError(f"invalid image size: {value!r}") from error

    if size <= 0 or size % 512:
        raise argparse.ArgumentTypeError("image size must be a positive multiple of 512 bytes")
    return size


def choose_fat_type(size: int, requested: str) -> int:
    if requested != "auto":
        return int(requested)
    if size < 16 * 1024**2:
        return 12
    if size < 512 * 1024**2:
        return 16
    return 32


def copy_tree(source: Path, image: PyFatFS) -> tuple[int, int]:
    files = 0
    directories = 0

    def copy_directory(host_directory: Path, image_directory: str) -> None:
        nonlocal files, directories

        for entry in sorted(host_directory.iterdir(), key=lambda item: item.name.casefold()):
            if entry.is_symlink():
                print(f"warning: skipping symlink: {entry}", file=sys.stderr)
                continue

            destination = f"{image_directory.rstrip('/')}/{entry.name}"
            if entry.is_dir():
                image.makedir(destination, recreate=True)
                directories += 1
                copy_directory(entry, destination)
            elif entry.is_file():
                with entry.open("rb") as input_file, image.openbin(destination, "wb") as output_file:
                    shutil.copyfileobj(input_file, output_file, length=1024 * 1024)
                files += 1

    copy_directory(source, "/")
    return files, directories


def build_image(source: Path, output: Path, size: int, fat_type: int, label: str, force: bool) -> None:
    source = source.expanduser().resolve()
    output = output.expanduser().resolve()

    if not source.is_dir():
        raise ValueError(f"source is not a directory: {source}")
    if source == output or source in output.parents:
        raise ValueError("output image must not be inside the source directory")
    if output.exists() and not force:
        raise ValueError(f"output already exists (use --force to replace it): {output}")

    output.parent.mkdir(parents=True, exist_ok=True)
    if output.exists():
        output.unlink()
    output.touch()

    fat = PyFat()
    fat.mkfs(
        str(output),
        fat_type=FAT_TYPES[fat_type],
        size=size,
        sector_size=512,
        label=label,
        number_of_fats=2,
    )
    fat.close()

    image = PyFatFS(str(output), preserve_case=True, utc=True)
    try:
        files, directories = copy_tree(source, image)
    finally:
        image.close()

    print(f"created {output}")
    print(f"  size: {size} bytes ({size / 1024**2:g} MiB)")
    print(f"  FAT: FAT{fat_type}")
    print(f"  files: {files}")
    print(f"  directories: {directories}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="directory to copy into the image")
    parser.add_argument("output", type=Path, help="raw output image path")
    parser.add_argument("--size", type=parse_size, default=64 * 1024**2, help="image size, for example 64M (default: 64M)")
    parser.add_argument("--fat", choices=("auto", "12", "16", "32"), default="auto", help="FAT type (default: auto)")
    parser.add_argument("--label", default="ARES SD", help="FAT volume label")
    parser.add_argument("--force", action="store_true", help="replace an existing output image")
    args = parser.parse_args()

    if not 1 <= len(args.label.encode("ascii", errors="ignore")) <= 11 or not args.label.isascii():
        parser.error("--label must be 1-11 ASCII characters")

    fat_type = choose_fat_type(args.size, args.fat)
    try:
        build_image(args.source, args.output, args.size, fat_type, args.label, args.force)
    except (OSError, RuntimeError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
