from __future__ import annotations

import re
import sys
import time
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple

from . import inputs, mame
from .config import SOURCE_FILES
from .exclusions import EXCLUDED_GAMES
from .overrides import ROM_LAYOUTS, ROM_REPLACEMENTS


def _parse_int(value: str) -> int:
    return int(value, 16) if value.lower().startswith("0x") else int(value)


def _split_game_blocks(text: str) -> Tuple[str, List[str]]:
    header, *blocks = re.split(r"(?m)(?=^game[^\S\n]*$)", text)
    return header, blocks


def inherit_software_layout(machine: ET.Element, regions: list[str]) -> list[str]:
    name = machine.get("name", "")
    replacements = ROM_REPLACEMENTS.get(name, {})
    used = set()
    output = []
    for region in re.split(r"(?m)(?=^  [\w-]+\n)", "".join(regions)):
        heading = re.match(r"  ([\w-]+)\n", region)
        if not heading:
            output.append(region)
            continue
        region_name = heading[1]
        roms = re.findall(r"^    rom\n(?:      .*\n)*", region, re.M)
        for i, rom in enumerate(roms):
            fields = dict(re.findall(r"^      (\w+):\s*(.*)$", rom, re.M))
            if "name" not in fields:
                continue
            filename = replacements.get(fields["name"], fields["name"])
            candidates = []
            for candidate in machine.findall("rom"):
                candidate_region = candidate.get("region", "").removeprefix("cslot1:")
                if candidate_region.replace(":", "-") != region_name:
                    continue
                if fields["name"] in replacements:
                    matches = candidate.get("name") == filename
                else:
                    matches = all(
                        fields.get(key) and candidate.get(key) == fields[key]
                        for key in ("crc", "sha1")
                    )
                if matches:
                    candidates.append(candidate)
            if len(candidates) != 1:
                raise ValueError(f"{name}: software layout ROM {filename} is missing or ambiguous")
            source = candidates[0]
            size = int(fields["size"], 0)
            for following in roms[i + 1:]:
                extra = dict(re.findall(r"^      (\w+):\s*(.*)$", following, re.M))
                if extra.get("type") not in ("continue", "ignore"):
                    break
                size += int(extra["size"], 0)
            source_size = int(source.get("size", "0"), 0)
            source_offset = int(source.get("offset", "0"), 16)
            if source_size != size or source_offset != int(fields["offset"], 0):
                raise ValueError(f"{name}/{filename}: software layout size/offset mismatch")
            if fields["name"] not in replacements:
                for key in ("crc", "sha1"):
                    if fields.get(key) != source.get(key):
                        raise ValueError(f"{name}/{filename}: software layout {key} mismatch")
            updated = rom
            for key in ("name", "crc", "sha1"):
                value = source.get(key)
                if not value:
                    raise ValueError(f"{name}/{filename}: missing {key}")
                updated = re.sub(rf"^      {key}:.*$", f"      {key + ':':8s}{value}", updated, flags=re.M)
            region = region.replace(rom, updated, 1)
            used.add(id(source))
        output.append(region)
    cartridge_roms = [r for r in machine.findall("rom") if r.get("region", "").startswith("cslot1:")]
    if any(id(r) not in used for r in cartridge_roms):
        raise ValueError(f"{name}: software layout does not cover all cartridge ROMs")
    return ["".join(output)]


def apply_overrides(bml_text: str) -> str:
    blocks = re.split(r"(?m)(?=^game\n)", bml_text)
    for i, block in enumerate(blocks):
        name = re.search(r"^  name:\s+(.+)$", block, re.M)
        if not name:
            continue
        for region, layout in ROM_LAYOUTS.get(name[1].strip(), {}).items():
            match = re.search(rf"^  {re.escape(region)}\n(?:   .*\n)*", block, re.M)
            if not match:
                # Software lists use region names without the cslot1 prefix.
                if re.search(rf"^  {re.escape(region.removeprefix('cslot1-'))}\n", block, re.M):
                    continue
                raise ValueError(f"{name[1]}: missing override region {region}")
            if re.search(r"^      type:", match[0], re.M):
                continue
            roms = re.findall(r"^    rom\n(?:      .*\n)*", match[0], re.M)
            sizes = tuple(int(re.search(r"^      size:\s+(\S+)", rom, re.M)[1], 0) for rom in roms)
            if sizes != layout.sizes:
                raise ValueError(f"{name[1]}/{region}: ROM sizes changed; review layout override")
            replacement = match[0].split("    rom\n", 1)[0]
            for rom in roms:
                rom = re.sub(r"(^      name:.*\n)", rf"\g<1>      type:   {layout.type}\n", rom, flags=re.M)
                replacement += rom
            block = block[:match.start()] + replacement + block[match.end():]
        blocks[i] = block
    return "".join(blocks)


