from __future__ import annotations

import re
import sys
from copy import deepcopy
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

from . import mame
from .config import SOURCE_FILES


_IPT_MAP = {
    "IPT_JOYSTICK_UP":    ("JOYSTICK_UP", True),
    "IPT_JOYSTICK_DOWN":  ("JOYSTICK_DOWN", True),
    "IPT_JOYSTICK_LEFT":  ("JOYSTICK_LEFT", True),
    "IPT_JOYSTICK_RIGHT": ("JOYSTICK_RIGHT", True),
    "IPT_BUTTON1":        ("BUTTON1", True),
    "IPT_BUTTON2":        ("BUTTON2", True),
    "IPT_BUTTON3":        ("BUTTON3", True),
    "IPT_BUTTON4":        ("BUTTON4", True),
    "IPT_BUTTON5":        ("BUTTON5", True),
    "IPT_BUTTON6":        ("BUTTON6", True),
    "IPT_BUTTON7":        ("BUTTON7", True),
    "IPT_BUTTON8":        ("BUTTON8", True),
    "IPT_START":          ("START", True),
    "IPT_SELECT":         ("SELECT", True),
    "IPT_START1":         ("START1", False),
    "IPT_START2":         ("START2", False),
    "IPT_START3":         ("START3", False),
    "IPT_START4":         ("START4", False),
    "IPT_COIN1":          ("COIN1", False),
    "IPT_COIN2":          ("COIN2", False),
    "IPT_COIN3":          ("COIN3", False),
    "IPT_COIN4":          ("COIN4", False),
    "IPT_SERVICE":        ("SERVICE", False),
    "IPT_SERVICE1":       ("SERVICE1", False),
    "IPT_PADDLE":         ("PADDLE", True),
    "IPT_PEDAL":          ("PEDAL", True),
    "IPT_DIAL":           ("DIAL", True),
    "IPT_TRACKBALL_X":    ("TRACKBALL_X", True),
    "IPT_TRACKBALL_Y":    ("TRACKBALL_Y", True),
    "IPT_LIGHTGUN_X":     ("LIGHTGUN_X", True),
    "IPT_LIGHTGUN_Y":     ("LIGHTGUN_Y", True),
    "IPT_POSITIONAL":     ("POSITIONAL", True),
}

_PORT_BIT_RE = re.compile(
    r"PORT_BIT\s*\(\s*(0x[0-9a-fA-F]+|\d+)\s*,"
    r"\s*(IP_ACTIVE_LOW|IP_ACTIVE_HIGH)\s*,"
    r"\s*(IPT_\w+)\s*\)"
    r"(.*)"
)

_PORT_BIT_ANALOG_RE = re.compile(
    r"PORT_BIT\s*\(\s*(0x[0-9a-fA-F]+|\d+)\s*,"
    r"\s*(0x[0-9a-fA-F]+|\d+)\s*,"
    r"\s*(IPT_\w+)\s*\)"
    r"(.*)"
)

_PORT_PLAYER_RE = re.compile(r"PORT_PLAYER\s*\(\s*(\d+)\s*\)")
_PORT_NAME_RE = re.compile(r'PORT_NAME\s*\(\s*"([^"]+)"\s*\)')
_PORT_START_RE = re.compile(r'PORT_START\s*\(\s*"([^"]+)"\s*\)')
_PORT_MODIFY_RE = re.compile(r'PORT_MODIFY\s*\(\s*"([^"]+)"\s*\)')
_INPUT_PORTS_START_RE = re.compile(r"INPUT_PORTS_START\s*\(\s*(\w+)\s*\)")
_PORT_INCLUDE_RE = re.compile(r"PORT_INCLUDE\s*\(\s*(\w+)\s*\)")
_PORT_SERVICE_RE = re.compile(
    r"PORT_SERVICE_NO_TOGGLE\s*\(\s*(0x[0-9a-fA-F]+|\d+)\s*,"
    r"\s*(IP_ACTIVE_LOW|IP_ACTIVE_HIGH)\s*\)"
)


