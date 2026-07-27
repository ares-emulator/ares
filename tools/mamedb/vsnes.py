"""Transform Nintendo Vs. System MAME XML into deterministic BML metadata."""

from __future__ import annotations

import re
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from typing import Dict, Iterable, List, Optional, Tuple

SOURCE = "nintendo/vsnes.cpp"


@dataclass(frozen=True)
class Machine:
    mapper: str
    protection: str
    input: str
    not_working: bool = False


def machine(
    mapper: str = "mapper-99",
    protection: str = "normal",
    input_mode: str = "standard",
    not_working: bool = False,
) -> Machine:
    return Machine(mapper, protection, input_mode, not_working)


SUPPORTED = {
    "btlecity": machine(),
    "starlstr": machine(),
    "cstlevna": machine("unrom"),
    "cluclu": machine(),
    "drmario": machine("mmc1"),
    "excitebk": machine(),
    "excitebko": machine(),
    "excitebkj": machine(),
    "goonies": machine("vrc1"),
    "iceclimb": machine(),
    "iceclimba": machine(),
    "nvs_machrider": machine(),
    "nvs_machridera": machine(),
    "rbibb": machine("namco-108", protection="rbi-baseball"),
    "rbibba": machine("namco-108", protection="rbi-baseball"),
    "suprmrio": machine(input_mode="swapped"),
    "suprmrioa": machine(input_mode="swapped"),
    "skatekds": machine(input_mode="swapped"),
    "vsskykid": machine("namco-108"),
    "tkoboxng": machine("namco-108", protection="tko-boxing", input_mode="swapped"),
    "smgolf": machine(),
    "smgolfb": machine(),
    "smgolfj": machine(),
    "ladygolfe": machine(),
    "ladygolf": machine(),
    "vspinbal": machine(),
    "vspinbalj": machine(input_mode="swap-ab"),
    "vsslalom": machine(),
    "vssoccer": machine(),
    "vssoccera": machine(),
    "vsgradus": machine("vrc1"),
    "nvs_platoon": machine("sunsoft-3"),
    "vstetris": machine(),
    "nvs_mightybj": machine(input_mode="swapped"),
    "jajamaru": machine(),
    "topgun": machine("unrom"),
    "supxevs": machine("namco-108", protection="super-xevious"),
    "duckhunt": machine(input_mode="zapper"),
    "hogalley": machine(input_mode="zapper"),
    "hogalleyi": machine(input_mode="zapper", not_working=True),
    "vsgshoe": machine(input_mode="zapper"),
    "vsfdf": machine("namco-108", input_mode="zapper"),
}

UNSUPPORTED = {
    "bnglngby": "raid-protection",
    "suprmriobl": "bootleg-z80",
    "suprmriobl2": "bootleg-z80",
    "vstennis": "dualsystem",
    "vstennisa": "dualsystem",
    "vstennisb": "dualsystem",
    "wrecking": "dualsystem",
    "balonfgt": "dualsystem",
    "vsmahjng": "dualsystem",
    "vsbball": "dualsystem",
    "vsbballj": "dualsystem",
    "vsbballja": "dualsystem",
    "vsbballjb": "dualsystem",
    "iceclmrd": "dualsystem",
}


class MetadataError(ValueError):
    pass


def require_classification() -> None:
    overlap = SUPPORTED.keys() & UNSUPPORTED.keys()
    if overlap:
        raise MetadataError(f"duplicate Vs classification: {sorted(overlap)[0]}")
    if len(SUPPORTED) != 42 or len(UNSUPPORTED) != 14:
        raise MetadataError("expected 42 supported and 14 unsupported Vs machines")


def parse_number(value: Optional[str], field: str, machine_name: str) -> int:
    if value is None:
        raise MetadataError(f"missing {field} for {machine_name}")
    base = 16 if value.lower().startswith("0x") else 10
    return int(value, base)


def parse_hex_number(value: Optional[str], field: str, machine_name: str) -> int:
    if value is None:
        raise MetadataError(f"missing {field} for {machine_name}")
    if value.lower().startswith("0x"):
        value = value[2:]
    return int(value, 16)


def normalize_port(value: Optional[str], machine_name: str) -> Optional[str]:
    value = optional_text(value, "DIP port", machine_name)
    if value is None:
        return None
    port = value[1:] if value.startswith(":") else value
    return port