def convert_machine_list(
    xml_path: Path,
    drivers: List[str],
    existing_text: Optional[str] = None,
    software_data: Optional[Dict[str, SoftwareListData]] = None,
) -> str:
    tree = ET.parse(xml_path)
    root = tree.getroot()
    if root.tag != "mame":
        raise ValueError("XML is not a MAME machine list (expected <mame> root)")

    build = root.get("build", "unknown")
    driver_set = set(drivers)
    machines = {m.get("name"): m for m in root.findall("machine")}
    output = []
    imported_names = set()

    output.append("database\n")
    output.append(f"  revision: {time.strftime('%Y-%m-%d')}\n")
    output.append(f"  romset: {build}\n")
    output.append(f"  drivers: {', '.join(drivers)}\n")

    for machine in root.findall("machine"):
        sourcefile = machine.get("sourcefile", "")
        if sourcefile not in driver_set:
            continue

        name = machine.get("name", "")
        if name in EXCLUDED_GAMES:
            print(f"Skipping {name}: {EXCLUDED_GAMES[name]}", file=sys.stderr)
            continue
        description = machine.findtext("description", "")
        is_bios = machine.get("isbios") == "yes"
        # AES and MVS motherboard variants are not all flagged isbios by MAME.
        # They contain system firmware but no cartridge program, so are not games.
        if sourcefile == "snk/neogeo.cpp":
            has_program = any(
                rom.get("region") in ("maincpu", "cslot1:maincpu")
                for rom in machine.findall("rom")
            )
            if software_data and name in software_data:
                has_program |= any(
                    line in ("  maincpu\n", "  cslot1-maincpu\n")
                    for line in software_data[name].rom_regions
                )
            if not has_program:
                is_bios = True
        parent = machine.get("romof", "")
        machine_type = "bios" if is_bios else "game"

        print(f"found {machine_type}: {name} ({description})")
        imported_names.add(name)

        output.append("game\n")
        output.append(f"  name:    {name}\n")
        output.append(f"  title:   {description}\n")
        output.append(f"  board:   {sourcefile.removesuffix('.cpp')}\n")

        for chip in machine.findall("chip"):
            if chip.get("type") == "cpu" and chip.get("tag") == "maincpu":
                cpu_name = chip.get("name", "")
                if cpu_name:
                    output.append(f"  cpu:     {cpu_name}\n")
                break

        for display in machine.findall("display"):
            rotate = display.get("rotate", "0")
            if rotate and rotate != "0":
                output.append(f"  rotation: {rotate}\n")
                break

        output.append(f"  type:    {machine_type}\n")
        if parent:
            output.append(f"  parent:  {parent}\n")

        if software_data and name in software_data:
            entry = software_data[name]
            if entry.release:
                output.append(f"  release: {entry.release}\n")
            if entry.compatibility:
                output.append(f"  compatibility: {entry.compatibility}\n")
        elif sourcefile == "snk/neogeo.cpp" and not is_bios:
            # Machine-only arcade sets do not establish an AES release.
            output.append("  release: MVS\n")

        if software_data and name in software_data:
            for feature_bml in software_data[name].features:
                output.append(feature_bml)

        for biosset in machine.findall("biosset"):
            bios_id = biosset.get("name", "")
            for rom in machine.findall("rom"):
                if rom.get("bios") == bios_id and rom.get("region") == "mainbios":
                    if rom.get("name"):
                        output.append("  bios\n")
                        output.append(f"    name: {biosset.get('description', '')}\n")
                        output.append(f"    rom:  {rom.get('name', '')}\n")
                        if biosset.get("default") == "yes":
                            output.append("    default: yes\n")
                        break

        for dip in machine.findall("dipswitch"):
            output.append("  dip\n")
            output.append(f"    name:    {dip.get('name', '')}\n")
            output.append(f"    port:    {dip.get('tag', '')}\n")
            output.append(f"    mask:    {int(dip.get('mask', '0'))}\n")
            for option in dip.findall("dipvalue"):
                if option.get("default") == "yes":
                    output.append(f"    default: {int(option.get('value', '0'))}\n")
            for option in dip.findall("dipvalue"):
                output.append("    option\n")
                output.append(f"      name:  {option.get('name', '')}\n")
                output.append(f"      value: {int(option.get('value', '0'))}\n")

        for inp in machine.findall("input"):
            output.append("  input\n")
            if inp.get("players"):
                output.append(f"    players: {int(inp.get('players'))}\n")
            if inp.get("coins"):
                output.append(f"    coins:   {int(inp.get('coins'))}\n")
            if inp.get("service") == "yes":
                output.append("    service: yes\n")
            if inp.get("tilt") == "yes":
                output.append("    tilt:    yes\n")
            for control in inp.findall("control"):
                output.append("    control\n")
                for attr in ("type", "player", "buttons", "ways", "minimum", "maximum"):
                    val = control.get(attr)
                    if val is not None:
                        if attr in ("player", "buttons", "minimum", "maximum"):
                            output.append(f"      {attr + ':':10s}{int(val)}\n")
                        else:
                            output.append(f"      {attr + ':':10s}{val}\n")

        # ROM regions — prefer software lists; XML clone relationships identify
        # candidates for machine-only variants, but do not prove identical wiring.
        regions = None
        if software_data and name in software_data and software_data[name].rom_regions:
            regions = software_data[name].rom_regions
        elif software_data:
            parents = [machine.get("cloneof"), machine.get("romof")]
            visited = {name}
            while parents:
                parent = parents.pop(0)
                if not parent or parent in visited:
                    continue
                visited.add(parent)
                if parent in software_data and software_data[parent].rom_regions:
                    try:
                        regions = inherit_software_layout(machine, software_data[parent].rom_regions)
                        break
                    except ValueError as error:
                        if name in ROM_REPLACEMENTS:
                            raise  # A declared replacement must remain compatible.
                        print(f"Not inheriting {parent} layout for {name}: {error}", file=sys.stderr)
                if parent in machines:
                    ancestor = machines[parent]
                    parents.extend([ancestor.get("cloneof"), ancestor.get("romof")])
        if name in ROM_REPLACEMENTS and regions is None:
            raise ValueError(f"{name}: no compatible parent software-list layout")
        if regions is not None:
            output.extend(regions)
        else:
            current_region = ""
            for rom in machine.findall("rom"):
                region = (rom.get("region") or "").replace(":", "-")
                if region != current_region:
                    output.append(f"  {region}\n")
                    current_region = region
                output.append("    rom\n")
                if rom.get("name"):
                    output.append(f"      name:   {rom.get('name')}\n")
                if rom.get("value"):
                    output.append(f"      value:  {int(rom.get('value'))}\n")
                output.append(f"      offset: 0x{rom.get('offset', '0')}\n")
                output.append(f"      size:   {int(rom.get('size', '0'))}\n")
                output.append(f"      crc:    {rom.get('crc', '')}\n")
                output.append(f"      sha1:   {rom.get('sha1', '')}\n")

    if existing_text:
        _preserve_custom_entries(existing_text, imported_names, output)

    return "".join(output)