@dataclass
class PortBit:
    mask: int
    ipt_type: str
    player: int = 1
    name: Optional[str] = None

    @property
    def token(self) -> Optional[str]:
        spec = _IPT_MAP.get(self.ipt_type)
        if spec is None:
            return None
        base, player_relative = spec
        if player_relative:
            return f"P{self.player}_{base}"
        return base


@dataclass
class Port:
    bits: List[PortBit] = field(default_factory=list)


@dataclass
class InputPorts:
    ports: Dict[str, Port] = field(default_factory=dict)


def _parse_int(value: str) -> int:
    if value.lower().startswith("0x"):
        return int(value, 16)
    return int(value)


def _parse_port_bit_line(line: str) -> Optional[PortBit]:
    service = _PORT_SERVICE_RE.search(line)
    if service:
        return PortBit(
            mask=_parse_int(service[1]),
            ipt_type="IPT_SERVICE", player=0,
        )
    m = _PORT_BIT_RE.search(line)
    if not m:
        m = _PORT_BIT_ANALOG_RE.search(line)
    if not m:
        return None
    mask = _parse_int(m[1])
    ipt_type = m[3]
    if ipt_type in ("IPT_UNUSED", "IPT_UNKNOWN", "IPT_CUSTOM", "IPT_OTHER"):
        return None
    rest = m[4]
    player_m = _PORT_PLAYER_RE.search(rest)
    player = int(player_m[1]) if player_m else 1
    name_m = _PORT_NAME_RE.search(rest)
    name = name_m[1] if name_m else None
    return PortBit(
        mask=mask,
        ipt_type=ipt_type, player=player, name=name,
    )


def parse_input_ports(source: str) -> Dict[str, InputPorts]:
    blocks = {}
    raw_blocks = {}
    current_name = None
    current_lines = []

    for raw_line in source.splitlines():
        line = raw_line.strip()
        if line.startswith("//"):
            continue
        m = _INPUT_PORTS_START_RE.search(line)
        if m:
            current_name = m[1]
            current_lines = []
            continue
        if current_name is not None:
            if "INPUT_PORTS_END" in line:
                raw_blocks[current_name] = current_lines
                current_name = None
                current_lines = []
            else:
                current_lines.append(line)

    def resolve(name: str) -> InputPorts:
        if name in blocks:
            return blocks[name]
        lines = raw_blocks.get(name)
        if lines is None:
            blocks[name] = InputPorts()
            return blocks[name]
        result = InputPorts()
        current_port = None
        modifying = None
        for line in lines:
            inc = _PORT_INCLUDE_RE.search(line)
            if inc:
                parent = resolve(inc[1])
                for tag, port in deepcopy(parent.ports).items():
                    if tag not in result.ports:
                        result.ports[tag] = port
                    else:
                        result.ports[tag].bits.extend(port.bits)
                continue
            start = _PORT_START_RE.search(line)
            if start:
                tag = start[1]
                current_port = Port()
                result.ports[tag] = current_port
                modifying = None
                continue
            mod = _PORT_MODIFY_RE.search(line)
            if mod:
                tag = mod[1]
                if tag in result.ports:
                    current_port = result.ports[tag]
                    modifying = tag
                else:
                    current_port = Port()
                    result.ports[tag] = current_port
                    modifying = tag
                continue
            if current_port is None:
                continue
            bit = _parse_port_bit_line(line)
            if bit is None:
                continue
            if modifying is not None:
                current_port.bits = [
                    b for b in current_port.bits if b.mask != bit.mask
                ]
            current_port.bits.append(bit)
        blocks[name] = result
        return result

    for name in raw_blocks:
        resolve(name)
    return blocks


def _resolve_player(token: str, parser_player: int) -> int:
    m = re.fullmatch(r"(COIN|START)(\d+)", token)
    if m:
        return int(m[2])
    if token in ("SERVICE", "SERVICE1"):
        return 1
    return parser_player