def required_text(value: Optional[str], field: str, machine_name: str) -> str:
    if not value or "\n" in value or "\r" in value:
        raise MetadataError(f"missing {field} for {machine_name}")
    return value


def optional_text(value: Optional[str], field: str, machine_name: str) -> Optional[str]:
    if not value:
        return None
    if "\n" in value or "\r" in value:
        raise MetadataError(f"{field} cannot be emitted for {machine_name}")
    return value


def ppu_devices_for(machine_element: ET.Element) -> List[str]:
    """Return target PPU identifiers independently of palette ROM data."""
    machine_name = machine_element.get("name", "<unnamed>")
    devices: List[str] = []
    for device in machine_element.findall("device_ref"):
        if device.get("tag") != ":ppu1":
            continue
        name = optional_text(device.get("name"), "PPU device identifier", machine_name)
        if name is not None:
            devices.append(name)
    return devices


def bml_number(value: Optional[str], field: str, machine_name: str) -> Optional[str]:
    text = optional_text(value, field, machine_name)
    if text is None:
        return None
    try:
        return f"{parse_number(text, field, machine_name):#x}"
    except ValueError:
        return text


def dips_for(machine_element: ET.Element) -> List[str]:
    result: List[str] = []
    machine_name = machine_element.get("name", "<unnamed>")
    for dip in machine_element.findall("dipswitch"):
        name = optional_text(dip.get("name"), "DIP name", machine_name)
        port = normalize_port(dip.get("tag"), machine_name)
        mask = bml_number(dip.get("mask"), "DIP mask", machine_name)

        choices: List[Tuple[Optional[str], Optional[str], bool]] = []
        for choice in dip.findall("dipvalue"):
            choice_name = optional_text(
                choice.get("name"), "DIP choice name", machine_name
            )
            value = bml_number(choice.get("value"), "DIP choice", machine_name)
            choices.append((choice_name, value, choice.get("default") == "yes"))
        defaults = [value for _, value, default in choices if default]

        result.append("  dip\n")
        if name is not None:
            result.append(f"    name: {name}\n")
        if port is not None:
            result.append(f"    port: {port}\n")
        if mask is not None:
            result.append(f"    mask: {mask}\n")
        for default in defaults:
            if default is not None:
                result.append(f"    default: {default}\n")
        for choice_name, value, _ in choices:
            result.append("    option\n")
            if choice_name is not None:
                result.append(f"      name: {choice_name}\n")
            if value is not None:
                result.append(f"      value: {value}\n")
    return result


def hardware_lines(name: str, ppu_devices: Iterable[str] = ()) -> List[str]:
    if name in UNSUPPORTED:
        return [
            "  hardware\n",
            "    topology: unsupported\n",
            f"    reason: {UNSUPPORTED[name]}\n",
        ]
    item = SUPPORTED[name]
    lines = [
        "  hardware\n",
        "    topology: unisystem\n",
    ]
    for ppu_device in ppu_devices:
        lines.append(f"    ppu: {ppu_device}\n")
    lines.extend(
        (
            f"    mapper: {item.mapper}\n",
            f"    protection: {item.protection}\n",
            f"    input: {item.input}\n",
        )
    )
    if item.not_working:
        lines.append("    upstream-status: not-working\n")
    return lines


@dataclass(frozen=True)
class RomRecord:
    name: Optional[str]
    value: Optional[int]
    load_type: Optional[str]
    offset: int
    size: int
    status: str
    crc: Optional[str]
    sha1: Optional[str]


def parse_rom_record(rom: ET.Element, machine_name: str) -> RomRecord:
    loadflag = rom.get("loadflag")
    legacy_type = rom.get("type")
    if loadflag and legacy_type and loadflag != legacy_type:
        raise MetadataError(f"conflicting ROM load types for {machine_name}")
    raw_value = optional_text(rom.get("value"), "ROM value", machine_name)
    return RomRecord(
        name=optional_text(rom.get("name"), "ROM name", machine_name),
        value=(
            parse_number(raw_value, "ROM value", machine_name)
            if raw_value is not None
            else None
        ),
        load_type=optional_text(loadflag or legacy_type, "ROM load type", machine_name),
        offset=parse_hex_number(rom.get("offset"), "ROM offset", machine_name),
        size=parse_number(rom.get("size"), "ROM size", machine_name),
        status=optional_text(rom.get("status"), "ROM status", machine_name) or "good",
        crc=optional_text(rom.get("crc"), "ROM CRC", machine_name),
        sha1=optional_text(rom.get("sha1"), "ROM SHA1", machine_name),
    )