@dataclass
class SoftwareListData:
    release: str = ""
    compatibility: str = ""
    features: List[str] = field(default_factory=list)
    rom_regions: List[str] = field(default_factory=list)


def convert_software_list(
    xml_path: Path,
    board: str,
    existing_text: Optional[str] = None,
    software_data: Optional[Dict[str, SoftwareListData]] = None,
) -> Tuple[str, Dict[str, SoftwareListData]]:
    tree = ET.parse(xml_path)
    root = tree.getroot()
    if root.tag != "softwarelist":
        raise ValueError("XML is not a MAME software list (expected <softwarelist> root)")

    output = []
    imported_names = set()
    if software_data is None:
        software_data = {}

    output.append("database\n")
    output.append(f"  revision: {time.strftime('%Y-%m-%d')}\n")

    for software in root.findall("software"):
        name = software.get("name", "")
        if name in EXCLUDED_GAMES:
            print(f"Skipping {name}: {EXCLUDED_GAMES[name]}", file=sys.stderr)
            continue
        description = software.findtext("description", "")

        print(f"found {name} ({description})")
        imported_names.add(name)

        shared_features = {feature.get("name"): feature.get("value", "")
                           for feature in software.findall("sharedfeat")}
        entry = SoftwareListData(
            release=shared_features.get("release", ""),
            compatibility=shared_features.get("compatibility", ""),
        )

        output.append("game\n")
        output.append(f"  name:  {name}\n")
        output.append(f"  title: {description}\n")
        output.append(f"  board: {board}\n")
        if parent := software.get("cloneof"):
            output.append(f"  parent: {parent}\n")
        if entry.release:
            output.append(f"  release: {entry.release}\n")
        if entry.compatibility:
            output.append(f"  compatibility: {entry.compatibility}\n")

        part = software.find("part")
        if part is not None:
            for feature in part.findall("feature"):
                feature_bml = (
                    f"  feature\n"
                    f"    name:  {feature.get('name', '')}\n"
                    f"    value: {feature.get('value', '')}\n"
                )
                output.append(feature_bml)
                entry.features.append(feature_bml)

            for dataarea in part.findall("dataarea"):
                region = dataarea.get("name", "").replace(":", "-")
                region_lines = []
                region_lines.append(f"  {region}\n")
                size = dataarea.get("size")
                if size:
                    region_lines.append(f"    size: {_parse_int(size)}\n")
                for rom in dataarea.findall("rom"):
                    region_lines.append("    rom\n")
                    if rom.get("name"):
                        region_lines.append(f"      name:   {rom.get('name')}\n")
                    loadflag = rom.get("loadflag")
                    if loadflag:
                        region_lines.append(f"      type:   {loadflag}\n")
                    if rom.get("value"):
                        region_lines.append(f"      value:  {_parse_int(rom.get('value'))}\n")
                    region_lines.append(f"      offset: {_parse_int(rom.get('offset', '0'))}\n")
                    region_lines.append(f"      size:   {_parse_int(rom.get('size', '0'))}\n")
                    if rom.get("crc"):
                        region_lines.append(f"      crc:    {rom.get('crc')}\n")
                    if rom.get("sha1"):
                        region_lines.append(f"      sha1:   {rom.get('sha1')}\n")
                output.extend(region_lines)
                entry.rom_regions.extend(region_lines)

        software_data[name] = entry
        output.append("\n")

    if existing_text:
        _preserve_custom_entries(existing_text, imported_names, output)

    return "".join(output), software_data