def _token_to_label(token: str, player: int) -> str:
    m = re.fullmatch(r"P(\d+)_(.*)", token)
    if m:
        suffix = m[2]
        labels = {
            "JOYSTICK_UP": "Up", "JOYSTICK_DOWN": "Down",
            "JOYSTICK_LEFT": "Left", "JOYSTICK_RIGHT": "Right",
            "BUTTON1": "Button 1", "BUTTON2": "Button 2",
            "BUTTON3": "Button 3", "BUTTON4": "Button 4",
            "BUTTON5": "Button 5", "BUTTON6": "Button 6",
            "BUTTON7": "Button 7", "BUTTON8": "Button 8",
            "START": "Start", "SELECT": "Coin",
            "PADDLE": "Paddle", "PEDAL": "Pedal 1",
            "DIAL": "Dial", "TRACKBALL_X": "Track X",
            "TRACKBALL_Y": "Track Y", "LIGHTGUN_X": "Gun X",
            "LIGHTGUN_Y": "Gun Y", "POSITIONAL": "Positional",
        }
        readable = labels.get(suffix, suffix.replace("_", " ").title())
        return f"P{player} {readable}"
    global_labels = {
        "COIN1": "Coin 1", "COIN2": "Coin 2",
        "COIN3": "Coin 3", "COIN4": "Coin 4",
        "START1": "1 Player Start", "START2": "2 Players Start",
        "START3": "3 Players Start", "START4": "4 Players Start",
        "SERVICE": "Service Mode", "SERVICE1": "Service 1",
    }
    return global_labels.get(token, token)


def _token_to_name(token: str, player: int) -> str:
    names = {
        "JOYSTICK_UP": "Up", "JOYSTICK_DOWN": "Down",
        "JOYSTICK_LEFT": "Left", "JOYSTICK_RIGHT": "Right",
        "BUTTON1": "Button 1", "BUTTON2": "Button 2",
        "BUTTON3": "Button 3", "BUTTON4": "Button 4",
        "BUTTON5": "Button 5", "BUTTON6": "Button 6",
        "BUTTON7": "Button 7", "BUTTON8": "Button 8",
        "START": "Start", "SELECT": "Coin",
        "PADDLE": "X-Axis", "PEDAL": "Accelerator",
        "DIAL": "X-Axis", "TRACKBALL_X": "X-Axis",
        "TRACKBALL_Y": "Y-Axis", "LIGHTGUN_X": "X-Axis",
        "LIGHTGUN_Y": "Y-Axis", "POSITIONAL": "X-Axis",
    }
    m = re.fullmatch(r"P(\d+)_(.*)", token)
    if m:
        suffix = m[2]
        readable = names.get(suffix, suffix.replace("_", " ").title())
        return f"Player {player} {readable}"
    global_names = {
        "COIN1": "Player 1 Coin", "COIN2": "Player 2 Coin",
        "COIN3": "Player 3 Coin", "COIN4": "Player 4 Coin",
        "START1": "Player 1 Start", "START2": "Player 2 Start",
        "START3": "Player 3 Start", "START4": "Player 4 Start",
        "SERVICE": "Test", "SERVICE1": "Service",
    }
    return global_names.get(token, token)


def _token_to_default(token: str) -> str:
    m = re.fullmatch(r"P(\d+)_(.*)", token)
    suffix = m[2] if m else token
    defaults = {
        "JOYSTICK_UP": "up", "JOYSTICK_DOWN": "down",
        "JOYSTICK_LEFT": "left", "JOYSTICK_RIGHT": "right",
        "START": "start", "SELECT": "select",
    }
    if suffix in defaults:
        return defaults[suffix]
    button_defaults = {
        "BUTTON1": "south", "BUTTON2": "east",
        "BUTTON3": "west", "BUTTON4": "north",
        "BUTTON5": "l_bumper", "BUTTON6": "r_bumper",
        "BUTTON7": "l_trigger", "BUTTON8": "r_trigger",
    }
    if suffix in button_defaults:
        return button_defaults[suffix]
    global_defaults = {
        "COIN1": "select", "COIN2": "select",
        "START1": "start", "START2": "start",
        "SERVICE": "rstick_click", "SERVICE1": "lstick_click",
    }
    return global_defaults.get(suffix, "")


