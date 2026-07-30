"""Generic acquisition of machine metadata from a local MAME executable."""

from __future__ import annotations

import concurrent.futures
import re
import shutil
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Set, Tuple


class MameError(RuntimeError):
    """A normalized executable, process, inventory, or XML acquisition error."""


def resolve_executable(value: Optional[str]) -> Path:
    if value is None:
        discovered = shutil.which("mame") or shutil.which("mame.exe")
        if discovered is None:
            raise MameError(
                "MAME could not be found on PATH; pass its executable or directory"
            )
        return Path(discovered)

    supplied = Path(value).expanduser()
    if supplied.is_dir():
        for filename in ("mame", "mame.exe"):
            candidate = supplied / filename
            if candidate.is_file():
                return candidate
        raise MameError(f"MAME executable not found in directory: {supplied}")
    if not supplied.is_file():
        raise MameError(f"MAME executable not found: {supplied}")
    return supplied


def _decode_output(output: object, context: str) -> str:
    if isinstance(output, str):
        return output
    if not isinstance(output, bytes):
        raise MameError(f"MAME returned invalid output for {context}")
    try:
        return output.decode("utf-8-sig")
    except UnicodeDecodeError as error:
        raise MameError(
            f"MAME returned non-UTF-8 output for {context}: {error}"
        ) from error


def _decode_stderr(output: object, fallback: bytes) -> str:
    """Normalize real and mocked subprocess stderr without masking failures."""
    if isinstance(output, str):
        return output
    if isinstance(output, bytes):
        return output.decode("utf-8", errors="replace")
    return fallback.decode("utf-8", errors="replace")


def _run(executable: Path, arguments: List[str], context: str) -> str:
    command = [str(executable), *arguments]
    try:
        with tempfile.TemporaryFile() as stderr_file:
            completed = subprocess.run(
                command, stdout=subprocess.PIPE, stderr=stderr_file, check=False
            )
            _ = stderr_file.seek(0)
            stderr_bytes = stderr_file.read(65536)
    except OSError as error:
        raise MameError(
            f"could not run MAME command for {context}: {command!r}: {error}"
        ) from error
    stderr = _decode_stderr(completed.stderr, stderr_bytes)
    if completed.returncode != 0:
        detail = " ".join(stderr.split())
        if len(detail) > 1000:
            detail = detail[:997] + "..."
        message = (
            f"MAME command failed for {context} with exit code "
            f"{completed.returncode}: {command!r}"
        )
        if detail:
            message += f": {detail}"
        raise MameError(message)
    return _decode_output(completed.stdout, context)


def _inventory(output: str, source: str) -> List[str]:
    names: List[str] = []
    seen: Set[str] = set()
    for line in output.splitlines():
        fields = line.split()
        if len(fields) < 2 or fields[-1].replace("\\", "/") != source:
            continue
        name = fields[0]
        if not re.fullmatch(r"[A-Za-z0-9_]+", name):
            raise MameError(
                f"invalid {source} machine name in MAME -listsource: {name!r}"
            )
        if name in seen:
            raise MameError(f"duplicate {source} machine in MAME -listsource: {name}")
        seen.add(name)
        names.append(name)
    if not names:
        raise MameError(f"MAME -listsource contains no machines for {source}")
    return sorted(names)


def _check_inventory(actual: Iterable[str], expected: Iterable[str]) -> None:
    actual_names = set(actual)
    expected_names = set(expected)
    if actual_names == expected_names:
        return
    details: List[str] = []
    unknown = sorted(actual_names - expected_names)
    missing = sorted(expected_names - actual_names)
    if unknown:
        details.append(f"unexpected source machines: {', '.join(unknown)}")
    if missing:
        details.append(f"expected machines absent from source: {', '.join(missing)}")
    raise MameError("; ".join(details))


def _query_machine(executable: Path, source: str, name: str) -> Tuple[str, ET.Element]:
    output = _run(executable, ["-listxml", name], f"machine {name}")
    try:
        root = ET.fromstring(output)
    except ET.ParseError as error:
        raise MameError(f"invalid MAME XML for machine {name}: {error}") from error
    if root.tag != "mame":
        raise MameError(f"MAME -listxml target {name} did not return a mame root")
    revision = root.get("build")
    if not revision:
        raise MameError(f"MAME -listxml target {name} has no build revision")
    named = [item for item in root.findall("machine") if item.get("name") == name]
    selected = [item for item in named if item.get("sourcefile") == source]
    if len(selected) != 1:
        if named and not selected:
            sources = sorted({item.get("sourcefile", "<missing>") for item in named})
            raise MameError(
                "MAME -listxml target {} has wrong source: {}; expected {}".format(
                    name, ", ".join(sources), source
                )
            )
        condition = "missing" if not selected else "duplicate"
        raise MameError(
            f"{condition} requested {source} machine in MAME -listxml target {name}"
        )
    return revision, selected[0]


def load_source(
    executable: Path, source: str, expected_machines: Iterable[str], jobs: int = 4
) -> ET.Element:
    """Load one complete source-driver inventory into a deterministic XML root."""
    if jobs < 1:
        raise MameError("--jobs must be at least 1")
    names = _inventory(_run(executable, ["-listsource"], source), source)
    _check_inventory(names, expected_machines)
    results: Dict[str, Tuple[str, ET.Element]] = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as executor:
        futures = {
            executor.submit(_query_machine, executable, source, name): name
            for name in names
        }
        for future in concurrent.futures.as_completed(futures):
            name = futures[future]
            try:
                results[name] = future.result()
            except MameError:
                for pending in futures:
                    _ = pending.cancel()
                raise
            except Exception as error:
                for pending in futures:
                    _ = pending.cancel()
                raise MameError(
                    f"worker failed for MAME -listxml machine {name}: {error}"
                ) from error
    revisions = {revision for revision, _ in results.values()}
    if len(revisions) != 1:
        details = ", ".join(f"{name}={results[name][0]}" for name in sorted(results))
        raise MameError(
            f"MAME build revision mismatch across machine queries: {details}"
        )
    root = ET.Element("mame", build=next(iter(revisions)))
    for name in sorted(results):
        root.append(results[name][1])
    return root