def _preserve_custom_entries(
    existing_text: str, imported_names: Set[str], output: List[str]
) -> None:
    preserved = 0
    _, blocks = _split_game_blocks(existing_text)
    for block in blocks:
        match = re.search(r"^  name:\s+(.+)$", block, re.M)
        if not match:
            continue
        name = match[1].strip()
        if name in imported_names or name in EXCLUDED_GAMES:
            continue
        output.append(block)
        preserved += 1
    if preserved:
        print(f"preserved {preserved} custom entries from existing database")


def enrich_bml(text: str, game_filter: Optional[List[str]] = None) -> str:
    header, blocks = _split_game_blocks(text)
    return header + "".join(inputs.enrich(blocks, game_filter))


def parse_rom_copies(source: str) -> Dict[str, List[Tuple[str, int, int, int]]]:
    # Machine XML omits ROM_COPY; read it from the driver source.
    copies = {}
    for block in re.finditer(r"ROM_START\(\s*(\w+)\s*\)(.*?)ROM_END", source, re.S):
        for m in re.finditer(r'ROM_COPY\(\s*"([^"]+)"\s*,\s*(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*\)', block[2]):
            regions = list(re.finditer(r'ROM_REGION\w*\(\s*[^,]+,\s*"([^"]+)"', block[2][:m.start()]))
            # Cross-region copies are already represented by the software lists.
            # A copy record can only duplicate bytes within its own region.
            if not regions or regions[-1][1] != m[1]:
                continue
            copies.setdefault(block[1], []).append(
                (m[1].replace(":", "-"), int(m[2], 0), int(m[3], 0), int(m[4], 0))
            )
    return copies


def add_rom_copies(bml_text: str, drivers: List[str]) -> str:
    copies = {}
    for board in sorted({d.removesuffix(".cpp") for d in drivers}):
        for path in SOURCE_FILES.get(board, [f"src/mame/{board}.cpp"]):
            copies.update(parse_rom_copies(mame.fetch_source(path)))

    header, blocks = _split_game_blocks(bml_text)
    output = [header]
    for block in blocks:
        name_m = re.search(r"  name:\s+(.+)", block)
        for region, source_offset, destination, length in copies.get(name_m[1].strip() if name_m else "", []):
            for candidate in (region, region.removeprefix("cslot1-")):
                m = re.search(rf"^  {re.escape(candidate)}\n(?:   .*\n)*", block, re.M)
                if m:
                    break
            if not m:
                continue
            record = (
                "    rom\n"
                "      type:   copy\n"
                f"      from:   0x{source_offset:x}\n"
                f"      offset: 0x{destination:x}\n"
                f"      size:   {length}\n"
            )
            block = block[:m.end()] + record + block[m.end():]
            print(f"  {name_m[1].strip()}: ROM_COPY into {region}", file=sys.stderr)
        output.append(block)
    return "".join(output)


def generate(
    mame_xml: Path,
    hash_path: Path,
    drivers: List[str],
    software_lists: Dict[str, str],
) -> str:
    result = None
    sw_data = {}

    for mame_name, board in sorted(software_lists.items()):
        xml_file = hash_path / f"{mame_name}.xml"
        print(f"Importing software list: {mame_name}...", file=sys.stderr)
        result, sw_data = convert_software_list(xml_file, board, result, sw_data)

    print("Importing machine list...", file=sys.stderr)
    result = convert_machine_list(mame_xml, drivers, result, sw_data)

    print("Enriching with input mappings from MAME source...", file=sys.stderr)
    result = enrich_bml(result)

    print("Applying ROM layout overrides...", file=sys.stderr)
    result = apply_overrides(result)

    print("Adding ROM_COPY records from MAME source...", file=sys.stderr)
    result = add_rom_copies(result, drivers)

    return result