def inputs_to_actions(ports: InputPorts) -> List[Dict[str, str]]:
    actions = []
    seen_tokens = set()
    for port in ports.ports.values():
        for bit in port.bits:
            token = bit.token
            if token is None or token in seen_tokens:
                continue
            seen_tokens.add(token)
            player = _resolve_player(token, bit.player)
            name = _token_to_name(token, player)
            label = bit.name.replace("%p", f"P{player}") if bit.name else _token_to_label(token, player)
            default = _token_to_default(token)
            entry = {
                "token": token, "name": name,
                "label": label, "player": str(player),
            }
            if default:
                entry["default"] = default
            actions.append(entry)
    return actions


def fetch_ports(board: str) -> Dict[str, InputPorts]:
    ports = {}
    for path in SOURCE_FILES.get(board, [f"src/mame/{board}.cpp"]):
        ports.update(parse_input_ports(mame.fetch_source(path)))
    return ports


_SIX_BUTTON_LAYOUT = (
    "west", "north", "r_bumper",
    "south", "east", "r_trigger",
    "l_bumper", "l_trigger",
)
_FOUR_BUTTON_LAYOUT = (
    "south", "east", "west", "north",
    "l_bumper", "r_bumper", "l_trigger", "r_trigger",
)


def _button_defaults(count: int) -> Tuple[str, ...]:
    if count >= 6:
        return _SIX_BUTTON_LAYOUT
    return _FOUR_BUTTON_LAYOUT


def _generate_actions_from_ports(
    ports: InputPorts,
    button_count: int,
) -> List[Dict[str, str]]:
    raw = inputs_to_actions(ports)
    layout = _button_defaults(button_count)

    for action in raw:
        m = re.fullmatch(r"P(\d+)_BUTTON(\d+)", action["token"])
        if m:
            index = int(m[2]) - 1
            if index < len(layout):
                action["default"] = layout[index]

    return raw


def _generate_standard_actions(
    players: int,
    button_count: int,
    control_type: str,
) -> List[Dict[str, str]]:
    actions = []
    layout = _button_defaults(button_count)
    if players > 5:
        players = 5

    for p in range(1, players + 1):
        if control_type in ("lightgun", "positional"):
            for token_suffix, name_suffix in (
                ("LIGHTGUN_X", "X-Axis"),
                ("LIGHTGUN_Y", "Y-Axis"),
            ):
                actions.append({
                    "token": f"P{p}_{token_suffix}",
                    "name": f"Player {p} {name_suffix}",
                    "label": f"P{p} {name_suffix}",
                    "player": str(p),
                })
        else:
            for dir_token, dir_name, dir_default in (
                ("JOYSTICK_UP", "Up", "up"),
                ("JOYSTICK_DOWN", "Down", "down"),
                ("JOYSTICK_LEFT", "Left", "left"),
                ("JOYSTICK_RIGHT", "Right", "right"),
            ):
                actions.append({
                    "token": f"P{p}_{dir_token}",
                    "name": f"Player {p} {dir_name}",
                    "label": f"P{p} {dir_name}",
                    "player": str(p),
                    "default": dir_default,
                })

        for b in range(1, button_count + 1):
            default = layout[b - 1] if b - 1 < len(layout) else ""
            actions.append({
                "token": f"P{p}_BUTTON{b}",
                "name": f"Player {p} Button {b}",
                "label": f"P{p} Button {b}",
                "player": str(p),
                "default": default,
            })

        start_labels = {1: "1 Player Start", 2: "2 Players Start",
                         3: "3 Players Start", 4: "4 Players Start"}
        actions.append({
            "token": f"START{p}",
            "name": f"Player {p} Start",
            "label": start_labels.get(p, f"{p} Player Start"),
            "player": str(p),
            "default": "start",
        })
        actions.append({
            "token": f"COIN{p}",
            "name": f"Player {p} Coin",
            "label": f"Coin {p}",
            "player": str(p),
            "default": "select",
        })

    actions.append({
        "token": "SERVICE",
        "name": "Test",
        "label": "Service Mode",
        "player": "1",
        "default": "rstick_click",
    })
    actions.append({
        "token": "SERVICE1",
        "name": "Service",
        "label": "Service 1",
        "player": "1",
        "default": "lstick_click",
    })

    return actions