def bml_rom_lines(record: RomRecord) -> List[str]:
    lines = ["    rom\n"]
    if record.name:
        lines.append(f"      name:   {record.name}\n")
    if record.value is not None:
        lines.append(f"      value:  {record.value}\n")
    if record.load_type:
        lines.append(f"      type:   {record.load_type}\n")
    lines.extend(
        (
            f"      offset: {record.offset:#x}\n",
            f"      size:   {record.size}\n",
        )
    )
    if record.crc is not None:
        lines.append(f"      crc:    {record.crc.lower()}\n")
    if record.sha1 is not None:
        lines.append(f"      sha1:   {record.sha1.lower()}\n")
    if record.status != "good":
        lines.append(f"      status: {record.status}\n")
    return lines


def rom_lines(machine_element: ET.Element) -> List[str]:
    machine_name = machine_element.get("name", "<unnamed>")
    lines: List[str] = []
    current_region = ""
    for rom in machine_element.findall("rom"):
        region = required_text(rom.get("region"), "ROM region", machine_name).replace(
            ":", "-"
        )
        if not re.fullmatch(r"[A-Za-z0-9_.-]+", region):
            raise MetadataError(f"ROM region cannot be emitted for {machine_name}")
        if region != current_region:
            lines.append(f"  {region}\n")
            current_region = region
        record = parse_rom_record(rom, machine_name)
        lines.extend(bml_rom_lines(record))
    return lines


def source_machines(root: ET.Element) -> Tuple[str, Dict[str, ET.Element]]:
    if root.tag != "mame":
        raise MetadataError("input XML is not a MAME machine list")
    mame_revision = required_text(root.get("build"), "MAME build revision", "<root>")
    machines: Dict[str, ET.Element] = {}
    for element in root.findall("machine"):
        if element.get("sourcefile") != SOURCE:
            continue
        name = required_text(element.get("name"), "machine name", "<source>")
        if name in machines:
            raise MetadataError(f"duplicate Vs source machine: {name}")
        machines[name] = element
    validate_inventory(machines)
    return mame_revision, machines


def process_root(root: ET.Element) -> str:
    require_classification()
    mame_revision, machines = source_machines(root)

    output: List[str] = [
        "database\n",
        f"  revision: {mame_revision}\n",
        f"  romset: {mame_revision}\n",
        f"  drivers: {SOURCE}\n",
    ]
    for name in sorted(machines):
        element = machines[name]
        title = required_text(element.findtext("description"), "title", name)
        ppu_devices = ppu_devices_for(element) if name in SUPPORTED else ()
        machine_type = "bios" if element.get("isbios") == "yes" else "game"
        parent = optional_text(element.get("romof"), "parent", name)
        roms = rom_lines(element)
        output.extend(
            (
                "game\n",
                f"  name:    {name}\n",
                f"  title:   {title}\n",
                "  board:   nintendo/vs\n",
                f"  type:    {machine_type}\n",
            )
        )
        if parent:
            output.append(f"  parent:  {parent}\n")
        output.extend(hardware_lines(name, ppu_devices))
        if name in SUPPORTED:
            output.extend(dips_for(element))
        output.extend(roms)
    return "".join(output)


def validate_inventory(names: Iterable[str]) -> None:
    actual = set(names)
    classified = set(SUPPORTED) | set(UNSUPPORTED)
    if actual == classified:
        return
    unknown = sorted(actual - classified)
    missing = sorted(classified - actual)
    details: List[str] = []
    if unknown:
        details.append(f"unclassified source machines: {', '.join(unknown)}")
    if missing:
        details.append(f"classified machines absent from source: {', '.join(missing)}")
    raise MetadataError("; ".join(details))


EXPECTED_MACHINES = frozenset(SUPPORTED) | frozenset(UNSUPPORTED)