def _format_actions(actions: List[Dict[str, str]]) -> str:
    lines = []
    for a in sorted(actions, key=lambda x: x["token"]):
        lines.append("    action\n")
        lines.append(f"      name: {a['name']}\n")
        lines.append(f"      label: {a['label']}\n")
        lines.append(f"      token: {a['token']}\n")
        lines.append(f"      player: {a['player']}\n")
        if a.get("default"):
            lines.append(f"      default: {a['default']}\n")
    return "".join(lines)


def _parse_game_input_meta(block: str) -> Tuple[int, int, str]:
    players_m = re.search(r"    players:\s+(\d+)", block)
    players = int(players_m[1]) if players_m else 2

    button_count = 0
    control_type = "joy"
    for m in re.finditer(
        r"    control\n((?:      \w+.*\n)*)", block
    ):
        ctrl = m[1]
        bt = re.search(r"      buttons:\s+(\d+)", ctrl)
        if bt:
            button_count = max(button_count, int(bt[1]))
        tp = re.search(r"      type:\s+(\w+)", ctrl)
        if tp:
            control_type = tp[1]

    return players, button_count, control_type


def _enrich_game_block(
    block: str,
    board_inputs: Dict[str, Dict[str, InputPorts]],
    game_filter: Optional[List[str]],
) -> str:
    name_m = re.search(r"  name:\s+(.+)", block)
    if not name_m:
        return block
    name = name_m[1].strip()

    if game_filter is not None and name not in game_filter:
        return block

    board_m = re.search(r"  board:\s+(.+)", block)
    if not board_m:
        return block
    board = board_m[1].strip()

    if "  input\n" not in block:
        return block

    type_m = re.search(r"  type:\s+(.+)", block)
    if type_m and type_m[1].strip() == "bios":
        return block

    players, button_count, control_type = _parse_game_input_meta(block)
    if button_count == 0:
        button_count = 4

    all_blocks = board_inputs.get(board, {})
    game_ports = all_blocks.get(name)
    if game_ports is None:
        parent_m = re.search(r"  parent:\s+(.+)", block)
        if parent_m:
            game_ports = all_blocks.get(parent_m[1].strip())

    if game_ports is not None:
        actions = _generate_actions_from_ports(game_ports, button_count)
        has_player_controls = any(a["token"].startswith("P1_") for a in actions)
        if not has_player_controls:
            std_actions = _generate_standard_actions(players, button_count, control_type)
            existing_tokens = {a["token"] for a in actions}
            for a in std_actions:
                if a["token"] not in existing_tokens:
                    actions.append(a)
            print(f"  {name}: {len(actions)} actions from driver source + control metadata", file=sys.stderr)
        else:
            print(f"  {name}: {len(actions)} actions from driver source", file=sys.stderr)
    else:
        actions = _generate_standard_actions(players, button_count, control_type)
        print(f"  {name}: {len(actions)} actions from control metadata", file=sys.stderr)

    out_lines = []
    skip_action = False
    for line in block.splitlines(keepends=True):
        if line.strip() == "action" and line.startswith("    "):
            skip_action = True
            continue
        if skip_action:
            if line.startswith("      "):
                continue
            skip_action = False
        out_lines.append(line)
    block = "".join(out_lines)

    action_text = _format_actions(actions)
    insert_pos = block.rfind("  input\n")
    if insert_pos >= 0:
        pos = insert_pos + len("  input\n")
        while pos < len(block):
            eol = block.find("\n", pos)
            if eol == -1:
                eol = len(block)
            line = block[pos:eol + 1]
            if line.startswith("    "):
                pos = eol + 1
            else:
                break
        block = block[:pos] + action_text + block[pos:]

    return block


def enrich(blocks: List[str], game_filter: Optional[List[str]] = None) -> List[str]:
    boards = set()
    for block in blocks:
        for match in re.finditer(r"  board:\s+(.+)", block):
            boards.add(match[1].strip())

    print(f"Fetching MAME source for {len(boards)} boards...", file=sys.stderr)
    ports = {board: fetch_ports(board) for board in sorted(boards)}
    return [_enrich_game_block(block, ports, game_filter) for block in blocks]
